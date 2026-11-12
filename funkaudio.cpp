#include "pxt.h"

#if MICROBIT_CODAL
#include "StreamSplitter.h"
#include "DataStream.h"
#endif

#define AUDIO_RATE  4000      // Abtastrate in Hz
#define MAX_SAMPLES 16000     // = 4 Sekunden

namespace funkaudio {

#if MICROBIT_CODAL
static uint8_t samples[MAX_SAMPLES];   // 8 Bit unsigned, 128 = Stille
static volatile int length = 0;
static volatile bool recording = false;
static int gain = 4;

// ---------- Aufnahme: holt Daten vom Mikrofon ----------
class Recorder : public DataSink {
  public:
    DataSource &src;
    Recorder(DataSource &s) : src(s) { src.connect(*this); }

    virtual int pullRequest() {
        ManagedBuffer b = src.pull();
        if (!recording) return DEVICE_OK;

        int fmt = src.getFormat();
        int bps = DATASTREAM_FORMAT_BYTES_PER_SAMPLE(fmt);
        if (bps <= 0) return DEVICE_OK;
        uint8_t *p = b.getBytes();
        int n = b.length() / bps;

        for (int i = 0; i < n && length < MAX_SAMPLES; i++) {
            int v16;  // auf 16-Bit-Skala normalisieren
            if (fmt == DATASTREAM_FORMAT_16BIT_SIGNED) {
                int16_t s; memcpy(&s, p + i * 2, 2); v16 = s;
            } else if (fmt == DATASTREAM_FORMAT_8BIT_SIGNED) {
                v16 = ((int8_t)p[i]) * 256;
            } else if (fmt == DATASTREAM_FORMAT_8BIT_UNSIGNED) {
                v16 = ((int)p[i] - 128) * 256;
            } else {
                return DEVICE_OK;
            }
            int v = ((v16 * gain) >> 8) + 128;
            if (v < 0) v = 0;
            if (v > 255) v = 255;
            samples[length++] = (uint8_t)v;
        }
        if (length >= MAX_SAMPLES) recording = false;
        return DEVICE_OK;
    }
};

// ---------- Wiedergabe: liefert Daten an den Mixer ----------
class Player : public DataSource {
  public:
    DataSink *sink = NULL;
    int pos = 0;
    bool active = false;

    virtual void connect(DataSink &s) { sink = &s; }
    virtual void disconnect() { sink = NULL; }
    virtual int getFormat() { return DATASTREAM_FORMAT_8BIT_UNSIGNED; }

    virtual ManagedBuffer pull() {
        int n = 128;
        if (!active || pos >= length) {
            active = false;
            ManagedBuffer silence(n);
            memset(silence.getBytes(), 128, n);
            return silence;
        }
        if (length - pos < n) n = length - pos;
        ManagedBuffer b(n);
        memcpy(b.getBytes(), samples + pos, n);
        pos += n;
        return b;
    }

    void start() { pos = 0; active = true; if (sink) sink->pullRequest(); }
};

static Recorder *recorder = NULL;
static Player *player = NULL;
#endif

//%
void startRecording() {
#if MICROBIT_CODAL
    if (!recorder) {
        SplitterChannel *ch = uBit.audio.splitter->createChannel();
        ch->requestSampleRate(AUDIO_RATE);
        recorder = new Recorder(*ch);
    }
    uBit.audio.activateMic();
    length = 0;
    recording = true;
#endif
}

//%
void stopRecording() {
#if MICROBIT_CODAL
    recording = false;
#endif
}

//%
int recordedLength() {
#if MICROBIT_CODAL
    return length;
#else
    return 0;
#endif
}

//%
int maxLength() { return MAX_SAMPLES; }

//%
void setGain(int g) {
#if MICROBIT_CODAL
    if (g < 1) g = 1;
    if (g > 64) g = 64;
    gain = g;
#endif
}

//% 
Buffer getChunk(int offset, int len) {
#if MICROBIT_CODAL
    if (offset < 0) offset = 0;
    if (offset + len > length) len = length - offset;
    if (len < 0) len = 0;
    return mkBuffer(samples + offset, len);
#else
    return mkBuffer(NULL, 0);
#endif
}

//%
void setChunk(int offset, Buffer buf) {
#if MICROBIT_CODAL
    for (int i = 0; i < (int)buf->length; i++) {
        int o = offset + i;
        if (o >= 0 && o < MAX_SAMPLES) samples[o] = buf->data[i];
    }
#endif
}

//%
void prepareReceive(int len) {
#if MICROBIT_CODAL
    recording = false;
    if (len > MAX_SAMPLES) len = MAX_SAMPLES;
    if (len < 0) len = 0;
    memset(samples, 128, MAX_SAMPLES);   // Fehlende Pakete = Stille
    length = len;
#endif
}

//%
void playRecording() {
#if MICROBIT_CODAL
    if (!player) {
        player = new Player();
        MixerChannel *ch = uBit.audio.mixer.addChannel(*player, AUDIO_RATE);
        ch->setVolume(1023);
    }
    player->start();
#endif
}

//%
bool isPlaying() {
#if MICROBIT_CODAL
    return player && player->active;
#else
    return false;
#endif
}

} // namespace funkaudio