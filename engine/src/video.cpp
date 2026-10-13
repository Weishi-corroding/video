#include "eng/video.h"

#include "eng/math.h"

#include <png.h>
#include <sys/wait.h>

#include <cmath>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace eng {

namespace {
std::string quote(const std::string& s) {
  std::string q = "'";
  for (char c : s) q += c == '\'' ? std::string("'\\''") : std::string(1, c);
  return q + "'";
}
}  // namespace

VideoWriter::VideoWriter(const Options& o) : opt_(o) {
  std::string cmd = "ffmpeg -y -hide_banner -loglevel error -f rawvideo -pix_fmt rgba -s " + std::to_string(o.width) + "x" +
                    std::to_string(o.height) + " -framerate " + std::to_string(o.fps) + " -i -";
  if (!o.audioPath.empty()) cmd += " -i " + quote(o.audioPath);
  cmd += " -map 0:v";
  if (!o.audioPath.empty()) cmd += " -map 1:a";
  // RGB to BT.709 limited-range YUV, and tag the stream so players decode the colours correctly.
  cmd += " -vf scale=out_color_matrix=bt709:out_range=tv,format=yuv420p";
  cmd += " -c:v libx264 -preset " + o.preset + " -crf " + std::to_string(o.crf);
  cmd += " -colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv";
  if (!o.audioPath.empty()) cmd += " -c:a aac -b:a 160k -ar 48000 -ac 2";
  cmd += " -movflags +faststart " + quote(o.path);
  pipe_ = popen(cmd.c_str(), "w");
  if (!pipe_) throw std::runtime_error("could not start ffmpeg");
}

VideoWriter::~VideoWriter() { close(); }

void VideoWriter::write(const uint8_t* rgba) {
  size_t n = (size_t)opt_.width * opt_.height * 4;
  if (!pipe_ || std::fwrite(rgba, 1, n, pipe_) != n) throw std::runtime_error("ffmpeg stopped accepting frames");
}

bool VideoWriter::close() {
  if (!pipe_) return true;
  int status = pclose(pipe_);
  pipe_ = nullptr;
  return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool savePng(const std::string& path, const uint8_t* rgba, int w, int h) {
  png_image img;
  std::memset(&img, 0, sizeof img);
  img.version = PNG_IMAGE_VERSION;
  img.width = w;
  img.height = h;
  img.format = PNG_FORMAT_RGBA;
  return png_image_write_to_file(&img, path.c_str(), 0, rgba, 0, nullptr) != 0;
}

Audio Audio::loadWav(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot open " + path);
  std::vector<char> d((std::istreambuf_iterator<char>(f)), {});
  auto u32 = [&](size_t o) { uint32_t v; std::memcpy(&v, &d[o], 4); return v; };
  auto u16 = [&](size_t o) { uint16_t v; std::memcpy(&v, &d[o], 2); return v; };
  if (d.size() < 12 || std::memcmp(d.data(), "RIFF", 4) || std::memcmp(d.data() + 8, "WAVE", 4)) throw std::runtime_error(path + " is not a WAV file");
  Audio a;
  int bits = 16;
  size_t pos = 12;
  while (pos + 8 <= d.size()) {
    uint32_t size = u32(pos + 4);
    if (!std::memcmp(&d[pos], "fmt ", 4)) {
      a.channels = u16(pos + 10);
      a.rate = (int)u32(pos + 12);
      bits = u16(pos + 22);
    } else if (!std::memcmp(&d[pos], "data", 4)) {
      if (bits != 16) throw std::runtime_error(path + ": only 16-bit PCM is supported");
      size_t n = std::min<size_t>(size, d.size() - pos - 8) / 2;
      n -= n % a.channels;
      a.samples.resize(n);
      for (size_t i = 0; i < n; i++) {
        int16_t v;
        std::memcpy(&v, &d[pos + 8 + i * 2], 2);
        a.samples[i] = v / 32768.f;
      }
    }
    pos += 8 + size + (size & 1);
  }
  return a;
}

Audio Audio::resampled(int newRate) const {
  if (newRate == rate) return *this;
  Audio out(newRate, channels);
  const double ratio = (double)newRate / rate;
  const size_t nIn = frames(), nOut = (size_t)std::floor(nIn * ratio);
  out.samples.assign(nOut * channels, 0.f);
  // Low-pass at the lower of the two Nyquist rates, with a Blackman-windowed sinc of 32 zero crossings.
  const double cutoff = std::min(1.0, ratio);
  const int half = 32;
  const double reach = half / cutoff;
  for (size_t o = 0; o < nOut; o++) {
    double x = o / ratio;
    long lo = (long)std::ceil(x - reach), hi = (long)std::floor(x + reach);
    for (int c = 0; c < channels; c++) {
      double acc = 0, wsum = 0;
      for (long i = std::max(0L, lo); i <= std::min((long)nIn - 1, hi); i++) {
        double t = (i - x) * cutoff;
        double sinc = t == 0 ? 1 : std::sin(PI * t) / (PI * t);
        double u = (i - x) / reach;  // -1..1 across the window
        double win = 0.42 + 0.5 * std::cos(PI * u) + 0.08 * std::cos(2 * PI * u);
        double w = sinc * win;
        acc += samples[(size_t)i * channels + c] * w;
        wsum += w;
      }
      out.samples[o * channels + c] = (float)(wsum != 0 ? acc / wsum : 0);
    }
  }
  return out;
}

void Audio::mix(const Audio& src, double at, float gain, float pan) {
  if (src.rate != rate) throw std::runtime_error("audio sample rates differ");
  size_t start = (size_t)std::max(0.0, std::round(at * rate));
  size_t n = src.frames();
  if (frames() < start + n) samples.resize((start + n) * channels, 0.f);
  if (src.channels == channels) {
    for (size_t i = 0; i < n * channels; i++) samples[start * channels + i] += src.samples[i] * gain;
  } else if (src.channels == 1 && channels == 2) {
    // Equal power, scaled so that the centre keeps unit gain in each channel.
    float a = (clamp(pan, -1.f, 1.f) + 1) * PI / 4;
    float gl = std::cos(a) * std::sqrt(2.f) * gain, gr = std::sin(a) * std::sqrt(2.f) * gain;
    for (size_t i = 0; i < n; i++) {
      samples[(start + i) * 2] += src.samples[i] * gl;
      samples[(start + i) * 2 + 1] += src.samples[i] * gr;
    }
  } else if (src.channels == 2 && channels == 1) {
    for (size_t i = 0; i < n; i++) samples[start + i] += (src.samples[i * 2] + src.samples[i * 2 + 1]) * 0.5f * gain;
  } else {
    throw std::runtime_error("unsupported channel layout for mixing");
  }
}

float Audio::peak() const {
  float p = 0;
  for (float s : samples) p = std::max(p, std::fabs(s));
  return p;
}

bool Audio::saveWav(const std::string& path) const {
  std::ofstream f(path, std::ios::binary);
  if (!f) return false;
  uint32_t dataBytes = (uint32_t)(samples.size() * 2);
  auto w32 = [&](uint32_t v) { f.write((const char*)&v, 4); };
  auto w16 = [&](uint16_t v) { f.write((const char*)&v, 2); };
  f.write("RIFF", 4); w32(36 + dataBytes); f.write("WAVE", 4);
  f.write("fmt ", 4); w32(16); w16(1); w16((uint16_t)channels); w32(rate); w32(rate * 2 * channels); w16((uint16_t)(2 * channels)); w16(16);
  f.write("data", 4); w32(dataBytes);
  for (float s : samples) {
    int v = (int)std::lround(std::max(-1.f, std::min(1.f, s)) * 32767.f);
    w16((uint16_t)(int16_t)v);
  }
  return (bool)f;
}

}  // namespace eng
