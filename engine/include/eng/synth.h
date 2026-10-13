// Offline synthesis for film scores: a few instruments rendered note by note into a stereo buffer
// through a shared reverb, plus chord spelling, ducking under a voice, and a limiter.
#pragma once
#include <string>
#include <vector>
#include "video.h"

namespace eng::synth {

// MIDI note number to frequency (A4 = 69 = 440 Hz).
float hz(float midi);

enum class Instrument {
  Piano,    // felt piano: additive partials with a soft hammer, two detuned strings per partial
  Pluck,    // plucked string (Karplus-Strong)
  Pad,      // warm string pad: detuned saws through a low-pass filter, slow attack and release
  Bell,     // music box / glockenspiel: inharmonic bar partials
  Bass,     // round plucked bass
  Shaker,   // band-passed noise hit
  Tick,     // wood-block click (a clock)
  Thud,     // low impact (something heavy landing)
  Shutter,  // camera shutter: two short clicks
  Swell,    // rising breath of noise that ends at t + dur
};

struct Note {
  double t = 0;      // start, in seconds
  float dur = 0.5f;  // held for this long; the sound then rings on as the instrument does
  float pitch = 60;  // MIDI note number (ignored by the noise instruments)
  float vel = 0.7f;  // 0..1
  float pan = 0;     // -1 left .. 1 right
};

struct Track {
  Instrument instrument = Instrument::Piano;
  std::vector<Note> notes;
  float gain = 1;
  float reverb = 0.25f;     // send to the shared reverb
  float brightness = 0.5f;  // 0 dark .. 1 bright
};

struct ReverbSettings {
  float room = 0.82f;      // 0..1, longer tail when larger
  float damping = 0.45f;   // 0..1, darker tail when larger
  float predelay = 0.02f;  // seconds
  float width = 1;         // 0 mono .. 1 wide
};

// Render tracks into a stereo buffer `seconds` long.
Audio render(const std::vector<Track>& tracks, double seconds, int rate = 48000, const ReverbSettings& reverb = {});

// A chord by name: "D", "Bm7", "A/C#", "Gmaj7", "Esus4", "F#m", "Cadd9", "Bdim", "E7", "Am6", "Dmaj9".
struct Chord {
  int root = 0, bass = 0;   // pitch classes (C = 0)
  std::vector<int> tones;   // pitch classes, root first
};
Chord chord(const std::string& name);
// `count` chord tones stacked upward from the lowest one at or above `low` (MIDI).
std::vector<float> voicing(const Chord& c, float low, int count);
// The bass note at or above `low`.
float bassNote(const Chord& c, float low);

// A gain curve, one value per frame of `rate`, that dips to `depth` while `voice` is sounding.
std::vector<float> duckingCurve(const Audio& voice, int rate, size_t frames, float depth, float attack = 0.12f, float release = 0.7f);
// Multiply a stereo or mono buffer by a per-frame gain curve.
void applyGain(Audio& a, const std::vector<float>& gain);
// Keep peaks under `ceiling` with a short look-ahead limiter.
void limit(Audio& a, float ceiling = 0.95f);

}  // namespace eng::synth
