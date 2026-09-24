# Pilz-Dungeon: Abschluss und Vorlage fuer weitere Dungeons

## Gestalterische Vorgabe

Referenz vom 22.09.2026: breite blauviolette Pilzkappen, helle faserige
Stiele, deutlich sichtbare Lamellen, kleine pfirsichfarbene Flecken und
sparsame tuerkisfarbene Leuchtakzente. Diese Gestaltung gilt fuer den
Pilz-Dungeon. Andere Dungeons erhalten ihre eigene Palette und Raumformen.

## Umgesetzter erster Abschnitt

### Offene Pilzkathedrale (24.09.2026)

- Der Generator reserviert die mittleren 3 x 3 Rasterzellen auf allen vier
  Etagen als Luftraum. Nur Bossarena und ihre Anschlussbruecke liegen darin.
  Der feste Fallback umgeht denselben Bereich; die Layoutvalidierung erzwingt ihn.
- Eingang, Gaenge, Kurven, kleine/grosse Kampfplattformen und Grotte sind
  offene Galerien mit niedrigen Bruestungen, facettierten Steinplatten und
  cyanfarbenen Wegmarkierungen. Nebenraeume behalten ihre Gewoelbe.
- Runde Arena mit ebenem Suedanschluss, Pilzlaternen, Tor und Bannern;
  hohe Saeulen mit Leuchtringen und Stuetzen unter der Arena.
- Radiale indigo/violette Deckenbaender und ein leuchtender, geschlossener
  Oculus. Die Aussenkappe bleibt geschlossen, damit der Nachthimmel nicht
  durch das Innere sichtbar wird.
- Lokale zylindrische Atmosphaere mit eigenem Hoehennebel und sanftem
  Uebergang am Eingang. Ein begrenzter Lichtschacht wird bis zur sichtbaren
  Oberflaeche integriert; bis zu etwa 108 schwebende Sporen nutzen das
  bestehende Partikelsystem. Die Engine erhaelt nur generische Volumendaten.
- Debug-Start `game.exe --mushroom-preview`: feste Kamera vor der Bossbruecke,
  pausiertes Gameplay, ausgeblendetes Spiel-HUD. Normaler Start bleibt spielbar.
  Bei gesperrter `game.exe` wurde separat `game-cathedral-review.exe` gebaut.

Validierung: Engine und Game Debug x64 sowie alle Shader kompiliert;
10.000 Seeds mit Wiederholungen und erzwungenen Fallbacks bestanden,
keine natuerlichen Fallbacks. Asset-Audit prueft zusaetzlich die runde
Arenaflaeche und ihren Brueckenanschluss. Die offenen Standardgalerien
benoetigen 94 Instanzen pro Pass, die grosse Kampfplattform 132, die
Arena 148 und die gesamte Pilzhuelle 163 (ohne Laufzeitdekoration).

Die Vorschau wurde im laufenden Vulkan-Renderer betrachtet. Ein kompletter
Durchlauf mit Spruengen, Kaempfen, Kamera und Streaming auf allen vier
Etagen steht weiterhin aus. Niedrige Bruestungen erlauben Spruenge;
die Graphpruefung allein garantiert keine unverkuerzbare Laufroute.

### Vorheriger Stand (22.09.2026)

- Blaue, breitere Aussenkappe mit sichtbaren Lamellen; passende kleine Pilze.
- Drei geschlossene Raumgewoelbe: niedriger Durchgang, Kammer, hohe Halle.
- Weniger wiederholte Eckdekoration; kuehle und warme Lichtakzente.
- Eigene `violet_*`-Varianten der gemeinsam verwendeten Grundmodelle.
  Die bisherigen Grundmodelle werden weiterhin als Geometriequellen gelesen.
- Wiederverwendbares Zusammenfassen statischer Dreiecksmodelle in der Engine.
  Material und Farbton bleiben getrennt. Der Spielcode cached die Meshes pro
  unveraenderlichem Modell; Chunk-Unload und Reload erstellen keine neuen Meshes.
  Die Anwendung besitzt die GPU-Meshes und zerstoert sie beim Herunterfahren.
- Instanzzaehler getrennt nach Hauptbild, Schatten, Reflexionen und AO-Geometrie.
- Asset-Pruefungen fuer Decken und Instanzbudgets ergaenzt.

Der Seed-Graph, das Raster, die Spawnpositionen und die vier Etagen bleiben in
diesem Schritt erhalten. Der Baukasten hat noch keine unterschiedlichen
Raumgrundrisse. Eine optische Abnahme und ein Durchspielen im Spiel stehen aus.

## Messbarer Ausgangspunkt

Auszaehlung der Prefabs; Instanzen fuer eine einzelne vollstaendige Einreichung,
ohne zur Laufzeit hinzugefuegte Dekoration, Gegner, Tuerverschluesse und Umgebung:

| Bestandteil | Vorher | Nachher |
| --- | ---: | ---: |
| Pilzhuelle mit Aussenbereich | 16.511 | 158 |
| Gang / kleiner Kampfraum | 181 | 131 |
| Eingang | 159 | 132 |
| Grosse Kampfhalle | 198 | 171 |
| Treppe | 544 | 544 |

Die Nachher-Werte beruecksichtigen den neuen Mesh-Pfad. Shapes bleiben die
bearbeitbare Quelle. Zusammenfassen reduziert weder automatisch Dreiecke noch
Pixelarbeit; der FPS-Gewinn muss auf der Zielhardware gemessen werden.
Es gibt noch kein vereinbartes Hardware-/FPS-Ziel und keinen GPU-Vergleich.

## Reihenfolge bis zur Abnahme

1. **Referenzabschnitt im Spiel beurteilen.** Eingang, Gang, Kampfkammer,
   Treppe und Bosszugang bei Seed 1337 ansehen. Kamera, Helligkeit, Orientierung
   und Groessenverhaeltnisse pruefen. Erst nach diesem Vergleich den Stil fixieren.
2. **Raumformen fertigstellen.** Engere Gaenge und unterscheidbare Kammergrundrisse
   innerhalb klarer Anschlussvertraege entwickeln. Spawnflaechen und Kamerawege
   freihalten; Decken- und Kollisionspruefungen entsprechend erweitern.
3. **Sichtbarkeit und Kosten messen.** Bei Seeds 42, 1337 und 2026 jeweils Eingang,
   mittlere Etage und Bossarena mit derselben Aufloesung/Kamera messen. Aufloesung,
   Hardware und Framezeitbudget festhalten. Bei Bedarf Raum-/Huellensegmente
   separat cullen; Schatten und Reflexionen muessen ihre eigene Sichtbarkeit behalten.
4. **Ablauf abstimmen.** Weglaenge nach gemessener Durchlaufzeit einstellen,
   Raumwiederholungen begrenzen und optionale Wege mit dem bestehenden Gameplay
   sinnvoll fuellen. Keine neuen Loot-/Quest-Systeme fuer diese Abnahme einfuehren.
5. **Enddurchlauf.** Alle drei Seeds vom Eingang bis zum Boss und zurueck spielen.
   Tod/Neustart, Treppen, Gegneraktivierung, Kamera sowie Chunk-Unload/Reload pruefen.
   Keine blockierenden Fehler; vereinbartes Framezeitbudget an allen Messpunkten.
6. **Uebertragbarkeit pruefen.** Zwei verbundene Raeume eines zweiten Dungeon-Themas
   gegen dieselben Anschluss-, Kollisions- und Performancevertraege pruefen.
   Erst dann gemeinsame Layoutregeln herausloesen; Pilzdetails bleiben themenspezifisch.

## Pruefungen

```powershell
python scripts/mushroom_modules.py

# x64 VS Developer PowerShell, Repository-Wurzel:
New-Item -ItemType Directory -Force build/mushroom-audit
cl /nologo /std:c++20 /EHsc /O2 /Iengine/src scripts/check_shape_batches.cpp engine/src/graphics/shapeModel/shapeMeshLibrary.cpp engine/src/graphics/shapeModel/meshGenerator.cpp /Febuild/mushroom-audit/check_shapes.exe /Fobuild/mushroom-audit/
build/mushroom-audit/check_shapes.exe
cl /nologo /std:c++20 /EHsc /O2 /Iengine/src scripts/check_mushroom_layout.cpp /Febuild/mushroom-audit/check_layout.exe /Fobuild/mushroom-audit/check_layout.obj
build/mushroom-audit/check_layout.exe 10000
```

Build Debug x64 (Engine und Spiel), 10.000 Seeds, Mesh-Pruefungen sowie die
Asset-Pruefungen bestanden am 22.09.2026. Diese Checks ersetzen keine visuelle
Abnahme, keine GPU-Messung und keinen vollstaendigen Gameplay-Durchlauf.
