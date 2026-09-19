"""Read-only audit of the handcrafted Bernsteinkern prefab. No asset generation.

Run from the repository root: python -B scripts/acorn_dungeon_audit.py
Coordinates are prefab-local. Route annotations live here, outside the asset schema.
"""
import collections
import itertools
import json
import math
import re
from pathlib import Path

from mushroom_dungeon import Geometry, transform
import mushroom_dungeon as geometry_tools

# Existing organic props block movement but are not designated walking surfaces.
_structural_faces = geometry_tools.faces
geometry_tools.faces = lambda mesh: [] if mesh in ('Sphere', 'Crystal') else _structural_faces(mesh)

ROOT = Path(__file__).resolve().parents[1]
MODELS = ROOT / 'game/assets/models/acorn_dungeon'
PREFAB = ROOT / 'game/assets/prefabs/acorn_dungeon.prefab.json'

# x, walking Y, z, floor radius, district. Junctions are deliberately smaller.
NODES = {
    'Pilgrim threshold': (0, 0, -248, 4, 'approach'),
    'Pilgrim road': (0, 0, -208, 4, 'approach'),
    'Root archway': (0, 0, -164, 30, 'approach'),
    'Split husk': (0, 0, -112, 21, 'husk'),
    'West shell gate': (-62, 8, -89, 11, 'husk'),
    'Fracture gallery': (-99, 16, -47, 12, 'gallery'),
    'Husk watch': (-108, 24, 9, 13, 'gallery'),
    'Seed vault': (-83, 32, 67, 21, 'vault'),
    'Shell junction': (-27, 40, 120, 13, 'gallery'),
    'East shell gate': (59, 8, -91, 12, 'husk'),
    'Root wound': (109, 16, -57, 11, 'breach'),
    'Root breach': (147, 24, 2, 23, 'breach'),
    'Exposed seed': (111, 32, 66, 12, 'breach'),
    'North husk': (61, 40, 120, 13, 'gallery'),
    'Great seed chamber': (0, 48, 68, 25, 'seed'),
    'Seed memory': (-76, 56, 44, 12, 'vault'),
    'Inner sanctum': (-85, 68, -30, 22, 'sanctum'),
    'Sanctum balcony': (-53, 80, -97, 11, 'sanctum'),
    'Amber reservoir': (59, 56, 45, 14, 'amber'),
    'Vein gallery': (94, 64, 15, 11, 'amber'),
    'Amber cathedral': (78, 76, -56, 23, 'amber'),
    'Core threshold': (0, 88, -92, 13, 'heart'),
    'Heart south': (0, 104, -30, 7, 'heart'),
    'Heart west': (-30, 104, 0, 7, 'heart'),
    'Heart north': (0, 104, 40, 7, 'heart'),
    'Heart east': (30, 104, 0, 7, 'heart'),
    'Crack balcony': (75, 116, -36, 12, 'ascent'),
    'Fracture crossing': (125, 128, 13, 13, 'ascent'),
    'Upper wound': (90, 140, 68, 12, 'ascent'),
    'Cap interior': (0, 152, 81, 21, 'cap'),
    'Cap west': (-63, 160, 37, 13, 'cap'),
    'Cap east': (63, 160, 37, 12, 'cap'),
    'Windward scales': (-65, 168, -27, 11, 'cap'),
    'Living seam': (65, 168, -27, 11, 'sprout'),
    'Sprout sanctuary': (0, 176, -62, 14, 'sprout'),
    'Final arena': (0, 184, 0, 27, 'sprout'),
    'Empty seed bowl': (0, 0, 0, 38, 'seed'),
    'Shell wicket': (-64, 16, -30, 10, 'gallery'),
    'Cotyledon bridge': (0, 56, 8, 10, 'seed'),
    'Husk shrine': (-137, 16, -72, 10, 'optional'),
    'Buried cotyledon': (-156, 24, 39, 12, 'optional'),
    'Root nursery': (159, 32, 99, 12, 'optional'),
    'Seed archive': (-100, 56, 85, 11, 'optional'),
    'Quiet germ': (-113, 160, 6, 10, 'optional'),
}

# Each connection has an explicit width; all connections are reversible.
EDGES = [
    ('Pilgrim threshold', 'Pilgrim road', 14),
    ('Pilgrim road', 'Root archway', 14),
    ('Root archway', 'Split husk', 14),
    ('Split husk', 'West shell gate', 10),
    ('West shell gate', 'Fracture gallery', 9),
    ('Fracture gallery', 'Husk watch', 9),
    ('Husk watch', 'Seed vault', 10),
    ('Seed vault', 'Shell junction', 10),
    ('Split husk', 'East shell gate', 10),
    ('East shell gate', 'Root wound', 9),
    ('Root wound', 'Root breach', 11),
    ('Root breach', 'Exposed seed', 11),
    ('Exposed seed', 'North husk', 9),
    ('Shell junction', 'North husk', 10),
    ('Shell junction', 'Great seed chamber', 12),
    ('North husk', 'Great seed chamber', 12),
    ('Great seed chamber', 'Seed memory', 10),
    ('Seed memory', 'Inner sanctum', 10),
    ('Inner sanctum', 'Sanctum balcony', 10),
    ('Sanctum balcony', 'Core threshold', 10),
    ('Great seed chamber', 'Amber reservoir', 10),
    ('Amber reservoir', 'Vein gallery', 10),
    ('Vein gallery', 'Amber cathedral', 11),
    ('Amber cathedral', 'Core threshold', 11),
    ('Core threshold', 'Heart south', 12),
    ('Heart south', 'Heart west', 10),
    ('Heart west', 'Heart north', 10),
    ('Heart north', 'Heart east', 10),
    ('Heart east', 'Heart south', 10),
    ('Heart east', 'Crack balcony', 10),
    ('Crack balcony', 'Fracture crossing', 10),
    ('Fracture crossing', 'Upper wound', 10),
    ('Upper wound', 'Cap interior', 11),
    ('Cap interior', 'Cap west', 10),
    ('Cap interior', 'Cap east', 10),
    ('Cap west', 'Windward scales', 9),
    ('Cap east', 'Living seam', 9),
    ('Windward scales', 'Sprout sanctuary', 10),
    ('Living seam', 'Sprout sanctuary', 10),
    ('Sprout sanctuary', 'Final arena', 14),
    ('Split husk', 'Empty seed bowl', 14),
    ('Fracture gallery', 'Husk shrine', 7),
    ('Husk watch', 'Buried cotyledon', 8),
    ('Exposed seed', 'Root nursery', 8),
    ('Seed memory', 'Seed archive', 7),
    ('Cap west', 'Quiet germ', 7),
    # Lower shell bypass and two bridges through previously seen spaces.
    ('West shell gate', 'Shell wicket', 8),
    ('Shell wicket', 'Husk watch', 8),
    ('Seed memory', 'Cotyledon bridge', 9),
    ('Cotyledon bridge', 'Amber reservoir', 9),
    ('Inner sanctum', 'Crack balcony', 9),
    ('Seed vault', 'Great seed chamber', 7),
]


def route(a, b):
    """Flat room interiors meet ramps .8 units inside each floor boundary."""
    a, b = NODES[a], NODES[b]
    d = math.hypot(b[0] - a[0], b[2] - a[2])
    dx, dz = (b[0] - a[0]) / d, (b[2] - a[2]) / d
    # Small overlaps keep the high-end riser below the controller's .5-unit step.
    ar, br = max(0, a[3] - .8), max(0, b[3] - .8)
    return [(a[0], a[1], a[2]), (a[0] + dx * ar, a[1], a[2] + dz * ar),
            (b[0] - dx * br, b[1], b[2] - dz * br), (b[0], b[1], b[2])]


def samples(points, spacing=.75):
    for a, b in zip(points, points[1:]):
        n = max(1, math.ceil(math.dist(a, b) / spacing))
        for i in range(n + 1):
            yield tuple(a[j] + (b[j] - a[j]) * i / n for j in range(3))


def audit():
    objects = json.loads(PREFAB.read_text())['objects']
    shape_keys = {'meshType', 'position', 'rotation', 'scale', 'color', 'materialIndex'}
    object_keys = {'asset', 'position', 'rotation', 'scale', 'generateColliders', 'generateLights'}
    material_keys = {'albedo', 'roughness', 'metallic', 'ambientStrength', 'lightWrap',
                     'shapeContrast', 'emissiveColor', 'emissiveStrength'}
    loader = (ROOT / 'engine/src/graphics/shapeModel/shapeModelLoader.cpp').read_text()
    models = {}
    for path in MODELS.glob('*.json'):
        model = json.loads(path.read_text())
        assert set(model) <= {'name', 'materials', 'shapes', 'lights'}, path
        for material in model['materials']:
            assert set(material) == material_keys, path
        for shape in model['shapes']:
            assert set(shape) == shape_keys, path
            assert '"' + shape['meshType'] + '"' in loader, path
            assert 0 <= shape['materialIndex'] < len(model['materials']), path
            assert len(shape['color']) == 4, path
            for key in ('position', 'rotation', 'scale'):
                assert len(shape[key]) == 3 and all(math.isfinite(v) for v in shape[key]), path
            assert min(shape['scale']) > 0, path
        names = []
        for light in model.get('lights', []):
            assert set(light) == {'name', 'type', 'position', 'color', 'intensity', 'radius', 'castsShadow'}, path
            assert light['type'] == 'Point' and light['radius'] > 0 and light['intensity'] >= 0, path
            names.append(light['name'])
        assert len(names) == len(set(names)), path
        raw = path.read_bytes()
        assert raw.count(b'\n') == raw.count(b'\r\n'), ('CRLF', path)
        models[path.name] = model
    for obj in objects:
        assert set(obj) == object_keys, obj
        assert (PREFAB.parent / obj['asset']).is_file(), obj
        for key in ('position', 'rotation', 'scale'):
            assert len(obj[key]) == 3 and all(math.isfinite(v) for v in obj[key]), obj
        assert min(obj['scale']) > 0, obj
        assert type(obj['generateColliders']) is bool and type(obj['generateLights']) is bool
        assert not obj['generateLights'] or models[Path(obj['asset']).name].get('lights'), obj
    labels = [Path(o['asset']).stem for o in objects]
    geometry = Geometry(objects, labels)
    decoration = Geometry(objects, labels, decorative=True)
    errors = []
    count = 0
    def check(point, label):
        nonlocal count
        x, y, z = point
        count += 1
        floor = geometry.support(x, z, y)
        if floor is None:
            errors.append((label, 'missing floor', tuple(round(v, 2) for v in point)))
            return
        obstruction = geometry.blocked(x, floor, z) or decoration.blocked(x, floor, z)
        if obstruction:
            errors.append((label, obstruction, tuple(round(v, 2) for v in point)))
    max_grade = 0
    graph = collections.defaultdict(set)
    for a, b, width in EDGES:
        graph[a].add(b)
        graph[b].add(a)
        points = route(a, b)
        d = math.hypot(points[2][0] - points[1][0], points[2][2] - points[1][2])
        grade = abs(points[2][1] - points[1][1]) / d
        max_grade = max(max_grade, grade)
        assert grade <= .46, (a, b, grade)
        dx, dz = NODES[b][0] - NODES[a][0], NODES[b][2] - NODES[a][2]
        length = math.hypot(dx, dz)
        # Full central movement lane, plus capsule footprint at its edges.
        for p in samples(points):
            for offset in (-width * .28, 0, width * .28):
                check((p[0] - dz / length * offset, p[1], p[2] + dx / length * offset), a + ' -> ' + b)
    for name, (x, y, z, radius, district) in NODES.items():
        if name.startswith('Heart '):
            continue
        for dx in range(-int(radius * .55), int(radius * .55) + 1, 3):
            for dz in range(-int(radius * .55), int(radius * .55) + 1, 3):
                if math.hypot(dx, dz) < radius * .6:
                    check((x + dx, y, z + dz), name + ' combat/rest core')
    # Original four furnished side rooms and their real 8-unit door openings.
    for side in (-1, 1):
        for z in (-234, -200):
            for p in samples([(side * 8, .04, z), (side * 24, .04, z)]):
                check(p, 'Retained pilgrim room')
    source = (ROOT / 'game/src/world/biome/forestGenerator.cpp').read_text()
    guards = source.split('c_acornGuardPositions[] =')[1].split('};')[0]
    guards = [tuple(map(float, match)) for match in re.findall(
        r'\{\s*([-\d.]+)f,\s*([-\d.]+)f,\s*([-\d.]+)f\s*\}', guards)]
    assert len(guards) == 36
    for guard in guards:
        check(guard, 'Authored guard')
    # Boss movement bounds must fit the actual upper floor, including decoration.
    for radius in (0, 10, 20):
        for angle in range(0, 360, 5):
            t = math.radians(angle)
            check((radius * math.cos(t), 184, radius * math.sin(t)), 'Thornwolf movement area')
    boss_source = (ROOT / 'game/src/world/enemy/enemySpawn.h').read_text()
    assert 'if (_bossId == sBossId::ForestThornwolf)\n            return 184.0f;' in boss_source
    assert 'if (_bossId == sBossId::ForestThornwolf)\n            return 20.0f;' in boss_source
    # Every visible primitive is bounded using the engine's transform order.
    bounds = []
    for obj in objects:
        for part in models[Path(obj['asset']).name]['shapes']:
            bounds.extend(transform(transform(v, part), obj)
                          for v in itertools.product((-.5, .5), repeat=3))
    lo = tuple(min(p[i] for p in bounds) for i in range(3))
    hi = tuple(max(p[i] for p in bounds) for i in range(3))
    assert lo[0] >= -184 and hi[0] <= 184 and lo[2] >= -252 and hi[2] <= 184, (lo, hi)
    # The complete prefab stays loaded throughout traversal, even at the worst
    # owner-chunk offset. The terrain approach reaches local Z=-300.
    assert max(math.ceil(abs(v) / 64) for v in (lo[0], hi[0], -300, hi[2])) <= 5
    # Representative camera rays stop before the destination crystal/shell.
    sightlines = [
        ('First heart reveal', (0, 50.4, 68), (0, 121, 9)),
        ('Look back into the seed chamber', (75, 118.4, -24), (0, 50.4, 68)),
        ('Future fracture crossing', (0, 50.4, 68), (125, 136, 13)),
    ]
    for name, a, b in sightlines:
        for p in samples([a, b], .5):
            if geometry.contains(p) or decoration.contains(p):
                errors.append((name, 'occluded sightline', tuple(round(v, 2) for v in p)))
                break
    visited, pending = set(), ['Pilgrim threshold']
    while pending:
        name = pending.pop()
        if name not in visited:
            visited.add(name)
            pending.extend(graph[name] - visited)
    assert visited == set(NODES), set(NODES) - visited
    unique_errors = list(dict.fromkeys(errors))
    groups = collections.defaultdict(list)
    for error in unique_errors:
        groups[error[:2]].append(error[2])
    for key, points in groups.items():
        print('FAIL', key, len(points), 'samples, first', points[0])
    print(f'{len(models)} models; {len(objects)} instances; '
          f'{sum(len(models[Path(o["asset"]).name]["shapes"]) for o in objects)} shapes; '
          f'{sum(o["generateLights"] for o in objects)} light sources')
    print(f'{len(NODES)} destinations; {len(EDGES)} links; '
          f'{len(EDGES) - len(NODES) + 1} independent cycles; max ramp grade {max_grade:.3f}')
    print(f'{count} geometry samples; {len(errors)} failures')
    print('Conservative geometry bounds:', tuple(round(v, 2) for v in lo),
          tuple(round(v, 2) for v in hi))
    assert not errors, 'Static traversal audit failed'
    print('PASS: JSON/schema, references, graph, walking support, sampled clearance and combat cores.')
    print('Static geometry validation is not a controller simulation or an in-game playtest.')


if __name__ == '__main__':
    audit()
