# TMC Practice — Deutsche Anleitung

TMC Practice erweitert **The Legend of Zelda: The Minish Cap** um ein
Practice- und Exploration-Menü. Es gibt getrennte BPS-Patches für USA, Europa
und Japan. Die Practice-Menütexte sind Englisch; die normale Spielsprache bleibt erhalten.

## Installation

1. Öffne auf der GitHub-Projektseite **Releases** und dann **TMC Practice v1.0.0**.
2. Lade den zu deiner Original-ROM passenden USA-, Europe- oder Japan-BPS herunter.
3. Sichere deinen bisherigen Spielstand separat.
4. Wende den BPS mit einem BPS-kompatiblen Patcher auf die **unveränderte Original-ROM** an.
5. Öffne die erzeugte `.gba` in deinem Emulator, beispielsweise LinkBoy.
6. Öffne das Menü mit **L + R + Select**.

Eine bereits gespeicherte eigene Tastenkombination gilt weiterhin. Verwende einen
normalen Spielstand derselben Region, keine alten Emulator-Savestates. Nicht mit
anderen Practice-/No-Clip-Patches oder parallelen AR-Codes kombinieren.
Die [ROM-Prüfsummen](ROM_COMPATIBILITY.md) zeigen genau, welche Versionen passen.
Das Paket enthält keine ROMs, Spielstände oder Savestates.

## Orientierung im Menü

| Bereich | Zweck |
| --- | --- |
| PRACTICE | Timer und Practice-Steuerung |
| PLAYER / MOVEMENT | Link, Bewegung, No-Clip und Kamera |
| INVENTORY | Items, Ausrüstung, Flaschen und einzelne Elemente |
| WORLD / WARP | Räume, Favoriten, Intro und Ende |
| FLAGS | Spielzustände ansehen und gezielt bearbeiten |
| DEBUG | Debug-Anzeigen |
| CHEATS | Ressourcen, Enemy Freeze, Infinite Time und weitere Cheats |
| SETTINGS | Design, Tasten und gespeicherte Einstellungen |
| ACTORS / OBJECTS | Actor-Liste, Roh-Spawner, Freeze, Entfernen und Teleport |

Die vorhandenen unbenutzten/Beta-Platzhalter sind keine rekonstruierten Beta-Inhalte.
25 nicht identifizierte Actor-Einträge bleiben bewusst als UNKNOWN bezeichnet.

## Zum Intro oder Ende

**WORLD / WARP → STORY INTRO** oder **ENDING / CREDITS** auswählen.
Einmal **A**, danach noch einmal **A** zur Bestätigung drücken.
Das geht aus dem normalen Gameplay, nicht aus einer bereits laufenden Unterhaltung,
Zwischensequenz oder einem offenen nativen Inventar.

**RETURN FROM REPLAY** beendet die Wiederholung und lädt den Ursprungsraum neu.
Der gesicherte Spielstand wird wiederhergestellt. Das Ending-Replay kehrt vor dem
normalen Speicherdialog zurück. Ein Emulator-Reset verliert den temporären RAM-Rückkehrpunkt.

## Actor-Werkzeuge und Vorsicht

**ACTORS / OBJECTS → RAW NATIVE SPAWNER** bietet native Kategorien und Namen.
ID, TYPE und andere Rohwerte bleiben editierbar; Zahlen sind hexadezimal.
Die Schnellvorlagen beschränken den Roh-Spawner nicht.

In der **ROOM ACTOR LIST** kannst du einen Actor auswählen, einfrieren, entfernen
oder räumlich bearbeiten. **LINK TO ACTOR** und **ACTOR TO LINK** teleportieren in
die jeweilige Richtung. Manager ohne gemeinsame Raumkoordinaten zeigen
**NO GENERIC XYZ**; Freeze/Remove bleiben möglich.

Unpassende Bosse, Typen, Eltern-Actors oder Skriptabhängigkeiten können native
Abstürze und unspielbare Räume verursachen. Entfernen ist kein Boss-Sieg.
Zerstörerische Aktionen verlangen die angezeigte Bestätigung, standardmäßig
**L + R halten und A neu drücken**. Immer einen separaten Save-Backup behalten.

## Was geprüft wurde

166 automatisierte mGBA-Testreihen insgesamt: 53 USA und 113 EU/JP.
Die getesteten Patches wurden für diese Veröffentlichung nicht verändert.
Menüöffnung und No-Clip wurden zusätzlich vom Projekt-Tester praktisch bestätigt;
dabei wurde keine vollständige separate Android-Abnahme jeder Region dokumentiert.
Echte GBA-Hardware wurde nicht geprüft. Details: [Verification](VERIFICATION.md).

Fehler bitte über **Issues** melden: Region, Emulator samt Version, genaue Schritte
und gegebenenfalls Screenshot. Bitte keine ROMs, privaten Saves oder Zugangsdaten hochladen.
