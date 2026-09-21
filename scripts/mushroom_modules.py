"""Author the procedural mushroom rooms. Run --write to export, otherwise audit.

Rooms use a 48-unit socket grid, 40-unit interiors and ten-unit doorways.
Four sparse floors fit inside an ancient hollow elder mushroom. Each stair joins one floor pair.
The runtime closes unused sockets with doorway_seal.prefab.json. All four
axes and the combat spawn pockets (+/-7, +/-7) remain clear of furniture.
This is separate from mushroom_dungeon.py, which authors the legacy landmark.
"""

import argparse
import json
import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MODELS = ROOT / "game/assets/models/mushroom_dungeon"
PREFABS = ROOT / "game/assets/prefabs/mushroom"
PI = math.pi
BOSS_HEIGHT = 234
FLOOR_SPACING = BOSS_HEIGHT / 3
FLOOR_HEIGHTS = tuple(i * FLOOR_SPACING for i in range(4))


def write(path, data):
    import re
    text = json.dumps(data, indent=4, ensure_ascii=False)
    text = re.sub(r"\[\s+([-\d.,eE+\s]+)\s+\]",
                  lambda match: "[" + " ".join(match[1].split()) + "]", text)
    path.write_bytes((text + "\n").replace("\n", "\r\n").encode("utf-8"))


def shape(mesh, position, scale, material=1, yaw=0, pitch=0):
    return dict(meshType=mesh, position=position, scale=scale,
                rotation=[pitch, yaw, 0], color=[1, 1, 1, 1], materialIndex=material)


def rotate(x, z, yaw):
    return (round(x * math.cos(yaw) + z * math.sin(yaw), 6),
            round(-x * math.sin(yaw) + z * math.cos(yaw), 6))


def object_(asset, position=(0, 0, 0), scale=(1, 1, 1), yaw=0,
            collider=False, light=False):
    return dict(asset="../../models/mushroom_dungeon/" + asset + ".json",
                position=list(position), rotation=[0, yaw, 0], scale=list(scale),
                generateColliders=collider, generateLights=light)



def subtract(a, b):
    return [a[i] - b[i] for i in range(3)]


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]]


def triangle(parts, a, b, c, material):
    """Split at the longest edge's altitude into two disjoint right triangles.

    The shared endpoints are authored once; no inflated boxes, backing planes,
    duplicate backfaces or depth offsets are needed to close the shell.
    """
    vertices = [a, b, c]
    edge = max(range(3), key=lambda i: dot(subtract(vertices[(i + 1) % 3], vertices[i]),
                                         subtract(vertices[(i + 1) % 3], vertices[i])))
    a, b, c = (vertices[(edge + i) % 3] for i in range(3))
    ab = subtract(b, a)
    length = math.sqrt(dot(ab, ab))
    axis = [v / length for v in ab]
    distance = dot(subtract(c, a), axis)
    foot = [a[i] + axis[i] * distance for i in range(3)]
    for origin, u, v in ((foot, b, c), (foot, c, a)):
        x, y = subtract(u, origin), subtract(v, origin)
        sx, sy = math.sqrt(dot(x, x)), math.sqrt(dot(y, y))
        if sx < 1e-7 or sy < 1e-7:
            continue
        x, y = [t / sx for t in x], [t / sy for t in y]
        z = cross(x, y)
        yaw = math.asin(max(-1, min(1, -x[2])))
        if abs(math.cos(yaw)) > 1e-7:
            pitch, roll = math.atan2(y[2], z[2]), math.atan2(x[1], x[0])
        else:
            pitch, roll = 0, math.atan2(-y[0], y[1])
        part = shape("Triangle", origin, [sx, sy, 1], material, yaw, pitch)
        part["rotation"][2] = roll
        parts.append(part)


def quad(parts, a, b, c, d, material):
    triangle(parts, a, b, c, material)
    triangle(parts, a, c, d, material)


def spotted_quad(parts, a, b, c, d):
    # Partition the face around an inset cream octagon. The spot replaces
    # surface triangles instead of lying on top of the red cap.
    def point(u, v):
        return [(1 - v) * ((1 - u) * a[i] + u * b[i])
                + v * ((1 - u) * d[i] + u * c[i]) for i in range(3)]

    boundary = [(1, .5), (1, 1), (.5, 1), (0, 1), (0, .5), (0, 0), (.5, 0), (1, 0)]
    outer = [point(u, v) for u, v in boundary]
    inner = [point(.5 + .33 * math.cos(i * PI / 4), .5 + .33 * math.sin(i * PI / 4))
             for i in range(8)]
    center = point(.5, .5)
    for i in range(8):
        j = (i + 1) % 8
        quad(parts, outer[i], outer[j], inner[j], inner[i], 2)
        triangle(parts, center, inner[i], inner[j], 14)


def radial(radius, height, sector):
    # Integer sectors wrap to exactly the same coordinate at the seam.
    angle = (sector % 64 - .5) * PI * 2 / 64
    return [math.sin(angle) * radius, height, math.cos(angle) * radius]


def outside_shaft(polygon):
    """Clip a convex crown tile around the rectangular staircase opening."""
    remaining = polygon
    for axis, bound, direction in ((0, -25, 1), (0, 25, -1), (2, -73, 1), (2, -23, -1)):
        inside, outside = [], []
        for a, b in zip(remaining, remaining[1:] + remaining[:1]):
            da, db = direction * (a[axis] - bound), direction * (b[axis] - bound)
            (inside if da >= 0 else outside).append(a)
            if (da < 0) != (db < 0):
                t = da / (da - db)
                point = [a[i] + (b[i] - a[i]) * t for i in range(3)]
                inside.append(point)
                outside.append(point)
        if len(outside) >= 3:
            yield outside
        remaining = inside
        if not remaining:
            break


def export():
    palette = json.loads((MODELS / "floor_plate.json").read_text())["materials"]
    shell = [shape("Cube", [0, -.6, 0], [40, 1.2, 40], 5)]
    for side in range(4):
        yaw = side * PI / 2
        for x, y, z, scale, material in [
            (0, -.6, 22, [10, 1.2, 4], 5),
            (-12.5, 7, 19.5, [15, 14, 1], 1),
            (12.5, 7, 19.5, [15, 14, 1], 1),
            (0, 12, 19.5, [10, 4, 1], 6),
            (0, 14.3, 19.5, [40, .6, 1.8], 7),
            (-5.5, 4.5, 22, [1, 9, 4], 6),
            (5.5, 4.5, 22, [1, 9, 4], 6),
        ]:
            px, pz = rotate(x, z, yaw)
            shell.append(shape("Cube", [px, y, pz], scale, material, yaw))
    write(MODELS / "module_shell.json", dict(name="Sporenkrypta Raumhuelle",
          materials=palette, shapes=shell))

    # Living tissue carries each room and continues underneath its four sockets.
    # It stays inside the reserved footprint and cannot block the walkable floor.
    tissue = []
    for d in range(4):
        yaw = d * PI / 2
        for x, y, z, scale, material in [
            (0, -4, 14, [12, 7, 20], 1),
            (-11, -6, 8, [7, 13, 26], 10 if len(palette) > 10 else 1),
        ]:
            px, pz = rotate(x, z, yaw)
            tissue.append(shape("Sphere", [px, y, pz], scale, material, yaw))
    write(MODELS / "module_mycelium.json", dict(name="Living root supports", materials=palette, shapes=tissue))

    # Giant mushrooms are local assets with a ground origin, unlike the legacy elder.
    giant = [shape("Cylinder", [0, 8, 0], [2.4, 16, 2.4], 1),
             shape("Sphere", [0, 17, 0], [13, 4, 13], 2),
             shape("Sphere", [0, 15.6, 0], [11, .6, 11], 3)]
    for i in range(7):
        a = i * PI * 2 / 7
        giant.append(shape("Sphere", [math.sin(a) * 4.2, 18.5, math.cos(a) * 4.2],
                           [1.3, .4, 1.3], 4))
    write(MODELS / "module_giant.json", dict(name="Leuchtender Riesenschirm",
          materials=palette, shapes=giant))

    def save(name, title, objects):
        write(PREFABS / (name + ".prefab.json"),
              dict(assetType="Prefab", name=title, objects=objects))

    # Warm woodland colours belong only to the exterior; room assets keep
    # their own palette. Material indices stay shared by all four shell models.
    import copy
    exterior_palette = copy.deepcopy(palette)

    def finish_material(index, albedo, glow, strength):
        exterior_palette[index].update(
            albedo=albedo, emissiveColor=glow, emissiveStrength=strength,
            roughness=.88, lightWrap=.42, ambientStrength=1.3, shapeContrast=.85)

    finish_material(1, [.86, .75, .57], [.65, .48, .29], .12)
    finish_material(2, [.72, .23, .16], [.55, .19, .10], .16)
    finish_material(3, [.48, .69, .55], [.34, .64, .44], .28)
    finish_material(4, [1.0, .72, .34], [1.0, .58, .22], .85)
    for albedo, glow, strength in [
        ([.83, .38, .24], [.60, .25, .13], .16),  # 9: rolled coral lip
        ([.48, .32, .20], [.36, .23, .13], .12),  # 10: warm bark
        ([.34, .46, .24], [.22, .32, .13], .10),  # 11: sage moss
        ([1.0, .83, .52], [1.0, .68, .32], .70),  # 12: honey window glass
        ([.91, .79, .61], [.74, .56, .35], .24),  # 13: ivory lamellae
        ([.98, .88, .66], [.77, .61, .36], .18),  # 14: cap freckles
    ]:
        exterior_palette.append(copy.deepcopy(exterior_palette[1]))
        finish_material(len(exterior_palette) - 1, albedo, glow, strength)

    export_landmark(exterior_palette, save)

    save("doorway_seal", "Verwachsene blinde Tuer", [
        object_("wall_bay", (0, 0, 19.5), (10, 10, 1), collider=True),
        object_("cluster", (-2, 0, 18), (.8, .8, .8)),
        object_("herald_banner", (0, 7, 18.8), (.7, .7, .7)),
    ])

    titles = {
        "entrance": "Tor der Sporenpilger", "corridor": "Leuchtender Myzelgang",
        "turn": "Wurzelkreuzgang", "small_combat": "Hof der Sporenwaechter",
        "large_combat": "Sporenkathedrale", "side_room": "Verborgene Pilgerklause",
        "nursery": "Bernsteinbrutstaette", "archive": "Archiv der stillen Sporen",
        "alchemy": "Myzel-Destillerie", "shrine": "Heiligtum der Wurzelkrone",
        "grotto": "Grotte der Riesenschirme",
    }
    for name, title in titles.items():
        objects = [object_("module_shell", collider=True), object_("module_mycelium")]
        warm = name in ("entrance", "nursery", "alchemy", "shrine", "side_room")
        lantern = "lantern_amber"
        for x, z in [(-16, -16), (16, 16), (-16, 16), (16, -16)]:
            objects.append(object_("fungal_pier", (x, 0, z), (.65, 1.25, .65), collider=True))
            objects.append(object_("cluster", (x * .82, 0, z * .82), (1.2, 1.2, 1.2)))
        for x, z in [(-16, -16), (16, 16)]:
            objects.append(object_(lantern, (x, 7, z), (1.2, 1.2, 1.2), light=True))
        # Warm pools at the walls and a small cool accent give the open rooms depth.
        if not warm:
            objects.append(object_("lantern_cyan", (15, 3, -15), (.7, .7, .7), light=True))
        for x, z in ((-15, 15), (15, -15)):
            objects.append(object_("hanging_spores", (x, 13, z), (1.1, 1.1, 1.1)))
        for side in range(4):
            yaw = side * PI / 2
            x, z = rotate(0, 19.4, yaw)
            objects.append(object_("root_portal", (x, 0, z), (1.2, 1.2, .75), yaw))
            x, z = rotate(-3.8, 12, yaw)
            objects.append(object_("inlay", (x, 0, z), (.12, 1, 23), yaw))

        # Furniture occupies quadrants, leaving a broad cross and four spawn pockets.
        if name == "nursery":
            for x in (-12, 12):
                for z in (-11, 11):
                    objects.append(object_("nursery", (x, 0, z), (1.4, 1.4, 1.4), collider=True))
                    objects.append(object_("hanging_spores", (x, 12, z), (1.7, 1.7, 1.7)))
        elif name == "archive":
            for x in (-17, 17):
                for z in (-10, 10):
                    objects.append(object_("scroll_shelf", (x, 0, z), (1.6, 1.6, 1.6), PI / 2, True))
            for x in (-11, 11):
                objects.append(object_("bench", (x, 0, 12), (1.3, 1.3, 1.3), collider=True))
        elif name == "alchemy":
            for x in (-12, 12):
                for z in (-11, 11):
                    objects.append(object_("alchemy_table", (x, 0, z), (1.5, 1.5, 1.5), collider=True))
                    objects.append(object_("growth_shelf", (x, 0, z * 1.48), (.8, .8, .8)))
        elif name == "grotto":
            for x, z, size in [(-12, 12, 1.05), (12, -12, .85), (12, 12, .65)]:
                objects.append(object_("module_giant", (x, 0, z), (size, size, size), collider=True))
            for x in (-11, 11):
                objects.append(object_("hanging_spores", (x, 15, 11), (2, 2, 2)))
        elif name in ("shrine", "side_room"):
            for x in (-12, 12):
                objects.append(object_("royal_throne" if name == "shrine" else "bench",
                                       (x, 0, 12), (.7, .7, .7), collider=True))
                objects.append(object_("arena_beacon", (x, 0, -12), (.65, .65, .65)))
            objects.append(object_("arena_mandala", scale=(.8, .8, .8)))
        elif name == "large_combat":
            for x in (-13, 13):
                objects.append(object_("vault_rib", (x, 0, 0), (1.65, 1.6, .6), PI / 2))
                objects.append(object_("herald_banner", (x, 10, 11), (1.6, 1.6, 1.6)))
            objects.append(object_("arena_mandala", scale=(1.35, 1, 1.35)))
        elif name == "entrance":
            for x in (-12, 12):
                objects.append(object_("waystone", (x, 0, -12), (1.3, 1.3, 1.3), collider=True))
                objects.append(object_("herald_banner", (x, 8, 12), (2, 2, 2)))
        else:
            for x, z in [(-12, 12), (12, -12)]:
                objects.append(object_("module_giant", (x, 0, z), (.55, .55, .55), collider=True))
                objects.append(object_("hanging_spores", (x, 12, z), (1.3, 1.3, 1.3)))
        save(name, title, objects)

    export_transition(palette, save)

    # The arena is local to its graph floor. A square floor meets the fitted
    # approach across the entire ten-unit doorway (a circle only meets at a point).
    arena = [object_("floor_plate", scale=(76, 2, 76), collider=True),
             object_("arena_mandala"), object_("royal_throne", (0, 0, 26), collider=True),
             object_("roof_lantern", (0, 11, 0), light=True)]
    for i in range(24):
        angle = i * PI * 2 / 24
        x, z = math.sin(angle) * 36, math.cos(angle) * 36
        if z < -30 and abs(x) < 14:
            continue
        arena.append(object_("parapet", (x, 0, z), (9.6, 1, 1), angle, True))
    for x in (-26, 26):
        for z in (-16, 16):
            arena.append(object_("arena_beacon", (x, 0, z), (.9, .9, .9), light=True))
    save("boss_arena", "Ancient heart of the Spore Crown", arena)

    approach = [object_("floor_plate", (0, 0, -7), (10, 1.2, 34), collider=True),
                object_("root_portal", (0, 0, -18), (1.2, 1.2, .8))]
    for x in (-6, 6):
        approach.append(object_("parapet", (x, 0, -7), (34, 1, 1), PI / 2, True))
        approach.append(object_("lantern_cyan", (x, 4, -15), light=True))
    save("boss_approach", "Bridge to the ancient heart", approach)


def export_transition(palette, save):
    # Three circuits, twelve flights: 78 units, .1625 risers, .8 treads.
    tower = [object_("floor_plate", scale=(48, 1.2, 48), collider=True)]
    corners = [(-19, -19), (-19, 19), (19, 19), (19, -19)]
    for flight in range(12):
        x, z = corners[flight % 4]
        nx, nz = corners[(flight + 1) % 4]
        height = flight * FLOOR_SPACING / 12
        yaw = math.atan2(nx - x, nz - z)
        tower.append(object_("floor_plate", (x, height, z), (6, 1, 6), collider=True))
        tower.append(object_("stair_flight", ((x + nx) / 2, height, (z + nz) / 2),
                             (.75, FLOOR_SPACING / 96, 32 / 24), yaw, True))
        if flight % 4 == 0:
            tower.append(object_("lantern_cyan", (x, height + 3, z), light=True))
    tower.extend([
        object_("floor_plate", (-19, FLOOR_SPACING, -19), (6, 1, 6), collider=True),
        object_("floor_plate", (-19, FLOOR_SPACING, 0), (6, 1, 32), collider=True),
        object_("floor_plate", (-19, FLOOR_SPACING, 19), (6, 1, 6), collider=True),
        object_("floor_plate", (-8, FLOOR_SPACING, 19), (16, 1, 6), collider=True),
        object_("floor_plate", (0, FLOOR_SPACING, 20), (10, 1, 8), collider=True),
        object_("fungal_pier", (0, 0, 0), (1.5, 5.5, 1.5), collider=True),
        object_("root_portal", (0, 0, -23), (1.2, 1.2, .7)),
        object_("root_portal", (0, FLOOR_SPACING, 23), (1.2, 1.2, .7)),
    ])
    # Closed shaft prevents unintended side exits and floor skipping. Only the
    # south bottom and north top sockets open; rotation moves both together.
    walls = []
    for d in range(4):
        yaw = d * PI / 2
        bands = [(0, 92, -14.5, 19), (0, 92, 14.5, 19)]
        if d == 2:
            bands += [(10, 92, 0, 10)]
        elif d == 0:
            bands += [(0, FLOOR_SPACING, 0, 10), (FLOOR_SPACING + 10, 92, 0, 10)]
        else:
            bands += [(0, 92, 0, 10)]
        for low, high, x, width in bands:
            px, pz = rotate(x, 23.5, yaw)
            walls.append(shape("Cube", [px, (low + high) / 2, pz], [width, high - low, 1], 1, yaw))
    write(MODELS / "transition_shell.json", dict(name="One-floor fungal stair shaft", materials=palette, shapes=walls))
    tower.append(object_("transition_shell", collider=True))
    save("vertical_transition", "Ascent through living mycelium", tower)


def export_landmark(palette, save):
    # A fluted stem opens into deep, descending lamellae. Keep all relief outside
    # the room envelope and join crown, gills and stem at identical vertices.
    for material in palette:
        material.update(metallic=0, emissiveStrength=0, roughness=.88,
                        lightWrap=.35, ambientStrength=1.1, shapeContrast=.85)
    palette[1].update(albedo=[.81, .73, .59])
    palette[2].update(albedo=[.54, .20, .12], roughness=.94)
    palette[3].update(albedo=[.40, .57, .46])
    palette[9].update(albedo=[.68, .32, .18])
    palette[10].update(albedo=[.30, .26, .17])
    palette[11].update(albedo=[.24, .34, .17], roughness=1)
    palette[12].update(albedo=[.49, .65, .51], emissiveColor=[.28, .65, .46], emissiveStrength=.25)
    palette[13].update(albedo=[.92, .80, .60], ambientStrength=1.12)
    palette[14].update(albedo=[.82, .70, .48])
    import copy
    palette.append(copy.deepcopy(palette[13]))
    palette[15].update(albedo=[.42, .29, .17], emissiveStrength=0, ambientStrength=.95)
    for albedo, glow, strength in (
            ([.58, .42, .27], [.52, .30, .12], .035),  # 16: warm inner tissue
            ([.34, .25, .18], [.34, .22, .12], .015),  # 17: recessed folds
            ([.38, .60, .43], [.24, .65, .40], .45)):  # 18: living mycelium
        palette.append(copy.deepcopy(palette[1]))
        palette[-1].update(albedo=albedo, emissiveColor=glow, emissiveStrength=strength,
                           roughness=.95, ambientStrength=1.15)
    sectors = 64

    def surface(radius, y, i, cap=False):
        # Subdivided borders stay on the neighbouring face's straight edge.
        sector = math.floor(i)
        fraction = i - sector
        if fraction > 1e-8:
            a, b = surface(radius, y, sector, cap), surface(radius, y, sector + 1, cap)
            return [a[j] + (b[j] - a[j]) * fraction for j in range(3)]
        if cap and radius == 244 and y == 198:
            return surface(radius, y, i)
        a = (i % sectors - .5) * 2 * PI / sectors
        bend = 16 * math.sin(y / 250)
        wobble = 1 + .009 * math.sin(3 * a + .4) + .006 * math.cos(5 * a + y * .004)
        if cap:
            wobble += .055 * math.sin(a + .8) + .025 * math.cos(3 * a - .5)
            skirt = min(1, radius / 300) * min(1, max(0, (335 - y) / 150))
            # Broad waves and three worn notches continue across the lip and gills.
            wear = sum(math.exp(-(math.atan2(math.sin(a - notch), math.cos(a - notch)) / .12) ** 2)
                       for notch in (.85, 2.6, 4.35))
            radius -= skirt * wear * 5
            y += skirt * (9 * math.sin(a - .4) + 5 * math.sin(5 * a + .7) + 14 * wear)
        else:
            # Outward-only fibres preserve clearance and continue into the gills.
            radius += (5 + 5 * math.sin(PI * max(0, y) / 396)) * (1 + math.cos(12 * a + y * .009))
            # Buttress the foot while keeping the entrance gallery exposed.
            gate_distance = math.atan2(math.sin(a - PI), math.cos(a - PI))
            radius += 18 * math.exp(-(y / 32) ** 2) * (1 - math.exp(-(gate_distance / .25) ** 2))
        return [bend + math.sin(a) * radius * wobble,
                y + (3 * math.sin(2 * a + .7) + 2 * math.cos(3 * a) if cap else 0),
                -3 * math.sin(y / 170) + math.cos(a) * radius * wobble]

    stem = []
    stem_rings = [(262, -140), (252, -8), (250, 0), (244, 24), (241, 65),
                  (242, 110), (242, 155), (244, 198)]
    for (r0, y0), (r1, y1) in zip(stem_rings, stem_rings[1:]):
        for i in range(sectors):
            gate = i == sectors // 2
            if gate and y0 < 24:
                continue
            for inset, reverse in ((0, False), (4, True)):
                points = [surface(r0 - inset, y0, i), surface(r0 - inset, y0, i + 1),
                          surface(r1 - inset, y1, i + 1), surface(r1 - inset, y1, i)]
                if reverse:
                    points.reverse()
                material = 16 if reverse else (10 if y1 <= 0 else 1)
                quad(stem, *points, material)
            if gate and y0 == 24:
                quad(stem, surface(r0, y0, i), surface(r0 - 4, y0, i),
                     surface(r0 - 4, y0, i + 1), surface(r0, y0, i + 1), 1)
    write(MODELS / "elder_hollow_stem.json", dict(name="Fluted ivory elder stem", materials=palette, shapes=stem))

    # A solid earthen base follows the irregular inner wall. Its top sits just
    # below the room floors and entrance gallery, avoiding coplanar surfaces.
    floor = []
    floor_top, floor_bottom = -.15, -4
    floor_center = [0, floor_top, 0]
    floor_under = [0, floor_bottom, 0]

    def floor_edge(sector, height):
        low = surface(249, -8, sector)
        high = surface(247, 0, sector)
        t = (floor_top + 8) / 8
        return [low[0] + (high[0] - low[0]) * t, height,
                low[2] + (high[2] - low[2]) * t]

    for sector in range(sectors):
        a, b = floor_edge(sector, floor_top), floor_edge(sector + 1, floor_top)
        c, d = floor_edge(sector, floor_bottom), floor_edge(sector + 1, floor_bottom)
        triangle(floor, floor_center, a, b, 16)
        triangle(floor, floor_under, d, c, 17)
        quad(floor, a, c, d, b, 10)
    floor_palette = copy.deepcopy(palette)
    floor_palette[16].update(albedo=[.32, .27, .18], roughness=1, emissiveStrength=0)
    write(MODELS / "elder_floor.json", dict(name="Continuous earthen mycelium floor",
          materials=floor_palette, shapes=floor))

    cap, gills = [], []
    # A deep bell-shaped skirt exposes the spotted russet cap from ground level.
    # Its inward-rolled edge stays outside the rooms and below the stem shoulder.
    profile = [(0, 335), (65, 331), (132, 321), (200, 306), (256, 270),
               (294, 225), (321, 182), (328, 155), (324, 143), (311, 145)]
    lip_band = len(profile) - 2
    for band, ((r0, y0), (r1, y1)) in enumerate(zip(profile, profile[1:])):
        for i in range(sectors):
            # Half-sectors along the lip match the lamellae exactly.
            for half in range(2) if band == lip_band else range(1):
                step = .5 if band == lip_band else 1
                sector = i + half * step
                points = [surface(r0, y0, sector, True), surface(r1, y1, sector, True),
                          surface(r1, y1, sector + step, True), surface(r0, y0, sector + step, True)]
                material = 2 if band < lip_band - 1 else 9
                if r0 == 0:
                    triangle(cap, [16 * math.sin(335 / 250), 335, -3 * math.sin(335 / 170)], points[1], points[2], material)
                elif 1 <= band < lip_band - 1 and (i + band * 3) % (9 + band % 2) == 0:
                    spotted_quad(cap, *points)
                else:
                    quad(cap, *points, material)

    # Each rib is part of the shell, with a deep rounded fold and a darker flank.
    # Lamellae curve up inside the lowered skirt and meet the unchanged stem.
    lamellae = [(311, 145, 0), (296, 169, 10), (279, 190, 17),
                (265, 201, 20), (253, 203, 12), (244, 198, 0)]

    def gill_point(ring, sector):
        radius, y, depth = ring
        point = surface(radius, y, sector, True)
        fraction = sector % 1
        fold = math.sin(PI * fraction)
        variation = .8 + .2 * math.sin(int(sector) * 2.4)
        point[1] -= depth * fold * variation
        return point

    for outer, inner in zip(lamellae, lamellae[1:]):
        for i in range(sectors):
            for half, material in ((0, 13), (1, 15)):
                a, b = i + half * .5, i + (half + 1) * .5
                quad(gills, gill_point(outer, a), gill_point(inner, a),
                     gill_point(inner, b), gill_point(outer, b), material)
    write(MODELS / "elder_hollow_cap.json", dict(name="Deep bell-shaped russet elder crown", materials=palette, shapes=cap))
    write(MODELS / "elder_gills.json", dict(name="Deep honey ivory descending lamellae", materials=palette, shapes=gills))

    # A separate inward-facing vault gives the crown real thickness. Its last
    # ring shares the inner stem vertices; no duplicate coplanar backfaces.
    interior = []
    vault = [(0, 329), (65, 325), (132, 315), (200, 300),
             (250, 264), (282, 222), (240, 198)]

    def vault_point(ring, sector):
        radius, height = ring
        if ring == vault[-1]:
            return surface(radius, height, sector)
        if radius == 0:
            return [16 * math.sin(height / 250), height, -3 * math.sin(height / 170)]
        point = surface(radius, height, sector, True)
        # Folds deepen toward the shoulder and taper out at the stem connection.
        point[1] -= 5 * math.sin(PI * (sector % 1)) * min(1, radius / 180)
        return point

    for outer, inner in zip(vault, vault[1:]):
        for i in range(sectors):
            for half, material in ((0, 16), (1, 17)):
                a, b = i + half * .5, i + (half + 1) * .5
                if outer[0] == 0:
                    triangle(interior, vault_point(outer, a), vault_point(inner, b),
                             vault_point(inner, a), material)
                else:
                    quad(interior, vault_point(outer, b), vault_point(inner, b),
                         vault_point(inner, a), vault_point(outer, a), material)

    # Thin branching veins sit just inside the wall, beyond all room footprints.
    def vein_point(height, sector):
        radius = next(r0 + (r1 - r0) * (height - y0) / (y1 - y0)
                      for (r0, y0), (r1, y1) in zip(stem_rings, stem_rings[1:]) if y0 <= height <= y1)
        return surface(radius - 5.5, height, sector)

    for sector in (4, 15, 26, 39, 49, 58):
        for start, end in ((18, 58), (92, 143), (165, 195)):
            for step in range(4):
                low, high = start + (end - start) * step / 4, start + (end - start) * (step + 1) / 4
                a = sector + .35 * math.sin(low * .07 + sector)
                b = sector + .35 * math.sin(high * .07 + sector)
                quad(interior, vein_point(low, a - .035), vein_point(high, b - .035),
                     vein_point(high, b + .035), vein_point(low, a + .035), 18)

    # Local light sources illuminate the vault without flooding every room.
    lights = [dict(name="heart_amber", type="Point", position=[0, 290, 0],
                   color=[1, .57, .26], intensity=3, radius=145, castsShadow=False)]
    for i, sector in enumerate((4, 26, 49)):
        point = vein_point(150, sector)
        point[0] *= .94
        point[2] *= .94
        lights.append(dict(name=f"mycelium_{i}", type="Point", position=point,
                           color=[.34, .64, .43], intensity=1.3, radius=75, castsShadow=False))
    write(MODELS / "elder_interior.json", dict(name="Honey vault and living mycelium",
          materials=palette, shapes=interior, lights=lights))

    details = []
    # Continuous tapered roots replace intersecting ellipsoids and thick columns.
    for root, angle in enumerate((.12, .92, 1.77, 2.43, 3.92, 4.82, 5.67)):
        rings = []
        for distance, y, width in ((242, 32, 23), (268, 8, 20), (298, -2, 14),
                                   (327, -14, 8), (343, -55, 3), (343, -135, .5)):
            a = angle + .09 * math.sin(distance / 70 + root)
            center = [math.sin(a) * distance, y, math.cos(a) * distance]
            rings.append([[center[0] + math.cos(a) * width * math.cos(j * PI / 4),
                           center[1] + width * .65 * math.sin(j * PI / 4),
                           center[2] - math.sin(a) * width * math.cos(j * PI / 4)] for j in range(8)])
        for first, second in zip(rings, rings[1:]):
            for j in range(8):
                k = (j + 1) % 8
                quad(details, first[j], second[j], second[k], first[k], 10)

    # Broad scalloped fans grow out from the fibres, in staggered colonies.
    # Their cream underside is actual geometry, visible from the approach below.
    def shelf(angle, height, width):
        radius = next(r0 + (r1 - r0) * (height - y0) / (y1 - y0)
                      for (r0, y0), (r1, y1) in zip(stem_rings, stem_rings[1:]) if y0 <= height <= y1)
        origin = surface(radius - 2, height, angle * sectors / (2 * PI) + .5)

        def fan_point(t, reach, rise):
            scallop = 1 + .055 * math.cos(t * 8)
            sideways = math.sin(t) * width * .5 * reach
            outward = math.cos(t) * width * .52 * reach * scallop
            return [origin[0] + math.cos(angle) * sideways + math.sin(angle) * outward,
                    height + rise,
                    origin[2] - math.sin(angle) * sideways + math.cos(angle) * outward]

        for segment in range(12):
            a, b = -PI / 2 + segment * PI / 12, -PI / 2 + (segment + 1) * PI / 12
            crown = [origin[0], height + width * .12, origin[2]]
            underside = [origin[0], height - width * .06, origin[2]]
            top_a, top_b = fan_point(a, .82, width * .045), fan_point(b, .82, width * .045)
            edge_a, edge_b = fan_point(a, 1, 0), fan_point(b, 1, 0)
            triangle(details, crown, top_a, top_b, 9)
            quad(details, top_a, edge_a, edge_b, top_b, 14)
            triangle(details, underside, edge_b, edge_a, 13 if segment % 2 else 15)
        moss = fan_point(.2, .38, width * .12)
        details.append(shape("Sphere", moss, [width * .32, 2.2, width * .22], 11, angle))

    for angle, height, width, count in ((.65, 105, 64, 4), (2.1, 68, 46, 2),
                                       (3.65, 120, 70, 4), (5.25, 155, 56, 3)):
        for j in range(count):
            shelf(angle + j * .09, height - j * 12, width * (1 - j * .18))

    # Low moss cushions nestle between the buttresses, leaving the approach clear.
    for angle, size in ((.42, 25), (.69, 18), (1.85, 22), (3.95, 26), (4.2, 17), (5.45, 23)):
        point = surface(266, 1, angle * sectors / (2 * PI) + .5)
        details.append(shape("Sphere", point, [size, 5, size * .7], 11, angle))

    # Attached pearl-like spore clusters replace the thin hanging strings.
    for sector in (6.5, 8.5, 35.5, 38.5, 53.5):
        for j in range(3):
            point = gill_point(lamellae[2], sector + j * .13)
            point[1] -= 1.5
            details.append(shape("Sphere", point, [3.0, 4.5 + j, 3.0], 12))
    write(MODELS / "elder_exterior_details.json", dict(name="Scalloped woodland fans and jade spore pearls", materials=palette, shapes=details))

    exterior = [object_("elder_hollow_stem", collider=True),
                object_("elder_floor", collider=True),
                object_("elder_hollow_cap"), object_("elder_gills"),
                object_("elder_interior", light=True),
                object_("elder_exterior_details"),
                object_("floor_plate", (0, 0, -258), (12, 1.2, 84), collider=True),
                object_("root_portal", (0, 0, -269), (2.1, 2.5, 1.5)),
                object_("root_portal", (0, 0, -222), (1.3, 1.6, 1))]
    for x in (-8, 8):
        exterior.append(object_("parapet", (x, 0, -258), (84, 1, 1), PI / 2, True))
        exterior.append(object_("lantern_amber", (x, 6, -272), (1.4, 1.4, 1.4), light=True))
        exterior.append(object_("lantern_amber", (x, 5, -239), (1.1, 1.1, 1.1), light=True))
        for z in (-294, -260, -229):
            exterior.append(object_("cluster", (x * 1.7, 0, z), (1.8, 1.8, 1.8)))
    for a, size in ((.5, 1.1), (1.8, .85), (4.5, 1.3), (5.2, .8)):
        exterior.append(object_("module_giant", (math.sin(a) * 272, 0, math.cos(a) * 272), (size,) * 3))
    save("elder_shell", "Quiet elder of the Spore Crown", exterior)


def cube_colliders(prefab):
    """Transform authored cube solids for floor/clearance checks, without a GPU."""
    result = []
    for obj in json.loads(prefab.read_text())["objects"]:
        if not obj["generateColliders"]:
            continue
        model = json.loads((prefab.parent / obj["asset"]).read_text())
        for part in model["shapes"]:
            if part["meshType"] != "Cube" or abs(part["rotation"][0]) + abs(part["rotation"][2]) > 1e-6:
                continue
            yaw = obj["rotation"][1]
            part_yaw = part["rotation"][1] + yaw
            x, y, z = [part["position"][i] * obj["scale"][i] for i in range(3)]
            x, z = rotate(x, z, yaw)
            center = [x + obj["position"][0], y + obj["position"][1], z + obj["position"][2]]
            sx, sy, sz = [part["scale"][i] * obj["scale"][i] for i in range(3)]
            extent = [(abs(math.cos(part_yaw)) * sx + abs(math.sin(part_yaw)) * sz) / 2,
                      sy / 2, (abs(math.sin(part_yaw)) * sx + abs(math.cos(part_yaw)) * sz) / 2]
            result.append(([center[i] - extent[i] for i in range(3)],
                           [center[i] + extent[i] for i in range(3)]))
    return result


def audit_walkways():
    import re
    config = (ROOT / "game/src/world/worldConfig.h").read_text()
    world_scale = float(re.search(r"c_mushroomDungeonScale = ([\d.]+)f", config)[1])

    def clear(solids, x, z, top, radius=.7):
        radius /= world_scale
        return not any(a[0] - radius < x < b[0] + radius and a[2] - radius < z < b[2] + radius
                       and a[1] < top + 2.2 / world_scale and b[1] > top + .2 / world_scale for a, b in solids)

    for prefab in PREFABS.glob("*.prefab.json"):
        if prefab.name in ("doorway_seal.prefab.json", "vertical_transition.prefab.json",
                           "boss_arena.prefab.json", "elder_shell.prefab.json", "boss_approach.prefab.json"):
            continue
        solids = cube_colliders(prefab)
        for direction in range(4):
            for step in range(97):
                x, z = rotate(0, step / 4, direction * PI / 2)
                assert any(a[0] - 1e-5 <= x <= b[0] + 1e-5 and a[2] - 1e-5 <= z <= b[2] + 1e-5
                           and abs(b[1]) < 1e-5 for a, b in solids), (prefab, x, z, "floor gap")
                assert clear(solids, x, z, 0), (prefab, x, z, "blocked doorway axis")
        for x in (-7, 7):
            for z in (-7, 7):
                assert clear(solids, x, z, 0), (prefab, x, z, "blocked guard pocket")

    solids = cube_colliders(PREFABS / "vertical_transition.prefab.json")
    corners = [(-19, -19), (-19, 19), (19, 19), (19, -19)]
    previous = 0
    for flight in range(12):
        x, z = corners[flight % 4]
        nx, nz = corners[(flight + 1) % 4]
        for step in range(153):
            t = step / 152
            px, pz = x + (nx - x) * t, z + (nz - z) * t
            support = [b[1] for a, b in solids if a[0] - 1e-5 <= px <= b[0] + 1e-5
                       and a[2] - 1e-5 <= pz <= b[2] + 1e-5 and previous - .2 <= b[1] <= previous + .2]
            assert support, (flight, step, "disconnected stair")
            previous = max(support)
            assert clear(solids, px, pz, previous, .65), (flight, step, "stair headroom")
    assert abs(previous - FLOOR_SPACING) < 1e-4, "Stair must meet next floor"
    for height, segments in [(0, [((0, -24), (0, -19)), ((0, -19), (-19, -19))]),
                             (FLOOR_SPACING, [((-19, -19), (-19, 19)), ((-19, 19), (0, 19)), ((0, 19), (0, 24))])]:
        for start, end in segments:
            for step in range(153):
                x = start[0] + (end[0] - start[0]) * step / 152
                z = start[1] + (end[1] - start[1]) * step / 152
                assert any(a[0] - 1e-5 <= x <= b[0] + 1e-5 and a[2] - 1e-5 <= z <= b[2] + 1e-5
                           and abs(b[1] - height) < 1e-5 for a, b in solids), "Landing floor gap"
                assert clear(solids, x, z, height, .65), (height, x, z, "Landing headroom")
    # Fitted approach and arena share an edge, not intersecting floor volumes.
    approach = cube_colliders(PREFABS / "boss_approach.prefab.json")
    arena = cube_colliders(PREFABS / "boss_arena.prefab.json")
    combined = [([a[0], a[1], a[2] - 48], [b[0], b[1], b[2] - 48]) for a, b in approach] + arena
    for step in range(145):
        z = -72 + step / 2
        for x in (-3, 0, 3):
            assert any(a[0] <= x <= b[0] and a[2] <= z <= b[2] and abs(b[1]) < 1e-5 for a, b in combined), "Boss bridge floor gap"
            assert clear(combined, x, z, 0), (x, z, "Boss approach obstruction")
    print("Doorways, guard pockets, 12 stair flights, both landings and boss bridge pass support/clearance checks.")



def triangle_vertices(part):
    """Reconstruct the same scale -> X/Y/Z rotation used by the engine."""
    points = []
    for vertex in ((0, 0, 0), (1, 0, 0), (0, 1, 0)):
        x, y, z = [vertex[i] * part["scale"][i] for i in range(3)]
        rx, ry, rz = part["rotation"]
        y, z = y * math.cos(rx) - z * math.sin(rx), y * math.sin(rx) + z * math.cos(rx)
        x, z = x * math.cos(ry) + z * math.sin(ry), -x * math.sin(ry) + z * math.cos(ry)
        x, y = x * math.cos(rz) - y * math.sin(rz), x * math.sin(rz) + y * math.cos(rz)
        points.append([v + t for v, t in zip((x, y, z), part["position"])])
    return points


def audit_surfaces():
    from collections import defaultdict
    planes = defaultdict(list)
    count = 0
    for name in ("elder_hollow_stem", "elder_hollow_cap", "elder_gills", "elder_interior", "elder_floor"):
        model = json.loads((MODELS / (name + ".json")).read_text())
        assert model["shapes"], (name, "empty shell")
        for part in model["shapes"]:
            assert part["meshType"] == "Triangle", (name, "layered solid in shell")
            a, b, c = triangle_vertices(part)
            normal = cross(subtract(b, a), subtract(c, a))
            length = math.sqrt(dot(normal, normal))
            assert length > 1e-7, (name, "degenerate triangle")
            normal = [v / length for v in normal]
            axis = max(range(3), key=lambda i: abs(normal[i]))
            if normal[axis] < 0:
                normal = [-v for v in normal]
            key = tuple(round(v, 4) for v in normal) + (round(dot(normal, a), 3),)
            projected = [[v[i] for i in range(3) if i != axis] for v in (a, b, c)]
            planes[key].append(projected)
            count += 1
    for faces in planes.values():
        for i, face in enumerate(faces):
            for other in faces[i + 1:]:
                assert not polygons_overlap(face, other), "Overlapping coplanar shell faces"
    print(f"{count} shell triangles: no coplanar surface overlap.")


def polygons_overlap(a, b):
    # Strict separating-axis test: a shared edge is valid, shared area is not.
    for polygon in (a, b):
        for p, q in zip(polygon, polygon[1:] + polygon[:1]):
            axis = [q[1] - p[1], p[0] - q[0]]
            length = math.hypot(*axis)
            if length < 1e-8:
                continue
            pa = [dot(v, axis) / length for v in a]
            pb = [dot(v, axis) / length for v in b]
            if min(max(pa), max(pb)) - max(min(pa), min(pb)) <= 1e-4:
                return False
    return True


def audit_interior_visibility():
    # Cull backfaces just like the main pass: outward crown triangles cannot
    # conceal the sky for an observer inside the mushroom.
    faces = []
    for name in ("elder_hollow_stem", "elder_hollow_cap", "elder_interior", "elder_floor"):
        model = json.loads((MODELS / (name + ".json")).read_text())
        for part in model["shapes"]:
            a, b, c = triangle_vertices(part)
            faces.append((a, subtract(b, a), subtract(c, a)))

    def nearest_hit(origin, direction):
        nearest = math.inf
        for a, edge1, edge2 in faces:
            h = cross(direction, edge2)
            determinant = dot(edge1, h)
            if determinant <= 1e-7:
                continue
            offset = subtract(origin, a)
            u = dot(offset, h) / determinant
            if u < -1e-7 or u > 1 + 1e-7:
                continue
            q = cross(offset, edge1)
            v = dot(direction, q) / determinant
            if v < -1e-7 or u + v > 1 + 1e-7:
                continue
            distance = dot(edge2, q) / determinant
            if distance > 1e-5:
                nearest = min(nearest, distance)
        return nearest

    count = 0
    for origin in ((0, 8, 0), (0, 86, 0), (0, 164, 0), (0, 242, 0), (0, 242, -90)):
        assert math.isfinite(nearest_hit(origin, (0, 1, 0))), (origin, "open crown above")
        for sector in range(8):
            angle = (sector + .25) * PI / 4
            for rise in (0, .5, 1.5):
                direction = (math.sin(angle), rise, math.cos(angle))
                assert math.isfinite(nearest_hit(origin, direction)), (origin, direction, "interior sees sky")
                count += 1
    # The authored entrance is the intentional opening, including its return path.
    assert nearest_hit((0, 5, -310), (0, 0, 1)) > 94, "Shell blocks entry gallery"
    assert nearest_hit((0, 5, -216), (0, 0, -1)) > 94, "Shell blocks exit gallery"
    print(f"{count + 5} interior sightlines are enclosed; entry and exit gallery remain open.")
    floor_samples = [(0, 2, 0)]
    for radius in (80, 170, 235):
        for sector in range(16):
            angle = sector * PI / 8
            floor_samples.append((math.sin(angle) * radius, 2, math.cos(angle) * radius))
    for origin in floor_samples:
        assert abs(nearest_hit(origin, (0, -1, 0)) - 2.15) < 1e-5, (origin, "floor gap or uneven support")
    shell = json.loads((PREFABS / "elder_shell.prefab.json").read_text())
    assert any(obj["asset"].endswith("/elder_floor.json") and obj["generateColliders"] for obj in shell["objects"])
    print(f"{len(floor_samples)} floor samples have level support; floor collision is enabled.")


def audit():
    import re
    header = (ROOT / "game/src/world/mushroomDungeon.h").read_text()
    assert float(re.search(r"c_mushroomDungeonFloorSpacing = ([\d.]+)f", header)[1]) == FLOOR_SPACING
    assert "78.0f * c_mushroomDungeonScale" in header and "48.0f * c_mushroomDungeonScale" in header
    assert float(re.search(r"c_mushroomDungeonCellSize = ([\d.]+)f", header)[1]) == 48
    assert int(re.search(r"c_mushroomDungeonFloorCount = (\d+)", header)[1]) == len(FLOOR_HEIGHTS)
    count = 0
    for path in sorted(PREFABS.glob("*.prefab.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        assert data["assetType"] == "Prefab", path
        for obj in data["objects"]:
            asset = (path.parent / obj["asset"]).resolve()
            model = json.loads(asset.read_text(encoding="utf-8"))
            assert all(math.isfinite(v) for field in ("position", "rotation", "scale") for v in obj[field]), path
            assert all(v > 0 for v in obj["scale"]), path
            for part in model["shapes"]:
                assert 0 <= part["materialIndex"] < len(model["materials"]), asset
            count += 1
    print(f"Audited {len(list(PREFABS.glob('*.prefab.json')))} prefabs, {count} objects; all model references resolve.")
    audit_walkways()
    audit_surfaces()
    audit_interior_visibility()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    if args.write:
        export()
    audit()
