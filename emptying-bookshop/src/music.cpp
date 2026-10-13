// The score. Everything is placed from cue times, so the music follows the narration when the
// script is edited. One theme in D major runs through the film:
//   1 · the theme over the campus, turning minor as the visitors thin out
//   2 · a quick plucked figure for the quick reactions; it freezes with the snapshot
//   3 · a walking groove for the investigation, a chime for each finding, a clock that ticks to five
//   4 · no beat: the two sides of the contradiction as two chords a tritone apart, one in each ear,
//       resolving to a warm chord when the cables turn gold
//   5 · a music-box note for every bead in the jar; when the beads link up, the theme returns in full
//   6 · the theme at stage 1, in the dominant at stage 2, and an octave higher at stage 3
//   7 · a note for each pin, the theme once more, and a long final chord
#include "music.h"

#include <cmath>
#include <map>
#include <sstream>

#include "kit.h"
#include "script.h"

namespace film {

using namespace eng::synth;

namespace {

constexpr int MAJOR[7] = {0, 2, 4, 5, 7, 9, 11};

// Time relative to a cue's start.
double T(const char* id, float dt = 0) { return c(id).s + dt; }
constexpr int D4 = 62;

struct Score {
  Track piano{Instrument::Piano, {}, 1.0f, 0.34f, 0.42f};
  Track pad{Instrument::Pad, {}, 0.9f, 0.5f, 0.35f};
  Track pluck{Instrument::Pluck, {}, 0.55f, 0.28f, 0.5f};
  Track bass{Instrument::Bass, {}, 0.75f, 0.12f, 0.3f};
  Track bell{Instrument::Bell, {}, 0.8f, 0.45f, 0.45f};
  Track shaker{Instrument::Shaker, {}, 0.35f, 0.15f, 0.5f};
  Track tick{Instrument::Tick, {}, 0.5f, 0.2f, 0.5f};
  Track thud{Instrument::Thud, {}, 0.8f, 0.25f, 0.5f};
  Track shutter{Instrument::Shutter, {}, 0.7f, 0.2f, 0.5f};
  Track swell{Instrument::Swell, {}, 0.5f, 0.4f, 0.5f};
  Rng rng{2024};

  // Small timing and velocity differences, so repeated figures don't sound mechanical.
  Note human(double t, float dur, float pitch, float vel, float pan = 0) {
    float dt = (float)(rng() - 0.5) * 0.016f;
    float dv = (float)(rng() - 0.5) * 0.08f;
    return {t + dt, dur, pitch, clamp(vel + dv, 0.05f, 1.f), pan};
  }
};

// A tempo grid from t0.
struct Grid {
  double t0;
  float bpm;
  double beat(double b) const { return t0 + b * 60.0 / bpm; }
  double secs(double beats) const { return beats * 60.0 / bpm; }
  // The beat number at time t.
  double at(double t) const { return (t - t0) * bpm / 60.0; }
};

// Scale degree (1 = tonic; 8 = tonic an octave up; 0 and below go down) with an optional '#' or 'b'.
float degree(const std::string& tok, int root) {
  int d = std::stoi(tok);
  int acc = tok.find('#') != std::string::npos ? 1 : tok.find('b', 1) != std::string::npos ? -1 : 0;
  int i = d - 1;
  int oct = (int)std::floor(i / 7.0);
  int k = i - oct * 7;
  return (float)(root + oct * 12 + MAJOR[k] + acc);
}

// A melody in "degree:beats" tokens ("r" is a rest, bars marked with "|" are ignored).
void phrase(Score& S, Track& tr, const Grid& g, double beat, const std::string& notes, int root, float vel, float pan = 0, float legato = 0.92f) {
  std::istringstream in(notes);
  std::string tok;
  while (in >> tok) {
    if (tok == "|") continue;
    size_t colon = tok.find(':');
    std::string d = tok.substr(0, colon);
    double len = std::stod(tok.substr(colon + 1));
    if (d != "r") tr.notes.push_back(S.human(g.beat(beat), (float)g.secs(len * legato), degree(d, root), vel, pan));
    beat += len;
  }
}

void padChord(Score& S, double t, double dur, const std::string& name, float low = 54, int count = 4, float vel = 0.55f, float pan = 0) {
  for (float p : voicing(chord(name), low, count)) S.pad.notes.push_back({t, (float)dur, p, vel, pan});
}

// Broken chord: `pattern` indexes the voicing (from `low`), one note every `step` beats.
void arpeggio(Score& S, Track& tr, const Grid& g, double beat, double beats, const std::string& name, float low, const std::vector<int>& pattern,
              double step, float vel, float pan = 0, float hold = 1.6f) {
  int top = 0;
  for (int i : pattern) top = std::max(top, i);
  std::vector<float> v = voicing(chord(name), low, top + 1);
  int k = 0;
  for (double b = 0; b < beats - 1e-6; b += step, k++) {
    float accent = (std::fmod(b, 2.0) < 1e-6) ? 0.06f : 0.f;
    tr.notes.push_back(S.human(g.beat(beat + b), (float)g.secs(step * hold), v[pattern[k % pattern.size()]], vel + accent, pan));
  }
}

void bassNoteAt(Score& S, const Grid& g, double beat, double beats, const std::string& name, float vel = 0.7f, float low = 38) {
  S.bass.notes.push_back(S.human(g.beat(beat), (float)g.secs(beats * 0.9), bassNote(chord(name), low), vel));
}

// Bars of a progression from `fromBeat` while the bar starts before `until`; returns the next beat.
template <class F>
double bars(const Grid& g, double fromBeat, double until, const std::vector<std::string>& prog, F&& each) {
  double b = fromBeat;
  for (int i = 0; g.beat(b) < until - 0.3; i++, b += 4) each(b, prog[i % prog.size()], i);
  return b;
}

// The theme: two four-bar phrases over D | A/C# | Bm | G | D/F# | G | Em7 | Asus4 A.
const std::string THEME_A = "3:1 5:1 8:2 | 7:1 6:1 5:2 | 6:1 5:1 3:1 1:1 | 2:3 r:1";
const std::string THEME_B = "3:1 5:1 8:1 9:1 | 8:1 6:1 4:2 | 4:1 3:1 2:1 1:1 | 2:4";
const std::vector<std::string> THEME_CHORDS = {"D", "A/C#", "Bm", "G", "D/F#", "G", "Em7", "Asus4"};

// A full statement of the theme from `beat`: melody, broken chords, pad and bass.
double theme(Score& S, const Grid& g, double beat, int root, float vel, bool full) {
  int shift = root - D4;
  auto tr = [&](const std::string& c) {
    // Transpose a chord name by `shift` semitones.
    static const char* names[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
    auto pc = [](const std::string& s, size_t& i) {
      static const std::map<char, int> base = {{'C', 0}, {'D', 2}, {'E', 4}, {'F', 5}, {'G', 7}, {'A', 9}, {'B', 11}};
      int p = base.at(s[i++]);
      if (i < s.size() && s[i] == '#') { p++; i++; }
      else if (i < s.size() && s[i] == 'b') { p--; i++; }
      return (p + 12) % 12;
    };
    size_t i = 0;
    int r = pc(c, i);
    size_t slash = c.find('/');
    std::string q = c.substr(i, slash == std::string::npos ? std::string::npos : slash - i);
    std::string out = std::string(names[(r + shift + 12) % 12]) + q;
    if (slash != std::string::npos) {
      size_t j = slash + 1;
      out += "/" + std::string(names[(pc(c, j) + shift + 12) % 12]);
    }
    return out;
  };
  phrase(S, S.piano, g, beat, THEME_A + " | " + THEME_B, root, vel);
  for (int i = 0; i < 8; i++) {
    std::string c = tr(THEME_CHORDS[i]);
    double b = beat + i * 4;
    arpeggio(S, S.piano, g, b, i == 7 ? 2 : 4, c, 45 + shift, {0, 2, 3, 2}, 0.5, vel * 0.55f, -0.25f);
    if (i == 7) arpeggio(S, S.piano, g, b + 2, 2, tr("A"), 45 + shift, {0, 2, 3, 2}, 0.5, vel * 0.55f, -0.25f);
    if (full) {
      padChord(S, g.beat(b), g.secs(4), c, 57, 4, 0.45f);
      bassNoteAt(S, g, b, 2, c, 0.62f);
      bassNoteAt(S, g, b + 2, 2, c, 0.5f);
    }
  }
  return beat + 32;
}

// ---------------- chapters ----------------

void chapter1(Score& S, const Chapter& ch) {
  // The title: a pad chord fading in and three notes of the theme on the music box.
  padChord(S, 0.3, c("1a").s - 0.3, "Dadd9", 50, 5, 0.42f);
  for (auto [dt, p] : std::vector<std::pair<double, float>>{{1.2, 78}, {2.0, 81}, {2.8, 86}}) S.bell.notes.push_back({dt, 0.5f, p, 0.4f, 0.2f});
  Grid g{c("1a").s - 0.25, 72};
  // The theme while Jia and the shop's history are introduced.
  theme(S, g, 0, D4, 0.5f, true);
  // As fewer students come, the harmony turns minor and thins out.
  Grid g2{c("1c").s + 0.4, 72};
  const std::vector<std::string> minor = {"Bm", "G", "D", "A", "Bm", "G", "Em7", "F#m"};
  bars(g2, 0, c("1d").s, minor, [&](double bb, const std::string& cn, int i) {
    if (g.at(g2.beat(bb)) < 32) return;  // the theme is still playing
    arpeggio(S, S.piano, g2, bb, 4, cn, 45, {0, 2, 1, 2}, 1, 0.34f, -0.2f);
    padChord(S, g2.beat(bb), g2.secs(4), cn, 57, 3, 0.4f);
    if (i % 2 == 0) bassNoteAt(S, g2, bb, 4, cn, 0.5f);
  });
  // A question: suspended chords, no melody.
  Grid g3{c("1d").s, 72};
  bars(g3, 0, ch.e + 0.4, {"Gmaj7", "Asus4", "Bm7", "Gmaj7", "Asus4", "A"}, [&](double bb, const std::string& cn, int i) {
    padChord(S, g3.beat(bb), g3.secs(4), cn, 55, 4, 0.42f);
    arpeggio(S, S.piano, g3, bb, 4, cn, 50, {0, 1, 2, 3, 2, 1}, 0.667, 0.3f, 0.15f);
    if (i % 2 == 0) bassNoteAt(S, g3, bb, 4, cn, 0.45f);
  });
  // The chapter closes on D with the music box.
  S.bell.notes.push_back({ch.e - 0.2, 0.5f, 74, 0.45f, 0.1f});
  S.bell.notes.push_back({ch.e + 0.2, 0.5f, 78, 0.35f, 0.25f});
}

void chapter2(Score& S, const Chapter& ch) {
  // Quick, light, a little too busy: a plucked figure in B minor.
  Grid g{ch.s + 0.2, 100};
  const std::vector<std::string> prog = {"Bm", "G", "Em", "F#"};
  auto figure = [&](double from, double until, float vel) {
    double b0 = std::ceil(g.at(from) / 4) * 4;
    bars(g, b0, until, prog, [&](double bb, const std::string& cn, int) {
      arpeggio(S, S.pluck, g, bb, 4, cn, 59, {0, 2, 1, 2, 3, 2, 1, 2}, 0.5, vel, 0.25f, 0.6f);
      bassNoteAt(S, g, bb, 1, cn, 0.55f);
      bassNoteAt(S, g, bb + 2, 1, cn, 0.45f);
    });
  };
  figure(ch.s + 0.2, c("2b").s + 0.6, 0.5f);
  // Reaction 1: the verdict lands with a thud; then everything outside the shop goes quiet.
  S.thud.notes.push_back({c("2b").s + 0.72, 0.4f, 0, 0.8f, 0});
  padChord(S, c("2b").s + 0.75, c("2d").s - c("2b").s - 0.6, "Bm", 47, 3, 0.45f);
  S.pad.notes.push_back({c("2b").s + 0.75, (float)(c("2d").s - c("2b").s - 0.6), 48, 0.3f, 0});  // a rub against B
  for (double t = c("2c").s + 0.5; t < c("2d").s - 0.5; t += 2.4) S.piano.notes.push_back(S.human(t, 1.2f, 71, 0.3f, 0.1f));
  // Reaction 2: another campus's cheerful tune, in a key that doesn't fit, on the music box.
  Grid gb{c("2d").s + 0.6, 112};
  for (int rep = 0; rep < 3; rep++)
    phrase(S, S.bell, gb, rep * 8, "1:0.5 3:0.5 5:0.5 8:1 5:0.5 6:1 5:2 r:2", 65, 0.42f, 0.4f);  // F major against B minor
  S.swell.notes.push_back({c("2d").s + 4.2, 1.8f, 0, 0.6f, 0});
  figure(c("2d").s + 6.0, c("2e").s + 3.0, 0.45f);
  // The half-price week changes nothing: the figure runs down.
  phrase(S, S.pluck, Grid{c("2e").s + 3.0, 80}, 0, "6:1 5:1 3:1 2:1 | 1:4", 59, 0.4f);
  // Lenin's line: warm, still chords.
  {
    double t = c("2f").s, end = c("2g").s - 0.3;
    double step = (end - t) / 3;
    const char* cs[3] = {"D", "A/C#", "Bm"};
    for (int i = 0; i < 3; i++) {
      padChord(S, t + i * step, step + 0.3, cs[i], 54, 4, 0.5f);
      S.bass.notes.push_back({t + i * step, (float)step, bassNote(chord(cs[i]), 38), 0.5f, 0});
    }
    S.piano.notes.push_back({t + 0.2, 2.0f, 74, 0.35f, 0.2f});
    S.piano.notes.push_back({t + step + 0.2, 2.0f, 73, 0.32f, 0.2f});
    S.piano.notes.push_back({t + 2 * step + 0.2, 2.0f, 71, 0.32f, 0.2f});
  }
  // Reaction 3: giving up, a falling line.
  phrase(S, S.piano, Grid{c("2g").s + 0.3, 70}, 0, "5:1 4:1 3:1 2:1 | 1:2 r:2 | 6:1 5:1 3:2", D4, 0.32f);
  padChord(S, c("2g").s + 0.2, c("2h").s - c("2g").s - 0.3, "Em7", 52, 4, 0.38f);
  // The snapshot: a shutter, and the music stops in place — one high chord held while the crowd is frozen.
  S.shutter.notes.push_back({c("2h").s + 0.12, 0.1f, 0, 0.9f, 0});
  padChord(S, c("2h").s + 0.15, c("2j").s - c("2h").s - 0.2, "Bm7", 71, 4, 0.36f);
  S.bell.notes.push_back({c("2h").s + 0.15, 0.4f, 83, 0.4f, 0});
  // Metaphysical thinking named: a low, plain drone.
  S.pad.notes.push_back({c("2i").s + 0.2, (float)(c("2j").s - c("2i").s), 35, 0.4f, 0});
  S.pad.notes.push_back({c("2i").s + 0.2, (float)(c("2j").s - c("2i").s), 47, 0.3f, 0});
  // Dialectics: going out to look. The music opens up into D major and leads into the next chapter.
  Grid gj{c("2j").s + 0.3, 72};
  arpeggio(S, S.piano, gj, 0, 4, "Gmaj7", 50, {0, 1, 2, 3, 4, 5, 6, 7}, 0.5, 0.36f);
  arpeggio(S, S.piano, gj, 4, 4, "Asus4", 52, {0, 1, 2, 3, 4, 5, 6, 7}, 0.5, 0.4f);
  padChord(S, gj.beat(0), gj.secs(4), "Gmaj7", 55, 4, 0.45f);
  padChord(S, gj.beat(4), std::max(gj.secs(4), ch.e - gj.beat(4) + 0.6), "Asus4", 57, 4, 0.48f);
  S.swell.notes.push_back({ch.e - 1.6, 1.6f, 0, 0.5f, 0});
}

void chapter3(Score& S, const Chapter& ch) {
  // Mao's line over a single chord; then a walking groove as Jia goes out to ask.
  padChord(S, ch.s, c("3b").s - ch.s, "D", 54, 4, 0.5f);
  S.bell.notes.push_back({c("3a").s + 0.6, 0.5f, 74, 0.45f, 0});
  S.piano.notes.push_back({c("3a").s + 0.6, 2.4f, 62, 0.4f, 0});
  Grid g{c("3b").s, 88};
  const std::vector<std::string> prog = {"G", "D/F#", "Em", "C", "G/B", "C", "Am7", "D"};
  double quiet0 = c("3g").s, quiet1 = c("3h").s;
  bars(g, 0, ch.e - 0.2, prog, [&](double bb, const std::string& cn, int i) {
    double t = g.beat(bb);
    float soft = (t > quiet0 && t < quiet1) ? 0.7f : 1.f;
    arpeggio(S, S.pluck, g, bb, 4, cn, 55, {0, 2, 1, 2, 0, 2, 3, 2}, 0.5, 0.42f * soft, 0.3f, 0.8f);
    // A walking bass: root, fifth, a passing step, and the next root's neighbour.
    Chord chd = chord(cn);
    float r = bassNote(chd, 40);
    float steps[4] = {r, r + 7, r + 5, r + 2};
    for (int k = 0; k < 4; k++) S.bass.notes.push_back(S.human(g.beat(bb + k), (float)g.secs(0.8), steps[k] > 55 ? steps[k] - 12 : steps[k], (k == 0 ? 0.62f : 0.5f) * soft));
    if (i >= 2) for (int k = 0; k < 4; k++) S.shaker.notes.push_back(S.human(g.beat(bb + k + 0.5), 0.1f, 0, 0.35f * soft, -0.3f));
    if (i % 2 == 1) padChord(S, t, g.secs(8), cn, 57, 3, 0.3f * soft);
  });
  // A chime as each finding's thread appears, a little higher each time.
  const std::vector<std::tuple<const char*, float, float>> finds = {{"3c", 3.2f, 79}, {"3d", 4.2f, 81}, {"3e", 3.6f, 83}, {"3f", 3.8f, 86}};
  for (auto [cue, at, p] : finds) {
    S.bell.notes.push_back({T(cue, at), 0.5f, p, 0.5f, 0.2f});
    S.bell.notes.push_back({T(cue, at) + 0.18, 0.5f, p + 7, 0.3f, 0.3f});
  }
  // The clock sweeps from 4:40 to 5:00 with ticks, then strikes five.
  for (double t = T("3d", 0.8f); t < T("3d", 3.2f); t += 0.2) S.tick.notes.push_back({t, 0.05f, 0, 0.55f, -0.4f});
  for (int k = 0; k < 2; k++) S.bell.notes.push_back({T("3d", 3.25f) + k * 0.55, 0.8f, k ? 67.f : 74.f, 0.55f, -0.3f});
  // Universal connection: a soft swell under the threads.
  S.swell.notes.push_back({c("3h").s - 1.2, 1.4f, 0, 0.35f, 0});
  padChord(S, c("3h").s + 0.2, c("3i").s - c("3h").s, "Cmaj7", 55, 4, 0.4f);
}

void chapter4(Score& S, const Chapter& ch) {
  // No beat. A low D under everything.
  S.pad.notes.push_back({ch.s + 0.2, (float)(c("4h").s - ch.s + 0.6), 38, 0.42f, 0});
  // The two sides, a tritone apart, one in each ear.
  double sideEnd = c("4h").s + 0.6;
  padChord(S, T("4b", 0.3f), sideEnd - T("4b", 0.3f), "Bb", 55, 3, 0.42f, -0.85f);
  padChord(S, T("4c", 0.3f), sideEnd - T("4c", 0.3f), "E", 56, 3, 0.42f, 0.85f);
  S.thud.notes.push_back({T("4b", 0.6f), 0.3f, 0, 0.35f, -0.6f});
  S.thud.notes.push_back({T("4c", 0.6f), 0.3f, 0, 0.35f, 0.6f});
  // Each depends on the other: a line that crosses from one side to the other as the arch is drawn.
  {
    const float line[7] = {58, 62, 65, 70, 68, 64, 59};
    for (int i = 0; i < 7; i++) S.bell.notes.push_back({T("4d", 0.4f) + i * 0.24, 0.4f, line[i] + 12, 0.38f, -0.8f + i * 0.27f});
  }
  // They pull apart: the two sides tremble.
  for (double t = T("4e", 0.2f); t < c("4h").s; t += 0.3) {
    S.pluck.notes.push_back(S.human(t, 0.15f, 46, 0.32f, -0.7f));
    S.pluck.notes.push_back(S.human(t + 0.15, 0.15f, 52, 0.32f, 0.7f));
  }
  // The smaller problems: a small coral ping as each is named.
  for (int i = 0; i < 3; i++) S.bell.notes.push_back({T("4f", 2.4f + i * 1.1f), 0.3f, 88.f - i * 2, 0.3f, 0.5f - i * 0.4f});
  // People come, look in, and leave.
  phrase(S, S.piano, Grid{T("4g", 1.2f), 76}, 0, "5:1 6:1 8:2 | 8:1 6:1 5:2", D4 - 12, 0.3f, 0.3f);
  // The principal contradiction, named: the cables turn gold and the harmony resolves.
  double gold = T("4h", 0.6f);
  S.swell.notes.push_back({gold - 1.4, 1.4f, 0, 0.45f, 0});
  padChord(S, gold, c("4i").s - gold + 0.8, "Dmaj9", 50, 5, 0.55f);
  S.bass.notes.push_back({gold, 3.f, 38, 0.6f, 0});
  arpeggio(S, S.piano, Grid{gold, 72}, 0, 8, "Dmaj9", 62, {0, 1, 2, 3, 4, 3, 2, 1}, 0.5, 0.36f, 0.1f);
  S.bell.notes.push_back({gold + 0.05, 0.6f, 86, 0.5f, 0});
  // The outside condition: a ring of arrows travels in; each wave lands with a soft note.
  padChord(S, T("4i", 1.0f), ch.e - T("4i", 1.0f) + 0.5, "Bm7", 47, 4, 0.4f);
  S.swell.notes.push_back({T("4i", 0.0f), 1.2f, 0, 0.3f, 0});
  const float lands[6] = {62, 64, 66, 69, 71, 74};
  for (int k = 0; k < 6; k++) S.bell.notes.push_back({T("4j", 0.6f + k * 0.45f + 3.0f), 0.5f, lands[k] + 12, 0.38f, -0.5f + k * 0.2f});
  padChord(S, T("4j", 4.0f), ch.e - T("4j", 4.0f) + 0.8, "Gmaj7", 55, 4, 0.45f);
  arpeggio(S, S.piano, Grid{T("4j", 4.0f), 72}, 0, std::max(4.0, (ch.e - T("4j", 4.0f)) * 72 / 60), "Gmaj7", 55, {0, 2, 1, 3}, 1, 0.3f);
}

void chapter5(Score& S, const Chapter& ch) {
  Grid g{ch.s + 0.3, 80};
  // Tests: a short figure as each one appears; the string lights come on bulb by bulb.
  bars(g, 0, c("5d").s, {"D", "G", "D", "Asus4"}, [&](double bb, const std::string& cn, int) {
    padChord(S, g.beat(bb), g.secs(4), cn, 55, 3, 0.36f);
    S.piano.notes.push_back(S.human(g.beat(bb), (float)g.secs(3.5), voicing(chord(cn), 50, 1)[0], 0.32f));
  });
  S.bell.notes.push_back({T("5b", 1.4f), 0.4f, 81, 0.42f, -0.3f});
  S.bell.notes.push_back({T("5b", 4.0f), 0.4f, 83, 0.42f, -0.5f});
  for (int i = 0; i <= 12; i++) {
    static const int up[13] = {0, 2, 4, 7, 9, 12, 14, 16, 19, 21, 24, 26, 28};
    S.bell.notes.push_back({T("5c", 1.0f) + i * 0.075, 0.3f, (float)(74 + up[i]) - 12, 0.28f, -0.6f + i * 0.1f});
  }
  // Weeks pass: a music-box note for each bead (the same times as the jar), over a groove that
  // gains a layer each week.
  const int NB = 46;
  const int penta[5] = {0, 2, 4, 7, 9};
  for (int i = 0; i < NB; i++) {
    double tl = lerp(T("5d", 0.4f), c("5f").e, std::pow(i / (float)(NB - 1), 0.9f)) + 0.7;
    int step = (i * 3 + (i / 5)) % 10;
    float p = (float)(74 + (step / 5) * 12 + penta[step % 5]);
    S.bell.notes.push_back({tl, 0.3f, p, 0.24f + 0.12f * (i / (float)NB), (float)std::sin(i * 1.7) * 0.6f});
  }
  Grid gw{c("5d").s, 80};
  double qualityAt = T("5g", 3.0f);
  bars(gw, 0, qualityAt - 1.0, {"D", "Bm", "G", "A"}, [&](double bb, const std::string& cn, int) {
    double t = gw.beat(bb);
    padChord(S, t, gw.secs(4), cn, 57, 3, 0.32f);
    S.piano.notes.push_back(S.human(t, (float)gw.secs(3.5), voicing(chord(cn), 50, 1)[0], 0.3f));
    if (t > c("5e").s) bassNoteAt(S, gw, bb, 4, cn, 0.5f);
    if (t > c("5f").s) {
      arpeggio(S, S.pluck, gw, bb, 4, cn, 62, {0, 1, 2, 1}, 0.5, 0.36f, 0.3f, 0.6f);
      for (int k = 0; k < 4; k++) S.shaker.notes.push_back(S.human(gw.beat(bb + k + 0.5), 0.1f, 0, 0.3f, -0.3f));
    }
  });
  // The change in kind: the beads link up, a gold ring spreads, and the theme comes back in full.
  S.swell.notes.push_back({qualityAt - 1.6, 1.6f, 0, 0.55f, 0});
  S.bell.notes.push_back({qualityAt, 0.6f, 86, 0.55f, 0});
  Grid gt{qualityAt, 76};
  theme(S, gt, 0, D4, 0.5f, true);
  for (int k = 0; k < 32; k++) {
    if (gt.beat(k + 0.5) > c("5i").s) break;
    S.shaker.notes.push_back(S.human(gt.beat(k + 0.5), 0.1f, 0, 0.3f, -0.3f));
  }
  // The setback: noise complaints. The theme breaks off into minor (notes after this point are dropped below).
  double setback = T("5i", 0.4f);
  for (Track* tr : {&S.piano, &S.pad, &S.bass, &S.pluck, &S.shaker})
    std::erase_if(tr->notes, [&](const Note& n) { return n.t > setback - 0.05 && n.t < ch.e + 0.1 && n.t > qualityAt - 0.01; });
  for (Note& n : S.pad.notes)
    if (n.t < setback && n.t + n.dur > setback && n.t > qualityAt - 0.01) n.dur = (float)(setback - n.t);
  Grid gs{setback, 76};
  bars(gs, 0, c("5j").s + 0.6, {"Bm", "G", "Em", "F#"}, [&](double bb, const std::string& cn, int) {
    padChord(S, gs.beat(bb), gs.secs(4), cn, 54, 3, 0.4f);
    arpeggio(S, S.piano, gs, bb, 4, cn, 47, {0, 2, 1, 2}, 1, 0.32f);
    bassNoteAt(S, gs, bb, 4, cn, 0.5f);
  });
  S.piano.notes.push_back({setback, 0.6f, 60, 0.45f, 0.5f});
  S.piano.notes.push_back({setback, 0.6f, 61, 0.42f, 0.5f});
  // The revision, and Mao's cycle: back to major, ending on a warm cadence.
  Grid gr{T("5j", 0.6f), 76};
  bars(gr, 0, ch.e - 0.5, {"G", "D/F#", "Em7", "Asus4", "G", "A", "D", "D"}, [&](double bb, const std::string& cn, int) {
    padChord(S, gr.beat(bb), gr.secs(4), cn, 55, 4, 0.42f);
    arpeggio(S, S.piano, gr, bb, 4, cn, 50, {0, 1, 2, 3, 2, 1, 2, 3}, 0.5, 0.32f, -0.1f);
    bassNoteAt(S, gr, bb, 4, cn, 0.52f);
  });
  phrase(S, S.piano, Grid{T("5k", 0.6f), 76}, 0, "3:2 5:2 | 8:3 7:1 | 8:4", D4, 0.42f);
}

void chapter6(Score& S, const Chapter& ch) {
  // Night: a shimmer of high bells and a soft pad.
  padChord(S, ch.s + 0.2, c("6b").s - ch.s, "Dadd9", 57, 4, 0.4f);
  for (int i = 0; i < 10; i++) S.bell.notes.push_back({ch.s + 0.5 + i * 0.32, 0.3f, (float)(86 + (i * 5) % 12), 0.18f, (float)std::sin(i * 2.1)});
  // Stage 1: the opening of the theme in D. Stage 2: the same in A, the contrast. Stage 3: back in D,
  // an octave higher — a return that has climbed.
  struct Stage { const char* cue; int root; const char* chordA; const char* chordB; };
  const Stage stages[3] = {{"6b", D4, "D", "A/C#"}, {"6c", D4 + 7, "A", "E/G#"}, {"6d", D4 + 12, "D", "A/C#"}};
  for (int s = 0; s < 3; s++) {
    Grid g{T(stages[s].cue, 0.4f), 66};
    phrase(S, S.piano, g, 0, "3:1 5:1 8:2 | 7:1 6:1 5:2", stages[s].root, 0.4f + s * 0.04f);
    padChord(S, g.beat(0), g.secs(4), stages[s].chordA, stages[s].root - 7, 4, 0.42f);
    padChord(S, g.beat(4), g.secs(4) + 0.5, stages[s].chordB, stages[s].root - 7, 4, 0.42f);
    S.bass.notes.push_back({g.beat(0), (float)g.secs(7.5), bassNote(chord(stages[s].chordA), 38), 0.45f, 0});
    S.bell.notes.push_back({g.beat(0), 0.5f, (float)stages[s].root + 24, 0.3f, 0});
  }
  // From above, a return; from the side, a climb: a rising spiral that circles from ear to ear.
  double t0 = T("6e", 0.6f);
  for (int i = 0; i < 24; i++) {
    static const int dpent[5] = {0, 2, 4, 7, 9};
    float p = (float)(D4 + 12 * (i / 5) + dpent[i % 5]);
    S.bell.notes.push_back({t0 + i * 0.32, 0.4f, p, 0.3f, (float)std::sin(i * 0.9)});
  }
  padChord(S, t0, c("6f").s - t0 + 0.5, "Gmaj7", 55, 4, 0.4f);
  // Negation of the negation, named: D, G over D, D.
  double f = c("6f").s;
  double step = (ch.e - f) / 3;
  const char* cs[3] = {"D", "G/D", "Dadd9"};
  for (int i = 0; i < 3; i++) {
    padChord(S, f + i * step, step + 0.4, cs[i], 55, 4, 0.45f);
    S.bass.notes.push_back({f + i * step, (float)step, 38, 0.45f, 0});
    arpeggio(S, S.piano, Grid{f + i * step, 66}, 0, std::floor(step * 66 / 60), cs[i], 62, {0, 1, 2, 3}, 1, 0.3f, 0.15f);
  }
}

void chapter7(Score& S, const Chapter& ch, float duration) {
  // A note for each pin as it appears, rising through the scale.
  const float pins[6] = {74, 76, 78, 81, 83, 86};
  const double times[6] = {T("7b", 0.2f), T("7b", 2.4f), T("7c", 0.3f), T("7c", 2.4f), T("7c", 4.4f), T("7c", 6.4f)};
  for (int i = 0; i < 6; i++) S.bell.notes.push_back({times[i] + 0.25, 0.5f, pins[i], 0.45f, -0.5f + i * 0.2f});
  Grid g{ch.s + 0.4, 72};
  bars(g, 0, c("7d").s, {"D", "Bm", "G", "A"}, [&](double bb, const std::string& cn, int) {
    padChord(S, g.beat(bb), g.secs(4), cn, 55, 4, 0.4f);
    arpeggio(S, S.piano, g, bb, 4, cn, 50, {0, 2, 1, 3}, 1, 0.3f, -0.15f);
    bassNoteAt(S, g, bb, 4, cn, 0.48f);
  });
  // The theme once more.
  Grid gt{c("7d").s, 72};
  theme(S, gt, 0, D4, 0.48f, true);
  // The question: everything but a quiet suspended chord falls away.
  double q = c("7e").s;
  for (Track* tr : {&S.piano, &S.pad, &S.bass})
    std::erase_if(tr->notes, [&](const Note& n) { return n.t > q - 0.1 && n.t < duration + 30 && n.t > gt.t0 - 0.01; });
  for (Note& n : S.pad.notes)
    if (n.t < q && n.t + n.dur > q && n.t > gt.t0 - 0.01) n.dur = (float)(q - n.t);
  padChord(S, q, c("7f").s - q, "Gmaj7", 55, 4, 0.38f);
  S.piano.notes.push_back({q + 0.4, 3.f, 69, 0.3f, 0});
  // The action, and the end title: a warm cadence and a long last chord.
  Grid ga{c("7f").s, 72};
  padChord(S, ga.beat(0), ga.secs(4), "Asus4", 57, 4, 0.42f);
  padChord(S, ga.beat(4), ga.secs(4), "A", 57, 4, 0.42f);
  arpeggio(S, S.piano, ga, 0, 8, "Asus4", 50, {0, 1, 2, 3}, 1, 0.3f);
  double last = std::max(ga.beat(8), (double)c("7f").e + 1.2);
  padChord(S, last, duration - last, "Dadd9", 50, 5, 0.5f);
  for (float p : voicing(chord("Dadd9"), 50, 5)) S.piano.notes.push_back({last + (p - 50) * 0.012, 6.f, p, 0.38f, 0});
  S.bass.notes.push_back({last, 6.f, 38, 0.55f, 0});
  S.bell.notes.push_back({last + 0.6, 0.6f, 86, 0.35f, 0.2f});
  S.bell.notes.push_back({last + 1.2, 0.6f, 90, 0.28f, -0.2f});
}

}  // namespace

eng::Audio composeScore(int rate) {
  Score S;
  const auto& chs = script::chapters();
  const float D = script::duration();
  chapter1(S, chs[0]);
  chapter2(S, chs[1]);
  chapter3(S, chs[2]);
  chapter4(S, chs[3]);
  chapter5(S, chs[4]);
  chapter6(S, chs[5]);
  chapter7(S, chs[6], D);
  std::vector<Track> tracks = {S.piano, S.pad, S.pluck, S.bass, S.bell, S.shaker, S.tick, S.thud, S.shutter, S.swell};
  eng::Audio music = render(tracks, D + 0.5, rate, {.room = 0.84f, .damping = 0.5f, .predelay = 0.025f, .width = 0.9f});
  // Fade in from silence and out with the picture.
  size_t n = music.frames();
  for (size_t i = 0; i < n; i++) {
    float t = (float)i / rate;
    float g = std::min(1.f, t / 0.3f) * clamp((D - t) / 3.f, 0.f, 1.f);
    music.samples[i * 2] *= g;
    music.samples[i * 2 + 1] *= g;
  }
  return music;
}

}  // namespace film
