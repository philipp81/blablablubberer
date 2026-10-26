/*
 * Blablablubber Megafon – Firmware für ESP32
 *
 * Abzug gedrückt halten  -> Mikrofon wird live mit der gewählten Stimme verfremdet
 *                           und über den Verstärker ausgegeben.
 * Schiebeschalter/Taster -> Mickey Maus / Darth Vader / Roboter
 * 3 Minuten nichts       -> Tiefschlaf, der Abzug weckt wieder auf.
 *
 * Zwei Audio-Ketten, Auswahl in include/config.h:
 *   AUDIO_ANALOG = 1: MAX4466 -> ESP32-ADC (I2S0-DMA), ESP32-DAC per Timer-Interrupt -> MAX98306
 *   AUDIO_ANALOG = 0: INMP441 -> I2S0, I2S1 -> MAX98357A
 * Pins und Einstellungen: include/config.h, Stimmen: include/voices.h
 */
#include <Arduino.h>
#include <driver/i2s.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include "config.h"
#include "voices.h"

#if AUDIO_ANALOG
#include <driver/adc.h>
#include <driver/dac.h>
#include <soc/rtc_io_reg.h>
#include <soc/sens_reg.h>
#endif

static VoiceFx fx;

// Kleiner Tongenerator für Start-Jingle und Stimmenwechsel (läuft am Effekt vorbei)
struct Beep {
    int   samplesLeft = 0;
    float phase = 0, step = 0, level = 0;
    void start(float hz, int ms, float lvl = 0.25f) {
        samplesLeft = SAMPLE_RATE * ms / 1000; step = 2.0f * (float)M_PI * hz / SAMPLE_RATE; level = lvl;
    }
    inline float next() {
        if (samplesLeft <= 0) return 0.0f;
        samplesLeft--;
        phase += step; if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
        float env = samplesLeft < 160 ? samplesLeft / 160.0f : 1.0f;   // kurzes Ausblenden gegen Knacken
        return level * env * sinf(phase);
    }
    bool active() const { return samplesLeft > 0; }
};
static Beep beep;
static int  beepQueue[4]; static int beepQueued = 0; static int beepMs = 120;

static void queueBeeps(std::initializer_list<int> hz, int ms) {
    beepQueued = 0; beepMs = ms;
    for (int h : hz) if (beepQueued < 4) beepQueue[beepQueued++] = h;
}

/* ================================================================== */
/*  Audio-Ein-/Ausgabe                                                 */
/* ================================================================== */
#if AUDIO_ANALOG

// ---- Eingang: SAR-ADC per I2S0-DMA, 16 Bit, 12 Bit davon Nutzdaten ----
static uint16_t adcBuf[FRAME_SAMPLES * ADC_DECIMATE];
static volatile uint32_t dacUnderruns = 0, dacWaits = 0;
struct AdcStats { uint32_t n = 0, clip = 0; int lo = 4095, hi = 0; double sum = 0; } adcStats;

static void audioInit() {
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
    cfg.sample_rate = ADC_I2S_RATE;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count = 6;
    cfg.dma_buf_len = FRAME_SAMPLES * ADC_DECIMATE;
    i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
    i2s_set_adc_mode(ADC_UNIT_1, MIC_ADC_CHANNEL);
    adc1_config_channel_atten(MIC_ADC_CHANNEL, ADC_ATTEN_DB_12);   // 0 .. ~3,1 V, Mikrofon-Ruhepegel liegt bei 1,65 V
    i2s_adc_enable(I2S_NUM_0);
}

// Liest einen Block und wandelt in Float -1..1
static int audioRead(float *out) {
    size_t got = 0;
    i2s_read(I2S_NUM_0, adcBuf, sizeof adcBuf, &got, portMAX_DELAY);
    int n = (int)(got / sizeof(uint16_t));
#if ADC_SWAP_PAIRS
    for (int i = 0; i + 1 < n; i += 2) { uint16_t t = adcBuf[i]; adcBuf[i] = adcBuf[i + 1]; adcBuf[i + 1] = t; }
#endif
    int m = n / ADC_DECIMATE;
    for (int i = 0; i < m; i++) {
        int acc = 0;
        for (int k = 0; k < ADC_DECIMATE; k++) {
            int raw = adcBuf[i * ADC_DECIMATE + k] & 0x0FFF;
            adcStats.n++; adcStats.sum += raw;
            if (raw < adcStats.lo) adcStats.lo = raw;
            if (raw > adcStats.hi) adcStats.hi = raw;
            if (raw < 40 || raw > 4055) adcStats.clip++;
            acc += raw;
        }
        out[i] = ((float)acc / ADC_DECIMATE - 2048.0f) / 2048.0f;
    }
    return m;
}

// ---- Ausgang: DAC1 (GPIO 25), per Hardware-Timer mit 16 kHz aus einem Ringpuffer gefüttert ----
static constexpr int OUT_RING = 1024;                 // Zweierpotenz
static volatile uint8_t outRing[OUT_RING];
static volatile uint32_t outWrite = 0, outRead = 0;
static volatile uint8_t  lastDac = 128;
static hw_timer_t *dacTimer = nullptr;

static void IRAM_ATTR onDacTimer() {
    uint8_t v = lastDac;
    if (outRead != outWrite) { v = outRing[outRead & (OUT_RING - 1)]; outRead++; lastDac = v; }
    else dacUnderruns++;
    // entspricht dac_output_voltage(DAC_CHANNEL_1, v), aber ohne Sperren -> ISR-tauglich
    SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, v, RTC_IO_PDAC1_DAC_S);
}

static void dacInit() {
    dac_output_enable(DAC_CHANNEL_1);
    dac_output_voltage(DAC_CHANNEL_1, 128);
    for (int i = 0; i < 2 * FRAME_SAMPLES; i++) outRing[i] = 128;   // kleiner Vorlauf gegen Leerlaufen
    outWrite = 2 * FRAME_SAMPLES;
    dacTimer = timerBegin(0, 5, true);                 // 80 MHz / 5 = 16 MHz Zählertakt
    timerAttachInterrupt(dacTimer, &onDacTimer, true);
    timerAlarmWrite(dacTimer, 16000000 / SAMPLE_RATE, true);   // 1000 Ticks = 62,5 µs = 16 kHz
    timerAlarmEnable(dacTimer);
}

// ADC (I2S-Takt) und DAC (Timer) laufen nicht exakt gleich schnell. Der Füllstand des Rings wird deshalb
// zwischen 128 und 512 Samples gehalten: zu voll -> ein Sample weglassen, zu leer -> eins doppelt (unhörbar selten).
static void audioWrite(const float *y, int n) {
    for (int i = 0; i < n; i++) {
        uint32_t fill = outWrite - outRead;
        if (fill > 512) { dacWaits++; continue; }
        uint8_t v = (uint8_t)(128 + (int)lrintf(y[i] * DAC_LEVEL));
        int reps = fill < 128 ? 2 : 1;
        while (reps--) { outRing[outWrite & (OUT_RING - 1)] = v; outWrite++; }
    }
}

static void audioStop() {
    timerAlarmDisable(dacTimer);
    dac_output_voltage(DAC_CHANNEL_1, 0);
    i2s_adc_disable(I2S_NUM_0);
    i2s_driver_uninstall(I2S_NUM_0);
}

#else  /* ---------------- I2S-Kette: INMP441 + MAX98357A ---------------- */

static int32_t micBuf[FRAME_SAMPLES * 2];   // 32-Bit-Slots, links + rechts; das Mikrofon liefert in einem davon
static int16_t outBuf[FRAME_SAMPLES * 2];   // 16-Bit, beide Kanäle gleich
static int     micSlot = -1;                // 0 = links, 1 = rechts, -1 = noch unbekannt
static float   slotEnergy[2] = {0, 0};
static int     slotFrames = 0;

static void audioInit() {
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    cfg.sample_rate = SAMPLE_RATE;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count = 6;
    cfg.dma_buf_len = FRAME_SAMPLES;
    i2s_pin_config_t pins = {};
    pins.mck_io_num = I2S_PIN_NO_CHANGE;
    pins.bck_io_num = MIC_PIN_SCK; pins.ws_io_num = MIC_PIN_WS;
    pins.data_out_num = I2S_PIN_NO_CHANGE; pins.data_in_num = MIC_PIN_SD;
    i2s_driver_install(MIC_I2S_PORT, &cfg, 0, NULL);
    i2s_set_pin(MIC_I2S_PORT, &pins);

    i2s_config_t out = {};
    out.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    out.sample_rate = SAMPLE_RATE;
    out.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    out.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    out.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    out.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    out.dma_buf_count = 6;
    out.dma_buf_len = FRAME_SAMPLES;
    i2s_pin_config_t opins = {};
    opins.mck_io_num = I2S_PIN_NO_CHANGE;
    opins.bck_io_num = AMP_PIN_BCLK; opins.ws_io_num = AMP_PIN_LRC;
    opins.data_out_num = AMP_PIN_DIN; opins.data_in_num = I2S_PIN_NO_CHANGE;
    i2s_driver_install(AMP_I2S_PORT, &out, 0, NULL);
    i2s_set_pin(AMP_I2S_PORT, &opins);
    i2s_zero_dma_buffer(AMP_I2S_PORT);
}

static int audioRead(float *out) {
    size_t got = 0;
    i2s_read(MIC_I2S_PORT, micBuf, sizeof micBuf, &got, portMAX_DELAY);
    int n = (int)(got / (2 * sizeof(int32_t)));
    // Welcher I2S-Slot trägt das Mikrofon? In der ersten Sekunde anhand des Signalanteils entscheiden.
    if (micSlot < 0) {
        for (int i = 1; i < n; i++) {
            slotEnergy[0] += fabsf((float)(micBuf[2 * i] - micBuf[2 * i - 2]));
            slotEnergy[1] += fabsf((float)(micBuf[2 * i + 1] - micBuf[2 * i - 1]));
        }
        if (++slotFrames >= SAMPLE_RATE / FRAME_SAMPLES) {
            micSlot = slotEnergy[1] > slotEnergy[0] ? 1 : 0;
            Serial.printf("Mikrofon liefert im %s Kanal.\n", micSlot ? "rechten" : "linken");
        }
    }
    int slot = micSlot < 0 ? 0 : micSlot;
    for (int i = 0; i < n; i++) out[i] = (float)(micBuf[2 * i + slot] >> 8) / 8388608.0f;   // 24 Bit -> -1..1
    return n;
}

static void dacInit() {}

static void audioWrite(const float *y, int n) {
    for (int i = 0; i < n; i++) { int16_t s = (int16_t)(y[i] * 30000.0f); outBuf[2 * i] = s; outBuf[2 * i + 1] = s; }
    size_t written = 0;
    i2s_write(AMP_I2S_PORT, outBuf, n * 2 * sizeof(int16_t), &written, portMAX_DELAY);
}

static void audioStop() {
    i2s_driver_uninstall(MIC_I2S_PORT);
    i2s_driver_uninstall(AMP_I2S_PORT);
}
#endif

/* ================================================================== */
/*  Bedienung                                                          */
/* ================================================================== */
RTC_DATA_ATTR static int savedVoice = 0;     // überlebt den Tiefschlaf
static Voice currentVoice;

#if VOICE_BUTTON
static Voice readVoiceSwitch() { return currentVoice; }     // wird per Taster weitergeschaltet
#else
static Voice readVoiceSwitch() {
    if (digitalRead(PIN_VOICE_A) == LOW) return VOICE_MICKEY;
    if (digitalRead(PIN_VOICE_B) == LOW) return VOICE_ROBOTER;
    return VOICE_DARTH;                       // Mittelstellung
}
#endif

static void goToSleep() {
    Serial.println("Tiefschlaf – Abzug drücken zum Aufwecken.");
    digitalWrite(AMP_PIN_SD_MODE, LOW);
    digitalWrite(PIN_LED, LOW);
    audioStop();
    rtc_gpio_pullup_en((gpio_num_t)PIN_TRIGGER);
    rtc_gpio_pulldown_dis((gpio_num_t)PIN_TRIGGER);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_TRIGGER, 0);
    esp_deep_sleep_start();
}

static bool     pressedStable = false;
static int      debounce = 0;
#if VOICE_BUTTON
static bool     voiceBtnStable = false;
static int      voiceDebounce = 0;
static uint32_t voicePressedAt = 0;
static bool     voiceLongDone = false;
#endif
static float    gate = 0.0f;
static int      squelchLeft = 0;                     // "Kschh" des Funkgeräts nach dem Loslassen
static constexpr int SQUELCH_LEN = SAMPLE_RATE * 220 / 1000;
static float    hpX1 = 0.0f, hpY1 = 0.0f;            // DC-Sperre fürs Mikrofon
static uint32_t lastActivity = 0, lastSound = 0;
static float    inBlock[FRAME_SAMPLES], outBlock[FRAME_SAMPLES];

void setup() {
    Serial.begin(115200);
    pinMode(PIN_TRIGGER, INPUT_PULLUP);
    pinMode(PIN_VOICE_A, INPUT_PULLUP);
    pinMode(PIN_VOICE_B, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    pinMode(AMP_PIN_SD_MODE, OUTPUT);
    digitalWrite(AMP_PIN_SD_MODE, HIGH);

    fx.reset();
    audioInit();
    dacInit();

    currentVoice = (Voice)(savedVoice % VOICE_COUNT);
    currentVoice = readVoiceSwitch();
    bool wokeUp = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0;
    Serial.printf("\nBlablablubber Megafon (%s) – Stimme: %s%s\n", AUDIO_ANALOG ? "analog" : "I2S",
                  VOICE_NAMES[currentVoice], wokeUp ? " (aufgewacht)" : "");
    queueBeeps({523, 659, 784}, 90);           // Start-Jingle
    lastActivity = lastSound = millis();
}

void loop() {
    int frames = audioRead(inBlock);
    uint32_t now = millis();

    // Abzug entprellen (5 Durchläufe = 20 ms)
    bool raw = digitalRead(PIN_TRIGGER) == LOW;
    if (raw != pressedStable) {
        if (++debounce >= 5) {
            pressedStable = raw; debounce = 0;
            if (!raw && currentVoice == VOICE_FUNK) squelchLeft = SQUELCH_LEN;
        }
    }
    else debounce = 0;
    if (pressedStable) lastActivity = now;
    digitalWrite(PIN_LED, pressedStable ? HIGH : LOW);

    // Stimme wählen: Schiebeschalter oder Taster, Wechsel wird mit einem Piep quittiert
    Voice v = readVoiceSwitch();
#if VOICE_BUTTON
    bool braw = digitalRead(PIN_VOICE_B) == LOW;
    // kurz drücken = nächste Stimme (beim Loslassen), 1 s halten = zurück zu Mickey Maus
    if (braw != voiceBtnStable) {
        if (++voiceDebounce >= 5) {
            voiceBtnStable = braw; voiceDebounce = 0;
            if (braw) { voicePressedAt = now; voiceLongDone = false; }
            else if (!voiceLongDone) v = (Voice)((currentVoice + 1) % VOICE_COUNT);
        }
    } else voiceDebounce = 0;
    if (voiceBtnStable && !voiceLongDone && now - voicePressedAt >= 1000) {
        voiceLongDone = true;
        v = VOICE_MICKEY;
        if (v == currentVoice) { queueBeeps({784, 659, 523}, 70); lastActivity = now; }
    }
#endif
    if (v != currentVoice) {
        currentVoice = v;
        savedVoice = v;
        lastActivity = now;
        queueBeeps({440 + 220 * (int)v}, 120);
        Serial.printf("Stimme: %s\n", VOICE_NAMES[v]);
    }

    // Nächsten Piep aus der Warteschlange starten
    if (!beep.active() && beepQueued > 0) {
        beep.start((float)beepQueue[0], beepMs);
        for (int i = 1; i < beepQueued; i++) beepQueue[i - 1] = beepQueue[i];
        beepQueued--;
    }

    const float gateStep = 1000.0f / (GATE_MS * SAMPLE_RATE);
    float gateTarget = pressedStable ? 1.0f : 0.0f;
    bool anySound = false;

    for (int i = 0; i < frames; i++) {
        float x = inBlock[i];
        float hp = x - hpX1 + 0.995f * hpY1; hpX1 = x; hpY1 = hp;   // Gleichanteil weg

        if (gate < gateTarget) gate = fminf(gateTarget, gate + gateStep);
        else if (gate > gateTarget) gate = fmaxf(gateTarget, gate - gateStep);

#if TEST_MODE == 1
        static float tph = 0; tph += 2.0f * (float)M_PI * 440.0f / SAMPLE_RATE; if (tph > 2.0f * (float)M_PI) tph -= 2.0f * (float)M_PI;
        float y = 0.7f * sinf(tph) * gate + beep.next();
#elif TEST_MODE == 2
        float y = hp * MASTER_GAIN * gate + beep.next();
#else
        // Eingang wird gegatet, damit Echo und Hall nach dem Loslassen ausklingen;
        // Funkgerät und Blubber erzeugen selbst Geräusche und werden am Ausgang gegatet
        float y = fx.process(hp * MASTER_GAIN * gate, currentVoice);
        if (currentVoice == VOICE_FUNK || currentVoice == VOICE_BLUBBER) y *= gate;
        if (squelchLeft > 0) { y += fx.squelch((float)squelchLeft / SQUELCH_LEN); squelchLeft--; }
        y = tanhf(y) + beep.next();
#endif
        if (y > 1.0f) y = 1.0f;
        if (y < -1.0f) y = -1.0f;
        if (fabsf(y) > 0.001f) anySound = true;
        outBlock[i] = y;
    }

    // Verstärker nur einschalten, wenn etwas zu hören ist (Shutdown spart Batterie)
    if (anySound) lastSound = now;
    digitalWrite(AMP_PIN_SD_MODE, now - lastSound < 300 ? HIGH : LOW);

    audioWrite(outBlock, frames);

#if DIAG_SERIAL && AUDIO_ANALOG
    // Jede Sekunde bei gedrücktem Abzug: echte ADC-Abtastrate, Ruhepegel, Spitze-Spitze, übersteuerte Samples, DAC-Aussetzer
    static uint32_t diagT = 0;
    if (now - diagT >= 1000) {
        uint32_t dt = now - diagT; diagT = now;
        if ((pressedStable || TEST_MODE) && adcStats.n > 0) {
            Serial.printf("ADC %5lu Hz  Mitte %4d (soll ~2048)  Spitze-Spitze %4d  übersteuert %4.1f %%  DAC-Lücken %lu  verworfen %lu  Ring %lu\n",
                          (unsigned long)(adcStats.n * 1000UL / dt), (int)(adcStats.sum / adcStats.n), adcStats.hi - adcStats.lo,
                          100.0 * adcStats.clip / adcStats.n, (unsigned long)dacUnderruns, (unsigned long)dacWaits, (unsigned long)(outWrite - outRead));
        }
        adcStats = AdcStats(); dacUnderruns = 0; dacWaits = 0;
    }
#endif

    if (now - lastActivity > SLEEP_AFTER_MS && !beep.active()) goToSleep();
}
