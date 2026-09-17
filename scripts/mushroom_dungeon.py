"""Author and audit the hand-placed fungal kingdom (standard library only).

Run --phase blockout, --phase decorated, or --phase lit to export supported
engine JSON. With no arguments, audit the checked-in assets without rewriting.
The room/route names below are authoring metadata, never prefab properties.
"""

import argparse
import collections
import copy
import html
import json
import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MODELS = ROOT / "game/assets/models/mushroom_dungeon"
PREFAB = ROOT / "game/assets/prefabs/mushroom_dungeon.prefab.json"
DOCS = ROOT / "docs"
PI = math.pi


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path, data):
    # Keep repository JSON line endings; vectors stay compact for spatial review.
    import re
    text = json.dumps(data, indent=4, ensure_ascii=False)
    text = re.sub(r"\[\s+([-\d.,eE+\s]+)\s+\]",
                  lambda m: "[" + " ".join(m[1].split()) + "]", text)
    path.write_bytes((text + "\n").replace("\n", "\r\n").encode("utf-8"))


def shape(mesh, position, scale, material=1, rotation=(0, 0, 0)):
    return dict(meshType=mesh, position=list(position), rotation=list(rotation),
                scale=list(scale), color=[1, 1, 1, 1], materialIndex=material)


def modules():
    palette = read(MODELS / "royal_throne.json")["materials"]
    parts = {
        # Origin is the usable top surface, matching foundation.json.
        "floor_plate": [shape("Cube", (0, -.5, 0), (1, 1, 1), 5)],
        # A single scalable bay; the cornice remains out of head clearance.
        "wall_bay": [shape("Cube", (0, .46, 0), (1, .92, 1)),
                     shape("Cube", (0, .96, 0), (1, .08, 1.18), 6)],
        "parapet": [shape("Cube", (0, .82, 0), (1, 1.64, .6)),
                    shape("Cube", (0, 1.72, 0), (1, .16, .8), 6)],
        "fungal_pier": [shape("Cylinder", (0, .6, 0), (5, 1.2, 5), 5),
                        shape("Cylinder", (0, 6, 0), (2.4, 10, 2.4)),
                        shape("Frustum", (0, 11, 0), (5, 2, 5), 6,
                              (PI, 0, 0))],
        # Low (-Z) landing at y=0, high (+Z) landing at y=8.
        # Forty .2-unit risers, .6-unit treads; both match the original stairs.
        "stair_flight": [shape("Cube", (0, (i + 1) * .2 - .5, -12 + (i + .5) * .6),
                               (8, 1, .6), 5) for i in range(40)],
        "vault_rib": [shape("Arch", (0, 0, 0), (20, 12, 4))],
        # Walkable inclined alternative to steps, rises from -Z to +Z.
        "ramp": [shape("Wedge", (0, 4, 0), (24, 8, 8), 5, (0, -PI / 2, 0))],
    }
    for name, shapes in parts.items():
        write_json(MODELS / (name + ".json"),
                   dict(name="mushroom_dungeon_" + name, materials=palette, shapes=shapes))


# x, z, elevation, width, depth, wall height, plan, district, category.
# These are authored spaces, not a random room generator.
ROOMS = {
    "Pilgrim gate": (-20, -54, 0, 24, 20, 12, "rect", "ruins", "entrance"),
    "Broken customs hall": (-44, -64, 0, 24, 24, 10, "rect", "ruins", "medium"),
    "Ossuary court": (-98, -102, 0, 64, 48, 14, "round", "crypt", "major"),
    "Burial registry": (-160, -116, 0, 28, 22, 9, "rect", "crypt", "medium"),
    "Collapsed vestry": (-192, -76, 0, 26, 26, 8, "rect", "crypt", "optional"),
    "Cistern landing": (-150, -48, 8, 30, 30, 13, "round", "cavern", "medium"),
    "Mycelium watch": (-104, -28, 8, 24, 22, 10, "rect", "crypt", "medium"),
    "Giant's hollow": (-160, 8, 8, 72, 64, 32, "round", "cavern", "major"),
    "Amber nursery": (-140, 100, 16, 64, 60, 26, "round", "nursery", "major"),
    "Seed bank": (-196, 64, 16, 24, 24, 11, "rect", "nursery", "medium"),
    "Spore belfry": (-192, 132, 16, 24, 24, 24, "round", "nursery", "optional"),
    "Incubators": (-94, 80, 16, 24, 28, 10, "rect", "nursery", "medium"),
    "Grafting room": (-92, 120, 16, 24, 20, 10, "rect", "nursery", "medium"),
    "Root parliament": (-40, 166, 24, 64, 60, 22, "round", "roots", "major"),
    "Buried chapel": (-34, 212, 24, 28, 20, 13, "rect", "roots", "optional"),
    "Root maze": (10, 196, 24, 28, 28, 12, "rect", "roots", "medium"),
    "Severed aqueduct": (20, 144, 24, 28, 20, 10, "rect", "roots", "medium"),
    "Distillery": (72, 202, 24, 28, 24, 12, "rect", "alchemy", "medium"),
    "Alchemists' forum": (102, 144, 24, 68, 52, 16, "rect", "alchemy", "major"),
    "Apothecary": (166, 184, 24, 28, 24, 11, "rect", "alchemy", "medium"),
    "Quarantine still": (186, 132, 24, 24, 28, 12, "rect", "alchemy", "optional"),
    "Seal office": (152, 98, 24, 24, 22, 10, "rect", "archive", "medium"),
    "Silent archive": (152, 40, 24, 60, 60, 19, "rect", "archive", "major"),
    "Readers' overlook": (202, 52, 32, 24, 28, 10, "rect", "archive", "medium"),
    "Scriptorium": (132, -16, 24, 28, 20, 10, "rect", "archive", "medium"),
    "Envoys' cloister": (78, -20, 24, 26, 24, 12, "rect", "royal", "medium"),
    "Crown reliquary": (170, -52, 24, 28, 24, 12, "round", "royal", "optional"),
    "Return belvedere": (36, -86, 16, 24, 20, 9, "rect", "ruins", "medium"),
}


# All points are (x, y, z). Endpoints extend into rooms to make floor seams
# robust. Room walls are cut only where an actual route crosses the perimeter.
# The graph includes the retained tower, temple, and boss crown.
ROUTES = [
    ("Approach", "Pilgrim gate", 3, [(0, 0, -44), (0, 0, -43), (-20, 0, -43), (-20, 0, -50)]),
    ("Pilgrim gate", "Broken customs hall", 10, [(-20, 0, -50), (-44, 0, -50), (-44, 0, -64)]),
    ("Broken customs hall", "Ossuary court", 10, [(-44, 0, -64), (-66, 0, -58), (-98, 0, -70), (-98, 0, -102)]),
    ("Ossuary court", "Burial registry", 7, [(-98, 0, -106), (-160, 0, -106), (-160, 0, -116)]),
    ("Burial registry", "Collapsed vestry", 6, [(-160, 0, -116), (-192, 0, -116), (-192, 0, -76)]),
    ("Ossuary court", "Cistern landing", 8, [(-114, 0, -92), (-144, 0, -92), (-144, 0, -88), (-144, 8, -64), (-144, 8, -48), (-150, 8, -48)]),
    ("Cistern landing", "Giant's hollow", 10, [(-150, 8, -48), (-150, 8, 8), (-160, 8, 8)]),
    ("Ossuary court", "Mycelium watch", 7, [(-104, 0, -102), (-104, 0, -70), (-104, 8, -46), (-104, 8, -28)]),
    ("Mycelium watch", "Giant's hollow", 8, [(-104, 8, -28), (-104, 8, 0), (-160, 8, 0)]),
    ("Giant's hollow", "Amber nursery", 10, [(-160, 8, 8), (-160, 8, 40), (-160, 16, 64), (-160, 16, 100), (-140, 16, 100)]),
    ("Amber nursery", "Seed bank", 7, [(-140, 16, 92), (-196, 16, 92), (-196, 16, 64)]),
    ("Seed bank", "Spore belfry", 6, [(-196, 16, 64), (-214, 16, 64), (-214, 16, 132), (-192, 16, 132)]),
    ("Spore belfry", "Amber nursery", 7, [(-192, 16, 132), (-192, 16, 148), (-156, 16, 148), (-156, 16, 100)]),
    ("Amber nursery", "Incubators", 8, [(-140, 16, 86), (-94, 16, 86), (-94, 16, 80)]),
    ("Incubators", "Grafting room", 7, [(-94, 16, 80), (-94, 16, 120), (-92, 16, 120)]),
    ("Grafting room", "Amber nursery", 7, [(-92, 16, 120), (-140, 16, 120), (-140, 16, 100)]),
    ("Amber nursery", "Root parliament", 8, [(-130, 16, 100), (-130, 16, 152), (-124, 16, 152), (-100, 24, 152), (-40, 24, 152), (-40, 24, 166)]),
    ("Root parliament", "Buried chapel", 7, [(-40, 24, 166), (-40, 24, 212), (-34, 24, 212)]),
    ("Root parliament", "Severed aqueduct", 10, [(-40, 24, 144), (20, 24, 144)]),
    ("Root parliament", "Root maze", 7, [(-40, 24, 180), (10, 24, 180), (10, 24, 196)]),
    ("Root maze", "Distillery", 6, [(10, 24, 196), (42, 24, 182), (72, 24, 202)]),
    ("Severed aqueduct", "Alchemists' forum", 8, [(20, 24, 144), (102, 24, 144)]),
    ("Distillery", "Alchemists' forum", 8, [(72, 24, 202), (72, 24, 144), (102, 24, 144)]),
    ("Alchemists' forum", "Apothecary", 8, [(120, 24, 144), (120, 24, 184), (166, 24, 184)]),
    ("Alchemists' forum", "Quarantine still", 7, [(102, 24, 132), (186, 24, 132)]),
    ("Alchemists' forum", "Seal office", 10, [(124, 24, 144), (124, 24, 98), (152, 24, 98)]),
    ("Seal office", "Silent archive", 10, [(152, 24, 98), (152, 24, 40)]),
    ("Silent archive", "Readers' overlook", 7, [(152, 24, 20), (186, 24, 20), (186, 24, -2), (202, 24, -2), (202, 24, 4), (202, 32, 28), (202, 32, 52)]),
    ("Readers' overlook", "Seal office", 7, [(202, 32, 52), (202, 32, 80), (196, 32, 80), (172, 24, 80), (166, 24, 80), (166, 24, 98), (152, 24, 98)]),
    ("Silent archive", "Scriptorium", 10, [(138, 24, 40), (138, 24, -16), (132, 24, -16)]),
    ("Scriptorium", "Envoys' cloister", 8, [(132, 24, -16), (78, 24, -16), (78, 24, -20)]),
    ("Scriptorium", "Crown reliquary", 6, [(132, 24, -16), (170, 24, -16), (170, 24, -52)]),
    ("Envoys' cloister", "Tower landing", 8, [(78, 24, -20), (78, 24, 0), (29, 24, 0), (22, 24, 0)]),
    ("Envoys' cloister", "Return belvedere", 8, [(78, 24, -20), (78, 24, -86), (72, 24, -86), (48, 16, -86), (36, 16, -86)]),
    ("Return belvedere", "Broken customs hall", 8, [(36, 16, -86), (24, 16, -86), (0, 8, -86), (-24, 0, -86), (-44, 0, -86), (-44, 0, -64)]),
]


def polygon(room):
    x, z, y, w, d, h, plan, district, category = room
    if plan == "round":
        return [(x + math.sin(i * PI / 16) * w / 2,
                 z + math.cos(i * PI / 16) * d / 2) for i in range(32)]
    return [(x - w / 2, z - d / 2), (x - w / 2, z + d / 2),
            (x + w / 2, z + d / 2), (x + w / 2, z - d / 2)]


def inside(x, z, poly):
    hit = False
    for a, b in zip(poly, poly[1:] + poly[:1]):
        if (a[1] > z) != (b[1] > z):
            if x < (b[0] - a[0]) * (z - a[1]) / (b[1] - a[1]) + a[0]:
                hit = not hit
    return hit


def distance_segment(x, z, a, b):
    dx, dz = b[0] - a[0], b[2] - a[2]
    t = max(0, min(1, ((x - a[0]) * dx + (z - a[2]) * dz) / (dx * dx + dz * dz)))
    return math.hypot(x - a[0] - t * dx, z - a[2] - t * dz), a[1] + t * (b[1] - a[1])


class Kingdom:
    def __init__(self):
        self.objects = []
        self.labels = []
        self.walks = []
        self.portals = []
        self.segments = [(a, b, width) for _, _, width, points in ROUTES
                         for a, b in zip(points, points[1:]) if a != b]

    def put(self, asset, p, scale=(1, 1, 1), yaw=0, collider=False, light=False, label=""):
        self.objects.append(dict(asset="../models/mushroom_dungeon/" + asset + ".json",
                                 position=[round(v, 6) for v in p],
                                 rotation=[0, round(yaw, 8), 0],
                                 scale=[round(v, 6) for v in scale],
                                 generateColliders=collider, generateLights=light))
        self.labels.append(label or asset)

    def floor(self, x, y, z, w, d, thickness=3, label="floor"):
        self.put("floor_plate", (x, y, z), (w, thickness, d), collider=True, label=label)

    def wall(self, a, b, y, height, label, rail=False):
        dx, dz = b[0] - a[0], b[1] - a[1]
        length = math.hypot(dx, dz)
        if length < .15:
            return
        self.put("parapet" if rail else "wall_bay", ((a[0] + b[0]) / 2, y, (a[1] + b[1]) / 2),
                 (length, 1 if rail else height, 1 if rail else 1.2),
                 -math.atan2(dz, dx), True, label=label)

    def route_at(self, x, z, y, margin=0):
        for a, b, width in self.segments:
            distance, floor = distance_segment(x, z, a, b)
            if distance < width / 2 + margin and abs(floor - y) < 3:
                return True
        return False

    def in_room(self, x, z, y, margin=0):
        for room in ROOMS.values():
            if abs(room[2] - y) < 2 and inside(x, z, polygon(room)):
                return True
        # Tower foundation/decks are retained geometry.
        return math.hypot(x, z) < 38 and y >= 0

    def cut_wall(self, a, b, y, height, label, rail=False):
        length = math.dist(a, b)
        count = max(1, math.ceil(length / .4))
        start = None
        for i in range(count + 1):
            t = (i + .5) / count
            x, z = a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
            closed = i < count and not self.route_at(x, z, y, .9)
            if closed and start is None:
                start = i / count
            if not closed and start is not None:
                end = i / count
                self.wall((a[0] + (b[0] - a[0]) * start, a[1] + (b[1] - a[1]) * start),
                          (a[0] + (b[0] - a[0]) * end, a[1] + (b[1] - a[1]) * end),
                          y, height, label, rail)
                start = None

    def rooms(self):
        for name, room in ROOMS.items():
            x, z, y, w, d, h, plan, district, category = room
            if plan == "round":
                self.put("foundation", (x, y, z), (w / 88, 1, d / 88),
                         collider=True, label=name + " floor")
            else:
                self.floor(x, y, z, w, d, 100, name + " floor")
            poly = polygon(room)
            for a, b in zip(poly, poly[1:] + poly[:1]):
                self.cut_wall(a, b, y, h, name + " wall", name == "Return belvedere")
            if name == "Collapsed vestry":
                # Two surviving roof pieces leave a real breach over the fallen slab.
                self.floor(x - 6, y + h + 1.5, z, 12, d, 1.5, name + " broken ceiling")
                self.floor(x + 6, y + h + 1.5, z + 6, 12, 12, 1.5, name + " broken ceiling")
                self.put("floor_plate", (x + 8, y + 1.6, z - 6), (8, 1, 5), .55,
                         True, label=name + " fallen lintel")
                self.objects[-1]["rotation"][2] = -.18
            elif name not in ("Pilgrim gate", "Return belvedere", "Readers' overlook"):
                if plan == "round":
                    self.put("foundation", (x, y + h + 2, z), (w / 88, .02, d / 88),
                             collider=True, label=name + " ceiling")
                else:
                    self.floor(x, y + h + 1.5, z, w - .16, d - .16, 1.5, name + " ceiling")
            # Sample interior arrival/turn points in addition to route corridors.
            self.walks.append((name + " center", [(x, y, z)]))

    def routes(self):
        for route_index, (source, target, width, points) in enumerate(ROUTES):
            label = source + " -> " + target
            enclosed = (source, target) in {
                ("Ossuary court", "Burial registry"),
                ("Burial registry", "Collapsed vestry"),
                ("Incubators", "Grafting room"),
                ("Root maze", "Distillery"),
                ("Alchemists' forum", "Seal office"),
                ("Silent archive", "Scriptorium"),
            }
            self.walks.append((label, points))
            for a, b in zip(points, points[1:]):
                dx, dy, dz = (b[i] - a[i] for i in range(3))
                length = math.hypot(dx, dz)
                if length < .01:
                    continue
                yaw = math.atan2(dx, dz)
                mid = [(a[i] + b[i]) / 2 for i in range(3)]
                if dy:
                    assert abs(abs(dy) - 8) < .01 and abs(length - 24) < .01, label
                    low, high = (a, b) if dy > 0 else (b, a)
                    up_yaw = math.atan2(high[0] - low[0], high[2] - low[2])
                    # The archive's lower access uses a ramp; other connections are stairs.
                    asset = "ramp" if (source, target) == ("Silent archive", "Readers' overlook") else "stair_flight"
                    self.put(asset, (mid[0], low[1], mid[2]), (width / 8, 1, 1),
                             up_yaw, True, label=label)
                    # Thin sloping balustrades follow the pitch without step obstacles.
                    for sign in (-1, 1):
                        ox, oz = math.cos(up_yaw) * (width / 2 - .25) * sign, -math.sin(up_yaw) * (width / 2 - .25) * sign
                        # A plain beam joins successive flights end to end; wall_bay's
                        # projecting cornice would overlap along the incline.
                        self.put("floor_plate", (low[0] + ox, low[1] + .5, low[2] + oz),
                                 (.45, 1.5, math.hypot(length, 8)), up_yaw, True, label=label + " stair rail")
                        obj = self.objects[-1]
                        obj["rotation"][0] = -math.atan2(8, length)
                        top = rotate((0, 1.5, 0), obj["rotation"])
                        obj["position"] = [round(mid[0] + ox + top[0], 6),
                                           round(mid[1] + .45 + top[1], 6),
                                           round(mid[2] + oz + top[2], 6)]
                    continue
                # Floor only outside rooms. Segment intervals avoid coplanar z fighting
                # and keep low corridors from becoming slabs inside higher rooms.
                count = max(1, math.ceil(length / .5))
                start = None
                for i in range(count + 1):
                    t = (i + .5) / count
                    x, z = a[0] + dx * t, a[2] + dz * t
                    outside = i < count and not self.in_room(x, z, a[1])
                    if outside and start is None:
                        start = i / count
                    if not outside and start is not None:
                        end = i / count
                        tmid = (start + end) / 2
                        self.put("floor_plate", (a[0] + dx * tmid, a[1] - .02, a[2] + dz * tmid),
                                 (width - .08, 3, length * (end - start) + .8), yaw, True, label=label + " bridge")
                        if enclosed:
                            self.put("floor_plate", (a[0] + dx * tmid, a[1] + 8, a[2] + dz * tmid),
                                     (width + 1, 1, length * (end - start) + .8), yaw, True,
                                     label=label + " corridor ceiling")
                        start = None
                # Boundary rails skip rooms, junctions, and neighboring route decks.
                for side in (-1, 1):
                    ox, oz = math.cos(yaw) * (width / 2 - .25) * side, -math.sin(yaw) * (width / 2 - .25) * side
                    start = None
                    for i in range(count + 1):
                        t = (i + .5) / count
                        x, z = a[0] + dx * t + ox, a[2] + dz * t + oz
                        closed = i < count and not self.in_room(x, z, a[1])
                        if closed:
                            for c, e, other_width in self.segments:
                                if (c, e) == (a, b):
                                    continue
                                distance, height = distance_segment(x, z, c, e)
                                if distance < other_width / 2 + .8 and abs(height - a[1]) < 3:
                                    closed = False
                                    break
                        if closed and start is None:
                            start = i / count
                        if not closed and start is not None:
                            end = i / count
                            self.wall((a[0] + dx * start + ox, a[2] + dz * start + oz),
                                      (a[0] + dx * end + ox, a[2] + dz * end + oz),
                                      a[1] - .1, 8 if enclosed else 1.7, label + " boundary", not enclosed)
                            start = None

    def tower(self):
        # Keep the original tower kit, its original staircase, and crown datum.
        for name in ("stem_shell", "foundation", "decks", "stairs", "partitions", "railings",
                     "roof_shell", "roof_stairs", "roof_railings", "roof_arena", "roof_balustrade"):
            self.put(name, (0, 0, 0), collider=True, label="retained " + name)
        # Close the former direct ascent; arriving pilgrims enter the west district.
        self.wall((-16, -39), (16, -39), 0, 16, "sealed tower southern gate")
        self.floor(0, -.08, -43, 15.8, 2, 1, "entrance approach landing")
        for x in (-9, 9):
            self.floor(x, 0, -46, 3.8, 4.5, 6, "gateway footing")
        # East archive bridge passes through a real opening in stem_shell.
        self.floor(40, 24.1, 0, 12, 6.12, 1, "tower east threshold")
        # Preserve the existing floor landing links at radius 30..38.
        for name, start, end in (("Lower tower crypt", 0, 120), ("Tower landing", 120, 240), ("Royal temple", 240, 360)):
            stairs = read(MODELS / "stairs.json")["shapes"][start:end]
            points = [(s["position"][0], s["position"][1] + .4, s["position"][2]) for s in stairs]
            self.walks.append((name + " spiral", points))
        roof = read(MODELS / "roof_stairs.json")["shapes"]
        self.walks.append(("Royal temple -> Crown arena", [
            (s["position"][0], s["position"][1] + s["scale"][1] / 2, s["position"][2]) for s in roof]))
        for y, x, z in ((24, 33.5, 0), (48, 0, 33.5), (72, -33.5, 0)):
            self.walks.append(("tower deck threshold", [(x * .8, y, z * .8), (x, y, z)]))
        self.walks.append(("Crown entry", [(0, 134, -40), (0, 134, -32), (0, 134, 0)]))
        self.walks.append(("Royal temple nave", [(-33.5, 72, 0), (0, 72, 0), (0, 72, 17)]))
        self.walks.append(("Lower tower crypt nave", [(0, 0, 0), (0, 0, -35)]))

    def architecture(self):
        # Distinct combat silhouettes, with open centers and multiple edge routes.
        for name, locations in {
            "Ossuary court": [(-114, -109), (-81, -96)],
            "Giant's hollow": [(-180, 2), (-143, 20)],
            "Amber nursery": [(-148, 80), (-117, 104)],
            "Root parliament": [(-57, 161), (-22, 176)],
            "Alchemists' forum": [(86, 134), (113, 157)],
            "Silent archive": [(130, 39), (169, 48)],
        }.items():
            y = ROOMS[name][2]
            for x, z in locations:
                self.put("fungal_pier", (x, y, z), collider=True, label=name + " cover")
        # Root maze is deliberately staggered, with broad central and edge routes.
        self.wall((0, 186), (0, 191), 24, 8, "root maze screen")
        self.wall((19, 201), (19, 207), 24, 7, "root maze screen")
        # A genuine upper archive balcony overlooking the lower reading floor.
        self.floor(173, 32, 60, 18, 12, 1, "archive balcony")
        self.floor(186, 32, 60, 10, 7.84, 1, "archive balcony bridge")
        self.wall((164, 54), (179, 54), 32, 2, "archive balcony edge", True)
        self.wall((164, 54), (164, 66), 32, 2, "archive balcony edge", True)
        self.walks.append(("Readers' overlook -> archive balcony", [(202, 32, 60), (170, 32, 60)]))
        # Carve this upper doorway independently of the lower-level graph.
        for i in range(len(self.objects) - 1, -1, -1):
            o = self.objects[i]
            if self.labels[i] == "Silent archive wall" and abs(o["position"][0] - 182) < .1:
                z, length = o["position"][2], o["scale"][0]
                lo, hi = z - length / 2, z + length / 2
                if lo < 64 and hi > 56:
                    del self.objects[i]; del self.labels[i]
                    for a, b in ((lo, min(56, hi)), (max(64, lo), hi)):
                        if b > a:
                            self.wall((182, a), (182, b), 24, 19, "archive doorway side")
                    self.wall((182, max(lo, 56)), (182, min(hi, 64)), 24, 8, "archive doorway sill")
                    self.wall((182, max(lo, 56)), (182, min(hi, 64)), 40, 3, "archive doorway lintel")
        # Match the overlook's western opening to the balcony bridge.
        for i in range(len(self.objects) - 1, -1, -1):
            o = self.objects[i]
            if self.labels[i] == "Readers' overlook wall" and abs(o["position"][0] - 190) < .1:
                lo, hi = o["position"][2] - o["scale"][0] / 2, o["position"][2] + o["scale"][0] / 2
                if lo < 64 and hi > 56:
                    del self.objects[i]; del self.labels[i]
                    for a, b in ((lo, min(56, hi)), (max(64, lo), hi)):
                        if b > a:
                            self.wall((190, a), (190, b), 32, 10, "overlook doorway side")

    def masonry_joints(self):
        # Adjacent rooms share one wall, even when their ceiling heights differ.
        # Give the taller bay ownership of coincident centerline intervals.
        walls = [i for i, obj in enumerate(self.objects)
                 if Path(obj["asset"]).stem == "wall_bay" and abs(obj["rotation"][0]) < 1e-6]
        owned, replacements = [], {}
        for i in sorted(walls, key=lambda i: -self.objects[i]["scale"][1]):
            obj = self.objects[i]
            x, y, z = obj["position"]
            ux, uz = math.cos(obj["rotation"][1]), -math.sin(obj["rotation"][1])
            intervals = [(-obj["scale"][0] / 2, obj["scale"][0] / 2)]
            for other in owned:
                ox, oy, oz = other["position"]
                if abs(y - oy) > 1e-6 or abs(math.sin(obj["rotation"][1] - other["rotation"][1])) > 1e-6:
                    continue
                if abs((ox - x) * uz - (oz - z) * ux) > 1e-5:
                    continue
                mid = (ox - x) * ux + (oz - z) * uz
                lo, hi = mid - other["scale"][0] / 2, mid + other["scale"][0] / 2
                clipped = []
                for a, b in intervals:
                    if hi <= a or lo >= b:
                        clipped.append((a, b))
                    else:
                        clipped.extend(((a, min(b, lo)), (max(a, hi), b)))
                intervals = [(a, b) for a, b in clipped if b - a > .08]
            replacements[i] = []
            for a, b in intervals:
                bay = copy.deepcopy(obj)
                bay["position"][0] = round(x + (a + b) / 2 * ux, 6)
                bay["position"][2] = round(z + (a + b) / 2 * uz, 6)
                bay["scale"][0] = round(b - a, 6)
                replacements[i].append(bay)
                owned.append(bay)
        wall_objects, wall_labels = [], []
        for i, (obj, label) in enumerate(zip(self.objects, self.labels)):
            for bay in replacements.get(i, [obj]):
                wall_objects.append(bay); wall_labels.append(label)
        self.objects, self.labels = wall_objects, wall_labels

        # Share each coplanar rectangular floor area once. Subtract earlier
        # slabs at bends, T junctions and balcony landings, retaining butt joints.
        # The few diagonal approaches sit .08 lower, like inset bridge paving.
        objects, labels, occupied = [], [], collections.defaultdict(list)
        diagonal = collections.defaultdict(list)
        for obj, label in zip(self.objects, self.labels):
            if Path(obj["asset"]).stem != "floor_plate" or any(abs(obj["rotation"][i]) > 1e-6 for i in (0, 2)):
                objects.append(obj); labels.append(label)
                continue
            yaw = obj["rotation"][1]
            if abs(math.sin(2 * yaw)) > 1e-6:
                poly = [(p[0], p[2]) for p in [transform(v, obj) for v in
                        ((-.5, 0, -.5), (-.5, 0, .5), (.5, 0, .5), (.5, 0, -.5))]]
                y = obj["position"][1]
                unavailable = {tier for other, tier in diagonal[y]
                               if polygon_area(clip_polygon(poly, other)) > 1e-5}
                tier = next(i for i in range(len(unavailable) + 1) if i not in unavailable)
                diagonal[y].append((poly, tier))
                obj["position"][1] = round(y - .08 * (tier + 1), 6)
                objects.append(obj); labels.append(label)
                continue
            x, y, z = obj["position"]
            w, thickness, depth = obj["scale"]
            if abs(math.sin(yaw)) > .5:
                w, depth = depth, w
            pieces = [(x - w / 2, z - depth / 2, x + w / 2, z + depth / 2)]
            for ax, az, bx, bz in occupied[y]:
                result = []
                for lx, lz, hx, hz in pieces:
                    ix, iz, jx, jz = max(lx, ax), max(lz, az), min(hx, bx), min(hz, bz)
                    if ix >= jx - 1e-6 or iz >= jz - 1e-6:
                        result.append((lx, lz, hx, hz))
                    else:
                        result.extend([(lx, lz, ix, hz), (jx, lz, hx, hz),
                                       (ix, lz, jx, iz), (ix, jz, jx, hz)])
                pieces = [p for p in result if p[2] - p[0] > 1e-5 and p[3] - p[1] > 1e-5]
            for lx, lz, hx, hz in pieces:
                slab = copy.deepcopy(obj)
                slab["position"] = [round((lx + hx) / 2, 6), y, round((lz + hz) / 2, 6)]
                slab["rotation"] = [0, 0, 0]
                slab["scale"] = [round(hx - lx, 6), thickness, round(hz - lz, 6)]
                objects.append(slab); labels.append(label)
                occupied[y].append((lx, lz, hx, hz))
        self.objects, self.labels = objects, labels

        # Slightly stepped coping is intentional masonry detail. Neighboring
        # wall caps must not share an exposed top plane at angled corners.
        coping = collections.defaultdict(list)
        for obj in self.objects:
            asset = Path(obj["asset"]).stem
            if asset not in {"wall_bay", "parapet"} or abs(obj["rotation"][0]) > 1e-6:
                continue
            # Inset end grain avoids coplanar foundation sides at room corners.
            obj["scale"][0] = round(obj["scale"][0] - .08, 6)
            points = [transform(p, obj) for p in ((-.5, 0, -.59), (-.5, 0, .59),
                                                   (.5, 0, .59), (.5, 0, -.59))]
            poly = [(p[0], p[2]) for p in points]
            key = (obj["position"][1], obj["scale"][1], asset)
            unavailable = {tier for other, tier in coping[key]
                           if polygon_area(clip_polygon(poly, other)) > 1e-5}
            tier = next(i for i in range(len(unavailable) + 1) if i not in unavailable)
            coping[key].append((poly, tier))
            obj["position"][1] = round(obj["position"][1] - .04 * (tier + 1), 6)
            obj["scale"][1] = round(obj["scale"][1] + .16 * (tier + 1) / (1.8 if asset == "parapet" else 1), 6)

    def decorate(self):
        for asset in ("elder", "roots", "joinery"):
            self.put(asset, (0, 0, 0), yaw=PI / 6 if asset == "roots" else 0)
        # Large crowns identify the hollow and nursery from distant overlooks.
        for name, height in (("Giant's hollow", 12), ("Amber nursery", 8)):
            x, z, y, w, d, h, *_ = ROOMS[name]
            sy = height / 42
            self.put("roof_shell", (x, y + h + 1.5 - 90 * sy, z),
                     (w * 1.1 / 152, sy, d * 1.1 / 152))
        # Deliberate district compositions; no random scatter or room-wide grids.
        decor = [
            ("gateway", (0, 0, -46), (1, 1, 1), 0),
            ("waystone", (-25, 0, -55), (1, 1, 1), 0),
            ("cluster", (-13, 0, -54), (1.3, 1.3, 1.3), .4),
            ("bench", (-50, 0, -57), (1, 1, 1), .2),
            ("growth_shelf", (-51, 6, -72), (1.4, 1, 1), PI),
            ("scroll_shelf", (-166, 0, -123), (1, 1, 1), 0),
            ("bench", (-187, 0, -72), (1.2, 1, 1), .7),
            ("cluster", (-199, 0, -68), (1.7, 1.7, 1.7), 1.2),
            ("royal_throne", (-98, 0, -119), (.6, .6, .6), 0),
            ("floor_roots", (-98, 0, -101), (1.8, 1, .8), .5),
            ("growth_shelf", (-158, 14, -54), (1.3, 1.2, 1.2), -.6),
            ("cluster", (-182, 8, 16), (4, 6, 4), -.35),
            ("cluster", (-139, 8, -7), (2, 3.2, 2), 1.4),
            ("growth_shelf", (-185, 23, 20), (2, 1.5, 2), 1.2),
            ("hanging_spores", (-167, 40, 3), (2.5, 2.5, 2.5), 0),
            ("hanging_spores", (-139, 40, 11), (1.7, 2, 1.7), .5),
            ("waystone", (-109, 8, -23), (1, 1, 1), 0),
            ("nursery", (-153, 16, 86), (1.6, 1.4, 1.5), .35),
            ("nursery", (-147, 16, 116), (1.8, 1.4, 1.4), -.4),
            ("nursery", (-121, 16, 95), (1.3, 1.4, 1.4), 1.2),
            ("cluster", (-148, 16, 106), (2.5, 4, 2.5), .5),
            ("hanging_spores", (-140, 40, 100), (3, 3, 3), .7),
            ("hanging_spores", (-153, 42, 91), (1.5, 1.5, 1.5), -.4),
            ("growth_shelf", (-131, 30, 127), (1.5, 1.2, 1.3), PI),
            ("nursery", (-201, 16, 58), (1, 1, 1), 0),
            ("growth_shelf", (-190, 20, 53.5), (1, 1, 1), 0),
            ("nursery", (-88, 16, 75), (1, 1, 1), PI / 2),
            ("nursery", (-100, 16, 89), (1, 1, 1), -.3),
            ("alchemy_table", (-88, 16, 125), (1, 1, 1), 0),
            ("growth_shelf", (-87, 21, 111), (1, 1, 1), 0),
            ("hanging_spores", (-192, 40, 132), (2, 3.3, 2), 0),
            ("waystone", (-187, 16, 127), (1, 1, 1), 0),
            ("roots", (-40, 32, 166), (.5, .32, .45), .8),
            ("floor_roots", (-40, 24, 166), (2.8, 1, 1.5), -.6),
            ("growth_shelf", (-64, 36, 175), (2, 1.8, 2), PI / 2),
            ("cluster", (-23, 24, 185), (2.6, 3, 2.6), .7),
            ("royal_throne", (-30, 24, 217), (.55, .55, .55), 0),
            ("growth_shelf", (-40, 30, 220), (1.5, 1, 1), PI),
            ("floor_roots", (10, 24, 196), (1, 1, .7), 0),
            ("cluster", (2, 24, 204), (1.4, 2, 1.4), -.8),
            ("growth_shelf", (33, 29, 139), (1, 1, 1), PI / 2),
            ("alchemy_table", (86, 24, 155), (1.4, 1.2, 1.2), -.2),
            ("alchemy_table", (118, 24, 139), (1.2, 1.2, 1.2), PI / 2),
            ("growth_shelf", (69.5, 30, 161), (1.7, 1, 1.2), -PI / 2),
            ("nursery", (126, 24, 160), (1.5, 1.5, 1.5), PI / 2),
            ("hanging_spores", (91, 40, 155), (1.5, 1.5, 1.5), 0),
            ("alchemy_table", (64, 24, 208), (1, 1, 1), 0),
            ("alchemy_table", (79, 24, 206), (1, 1, 1), .5),
            ("scroll_shelf", (173, 24, 189), (1, 1, 1), 0),
            ("alchemy_table", (159, 24, 190), (1, 1, 1), 0),
            ("alchemy_table", (191, 24, 124), (1, 1, 1), PI / 2),
            ("cluster", (181, 24, 141), (1.6, 2, 1.6), .3),
            ("scroll_shelf", (159, 24, 103), (1, 1, 1), 0),
            ("scroll_shelf", (128, 24, 27), (1.6, 1.6, 1.3), -PI / 2),
            ("scroll_shelf", (128, 24, 51), (1.6, 1.8, 1.3), -PI / 2),
            ("scroll_shelf", (176, 24, 36), (1.4, 1.8, 1.3), PI / 2),
            ("bench", (145, 24, 59), (1.4, 1, 1), 0),
            ("bench", (165, 32, 62), (1, 1, 1), -PI / 2),
            ("scroll_shelf", (207, 32, 45), (1, 1, 1), PI / 2),
            ("bench", (206, 32, 61), (1, 1, 1), .2),
            ("scroll_shelf", (125, 24, -22), (1, 1, 1), 0),
            ("alchemy_table", (140, 24, -22), (1, 1, 1), 0),
            ("herald_banner", (69, 24, -25), (1.3, 1.3, 1.3), .1),
            ("bench", (85, 24, -25), (1, 1, 1), 0),
            ("royal_throne", (170, 24, -59), (.65, .65, .65), PI),
            ("waystone", (39, 16, -91), (1, 1, 1), 0),
            ("bench", (30, 16, -91), (1, 1, 1), 0),
            ("arena_mandala", (0, 72, 0), (.7, 1, .7), 0),
            ("royal_throne", (0, 72, 22), (.9, .9, .9), 0),
            ("herald_banner", (-8, 72, 21), (1.3, 1.3, 1.3), -.2),
            ("herald_banner", (8, 72, 21), (1.3, 1.3, 1.3), .2),
            ("arena_mandala", (0, 134, 0), (1, 1, 1), 0),
            ("royal_throne", (0, 134, 31), (1, 1, 1), 0),
            ("herald_banner", (-9, 134, 30), (1.4, 1.4, 1.4), 0),
            ("herald_banner", (9, 134, 30), (1.4, 1.4, 1.4), 0),
        ]
        for asset, p, scale, yaw in decor:
            self.put(asset, p, scale, yaw)
        # Roof ribs anchor hanging props and break up low hallway silhouettes.
        for name in ("Ossuary court", "Amber nursery", "Alchemists' forum", "Silent archive"):
            x, z, y, w, d, h, *_ = ROOMS[name]
            self.put("vault_rib", (x, y + h - 6, z), (w / 20, 1, 1))
        # Portals are paired with real thresholds, never placed across room centers.
        for p, yaw, scale in [((-44, 0, -56), 0, 1),
                              ((-98, 0, -78), 0, .9),
                              ((-150, 8, -31), 0, 1),
                              ((-160, 16, 70), 0, 1.2),
                              ((-79, 24, 152), PI / 2, 1),
                              ((56, 24, 144), PI / 2, 1),
                              ((152, 24, 77), 0, 1.2),
                              ((50, 24, 0), PI / 2, 1),
                              ((0, 72, 17), 0, 1),
                              ((0, 134, -32), 0, 1)]:
            self.put("root_portal", p, (scale, 1, 1), yaw)
        for y in (0, 24, 48):
            self.put("floor_roots", (0, y, 0), yaw=PI / 2 if y == 24 else 0)
            self.put("bench", (-18, y, 12), yaw=PI / 2)
            self.put("scroll_shelf", (18, y, 18), yaw=PI)
        # Emissive direction lines mark selected progression thresholds.
        for p, yaw in [((-44, 0, -78), 0), ((-160, 16, 67), 0),
                       ((-87, 24, 152), PI / 2), ((56, 24, 144), PI / 2),
                       ((50, 24, 0), PI / 2), ((0, 134, -33), 0)]:
            self.put("inlay", p, (.22, 1, 6), yaw)

    def lighting(self):
        # Pools of light at route decisions and elevation changes. Dark rooms rely
        # on their threshold light and emissive props, not evenly spaced lamps.
        lamps = {
            "lantern_amber": [(-25, 0, -46), (-47, 0, -68), (-93, 0, -83),
                              (-142, 8, -60), (-164, 8, 36), (-164, 16, 66),
                              (-147, 16, 89), (-133, 16, 116), (-98, 16, 74),
                              (-127, 16, 150), (-96, 24, 150),
                              (-47, 24, 158), (60, 24, 140.75), (98, 24, 152),
                              (119, 24, 121), (147.75, 24, 83), (73, 24, -12),
                              (49, 24, -3), (-5, 72, 12), (5, 72, 12)],
            "lantern_cyan": [(-165, 0, -111), (-188, 0, -82), (-109, 8, -33),
                             (-185, 8, -3), (-195, 16, 57), (-187, 16, 135),
                             (-96, 16, 126), (-36, 24, 217), (15, 24, 203),
                             (75, 24, 206), (162, 24, 179), (181, 24, 126),
                             (135, 24, 50), (168, 24, 26), (200, 24, 1),
                             (200, 32, 33), (200, 32, 74), (164, 24, 84),
                             (125, 24, -12), (165, 24, -48), (40, 16, -82),
                             (0, 8.2, -84), (-18, 0, -12), (18, 24, -12),
                             (-18, 48, 12), (18, 72, -12)],
            "arena_beacon": [(-29, 134, -12), (29, 134, -12),
                             (-29, 134, 12), (29, 134, 12)],
        }
        for asset, positions in lamps.items():
            for p in positions:
                self.put(asset, p, light=True)
        for asset, indices in (("stairs", (40, 140, 240, 320)),
                               ("roof_stairs", (20, 80, 140, 220, 280))):
            steps = read(MODELS / (asset + ".json"))["shapes"]
            for i in indices:
                s = steps[i]
                p = list(s["position"])
                p[1] += s["scale"][1] / 2
                # Local Z is the tread's cross direction, including the roof helix.
                p[0] += math.sin(s["rotation"][1]) * 3.1
                p[2] += math.cos(s["rotation"][1]) * 3.1
                self.put("roof_lantern", p, light=True)


def open_tower():
    path = MODELS / "stem_shell.json"
    data = read(path)
    # Idempotent: the old 90-high eastern panel is replaced with two pieces.
    for i, part in enumerate(data["shapes"]):
        if abs(part["position"][0] - 42) < .01 and part["scale"][1] == 90:
            below, above = copy.deepcopy(part), copy.deepcopy(part)
            below["position"][1], below["scale"][1] = 12, 24
            above["position"][1], above["scale"][1] = 64, 52
            data["shapes"][i:i + 1] = [below, above]
            path.write_bytes((json.dumps(data, indent=4) + "\n").replace("\n", "\r\n").encode())
            break
    # The first roof tread used to present a .584 lip to the last spiral turn.
    # Lower just this tread so the transition respects the controller's .5 step.
    path = MODELS / "roof_stairs.json"
    data = read(path)
    data["shapes"][0]["position"][1] = 71.6
    path.write_bytes((json.dumps(data, indent=4) + "\n").replace("\n", "\r\n").encode())
    # Lift the broad gill skirt clear of the outgoing roof stairs. Cap freckles
    # retain their original positions on the outside of roof_shell.
    path = MODELS / "elder.json"
    data = read(path)
    for part in data["shapes"]:
        if part["meshType"] == "Cube":
            part["position"][1] = 93
        elif part["scale"][0] == 152:
            part["position"][1] = 95
    path.write_bytes((json.dumps(data, indent=4) + "\n").replace("\n", "\r\n").encode())


def build(phase):
    kingdom = Kingdom()
    kingdom.rooms()
    kingdom.routes()
    kingdom.tower()
    kingdom.architecture()
    kingdom.masonry_joints()
    if phase in ("decorated", "lit"):
        kingdom.decorate()
    if phase == "lit":
        kingdom.lighting()
    return kingdom


def rotate(v, r):
    x, y, z = v
    a, b, c = r
    y, z = y * math.cos(a) - z * math.sin(a), y * math.sin(a) + z * math.cos(a)
    x, z = x * math.cos(b) + z * math.sin(b), -x * math.sin(b) + z * math.cos(b)
    x, y = x * math.cos(c) - y * math.sin(c), x * math.sin(c) + y * math.cos(c)
    return x, y, z


def unrotate(v, r):
    x, y, z = v
    a, b, c = r
    x, y = x * math.cos(c) + y * math.sin(c), -x * math.sin(c) + y * math.cos(c)
    x, z = x * math.cos(b) - z * math.sin(b), x * math.sin(b) + z * math.cos(b)
    y, z = y * math.cos(a) + z * math.sin(a), -y * math.sin(a) + z * math.cos(a)
    return x, y, z


def transform(v, t):
    v = rotate([v[i] * t["scale"][i] for i in range(3)], t["rotation"])
    return tuple(v[i] + t["position"][i] for i in range(3))


def inverse(v, t):
    v = unrotate([v[i] - t["position"][i] for i in range(3)], t["rotation"])
    return tuple(v[i] / t["scale"][i] for i in range(3))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


def faces(mesh):
    # Match the engine's unit primitive descriptors. Faces point outward.
    if mesh == "Cube":
        return [
            [(-.5, .5, -.5), (-.5, .5, .5), (.5, .5, .5), (.5, .5, -.5)],
            [(-.5, -.5, -.5), (.5, -.5, -.5), (.5, -.5, .5), (-.5, -.5, .5)],
            [(-.5, -.5, .5), (.5, -.5, .5), (.5, .5, .5), (-.5, .5, .5)],
            [(.5, -.5, -.5), (-.5, -.5, -.5), (-.5, .5, -.5), (.5, .5, -.5)],
            [(.5, -.5, .5), (.5, -.5, -.5), (.5, .5, -.5), (.5, .5, .5)],
            [(-.5, -.5, -.5), (-.5, -.5, .5), (-.5, .5, .5), (-.5, .5, -.5)],
        ]
    if mesh in ("Cylinder", "Frustum"):
        bottom = [(math.sin(i * PI / 16) * .5, -.5, math.cos(i * PI / 16) * .5) for i in range(32)]
        radius = .25 if mesh == "Frustum" else .5
        top = [(math.sin(i * PI / 16) * radius, .5, math.cos(i * PI / 16) * radius) for i in range(32)]
        return [top, bottom[::-1]] + [[bottom[i], bottom[(i + 1) % 32], top[(i + 1) % 32], top[i]] for i in range(32)]
    if mesh == "Disc":
        return [[(math.sin(i * PI / 16) * .5, 0, math.cos(i * PI / 16) * .5) for i in range(32)]]
    if mesh == "Wedge":
        return [[(-.5, -.5, -.5), (-.5, -.5, .5), (.5, .5, .5), (.5, .5, -.5)],
                [(.5, -.5, .5), (.5, -.5, -.5), (.5, .5, -.5), (.5, .5, .5)]]
    raise AssertionError("Unhandled collidable mesh: " + mesh)


def polygon_area(points):
    return abs(sum(a[0] * b[1] - b[0] * a[1]
                   for a, b in zip(points, points[1:] + points[:1]))) / 2


def clip_polygon(subject, boundary):
    """Intersect convex face projections; touching edges have zero area."""
    winding = sum(a[0] * b[1] - b[0] * a[1]
                  for a, b in zip(boundary, boundary[1:] + boundary[:1]))
    sign = 1 if winding > 0 else -1
    for a, b in zip(boundary, boundary[1:] + boundary[:1]):
        def side(p):
            return sign * ((b[0] - a[0]) * (p[1] - a[1])
                           - (b[1] - a[1]) * (p[0] - a[0]))
        result = []
        for p, q in zip(subject, subject[1:] + subject[:1]):
            dp, dq = side(p), side(q)
            if dp >= 0:
                result.append(p)
            if (dp >= 0) != (dq >= 0):
                t = dp / (dp - dq)
                result.append(tuple(p[i] + (q[i] - p[i]) * t for i in range(2)))
        subject = result
        if not subject:
            break
    return subject


def coplanar_overlaps(objects, geometry=None):
    """Audit same-facing planar surfaces, including parts within one model.

    Opposing faces at butt joints are harmless. Curved decoration is excluded;
    this checks actual cube/cylinder/frustum/wedge/disc faces, not their bounds.
    If supplied, collision geometry excludes faces buried inside solid structure
    using an outward coverage sample at the overlap center, edges and vertices.
    """
    planes = collections.defaultdict(list)
    models = {}
    overlaps = []
    for oi, obj in enumerate(objects):
        path = (PREFAB.parent / obj["asset"]).resolve()
        if path not in models:
            models[path] = read(path)
        for si, part in enumerate(models[path]["shapes"]):
            if part["meshType"] not in {"Cube", "Cylinder", "Frustum", "Wedge", "Disc"}:
                continue
            for fi, face in enumerate(faces(part["meshType"])):
                vertices = [transform(transform(p, part), obj) for p in face]
                normal = cross(subtract(vertices[1], vertices[0]),
                               subtract(vertices[2], vertices[0]))
                length = math.sqrt(sum(v * v for v in normal))
                normal = tuple(v / length for v in normal)
                distance = sum(a * b for a, b in zip(normal, vertices[0]))
                key = tuple(round(v, 4) for v in (*normal, distance))
                axis = max(range(3), key=lambda i: abs(normal[i]))
                projected = [tuple(p[i] for i in range(3) if i != axis) for p in vertices]
                lo = tuple(min(p[i] for p in projected) for i in range(2))
                hi = tuple(max(p[i] for p in projected) for i in range(2))
                for other, olo, ohi, identity in planes[key]:
                    if any(lo[i] >= ohi[i] - 1e-5 or hi[i] <= olo[i] + 1e-5 for i in range(2)):
                        continue
                    intersection = clip_polygon(projected, other)
                    area = polygon_area(intersection) / abs(normal[axis])
                    if area > .001:
                        if geometry is not None:
                            center = tuple(sum(p[i] for p in intersection) / len(intersection) for i in range(2))
                            samples = [center]
                            for a, b in zip(intersection, intersection[1:] + intersection[:1]):
                                samples.extend([tuple(.95 * a[i] + .05 * center[i] for i in range(2)),
                                                tuple(.475 * (a[i] + b[i]) + .05 * center[i] for i in range(2))])
                            exposed = False
                            axes = [i for i in range(3) if i != axis]
                            for sample in samples:
                                p = [0, 0, 0]
                                for i, value in zip(axes, sample):
                                    p[i] = value
                                p[axis] = (distance - sum(normal[i] * p[i] for i in axes)) / normal[axis]
                                p = [p[i] + normal[i] * .002 for i in range(3)]
                                if not geometry.contains(p):
                                    exposed = True
                                    break
                            if not exposed:
                                continue
                        overlaps.append((identity, (oi, si, fi), area))
                planes[key].append((projected, lo, hi, (oi, si, fi)))
    return overlaps


class Geometry:
    def __init__(self, objects, labels, decorative=False):
        self.parts = []
        self.cells = collections.defaultdict(list)
        self.models = {}
        self.triangle_count = 0
        for oi, obj in enumerate(objects):
            path = (PREFAB.parent / obj["asset"]).resolve()
            self.models.setdefault(path, read(path))
            if obj["generateColliders"] == decorative:
                continue
            for si, part in enumerate(self.models[path]["shapes"]):
                points = [transform(transform((x, y, z), part), obj)
                          for x in (-.5, .5) for y in (-.5, .5) for z in (-.5, .5)]
                lo = [min(p[i] for p in points) for i in range(3)]
                hi = [max(p[i] for p in points) for i in range(3)]
                triangles = []
                for face in ([] if decorative else faces(part["meshType"])):
                    vertices = [transform(transform(v, part), obj) for v in face]
                    for j in range(1, len(vertices) - 1):
                        a, b, c = vertices[0], vertices[j], vertices[j + 1]
                        normal = cross(subtract(b, a), subtract(c, a))
                        norm = math.sqrt(sum(v * v for v in normal))
                        if norm > 1e-8 and normal[1] / norm > .5:
                            triangles.append((a, b, c))
                self.triangle_count += len(triangles)
                index = len(self.parts)
                self.parts.append((obj, part, lo, hi, triangles, f"{labels[oi]} [{oi}:{si}]"))
                for cx in range(math.floor(lo[0] / 8), math.floor(hi[0] / 8) + 1):
                    for cz in range(math.floor(lo[2] / 8), math.floor(hi[2] / 8) + 1):
                        self.cells[cx, cz].append(index)

    def nearby(self, x, z):
        return self.cells.get((math.floor(x / 8), math.floor(z / 8)), [])

    def contains(self, point):
        for index in self.nearby(point[0], point[2]):
            obj, part, lo, hi, _, _ = self.parts[index]
            if any(point[i] <= lo[i] or point[i] >= hi[i] for i in range(3)):
                continue
            p = inverse(inverse(point, obj), part)
            if not -.5 < p[1] < .5:
                continue
            mesh = part["meshType"]
            if mesh == "Cube" and abs(p[0]) < .5 and abs(p[2]) < .5:
                return True
            if mesh in {"Cylinder", "Frustum"}:
                radius = .5 if mesh == "Cylinder" else .5 - (p[1] + .5) * .25
                # Match the 32-sided mesh, rather than occluding with a smooth
                # cylinder that protrudes through its actual facets.
                if all(p[0] * math.sin((i + .5) * PI / 16) + p[2] * math.cos((i + .5) * PI / 16)
                       < radius * math.cos(PI / 32) for i in range(32)):
                    return True
            if mesh == "Wedge" and abs(p[0]) < .5 and abs(p[2]) < .5 and p[1] < p[0]:
                return True
        return False

    def support(self, x, z, expected):
        heights = []
        for index in self.nearby(x, z):
            _, _, lo, hi, triangles, label = self.parts[index]
            if hi[1] < expected - .55 or lo[1] > expected + .51:
                continue
            for a, b, c in triangles:
                denominator = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
                if abs(denominator) < 1e-10:
                    continue
                u = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / denominator
                v = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / denominator
                if min(u, v, 1 - u - v) >= -1e-5:
                    y = u * a[1] + v * b[1] + (1 - u - v) * c[1]
                    if abs(y - expected) <= .51:
                        heights.append(y)
        return max(heights) if heights else None

    def blocked(self, x, y, z):
        # Sample a conservative .45-radius, 2.4-high player envelope. This is a
        # static clearance audit, not a substitute for the capsule controller.
        for index in self.nearby(x, z):
            obj, part, lo, hi, triangles, label = self.parts[index]
            if hi[1] < y + .51 or lo[1] > y + 2.4:
                continue
            for height in (.55, 1.2, 2.35):
                for ox, oz in ((0, 0), (.45, 0), (-.45, 0), (0, .45), (0, -.45)):
                    p = inverse(inverse((x + ox, y + height, z + oz), obj), part)
                    if not -.49999 < p[1] < .49999:
                        continue
                    mesh = part["meshType"]
                    if mesh == "Cube" and abs(p[0]) < .49999 and abs(p[2]) < .49999:
                        return label
                    if mesh in ("Cylinder", "Frustum"):
                        radius = .5 if mesh == "Cylinder" else .5 - (p[1] + .5) * .25
                        if p[0] ** 2 + p[2] ** 2 < (radius - 1e-5) ** 2:
                            return label
                    if mesh == "Wedge" and abs(p[0]) < .5 and abs(p[2]) < .5 and p[1] < p[0] - 1e-5:
                        return label
                    if mesh in ("Sphere", "IcoSphere") and sum(v * v for v in p) < .24999:
                        return label
                    if mesh == "Crystal" and p[0] ** 2 + p[2] ** 2 < .16:
                        return label
                    if mesh == "Arch" and p[1] >= 0 and abs(p[2]) < .125 and .09 < p[0] ** 2 + p[1] ** 2 < .25:
                        return label
        return None


def validate(kingdom):
    prefab = read(PREFAB)
    errors = []
    assert set(prefab) == {"assetType", "name", "objects"}
    assert prefab["assetType"] == "Prefab"
    expected_fields = {"asset", "position", "rotation", "scale", "generateColliders", "generateLights"}
    shape_fields = {"meshType", "position", "rotation", "scale", "color", "materialIndex"}
    material_fields = {"albedo", "roughness", "metallic", "lightWrap", "shapeContrast",
                       "ambientStrength", "emissiveColor", "emissiveStrength"}
    allowed_meshes = {"Cube", "Cylinder", "Sphere", "IcoSphere", "Frustum", "Crystal", "Disc", "Arch", "Wedge"}
    for path in sorted(MODELS.glob("*.json")):
        model = read(path)
        assert set(model) <= {"name", "materials", "shapes", "lights"}, path
        assert isinstance(model["name"], str) and model["name"], path
        assert b"\n" not in path.read_bytes().replace(b"\r\n", b""), path
        for material in model["materials"]:
            assert set(material) == material_fields, path
            for field, value in material.items():
                if isinstance(value, list):
                    assert len(value) == 3 and all(math.isfinite(v) for v in value), path
                else:
                    assert math.isfinite(value), path
        for part in model["shapes"]:
            assert set(part) == shape_fields, path
            assert part["meshType"] in allowed_meshes, path
            assert 0 <= part["materialIndex"] < len(model["materials"]), path
            for field in ("position", "rotation", "scale"):
                assert len(part[field]) == 3 and all(math.isfinite(v) for v in part[field]), path
            assert min(part["scale"]) > 0, path
            assert len(part["color"]) == 4 and all(math.isfinite(v) for v in part["color"]), path
        for light in model.get("lights", []):
            assert set(light) == {"name", "type", "position", "color", "intensity", "radius", "castsShadow"}, path
            assert light["type"] == "Point" and light["radius"] > 0 and light["intensity"] >= 0, path
            assert math.isfinite(light["radius"]) and math.isfinite(light["intensity"]), path
            assert len(light["position"]) == 3 and all(math.isfinite(v) for v in light["position"]), path
            assert len(light["color"]) == 3 and all(math.isfinite(v) and v >= 0 for v in light["color"]), path
            assert type(light["castsShadow"]) is bool, path
        assert len({light["name"] for light in model.get("lights", [])}) == len(model.get("lights", [])), path
    assert b"\n" not in PREFAB.read_bytes().replace(b"\r\n", b""), PREFAB
    for obj in prefab["objects"]:
        assert set(obj) == expected_fields
        assert (PREFAB.parent / obj["asset"]).is_file(), obj
        assert type(obj["generateColliders"]) is bool and type(obj["generateLights"]) is bool
        for field in ("position", "rotation", "scale"):
            assert len(obj[field]) == 3 and all(math.isfinite(v) for v in obj[field])
        assert min(obj["scale"]) > 0
        if obj["generateLights"]:
            assert read((PREFAB.parent / obj["asset"]).resolve()).get("lights"), obj
    labels = kingdom.labels if len(prefab["objects"]) == len(kingdom.labels) else [Path(o["asset"]).stem for o in prefab["objects"]]
    geometry = Geometry(prefab["objects"], labels)
    visual_geometry = Geometry(prefab["objects"], labels, decorative=True)
    surface_overlaps = coplanar_overlaps(prefab["objects"], geometry)
    for a, b, area in surface_overlaps:
        errors.append(f"Exposed coplanar faces: {labels[a[0]]} [{a}] / "
                      f"{labels[b[0]]} [{b}], overlap area {area:.4f}")
    grounded_props = {"cluster", "nursery", "alchemy_table", "scroll_shelf", "bench",
                      "royal_throne", "waystone", "herald_banner"}
    for i, obj in enumerate(prefab["objects"]):
        if obj["generateLights"] or Path(obj["asset"]).stem in grounded_props:
            x, y, z = obj["position"]
            assert geometry.support(x, z, y) is not None, ("Floating fixture/furniture", i, obj["asset"], obj["position"])
    # Distinct rooms may meet at their boundary, but should not occupy one
    # another's floor area. Balconies are separately authored and intentionally
    # overlap the room beneath them.
    for i, (name, room) in enumerate(ROOMS.items()):
        for other_name, other in list(ROOMS.items())[i + 1:]:
            poly, other_poly = polygon(room), polygon(other)
            for x, z in poly:
                x = room[0] + (x - room[0]) * .999
                z = room[1] + (z - room[1]) * .999
                assert not inside(x, z, other_poly), ("Overlapping rooms", name, other_name)
    graph = collections.defaultdict(set)
    for source, target, _, _ in ROUTES:
        graph[source].add(target); graph[target].add(source)
    for source, target in (("Tower landing", "Lower tower crypt"), ("Tower landing", "Royal temple"), ("Royal temple", "Crown arena")):
        graph[source].add(target); graph[target].add(source)
    reached = set()
    stack = ["Pilgrim gate"]
    while stack:
        node = stack.pop()
        if node not in reached:
            reached.add(node); stack.extend(graph[node] - reached)
    assert reached == set(graph), set(graph) - reached
    loops = sum(map(len, graph.values())) // 2 - len(graph) + 1
    assert loops >= 3
    bounds = [[min(part[2][i] for part in geometry.parts), max(part[3][i] for part in geometry.parts)] for i in range(3)]
    assert bounds[0][0] >= -224 and bounds[0][1] <= 224 and bounds[2][0] >= -140 and bounds[2][1] <= 232, bounds
    # Even with the owning point at the far edge of its chunk, the prefab and
    # approach fit inside the existing Chebyshev streaming window (5 * 64).
    assert max(abs(bounds[0][0]), abs(bounds[0][1]), abs(bounds[2][0]), abs(bounds[2][1]), 200) + 64 < 5 * 64
    # Conservative center envelopes over the generator's entire +/- .16-radian
    # jitter range. Treat X and Z independently, so the envelope also covers
    # impossible corner combinations rather than missing an intermediate seed.
    import re
    config = (ROOT / "game/src/world/worldConfig.h").read_text()
    radius = float(re.search(r"c_mushroomDungeonRadius\s*=\s*([\d.]+)f", config)[1])
    def center_envelope(index, distance):
        angles = [.785398 + index * 1.570796 + offset for offset in (-.16, .16)]
        xs, zs = [math.cos(a) * distance for a in angles], [math.sin(a) * distance for a in angles]
        return min(xs), max(xs), min(zs), max(zs)
    owner = center_envelope(3, radius)
    reserved = [("spawn clearing", -90, 90, -56, 124)]
    for i in range(3):
        lo_x, hi_x, lo_z, hi_z = center_envelope(i, 190)
        reserved.extend([(f"boss {i} court", lo_x - 52, hi_x + 52, lo_z - 108, hi_z + 44),
                         (f"boss {i} approach", lo_x - 13, hi_x + 13, lo_z - 164, hi_z - 108)])
    for obj, part, lo, hi, triangles, label in geometry.parts:
        for name, xmin, xmax, zmin, zmax in reserved:
            overlap = (lo[0] + owner[0] < xmax and hi[0] + owner[1] > xmin
                       and lo[2] + owner[2] < zmax and hi[2] + owner[3] > zmin)
            assert not overlap, ("World reservation overlap", label, name)
    samples = 0
    for name, points in kingdom.walks:
        path_errors = []
        pairs = list(zip(points, points[1:])) or [(points[0], points[0])]
        previous_height = None
        for a, b in pairs:
            length = math.dist(a, b)
            count = max(1, math.ceil(length / .4))
            for i in range(count + 1):
                t = i / count
                x, y, z = (a[k] + (b[k] - a[k]) * t for k in range(3))
                samples += 1
                support = geometry.support(x, z, y)
                if support is None:
                    path_errors.append(f"no support at ({x:.2f}, {y:.2f}, {z:.2f})")
                    continue
                obstacle = geometry.blocked(x, support, z)
                if obstacle:
                    path_errors.append(f"blocked at ({x:.2f}, {support:.2f}, {z:.2f}) by {obstacle}")
                visual = visual_geometry.blocked(x, support, z)
                if visual:
                    path_errors.append(f"visual obstruction at ({x:.2f}, {support:.2f}, {z:.2f}) by {visual}")
                if previous_height is not None and abs(support - previous_height) > .501:
                    path_errors.append(f"step exceeds .5 at ({x:.2f}, {support:.2f}, {z:.2f})")
                previous_height = support
                # Check the capsule's supporting footprint, not just its center.
                for ox, oz in ((.4, 0), (-.4, 0), (0, .4), (0, -.4)):
                    if geometry.support(x + ox, z + oz, support) is None:
                        path_errors.append(f"unsupported footprint at ({x + ox:.2f}, {support:.2f}, {z + oz:.2f})")
        if path_errors:
            errors.append(name + ": " + "; ".join(path_errors[:3]) + f" ({len(path_errors)} samples)")
    print(f"Schema/path checks: {len(list(MODELS.glob('*.json')))} models, {len(prefab['objects'])} objects; "
          f"{len(graph)} connected spaces, {loops} independent loops.")
    print(f"Geometry audit: {len(geometry.parts)} collidable parts, {geometry.triangle_count} upward triangles, "
          f"{samples} route samples, {len(errors) - len(surface_overlaps)} routes with issues.")
    print("Structural bounds:", [[round(v, 2) for v in axis] for axis in bounds])
    print("World placement: no overlap with the spawn clearing or other boss courts/approaches across seed jitter.")
    print(f"Planar surface audit: {len(surface_overlaps)} exposed coplanar overlap candidates "
          "(covered joins excluded by outward sampling).")
    for error in errors:
        print("ERROR:", error)
    if errors:
        raise SystemExit(1)
    print("PASS: all authored centerline routes have floor support and player-envelope clearance.")


def draw_map():
    DOCS.mkdir(exist_ok=True)
    colors = dict(ruins="#b9a48a", crypt="#9685a3", cavern="#659f99", nursery="#dbaf60",
                  roots="#81965b", alchemy="#b783ab", archive="#6b93b4", royal="#d4c37e")
    parts = ['<svg xmlns="http://www.w3.org/2000/svg" width="1440" height="1160" viewBox="0 0 1440 1160">',
             '<rect width="1440" height="1160" fill="#111a20"/>',
             '<g font-family="Segoe UI, sans-serif" fill="#e9e1ce">',
             '<text x="48" y="48" font-size="29">SPORENKRYPTA / THE FUNGAL KINGDOM</text>',
             '<text x="48" y="76" font-size="15" fill="#a6b9bb">Authored floor plan • X right / +Z up • all elevations relative to the entrance</text>']
    def point(x, z):
        return 504 + x * 2, 610 - z * 2
    def path(points):
        return " ".join(f"{x:.2f},{y:.2f}" for x, y in points)
    for source, target, width, points in ROUTES:
        coords = [point(p[0], p[2]) for p in points]
        parts.append(f'<polyline points="{path(coords)}" fill="none" stroke="#495b64" stroke-width="{width * 2}" stroke-linejoin="round"><title>{html.escape(source + " to " + target)}</title></polyline>')
        if any(a[1] != b[1] for a, b in zip(points, points[1:])):
            for a, b in zip(points, points[1:]):
                if a[1] != b[1]:
                    coords = [point(p[0], p[2]) for p in (a, b)]
                    parts.append(f'<polyline points="{path(coords)}" stroke="#f3df9c" stroke-width="4" stroke-dasharray="2 5"/>')
    for index, (name, room) in enumerate(ROOMS.items(), 1):
        x, z, y, w, d, h, plan, district, category = room
        points = [point(px, pz) for px, pz in polygon(room)]
        parts.append(f'<polygon points="{path(points)}" fill="{colors[district]}" fill-opacity=".7" stroke="{colors[district]}" stroke-width="2"><title>{html.escape(name)}: floor {y}, {w} × {d}, height {h}</title></polygon>')
        cx, cz = point(x, z)
        parts.append(f'<text x="{cx}" y="{cz}" text-anchor="middle" font-size="16" font-weight="bold">{index:02}</text>')
        parts.append(f'<text x="{cx}" y="{cz + 17}" text-anchor="middle" font-size="12">Y {y}</text>')
        label_y = 155 + (index - 1) * 26
        parts.append(f'<rect x="1000" y="{label_y - 12}" width="9" height="12" fill="{colors[district]}"/>')
        suffix = " / side area" if category == "optional" else ""
        parts.append(f'<text x="1018" y="{label_y}" font-size="15">{index:02}  {html.escape(name + suffix)}</text>')
    cx, cz = point(0, 0)
    for radius in (76, 42, 30):
        parts.append(f'<circle cx="{cx}" cy="{cz}" r="{radius * 2}" fill="#776a4e" fill-opacity=".12" stroke="#cdb779" stroke-width="2" stroke-dasharray="6 5"/>')
    for offset, text in ((-31, "THE ELDER STEM"), (-8, "Tower 0 / 24 / 48"), (15, "Royal temple 72"), (38, "Crown arena 134")):
        parts.append(f'<text x="{cx}" y="{cz + offset}" text-anchor="middle" font-size="16">{text}</text>')
    ax, az = point(0, -200)
    bx, bz = point(0, -44)
    parts.append(f'<line x1="{ax}" y1="{az}" x2="{bx}" y2="{bz}" stroke="#d8cdad" stroke-width="5" stroke-dasharray="4 7"/>')
    parts.append(f'<text x="{ax + 15}" y="{az}" font-size="15">Existing forest approach</text>')
    parts.extend(['<text x="48" y="1080" font-size="17">Main progression</text>',
                  '<text x="48" y="1108" font-size="15">01 → 02 → 03 → 06 → 08 → 09 → 14 → 17 → 19 → 22 → 23 → 25 → 26 → tower → temple → crown</text>',
                  '<text x="48" y="1134" font-size="14" fill="#a6b9bb">Dashed gold: stairs / ramp. Dashed crown: tower canopy above the lower districts. Diagram is an authoring map, not a runtime screenshot.</text>',
                  '<text x="1000" y="935" font-size="17">6 independent exploration loops</text>',
                  '<text x="1000" y="963" font-size="14">Crypt / watch · seed bank / belfry</text>',
                  '<text x="1000" y="987" font-size="14">Nursery / grafting · root / distillery</text>',
                  '<text x="1000" y="1011" font-size="14">Archive / overlook · entrance return</text>',
                  '</g></svg>'])
    (DOCS / "mushroom-dungeon-map.svg").write_text("\n".join(parts) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("blockout", "decorated", "lit"))
    args = parser.parse_args()
    if args.phase:
        modules()
        open_tower()
        kingdom = build(args.phase)
        write_json(PREFAB, dict(assetType="Prefab", name="Sporenkrypta - Das unterirdische Pilzkoenigreich",
                                objects=kingdom.objects))
        print(f"Exported {args.phase}: {len(kingdom.objects)} placed modules.")
    else:
        kingdom = build("lit")
    validate(kingdom)
    if args.phase:
        draw_map()


if __name__ == "__main__":
    main()
