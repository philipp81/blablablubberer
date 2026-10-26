// Pinbelegung und Einstellungen – hier anpassen, wenn anders verdrahtet.
#pragma once

// ---- Audio-Hardware auswählen ----
// 1 = analoge Kette aus dem Lagerbestand: MAX4466-Mikrofonmodul -> ESP32-ADC, ESP32-DAC -> MAX98306-Verstärker
// 0 = digitale Kette: INMP441 (I2S-Mikrofon) und MAX98357A (I2S-Verstärker)
#ifndef AUDIO_ANALOG
#define AUDIO_ANALOG     1
#endif

#define SAMPLE_RATE      16000   // Hz, reicht für Sprache und schont die CPU
#define FRAME_SAMPLES    64      // Samples pro Durchlauf (4 ms)

// ---- Analoge Kette (AUDIO_ANALOG = 1) ----
#define MIC_ADC_PIN      34      // MAX4466 OUT -> GPIO 34 (ADC1, Kanal 6). Modul-VCC an 3V3!
#define MIC_ADC_CHANNEL  ADC1_CHANNEL_6
#ifndef ADC_I2S_RATE
#define ADC_I2S_RATE     64000   // Abtastrate des ADC; gemessen: 64000 eingestellt = 63,7 kHz echt, 16000 eingestellt = 88 kHz (!)
#endif
#define ADC_DECIMATE     4       // je 4 ADC-Samples gemittelt = 16 kHz, nebenbei weniger Rauschen
#define ADC_SWAP_PAIRS   0       // auf 1 setzen, falls der Ton zischt: manche Chip-Revisionen liefern die Samples paarweise vertauscht
#define DAC_PIN          25      // DAC1 -> MAX98306 L+ (L- an GND); GPIO 25 ist fest DAC-Kanal 1
#define AMP_PIN_SD_MODE  4       // MAX98306 SD: high = Verstärker an, low = Shutdown (spart Strom)

// ---- Digitale Kette (AUDIO_ANALOG = 0) ----
#define MIC_I2S_PORT     I2S_NUM_0
#define MIC_PIN_SCK      26
#define MIC_PIN_WS       25
#define MIC_PIN_SD       33
#define AMP_I2S_PORT     I2S_NUM_1
#define AMP_PIN_BCLK     14
#define AMP_PIN_LRC      15
#define AMP_PIN_DIN      32

// ---- Bedienung ----
#define PIN_TRIGGER      27      // Abzug: Taster nach GND (RTC-fähig, weckt aus dem Tiefschlaf)
#ifndef VOICE_BUTTON
#define VOICE_BUTTON     1
#endif
// 1 = ein Taster an GPIO 17 schaltet die Stimmen durch (Lagerbestand: Mini-Taster), 0 = Schiebeschalter ON-OFF-ON an GPIO 16/17
#define PIN_VOICE_A      16      // Schalter links  -> GPIO 16 nach GND  (Mickey Maus)
#define PIN_VOICE_B      17      // Schalter rechts -> GPIO 17 nach GND  (Roboter), Mitte = Darth Vader; bei VOICE_BUTTON der Taster
#define PIN_LED          2       // Status-LED (auf dem DevKitC fest verbaut)

// ---- Verhalten ----
#define MASTER_GAIN      1.5f    // Verstärkung vor dem weichen Begrenzer (tanh); zu hoch = Verzerrung. Erst den MAX4466-Trimmer einstellen
#define DAC_LEVEL        70      // Ausschlag des DAC um die Mitte 128 (max. 127). 70 = ca. 0,9 V Spitze; mit 9 dB am MAX98306
                                 // sind das ca. 2,5 V am Lautsprecher. Krächzt es bei lauten Stellen, wieder runter

// ---- Test und Diagnose ----
// 0 = normal, 1 = Abzug spielt einen 440-Hz-Sinus (prüft DAC, Verstärker, Lautsprecher, Stromversorgung),
// 2 = Abzug reicht das Mikrofon ohne Effekt und ohne Begrenzer durch (prüft Mikrofon und Pegel)
#ifndef TEST_MODE
#define TEST_MODE        0
#endif
#define DIAG_SERIAL      1       // 1 = bei gedrücktem Abzug jede Sekunde Pegel und Taktwerte auf der seriellen Konsole
#define GATE_MS          5       // Ein-/Ausblendrampe beim Drücken/Loslassen
#define SLEEP_AFTER_MS   (3UL * 60UL * 1000UL)   // Tiefschlaf nach 3 Minuten ohne Abzug, Wecken per Abzug
