# Waldabschnitt: vier Hueter

Die Welt ist als Folge konzentrischer Gebiete gedacht: Wald, Wueste, Eis und Lava.
Diese Umsetzung umfasst den inneren Waldkreis (Radius 100 Welteinheiten).
Die aeusseren Biome sind noch nicht generiert; der Felsrand begrenzt das spielbare Gebiet.

Der Spieler startet auf einer freien Lichtung im Zentrum. Vier mit flachen Steinen
markierte Wege fuehren zu Ruinen in den vier Quadranten. Ihr Winkel variiert mit
dem Welt-Seed; Typen und Anzahl bleiben stabil. Jeder Dungeon ist eine offene,
unbedachte Ruine mit bewachtem Vorraum, Suedeingang (-Z) und freier Bosskammer.
Baeume, Steine und zufaellige Monster werden aus diesen Bereichen ferngehalten.

| Dungeon | Bossbasis | Boss | Lebenspunkte |
| --- | --- | --- | --- |
| Wurzelgruft | ForestCrawler | Uralter Kriecher | 270 |
| Steinheiligtum | ForestBrute | Waldkoloss | 720 |
| Dornenbau | ForestThornwolf | Dornenalpha | 390 |
| Sporenkrypta | ForestSporecap | Sporenkoenig | 480 |

Bosse sind 2,5-fach vergroesserte Varianten mit sechsfachen Lebenspunkten,
1,5-fachem Schaden und laengerer Angriffsvorbereitung. Nahkampfreichweite und
Bewegungskollision beruecksichtigen die Groesse. Die vorhandenen individuellen
Angriffe und Animationen bleiben erhalten. Fernangriffe bleiben auf Spielerhoehe.

Bosse bleiben in ihrer Kammer. Verlaesst der Spieler die Kammer, wird ein lebender
Boss auf seine Startposition und volle Gesundheit zurueckgesetzt. Besiegte Bosse
bleiben fuer die laufende Welt besiegt. Das HUD zeigt Bossleben in der Kammer,
Entfernung, Weltrichtung (+Z = Nord) und den Abschluss nach vier Siegen.
Speichern, Beute und ein Uebergang in die Wueste sind noch nicht implementiert.

## Manuelle Spielpruefung

- Vom Zentrum allen vier Steinwegen folgen: freier Zugang durch den Vorraum.
- Je Dungeon zwei normale Waechter und genau einen grossen Boss pruefen.
- Nahkampf, beide Projektilarten, Treffer und skalierte Lebensbalken pruefen.
- Angeschlagenen Boss verlassen: Ruecksetzung; erneut betreten: neuer Kampf.
- Alle vier besiegen: HUD zeigt 4/4 und Abgeschlossen; kein Wiedererscheinen.
- Felsrand auf Durchgaenge pruefen; bei anderem Seed vier erreichbare Dungeons.

Debug-Build und statische Pruefung ersetzen diese Spielpruefung nicht.
