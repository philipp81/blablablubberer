# Bauteile im Lager und was damit geht

Stand 24.10.2026, nach den Fotos in `components stock/` und den Nachmeldungen. Das
Verdrahtungsdiagramm mit Bildern liegt in `Verdrahtung.html`.

## Was da ist

| # | Bauteil | Foto | Für das Megafon | Bewertung |
|---|---|---|---|---|
| 1 | **ESP32 DevKitC** (ESP32-WROOM-32, 30 Pins, USB-C, CP2102, AMS1117-Regler) | IMG_3972 | **Gehirn** | Passt. Hat ADC-Eingänge (GPIO 34/35/36/39), zwei DAC-Ausgänge (GPIO 25/26) und einen 5-V-Eingang (VIN) für den 9-V-Block mit Abwärtswandler. Gemessen 54 × 29 mm, 13 mm hoch mit Stiften (ESP32-masse.jpg) |
| 2 | **Adafruit MAX98306** Stereo-Class-D-Verstärker, 2 × 3,7 W, VDD 2,7–5,5 V, analoger Eingang, Gain 6–18 dB per Jumper, SD-Pin | IMG_3973 | **Verstärker** | Passt. Analoger Eingang statt I2S, deshalb spielt der ESP32 über seinen DAC. Läuft direkt an 5 V. Gemessen 28 × 24 mm, 13 mm hoch mit Schraubklemmen, abgewinkelte Stiftleiste ragt 6 mm über die Kante (MAX98306-masse.jpg). Liegt in der Kammer im Griff |
| 3 | **Wemos Lolin32 Lite** (ESP32, Micro-USB, LiPo-Buchse mit TP4054-Lader) | IMG_3974, IMG_3975 | Alternative zu 1 | Nur sinnvoll mit LiPo-Akku, dann sehr elegant (Laden per USB). Für den 9-V-Plan ungeeignet, weil es keinen 5-V-Eingang hat. DevKitC ist hier die bessere Wahl |
| 4 | Arduino Micro (ATmega32U4) | IMG_3976 | nein | Zu langsam und zu wenig RAM für Pitch-Shifting in Echtzeit |
| 5 | Arduino Uno SMD | IMG_3977 | nein | wie 4 |
| 6 | Elektret-Mikrofonkapsel, lose, zwei Litzen | IMG_3978 | Ersatz | Bräuchte eine eigene Vorverstärkerschaltung. Das MAX4466-Modul (9) hat schon eine Kapsel drauf. Gemessen Platine 20 × 14 mm, Kapsel Ø 10, 12 mm hoch, zwei M2-Löcher oben neben der Kapsel (MAX4466-masse.jpg) |
| 7 | Mini-Lautsprecher Ø ca. 15 mm, vier Löcher | IMG_3979 | nein | Hörkapsel-Klasse, zu leise für ein Megafon. Höchstens als Piepser. Gemessen Ø 28, 9 mm tief mit Magnet, keine Löcher, wird mit drei M2 geklemmt (Lautsprecher-masse.jpg) |
| 8 | Adafruit Wave Shield v1.1 (Bausatz, unbestückt) | IMG_3980 | nein | WAV-Player-Shield für den Uno, hier ohne Nutzen |
| 9 | **GY-MAX4466** Elektret-Mikrofonmodul mit einstellbarem Gain, VCC 2,4–5,5 V, OUT/GND/VCC | IMG_3981, IMG_3982 | **Mikrofon** | Passt. Analoger Ausgang mit Ruhepegel VCC/2, geht direkt in den ESP32-ADC. Gain am Trimmer einstellen |
| 10 | DC-DC-**Aufwärtswandler** MT3608 (Platine XY-016, Chip B6287Y, 22-µH-Spule, Trimmer 3296, Micro-USB-Eingang), 2–24 V → bis 28 V, max. 2 A Spulenstrom | MT3608-oben.jpg, MT3608-unten.jpg | **Stromversorgung** | Macht aus 3 × AA (3,0–4,8 V) die 5 V für DevKitC VIN und MAX98306. Arbeitet ab 2 V Eingang, also bis die Zellen leer sind. Gemessen 31 × 18 mm, 7 mm hoch (MT3608-masse.jpg). Ohne Last auf 5,0 V trimmen, der Trimmer reicht bis 28 V |
| 11 | Drei Lautsprecher Ø ca. 28–30 mm, Metallkorb, rot/schwarze Litzen | IMG_3983 | **Lautsprecher** | Gehen. Vermutlich 8 Ω, 1–2 W. Mit dem Trichter davor wird es ordentlich laut, ein 50-mm-Chassis wäre lauter, ist aber nicht nötig |
| 12 | Mini-Taster 7 mm, SPST, Schließer, 6 Farben (GTIWUNG, 24 Stück) | nachgemeldet | **Abzug und Stimmenwahl** | Rot = Abzug, Gelb = Stimme weiterschalten. Loch Ø 7,3 mm. Ohne Muttern geliefert, Gewinde 7 mm (M7 × 0,75): gedruckte Mutter `3d/megafon-tastermutter.stl` mit modelliertem Gewinde M7 × 0,75, drei Probestücke mit 0,15/0,25/0,35 mm Spiel (1/2/3 Kerben); alternativ Muttern M7 × 0,75 kaufen |
| 13 | Mini-Kippschalter MTS-101, SPST, 2 Pins (Taiss, 10 Stück) | nachgemeldet | **Ein/Aus** | Loch Ø 6,3 mm links in der Seitenwand des Laufs, Mutter von außen |
| 14 | DC-DC-Abwärtswandler LM2596S-ADJ, 3,2–46 V → 1,25–35 V (Yizhet, 5 Stück; Platine V151, 47-µH-Spule, 2 × 100 µF) | LM2596.jpg, bestätigt | nein, bleibt in der Schublade | War im Plan mit 4 × AA als Wandlerpaar (3,3 V und 4,5 V) vorgesehen. Mit 3 × AA und Step-up nicht mehr nötig. 43 × 21 × 14 mm |
| 15 | Pegelwandler 4-Kanal bidirektional 3,3/5 V (3 Stück) | nachgemeldet | nein | Alle Signale sind 3,3 V, nichts umzusetzen |
| 16 | DC-DC-**Aufwärtswandler** XL6009E1 (TO-263, 12-mm-Schirmspule, SS34-Diode, zwei Elkos, Trimmer), 3–32 V → 5–35 V | XL6009.jpg, gemessen 43 × 21 mm | nein, bleibt in der Schublade | Auch ein Step-up, braucht aber 4 bis 5 V am Eingang und wäre mit 3 × AA schon bei halbvollen Zellen raus. Der Abwärtswandler (Nr. 14) hat dieselbe Platinengröße, trägt „LM2596S-ADJ“ auf dem Chip und eine flache runde Spule statt der eckigen Schirmspule |

## Geht es damit? Ja.

Die Kette sieht dann so aus:

```
MAX4466 (Mikro) --analog--> ESP32 ADC (GPIO 34)  ==>  Effekt  ==>  ESP32 DAC (GPIO 25) --analog--> MAX98306 --> Lautsprecher 28 mm
                                                                                                 ^
3 × AA (3,0–4,8 V) --> Kippschalter --> MT3608 Step-up 5 V --> DevKitC VIN und MAX98306 VDD ---+
```

Die Firmware ist darauf umgestellt (`AUDIO_ANALOG 1` in `firmware/include/config.h`):
Der ADC wird per I2S-DMA mit 16 kHz gelesen, der 8-Bit-DAC per Hardware-Timer
mit 16 kHz beschrieben. 8 Bit klingen etwas rau, für Mickey Maus aus einem
Spielzeugmegafon ist das genau richtig. Die digitale Kette (INMP441 +
MAX98357A) bleibt als Option im Code, falls du die Teile mal kaufst.

## Was fehlt

| Teil | Wozu | Zwingend? |
|---|---|---|
| Kontaktstreifen (Kupfer oder Messing, 4 mm breit, 0,3–0,5 mm; Büroklammern gehen auch) und drei Federn | gedrucktes Batteriefach: zwei Brücken, zwei Anschlüsse, Federn an den Minuspolen | laut Philipp vorhanden |
| Elko 470–1000 µF / 10 V an der 5-V-Schiene, nahe am Verstärker | Verstärker-Spitzen abfangen, sonst startet der ESP32 bei lauten Stellen neu | dringend empfohlen |
| 1 µF Kondensator (Keramik oder Folie) zwischen DAC und L+ | Gleichspannung des DAC vom Verstärkereingang fernhalten | empfohlen |
| 2-polige Steckverbindung (JST XH oder Dupont) für den Halter, Litzen, Schrumpfschlauch, Lötzinn, 14 × M3 × 10 und 1 × M3 × 12 (Philipp hat M3 × 10/12 reichlich), 3 Unterlegscheiben M3, 2 × M2 × 4 fürs Mikrofon oder Heißkleber | Mechanik | ja |

## Stromversorgung: 3 × AA und ein Aufwärtswandler

Entscheidung vom 24.10.2026: drei AA-Zellen in Reihe im Dreieck in einem
mitgedruckten Fach oben im Lauf, dazu der MT3608 aus dem Lager auf 5 V. Vorher waren 4 × AA mit
zwei LM2596 geplant, das brauchte selbstgebaute Kontakte und zwei Wandler. Mit
drei Zellen reicht ein Wandler: die 5 V versorgen den Board-Regler des DevKitC
(VIN) und den Verstärker, und der MT3608 arbeitet ab 2 V Eingang, also bis die
Zellen leer sind.

| Zellen | Spannung | Strom aus der Batterie, normal | bei Lautstärkespitzen | Urteil |
|---|---|---|---|---|
| 3 × AA | 4,8 → 3,0 V | 0,3–0,5 A | 0,9 A | geht |
| 2 × AA | 3,2 → 2,0 V | 0,5–0,7 A | 1,2 A und mehr | nicht zuverlässig: die Zellen brechen ein, der Wandler fällt unter 2 V, der ESP32 startet neu |

Der XL6009 (Nr. 16) ist dafür ungeeignet, er braucht 4 bis 5 V am Eingang.

Einstellen: MT3608 **ohne Last** auf 5,0 V trimmen und nachmessen. Der Trimmer
reicht bis 28 V, das überleben weder ESP32 noch Verstärker (max. 5,5 V). Zum
Flashen per USB den Kippschalter ausschalten.

| Zustand | Strom aus den Zellen (bei 3,6 V) |
|---|---|
| Reden, Verstärker arbeitet | 300–500 mA |
| Eingeschaltet, Abzug los | ca. 80 mA |
| Tiefschlaf | ca. 10 mA (Board-Regler und MT3608 im Leerlauf), deshalb der Kippschalter |

Macht etwa fünf bis sechs Stunden Dauerreden pro Satz Alkaline-Zellen.

## Verdrahtung (analoge Kette)

| Von | Nach | Hinweis |
|---|---|---|
| 3 × AA + | Kippschalter | |
| Kippschalter | MT3608 VIN+ | |
| 3 × AA – | MT3608 VIN– und GND überall | gemeinsame Masse |
| MT3608 OUT+ (5 V) | DevKitC **VIN**, MAX98306 **VDD**, Elko 470 µF | vorher ohne Last auf 5,0 V trimmen |
| MT3608 OUT– | GND | |
| DevKitC **3V3-Pin** | MAX4466 VCC | Ruhepegel des Mikrofons liegt dann bei 1,65 V |
| MAX4466 OUT | DevKitC **GPIO 34** (D34) | ADC1 Kanal 6 |
| DevKitC **GPIO 25** (D25, DAC1) | 1 µF | MAX98306 **L+** |
| MAX98306 L– | GND | Eingang ist differenziell, Minus auf Masse |
| MAX98306 SD | DevKitC **GPIO 4** | high = an, low = Shutdown |
| MAX98306 Gain-Jumper | offen lassen | 9 dB, reicht mit dem DAC |
| MAX98306 LOUT+ / LOUT– | Lautsprecher | R-Kanal bleibt frei |
| Abzug-Taster (rot, 7 mm) | **GPIO 27** und GND | interner Pullup, RTC-Pin fürs Aufwachen |
| Stimmen-Taster (gelb, 7 mm) | **GPIO 17** und GND | schaltet die Stimmen durch (`VOICE_BUTTON 1`, Standard) |

Den Gain-Trimmer am MAX4466 so einstellen, dass normales Sprechen in 10 cm
Abstand nicht übersteuert (seriell lässt sich der Pegel beobachten).

## 3D-Modell, Stand 6

`3d/megafon.scad` (24.10.2026, mit den nachgemessenen Bauteilen), fünf Teile:
Lauf Ø 72 × 100 mm mit abnehmbarer Haube (Lippe, vier Rastnasen, M3 hinten,
vorn vom Trichterkragen gehalten). Unter der Haube das mitgedruckte Batteriefach:
drei AA im Dreieck (A und C unten in zwei U-Rippen, B oben im Tal) zwischen zwei
Stirnwänden mit Kontakttaschen 12 × 12 × 1,5 mm, Nuten für die Brücken und je
einem Kabelloch. Zellen werden von oben eingelegt, die Haube hält sie.
Der Lauf ist hinten offen und wird von einer Rückplatte (4 × M3 × 10)
verschlossen; sie trägt das Mikrofon in einem Kasten (Kapsel außen bündig) und
den USB-C-Schlitz. Der DevKitC liegt mit den Stiften nach unten auf zwei
Schienen und wird von hinten eingeschoben, der MT3608 sitzt links auf einem
kleinen Deck in Kantenklammern, der Kippschalter links in der Seitenwand.
Der MAX98306 passt nicht neben den ESP32 und liegt in einer Kammer im Griff
(30 × 32 × 40 mm) unter dem Hals; Griff 40 × 38 mm mit Seitenschale (4 × M3)
und zwei Fenstern. Skizze der Kontakte: `3d/batteriefach.png`.
