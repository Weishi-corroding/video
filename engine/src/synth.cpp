#include "eng/synth.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <map>
#include <stdexcept>

#include "eng/math.h"

namespace eng::synth {

float hz(float midi) { return 440.f * std::pow(2.f, (midi - 69) / 12.f); }

namespace {

constexpr float TAU = 2 * PI;

// Deterministic white noise in -1..1 (xorshift).
struct Noise {
  uint32_t s;
  explicit Noise(uint32_t seed) : s(seed * 2654435761u + 0x9e3779b9u) {
    if (!s) s = 1;
  }
  float operator()() {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return (float)(s / 4294967296.0) * 2 - 1;
  }
};

// A topology-preserving state-variable filter: low-pass output, band-pass on request.
struct SVF {
  float a1 = 0, a2 = 0, a3 = 0, k = 1.4f, ic1 = 0, ic2 = 0;
  void set(float cutoff, float q, float rate) {
    cutoff = clamp(cutoff, 10.f, rate * 0.45f);
    float g = std::tan(PI * cutoff / rate);
    k = 1 / q;
    a1 = 1 / (1 + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
  }
  float lp(float x, float* bp = nullptr) {
    float v3 = x - ic2;
    float v1 = a1 * ic1 + a2 * v3;
    float v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2 * v1 - ic1;
    ic2 = 2 * v2 - ic2;
    if (bp) *bp = v1;
    return v2;
  }
  float bp(float x) {
    float b;
    lp(x, &b);
    return b;
  }
};

// A sine oscillator by rotation: two multiplies and adds per sample.
struct Osc {
  double c, s, x = 1, y = 0;
  Osc(double freq, double rate, double phase = 0) : c(std::cos(TAU * freq / rate)), s(std::sin(TAU * freq / rate)), x(std::cos(phase)), y(std::sin(phase)) {}
  float next() {
    double nx = x * c - y * s;
    y = x * s + y * c;
    x = nx;
    return (float)y;
  }
  void renormalise() {
    double r = 1 / std::sqrt(x * x + y * y);
    x *= r;
    y *= r;
  }
};

// One note's sound, stereo, starting at frame `start`.
struct Voice {
  long start = 0;
  std::vector<float> l, r;
  void resize(size_t n) {
    l.assign(n, 0.f);
    r.assign(n, 0.f);
  }
  // Equal-power pan of a mono signal (centre = unit gain per channel).
  void addMono(size_t i, float x, float pan) {
    float a = (clamp(pan, -1.f, 1.f) + 1) * PI / 4;
    l[i] += x * std::cos(a) * 1.41421356f;
    r[i] += x * std::sin(a) * 1.41421356f;
  }
};

float panGainL(float pan) { return std::cos((clamp(pan, -1.f, 1.f) + 1) * PI / 4) * 1.41421356f; }
float panGainR(float pan) { return std::sin((clamp(pan, -1.f, 1.f) + 1) * PI / 4) * 1.41421356f; }

Voice piano(const Note& n, float bright, int rate, uint32_t seed) {
  Voice v;
  const float f0 = hz(n.pitch);
  const float vel = clamp(n.vel, 0.f, 1.f);
  const float T0 = clamp(2.8f * std::pow(2.f, -(n.pitch - 60) / 16.f), 0.45f, 8.f);  // decay of the fundamental
  const float hold = std::max(n.dur, 0.06f);
  const float rel = 0.14f + 0.12f * clamp((60 - n.pitch) / 24.f, 0.f, 1.f);         // damper
  const float len = std::min(hold + 7 * rel, 9.2f * T0) + 0.02f;
  const size_t N = (size_t)(len * rate);
  v.resize(N);
  const float B = 0.00009f * std::pow(2.f, (n.pitch - 60) / 15.f);  // string stiffness
  const float fc = 260 + f0 * (1.2f + 6.5f * bright * (0.35f + 0.65f * vel));     // felt: brighter when struck harder
  const float attack = 0.003f + 0.007f * (1 - vel);
  Noise noise(seed);
  std::vector<float> mono(N, 0.f);
  float norm = 0;
  for (int k = 1; k <= 18; k++) {
    float fk = f0 * k * std::sqrt(1 + B * k * k);
    if (fk > std::min(rate * 0.45f, 12000.f)) break;
    float amp = std::pow((float)k, -1.15f) / (1 + (fk / fc) * (fk / fc));
    if (amp < 0.004f) continue;
    norm += amp;
    float tau = T0 / (1 + 0.55f * (k - 1));
    float det = 0.00032f * (1 + 0.05f * k);
    Osc a(fk * (1 - det), rate, noise() * PI), b(fk * (1 + det), rate, noise() * PI);
    // Two-stage decay: a quick prompt sound and a long aftersound, as in a real piano.
    double e1 = 1, e2 = 1;
    const double d1 = std::exp(-1.0 / (tau * 0.28 * rate)), d2 = std::exp(-1.0 / (tau * rate));
    for (size_t i = 0; i < N; i++) {
      float s = 0.5f * (a.next() + b.next());
      mono[i] += amp * s * (float)(0.6 * e1 + 0.4 * e2);
      e1 *= d1;
      e2 *= d2;
      if ((i & 1023) == 0) { a.renormalise(); b.renormalise(); }
    }
  }
  // The felt hammer: a very short, soft knock.
  SVF hf;
  hf.set(std::min(fc * 1.5f, 3500.f), 0.7f, (float)rate);
  for (size_t i = 0; i < std::min<size_t>(N, (size_t)(0.03f * rate)); i++) {
    float t = (float)i / rate;
    mono[i] += hf.lp(noise()) * 0.25f * bright * std::exp(-t / 0.005f) * norm;
  }
  float gain = 0.42f * vel / std::max(norm, 0.5f);
  for (size_t i = 0; i < N; i++) {
    float t = (float)i / rate;
    float env = std::min(1.f, t / attack);
    if (t > hold) env *= std::exp(-(t - hold) / rel);
    v.addMono(i, mono[i] * env * gain, n.pan);
  }
  return v;
}

Voice pluck(const Note& n, float bright, int rate, uint32_t seed, bool bass) {
  Voice v;
  const float f0 = hz(n.pitch);
  const float vel = clamp(n.vel, 0.f, 1.f);
  const float hold = std::max(n.dur, 0.05f);
  const float T60 = clamp((bass ? 2.6f : 3.2f) * std::pow(2.f, -(n.pitch - 48) / 22.f), 0.5f, 6.f);
  const float len = std::min(hold + 0.6f, T60 * 1.1f) + 0.02f;
  const size_t N = (size_t)(len * rate);
  v.resize(N);
  // Delay line, with the fractional part of the period in a first-order all-pass.
  float period = rate / f0;
  int L = std::max(2, (int)std::floor(period - 0.5f));
  float frac = period - 0.5f - L;
  float ap = (1 - frac) / (1 + frac);
  std::vector<float> buf(L);
  Noise noise(seed);
  // Excitation: noise, darker when softer or when `bright` is low, minus a shifted copy (plucking point).
  float lpCoef = clamp(0.15f + 0.75f * bright * (0.4f + 0.6f * vel), 0.05f, 0.95f);
  float z = 0;
  for (int i = 0; i < L; i++) {
    z += (noise() - z) * lpCoef;
    buf[i] = z;
  }
  int pick = std::max(1, (int)(L * 0.18f));
  std::vector<float> ex(buf);
  for (int i = 0; i < L; i++) buf[i] = ex[i] - ex[(i + pick) % L];
  float mean = 0;
  for (float x : buf) mean += x;
  mean /= L;
  for (float& x : buf) x -= mean;
  const float gHold = std::pow(10.f, -3.f / (T60 * f0)), gDamp = std::pow(10.f, -3.f / (0.15f * f0));
  float apIn = 0, apOut = 0;
  int p = 0;
  SVF tone;
  tone.set(bass ? 900 + 1200 * bright : 2500 + 6000 * bright, 0.7f, (float)rate);
  float gain = (bass ? 0.9f : 0.55f) * vel;
  for (size_t i = 0; i < N; i++) {
    float t = (float)i / rate;
    float y = buf[p];
    float g = t < hold ? gHold : gDamp;
    float avg = g * 0.5f * (buf[p] + buf[(p + 1) % L]);
    float o = ap * avg + apIn - ap * apOut;
    apIn = avg;
    apOut = o;
    buf[p] = o;
    p = (p + 1) % L;
    float s = tone.lp(y);
    if (bass) s = std::tanh(1.6f * s) / std::tanh(1.6f);
    v.addMono(i, s * gain * std::min(1.f, t / 0.002f), n.pan);
  }
  return v;
}

// A band-limited sawtooth (polyBLEP).
struct Saw {
  double phase, inc = 0;
  explicit Saw(double ph) : phase(ph) {}
  float next(double freq, double rate) {
    inc = freq / rate;
    phase += inc;
    if (phase >= 1) phase -= 1;
    double t = phase, y = 2 * t - 1;
    if (t < inc) { t /= inc; y -= t + t - t * t - 1; }
    else if (t > 1 - inc) { t = (t - 1) / inc; y -= t * t + t + t + 1; }
    return (float)y;
  }
};

Voice pad(const Note& n, float bright, int rate, uint32_t seed) {
  Voice v;
  const float f0 = hz(n.pitch);
  const float vel = clamp(n.vel, 0.f, 1.f);
  const float hold = std::max(n.dur, 0.2f);
  const float A = std::min(1.4f, hold * 0.5f), R = 2.2f;
  const size_t N = (size_t)((hold + R * 3) * rate);
  v.resize(N);
  Noise noise(seed);
  const float det[3] = {-0.075f, 0.f, 0.065f}, pans[3] = {-0.55f, 0.f, 0.55f};
  Saw saw[3] = {Saw(0.5 + 0.5 * noise()), Saw(0.5 + 0.5 * noise()), Saw(0.5 + 0.5 * noise())};
  SVF filt[3];
  float lfoPh[3] = {noise() * PI, noise() * PI, noise() * PI};
  Osc sub(f0 / 2, rate);
  const float cutoffBase = std::min(f0 * (2.2f + 5.0f * bright), 7000.f);
  const float gain = 0.075f * vel;
  for (size_t i = 0; i < N; i++) {
    float t = (float)i / rate;
    float env = t < A ? 0.5f - 0.5f * std::cos(PI * t / A) : 1.f;
    if (t > hold) env *= std::exp(-(t - hold) / (R * 0.45f));
    if ((i & 31) == 0)
      for (int k = 0; k < 3; k++) filt[k].set(cutoffBase * (0.55f + 0.45f * env), 0.75f, (float)rate);
    float sl = 0, sr = 0;
    for (int k = 0; k < 3; k++) {
      float vib = 0.035f * std::sin(TAU * 0.17f * t + lfoPh[k]);
      float s = filt[k].lp(saw[k].next(hz(n.pitch + det[k] + vib), rate));
      float pk = clamp(n.pan + pans[k], -1.f, 1.f);
      sl += s * panGainL(pk);
      sr += s * panGainR(pk);
    }
    float sb = sub.next() * 0.35f;
    if ((i & 1023) == 0) sub.renormalise();
    v.l[i] = (sl + sb) * env * gain;
    v.r[i] = (sr + sb) * env * gain;
  }
  return v;
}

Voice bell(const Note& n, float bright, int rate, uint32_t seed) {
  Voice v;
  const float f0 = hz(n.pitch);
  const float vel = clamp(n.vel, 0.f, 1.f);
  const float ratio[4] = {1.f, 2.756f, 5.404f, 8.933f};
  const float amp[4] = {1.f, 0.38f, 0.2f, 0.09f};
  const float tau0 = clamp(1.7f * std::pow(2.f, -(n.pitch - 72) / 24.f), 0.4f, 4.f);
  const float tau[4] = {tau0, tau0 * 0.42f, tau0 * 0.2f, tau0 * 0.09f};
  const size_t N = (size_t)(tau0 * 7 * rate);
  v.resize(N);
  Noise noise(seed);
  std::vector<float> mono(N, 0.f);
  for (int k = 0; k < 4; k++) {
    float fk = f0 * ratio[k];
    if (fk > rate * 0.45f) break;
    float a = amp[k] * (k == 0 ? 1.f : 0.4f + 1.2f * bright);
    Osc o(fk, rate, noise() * PI);
    double e = 1, d = std::exp(-1.0 / (tau[k] * rate));
    for (size_t i = 0; i < N; i++) {
      mono[i] += a * o.next() * (float)e;
      e *= d;
      if ((i & 1023) == 0) o.renormalise();
    }
  }
  float gain = 0.2f * vel;
  for (size_t i = 0; i < N; i++) v.addMono(i, mono[i] * gain * std::min(1.f, (float)i / (0.0015f * rate)), n.pan);
  return v;
}

Voice noiseHit(Instrument kind, const Note& n, float bright, int rate, uint32_t seed) {
  Voice v;
  const float vel = clamp(n.vel, 0.f, 1.f);
  Noise noise(seed), noise2(seed + 77);
  SVF f, f2;
  size_t N = 0;
  switch (kind) {
    case Instrument::Shaker: {
      N = (size_t)(0.25f * rate);
      v.resize(N);
      f.set(6500 + 2500 * bright, 1.1f, (float)rate);
      for (size_t i = 0; i < N; i++) {
        float t = (float)i / rate;
        float env = std::min(1.f, t / 0.008f) * std::exp(-t / 0.05f);
        v.addMono(i, f.bp(noise()) * env * 0.5f * vel, n.pan);
      }
      break;
    }
    case Instrument::Tick: {
      N = (size_t)(0.12f * rate);
      v.resize(N);
      Osc o(1700 + 600 * bright, rate);
      f.set(2600, 1.5f, (float)rate);
      for (size_t i = 0; i < N; i++) {
        float t = (float)i / rate;
        float s = o.next() * std::exp(-t / 0.012f) * 0.6f + f.bp(noise()) * std::exp(-t / 0.005f) * 0.9f;
        v.addMono(i, s * 0.35f * vel, n.pan);
      }
      break;
    }
    case Instrument::Thud: {
      N = (size_t)(1.2f * rate);
      v.resize(N);
      f.set(420, 0.7f, (float)rate);
      double ph = 0;
      for (size_t i = 0; i < N; i++) {
        float t = (float)i / rate;
        float freq = 42 + 70 * std::exp(-t / 0.06f);
        ph += TAU * freq / rate;
        float s = (float)std::sin(ph) * std::exp(-t / 0.3f) + f.lp(noise()) * std::exp(-t / 0.035f) * 0.8f;
        v.addMono(i, std::tanh(1.4f * s) * 0.75f * vel, n.pan);
      }
      break;
    }
    case Instrument::Shutter: {
      N = (size_t)(0.2f * rate);
      v.resize(N);
      f.set(3300, 0.9f, (float)rate);
      Osc o(620, rate);
      for (size_t i = 0; i < N; i++) {
        float t = (float)i / rate;
        float s = 0;
        for (float c : {0.f, 0.06f}) {
          float u = t - c;
          if (u >= 0) s += std::exp(-u / 0.007f) * (c == 0 ? 1.f : 0.7f);
        }
        float x = f.bp(noise()) * s + o.next() * s * 0.25f;
        v.addMono(i, x * 0.6f * vel, n.pan);
      }
      break;
    }
    case Instrument::Swell: {
      float d = std::max(n.dur, 0.3f);
      N = (size_t)((d + 0.25f) * rate);
      v.resize(N);
      v.start = -(long)(0 * rate);
      for (size_t i = 0; i < N; i++) {
        float t = (float)i / rate;
        float u = clamp(t / d, 0.f, 1.f);
        if ((i & 31) == 0) {
          float fc = 250 + (2500 + 5000 * bright) * u * u;
          f.set(fc, 0.8f, (float)rate);
          f2.set(fc * 1.1f, 0.8f, (float)rate);
        }
        float env = std::pow(u, 2.2f) * (t > d ? std::exp(-(t - d) / 0.06f) : 1.f);
        v.l[i] = f.lp(noise()) * env * 0.3f * vel;
        v.r[i] = f2.lp(noise2()) * env * 0.3f * vel;
      }
      break;
    }
    case Instrument::Page: {
      float d = std::max(n.dur, 0.4f);
      N = (size_t)((d + 0.35f) * rate);
      v.resize(N);
      SVF lo;
      lo.set(160, 0.7f, (float)rate);
      float flutterPh = noise() * PI, flutterHz = 21 + 9 * noise2();
      for (size_t i = 0; i < N; i++) {
        float t = (float)i / rate;
        float u = clamp(t / d, 0.f, 1.f);
        if ((i & 31) == 0) {
          f.set(1400 + 2600 * std::sin(PI * u) * (0.6f + 0.4f * bright), 0.9f, (float)rate);
          f2.set(1600 + 2400 * std::sin(PI * u), 0.9f, (float)rate);
        }
        // Rises quickly, flutters as the sheet bends, and ends with the soft slap of the page landing.
        float env = std::min(1.f, t / 0.12f) * (t < d ? 1.f : std::exp(-(t - d) / 0.05f)) * (0.55f + 0.45f * std::sin(PI * u));
        float flutter = 0.65f + 0.35f * std::fabs(std::sin(TAU * flutterHz * t + flutterPh));
        float l = f.bp(noise()) * env * flutter, r = f2.bp(noise2()) * env * flutter;
        float land = t > d * 0.92f ? lo.lp(noise()) * std::exp(-(t - d * 0.92f) / 0.06f) * 2.5f : 0.f;
        float pan = 0.7f - 1.4f * u;  // right to left
        v.l[i] = (l + land) * panGainL(pan) * 0.35f * vel;
        v.r[i] = (r + land) * panGainR(pan) * 0.35f * vel;
      }
      break;
    }
    case Instrument::Flap: {
      N = (size_t)(0.3f * rate);
      v.resize(N);
      f.set(900 + 900 * bright, 0.8f, (float)rate);
      Osc o(95, rate);
      for (size_t i = 0; i < N; i++) {
        float t = (float)i / rate;
        float s = f.lp(noise()) * std::min(1.f, t / 0.004f) * std::exp(-t / 0.035f) + o.next() * std::exp(-t / 0.05f) * 0.5f;
        v.addMono(i, s * 0.6f * vel, n.pan);
      }
      break;
    }
    default: break;
  }
  return v;
}

// ---------- reverb (after Freeverb by Jezar at Dreampoint) ----------
struct Comb {
  std::vector<float> buf;
  size_t i = 0;
  float store = 0;
  float process(float in, float fb, float damp) {
    float out = buf[i];
    store = out * (1 - damp) + store * damp;
    buf[i] = in + store * fb;
    if (++i >= buf.size()) i = 0;
    return out;
  }
};
struct Allpass {
  std::vector<float> buf;
  size_t i = 0;
  float process(float in) {
    float b = buf[i];
    buf[i] = in + b * 0.5f;
    if (++i >= buf.size()) i = 0;
    return b - in;
  }
};

void reverbInto(std::vector<float>& L, std::vector<float>& R, const std::vector<float>& sL, const std::vector<float>& sR, int rate, const ReverbSettings& rs) {
  const int combT[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617}, apT[4] = {556, 441, 341, 225};
  const float scale = rate / 44100.f;
  Comb cl[8], cr[8];
  Allpass al[4], ar[4];
  for (int k = 0; k < 8; k++) {
    cl[k].buf.assign((size_t)(combT[k] * scale), 0.f);
    cr[k].buf.assign((size_t)((combT[k] + 23) * scale), 0.f);
  }
  for (int k = 0; k < 4; k++) {
    al[k].buf.assign((size_t)(apT[k] * scale), 0.f);
    ar[k].buf.assign((size_t)((apT[k] + 23) * scale), 0.f);
  }
  const float fb = rs.room * 0.28f + 0.7f, damp = rs.damping * 0.4f;
  const float wet1 = rs.width / 2 + 0.5f, wet2 = (1 - rs.width) / 2;
  const size_t pre = (size_t)(rs.predelay * rate);
  for (size_t i = 0; i < L.size(); i++) {
    float in = i >= pre ? (sL[i - pre] + sR[i - pre]) * 0.015f : 0.f;
    float ol = 0, orr = 0;
    for (int k = 0; k < 8; k++) {
      ol += cl[k].process(in, fb, damp);
      orr += cr[k].process(in, fb, damp);
    }
    for (int k = 0; k < 4; k++) {
      ol = al[k].process(ol);
      orr = ar[k].process(orr);
    }
    L[i] += ol * wet1 + orr * wet2;
    R[i] += orr * wet1 + ol * wet2;
  }
}

}  // namespace

Audio render(const std::vector<Track>& tracks, double seconds, int rate, const ReverbSettings& reverb) {
  const size_t N = (size_t)std::ceil(seconds * rate);
  std::vector<float> L(N, 0.f), R(N, 0.f), sL(N, 0.f), sR(N, 0.f);
  for (size_t ti = 0; ti < tracks.size(); ti++) {
    const Track& tr = tracks[ti];
    for (size_t ni = 0; ni < tr.notes.size(); ni++) {
      const Note& n = tr.notes[ni];
      uint32_t seed = (uint32_t)(ti * 7919 + ni * 104729 + 17);
      Voice v;
      switch (tr.instrument) {
        case Instrument::Piano: v = piano(n, tr.brightness, rate, seed); break;
        case Instrument::Pluck: v = pluck(n, tr.brightness, rate, seed, false); break;
        case Instrument::Bass: v = pluck(n, tr.brightness, rate, seed, true); break;
        case Instrument::Pad: v = pad(n, tr.brightness, rate, seed); break;
        case Instrument::Bell: v = bell(n, tr.brightness, rate, seed); break;
        default: v = noiseHit(tr.instrument, n, tr.brightness, rate, seed); break;
      }
      long start = (long)std::llround(n.t * rate) + v.start;
      if (tr.instrument == Instrument::Swell) start = (long)std::llround(n.t * rate);
      for (size_t i = 0; i < v.l.size(); i++) {
        long j = start + (long)i;
        if (j < 0) continue;
        if ((size_t)j >= N) break;
        L[j] += v.l[i] * tr.gain;
        R[j] += v.r[i] * tr.gain;
        sL[j] += v.l[i] * tr.gain * tr.reverb;
        sR[j] += v.r[i] * tr.gain * tr.reverb;
      }
    }
  }
  reverbInto(L, R, sL, sR, rate, reverb);
  Audio out(rate, 2);
  out.samples.resize(N * 2);
  for (size_t i = 0; i < N; i++) {
    out.samples[i * 2] = L[i];
    out.samples[i * 2 + 1] = R[i];
  }
  return out;
}

// ---------- chords ----------
namespace {
int pitchClass(const std::string& s, size_t& i) {
  static const std::map<char, int> base = {{'C', 0}, {'D', 2}, {'E', 4}, {'F', 5}, {'G', 7}, {'A', 9}, {'B', 11}};
  if (i >= s.size() || !base.count(s[i])) throw std::runtime_error("bad chord name: " + s);
  int pc = base.at(s[i++]);
  if (i < s.size() && s[i] == '#') { pc++; i++; }
  else if (i < s.size() && s[i] == 'b') { pc--; i++; }
  return (pc + 12) % 12;
}
}  // namespace

Chord chord(const std::string& name) {
  static const std::map<std::string, std::vector<int>> qualities = {
      {"", {0, 4, 7}},          {"m", {0, 3, 7}},          {"7", {0, 4, 7, 10}},      {"maj7", {0, 4, 7, 11}},
      {"m7", {0, 3, 7, 10}},    {"sus2", {0, 2, 7}},       {"sus4", {0, 5, 7}},       {"add9", {0, 4, 7, 14}},
      {"madd9", {0, 3, 7, 14}}, {"dim", {0, 3, 6}},        {"6", {0, 4, 7, 9}},       {"m6", {0, 3, 7, 9}},
      {"9", {0, 4, 7, 10, 14}}, {"maj9", {0, 4, 7, 11, 14}}, {"m9", {0, 3, 7, 10, 14}}, {"7sus4", {0, 5, 7, 10}},
      {"aug", {0, 4, 8}},       {"5", {0, 7}},
  };
  Chord c;
  size_t i = 0;
  c.root = pitchClass(name, i);
  size_t slash = name.find('/', i);
  std::string q = name.substr(i, slash == std::string::npos ? std::string::npos : slash - i);
  auto it = qualities.find(q);
  if (it == qualities.end()) throw std::runtime_error("unknown chord quality: " + name);
  for (int iv : it->second) c.tones.push_back((c.root + iv) % 12);
  c.bass = c.root;
  if (slash != std::string::npos) {
    size_t j = slash + 1;
    c.bass = pitchClass(name, j);
  }
  return c;
}

std::vector<float> voicing(const Chord& c, float low, int count) {
  std::vector<float> out;
  int p = (int)std::ceil(low);
  while ((int)out.size() < count) {
    if (std::find(c.tones.begin(), c.tones.end(), ((p % 12) + 12) % 12) != c.tones.end()) out.push_back((float)p);
    p++;
  }
  return out;
}

float bassNote(const Chord& c, float low) {
  int p = (int)std::ceil(low);
  while (((p % 12) + 12) % 12 != c.bass) p++;
  return (float)p;
}

// ---------- mixing ----------
std::vector<float> duckingCurve(const Audio& voice, int rate, size_t frames, float depth, float attack, float release) {
  if (voice.rate != rate) throw std::runtime_error("ducking: sample rates differ");
  std::vector<float> target(frames, 1.f);
  // Speech is present where a 40 ms RMS window is above -36 dBFS.
  const size_t win = (size_t)(0.04f * rate);
  const size_t vf = voice.frames();
  double acc = 0;
  std::vector<float> sq(vf);
  for (size_t i = 0; i < vf; i++) {
    float s = 0;
    for (int c = 0; c < voice.channels; c++) s += voice.samples[i * voice.channels + c];
    s /= voice.channels;
    sq[i] = s * s;
  }
  for (size_t i = 0; i < std::min(frames, vf); i++) {
    acc += sq[i];
    if (i >= win) acc -= sq[i - win];
    float rms = (float)std::sqrt(std::max(0.0, acc / win));
    if (rms > 0.016f) target[i] = depth;
  }
  // Release (rising back to 1) runs forward in time; attack runs backward so the dip starts just
  // before the voice does.
  std::vector<float> g(frames);
  const float kr = 1 - std::exp(-1.f / (release * rate)), ka = 1 - std::exp(-1.f / (attack * rate));
  float cur = 1;
  for (size_t i = 0; i < frames; i++) {
    cur = target[i] < cur ? target[i] : cur + (target[i] - cur) * kr;
    g[i] = cur;
  }
  cur = 1;
  for (size_t i = frames; i-- > 0;) {
    cur = target[i] < cur ? target[i] : cur + (target[i] - cur) * ka;
    g[i] = std::min(g[i], cur);
  }
  return g;
}

void applyGain(Audio& a, const std::vector<float>& gain) {
  size_t n = std::min(a.frames(), gain.size());
  for (size_t i = 0; i < n; i++)
    for (int c = 0; c < a.channels; c++) a.samples[i * a.channels + c] *= gain[i];
}

void limit(Audio& a, float ceiling) {
  const size_t n = a.frames(), look = (size_t)(0.005f * a.rate);
  std::vector<float> need(n, 1.f);
  for (size_t i = 0; i < n; i++) {
    float p = 0;
    for (int c = 0; c < a.channels; c++) p = std::max(p, std::fabs(a.samples[i * a.channels + c]));
    if (p > ceiling) need[i] = ceiling / p;
  }
  // The smallest gain needed within the look-ahead window, then a smooth release.
  std::vector<float> g(n);
  std::deque<size_t> q;
  for (size_t i = 0; i < n + look; i++) {
    if (i < n) {
      while (!q.empty() && need[q.back()] >= need[i]) q.pop_back();
      q.push_back(i);
    }
    if (i >= look) {
      size_t o = i - look;
      while (!q.empty() && q.front() + look < o) q.pop_front();
      g[o] = q.empty() ? 1.f : need[q.front()];
    }
  }
  const float kr = 1 - std::exp(-1.f / (0.08f * a.rate));
  float cur = 1;
  for (size_t i = 0; i < n; i++) {
    cur = g[i] < cur ? g[i] : cur + (g[i] - cur) * kr;
    for (int c = 0; c < a.channels; c++) a.samples[i * a.channels + c] *= cur;
  }
}

}  // namespace eng::synth
