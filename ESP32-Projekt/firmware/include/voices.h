// Die Stimmen – teils portiert aus blablablubber.c (Desktop), hier für 16 kHz und Echtzeit.
#pragma once
#include <math.h>
#include <stdint.h>
#include "config.h"

enum Voice {
    VOICE_MICKEY = 0,   // hoch und quietschig
    VOICE_DARTH,        // tief und dunkel
    VOICE_ROBOTER,      // metallisch, Ringmodulator
    VOICE_ALIEN,        // hoch mit zitterndem Vibrato
    VOICE_MONSTER,      // ganz tief mit Knurren
    VOICE_ECHO,         // Halle mit Echo (Stadiondurchsage)
    VOICE_FUNK,         // Funkgerät: blechern, rauschend, "Kschh" beim Loslassen
    VOICE_BLUBBER,      // unter Wasser, mit Blubberblasen
    VOICE_COUNT
};
static const char *const VOICE_NAMES[VOICE_COUNT] = {
    "Mickey Maus", "Darth Vader", "Roboter", "Alien", "Monster", "Echo", "Funkgerät", "Blubber"
};

// Einpoliger Tiefpass, Grenzfrequenz in Hz
struct OnePole {
    float a = 0, z = 0;
    void set(float hz) { a = 1.0f - expf(-2.0f * (float)M_PI * hz / SAMPLE_RATE); }
    inline float lp(float x) { z += a * (x - z); return z; }
    inline float hp(float x) { return x - lp(x); }
};

class VoiceFx {
public:
    static constexpr int WINDOW = SAMPLE_RATE * 60 / 1000;      // 60 ms Grain-Fenster
    static constexpr int RING   = 4096;                          // > WINDOW + Vibrato, Zweierpotenz
    static constexpr int ECHO   = 8192;                          // 512 ms Echospeicher

    void reset() {
        for (int i = 0; i < RING; i++) ring_[i] = 0.0f;
        for (int i = 0; i < ECHO; i++) echo_[i] = 0.0f;
        w_ = 0; ew_ = 0; ph_ = 0.0f; lfo_ = 0.0f; lfo2_ = 0.0f;
        funkHp_.set(450); funkLp1_.set(2600); funkLp2_.set(2600);
        wetLp1_.set(650); wetLp2_.set(650); monLp_.set(1800);
        bubbleLeft_ = 0;
    }

    // Ein Eingangssample rein, ein Effektsample raus.
    inline float process(float in, Voice v) {
        ring_[w_] = in;
        float y;
        switch (v) {
        case VOICE_MICKEY:  y = pitch(1.6f); break;
        case VOICE_DARTH:   y = tanhf(1.6f * pitch(0.6f)); break;
        case VOICE_ROBOTER:
            y = in * (0.25f + 0.75f * sinf(osc(lfo_, 42.0f)));
            break;
        case VOICE_ALIEN:                                        // 6,5 Hz Vibrato auf leicht erhöhter Stimme
            y = pitch(1.35f, 0.0025f * SAMPLE_RATE * (1.0f + sinf(osc(lfo_, 6.5f))));
            break;
        case VOICE_MONSTER: {                                    // eine Oktave tiefer, 28-Hz-Knurren, angezerrt
            float p = monLp_.lp(pitch(0.5f));
            y = tanhf(2.2f * p * (0.55f + 0.45f * sinf(osc(lfo_, 28.0f))));
            break;
        }
        case VOICE_ECHO: {                                       // zwei Echos (250 und 420 ms) mit Rückkopplung
            float d1 = echo_[(ew_ - 4000) & (ECHO - 1)], d2 = echo_[(ew_ - 6720) & (ECHO - 1)];
            echo_[ew_] = in + 0.35f * d1 + 0.2f * d2;
            ew_ = (ew_ + 1) & (ECHO - 1);
            y = 0.6f * in + 0.4f * d1 + 0.28f * d2;
            break;
        }
        case VOICE_FUNK: {                                       // 450..2600 Hz, verzerrt, leises Rauschen
            float b = funkLp2_.lp(funkLp1_.lp(funkHp_.hp(in)));
            y = 0.6f * tanhf(6.0f * b) + 0.025f * noise();
            break;
        }
        case VOICE_BLUBBER: {                                    // schnelles, unruhiges Wackeln, dumpf, dazu Blasen
            float wob = sinf(osc(lfo_, 9.0f)) + 0.6f * sinf(osc(lfo2_, 13.7f));
            float p = pitch(0.9f, 0.003f * SAMPLE_RATE * (1.6f + wob));
            y = 1.6f * wetLp2_.lp(wetLp1_.lp(p)) * (0.75f + 0.25f * wob / 1.6f) + bubble();
            break;
        }
        default: y = in; break;
        }
        w_ = (w_ + 1) & (RING - 1);
        return y;
    }

    // Rauschstoß für das Funkgerät nach dem Loslassen (Wert 0..1 abklingend)
    inline float squelch(float env) { return env * 0.35f * funkLp1_.lp(noise()) * 3.0f; }

private:
    static inline float osc(float &ph, float hz) {
        ph += 2.0f * (float)M_PI * hz / SAMPLE_RATE;
        if (ph >= 2.0f * (float)M_PI) ph -= 2.0f * (float)M_PI;
        return ph;
    }
    inline float noise() { seed_ = seed_ * 1664525u + 1013904223u; return (float)(int32_t)seed_ / 2147483648.0f; }

    // Blubberblase: kurzer, nach oben gleitender Ton, zufällig etwa zweimal pro Sekunde
    inline float bubble() {
        if (bubbleLeft_ <= 0) {
            if ((seed_ >> 8) % (SAMPLE_RATE / 2) == 0) { bubbleLeft_ = SAMPLE_RATE * 45 / 1000; bubbleHz_ = 350.0f + 300.0f * (noise() + 1.0f); }
            noise();
            return 0.0f;
        }
        bubbleLeft_--;
        bubbleHz_ *= 1.00012f;
        float env = (float)bubbleLeft_ / (SAMPLE_RATE * 45 / 1000);
        return 0.18f * env * sinf(osc(bubblePh_, bubbleHz_));
    }

    // Lesen `delay` Samples hinter dem Schreibzeiger, linear interpoliert
    inline float read(float delay) const {
        float pos = (float)w_ - delay;
        while (pos < 0) pos += RING;
        int i0 = (int)pos;
        float fr = pos - (float)i0;
        int i1 = (i0 + 1) & (RING - 1);
        i0 &= (RING - 1);
        return ring_[i0] + fr * (ring_[i1] - ring_[i0]);
    }

    // Pitch-Shifter im Zeitbereich: zwei Leseköpfe, Hann-Überblendung (Summe = 1); extra = zusätzliche Verzögerung (Vibrato)
    inline float pitch(float ratio, float extra = 0.0f) {
        const float W = (float)WINDOW;
        ph_ += (ratio - 1.0f);
        while (ph_ >= W) ph_ -= W;
        while (ph_ <  0) ph_ += W;
        float phA = ph_, phB = phA + W * 0.5f;
        if (phB >= W) phB -= W;
        float gA = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * phA / W));
        float gB = 1.0f - gA;
        return gA * read(W - phA + extra) + gB * read(W - phB + extra);
    }

    float ring_[RING];
    float echo_[ECHO];
    int   w_ = 0, ew_ = 0;
    float ph_ = 0.0f, lfo_ = 0.0f, lfo2_ = 0.0f;
    OnePole funkHp_, funkLp1_, funkLp2_, wetLp1_, wetLp2_, monLp_;
    uint32_t seed_ = 12345;
    int   bubbleLeft_ = 0;
    float bubbleHz_ = 500.0f, bubblePh_ = 0.0f;
};
