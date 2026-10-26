/*
 * Blablablubber – nimmt deine Stimme über das Mikrofon auf, wartet ein paar
 * Sekunden und plappert alles mit einer verrückten Stimme nach.
 *
 * Aufbau: Mikrofon -> Ringpuffer (Verzögerung) -> Stimm-Effekt -> Lautsprecher
 * Audio-Backend: miniaudio (third_party/miniaudio.h), Implementierung liegt in
 * miniaudio_impl.c.
 */
#define _DARWIN_C_SOURCE
#include "third_party/miniaudio.h"

#include <CoreFoundation/CoreFoundation.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SR                48000      /* Abtastrate in Hz */
#define MIN_DELAY_MS      500
#define MAX_DELAY_MS      30000
#define DEFAULT_DELAY_MS  5000
#define EXTRA_SEC         4.0        /* Reserve im Ringpuffer für Pitch-Fenster, Rückwärts-Grains, Vibrato */
#define WALKIE_MAX_SEC    60         /* längster Clip im Walkie-Talkie-Modus */
#define RANDOM_EVERY_SEC  4.0
#define OUTBUF_SEC        70         /* so viel Ausgabe wird fürs Speichern vorgehalten */
#define SAVE_MAX_SEC      60
#define SILENCE_PEAK      0.0056f    /* -45 dBFS: darunter gilt ein 10-ms-Block als still */

extern char **environ;

/* ------------------------------------------------------------------ */
/* Stimmen                                                             */
/* ------------------------------------------------------------------ */

enum {
    V_NORMAL = 0, V_MICKEY, V_HELIUM, V_DARTH, V_ROBOTER, V_ALIEN,
    V_BETRUNKEN, V_KATHEDRALE, V_TELEFON, V_RUECKWAERTS, V_COUNT
};

typedef struct { char key; const char *name; const char *desc; } voice_info_t;

static const voice_info_t VOICES[V_COUNT] = {
    {'0', "normal",      "nur Verzögerung, deine echte Stimme"},
    {'1', "mickey",      "Mickey Maus: hoch und quietschig"},
    {'2', "helium",      "Helium-Ballon: noch höher, Chipmunk-Level"},
    {'3', "darth",       "Darth Vader: tief, dunkel, bedrohlich"},
    {'4', "roboter",     "Roboter/Dalek: metallisch per Ringmodulator"},
    {'5', "alien",       "Alien: hoch und mit zitterndem Vibrato"},
    {'6', "betrunken",   "Betrunken: taumelt langsam auf und ab"},
    {'7', "kathedrale",  "Kathedrale: riesiger Hall und Echo"},
    {'8', "telefon",     "Telefon: blechern, schmalbandig, verzerrt"},
    {'9', "rueckwaerts", "Rückwärts: in 1,5-s-Häppchen verkehrt herum"},
};

/* ------------------------------------------------------------------ */
/* DSP-Bausteine                                                       */
/* ------------------------------------------------------------------ */

typedef struct { double b0, b1, b2, a1, a2, z1, z2; } biquad_t;

static void biquad_set(biquad_t *f, bool highpass, double fc, double q)
{
    double w0 = 2.0 * M_PI * fc / SR, cw = cos(w0), sw = sin(w0);
    double alpha = sw / (2.0 * q);
    double b0, b1, b2, a0 = 1.0 + alpha, a1 = -2.0 * cw, a2 = 1.0 - alpha;
    if (highpass) { b0 = (1.0 + cw) / 2.0; b1 = -(1.0 + cw); b2 = b0; }
    else          { b0 = (1.0 - cw) / 2.0; b1 =  (1.0 - cw); b2 = b0; }
    f->b0 = b0 / a0; f->b1 = b1 / a0; f->b2 = b2 / a0;
    f->a1 = a1 / a0; f->a2 = a2 / a0;
    f->z1 = f->z2 = 0.0;
}

static inline float biquad_run(biquad_t *f, float x)
{
    double y = f->b0 * x + f->z1;
    f->z1 = f->b1 * x - f->a1 * y + f->z2;
    f->z2 = f->b2 * x - f->a2 * y;
    return (float)y;
}

/* Kammfilter mit Rückkopplung = ein einzelnes Echo, das sich wiederholt */
typedef struct { float *buf; size_t n, w; float fb; } comb_t;

static void comb_init(comb_t *c, double len_sec, float fb)
{
    c->n = (size_t)(len_sec * SR) + 1;
    c->buf = calloc(c->n, sizeof(float));
    c->w = 0;
    c->fb = fb;
}

static inline float comb_run(comb_t *c, float x)
{
    size_t r = c->w + 1;
    if (r >= c->n) r = 0;
    float e = c->buf[r];
    c->buf[c->w] = x + c->fb * e;
    c->w = r;
    return e;
}

typedef struct {
    float   *buf;          /* Ringpuffer mit dem Mikrofonsignal */
    size_t   n, w;
    double   ph;           /* Phase des Pitch-Shifters (Samples, 0..W) */
    double   lfo;          /* LFO-Phase (rad) für Vibrato/Taumeln */
    double   ring;         /* Phase des Ringmodulators (rad) */
    uint64_t t;            /* Sample-Zähler (Rückwärts-Grains) */
    biquad_t hp, lp;
    comb_t   comb1, comb2;
    int      cur;          /* aktive Stimme im Audio-Thread */
    double   delay;        /* aktive Verzögerung in Samples */
    int      fade;         /* 0 stabil, 1 ausblenden, 2 einblenden */
    float    gain;
    float    in_gain;      /* Mikrofon-Gate: 1 = Aufnahme läuft, 0 = stumm */
    float    out_gain;     /* Lautsprecher-Gate: im Walkie-Talkie-Modus während der Aufnahme stumm */
} dsp_t;

static void dsp_init(dsp_t *d, int voice, double delay_samples)
{
    memset(d, 0, sizeof *d);
    d->n = (size_t)((WALKIE_MAX_SEC + EXTRA_SEC) * SR);
    d->buf = calloc(d->n, sizeof(float));
    biquad_set(&d->hp, true, 300.0, 0.707);
    biquad_set(&d->lp, false, 3400.0, 0.707);
    comb_init(&d->comb1, 0.211, 0.55f);
    comb_init(&d->comb2, 0.347, 0.50f);
    d->cur = voice;
    d->delay = delay_samples;
    d->gain = 1.0f;
    d->in_gain = 0.0f;   /* Mikro startet zu, das Gate öffnet in 5 ms, sobald aufgenommen wird */
    d->out_gain = 1.0f;
}

static void dsp_free(dsp_t *d)
{
    free(d->buf); free(d->comb1.buf); free(d->comb2.buf);
}

/* Liest das Signal `delay` Samples hinter dem Schreibzeiger, linear interpoliert. */
static inline float rb_read(const dsp_t *d, double delay)
{
    double pos = (double)d->w - delay;
    if (pos < 0) pos += (double)d->n;
    size_t i0 = (size_t)pos;
    if (i0 >= d->n) i0 = d->n - 1;
    double fr = pos - (double)i0;
    size_t i1 = i0 + 1;
    if (i1 >= d->n) i1 = 0;
    return d->buf[i0] + (float)fr * (d->buf[i1] - d->buf[i0]);
}

/*
 * Pitch-Shifter im Zeitbereich: zwei Leseköpfe laufen mit anderer
 * Geschwindigkeit als der Schreibzeiger durch den Puffer und werden mit
 * Hann-Fenstern überblendet (Summe der Fenster ist konstant 1).
 */
static inline float pitch_read(dsp_t *d, double D, double ratio)
{
    const double W = 0.060 * SR; /* 60 ms Grain-Fenster */
    d->ph += (ratio - 1.0);
    while (d->ph >= W) d->ph -= W;
    while (d->ph <  0) d->ph += W;
    double phA = d->ph, phB = phA + W * 0.5;
    if (phB >= W) phB -= W;
    double gA = 0.5 * (1.0 - cos(2.0 * M_PI * phA / W));
    double gB = 1.0 - gA;
    return (float)(gA * rb_read(d, D + W - phA) + gB * rb_read(d, D + W - phB));
}

static inline double wrap2pi(double x) { return x >= 2.0 * M_PI ? x - 2.0 * M_PI : x; }

static float render_voice(dsp_t *d, int v, double D)
{
    switch (v) {
    case V_MICKEY:  return pitch_read(d, D, 1.6);
    case V_HELIUM:  return pitch_read(d, D, 2.1);
    case V_DARTH:   return tanhf(1.6f * pitch_read(d, D, 0.58));
    case V_ROBOTER: {
        d->ring = wrap2pi(d->ring + 2.0 * M_PI * 42.0 / SR);
        return rb_read(d, D) * (0.25f + 0.75f * (float)sin(d->ring));
    }
    case V_ALIEN: {
        d->lfo = wrap2pi(d->lfo + 2.0 * M_PI * 6.5 / SR);
        double mod = 0.0025 * SR * (1.0 + sin(d->lfo));
        return pitch_read(d, D + mod, 1.35);
    }
    case V_BETRUNKEN: {
        d->lfo = wrap2pi(d->lfo + 2.0 * M_PI * 0.6 / SR);
        double mod = 0.030 * SR * (1.0 + sin(d->lfo));
        return pitch_read(d, D + mod, 0.92);
    }
    case V_KATHEDRALE: {
        float x = rb_read(d, D);
        float e1 = comb_run(&d->comb1, x);
        float e2 = comb_run(&d->comb2, x);
        return 0.7f * x + 0.5f * e1 + 0.4f * e2;
    }
    case V_TELEFON: {
        float x = biquad_run(&d->lp, biquad_run(&d->hp, rb_read(d, D)));
        return 0.8f * tanhf(4.0f * x);
    }
    case V_RUECKWAERTS: {
        const double G = 1.5 * SR, F = 0.020 * SR;      /* Grain 1,5 s, Fade 20 ms */
        double s = fmod((double)d->t, G);
        double g = fmin(1.0, fmin(s / F, (G - s) / F));
        return (float)(g * rb_read(d, D + 2.0 * s));   /* Delay wächst doppelt so schnell wie die Zeit = rückwärts */
    }
    case V_NORMAL:
    default:
        return rb_read(d, D);
    }
}

static inline float process_sample(dsp_t *d, float in, int want_voice, double want_delay, int want_rec, int want_out)
{
    /* Mikrofon-Gate mit 5 ms Rampe: ohne Aufnahme landet Stille im Puffer */
    const float gstep = 1.0f / (0.005f * SR);
    float target = want_rec ? 1.0f : 0.0f;
    if (d->in_gain < target)      d->in_gain = fminf(target, d->in_gain + gstep);
    else if (d->in_gain > target) d->in_gain = fmaxf(target, d->in_gain - gstep);
    d->buf[d->w] = in * d->in_gain;

    /* Stimm-/Delay-Wechsel: 10 ms aus- und wieder einblenden, damit es nicht knackt */
    if (d->fade == 0 && (want_voice != d->cur || want_delay != d->delay)) d->fade = 1;
    const float step = 1.0f / (0.010f * SR);
    if (d->fade == 1) {
        d->gain -= step;
        if (d->gain <= 0.0f) { d->gain = 0.0f; d->cur = want_voice; d->delay = want_delay; d->fade = 2; }
    } else if (d->fade == 2) {
        d->gain += step;
        if (d->gain >= 1.0f) { d->gain = 1.0f; d->fade = 0; }
    }

    float otarget = want_out ? 1.0f : 0.0f;
    if (d->out_gain < otarget)      d->out_gain = fminf(otarget, d->out_gain + gstep);
    else if (d->out_gain > otarget) d->out_gain = fmaxf(otarget, d->out_gain - gstep);

    float y = render_voice(d, d->cur, d->delay) * d->gain * d->out_gain;
    d->t++;
    if (++d->w >= d->n) d->w = 0;
    return fmaxf(-1.0f, fminf(1.0f, y));
}

/* ------------------------------------------------------------------ */
/* Audio-Callback                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    dsp_t      dsp;
    atomic_int voice;      /* gewünschte Stimme (vom Hauptthread gesetzt) */
    atomic_int delay_ms;   /* gewünschte Verzögerung */
    atomic_int recording;  /* 1 = Mikrofon offen */
    atomic_int walkie;     /* 1 = Wiedergabe startet beim Ende der Aufnahme statt nach festem Delay */
    atomic_int clip_ms;    /* Länge des zuletzt aufgenommenen Clips (für die Anzeige) */
    int        was_rec;    /* nur Audio-Thread */
    uint64_t   rec_start;  /* Sample-Zähler beim Aufnahmestart, nur Audio-Thread */
    double     walkie_delay;
    atomic_int testtone;   /* 1 = statt Mikrofon eine kleine Melodie einspeisen */
    double     tone_phase;
    uint64_t   tone_t;
    float      *outbuf;    /* Ringpuffer mit der fertigen Ausgabe (fürs Speichern) */
    size_t      on;
    atomic_size_t ow;
    atomic_int in_peak;    /* Spitzenpegel seit letztem Auslesen, in 1/1000 */
    atomic_int out_peak;
} app_t;

static void store_max(atomic_int *dst, int v)
{
    int old = atomic_load(dst);
    while (v > old && !atomic_compare_exchange_weak(dst, &old, v)) {}
}

static void data_callback(ma_device *dev, void *out, const void *in, ma_uint32 frames)
{
    app_t *app = (app_t *)dev->pUserData;
    const float *x = (const float *)in;
    float *y = (float *)out;
    int v = atomic_load(&app->voice);
    int rec = atomic_load(&app->recording);
    int walkie = atomic_load(&app->walkie);
    double D;
    if (walkie) {
        /* Delay = Länge der Aufnahme: beim Loslassen springt der Lesekopf genau an den Anfang des Clips */
        if (rec && !app->was_rec) app->rec_start = app->dsp.t;
        if (!rec && app->was_rec) {
            uint64_t len = app->dsp.t - app->rec_start;
            if (len > (uint64_t)WALKIE_MAX_SEC * SR) len = (uint64_t)WALKIE_MAX_SEC * SR;
            app->walkie_delay = (double)len;
            atomic_store(&app->clip_ms, (int)(len * 1000 / SR));
        }
        D = app->walkie_delay;
    } else {
        D = atomic_load(&app->delay_ms) * (SR / 1000.0);
    }
    app->was_rec = rec;
    int want_out = !(walkie && rec);   /* Walkie-Talkie: beim Senden ist der Lautsprecher aus */
    size_t ow = atomic_load_explicit(&app->ow, memory_order_relaxed);
    float ip = 0.0f, op = 0.0f;

    int tone = atomic_load(&app->testtone);
    static const double MELODY[] = { 220.0, 277.2, 329.6, 440.0, 329.6, 277.2 };

    for (ma_uint32 i = 0; i < frames; i++) {
        float s = x ? x[i] : 0.0f;
        if (tone) {
            double f = MELODY[(app->tone_t / (SR * 4 / 10)) % 6];   /* alle 0,4 s ein neuer Ton */
            app->tone_phase = wrap2pi(app->tone_phase + 2.0 * M_PI * f / SR);
            s = 0.3f * (float)sin(app->tone_phase);
            app->tone_t++;
        }
        float o = process_sample(&app->dsp, s, v, D, rec, want_out);
        ip = fmaxf(ip, rec ? fabsf(s) : 0.0f);
        op = fmaxf(op, fabsf(o));
        y[i] = o;
        app->outbuf[ow] = o;
        if (++ow >= app->on) ow = 0;
    }
    atomic_store_explicit(&app->ow, ow, memory_order_release);
    store_max(&app->in_peak, (int)(ip * 1000.0f));
    store_max(&app->out_peak, (int)(op * 1000.0f));
}

/* ------------------------------------------------------------------ */
/* Terminal-Schnickschnack                                             */
/* ------------------------------------------------------------------ */

#define ESC   "\x1b["
#define RESET ESC "0m"
#define BOLD  ESC "1m"

static const int RAINBOW[] = {196,202,208,214,220,226,190,154,118,82,46,48,50,51,45,39,33,27,57,93,129,165,201,199,197};
#define RAINBOW_N ((int)(sizeof RAINBOW / sizeof RAINBOW[0]))

static void color256(int c) { printf(ESC "38;5;%dm", c); }

/* Druckt einen UTF-8-String, jedes Zeichen in der nächsten Regenbogenfarbe. */
static void rainbow(const char *s, int offset)
{
    int k = offset;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if ((*p & 0xC0) != 0x80) color256(RAINBOW[((k++ % RAINBOW_N) + RAINBOW_N) % RAINBOW_N]);
        putchar(*p);
    }
    fputs(RESET, stdout);
}

static void print_banner(void)
{
    static const char *ORN = "✦ ★ ✧ ❀ ♫ ❦ ✦ ★ ✧ ❀ ♫ ❦ ✦ ★ ✧ ❀ ♫ ❦ ✦ ★ ✧ ❀ ♫ ❦ ✦ ★ ✧";
    static const char *ART[] = {
        "╔╗  ╦   ╔═╗ ╔╗  ╦   ╔═╗ ╔╗  ╦ ╦ ╔╗  ╔╗  ╔═╗ ╔═╗",
        "╠╩╗ ║   ╠═╣ ╠╩╗ ║   ╠═╣ ╠╩╗ ║ ║ ╠╩╗ ╠╩╗ ╠═  ╠╦╝",
        "╚═╝ ╩═╝ ╩ ╩ ╚═╝ ╩═╝ ╩ ╩ ╚═╝ ╚═╝ ╚═╝ ╚═╝ ╚═╝ ╩╚═",
    };
    putchar('\n');
    fputs("  ", stdout); rainbow(ORN, 0); putchar('\n');
    for (int i = 0; i < 3; i++) {
        fputs("   " BOLD, stdout); rainbow(ART[i], i * 3); putchar('\n');
    }
    fputs("  ", stdout); rainbow(ORN, 7); putchar('\n');
    fputs("   ", stdout); color256(213); fputs("♪ spricht dir alles nach – nur später und viel alberner ♪" RESET "\n\n", stdout);
}

static void print_voices(int current)
{
    for (int i = 0; i < V_COUNT; i++) {
        printf("   %s", i == current ? BOLD "▶" : " ");
        color256(226); printf(" %c ", VOICES[i].key);
        color256(RAINBOW[(i * 3) % RAINBOW_N]); printf(BOLD "%-11s" RESET, VOICES[i].name);
        color256(250); printf(" %s" RESET "\n", VOICES[i].desc);
    }
    printf("   "); color256(226); printf(" z "); color256(201); printf(BOLD "%-11s" RESET, "zufall");
    color256(250); printf(" alle %.0f s eine andere Stimme" RESET "\n", RANDOM_EVERY_SEC);
}

static void draw_meter(float level, int width)
{
    for (int i = 0; i < width; i++) {
        if (level > (float)i / (float)width) {
            color256(i < width * 6 / 10 ? 46 : i < width * 85 / 100 ? 226 : 196);
            fputs("█", stdout);
        } else {
            color256(238);
            fputs("·", stdout);
        }
    }
    fputs(RESET, stdout);
}

/* Spitzenpegel (0..1) -> Anzeige (0..1), logarithmisch: -40 dB = 0, 0 dB = 1 */
static float to_display(float peak)
{
    if (peak <= 0.0001f) return 0.0f;
    float v = 1.0f + log10f(peak) / 2.0f;
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

typedef struct {
    bool   recording, playing, walkie, saving, random_mode;
    double rec_seconds, play_seconds;   /* laufende Aufnahme bzw. Rest der Wiedergabe */
    int    voice, delay_ms, clip_ms;
    double elapsed;
    float  in_lvl, out_lvl;
} status_t;

static void draw_status(const status_t *st)
{
    int secs = (int)st->elapsed;
    fputs("\r" ESC "2K   ", stdout);
    if (st->recording)    { color256(196); printf(BOLD "●%4.1fs " RESET, st->rec_seconds); }
    else if (st->playing) { color256(118); printf(BOLD "▶%4.1fs " RESET, st->play_seconds); }
    else                  { color256(240); fputs("○ ---- " RESET, stdout); }
    color256(226); fputs("♫ ", stdout);
    color256(RAINBOW[(st->voice * 3) % RAINBOW_N]); printf(BOLD "%-11s" RESET, VOICES[st->voice].name);
    if (st->random_mode) { color256(201); fputs("⚄", stdout); } else fputs(" ", stdout);
    color256(245); printf(" │ ");
    if (st->walkie) { color256(245); printf("Clip "); color256(51); printf("%4.1f s", st->clip_ms / 1000.0); }
    else            { color256(245); printf("Delay "); color256(51); printf("%4.1f s", st->delay_ms / 1000.0); }
    color256(245); printf(" │ 🎤 "); draw_meter(st->in_lvl, 10);
    color256(245); printf(" │ 🔊 "); draw_meter(st->out_lvl, 10);
    color256(245);
    if (st->saving) { fputs(" │ ", stdout); color256(226); fputs("⏳💾" RESET, stdout); }
    else printf(" │ %02d:%02d" RESET, secs / 60, secs % 60);
    fflush(stdout);
}

static struct termios g_termios;
static bool g_tty_raw = false;

static void tty_raw(void)
{
    if (!isatty(STDIN_FILENO)) return;
    if (tcgetattr(STDIN_FILENO, &g_termios) != 0) return;
    struct termios t = g_termios;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &t) == 0) g_tty_raw = true;
    fputs(ESC "?25l", stdout); /* Cursor verstecken */
}

static void tty_restore(void)
{
    if (g_tty_raw) tcsetattr(STDIN_FILENO, TCSANOW, &g_termios);
    g_tty_raw = false;
    fputs(ESC "?25h" RESET, stdout);
    fflush(stdout);
}

static volatile sig_atomic_t g_stop = 0;
static void on_signal(int sig) { (void)sig; g_stop = 1; }

static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + ts.tv_nsec / 1e9;
}

/* ------------------------------------------------------------------ */
/* Aufnahme-Modi: Leertaste halten / umschalten / immer an             */
/* ------------------------------------------------------------------ */

enum { MODE_HALTEN = 0, MODE_UMSCHALTEN, MODE_IMMER, MODE_COUNT };

static const char *MODE_NAME[MODE_COUNT] = { "halten", "umschalten", "immer" };
static const char *MODE_TEXT[MODE_COUNT] = {
    "Walkie-Talkie: Leertaste gedrückt halten, beim Loslassen kommt die Antwort",
    "Leertaste startet die Aufnahme, nochmal Leertaste beendet sie und spielt ab",
    "Dauer-Echo: Mikrofon immer an, Wiedergabe nach festem Delay – nur mit Kopfhörern!",
};

/* Tastenwiederholung aus den macOS-Einstellungen (Einheit 15 ms), Rückgabe in Sekunden */
static double key_repeat_pref(const char *name, double default_units)
{
    CFStringRef key = CFStringCreateWithCString(NULL, name, kCFStringEncodingUTF8);
    CFPropertyListRef v = CFPreferencesCopyValue(key, kCFPreferencesAnyApplication,
                                                 kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
    double units = default_units;
    if (v) {
        if (CFGetTypeID(v) == CFNumberGetTypeID()) CFNumberGetValue((CFNumberRef)v, kCFNumberDoubleType, &units);
        CFRelease(v);
    }
    CFRelease(key);
    return units * 0.015;
}

/*
 * Ein Terminal meldet kein Loslassen einer Taste. Gehaltene Leertaste erkennen
 * wir deshalb an der Tastenwiederholung: solange innerhalb des Timeouts wieder
 * ein Leerzeichen kommt, gilt die Taste als gedrückt. Die Abstände werden im
 * Betrieb gemessen, damit das Timeout zur Einstellung des Macs passt.
 */
typedef struct {
    double init_delay;   /* Zeit bis zur ersten Wiederholung */
    double repeat_iv;    /* Abstand der Wiederholungen */
    double timeout;
    double last_press;
    int    presses;      /* Ereignisse im aktuellen Halten */
} hold_t;

static double clampd(double x, double lo, double hi) { return x < lo ? lo : x > hi ? hi : x; }

static void hold_update_timeout(hold_t *h)
{
    h->timeout = clampd(fmax(h->init_delay, h->repeat_iv) * 1.4 + 0.08, 0.2, 2.0);
}

static void hold_init(hold_t *h)
{
    h->init_delay = clampd(key_repeat_pref("InitialKeyRepeat", 25), 0.1, 1.3);
    h->repeat_iv  = clampd(key_repeat_pref("KeyRepeat", 6), 0.02, 1.3);
    h->last_press = -1.0;
    h->presses = 0;
    hold_update_timeout(h);
}

static void hold_press(hold_t *h, double now, bool was_recording)
{
    double gap = now - h->last_press;
    if (was_recording) {
        h->presses++;
        if (h->presses == 2) h->init_delay = clampd(gap, 0.1, 1.3);
        else                 h->repeat_iv  = clampd(0.5 * h->repeat_iv + 0.5 * gap, 0.02, 1.3);
    } else {
        /* nur ein Ereignis beim letzten Halten und gleich wieder eins: Timeout war zu kurz */
        if (h->presses == 1 && h->last_press >= 0 && gap < 1.5) h->init_delay = clampd(gap, 0.1, 1.3);
        h->presses = 1;
    }
    h->last_press = now;
    hold_update_timeout(h);
}

static void print_mode(int mode)
{
    fputs("\r" ESC "2K   ", stdout);
    color256(213); printf("Modus: %s" RESET "\n", MODE_TEXT[mode]);
}

/* ------------------------------------------------------------------ */
/* Speichern als WAV (+ M4A)                                           */
/* ------------------------------------------------------------------ */

/* Kopiert die letzten max_samples der Ausgabe in dst. Der Audio-Thread schreibt
 * währenddessen weiter, aber der Puffer ist 10 s größer als das Maximum, also
 * kann er den kopierten Bereich nicht einholen. Zur Sicherheit wird geprüft. */
static size_t snapshot_output(app_t *app, float *dst, size_t max_samples)
{
    size_t n = app->on;
    size_t w1 = atomic_load_explicit(&app->ow, memory_order_acquire);
    size_t start = (w1 + n - max_samples) % n;
    size_t first = n - start < max_samples ? n - start : max_samples;
    memcpy(dst, app->outbuf + start, first * sizeof(float));
    if (first < max_samples) memcpy(dst + first, app->outbuf, (max_samples - first) * sizeof(float));
    size_t w2 = atomic_load_explicit(&app->ow, memory_order_acquire);
    if ((w2 + n - w1) % n > n - max_samples) return 0;
    return max_samples;
}

static float block_peak(const float *x, size_t n)
{
    float p = 0.0f;
    for (size_t i = 0; i < n; i++) p = fmaxf(p, fabsf(x[i]));
    return p;
}

/* Sucht den letzten Abschnitt mit Ton: Ende = letzter lauter 10-ms-Block,
 * Anfang = nach der letzten Pause länger als gap_sec (gap_sec <= 0: alles). */
static bool find_clip(const float *buf, size_t len, double gap_sec, size_t *out_start, size_t *out_end)
{
    const size_t B = SR / 100;
    long nb = (long)(len / B), last = -1;
    for (long b = nb - 1; b >= 0; b--)
        if (block_peak(buf + b * B, B) > SILENCE_PEAK) { last = b; break; }
    if (last < 0) return false;

    long first = last, quiet = 0, gap_blocks = gap_sec > 0 ? (long)(gap_sec * 100) : nb + 1;
    for (long b = last; b >= 0; b--) {
        if (block_peak(buf + b * B, B) > SILENCE_PEAK) { first = b; quiet = 0; }
        else if (++quiet >= gap_blocks) break;
    }
    size_t pre = (size_t)(0.15 * SR), post = (size_t)(0.3 * SR);
    size_t st = (size_t)first * B, en = (size_t)(last + 1) * B;
    *out_start = st > pre ? st - pre : 0;
    *out_end   = en + post > len ? len : en + post;
    return true;
}

static void put_u32(uint8_t *p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
static void put_u16(uint8_t *p, uint16_t v) { p[0] = v; p[1] = v >> 8; }

/* 16-Bit-PCM, mono, 48 kHz */
static bool write_wav(const char *path, const float *x, size_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    uint32_t data_bytes = (uint32_t)(n * 2);
    uint8_t h[44];
    memcpy(h, "RIFF", 4); put_u32(h + 4, 36 + data_bytes); memcpy(h + 8, "WAVE", 4);
    memcpy(h + 12, "fmt ", 4); put_u32(h + 16, 16); put_u16(h + 20, 1); put_u16(h + 22, 1);
    put_u32(h + 24, SR); put_u32(h + 28, SR * 2); put_u16(h + 32, 2); put_u16(h + 34, 16);
    memcpy(h + 36, "data", 4); put_u32(h + 40, data_bytes);
    bool ok = fwrite(h, 1, 44, f) == 44;
    int16_t chunk[4096];
    for (size_t i = 0; i < n && ok; i += 4096) {
        size_t m = n - i < 4096 ? n - i : 4096;
        for (size_t k = 0; k < m; k++) {
            float v = fmaxf(-1.0f, fminf(1.0f, x[i + k]));
            chunk[k] = (int16_t)lrintf(v * 32767.0f);
        }
        ok = fwrite(chunk, 2, m, f) == m;
    }
    return fclose(f) == 0 && ok;
}

static bool run_and_wait(char *const argv[])
{
    pid_t pid;
    if (posix_spawn(&pid, argv[0], NULL, NULL, argv, environ) != 0) return false;
    int st = 0;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) && WEXITSTATUS(st) == 0;
}

static void open_folder(const char *dir)
{
    char *argv[] = { "/usr/bin/open", (char *)dir, NULL };
    run_and_wait(argv);
}

/* Pfad mit ~ statt Home-Verzeichnis, nur für die Anzeige */
static const char *pretty_path(const char *path, char *buf, size_t cap)
{
    const char *home = getenv("HOME");
    if (home && strncmp(path, home, strlen(home)) == 0) snprintf(buf, cap, "~%s", path + strlen(home));
    else snprintf(buf, cap, "%s", path);
    return buf;
}

static void print_note(int color, const char *fmt, ...)
{
    va_list ap;
    fputs("\r" ESC "2K   ", stdout);
    color256(color);
    va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
    fputs(RESET "\n", stdout);
}

/* Speichert den letzten Blubber (oder alles aus dem Puffer) als WAV und, wenn afconvert da ist, als M4A. */
static void save_clip(app_t *app, bool everything, const char *dir, int voice)
{
    size_t max = (size_t)SAVE_MAX_SEC * SR, st = 0, en = 0;
    float *tmp = malloc(max * sizeof(float));
    if (!tmp) return;
    size_t n = snapshot_output(app, tmp, max);
    if (n == 0 || !find_clip(tmp, n, everything ? 0.0 : 2.0, &st, &en)) {
        print_note(208, "Nichts zu speichern – erst mal was blubbern.");
        free(tmp);
        return;
    }

    time_t t = time(NULL);
    struct tm tmv;
    localtime_r(&t, &tmv);
    char stamp[32], wav[1024], m4a[1024], shown[1024];
    strftime(stamp, sizeof stamp, "%Y-%m-%d-%H.%M.%S", &tmv);
    snprintf(wav, sizeof wav, "%s/blablablubber-%s-%s.wav", dir, stamp, VOICES[voice].name);
    snprintf(m4a, sizeof m4a, "%s/blablablubber-%s-%s.m4a", dir, stamp, VOICES[voice].name);

    if (!write_wav(wav, tmp + st, en - st)) {
        print_note(196, "Speichern fehlgeschlagen: %s", wav);
        free(tmp);
        return;
    }
    free(tmp);

    bool got_m4a = false;
    if (access("/usr/bin/afconvert", X_OK) == 0) {
        char *argv[] = { "/usr/bin/afconvert", "-f", "m4af", "-d", "aac", "-b", "128000", wav, m4a, NULL };
        got_m4a = run_and_wait(argv);
    }
    print_note(118, "💾 Gespeichert: %s (%.1f s)%s", pretty_path(wav, shown, sizeof shown),
               (double)(en - st) / SR, got_m4a ? " + .m4a für WhatsApp" : "");
}

/* ------------------------------------------------------------------ */
/* Selbsttest (ohne Audiogeräte)                                       */
/* ------------------------------------------------------------------ */

static int selftest(void)
{
    /* erwartete Grundfrequenz eines 440-Hz-Tons nach dem Effekt; 0 = nicht prüfbar */
    static const double EXPECT_HZ[V_COUNT] = {440, 440 * 1.6, 440 * 2.1, 440 * 0.58, 0, 0, 0, 440, 440, 440};
    const double D = 0.5 * SR;
    int failures = 0;

    printf("Selbsttest: 440-Hz-Ton, 6 s durch jede Stimme (Delay 0,5 s), Messung in der letzten Sekunde\n\n");
    printf("  %-11s %9s %9s %7s  %s\n", "Stimme", "gemessen", "erwartet", "Peak", "Ergebnis");
    for (int v = 0; v < V_COUNT; v++) {
        dsp_t d;
        dsp_init(&d, v, D);
        size_t total = 6 * SR, measure_from = 5 * SR;
        int crossings = 0;
        float prev = 0.0f, peak = 0.0f;
        bool finite = true;
        for (size_t i = 0; i < total; i++) {
            float in = 0.5f * sinf((float)(2.0 * M_PI * 440.0 * i / SR));
            float out = process_sample(&d, in, v, D, 1, 1);
            if (i >= measure_from) {
                if (!isfinite(out)) finite = false;
                if ((prev < 0 && out >= 0) || (prev >= 0 && out < 0)) crossings++;
                peak = fmaxf(peak, fabsf(out));
                prev = out;
            }
        }
        dsp_free(&d);
        double hz = crossings / 2.0;
        bool ok = finite && peak > 0.05f && peak <= 1.0f;
        if (EXPECT_HZ[v] > 0 && fabs(hz - EXPECT_HZ[v]) > 0.08 * EXPECT_HZ[v]) ok = false;
        if (!ok) failures++;
        printf("  %-11s %7.0f Hz ", VOICES[v].name, hz);
        if (EXPECT_HZ[v] > 0) printf("%7.0f Hz ", EXPECT_HZ[v]); else printf("%10s ", "-");
        printf("%6.2f  %s\n", peak, ok ? "OK" : "FEHLER");
    }
    printf("\n%s\n", failures ? "Selbsttest FEHLGESCHLAGEN" : "Selbsttest bestanden");
    return failures ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/* Hauptprogramm                                                       */
/* ------------------------------------------------------------------ */

static void print_help(const char *prog)
{
    printf("Blablablubber – spricht dir alles mit Verzögerung und verrückter Stimme nach.\n\n"
           "Aufruf: %s [Optionen]\n\n"
           "  -s, --stimme NAME|ZIFFER   Startstimme (Standard: mickey)\n"
           "  -d, --delay SEKUNDEN       Verzögerung im Modus immer, 0,5 bis 30 (Standard: 5)\n"
           "  -m, --modus MODUS          halten (Standard), umschalten oder immer\n"
           "                             halten: Leertaste gedrückt halten, Loslassen = Wiedergabe\n"
           "                             umschalten: Leertaste startet und beendet die Aufnahme\n"
           "                             immer: Mikrofon immer offen, Wiedergabe nach festem Delay\n"
           "  -o, --ordner PFAD          Speicherort für WAV/M4A (Standard: ~/Desktop/Blablablubber)\n"
           "  -z, --zufall               alle %.0f s eine zufällige Stimme\n"
           "  -t, --sekunden N           nach N Sekunden automatisch beenden\n"
           "  -l, --liste                alle Stimmen anzeigen\n"
           "      --selftest             DSP-Selbsttest ohne Audiogeräte\n"
           "      --testton              statt Mikrofon eine kleine Melodie einspeisen (zum Ausprobieren)\n"
           "  -h, --hilfe                diese Hilfe\n\n"
           "Tasten im Programm: Leertaste Aufnahme, 0-9 Stimme, z Zufall, +/- Delay, m Modus,\n"
           "                    s letzten Blubber speichern, S alles speichern, o Ordner öffnen, q beenden\n",
           prog, RANDOM_EVERY_SEC);
}

static int parse_voice(const char *s)
{
    if (strlen(s) == 1 && isdigit((unsigned char)s[0])) return s[0] - '0';
    for (int i = 0; i < V_COUNT; i++)
        if (strncasecmp(VOICES[i].name, s, strlen(s)) == 0) return i;
    return -1;
}

static double parse_number(const char *s)
{
    char buf[64];
    snprintf(buf, sizeof buf, "%s", s);
    for (char *p = buf; *p; p++) if (*p == ',') *p = '.';
    return strtod(buf, NULL);
}

int main(int argc, char **argv)
{
    int voice = V_MICKEY, delay_ms = DEFAULT_DELAY_MS, mode = MODE_HALTEN;
    bool random_mode = false, mode_given = false, testtone = false;
    double max_seconds = 0;
    char save_dir[1024];
    snprintf(save_dir, sizeof save_dir, "%s/Desktop/Blablablubber", getenv("HOME") ? getenv("HOME") : ".");

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        bool has_next = i + 1 < argc;
        if (!strcmp(a, "-h") || !strcmp(a, "--hilfe") || !strcmp(a, "--help")) { print_help(argv[0]); return 0; }
        else if (!strcmp(a, "-l") || !strcmp(a, "--liste") || !strcmp(a, "--list")) { print_voices(-1); return 0; }
        else if (!strcmp(a, "--selftest")) return selftest();
        else if (!strcmp(a, "--testton")) testtone = true;
        else if (!strcmp(a, "-z") || !strcmp(a, "--zufall") || !strcmp(a, "--random")) random_mode = true;
        else if ((!strcmp(a, "-m") || !strcmp(a, "--modus") || !strcmp(a, "--mode")) && has_next) {
            const char *m = argv[++i];
            mode = -1;
            for (int k = 0; k < MODE_COUNT; k++) if (strncasecmp(MODE_NAME[k], m, strlen(m)) == 0) mode = k;
            if (mode < 0) { fprintf(stderr, "Unbekannter Modus: %s (halten, umschalten, immer)\n", m); return 2; }
            mode_given = true;
        }
        else if ((!strcmp(a, "-s") || !strcmp(a, "--stimme") || !strcmp(a, "--voice")) && has_next) {
            voice = parse_voice(argv[++i]);
            if (voice < 0 || voice >= V_COUNT) { fprintf(stderr, "Unbekannte Stimme: %s (siehe --liste)\n", argv[i]); return 2; }
        }
        else if ((!strcmp(a, "-d") || !strcmp(a, "--delay") || !strcmp(a, "--verzoegerung")) && has_next) {
            double sec = parse_number(argv[++i]);
            delay_ms = (int)(sec * 1000.0 + 0.5);
            if (delay_ms < MIN_DELAY_MS || delay_ms > MAX_DELAY_MS) { fprintf(stderr, "Delay muss zwischen 0,5 und 30 Sekunden liegen.\n"); return 2; }
        }
        else if ((!strcmp(a, "-o") || !strcmp(a, "--ordner") || !strcmp(a, "--folder")) && has_next) {
            snprintf(save_dir, sizeof save_dir, "%s", argv[++i]);
        }
        else if ((!strcmp(a, "-t") || !strcmp(a, "--sekunden") || !strcmp(a, "--seconds")) && has_next) {
            max_seconds = parse_number(argv[++i]);
        }
        else { fprintf(stderr, "Unbekannte Option: %s\n\n", a); print_help(argv[0]); return 2; }
    }

    app_t *app = calloc(1, sizeof *app);
    if (!app) { perror("calloc"); return 1; }
    dsp_init(&app->dsp, voice, delay_ms * (SR / 1000.0));
    atomic_store(&app->voice, voice);
    atomic_store(&app->delay_ms, delay_ms);
    app->on = (size_t)OUTBUF_SEC * SR;
    app->outbuf = calloc(app->on, sizeof(float));
    atomic_store(&app->ow, 0);
    atomic_store(&app->testtone, testtone);
    if (!app->outbuf) { perror("calloc"); return 1; }
    mkdir(save_dir, 0755); /* existiert er schon, ist das egal; existiert der Elternordner nicht, meckert das Speichern */

    print_banner();

    ma_device_config cfg = ma_device_config_init(ma_device_type_duplex);
    cfg.sampleRate                 = SR;
    cfg.capture.format             = ma_format_f32;
    cfg.capture.channels           = 1;
    cfg.playback.format            = ma_format_f32;
    cfg.playback.channels          = 1;
    cfg.periodSizeInMilliseconds   = 10;
    cfg.dataCallback               = data_callback;
    cfg.pUserData                  = app;

    ma_device dev;
    ma_result r = ma_device_init(NULL, &cfg, &dev);
    if (r != MA_SUCCESS) {
        fprintf(stderr, "Audiogerät konnte nicht geöffnet werden: %s\n"
                        "Tipp: Hat das Terminal Mikrofon-Zugriff? (Systemeinstellungen → Datenschutz → Mikrofon)\n",
                ma_result_description(r));
        return 1;
    }
    r = ma_device_start(&dev);
    if (r != MA_SUCCESS) {
        fprintf(stderr, "Audiogerät konnte nicht gestartet werden: %s\n", ma_result_description(r));
        ma_device_uninit(&dev);
        return 1;
    }

    color256(245); printf("   🎤 %s   →   🔊 %s" RESET "\n\n", dev.capture.name, dev.playback.name);
    print_voices(voice);
    putchar('\n');
    color256(245); printf("   Tasten: " RESET); color256(226); printf("Leertaste" RESET); color256(245); printf(" Aufnahme · " RESET);
    color256(226); printf("0-9" RESET); color256(245); printf(" Stimme · " RESET);
    color256(226); printf("z" RESET); color256(245); printf(" Zufall · " RESET); color256(226); printf("+/-" RESET);
    color256(245); printf(" Delay · " RESET); color256(226); printf("m" RESET); color256(245); printf(" Modus · " RESET);
    color256(226); printf("q" RESET); color256(245); printf(" Ende" RESET "\n");
    color256(245); printf("           " RESET); color256(226); printf("s" RESET); color256(245); printf(" letzten Blubber speichern · " RESET);
    color256(226); printf("S" RESET); color256(245); printf(" alles speichern (60 s) · " RESET);
    color256(226); printf("o" RESET); color256(245); printf(" Ordner öffnen" RESET "\n");
    {
        char shown[1024];
        color256(245); printf("   💾 Speicherort: %s (WAV + M4A)" RESET "\n", pretty_path(save_dir, shown, sizeof shown));
    }

    hold_t hold;
    hold_init(&hold);
    if (!mode_given && hold.repeat_iv >= 1.3) {
        /* Tastenwiederholung ist am Mac abgeschaltet, Halten lässt sich nicht erkennen */
        mode = MODE_UMSCHALTEN;
    }
    print_mode(mode);
    color256(213);
    if (mode == MODE_HALTEN)          printf("   Leertaste halten, sprechen, loslassen – und Blablablubber plappert sofort nach." RESET "\n\n");
    else if (mode == MODE_UMSCHALTEN) printf("   Leertaste, sprechen, wieder Leertaste – und Blablablubber plappert sofort nach." RESET "\n\n");
    else                              printf("   Sprich einfach los – in %.1f s plappert Blablablubber nach." RESET "\n\n", delay_ms / 1000.0);

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    tty_raw();
    srand((unsigned)time(NULL));

    double t0 = now_seconds(), next_random = RANDOM_EVERY_SEC;
    float in_disp = 0.0f, out_disp = 0.0f;
    bool stdin_ok = isatty(STDIN_FILENO);
    bool recording = (mode == MODE_IMMER);
    atomic_store(&app->recording, recording);
    atomic_store(&app->walkie, mode != MODE_IMMER);
    int save_pending = 0;              /* 0 nichts, 1 letzter Blubber, 2 alles */
    double save_requested = 0, last_loud_out = -1e9;
    double rec_started = 0, play_until = -1e9;
    bool was_recording = recording;

    while (!g_stop) {
        unsigned char keys[64];
        ssize_t nk = 0;
        if (stdin_ok) {
            struct pollfd p = { .fd = STDIN_FILENO, .events = POLLIN, .revents = 0 };
            if (poll(&p, 1, 50) > 0) {
                nk = read(STDIN_FILENO, keys, sizeof keys);   /* alles abholen, was anliegt */
                if (nk <= 0) { nk = 0; stdin_ok = false; }
            }
        } else {
            usleep(50000);
        }
        double now = now_seconds() - t0;
        bool quit = false;

        for (ssize_t k = 0; k < nk; k++) {
            int c = keys[k];
            if (c == 'q' || c == 'Q' || c == 3) quit = true;
            else if (c == ' ') {
                if (mode == MODE_HALTEN)          { hold_press(&hold, now, recording); recording = true; }
                else if (mode == MODE_UMSCHALTEN) recording = !recording;
            }
            else if (c == 's') { save_pending = 1; save_requested = now; }
            else if (c == 'S') { save_pending = 2; save_requested = now; }
            else if (c == 'o' || c == 'O') open_folder(save_dir);
            else if (c == 'm' || c == 'M') {
                mode = (mode + 1) % MODE_COUNT;
                recording = (mode == MODE_IMMER);
                hold.presses = 0;
                atomic_store(&app->walkie, mode != MODE_IMMER);
                play_until = -1e9;
                print_mode(mode);
            }
            else if (c >= '0' && c <= '9') { voice = c - '0'; atomic_store(&app->voice, voice); random_mode = false; }
            else if (c == 'z' || c == 'Z') { random_mode = !random_mode; next_random = now; }
            else if (c == '+' || c == '=') { delay_ms = delay_ms + 1000 > MAX_DELAY_MS ? MAX_DELAY_MS : delay_ms + 1000; atomic_store(&app->delay_ms, delay_ms); }
            else if (c == '-' || c == '_') { delay_ms = delay_ms - 1000 < MIN_DELAY_MS ? MIN_DELAY_MS : delay_ms - 1000; atomic_store(&app->delay_ms, delay_ms); }
        }
        if (quit) break;

        if (mode == MODE_HALTEN && recording && now - hold.last_press > hold.timeout) recording = false;
        atomic_store(&app->recording, recording);
        if (recording && !was_recording) rec_started = now;
        if (!recording && was_recording && mode != MODE_IMMER) {
            double len = now - rec_started;
            if (len > WALKIE_MAX_SEC) len = WALKIE_MAX_SEC;
            play_until = now + len;      /* der Audio-Thread setzt den exakten Wert, das hier ist für die Anzeige */
        }
        was_recording = recording;

        if (random_mode && now >= next_random) {
            int nv;
            do nv = 1 + rand() % (V_COUNT - 1); while (nv == voice);
            voice = nv;
            atomic_store(&app->voice, voice);
            next_random = now + RANDOM_EVERY_SEC;
        }
        if (max_seconds > 0 && now >= max_seconds) break;

        float in_peak = atomic_exchange(&app->in_peak, 0) / 1000.0f;
        float out_peak = atomic_exchange(&app->out_peak, 0) / 1000.0f;
        if (out_peak > SILENCE_PEAK) last_loud_out = now;
        in_disp  = fmaxf(in_disp  * 0.8f, to_display(in_peak));
        out_disp = fmaxf(out_disp * 0.8f, to_display(out_peak));

        /* Speichern erst, wenn der Blubber fertig geplappert hat (oder spätestens nach einer Frist) */
        if (save_pending) {
            bool input_done = (mode == MODE_IMMER) || (!recording && now > play_until + 0.3);
            bool output_quiet = now - last_loud_out > 0.8;
            bool deadline = now > fmax(play_until, save_requested) + (mode == MODE_IMMER ? 3.0 : 6.0);

            if ((input_done && output_quiet) || deadline) {
                save_clip(app, save_pending == 2, save_dir, voice);
                save_pending = 0;
            }
        }
        status_t st = {
            .recording = recording, .playing = (mode != MODE_IMMER && now < play_until), .walkie = (mode != MODE_IMMER),
            .saving = save_pending != 0, .random_mode = random_mode,
            .rec_seconds = recording ? now - rec_started : 0, .play_seconds = play_until > now ? play_until - now : 0,
            .voice = voice, .delay_ms = delay_ms, .clip_ms = atomic_load(&app->clip_ms),
            .elapsed = now, .in_lvl = in_disp, .out_lvl = out_disp,
        };
        draw_status(&st);
    }

    ma_device_uninit(&dev);
    tty_restore();
    printf("\r" ESC "2K   ");
    rainbow("Blubb. Tschüss! ✦", 5);
    printf("\n\n");
    dsp_free(&app->dsp);
    free(app->outbuf);
    free(app);
    return 0;
}
