"""Herzholz: fixed, authored world-tree layout; exports ordinary prefab/model JSON.

No runtime generation. Names, route graph and audit evidence stay outside assets.
Run --phase blockout, --phase lit, or --audit (read-only geometry validation).
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
from mushroom_dungeon import Geometry, read, shape, transform, write_json

ROOT = Path(__file__).resolve().parents[1]
MODELS = ROOT / 'game/assets/models/tree_dungeon'
PREFAB = ROOT / 'game/assets/prefabs/tree_dungeon.prefab.json'
PI = math.pi

# XYZ floor centers, usable radius, district. The old tree stays at XZ=(0,0).
ROOMS = {
    'Root Gate': ((0, 0, -100), 9, 'approach'),
    'Root Nexus': ((0, 0, -28), 18, 'root'),
    'Catacombs': ((-64, 0, -28), 12, 'root'),
    'Root Arena': ((-110, 0, 12), 23, 'root'),
    'Dead Root': ((-164, 0, 12), 11, 'rot'),
    'Ossuary': ((-140, 8, 72), 11, 'root'),
    'Underroot Gallery': ((-58, 8, 58), 15, 'root'),
    'Underroot Cavern': ((0, 8, 28), 24, 'root'),
    'Sap Reservoir': ((20, 16, 90), 21, 'sap'),
    'Root Shrine': ((64, 16, 118), 11, 'sap'),
    'Root Return': ((72, 24, 24), 13, 'root'),
    'Lower Gate': ((0, 40, -68), 16, 'settlement'),
    'Lower Hollow': ((0, 48, 0), 35, 'hollow'),
    'Heartwood Settlement': ((-66, 60, 0), 18, 'settlement'),
    'Workshop': ((-110, 60, -18), 14, 'settlement'),
    'Sleeping Bough': ((-106, 66, -65), 12, 'settlement'),
    'Rot Chamber': ((-124, 84, 60), 23, 'rot'),
    'Silent Graft': ((-167, 84, 100), 10, 'rot'),
    'Sapworks': ((70, 84, 0), 19, 'sap'),
    'Grand Branch Hall': ((0, 96, 82), 24, 'branch'),
    'Split Bough': ((88, 112, 96), 14, 'branch'),
    'Branch Arena': ((150, 128, 38), 23, 'branch'),
    'Seed Archive': ((161, 144, 111), 11, 'upper'),
    'Windward Bough': ((-88, 152, -86), 17, 'branch'),
    'Upper Heartwood': ((-144, 168, -18), 18, 'upper'),
    'Listening Shrine': ((-79, 168, 67), 12, 'upper'),
    'Canopy West': ((-68, 200, -65), 14, 'canopy'),
    'Canopy Overlook': ((-94, 208, 0), 13, 'canopy'),
    'Canopy East': ((92, 200, 15), 17, 'canopy'),
    'Canopy Sanctuary': ((0, 216, 118), 23, 'canopy'),
    'Crown Threshold': ((0, 224, 60), 7, 'heart'),
    'Ancient Heart': ((0, 224, 0), 35, 'heart'),
}
for i in range(25):
    angle = PI + i * PI / 4
    ROOMS[f'Inner {i:02}'] = ((round(30.5 * math.sin(angle), 6), 48 + i * 6,
                             round(30.5 * math.cos(angle), 6)), 4.5, 'inner')

# Deliberately authored connections; intermediate points have small turning knots.
# Source, destination, intermediate XYZ points, clear tread width.
ROUTES = [
    ('Root Gate', 'Root Nexus', [], 12),
    ('Root Nexus', 'Catacombs', [], 8),
    ('Catacombs', 'Root Arena', [], 8),
    ('Root Arena', 'Dead Root', [], 7),
    ('Root Arena', 'Ossuary', [], 7),
    ('Ossuary', 'Underroot Gallery', [], 8),
    ('Root Arena', 'Underroot Gallery', [], 9),
    ('Underroot Gallery', 'Underroot Cavern', [], 9),
    ('Underroot Cavern', 'Root Nexus', [], 9),
    ('Underroot Gallery', 'Sap Reservoir', [], 9),
    ('Underroot Cavern', 'Sap Reservoir', [], 8),
    ('Sap Reservoir', 'Root Shrine', [], 7),
    ('Sap Reservoir', 'Root Return', [], 9),
    ('Root Return', 'Lower Gate', [(78, 32, -26), (62, 40, -68)], 10),
    ('Lower Hollow', 'Inner 00', [], 10),
    ('Inner 02', 'Heartwood Settlement', [], 9),
    ('Heartwood Settlement', 'Workshop', [], 8),
    ('Workshop', 'Sleeping Bough', [], 7),
    ('Workshop', 'Rot Chamber', [(-143, 72, 18)], 8),
    ('Rot Chamber', 'Silent Graft', [], 7),
    ('Rot Chamber', 'Inner 10', [(-74, 96, 34)], 8),
    ('Inner 06', 'Sapworks', [], 10),
    ('Sapworks', 'Split Bough', [(103, 96, 42)], 8),
    ('Inner 08', 'Grand Branch Hall', [(0, 96, -56), (58, 96, -56), (74, 96, 46)], 8),
    ('Grand Branch Hall', 'Split Bough', [], 10),
    ('Split Bough', 'Branch Arena', [], 10),
    ('Branch Arena', 'Inner 14', [], 10),
    ('Branch Arena', 'Seed Archive', [], 7),
    ('Inner 16', 'Windward Bough', [], 9),
    ('Windward Bough', 'Upper Heartwood', [], 9),
    ('Upper Heartwood', 'Listening Shrine', [], 9),
    ('Listening Shrine', 'Inner 20', [(0, 168, 62)], 8),
    ('Inner 24', 'Canopy West', [], 9),
    ('Canopy West', 'Canopy Overlook', [], 8),
    ('Canopy Overlook', 'Canopy Sanctuary', [], 9),
    ('Inner 22', 'Canopy East', [], 9),
    ('Canopy East', 'Canopy Sanctuary', [], 9),
    ('Canopy Sanctuary', 'Crown Threshold', [], 12),
    ('Crown Threshold', 'Ancient Heart', [], 12),
    # Diagonal living roots cross the hollow, bypassing half a spiral each.
    ('Inner 04', 'Inner 08', [], 8),
    ('Inner 12', 'Inner 16', [], 8),
]
ROUTES += [(f'Inner {i:02}', f'Inner {i+1:02}', [], 6.5) for i in range(24)]


def modules():
    palette = read(MODELS / 'foundation.json')['materials']
    def save(name, parts, lights=None):
        data = dict(name=name, materials=palette, shapes=parts)
        if lights:
            data['lights'] = lights
        write_json(MODELS / (name + '.json'), data)
    # These are normalized modules, with the walk surface at local Y=0.
    save('living_bridge', [shape('Cube', (0, -.5, 0), (1, 1, 1), 1),
         shape('Cylinder', (0, -2.2, 0), (1.05, 1, 3.8), 0, (PI/2,0,0)),
         shape('Cylinder', (0, -3.6, 0), (.55, 1, 2), 0, (PI/2,0,0)),
         shape('Cube', (-.56, .5, 0), (.12, 1.8, .65), 0),
         shape('Cube', (.56, .5, 0), (.12, 1.8, .65), 0)])
    save('cambium_shelf', [shape('Cylinder', (0, -.65, 0), (2, 1.3, 2), 1),
         shape('Cylinder', (0, -1.6, 0), (1.86, .6, 1.86), 0),
         shape('Frustum', (0, -3.4, 0), (1.3, 3, 1.3), 0)])
    # Unit root timber supports, also used as scar ribs on the outer bark.
    save('root_timber', [shape('Frustum', (0, 0, 0), (1, 1, 1), 0),
         shape('Cylinder', (.24, -.1, .22), (.22, .8, .22), 1)])
    save('bark_fold', [shape('Cylinder',(0,8,0),(6,16,4),0),
         shape('Cylinder',(-1.6,9,-1),(2,18,2),0),
         shape('Cylinder',(1.6,7,1),(1.6,14,2),1)])
    save('sap_vein', [shape('Cube', (0, 0, 0), (1, 1, 1), 4),
         shape('Cube', (-.8, 0, 0), (.3, 1.02, 1.3), 0),
         shape('Cube', (.8, 0, 0), (.3, 1.02, 1.3), 0)])
    save('heart_core', [shape('Crystal', (0, 0, 0), (9, 22, 9), 4),
         shape('Crystal', (-6, -3, 2), (3, 12, 4), 2),
         shape('Crystal', (6, 1, -2), (3, 15, 4), 4)],
         [dict(name='living_heart', type='Point', position=[0, -7, 0],
               color=[.63, .89, .3], intensity=5, radius=48, castsShadow=False)])
    # Reuse the actual old inhabitants' table, bench and storage geometry.
    furniture = copy.deepcopy(read(MODELS / 'furnished_rooms.json')['shapes'][33:65])
    for part in furniture:
        part['position'][0] += 24
        part['position'][1] -= .04
        part['position'][2] += 94
    save('inhabitant_furniture', furniture)
    save('sleeping_pod', [shape('Cube', (0, .4, 0), (3.5, .8, 6), 0),
         shape('Cube', (0, .86, 0), (3, .12, 5.6), 3),
         shape('Cube', (0, 1.1, 2), (2.8, .4, 1), 2)])


class Tree:
    def __init__(self):
        self.objects, self.labels, self.traces = [], [], []
        self.edges = []
        self.clearance_samples = None

    def clear(self, obj):
        if self.clearance_samples is None:
            self.clearance_samples=[]
            for _,points,width in self.traces:
                for a,b in zip(points,points[1:]):
                    length=math.hypot(b[0]-a[0],b[2]-a[2])
                    if length<.01: continue
                    count=max(1,math.ceil(length/.6))
                    for i in range(count+1):
                        t=i/count
                        for offset in (-2,0,2):
                            self.clearance_samples.append((a[0]+t*(b[0]-a[0])-(b[2]-a[2])*offset/length,
                                a[1]+t*(b[1]-a[1]),a[2]+t*(b[2]-a[2])+(b[0]-a[0])*offset/length))
        candidate=copy.deepcopy(obj)
        candidate['generateColliders']=False
        geo=Geometry([candidate],['candidate'],decorative=True)
        return not any(geo.blocked(*p) for p in self.clearance_samples)

    def place_clear(self, asset, positions, scale=(1,1,1), label='', light=False):
        for p in positions:
            self.put(asset,p,scale,label=label,light=light)
            if self.clear(self.objects[-1]): return
            self.objects.pop();self.labels.pop()
        raise AssertionError('No clear placement: '+label)

    def put(self, asset, p, scale=(1, 1, 1), yaw=0, collider=True,
            light=False, label='', pitch=0):
        self.objects.append(dict(asset='../models/tree_dungeon/' + asset + '.json',
            position=list(p), rotation=[pitch, yaw, 0], scale=list(scale),
            generateColliders=collider, generateLights=light))
        self.labels.append(label or asset)

    def shelf(self, p, r, label):
        self.put('cambium_shelf', p, (r, 1, r), label=label)

    def beam(self, a, b, width, label, depth=None):
        delta = [b[i] - a[i] for i in range(3)]
        length = math.dist(a, b)
        self.put('root_timber', [(a[i] + b[i]) / 2 for i in range(3)],
                 (width, length, depth or width), math.atan2(delta[0], delta[2]),
                 label=label, pitch=math.acos(delta[1] / length))

    def core(self):
        old = read(ROOT / 'scripts/tree_dungeon_core.json')['objects']
        for obj in old:
            obj = copy.deepcopy(obj)
            asset = Path(obj['asset']).stem
            # Foundation stays below Underroot. Gatehouses stay at the entrance.
            if asset in {'arena_floor', 'arena_rim', 'arena_plinth', 'staircase',
                         'bark_stave', 'bark_crown', 'branch', 'foliage'}:
                obj['position'][1] += 40
            if asset == 'lantern':
                continue  # Lighting is placed after the structural audit.
            if asset == 'root' and obj['position'][2] > -40:
                obj['position'][1] += 40
            if asset == 'arena_rim':
                continue  # Ledges need openings; retain the rim at the crown.
            if asset == 'branch':
                # Old rising limbs are silhouette supports, never advertised as paths.
                obj['generateColliders'] = False
                obj['position'][0] *= 1.3
                obj['position'][2] *= 1.3
                obj['scale'] = [1.4, 1.4, 1.4]
            if asset == 'foliage':
                obj['position'][0] *= 1.9
                obj['position'][2] *= 1.9
            self.objects.append(obj)
            self.labels.append('Original ' + asset)
        # Opening cuts are authored against route corridors after all routes exist.
        for tier, base in enumerate((104, 160)):
            for i in range(40):
                angle = i * PI / 20
                self.put('bark_stave', (40 * math.sin(angle), base, 40 * math.cos(angle)),
                         (1, .9, 1), angle, label='Upper trunk')
        for i in range(20):
            a = i * PI / 10
            self.put('bark_crown', (40 * math.sin(a), 160, 40 * math.cos(a)),
                     (1.2, .9 + .03 * (i % 3), 1.2), a, False, label='Crown scar')

    def floors(self):
        for name, (p, radius, district) in ROOMS.items():
            if name in {'Root Gate', 'Lower Hollow'}:
                continue
            if name == 'Ancient Heart':
                self.put('arena_floor', p, label=name)
                self.put('arena_rim', p, yaw=PI, label='Crown rim')
            else:
                self.shelf(p, radius, name)
        # Original staircase preserved: 32 treads, 0.25 rise, 24 run, 28 width.
        self.traces.append(('Old entrance stair', [(0, 40, -68), (0, 40, -64),
                            (0, 48, -40), (0, 48, -30), (0, 48, 0)], 8))
        self.edges.append(('Lower Gate', 'Lower Hollow'))

    def routes(self):
        for source, target, bends, width in ROUTES:
            start, sr, _ = ROOMS[source]
            end, er, _ = ROOMS[target]
            points = [start, *bends, end]
            radii = [sr, *[max(width * .7, 4.5) for _ in bends], er]
            label = source + ' -> ' + target
            self.edges.append((source, target))
            trace = [start]
            for p, r in zip(bends, radii[1:-1]):
                self.shelf(p, r, label + ' knot')
            for i, (a, b) in enumerate(zip(points, points[1:])):
                length = math.hypot(b[0] - a[0], b[2] - a[2])
                dx, dz = (b[0] - a[0]) / length, (b[2] - a[2]) / length
                # Flat room interiors terminate at exact ramp thresholds. Allow
                # only .12 overlap at a height transition (under 0.1 step).
                trim_a = radii[i] - .15
                trim_b = radii[i + 1] - .15
                if trim_a + trim_b >= length and a[1] == b[1]:
                    trace.append(b)
                    continue
                aa = (a[0] + dx * trim_a, a[1], a[2] + dz * trim_a)
                bb = (b[0] - dx * trim_b, b[1], b[2] - dz * trim_b)
                run = math.hypot(bb[0] - aa[0], bb[2] - aa[2])
                rise = bb[1] - aa[1]
                assert abs(rise) / run < .85, (label, rise, run)
                trace.extend([aa, bb, b])
                center = [(aa[j] + bb[j]) / 2 for j in range(3)]
                self.put('living_bridge', center, (width, 1, math.hypot(run, rise)),
                         math.atan2(dx, dz), label=label,
                         pitch=-math.atan2(rise, run))
            self.traces.append((label, trace, min(width - 1, 5)))

    def cut_trunk(self):
        # Split only those staves penetrated by authored routes. Keep the root
        # and crown above/below each opening, preserving the 40-stave silhouette.
        objects, labels = self.objects, self.labels
        self.objects, self.labels = [], []
        for obj, label in zip(objects, labels):
            if Path(obj['asset']).stem != 'bark_stave':
                self.objects.append(obj); self.labels.append(label)
                continue
            x, y, z = obj['position']
            top = y + obj['scale'][1] * 64
            cuts = []
            for _, points, width in self.traces:
                for a, b in zip(points, points[1:]):
                    dx, dz = b[0] - a[0], b[2] - a[2]
                    ll = dx * dx + dz * dz
                    if ll < .01:
                        continue
                    t = max(0, min(1, ((x-a[0])*dx + (z-a[2])*dz) / ll))
                    if math.hypot(x-a[0]-t*dx, z-a[2]-t*dz) < width / 2 + 7:
                        h = a[1] + t*(b[1]-a[1])
                        cuts.append((h - 4, h + 11))
            intervals = [(y, top)]
            for low, high in cuts:
                intervals = [(u, v) for a, b in intervals
                             for u, v in ((a, min(b, low)), (max(a, high), b)) if v-u > 1]
            for a, b in intervals:
                part = copy.deepcopy(obj)
                part['position'][1] = a
                part['scale'][1] = (b - a) / 64
                self.objects.append(part); self.labels.append(label)

        # Prune the rounded bark caps at new doorways as well as the staves.
        kept = []
        for obj,label in zip(self.objects,self.labels):
            if Path(obj['asset']).stem == 'bark_crown':
                p=transform((0,64,0),obj)
                blocked=False
                for _,points,width in self.traces:
                    for a,b in zip(points,points[1:]):
                        delta=[b[i]-a[i] for i in range(3)]
                        length=sum(v*v for v in delta)
                        if length < .01: continue
                        t=max(0,min(1,sum((p[i]-a[i])*delta[i] for i in range(3))/length))
                        q=[a[i]+t*delta[i] for i in range(3)]
                        if math.hypot(p[0]-q[0],p[2]-q[2]) < width/2+4 and abs(p[1]-q[1])<7:
                            blocked=True
                if blocked: continue
            kept.append((obj,label))
        self.objects=[o for o,_ in kept]; self.labels=[s for _,s in kept]

    def architecture(self):
        # Caverns are beneath a raised root mantle, not buried in solid terrain.
        for name in ['Catacombs', 'Root Arena', 'Underroot Gallery', 'Underroot Cavern',
                     'Sap Reservoir', 'Dead Root', 'Ossuary']:
            p, r, _ = ROOMS[name]
            x, y, z = p
            height = 30 if name in {'Root Arena', 'Underroot Cavern'} else 15
            self.shelf((x, y + height, z), r + 3, name + ' root mantle')
        for name, (p, r, district) in ROOMS.items():
            x, y, z = p
            if name.startswith('Inner'):
                # Embedded root corbels visibly meet the trunk's inner wall.
                length = math.hypot(x, z)
                self.beam((x * 40 / length, y - 9, z * 40 / length),
                          (x, y - 2, z), 2.4, name + ' corbel')
                continue
            if name in {'Root Gate', 'Lower Hollow', 'Ancient Heart', 'Crown Threshold'}:
                continue
            # Three grown pillars, outside circulation. Door lanes stay clear.
            placed=0
            for k in range(16):
                angle=.3+k*PI/8
                px, pz = x + (r+1.8)*math.sin(angle), z + (r+1.8)*math.cos(angle)
                h = 30 if name in {'Root Arena','Underroot Cavern'} else (21 if district in {'root', 'rot'} else 14)
                self.beam((px, y-5, pz), (px+2*math.sin(angle), y+h, pz+2*math.cos(angle)),
                          3.2 if district == 'root' else 2.2, name + ' grown rib')
                if not self.clear(self.objects[-1]):
                    self.objects.pop();self.labels.pop()
                else:
                    placed+=1
                if placed==3: break
            # Load-bearing roots reach ground or the central trunk, not thin air.
            if district in {'root', 'rot'} and y < 30:
                self.beam((x, -35, z), (x, y - 3, z), r * .65, name + ' taproot')
            elif y > 35:
                radial = max(math.hypot(x, z), 1)
                anchor = (x * 39 / radial, max(0, y - radial*.32), z * 39 / radial)
                # Route the load-bearing bough below intersecting branch paths.
                for thickness,drop in [(th,d) for th in (max(8,r*.62),max(4,r*.32)) for d in (4,10,18,28)]:
                    self.beam((anchor[0],anchor[1]-drop,anchor[2]),
                              (x,y-drop,z),thickness,name+' bearing bough')
                    if self.clear(self.objects[-1]):
                        if drop>4:
                            self.beam((x,y-drop,z),(x,y-4,z),4,name+' bough knot')
                        break
                    self.objects.pop();self.labels.pop()
                else: raise AssertionError('No support clearance: '+name)
            # Partial bark crescents grow around each chamber. Openings follow
            # the actual approach corridors, leaving combat centers uncluttered.
            if district in {'root','rot','settlement','upper','sap'}:
                for k in range(12):
                    angle=k*PI/6
                    if k%4==0: continue
                    self.put('bark_fold',(x+(r+1)*math.sin(angle),y,z+(r+1)*math.cos(angle)),
                             (r*.085, (.8 if district!='root' else 1.15), .8),angle,
                             label=name+' bark recess')
                    if not self.clear(self.objects[-1]):
                        self.objects.pop();self.labels.pop()
        # Crown floor rests in the tree's fork. Core is suspended above combat.
        for angle in (.4, 2, 3.7, 5.2):
            self.beam((35*math.sin(angle), 198, 35*math.cos(angle)),
                      (24*math.sin(angle), 222, 24*math.cos(angle)), 5, 'Crown fork')
        # Interwoven root vaults make the catacombs actual constricted passages.
        for a,b in [('Root Nexus','Catacombs'),('Catacombs','Root Arena'),
                    ('Root Arena','Dead Root'),('Underroot Cavern','Sap Reservoir')]:
            pa,pb=ROOMS[a][0],ROOMS[b][0]
            yaw=math.atan2(pb[0]-pa[0],pb[2]-pa[2])
            for t in (.42,.58):
                p=[pa[i]+t*(pb[i]-pa[i]) for i in range(3)]
                self.put('wooden_gate',p,(.45,.35,.7),yaw,label='Grown root vault')
                if not self.clear(self.objects[-1]):
                    self.objects.pop();self.labels.pop()
        # Roots frame the original entrance and the cavern perimeter.
        for p, yaw, scale in [((-43,0,-60),-1.1,(2,2,2)), ((42,0,-85),1.1,(2,2,2)),
                              ((-102,0,43),2.7,(2.4,1.8,2)), ((42,0,55),.8,(2,2.4,2)),
                              ((-45,0,10),4.1,(1.6,1.8,2))]:
            self.put('root', p, scale, yaw, label='Exposed giant root')
            if not self.clear(self.objects[-1]):
                self.objects[-1]['position'][2]-=28
                if not self.clear(self.objects[-1]):
                    self.objects.pop();self.labels.pop()

    def details(self):
        for name in ['Heartwood Settlement', 'Workshop', 'Upper Heartwood']:
            (x,y,z), r, _ = ROOMS[name]
            size=.65 if r<16 else 1
            positions=[(x+math.sin(k*PI/8)*(r-8*size),y,z+math.cos(k*PI/8)*(r-8*size)) for k in range(16)]
            self.place_clear('inhabitant_furniture',positions,(size,)*3,label=name+' inhabitants')
        for x in (-111, -102):
            self.put('sleeping_pod', (x,66,-71), label='Sleeping hollow')
        for name in ['Sap Reservoir', 'Sapworks', 'Root Shrine', 'Canopy Sanctuary']:
            (x,y,z), r, _ = ROOMS[name]
            self.put('sap_vein', (x+r*.66,y+.08,z), (.5,.12,r), collider=False,
                     label=name + ' sap channel')
        # A long vein on the inner north-east wall reads from several elevations.
        self.put('sap_vein', (24,130,24), (1.6,142,.5), PI/4, False, label='Sapfall')
        self.put('heart_core', (0,244,0), collider=False, light=True, label='Ancient core')
        for name in ['Dead Root', 'Rot Chamber', 'Silent Graft']:
            (x,y,z), r, _ = ROOMS[name]
            for side in (-1,1):
                self.put('branch', (x+side*r*.7,y,z+r*.6), (1,1,1),
                         side*.7, False, label='Dead tissue')
                if not self.clear(self.objects[-1]):
                    self.objects[-1]['position'][1]+=10
        # Living crown foliage frames views; never occupies the tread envelope.
        for p, scale, yaw in [((-83,224,-82),(1.6,1,1.2),.4), ((-114,236,8),(1.8,1.2,1.5),1),
            ((-23,242,100),(2.2,1.1,1.8),.3), ((26,250,96),(2,1,1.6),1.5),
            ((115,228,30),(2,1.2,1.8),.7), ((147,164,12),(1.8,1,1.6),.5),
            ((98,148,120),(1.5,1,1.4),.9), ((-164,201,-31),(1.8,1,1.6),1.2)]:
            self.put('branch', (p[0]-4,p[1]-20,p[2]), (1.8,1.4,1.8), yaw, False, label='Canopy twig')
            self.put('foliage', p, scale, yaw, False, label='Canopy foliage')
            # The decorative twig's base grows from the nearest platform bough.
            candidates=[(name,data) for name,data in ROOMS.items() if data[0][1]>90 and data[2]!='inner']
            name,(floor,r,_) = min(candidates,key=lambda item: math.dist(item[1][0],(p[0]-4,p[1]-20,p[2])))
            angle=math.atan2(p[0]-4-floor[0],p[2]-floor[2])
            for turn in (0,.5,-.5,1,-1,PI):
                anchor=(floor[0]+r*.85*math.sin(angle+turn),floor[1]-4,
                        floor[2]+r*.85*math.cos(angle+turn))
                self.beam(anchor,(p[0]-4,p[1]-20,p[2]),5,'Canopy limb '+name)
                if self.clear(self.objects[-1]): break
                self.objects.pop();self.labels.pop()
            else: raise AssertionError('Disconnected canopy twig '+name)

    def lights(self):
        # Sparse roots, inhabited pools, bright sacred destinations, no even grid.
        for name, offset, size in [
            ('Root Gate',(-12,0,0),1), ('Root Nexus',(7,0,5),.7),
            ('Catacombs',(-7,0,3),.6), ('Root Arena',(-12,0,-8),.8),
            ('Underroot Cavern',(12,0,8),.8), ('Sap Reservoir',(10,0,7),1.2),
            ('Lower Gate',(-12,0,0),1), ('Lower Hollow',(15,0,-18),1.2),
            ('Heartwood Settlement',(-11,0,8),.8), ('Workshop',(6,0,5),.65),
            ('Rot Chamber',(-12,0,9),.5), ('Sapworks',(10,0,8),1.1),
            ('Grand Branch Hall',(-14,0,10),1.2), ('Split Bough',(7,0,5),.8),
            ('Branch Arena',(13,0,9),1), ('Upper Heartwood',(-11,0,6),1),
            ('Canopy East',(10,0,7),1.1), ('Canopy Sanctuary',(12,0,10),1.3),
            ('Ancient Heart',(-22,0,21),1.4), ('Ancient Heart',(22,0,21),1.4)]:
            p = ROOMS[name][0]
            r=ROOMS[name][1]
            candidates=[[p[i]+offset[i] for i in range(3)]]
            candidates += [(p[0]+(r-3)*math.sin(a),p[1],p[2]+(r-3)*math.cos(a))
                           for a in (0,PI/2,PI,3*PI/2,.7,2.3,3.8,5.4)]
            self.place_clear('lantern',candidates,(size,)*3,name+' lantern',True)


def build(phase):
    level = Tree()
    level.core(); level.floors(); level.routes(); level.cut_trunk()
    if phase != 'blockout':
        level.architecture(); level.details(); level.lights()
    return level


def validate(level):
    objects = read(PREFAB)['objects']
    assert objects == level.objects, 'Export differs from authoring source'
    fields = {'asset','position','rotation','scale','generateColliders','generateLights'}
    for obj in objects:
        assert set(obj) == fields
        assert min(obj['scale']) > 0
        assert all(math.isfinite(v) for field in ('position','rotation','scale') for v in obj[field])
        assert type(obj['generateColliders']) is bool and type(obj['generateLights']) is bool
        path = (PREFAB.parent / obj['asset']).resolve()
        assert path.is_file(), path
        if obj['generateLights']:
            assert read(path).get('lights'), path
    for path in MODELS.glob('*.json'):
        model = read(path)
        assert set(model) <= {'name','materials','shapes','lights'}
        assert b'\n' not in path.read_bytes().replace(b'\r\n',b''), path
        for part in model['shapes']:
            assert set(part) == {'meshType','position','rotation','scale','color','materialIndex'}
            assert min(part['scale']) > 0 and 0 <= part['materialIndex'] < len(model['materials'])
            assert part['meshType'] in {'Cube','Cylinder','Frustum','Sphere','Crystal'}
            for key in ('position','rotation','scale','color'):
                assert len(part[key]) == (4 if key=='color' else 3)
                assert all(math.isfinite(v) for v in part[key])
        for material in model['materials']:
            assert set(material) == {'albedo','roughness','metallic','ambientStrength',
                'lightWrap','shapeContrast','emissiveColor','emissiveStrength'}
            for value in material.values():
                assert all(math.isfinite(v) for v in (value if isinstance(value,list) else [value]))
        for light in model.get('lights',[]):
            assert set(light) == {'name','type','position','color','intensity','radius','castsShadow'}
            assert light['type']=='Point' and light['radius']>0 and light['intensity']>=0
            assert type(light['castsShadow']) is bool
    assert b'\n' not in PREFAB.read_bytes().replace(b'\r\n',b'')
    # Existing geometry auditor handles exact cube/cylinder/wedge triangles.
    # The old gate's tiny decorative sphere does not provide traversal surfaces.
    audit_objects = copy.deepcopy(objects)
    for obj in audit_objects:
        if Path(obj['asset']).stem in {'wooden_gate','root','lantern','bark_crown'}:
            obj['generateColliders'] = False
    geometry = Geometry(audit_objects, level.labels)
    # Audit excluded primitive volumes as visual/obstruction envelopes as well.
    visual = Geometry(audit_objects, level.labels, decorative=True)
    errors, samples = [], 0
    for label, points, width in level.traces:
        for a,b in zip(points,points[1:]):
            length = math.hypot(b[0]-a[0],b[2]-a[2])
            if length < .01:
                continue
            nx,nz = -(b[2]-a[2])/length,(b[0]-a[0])/length
            count = max(1, math.ceil(length/.5))
            previous = {}
            for step in range(count+1):
                t = step/count
                p = [a[i]+t*(b[i]-a[i]) for i in range(3)]
                for offset in (-min(1.5,width/3),0,min(1.5,width/3)):
                    x,y,z = p[0]+nx*offset,p[1],p[2]+nz*offset
                    floor = geometry.support(x,z,y)
                    obstruction = geometry.blocked(x,floor if floor is not None else y,z)
                    obstruction = obstruction or visual.blocked(x,y,z)
                    if floor is None or obstruction:
                        errors.append((label, tuple(round(v,2) for v in (x,y,z)),
                                       'gap' if floor is None else obstruction))
                    if floor is not None:
                        if offset in previous and abs(floor-previous[offset])>.501:
                            errors.append((label,tuple(p),'step exceeds controller limit'))
                        previous[offset]=floor
                        if offset==0:
                            for ox,oz in ((.4,0),(-.4,0),(0,.4),(0,-.4)):
                                if geometry.support(x+ox,z+oz,floor) is None:
                                    errors.append((label,tuple(p),'unsupported capsule footprint'))
                    samples += 1
    # Spawns are maintained by the game's existing content integration, not JSON.
    source=(ROOT/'game/src/world/biome/forestGenerator.cpp').read_text()
    block=source.split('c_treeGuardPositions[]')[1].split('};')[0]
    spawns=[tuple(map(float,p)) for p in re.findall(r'\{\s*(-?[\d.]+)f,\s*(-?[\d.]+)f,\s*(-?[\d.]+)f\s*\}',block)]
    for x,y,z in [*spawns,(0,224,0)]:
        if geometry.support(x,z,y) is None or geometry.blocked(x,y,z) or visual.blocked(x,y,z):
            errors.append(('encounter',(x,y,z),'unsupported or obstructed spawn'))
    assert 'ForestBrute ? 224.0f' in (ROOT/'game/src/world/enemy/enemySpawn.h').read_text()
    # Reserve wide, unblocked inner discs for dodging, not just route centerlines.
    combat=['Root Arena','Lower Hollow','Sap Reservoir','Rot Chamber','Grand Branch Hall',
            'Branch Arena','Upper Heartwood','Canopy Sanctuary','Ancient Heart']
    for name in combat:
        (x,y,z),r,_=ROOMS[name]
        free=total=0
        for dx in range(-int(r*.62),int(r*.62)+1,2):
            for dz in range(-int(r*.62),int(r*.62)+1,2):
                if math.hypot(dx,dz)>r*.62: continue
                total+=1
                if geometry.support(x+dx,z+dz,y) is not None and not (geometry.blocked(x+dx,y,z+dz) or visual.blocked(x+dx,y,z+dz)):
                    free+=1
        if free/total<.8: errors.append((name,(free,total),'less than 80% clear combat core'))
    # Collider and foliage bounds remain in the reserved footprint and owner's
    # five-chunk window even when the dungeon center lies at a chunk boundary.
    all_parts=geometry.parts+visual.parts
    bounds=[[min(p[2][i] for p in all_parts),max(p[3][i] for p in all_parts)] for i in range(3)]
    assert max(abs(bounds[0][0]),abs(bounds[0][1]))<190
    assert bounds[2][0]>=-108.01 and bounds[2][1]<155
    assert max(abs(v) for axis in (bounds[0],bounds[2]) for v in axis) < 5*64-64
    config=(ROOT/'game/src/world/worldConfig.h').read_text()
    assert 'c_treeDungeonHalfWidth = 190.0f' in config and 'c_treeDungeonBack = 155.0f' in config
    # Across the full seed jitter, the tree reservation cannot touch other courts.
    def envelope(index,radius,left,right,front,back):
        angles=[.785398+index*1.570796+v for v in (-.16,.16)]
        xs=[math.cos(a)*radius for a in angles];zs=[math.sin(a)*radius for a in angles]
        return min(xs)+left,max(xs)+right,min(zs)+front,max(zs)+back
    tree=envelope(1,620,-202,202,-176,167)
    others=[(-90,90,-56,124),envelope(0,520,-162,162,-328,92),
            envelope(2,190,-64,64,-176,56),envelope(3,450,-224,224,-216,232)]
    for other in others:
        assert tree[1]<other[0] or tree[0]>other[1] or tree[3]<other[2] or tree[2]>other[3], ('world overlap',other)
    graph = collections.defaultdict(set)
    for a,b in level.edges:
        graph[a].add(b); graph[b].add(a)
    seen,queue = set(), ['Root Gate']
    while queue:
        p=queue.pop()
        if p in seen: continue
        seen.add(p); queue.extend(graph[p]-seen)
    assert set(ROOMS) == seen, set(ROOMS)-seen
    print(f'{len(objects)} objects; {len(ROOMS)} destinations; {len(level.edges)} connections; '
          f'{len(level.edges)-len(ROOMS)+1} independent loops; {samples} route samples')
    print(f'{len(spawns)} guards + crown boss; {len(combat)} combat cores; bounds {bounds}')
    for item in errors[:8]: print('FAIL',item)
    for key,count in collections.Counter((e[0],e[2]) for e in errors).items():
        print('ISSUE',count,*key)
    print('Traversal errors:',len(errors))
    return not errors


def draw_map(level):
    lines = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1250 950">',
             '<rect width="1250" height="950" fill="#111c19"/>',
             '<g font-family="sans-serif" fill="#e5e5cf">',
             '<text x="30" y="35" font-size="24">Herzholz — authored routes and elevations</text>']
    colors = {'root':'#bc9160','approach':'#ddd0aa','rot':'#aa6b8a','settlement':'#eeac67',
              'hollow':'#cdb996','sap':'#a8e96e','branch':'#92bd84','inner':'#779e97',
              'upper':'#ddd9a6','canopy':'#5fe4a7','heart':'#fff399'}
    for ox,oy,mode,title in [(340,430,'plan','Plan / height labels'),(930,810,'section','Section / Y=0–250')]:
        def point(p):
            return (ox+p[0]*1.65,oy+(p[2]*1.65 if mode=='plan' else -p[1]*2.7))
        lines.append(f'<text x="{ox-190}" y="80" font-size="18">{title}</text>')
        for _,points,_ in level.traces:
            coords=' '.join(f'{x:.1f},{y:.1f}' for x,y in map(point,points))
            lines.append(f'<polyline points="{coords}" fill="none" stroke="#73867b" stroke-width="2"/>')
        for name,(p,r,district) in ROOMS.items():
            x,y=point(p); color=colors[district]
            lines.append(f'<circle cx="{x}" cy="{y}" r="{max(3,r*.4)}" fill="{color}"/>')
            if not name.startswith('Inner'):
                lines.append(f'<text x="{x+5}" y="{y-8}" font-size="9">{html.escape(name)} [{p[1]}]</text>')
    lines.append('</g></svg>')
    (ROOT/'scripts/tree_dungeon_plan.svg').write_text('\n'.join(lines),encoding='utf-8')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--phase',choices=['blockout','lit'],default='lit')
    parser.add_argument('--audit',action='store_true')
    args=parser.parse_args()
    if not args.audit: modules()
    level=build(args.phase)
    if not args.audit:
        write_json(PREFAB,dict(assetType='Prefab',name='Herzholz - Der hohle Weltenstamm',objects=level.objects))
        draw_map(level)
    if not validate(level): sys.exit(1)


if __name__ == '__main__': main()
