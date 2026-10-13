// Output: an H.264 MP4 through an ffmpeg pipe, PNG stills, and simple audio buffers (narration, music).
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace eng {

class VideoWriter {
 public:
  struct Options {
    std::string path;
    int width = 1920, height = 1080, fps = 30;
    int crf = 18;
    std::string preset = "medium";
    std::string audioPath;  // optional WAV muxed as AAC
  };
  explicit VideoWriter(const Options& o);
  ~VideoWriter();
  VideoWriter(const VideoWriter&) = delete;
  VideoWriter& operator=(const VideoWriter&) = delete;
  // One RGBA8 frame, top row first.
  void write(const uint8_t* rgba);
  // Finish encoding; returns false if ffmpeg reported an error.
  bool close();

 private:
  Options opt_;
  FILE* pipe_ = nullptr;
};

bool savePng(const std::string& path, const uint8_t* rgba, int width, int height);

// Audio as floats, channels interleaved (1 = mono, 2 = stereo).
struct Audio {
  int rate = 48000;
  int channels = 1;
  std::vector<float> samples;
  Audio() = default;
  Audio(int rate_, int channels_, double seconds = 0) : rate(rate_), channels(channels_) { resize(seconds); }
  size_t frames() const { return samples.size() / channels; }
  double duration() const { return (double)frames() / rate; }
  // 16-bit PCM WAV, keeping its channels.
  static Audio loadWav(const std::string& path);
  // A copy at another sample rate (windowed-sinc interpolation).
  Audio resampled(int newRate) const;
  // Add `src` starting at `at` seconds, scaled by `gain`. Rates must match. A mono source mixed into
  // stereo is panned with equal power (pan -1 = left, 1 = right); a stereo source keeps its channels.
  void mix(const Audio& src, double at, float gain = 1, float pan = 0);
  void resize(double seconds) { samples.resize((size_t)std::llround(seconds * rate) * channels, 0.f); }
  float peak() const;
  bool saveWav(const std::string& path) const;
};

}  // namespace eng
