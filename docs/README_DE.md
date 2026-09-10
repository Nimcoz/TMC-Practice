# TMC Practice — Deutsche Anleitung

[English](README_EN.md) · [Deutsch](README_DE.md) · [Français](README_FR.md) · [Español](README_ES.md) · [Italiano](README_IT.md) · [日本語](README_JA.md)

TMC Practice ergänzt **The Legend of Zelda: The Minish Cap** um ein Practice- und Explorationsmenü.
Für **v1.0.0** gibt es getrennte BPS-Patches für USA, Europa und Japan.
Übersetzt ist diese Anleitung; **das Practice-Menü selbst bleibt Englisch**.
Die europäische Sprachauswahl und die japanischen Spieltexte bleiben erhalten.
Die englischen Menübezeichnungen stehen hier unverändert, damit du sie im Spiel wiederfindest.

## 1. Download und Installation

1. Öffne [TMC Practice v1.0.0](https://github.com/Nimcoz/TMC-Practice/releases/tag/v1.0.0).
2. Lade genau einen Patch für die **Region deiner Original-ROM**, unabhängig von der Sprache dieser Anleitung:
   - USA: `TMC-Practice-v1.0.0-USA.bps`.
   - Europa (Englisch, Französisch, Deutsch, Spanisch, Italienisch): `TMC-Practice-v1.0.0-Europe.bps`.
   - Japan: `TMC-Practice-v1.0.0-Japan.bps`.
3. Sichere deinen normalen Spielstand in einer separaten Kopie.
4. Wende den BPS-Patch mit einem BPS-kompatiblen Patcher auf deine **unveränderte Original-ROM** an.
5. Öffne die erzeugte `.gba` in deinem Emulator und verwende einen normalen Spielstand derselben Region.
6. Mit **L + R + Select** öffnest oder schließt du das Menü. Eine bereits gespeicherte eigene Tastenkombination hat Vorrang.

Die [ROM-Kompatibilitätsliste](ROM_COMPATIBILITY.md) enthält die genauen Prüfsummen.
Ignoriere keinen Prüfsummenfehler. Keine Patches, externen AR-Codes oder anderen
Practice-/No-Clip-Modifikationen kombinieren; keine Emulator-Savestates älterer Builds laden.
ROMs, Saves und Savestates sind nicht enthalten. Ein Konverter für Spielstände zwischen Regionen wird nicht angeboten.

## 2. Bedienung und Einstellungen

- **Steuerkreuz:** Zeile auswählen; links/rechts ändert einen bearbeitbaren Wert.
- **A:** ausführen/bestätigen. **B:** zurück/abbrechen.
- **Zerstörerische Aktionen:** angezeigte Bestätigung beachten; normalerweise **L + R** halten und auf der Bestätigungszeile **A** erneut drücken.
- **Rohwerte sind hexadezimal.** Links/rechts ändert um eins; L/R ändert geeignete Byte-Felder um `0x10`.
- **SETTINGS:** Design, Tastenbelegung und dauerhaft gespeicherte Menüeinstellungen. Das Speichern der Einstellungen speichert **nicht** automatisch den Spielfortschritt.

## 3. Orientierung im Menü

| Menü | Zweck |
| --- | --- |
| PRACTICE | Practice-Timer und Steuerung; Timeranzeige im Spiel |
| PLAYER / MOVEMENT | Link, Bewegung, No-Clip und Kamera |
| INVENTORY | Items, Ausrüstung, Flaschen und einzelne Elemente |
| WORLD / WARP | Räume, Favoriten sowie Intro- und Ende-Wiederholungen |
| FLAGS | Spielzustände ansehen und bearbeiten |
| DEBUG | Debug-Informationen im Spiel anzeigen |
| CHEATS | Ressourcen, Enemy Freeze, Infinite Time und weitere Cheats |
| SETTINGS | Aussehen, Tasten und gespeicherte Menüeinstellungen |
| ACTORS / OBJECTS | Actor-Liste, Roh-Spawner, Einfrieren, Entfernen und Teleport |

Sammlungen, Flags und die bestätigte **100%**-Aktion können den Story-Fortschritt
verändern. Vor Experimenten ein Backup anlegen. Unbenutzte/Beta-Platzhalter sind keine rekonstruierten Beta-Inhalte.

## 4. Menü außerhalb des normalen Gameplays öffnen

Unterstützt werden native Inventaransichten, Dialoge/Zwischensequenzen, Titel/Intro
und Endbildschirme. Zerstörerische Welt-Aktionen benötigen aktives Gameplay und
können dort gesperrt sein. Während Überblendungen, Ressourcenübertragungen oder
echten EEPROM-Speichervorgängen wartet das Menü; nicht jeder Übergangsframe ist unterbrechbar.

## 5. Intro oder Ende wiederholen

Wähle im normalen Gameplay **WORLD / WARP → STORY INTRO** oder **ENDING / CREDITS**.
Drücke **A** und danach noch einmal **A** zur Bestätigung. Starte die Wiederholung
nicht aus einem laufenden Gespräch, einer Zwischensequenz oder dem nativen Inventar.

**RETURN FROM REPLAY** beendet die Wiederholung und lädt den Ursprungsraum neu.
Alle 1.204 nativen Save-Bytes werden im RAM gesichert und bei der Rückkehr
wiederhergestellt. Temporäre Raum-Actors werden nicht wie in einem Emulator-Savestate
wiederhergestellt. Andere Änderungen, Cheats und Zeitmessung pausieren vorübergehend.
Das Ending-Replay kehrt vor dem normalen Speicherdialog zurück; das reguläre
Story-Ende behält seinen normalen Speicherablauf. Ein Emulator-Reset verliert den temporären RAM-Rückkehrpunkt.

## 6. Actor-Werkzeuge

**ACTORS / OBJECTS → RAW NATIVE SPAWNER** bietet native Kategorien, Namen und die
Rohwerte kind/ID/type/type2/timer/subtimer/flags/parent/layer. **QUICK SPAWN PRESETS**
beschränken den Rohmodus nicht. Es gibt keine kuratierte Raum-/Etagen-/Variantenliste;
fehlende native Routinen und volle native Actor-Pools werden weiterhin abgewiesen.
546 native Slots und 118 Boden-Item-Typen haben Bezeichnungen; 25 Einträge bleiben bewusst **UNKNOWN**.

Wähle in **ROOM ACTOR LIST** einen Actor zum Einfrieren, Entfernen oder Verschieben.
**LINK TO ACTOR** teleportiert Link zum Actor; **ACTOR TO LINK** holt ihn zu Link,
innerhalb desselben Raums ohne Raumwarp. Manager ohne gemeinsames XYZ-Layout zeigen
**NO GENERIC XYZ**: allgemeines Verschieben ist nicht verfügbar, Freeze/Remove bleiben möglich.
Friert man den Actor zuerst ein, bewegt seine native KI ihn nicht gleich wieder zurück.

Ein zweiter Link, unpassende Bosse/Typen oder fehlende Raum-/Skript-/Eltern-Actors
können Abstürze und unspielbare Räume verursachen. Einen Boss zu entfernen zählt
**nicht** als Sieg. Bei Problemen gegebenenfalls ohne Speichern neu laden.

## 7. Enemy Freeze, Break Free und Infinite Time

- **Enemy Freeze:** umfasst Bosse und sichtbare Körperteile ohne eigene Kollision. Projektile anderer Actor-Kategorien müssen eventuell einzeln eingefroren werden.
- **Break Free:** schließt aktiven Text über den nativen Schließzustand und gibt Link wieder frei. Bereits ausgeführte Story-Skripte werden nicht rückgängig gemacht; spätere Skripte können die Steuerung wieder übernehmen.
- **Infinite Time:** umfasst Anjus Hühner-Countdown, den Countdown im dunklen Schloss Hyrule, zeitbegrenzte Augenschalter-Aktivierung und bereits aktive Amulett-/Glückstrank-Laufzeiten. Es stoppt nicht jeden Timer, jede Animation oder Zwischensequenz. Ausschalten, damit Countdowns ausgewertet, Belohnungen vergeben und zeitbegrenzte Effekte beendet werden können.

## 8. Tests, Probleme und Mitarbeit

Der Release bestand **166 automatisierte mGBA-Testreihen: 53 USA + 113 EU/JP**.
Der Projekt-Tester bestätigte zusätzlich Menüöffnung und No-Clip in seinem Emulator.
Eine vollständige Android-Abnahme jeder Region wurde nicht dokumentiert; echte
GBA-Hardware wurde nicht getestet. Siehe [Testnachweise](VERIFICATION.md).

Fehler über [Issues](https://github.com/Nimcoz/TMC-Practice/issues) melden: Region,
Patch-Version, Emulator samt Version, genaue Schritte und gegebenenfalls Screenshot.
Keine ROMs, privaten Saves oder Zugangsdaten hochladen. Mitarbeit am offiziellen
Projekt nur nach vorheriger Absprache mit Nimcoz; Fehlerberichte brauchen keine
Absprache. Siehe [Mitarbeit](../CONTRIBUTING.md) und [Herkunft und Rechte](../THIRD_PARTY_NOTICES.md).
