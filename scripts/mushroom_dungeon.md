# Mushroom Dungeon

The blue-violet visual revision and the remaining completion gates are tracked
in [mushroom_completion.md](mushroom_completion.md). Room prefabs now have three
vault heights; large immutable triangle-only models are submitted as material/tint
mesh batches. The layout contracts below remain unchanged. The original shared
prop models are retained as geometry inputs for the new `violet_*` variants.

The existing four-portal room prefabs remain the building blocks. Generation uses
48-unit cells in a sparse nine-by-nine envelope, with floors at 0, 78, 156 and 234.
The boss stays at the existing dungeon centre and encounter height.

The pipeline is seeded main-path construction, optional branches, distance-checked
loops, floor-weighted room selection, socket-compatible rotation, module conversion,
and validation. The checked seed range produces 41?54 rooms and 24?32 edges from
entrance to boss. Branches are one to four rooms long. The final route always ends
in large combat, the fitted approach, and the arena. Each of three independent
stair shafts reserves its cell on both floors; there is no shared tower bypass.
Retries use independent deterministic seed sequences, with at most 32 attempts and
a fixed, validated fallback. No generation randomness comes from the chunk RNG.

North is +Z and positive quarter turns rotate North to East. `socketMask` describes
local authored openings; `connectionMask` describes active world-space openings.
The reusable room shells support all four sockets and receive walls at unused ones.
Stairs require South at the bottom and North at the top before rotation. Entrance,
boss and approach retain their authored orientation. `inputDirection` and
`outputDirection` are orientation hints for active sockets; the masks and adjacency
indices are authoritative for junctions. The arena reserves its surrounding cells.
Its approach ends at local Z=10, meeting the arena at Z=-38 without overlapping it.

Nursery is weighted toward the root floor, Alchemy toward floor one, and Grotto and
larger encounters toward floor two. The last floor has fewer branches and a protected
boss approach. Shrine and Archive are limited, usually at branch ends. Decoration
variants choose the existing prop families and sides and rotate guard spawn pockets.
Upper-floor rooms have more guards; deeper large rooms can have a Blue-tier guard.
Optional rooms remain exploration spaces: no new reward, shrine-interaction or loot
system is implied by their categories.

The Elder Shell now has a bent, irregular hollow stem, a weathered asymmetric cap
with a torn lip, deep underside pleats, large roots, ridges, shelf colonies and hanging
glowing growth. Its roots extend below the terrain reservation. The root arch and
lit level gallery guide the player through the stem to the first room. Room mycelium
supports continue under the four sockets. The final floor and boss sit beneath the
crown. The landmark owner chunk is retained near the reservation while normal room
streaming and chunk generation budgets still apply. Decorative shell elements and
small growths have no colliders; the stem and walkable surfaces retain collision.

Authored limitations: most room categories still share a forty-unit interior and
four-portal shell. LargeCombat describes the encounter and decoration, not a multi-cell
hall. Junctions use the generic shell. The boss is the oversized exception and needs
a dedicated approach. Changing the cell size or floor spacing requires reauthoring
and auditing these contracts together.

Modified implementation: `game/src/world/mushroomDungeon.{h,cpp}`,
`game/src/world/biome/forestGenerator.cpp`, `game/src/world/worldGenerator.cpp`.
Modified content: the room, stair, boss and Elder Shell prefabs in
`game/assets/prefabs/mushroom/`, and `elder_hollow_stem.json`,
`elder_hollow_cap.json`, `elder_gills.json`, `elder_exterior_details.json` in
`game/assets/models/mushroom_dungeon/`. New assets are `boss_approach.prefab.json`,
`transition_shell.json`, and `module_mycelium.json`. `scripts/mushroom_modules.py`
remains the reproducible authoring source; use `--write` to export, or no arguments
to audit references, sockets, stair support, headroom, the boss bridge and shell faces.

Validation:

```powershell
python scripts/mushroom_modules.py
# From an x64 VS Developer PowerShell, at the repository root:
New-Item -ItemType Directory -Force build/mushroom-audit
cl /nologo /std:c++20 /EHsc /O2 /Iengine/src scripts/check_mushroom_layout.cpp /Febuild/mushroom-audit/check.exe /Fobuild/mushroom-audit/check.obj
build/mushroom-audit/check.exe 10000
```

The standalone CPU audit checks repeatability, connectivity, socket rotations,
reserved volumes, encounter budgets, negative validator cases, extreme integer seeds,
layout variety, and forced fallbacks. It includes the generator implementation to
exercise the fallback without adding test hooks to the game API. The game itself
has no new source target or engine interface. `DescribeMushroomDungeonLayout` returns
a full ASCII report; define `MUSHROOM_DUNGEON_VERBOSE` in a Debug build to print it
at world generation. Validation errors already print seed, attempt and reason in Debug.

Debug x64 game plus engine build passed. 10,000 seeds, repeats and forced fallback
cases passed. All sixteen prefabs passed the asset audit. These are CPU/content
checks; the changes still need an in-game traversal and visual playtest.

Example: world seed 42, ForestSporecap. `V` on one floor corresponds to `v` at the
same coordinates on the next. Empty rows emphasize the sparse occupied area.

```text
Mushroom Dungeon
Seed: 2764066602  Attempt: 0
Rooms: 45  Main path: 29  Branches: 8  Loops: 2
Entrance -> Boss graph distance: 28
Floor 0 (height 0)
. . . . . . . . .
                 
. . . . . . . . .
                 
. . . . . . . . .
                 
. . . . . . . . .
                 
. . . . . . . . .
                 
. . . . . . V . .
            |    
. . S O-O-C-C . .
    |   | |      
. . O-C-C-R . . .
        |        
. . . . E . . . .
Floor 1 (height 78)
. . . . . . . . .
                 
. . . . . . . . .
                 
. . . C-O . . . .
      | |        
. . . S C . . . .
        |        
. . . . C-L-C . .
        |   |    
. . . . C-C v . .
          |      
. . . . . V . . .
                 
. . . . . . . . .
                 
. . . . . . . . .
Floor 2 (height 156)
. . . . . . . . .
                 
. . . . . . . . .
                 
. . . . . . . . .
                 
. . . . . . O . .
            |    
. . . . . C-C-R-O
          | | |  
. . . . . C-R V .
            |    
. . . . . v C . .
          | |    
. . . . . L-C . .
                 
. . . . . . . . .
Floor 3 (height 234)
. . . . . . . . .
                 
. . . . . . . . .
                 
. . . . . . . . .
                 
. . . . . . . . .
                 
. . . . B . S . .
        |   |    
. . . . G . C v .
        |   | |  
. . . C-L C-R-C .
      |   |      
. . S-C-C-R . . .
                 
. . . . . . . . .
E entrance, C combat, L large combat, O optional, S special, V stair up, v landing, G boss approach, B boss
```
