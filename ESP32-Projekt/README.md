# Blablablubber Megafon (ESP32)

Blablablubber zum Mitnehmen: ein batteriebetriebenes Kindermegafon im Stil des
Bob-der-Baumeister-Megafons, nur dass es funktioniert. Abzug drücken und
reden, vorn kommt eine von acht Stimmen raus. Der gelbe Taster oben auf der
Haube schaltet weiter, eine Sekunde halten springt zurück zu Mickey Maus.
Die gewählte Stimme bleibt auch über den Tiefschlaf erhalten.

| # | Stimme | Klang | Piep |
|---|---|---|---|
| 1 | Mickey Maus | hoch und quietschig | 440 Hz |
| 2 | Darth Vader | tief und dunkel | 660 Hz |
| 3 | Roboter | metallisch, Ringmodulator 42 Hz | 880 Hz |
| 4 | Alien | etwas höher, zitterndes Vibrato | 1100 Hz |
| 5 | Monster | eine Oktave tiefer, knurrt | 1320 Hz |
| 6 | Echo | Stadiondurchsage, klingt nach dem Loslassen aus | 1540 Hz |
| 7 | Funkgerät | blechern und rauschend, "Kschh" beim Loslassen | 1760 Hz |
| 8 | Blubber | unter Wasser, mit Blubberblasen | 1980 Hz |

```
ESP32-Projekt/
├── Bauteile.md          Lagerliste, Bewertung, Stromversorgung, Verdrahtungstabelle
├── Verdrahtung.html     Verdrahtungsplan mit Fotos (auch als Artefakt veröffentlicht)
├── Zusammenbau.html     bebilderte Schritt-für-Schritt-Anleitung, Kabelliste, Pinbelegung
├── components stock/    Fotos der Bauteile
├── firmware/            PlatformIO-Projekt (Arduino-Framework)
│   ├── platformio.ini
│   ├── include/config.h    Pins und Einstellungen
│   ├── include/voices.h    die acht Stimmen (Pitch-Shifter, Ringmodulator, Echo, Filter)
│   └── src/main.cpp        Hauptprogramm
└── 3d/
    ├── megafon.scad        parametrisches Modell (OpenSCAD), alle Maße oben in der Datei
    ├── megafon-koerper.stl Lauf (hinten offen) mit Griff, Batteriefach, Schlitten für den ESP32
    ├── megafon-haube.stl   abnehmbare obere Schale, darunter liegen die drei Zellen
    ├── megafon-rueckplatte.stl  verschließt das Laufende, trägt Mikrofon und USB-Schlitz
    ├── batteriefach.png    Skizze: welche Kontaktstreifen und Federn wohin
    ├── megafon-kegel.stl   Schalltrichter
    ├── megafon-griffschale.stl  linke Seitenschale des Griffs (Zugang zu Abzug und Verstärker)
    ├── megafon-tastermutter.stl drei Muttern M7 × 0,75 für die 7-mm-Taster (drei Toleranzen)
    ├── megafon-tastermutter-1..3.stl dieselben einzeln (1 = eng, 2 = mittel, 3 = locker)
    ├── render.sh           erzeugt STL und Vorschau neu
    ├── probe.scad, check.sh  Kollisionsprüfung (Schnittvolumen aller Teile und Hüllkörper)
    └── preview-*.png       Vorschaubilder
```

## Funktionsweise

Anders als die Desktop-Version arbeitet das Megafon live: Solange der Abzug
gedrückt ist, läuft das Mikrofonsignal mit etwa 60 ms Verzögerung durch den
Effekt auf den Lautsprecher. Lässt man los, ist Ruhe und der Verstärker geht
in den Stromsparmodus. Nach drei Minuten ohne Abzug legt sich der ESP32
schlafen, der nächste Druck auf den Abzug weckt ihn (Start-Jingle).

Rückkopplung ist bei einem Megafon konstruktiv gelöst: Das Mikrofon sitzt in
der Rückplatte und zeigt zum Mund, der Trichter strahlt nach vorn. Die
Tonhöhenverschiebung hilft zusätzlich, weil eine Rückkopplungsschleife bei
jedem Umlauf die Frequenz ändert und sich nicht aufschaukeln kann.

## Stückliste (Stand 6, 24.10.2026)

Alles liegt im Lager, Maße nachgemessen am 24.10.2026, Details in `Bauteile.md`.

| Teil | Hinweis |
|---|---|
| ESP32 DevKitC (WROOM-32, 30 Pins) | rechnet die Stimme um; ADC GPIO 34, DAC GPIO 25 |
| GY-MAX4466 | Elektret-Mikrofonmodul mit Verstärker, sitzt in einem Kasten auf der Rückplatte, Kapsel außen bündig |
| Adafruit MAX98306 | analoger Class-D-Verstärker, läuft mit 5 V; liegt in der Kammer im Griff |
| Lautsprecher 28 mm | vorn in der Tasche, drei M3-Klemmschrauben mit Unterlegscheiben |
| MT3608 Step-up (XY-016) | macht aus 3 × AA die 5 V für Board und Verstärker |
| Batteriefach | wird mitgedruckt: drei AA im Dreieck unter der Haube. Kontakte aus Streifen und Federn, siehe `3d/batteriefach.png` |
| MTS-101 Kippschalter | Ein/Aus, links in der Seitenwand des Laufs hinter dem Griff |
| 2 Minitaster 7 mm | rot = Abzug im Griff, gelb = Stimme in der Haube; kamen ohne Muttern, deshalb `megafon-tastermutter.stl` mit Gewinde M7 × 0,75, drei Spielstufen, die passende nehmen |
| Elko 470 µF / 10 V, Kondensator 1 µF | 5-V-Schiene am Verstärker (mit in die Griffkammer), DAC-Kopplung |
| Schrauben | 14 × M3 × 10 (4 Rückplatte, 4 Griffschale, 3 Trichter, 3 Lautsprecher), 1 × M3 × 12 (Haube), 2 × M2 × 4 fürs Mikrofon (sonst Heißkleber) |
| Filament | ca. 270 g buntes PLA, Trichter und Körper getrennt gedruckt |

## Verdrahtung

Die Firmware kennt zwei Audio-Ketten, Auswahl per `AUDIO_ANALOG` in
`firmware/include/config.h`:

* **analog** (Standard, Lagerbestand): MAX4466 → ESP32-ADC, ESP32-DAC → MAX98306.
  Versorgung 3 × AA → Kippschalter → MT3608 auf 5 V → DevKitC VIN und MAX98306 VDD.
  Vollständige Tabelle in **`Bauteile.md`**, Plan mit Fotos in `Verdrahtung.html`.
* **I2S** (`AUDIO_ANALOG 0`): INMP441 und MAX98357A, Tabelle unten, falls die
  Teile mal gekauft werden.

Alle Signale sind 3,3 V. Die Pins stehen in `firmware/include/config.h`.

| Modul | Pin am Modul | ESP32 |
|---|---|---|
| INMP441 | VDD | 3V3 |
| | GND | GND |
| | L/R | GND |
| | SCK | GPIO 26 |
| | WS | GPIO 25 |
| | SD | GPIO 33 |
| MAX98357A | VIN | 5 V |
| | GND | GND |
| | BCLK | GPIO 14 |
| | LRC | GPIO 15 |
| | DIN | GPIO 32 |
| | SD | GPIO 4 (high = an, low = Shutdown) |
| | GAIN | offen lassen (9 dB) oder an GND (12 dB) |
| Lautsprecher | + / – | MAX98357A Ausgang |
| Abzug | 1 | GPIO 27 |
| | 2 | GND |
| Stimmen-Taster | 1 | GPIO 17 (`VOICE_BUTTON 1`, schaltet durch) |
| | 2 | GND |
| Status-LED | | GPIO 2 (auf dem Board) |

Der Abzug liegt auf GPIO 27, weil das ein RTC-Pin ist: Nur damit kann der
Taster den ESP32 aus dem Tiefschlaf wecken.

Stromversorgung: Drei Zellen liefern 4,8 V voll und 3,0 V leer, der MT3608
arbeitet ab 2 V und macht daraus stabile 5 V. Den Wandler **ohne Last** auf
5,0 V trimmen (der Trimmer reicht bis 28 V). Zwei Zellen reichen nicht: bei
Lautstärkespitzen bricht die Spannung ein und der ESP32 startet neu.

## Firmware bauen und flashen

```sh
cd ESP32-Projekt/firmware
pio run                    # bauen (Umgebung esp32dev)
pio run -t upload          # flashen, Board per USB anstecken, Kippschalter aus
pio device monitor         # serielle Ausgabe, 115200 Baud
```

Für das Lolin D32: `pio run -e lolin_d32 -t upload`.

**Wenn es krächzt:** zwei Testfirmwares eingrenzen lassen, Konsole dabei offen (`pio device monitor`).

```sh
pio run -e test_sinus -t upload    # Abzug = sauberer 440-Hz-Ton. Krächzt der, liegt es am Ausgang oder Strom
pio run -e test_direkt -t upload   # Abzug = Mikrofon ohne Effekt
```

Bei gedrücktem Abzug kommt jede Sekunde eine Zeile mit ADC-Abtastrate (soll 16000),
Ruhepegel (soll ~2048), Spitze-Spitze und Anteil übersteuerter Samples.
Wichtig: Der ESP32 tastet im I2S-ADC-Modus nicht mit der eingestellten Rate ab
(16000 eingestellt ergab 88 kHz und damit Krächzen). Deshalb läuft der ADC auf 64 kHz
(`ADC_I2S_RATE`), je vier Samples werden gemittelt. Lautstärke:
`DAC_LEVEL` und `MASTER_GAIN` in `config.h`, Mikrofon am MAX4466-Trimmer.

Beim Start spielt das Megafon einen kurzen Jingle, bei jedem Stimmenwechsel
einen Piep. Lautstärke und Übersteuerung regelt `MASTER_GAIN` in `config.h`,
dahinter sitzt ein weicher Begrenzer. Den Gain-Trimmer am MAX4466 so
einstellen, dass normales Sprechen in 10 cm Abstand nicht übersteuert.

## 3D-Druck

Fünf Teile, alle STL-Dateien liegen druckfertig:

| Teil | Ausrichtung | Stützen | Material |
|---|---|---|---|
| `megafon-koerper.stl` | Vorderwand liegt auf dem Bett, Laufende oben offen, der Griff ragt seitlich heraus | ja, unter dem Griff (Baumstützen) und unter den Stirnwänden des Batteriefachs | ca. 90 g bei 15 % Füllung |
| `megafon-haube.stl` | steht auf der hinteren Stirnfläche | nein, Brim empfohlen | ca. 20 g |
| `megafon-rueckplatte.stl` | flach, Außenseite unten, Mikrofonkasten oben | nein | 15 g |
| `megafon-kegel.stl` | weite Öffnung unten | nein | ca. 60 g |
| `megafon-griffschale.stl` | flach, Innenseite unten, Senkungen oben | nein | 9 g |
| `megafon-tastermutter.stl` | flach, drei Spielstufen (1/2/3 Kerben = 0,15/0,25/0,35 mm); 0,1-mm-Schichten, 100 % Füllung | nein | 2 g |

Empfehlung: PLA, 0,2 mm Schichten, 3 Wände, 15 % Füllung. Für den bunten Look
ein Regenbogen- oder Multicolor-Filament nehmen, der Trichter kann in einer
anderen Farbe als der Körper gedruckt werden.

Innenleben (Stand 6):

* **Haube:** Die obere Schale des Laufs (54 mm breit, 77 mm lang) ist abnehmbar.
  Sie liegt auf einer 2 mm breiten Lippe auf, rastet mit vier Kugelnasen in
  Mulden ein und wird hinten mit einer M3 × 12 gesichert. Vorn hält zusätzlich
  der Trichterkragen die Haube fest. Der gelbe Stimmen-Taster sitzt vorn in der
  Haube.
* **Batteriefach (mitgedruckt):** Drei AA liegen im Dreieck direkt unter der
  Haube: A und C unten in zwei U-Rippen, B oben im Tal dazwischen. Vorn und
  hinten steht je eine 3-mm-Stirnwand mit drei Kontakttaschen (12 × 12 × 1,5 mm),
  einer Nut für den Brückenstreifen und einem Kabelloch Ø 3,2. Die Haube hält
  die Zellen, 1,3 mm Luft über B. Zellenwechsel: Haube ab, Zellen raus.
* **Rückplatte:** Der Lauf ist hinten offen. Eine Scheibe Ø 65,6 × 4 mm sitzt
  bündig im Laufende, vier M3 × 10 von hinten in Pfosten im Lauf (Senkungen für
  Linsenköpfe). Sie trägt das Mikrofon: innen ein Kasten, in dem die
  MAX4466-Platine quer liegt, die Kapsel steckt durch ein Ø-10,6-Loch und ist
  außen bündig. Zwei M2 × 4 von innen durch die Platinenlöcher in die Auflage,
  oder ein Tropfen Heißkleber. Daneben der USB-C-Schlitz (13 × 7,5 mm, für
  schlanke Stecker) und zwei Nasen, die die hinteren Ecken des ESP32 niederhalten.
  Mit abgenommener Platte kommt man an alles im hinteren Teil des Laufs.
* **ESP32-Schlitten:** Die DevKitC-Platine liegt mit der Antenne nach vorn und
  den Stiftleisten nach unten auf zwei Schienen, die zwischen den Stiftleisten
  unter der Platine stehen; die Stifte hängen daneben und führen seitlich. Die
  Platine wird von hinten eingeschoben, bis die Vorderkante am Anschlag mit Lippe
  sitzt, hinten hält die Rückplatte. Unter der Platine ist Platz, die Litzen
  werden an die Stiftspitzen gelötet und laufen nach hinten oder durch den Hals
  in den Griff. Die Stifte an den Schienen: 0,6 mm Luft, bei Problemen
  `esp_rail_in` anpassen.
* **Deck links:** Der MT3608 liegt links auf einem kleinen Deck in
  Kantenklammern (von oben eindrücken, Trimmer nach hinten links, dann ist er
  neben Zelle C erreichbar). Zwischen Schlitten und Deck ein Kanal, durch den
  die Kabel aus dem Hals nach oben kommen.
* Vorn die Tasche für den 28-mm-Lautsprecher (9 mm tief), Öffnung Ø 24 mm, mit
  drei M3-Klemmschrauben samt Unterlegscheiben (Lochkreis 33 mm) oder Heißkleber.
* Kippschalter links in der Seitenwand (x −79, 8 mm unter der Laufachse),
  Mutter von außen; der Schalterkörper liegt unter den Rippen des Batteriefachs
  und ist bei abgenommener Haube und ohne Zellen von oben erreichbar. Links
  außen eine Vertiefung für einen 30-mm-Aufkleber.
* **Griff 40 × 38 mm:** Oben der hohle Hals (30 × 30 mm) mit dem Abzug,
  darunter eine Kammer 30 × 32 × 40 mm für den MAX98306 (liegt flach, Stifte
  zur Seitenschale hin, Elko daneben). Zwischen Hals und Kammer eine 6-mm-Platte
  mit zwei Ø-4-Löchern für die Kabel. Die linke Seitenwand ist eine
  abschraubbare Schale 4 mm (4 × M3 × 10, versenkt) mit zwei Fenstern: Abzug von außen
  durch die Ø 7,3-mm-Bohrung, innen ist die Wand auf 3 mm gesenkt (Ø 13), die
  gedruckte Mutter wird durch das obere Fenster aufgeschraubt; der Verstärker kommt durch
  das untere Fenster.

Die Bauteilmaße sind am 24.10.2026 nachgemessen, die Rückplatte kam dazu,
weil Mikrofon, Kippschalter und ESP32 hinter der Stirnwand des Batteriefachs
sonst nicht erreichbar waren. Offen bleibt die Breite deiner Kontaktstreifen
(Taschen 12 mm, Nut 4 mm) und die Federlänge. Alle Maße sind
Variablen am Anfang von `megafon.scad`. Nach Änderungen `./render.sh`
ausführen (braucht OpenSCAD, `brew install --cask openscad@snapshot`).

## Zusammenbau

Ausführlich mit Bildern, Kabelliste und Pinbelegung in `Zusammenbau.html`. Kurzfassung:

1. Kontakte: Streifen und Federn nach `3d/batteriefach.png` löten, Litzen
   durch die Kabellöcher fädeln, Streifen in die Taschen der Stirnwände drücken,
   Tropfen Heißkleber. Die Litzen laufen hinter den Stirnwänden nach unten.
2. Griffschale ab. Abzug von vorn einsetzen, gedruckte Mutter durch das obere
   Fenster aufschrauben. MAX98306 verdrahten (VDD, GND, SD, L+, L−, Lautsprecher), Elko an
   VDD/GND, durch das untere Fenster in die Kammer legen, Kabel durch die zwei
   Löcher der Platte nach oben in den Lauf.
3. Lautsprecher von innen in die Tasche vorn, mit drei M3 und Unterlegscheiben
   klemmen. Kippschalter von innen durch das Loch in der linken Seitenwand
   stecken (Haube ab, Zellen raus), Mutter außen anziehen.
4. MT3608 ohne Last auf 5,0 V trimmen, dann in die Kantenklammern auf dem Deck
   drücken (Trimmer nach hinten links).
5. ESP32: Litzen an die Stiftspitzen löten (VIN, GND, 3V3, GPIO 34, 25, 27, 4,
   17), Platine mit der Antenne voran von hinten auf die Schienen schieben, bis
   sie am Anschlag sitzt. Litzen zu Verstärker, MT3608 und Taster durch den
   Kanal und den Hals ziehen, die Mikrofon-Litzen nach hinten legen.
6. Mikrofon in den Kasten der Rückplatte legen (Kapsel nach außen), mit zwei M2
   oder Heißkleber sichern, Litzen an VCC/GND/OUT. Gelben Taster in die Haube
   schrauben, Litzen lang genug lassen oder einen Stecker vorsehen. Alles
   verdrahten, Firmware flashen (USB-C durch den Schlitz der Platte,
   Kippschalter aus), testen. Rückplatte mit 4 × M3 × 10 anschrauben.
7. Zellen einlegen: A und C unten mit dem Pluspol nach hinten, B oben mit dem
   Pluspol nach vorn. Mit dem Multimeter 4,5 V an den Litzen prüfen. Haube
   einklicken und hinten mit M3 sichern, Trichter aufstecken und verschrauben.

Batteriewechsel: Trichter und Rückplatte bleiben dran. M3 hinten lösen, Haube
abziehen, Zellen tauschen. Firmware-Update: USB-C durch den Schlitz in der
Rückplatte, nichts aufschrauben.

Sicherheit: Für Kinder ist die Haube verschraubt, damit die Zellen nicht
herausfallen. Keine Li-Ion-Zellen ohne Schutzschaltung verwenden; der Plan
rechnet mit Alkaline- oder NiMH-AA.
