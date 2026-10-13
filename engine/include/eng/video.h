// Output: an H.264 MP4 through an ffmpeg pipe, PNG stills, and simple mono audio for narration.
#pragma once
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

// Mono audio as floats.
struct Audio {
  int rate = 22050;
  std::vector<float> samples;
  double duration() const { return (double)samples.size() / rate; }
  // 16-bit PCM WAV, mono or stereo (mixed down).
  static Audio loadWav(const std::string& path);
  // Add `src` starting at `at` seconds. Rates must match.
  void mix(const Audio& src, double at, float gain = 1);
  void resize(double seconds) { samples.resize((size_t)(seconds * rate), 0.f); }
  bool saveWav(const std::string& path) const;
};

}  // namespace eng
