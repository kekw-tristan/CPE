"""Hand-authored royal aviary. Export --phase blockout/decorated/lit, or audit.

Only engine-supported JSON is exported. Room names and route evidence live here,
not in the prefab schema. Geometry auditing reuses the existing content tool.
"""
import argparse
import collections
import copy
import html
import json
import math
import re
import sys
from pathlib import Path

sys.dont_write_bytecode = True

from mushroom_dungeon import Geometry, coplanar_overlaps, read, shape, write_json

ROOT = Path(__file__).resolve().parents[1]
MODELS = ROOT / "game/assets/models/cage_dungeon"
PREFAB = ROOT / "game/assets/prefabs/cage_dungeon.prefab.json"
PI = math.pi

# x, z, floor, width, depth, height, district. All connections are deliberate.
ROOMS = {
    "Ruined threshold": (0, -264, 0, 26, 24, 12, "ruins"),
    "Intake court": (0, -228, 0, 40, 28, 16, "prison"),
    "Chain gallery": (-62, -228, 0, 36, 20, 10, "prison"),
    "Holding cells": (-108, -228, 0, 24, 20, 9, "prison"),
    "Watch station": (-108, -190, 0, 24, 20, 10, "prison"),
    "Prison junction": (-62, -184, 0, 36, 36, 20, "prison"),
    "Inspection hall": (0, -184, 0, 30, 30, 12, "prison"),
    "Keeper common room": (56, -228, 0, 28, 28, 10, "keepers"),
    "Dormitory": (102, -228, 0, 28, 22, 9, "keepers"),
    "Seed store": (102, -188, 0, 24, 22, 9, "keepers"),
    "Feeding kitchen": (56, -184, 0, 28, 28, 10, "keepers"),
    "Lower aviary": (-86, -132, 0, 64, 44, 30, "aviary"),
    "Feeding hall": (78, -136, 0, 48, 36, 22, "feeding"),
    "Feed mill": (132, -136, 0, 24, 24, 12, "feeding"),
    "Infirmary": (132, -94, 0, 24, 22, 9, "keepers"),
    "Grand aviary": (0, -132, 0, 60, 48, 44, "royal"),
    "Nesting cathedral": (-108, -48, 8, 68, 56, 34, "nest"),
    "Egg reliquary": (-108, 0, 8, 24, 24, 16, "nest"),
    "West observatory": (-58, -48, 8, 20, 24, 18, "royal"),
    "East maintenance": (66, -66, 8, 24, 28, 16, "feeding"),
    "Royal overlook": (0, 64, 24, 32, 24, 14, "royal"),
}

# Boundary-to-boundary paths: endpoints align with real openings, not room centers.
# Each tuple: source, destination, XYZ polyline, width.
ROUTES = [
    ("Ruined threshold", "Intake court", [(0,0,-252),(0,0,-242)], 10),
    ("Intake court", "Chain gallery", [(-20,0,-228),(-44,0,-228)], 8),
    ("Chain gallery", "Holding cells", [(-80,0,-228),(-96,0,-228)], 7),
    ("Holding cells", "Watch station", [(-108,0,-218),(-108,0,-200)], 7),
    ("Watch station", "Prison junction", [(-96,0,-190),(-88,0,-190),(-88,0,-184),(-80,0,-184)], 7),
    ("Chain gallery", "Prison junction", [(-62,0,-218),(-62,0,-202)], 8),
    ("Intake court", "Inspection hall", [(0,0,-214),(0,0,-199)], 8),
    ("Prison junction", "Inspection hall", [(-44,0,-184),(-15,0,-184)], 8),
    ("Intake court", "Keeper common room", [(20,0,-228),(42,0,-228)], 8),
    ("Keeper common room", "Dormitory", [(70,0,-228),(88,0,-228)], 7),
    ("Dormitory", "Seed store", [(102,0,-217),(102,0,-199)], 7),
    ("Keeper common room", "Feeding kitchen", [(56,0,-214),(56,0,-198)], 8),
    ("Inspection hall", "Feeding kitchen", [(15,0,-184),(42,0,-184)], 8),
    ("Seed store", "Feeding kitchen", [(90,0,-188),(80,0,-188),(80,0,-184),(70,0,-184)], 7),
    ("Prison junction", "Lower aviary", [(-62,0,-166),(-62,0,-159),(-86,0,-159),(-86,0,-154)], 9),
    ("Feeding kitchen", "Feeding hall", [(56,0,-170),(56,0,-163),(78,0,-163),(78,0,-154)], 9),
    ("Lower aviary", "Grand aviary", [(-54,0,-132),(-30,0,-132)], 12),
    ("Feeding hall", "Grand aviary", [(54,0,-136),(44,0,-136),(44,0,-132),(30,0,-132)], 12),
    ("Feeding hall", "Feed mill", [(102,0,-136),(120,0,-136)], 8),
    ("Feed mill", "Infirmary", [(132,0,-124),(132,0,-105)], 7),
    ("Lower aviary", "Nesting cathedral", [(-86,0,-110),(-86,0,-106),(-108,0,-106),(-108,8,-76)], 10),
    ("Nesting cathedral", "Egg reliquary", [(-108,8,-20),(-108,8,-12)], 8),
    ("Nesting cathedral", "West observatory", [(-74,8,-48),(-68,8,-48)], 10),
    ("Feeding hall", "East maintenance", [(78,0,-118),(78,0,-112),(66,0,-112),(66,8,-80)], 10),
    ("West observatory", "Royal overlook", [(-58,8,-36),(-58,16,0),(-58,24,48),(-58,24,64),(-16,24,64)], 8),
    ("East maintenance", "Royal overlook", [(66,8,-52),(66,16,-16),(66,24,32),(66,24,64),(16,24,64)], 8),
    ("Grand aviary", "Cage floor", [(0,0,-108),(0,0,-28),(0,0,0)], 12),
    ("Royal overlook", "Cage north landing", [(0,24,52),(0,24,28)], 8),
]


def modules():
    palette = read(MODELS / "foundation.json")["materials"]
    parts = {
        "court_deck": [shape("Cube", (0,-.6,0), (24.16,1.2,24.16), 2),
                       shape("Cube", (0,-1.6,0), (23,1,23), 0)],
        "catwalk": [shape("Cube", (0,-.4,0), (8,.8,12), 2),
                    shape("Cube", (-3.6,-1,0), (.6,1.2,11.9), 1),
                    shape("Cube", (3.6,-1,0), (.6,1.2,11.9), 1)],
        "brace": [shape("Cube", (0,0,0), (1,1,.8), 1)],
        "landing": [shape("Cube", (0,-.47,0), (8,.94,8), 2)],
        "wall_panel": [shape("Cube", (0,1.94,0), (12,4.12,1), 2),
                       shape("Cube", (0,9.6,0), (12,.8,1.3), 1)]
                      + [shape("Cube", (x,6.5,0), (.35,5,.45), 0) for x in (-5,-2.5,0,2.5,5)],
        "stone_panel": [shape("Cube", (0,4.69,0), (12,9.62,1), 2),
                        shape("Cube", (0,9.6,0), (11.96,.8,1.3), 1)],
        "portal": [shape("Cube", (x,3.91,0), (1,8.18,1.4), 1) for x in (-4.5,4.5)]
                  + [shape("Cube", (0,8.54,0), (10,1.08,1.4), 1)],
        "parapet": [shape("Cube", (0,.7,0), (1,1.4,.45), 0),
                    shape("Cube", (0,1.45,0), (1,.1,.6), 1)],
        "buttress": [shape("Cube", (0,-38,0), (3,80,3), 2),
                     shape("Cube", (0,2,0), (3.9,1,3.9), 1)],
        "vault": [shape("Arch", (0,0,0), (24,20,8), 0)],
        "nest": [shape("Torus", (0,.45,0), (15,3,15), 2)]
                + [shape("Cube", (math.sin(a)*6,.3,math.cos(a)*6), (8,.55,.7), 0, (0,a+.4,.12))
                   for a in (0,1,2,3,4,5)]
                + [shape("Sphere", (x,1.5,z), (2.2,3,2.2), 3) for x,z in ((-2,0),(1,1))],
        "thorn_growth": [shape("Cylinder", (0,4,0), (.6,8,.6), 2, (0,0,.2)),
                         shape("Cone", (1,4.2,0), (1.4,4,1.4), 2, (0,0,-1.1)),
                         shape("Cone", (-.6,6.5,.2), (1,3,1), 2, (0,0,.9)),
                         shape("IcoSphere", (0,2,0), (4,1.2,3), 2)],
        "keeper_furniture": [shape("Cube", (-3,.7,0), (3,1.4,6), 0),
                             shape("Cube", (-3,1.5,0), (2.8,.2,5.7), 3),
                             shape("Cube", (3,1.8,0), (3,.35,4), 2)]
                            + [shape("Cube", (x,.85,z), (.35,1.7,.35), 1)
                               for x in (2,4) for z in (-1.5,1.5)],
        "feed_trough": [shape("Cube", (0,.5,0), (10,1,3), 2),
                        shape("Cube", (0,1.3,-1.3), (10,.6,.4), 1),
                        shape("Cube", (0,1.3,1.3), (10,.6,.4), 1)],
        "crown_floor": [shape("Cylinder", (0,-1,0), (40,2,40), 2),
                        shape("Frustum", (0,-3,0), (40,2,40), 1, (PI,0,0))],
    }
    # Rail-free ramp deck: slope is authored by rotating the whole catwalk.
    # Shared core pieces are kept small; no monolithic district model.
    for name, shapes in parts.items():
        write_json(MODELS / (name + ".json"), dict(name="cage_dungeon_"+name, materials=palette, shapes=shapes))
    lamp = read(MODELS / "lantern.json")
    lamp["name"] = "cage_dungeon_keeper_lantern"
    lamp["materials"][4]["albedo"] = [.95,.66,.28]
    lamp["materials"][4]["emissiveColor"] = [.95,.66,.28]
    lamp["lights"][0].update(name="keeper_amber", color=[1,.65,.28], radius=18)
    write_json(MODELS / "keeper_lantern.json", lamp)


class Aviary:
    def __init__(self):
        self.objects = []
        self.labels = []
        self.walks = []
        self.openings = collections.defaultdict(set)
        self.bridge_count = 0

    def put(self, asset, p, scale=(1,1,1), yaw=0, collision=False, light=False, rotation=None, label=""):
        obj = dict(asset="../models/cage_dungeon/"+asset+".json", position=list(p),
                   rotation=list(rotation or (0,yaw,0)), scale=list(scale),
                   generateColliders=collision, generateLights=light)
        self.objects.append(obj)
        self.labels.append(label or asset)

    def core_cage(self):
        # Read the stable original composition, stored as a source prefab so
        # phase exports are reproducible and do not accumulate decorations.
        core = read(ROOT / "scripts/cage_dungeon_core.json")
        for obj in core:
            obj = copy.deepcopy(obj)
            asset = Path(obj["asset"]).stem
            x,y,z = obj["position"]
            if asset == "perch":
                obj["position"][0] = 33 if x > 0 else -33
                obj["scale"][0] = .7
                obj["generateColliders"] = True
                if x > 0: obj["position"][1] = 6*24/11-9.25
                else: obj["position"][1],obj["position"][2] = 31.55,-14
            if asset == "lantern":
                obj["generateColliders"] = False
            if asset == "open_door":
                obj["position"][0] += -.06 if x > 0 else .06
                obj["position"][2] += .06
            # North wicket for the exterior balcony: preserve bars below and
            # above the opening, and preserve the uninterrupted royal bands.
            if asset == "bar" and z > 36 and abs(x) < 5 and y == 0:
                lower = copy.deepcopy(obj)
                lower["scale"][1] = 22/50
                self.objects.append(lower); self.labels.append("North wicket sill bar")
                obj["position"][1] = 29
                obj["scale"][1] = 21/50
            self.objects.append(obj); self.labels.append("Original " + asset)
        for x in (-24,24):
            for z in (-90,-56):
                self.walks.append(("Retained furnished side room",[(0,0,z),(0,0,z+2),(x,0,z+2),(x,0,z)]))

    def boundary(self, name, point):
        if name not in ROOMS:
            return
        x,z,y,w,d,h,district = ROOMS[name]
        px,py,pz = point
        side = min(("W","E","S","N"), key=lambda s: abs(px-(x-w/2)) if s=="W" else
                   abs(px-(x+w/2)) if s=="E" else abs(pz-(z-d/2)) if s=="S" else abs(pz-(z+d/2)))
        self.openings[name].add(side)

    def rooms(self):
        for a,b,points,width in ROUTES:
            self.boundary(a,points[0]); self.boundary(b,points[-1])
        self.openings["Ruined threshold"].add("S")
        self.openings["West observatory"].add("E")
        self.openings["East maintenance"].add("W")
        for name,(x,z,y,w,d,h,district) in ROOMS.items():
            self.put("court_deck", (x,y,z), (w/24,1,d/24), collision=True, label=name)
            self.walks.append((name, [(x,y,z)]))
            for side,length,cx,cz,yaw in (("S",w,x,z-d/2,0),("N",w,x,z+d/2,0),
                                         ("W",d,x-w/2,z,PI/2),("E",d,x+w/2,z,PI/2)):
                opened = side in self.openings[name]
                gap = 14 if opened else 0
                panel = "stone_panel" if district == "keepers" else "wall_panel"
                for sign in (-1,1):
                    span = (length-gap)/2
                    offset = sign*(gap/2+span/2)
                    px,pz = (cx+offset,cz) if yaw==0 else (cx,cz+offset)
                    self.put(panel,(px,y-(.04 if yaw else 0),pz),(span/12,h/10,1),yaw,True,label=name+" wall")
                if opened:
                    self.put("portal",(cx,y,cz),(1.5,1,1),yaw,True,label=name+" threshold")
            # Deep narrow piers reach the terrain; floors have a visible frame.
            for dx in (-w/2+2.8,w/2-2.8):
                for dz in (-d/2+2.8,d/2-2.8):
                    self.put("buttress",(x+dx,y-3,z+dz),collision=False)

    def bridge(self, a, b, width=8, rails=True, label="Bridge", end_gap=None):
        self.bridge_count += 1
        nominal_width = width
        # Slightly recessed alternate stringers keep overlapping angled joints
        # from sharing an exposed side plane. Walking centers stay unchanged.
        width -= .08*(self.bridge_count%2)
        dx,dy,dz = (b[i]-a[i] for i in range(3))
        run = math.hypot(dx,dz)
        assert run > 0 and abs(dy)/run <= .34, (label,a,b)
        yaw = math.atan2(dx,dz)
        pitch = -math.atan2(dy,run)
        # Rotated deck's local origin is its top plane. Add a tiny underlap at
        # seams; raised surfaces remain within the controller's .5 step limit.
        top = tuple((a[i]+b[i])/2 for i in range(3))
        if abs(dy) < 1e-8:
            top = (top[0],top[1]-.02*(self.bridge_count%5+1),top[2])
        self.put("catwalk",top,
                 (width/8,1,(math.hypot(run,dy)+.12)/12),collision=True,
                 rotation=(pitch,yaw,0),label=label)
        gap = nominal_width/2+1 if end_gap is None else end_gap
        if rails and run > 2*gap:
            # The rail's long axis is X; the deck's is Z. Convert the complete
            # basis to engine XYZ Euler order so rail tops follow both the slope
            # and yaw (adding a Z rotation after yaw would leave N/S rails flat).
            rail_rotation=(math.atan2(math.cos(yaw)*math.sin(pitch),math.sin(yaw)),
                           math.asin(max(-1,min(1,-math.cos(yaw)*math.cos(pitch)))),
                           math.atan2(-math.sin(pitch),math.sin(yaw)*math.cos(pitch)))
            for sign in (-1,1):
                # Rail follows the same incline as the deck.
                ox,oz = math.cos(yaw)*width/2*sign,-math.sin(yaw)*width/2*sign
                self.put("parapet",((a[0]+b[0])/2+ox,top[1],(a[2]+b[2])/2+oz),
                         (math.hypot(run,dy)-2*gap,1,1),collision=True,
                         rotation=rail_rotation,label=label+" rail")

    def routes(self):
        for source,target,points,width in ROUTES:
            label=source+" -> "+target
            # The retained approach already supplies this complete floor.
            if source != "Grand aviary":
                for a,b in zip(points,points[1:]):
                    self.bridge(a,b,width,label=label)
                for p in points[1:-1]:
                    self.put("landing",p,(.15,1,.15),collision=True,label=label+" turn")
                    self.put("buttress",(p[0],p[1]-3,p[2]),collision=False)
            # Center-to-threshold traces also detect furniture/wall blockers.
            walk = list(points)
            for name,start in ((source,True),(target,False)):
                if name in ROOMS:
                    x,z,y,*_=ROOMS[name]
                    if start: walk.insert(0,(x,y,z))
                    else: walk.append((x,y,z))
            self.walks.append((label,walk))

    def ascent(self):
        # One full outer revolution, followed by a tighter crown revolution.
        # Six broad resting platforms break up the climb and frame old views.
        points=[]
        for i in range(25):
            a=PI-i*PI/12
            height = min(24, i*24/11) if i <= 13 else min(48,24+(i-13)*2.4)
            points.append((28*math.sin(a),height,-28*math.cos(a)))
        # Start at south, reach north at y24, south again at y48.
        points=[(p[0],p[1],-p[2]) for p in points]
        for i,(a,b) in enumerate(zip(points,points[1:])):
            self.bridge(a,b,6,i not in (0,10,11,12,13,19,20),"Cage ascent",end_gap=.9)
            mx,mz=(a[0]+b[0])/2,(a[2]+b[2])/2
            r=math.hypot(mx,mz)
            self.put("brace",(mx*(r+5)/r,(a[1]+b[1])/2-1.1,mz*(r+5)/r),
                     (11,1,1),-math.atan2(mz,mx))
            if i > 0:
                self.put("landing",a,(.15,1,.15),collision=True,label="Ascent landing")
        self.put("landing",points[-1],(.15,1,.15),collision=True)
        self.walks.append(("Lower cage revolution",points))
        for point,top in ((points[6],(33,points[6][1],0)),(points[20],(-33,40.8,-14))):
            self.bridge(point,top,2.4,False,"Perch access")
            self.walks.append(("Walkable royal perch",[point,top,(top[0],top[1],top[2]+5)]))
        inner=[]
        for i in range(25):
            a=PI-i*PI/12
            inner.append((25*math.sin(a),48+i*2/3,25*math.cos(a)))
        self.bridge(points[-1],inner[0],6,False,"Crown transfer")
        for a,b in zip(inner,inner[1:]):
            self.bridge(a,b,6,True,"Crown ascent",end_gap=.9)
            self.put("landing",a,(.15,1,.15),collision=True,label="Crown landing")
        self.put("landing",inner[-1],(.15,1,.15),collision=True)
        self.bridge(inner[-1],(0,64,-18),8,False,"Boss threshold")
        self.put("crown_floor",(0,64,0),collision=True)
        for x in (-12.7,12.7):
            for z in (-12.7,12.7):
                self.put("bar",(x,62,z),(.65,14/50,.65))
        self.walks.extend([("Crown revolution",[points[-1]]+inner+[(0,64,-18),(0,64,0)]),
                           ("Cage entry to ascent",[(0,0,0),points[0]])])
        # Safe outer rails on spiral runs only; open landings retain the turns.
        for path in (points,inner):
            for a,b in zip(path,path[1:]):
                mid=((a[0]+b[0])/2,(a[2]+b[2])/2)
                radius=math.hypot(*mid)
                # Discrete curb uprights establish silhouette without putting
                # cross rails through the next sloping walking surface.
                for p in (() if abs(a[0]) < 1 or abs(a[2]) < 1 else (a,)):
                    self.put("bar",(p[0]*(radius+3.2)/radius,p[1],p[2]*(radius+3.2)/radius),
                             (.55,.04,.55),collision=True,label="Ascent edge post")
        # Grand hall upper crossing: later traversal overlooks its combat floor.
        upper=[(-48,8,-48),(-42,8,-48),(-42,16,-90),(-42,24,-126),(-42,24,-132),(-24,24,-132),(24,24,-132),
               (44,24,-132),(44,24,-126),(44,16,-90),(44,8,-48),(36,8,-48),(36,8,-76),(50,8,-76),(50,8,-66),(54,8,-66)]
        for a,b in zip(upper,upper[1:]): self.bridge(a,b,8,label="Grand upper crossing")
        for p in upper[1:-1]:
            self.put("landing",p,(.15,1,.15),collision=True)
            self.put("buttress",(p[0],p[1]-3,p[2]+3),(.65,1,.65))
        self.walks.append(("Grand upper crossing",[(-58,8,-48)]+upper+[(66,8,-66)]))
        nesting=[(-132,8,-68),(-132,20,-30),(-132,20,-24),(-84,20,-24),(-84,20,-30),(-84,8,-68)]
        for a,b in zip(nesting,nesting[1:]): self.bridge(a,b,6,label="Nesting observation gallery")
        for p in nesting[1:-1]:
            self.put("landing",p,(.15,1,.15),collision=True)
            self.put("bar",(p[0],8,p[2]),(1.4,(p[1]-8-.2)/50,1.4))
        self.walks.append(("Nesting observation loop",[(-108,8,-48),(-108,8,-68)]+nesting+[(-108,8,-68),(-108,8,-48)]))


    def decorate(self):
        # Ribbed silhouettes and hanging perches keep the large volumes open.
        for name in ("Lower aviary","Grand aviary","Nesting cathedral","Feeding hall"):
            x,z,y,w,d,h,_=ROOMS[name]
            for dz in (-d*.3,d*.3):
                self.put("vault",(x,y+h-10,z+dz),(w/24,1,1))
            self.put("perch",(x,y+h-19,z+5),(.8,1,1))
            for dx in (-9.6,9.6):
                self.put("bar",(x+dx,y+h-19,z+5),(.45,19/50,.45))
            for dx in (-w/2+2,w/2-2):
                for dz in (-d*.3,d*.3):
                    self.put("bar",(x+dx,y,z+dz),(2,h/50,2),collision=True)
        # Resting-platform hangers are attached to the preserved dome/bands.
        for i in (4,8,16,20):
            a=PI-i*PI/12
            x,z=32*math.sin(a),32*math.cos(a)
            self.put("bar",(x,i*2,z),(.6,(52-i*2)/50,.6))
        # Keep the boss center clear. The rim leaves its south entry open.
        for i in range(24):
            a=i*PI/12
            if 10<=i<=14: continue
            p=(19*math.sin(a),64,19*math.cos(a))
            self.put("parapet",p,(5,1+.04*(i%2),1),a,True)
        for name in ("Keeper common room","Dormitory","Seed store","Feeding kitchen","Infirmary"):
            x,z,y,w,d,*_=ROOMS[name]
            self.put("keeper_furniture",(x-w*.27,y,z+d*.28),(.7,1,.65),collision=True)
        for p in ((68,0,-125),(89,0,-145),(139,0,-130)):
            self.put("feed_trough",p,collision=True)
        for p,s,yaw in (((-123,8,-34),1.1,.4),((-93,8,-34),.85,-.5),((-114,8,5),.55,.8),
                        ((-103,0,-123),.8,0)):
            self.put("nest",p,(s,s,s),yaw)
        for p in ((-136,8,-30),(-139,8,-69),(-118,8,8),(-115,0,-118),(-12,0,-270),(140,0,-99)):
            self.put("thorn_growth",p)
        for p,yaw,scale in (((0,0,-274),0,(.65,.5,.8)),((0,0,-244),0,(.7,.6,1)),
                            ((-86,0,-152.5),0,(.7,.7,1)),((0,0,-108),0,(1,1,1))):
            self.put("gate",p,scale,yaw,True)
        # Smaller holding pens, with side walls and genuinely open cell doors.
        for x in (-116,-100):
            self.put("open_door",(x,0,-224),(.38,.24,.7),PI/2,True)
        for p,yaw in (((-10,0,-260),-.2),((11,0,-247),.35),((-65,0,-234),PI/2),
                      ((-16,24,67),0),((15,24,68),0)):
            self.put("feather",p,(.65,.65,.65),yaw)

    def lighting(self):
        geometry=Geometry(self.objects,self.labels)
        def lamp(asset,point,scale):
            x,y,z=point
            floor=geometry.support(x,z,y)
            assert floor is not None,("Unsupported light",point)
            self.put(asset,(x,floor,z),(scale,scale,scale),light=True)

        cyan=[(-8,0,-270),(8,0,-239),(-53,0,-232),(-55,0,-172),(-5,0,-179),
              (-102,0,-193),(-92,0,-150),(-54,0,-130),(-104,0,-109),(-114,8,-73),
              (-94,8,-35),(-54,8,-43),(-55,16,0),(-52,24,62),(7,24,55),
              (61,24,62),(63,16,-16),(60,8,-72),(-21,0,-114),(21,0,-114),
              (-23,24,-129),(23,24,-129),(16.454483,65.5,9.5),(-16.454483,65.5,9.5)]
        amber=[(49,0,-235),(107,0,-233),(108,0,-183),(49,0,-176),(87,0,-128),
               (139,0,-141),(136,0,-99)]
        for asset,positions in (("lantern",cyan),("keeper_lantern",amber)):
            for p in positions: lamp(asset,p,.65)
        for i in (4,10,16,22):
            a=PI-i*PI/12
            lamp("lantern",(26.2*math.sin(a),min(24,i*24/11) if i<=13 else min(48,24+(i-13)*2.4),26.2*math.cos(a)),.35)


def build(phase):
    level=Aviary()
    level.core_cage(); level.rooms(); level.routes(); level.ascent()
    if phase in ("decorated","lit"): level.decorate()
    if phase=="lit": level.lighting()
    return level


def validate(level):
    data=read(PREFAB)
    fields={"asset","position","rotation","scale","generateColliders","generateLights"}
    assert set(data)=={"assetType","name","objects"} and data["assetType"]=="Prefab"
    models={}
    for obj in data["objects"]:
        assert set(obj)==fields
        path=(PREFAB.parent/obj["asset"]).resolve()
        assert path.is_file(),path
        models[path]=read(path)
        for f in ("position","rotation","scale"):
            assert len(obj[f])==3 and all(math.isfinite(v) for v in obj[f])
        assert min(obj["scale"])>0
        assert type(obj["generateColliders"]) is bool and type(obj["generateLights"]) is bool
        if obj["generateLights"]: assert models[path].get("lights"),path
    for path in MODELS.glob("*.json"):
        model=read(path)
        assert set(model)<={"name","materials","shapes","lights"},path
        assert b"\n" not in path.read_bytes().replace(b"\r\n",b""),path
        for material in model["materials"]:
            assert set(material)=={"albedo","roughness","metallic","ambientStrength","lightWrap",
                                   "shapeContrast","emissiveColor","emissiveStrength"},path
            for value in material.values():
                assert all(math.isfinite(v) for v in (value if isinstance(value,list) else [value])),path
        for s in model["shapes"]:
            assert set(s)=={"meshType","position","rotation","scale","color","materialIndex"},path
            assert 0<=s["materialIndex"]<len(model["materials"]),path
            assert s["meshType"] in {"Cube","Cylinder","Sphere","Torus","Cone","Arch","Frustum","IcoSphere"},path
            for field,size in (("position",3),("rotation",3),("scale",3),("color",4)):
                assert len(s[field])==size and all(math.isfinite(v) for v in s[field]),path
            assert min(s["scale"])>0,path
        for light in model.get("lights",[]):
            assert set(light)=={"name","type","position","color","intensity","radius","castsShadow"},path
            assert light["type"]=="Point" and light["radius"]>0 and light["intensity"]>=0,path
            assert type(light["castsShadow"]) is bool,path
    assert b"\n" not in PREFAB.read_bytes().replace(b"\r\n",b""),PREFAB
    geo=Geometry(data["objects"],level.labels)
    visual=Geometry(data["objects"],level.labels,decorative=True)
    # Check exposed, same-facing surfaces, including joints inside reused models.
    # Buried intersections and opposing faces at ordinary butt joints are safe.
    overlaps=coplanar_overlaps(data["objects"],geo)
    assert not overlaps, [(level.labels[a[0]],a,level.labels[b[0]],b,area)
                          for a,b,area in overlaps[:10]]
    for obj in data["objects"]:
        if obj["generateLights"] or Path(obj["asset"]).stem in {"keeper_furniture","feed_trough","nest","thorn_growth"}:
            x,y,z=obj["position"]
            assert geo.support(x,z,y) is not None,("Floating fixture",obj)
    # Audit the actual C++ placements, avoiding a second list that can drift.
    source=(ROOT/"game/src/world/biome/forestGenerator.cpp").read_text()
    positions=source.split("c_guardPositions[] =")[1].split("};")[0]
    guards=re.findall(r"\{\s*([-\d.]+)f,\s*([-\d.]+)f,\s*([-\d.]+)f\s*\}",positions)
    assert len(guards)==30
    for position in guards:
        x,y,z=map(float,position)
        assert geo.support(x,z,y) is not None and not geo.blocked(x,y,z),position
        assert not visual.blocked(x,y,z),position
    # Clear dodge disc and bounded boss movement, with enough overhead room.
    for radius in (0,6,12,16):
        for i in range(32):
            x,z=radius*math.sin(i*PI/16),radius*math.cos(i*PI/16)
            assert geo.support(x,z,64) is not None and not geo.blocked(x,64,z),(x,z)
            assert not visual.blocked(x,64,z),(x,z)
    bounds=[[min(p[2][i] for p in geo.parts),max(p[3][i] for p in geo.parts)] for i in range(3)]
    assert bounds[0][0]>=-150 and bounds[0][1]<=150 and bounds[2][0]>=-277 and bounds[2][1]<=80,bounds
    # floor((owner + offset)/64+.5)-floor(owner/64+.5) is at most
    # ceil(abs(offset)/64), including owners at chunk boundaries.
    assert math.ceil(max(316,*(abs(v) for i in (0,2) for v in bounds[i]))/64)<=5
    # Conservative independent jitter envelopes also protect other dungeons.
    def envelope(index,radius,xmin,xmax,zmin,zmax):
        angles=[.785398+index*1.570796+jitter for jitter in (-.16,.16)]
        xs=[math.cos(a)*radius for a in angles]; zs=[math.sin(a)*radius for a in angles]
        return min(xs)+xmin,max(xs)+xmax,min(zs)+zmin,max(zs)+zmax
    cage=envelope(0,520,-150,150,-316,80)
    reservations=[(-90,90,-56,124),envelope(1,190,-52,52,-164,44),
                  envelope(2,190,-52,52,-164,44),envelope(3,450,-224,224,-216,232)]
    for other in reservations:
        assert cage[1]<=other[0] or cage[0]>=other[1] or cage[3]<=other[2] or cage[2]>=other[3],other
    errors=[]; samples=0
    for name,points in level.walks:
        failures=[]
        for a,b in list(zip(points,points[1:])) or [(points[0],points[0])]:
            count=max(1,math.ceil(math.dist(a,b)/.4))
            previous=None
            for i in range(count+1):
                p=[a[k]+(b[k]-a[k])*i/count for k in range(3)]
                x,y,z=p; samples+=1
                sy=geo.support(x,z,y)
                if sy is None:
                    failures.append(f"no floor {tuple(round(v,2) for v in p)}"); continue
                blocked=geo.blocked(x,sy,z) or visual.blocked(x,sy,z)
                if blocked: failures.append(f"blocked {tuple(round(v,2) for v in p)}: {blocked}")
                if previous is not None and abs(sy-previous)>.501: failures.append("step > .5")
                previous=sy
                for ox,oz in ((.4,0),(-.4,0),(0,.4),(0,-.4)):
                    if geo.support(x+ox,z+oz,sy) is None: failures.append("unsupported capsule footprint")
        if failures: errors.append(name+": "+"; ".join(failures[:3])+f" ({len(failures)} failures)")
    graph=collections.defaultdict(set)
    for a,b,_,_ in ROUTES: graph[a].add(b); graph[b].add(a)
    graph["Cage floor"].add("Cage north landing"); graph["Cage north landing"].add("Cage floor")
    reached=set(); todo=["Ruined threshold"]
    while todo:
        n=todo.pop()
        if n not in reached: reached.add(n); todo.extend(graph[n]-reached)
    assert reached==set(graph)
    loops=sum(map(len,graph.values()))//2-len(graph)+1
    assert loops>=3
    for i,(name,r) in enumerate(ROOMS.items()):
        for other,s in list(ROOMS.items())[i+1:]:
            assert abs(r[0]-s[0])>=(r[3]+s[3])/2 or abs(r[1]-s[1])>=(r[4]+s[4])/2,(name,other)
    print(f"{len(models)} models; {len(data['objects'])} objects; {len(ROOMS)} new rooms; {loops} loops; {samples} traversal samples")
    for e in errors: print(e)
    assert not errors, f"{len(errors)} route audits failed"
    print("Schema, references, exposed surface overlap, room separation, route support, capsule clearance, fixtures, guards, boss floor and world envelope passed.")


def draw_map(level):
    colors={"ruins":"#829383","prison":"#6a9297","keepers":"#c19458","aviary":"#729b69",
            "feeding":"#a79b70","royal":"#c6af5e","nest":"#668745"}
    lines=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="-175 -300 350 410">',
           '<rect x="-175" y="-300" width="350" height="410" fill="#101a20"/>']
    for a,b,ps,w in ROUTES:
        lines.append(f'<polyline points="{" ".join(f"{p[0]},{p[2]}" for p in ps)}" fill="none" stroke="#a7b5ad" stroke-width="{w}"/>')
    for name,(x,z,y,w,d,h,district) in ROOMS.items():
        lines.append(f'<rect x="{x-w/2}" y="{z-d/2}" width="{w}" height="{d}" fill="{colors[district]}" stroke="#e4deba" stroke-width=".6"/>')
        lines.append(f'<text x="{x}" y="{z}" text-anchor="middle" font-size="3.3" fill="#08131b">{html.escape(name)}<tspan x="{x}" dy="4">+{y}</tspan></text>')
    lines.extend(['<rect x="-32" y="-108" width="64" height="78" fill="#556d70"/>',
                  '<circle cx="0" cy="0" r="38" fill="#344e51" stroke="#d8bd68" stroke-width="2"/>',
                  '<circle cx="0" cy="0" r="20" fill="#8c793e"/>',
                  '<text x="0" y="0" text-anchor="middle" font-size="4" fill="white">Dornvogel +64</text>',
                  '<text x="0" y="97" text-anchor="middle" font-size="5" fill="white">ROYAL AVIARY | plan, heights in local units</text>','</svg>'])
    for name,points in level.walks:
        if name in {"Grand upper crossing","Nesting observation loop","Lower cage revolution","Crown revolution"}:
            lines.insert(-1,f'<polyline points="{" ".join(f"{p[0]},{p[2]}" for p in points)}" '
                         'fill="none" stroke="#f4dea1" stroke-width="1.1" stroke-dasharray="2 1"/>')
    (ROOT/"scripts/cage_dungeon_plan.svg").write_text("\n".join(lines),encoding="utf-8")


if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--phase",choices=("blockout","decorated","lit"))
    args=parser.parse_args()
    if args.phase: modules()
    level=build(args.phase or "lit")
    if args.phase:
        write_json(PREFAB,dict(assetType="Prefab",name="Die gruene Voliere - Kaefig des Dornvogels",objects=level.objects))
        draw_map(level)
    validate(level)
