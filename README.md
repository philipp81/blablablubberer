# Blablablubber

Blablablubber hört dir über das Mikrofon zu, solange du die Leertaste hältst,
und plappert beim Loslassen alles nach – wahlweise als Mickey Maus, Darth
Vader, Roboter, Alien, Betrunkener, aus der Kathedrale, durchs Telefon oder
rückwärts. Was gut war, speichert ein Tastendruck als WAV und M4A.

Geschrieben in C (C11), Audio über [miniaudio](https://miniaud.io) (liegt als
`third_party/miniaudio.h` bei, keine weiteren Abhängigkeiten). Nur macOS.

## Bauen und starten

```sh
make                 # baut ./blablablubber
./blablablubber      # los geht's (Standard: Mickey Maus, 5 s Verzögerung)
```

Beim ersten Start fragt macOS, ob das Terminal aufs Mikrofon darf – das muss
erlaubt werden (Systemeinstellungen → Datenschutz & Sicherheit → Mikrofon).

## Aufnahme mit der Leertaste

Blablablubber funktioniert wie ein Walkie-Talkie: **Leertaste gedrückt
halten**, sprechen, loslassen. Sobald du loslässt, plappert Blablablubber das
Gesagte mit der gewählten Stimme nach. Während du sprichst, ist der
Lautsprecher aus, danach ist das Mikrofon zu – die Wiedergabe kann also nie
wieder aufgenommen werden, auch ohne Kopfhörer gibt es keine Rückkopplung.

Ein Terminal meldet kein Loslassen einer Taste. Das Halten wird deshalb an der
Tastenwiederholung erkannt: solange der Mac weitere Leerzeichen schickt, gilt
die Taste als gedrückt. Die Abstände werden im Betrieb gemessen, das Timeout
passt sich der Einstellung unter Systemeinstellungen → Tastatur an. Nach dem
Loslassen läuft die Aufnahme noch etwa eine halbe Sekunde nach, dann startet
die Wiedergabe.

Drei Modi, per `--modus` oder im Programm mit `m` umschaltbar:

| Modus | Verhalten |
|---|---|
| `halten` (Standard) | Leertaste gedrückt halten, Loslassen startet die Wiedergabe |
| `umschalten` | Leertaste startet die Aufnahme, nochmal Leertaste beendet sie und spielt ab |
| `immer` | Dauer-Echo: Mikrofon immer offen, Wiedergabe nach festem Delay (`--delay`, Standard 5 s) – nur mit Kopfhörern |

Ein Clip darf bis zu 60 Sekunden lang sein. Ist die Tastenwiederholung am Mac
abgeschaltet, startet Blablablubber automatisch im Modus `umschalten`.

## Speichern und verschicken

Wenn ein Blubber besonders gut geworden ist: `s` drücken. Blablablubber wartet,
bis die Wiedergabe fertig ist, schneidet den letzten Abschnitt (alles seit der
letzten Pause von über zwei Sekunden) sauber aus und schreibt ihn nach
`~/Desktop/Blablablubber/`:

```
blablablubber-2026-10-23-18.05.42-mickey.wav   48 kHz, 16 Bit, mono
blablablubber-2026-10-23-18.05.42-mickey.m4a   AAC, für WhatsApp und Co.
```

`S` speichert stattdessen alles aus dem Puffer (die letzten 60 Sekunden, ohne
Stille am Anfang und Ende). `o` öffnet den Ordner im Finder. Der Speicherort
lässt sich mit `--ordner PFAD` ändern. Die M4A-Datei entsteht über das
mitgelieferte macOS-Werkzeug `afconvert`; die WAV-Datei bleibt immer erhalten.

## Optionen

| Option | Bedeutung |
|---|---|
| `-s, --stimme NAME\|ZIFFER` | Startstimme, z. B. `--stimme darth` oder `--stimme 3` |
| `-d, --delay SEKUNDEN` | Verzögerung im Modus `immer`, 0,5 bis 30 Sekunden (Standard 5) |
| `-m, --modus MODUS` | `halten` (Standard), `umschalten` oder `immer` |
| `-o, --ordner PFAD` | Speicherort für WAV/M4A (Standard `~/Desktop/Blablablubber`) |
| `-z, --zufall` | alle 4 Sekunden eine zufällige Stimme |
| `-t, --sekunden N` | nach N Sekunden automatisch beenden |
| `-l, --liste` | alle Stimmen anzeigen |
| `--selftest` | DSP-Selbsttest ohne Audiogeräte (`make test`) |
| `--testton` | speist statt dem Mikrofon eine kleine Melodie ein, zum Ausprobieren |

Tasten während das Programm läuft:

| Taste | Wirkung |
|---|---|
| Leertaste | Aufnahme (halten bzw. umschalten, je nach Modus) |
| `0` – `9` | Stimme wechseln |
| `z` | Zufallsmodus an/aus |
| `+` / `-` | Verzögerung im Modus `immer` um eine Sekunde länger/kürzer |
| `m` | Aufnahmemodus wechseln: halten → umschalten → immer |
| `s` | letzten Blubber als WAV + M4A speichern |
| `S` | alles aus dem Puffer speichern (letzte 60 s) |
| `o` | Speicherordner im Finder öffnen |
| `q` | beenden |

## Stimmen

| Taste | Name | Was passiert |
|---|---|---|
| 0 | normal | nur Verzögerung, deine echte Stimme |
| 1 | mickey | Tonhöhe ×1,6 (granularer Pitch-Shifter) |
| 2 | helium | Tonhöhe ×2,1 |
| 3 | darth | Tonhöhe ×0,58 plus leichtes Knurren |
| 4 | roboter | Ringmodulator mit 42 Hz (Dalek-Sound) |
| 5 | alien | Tonhöhe ×1,35 plus 6,5-Hz-Vibrato |
| 6 | betrunken | Tonhöhe ×0,92, taumelt langsam auf und ab |
| 7 | kathedrale | zwei rückgekoppelte Echos (211 ms und 347 ms) |
| 8 | telefon | Bandpass 300–3400 Hz plus Verzerrung |
| 9 | rueckwaerts | jeder 1,5-Sekunden-Block wird rückwärts abgespielt |
| z | zufall | alle 4 Sekunden eine andere Stimme |

Die Tonhöhe wird im Zeitbereich verschoben: zwei Leseköpfe laufen schneller
oder langsamer als der Schreibzeiger durch den Ringpuffer und werden mit
Hann-Fenstern überblendet. Die Sprechgeschwindigkeit bleibt dabei gleich.

## App mit Icon

```sh
make app             # baut Blablablubber.app
open Blablablubber.app
```

`Blablablubber.app` trägt das Icon aus `icon/Blablablubber.icns`. Ein
Doppelklick öffnet ein Terminal-Fenster und startet darin das Programm. Die App
kann nach `/Applications` kopiert oder ins Dock gezogen werden.

Das Icon wird aus `icon/make_icon.py` erzeugt (SVG → PNG per Quick Look →
`.icns` per `iconutil`, braucht `python3` mit Pillow):

```sh
make icon
```

## Dateien

```
blablablubber.c        Programm (Audio, Effekte, Terminal-Oberfläche)
miniaudio_impl.c       bindet die miniaudio-Implementierung ein
third_party/miniaudio.h
bundle/                Info.plist und Launcher-Skript für die App
icon/make_icon.py      erzeugt SVG, PNG, .iconset und .icns
icon/Blablablubber.icns
Makefile
```
