// The Emptying Bookshop: renders the film to an MP4 (or stills) with the C++/Vulkan engine.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "chapters/chapters.h"

using namespace film;
namespace fs = std::filesystem;

#ifndef FILM_DIR
#define FILM_DIR "."
#endif

namespace {

struct Options {
  std::string out;
  std::string outDir = "out/stills";
  bool zh = false;
  int width = 1920, height = 1080, fps = 30, samples = 4, crf = 18;
  std::string preset = "medium";
  float from = 0, to = -1;
  std::vector<float> stills;
  bool audio = true, check = false, cues = false, validation = false;
  std::string data = FILM_DIR;
};

void usage() {
  std::puts(
      "usage: emptying-bookshop [options]\n"
      "  --out FILE        output MP4 (default out/emptying-bookshop.mp4, or out/emptying-bookshop.zh.mp4 with --zh)\n"
      "  --zh              add Simplified Chinese subtitles and label lines\n"
      "  --size WxH        frame size (default 1920x1080)\n"
      "  --fps N           frame rate (default 30)\n"
      "  --from S --to S   render only part of the film, in seconds\n"
      "  --still T,T,...   write PNG stills at these times instead of a video (to --out-dir, default out/stills)\n"
      "  --no-audio        leave the narration out\n"
      "  --crf N           x264 quality (default 18; lower is better)\n"
      "  --preset NAME     x264 preset (default medium)\n"
      "  --samples N       multisampling (default 4)\n"
      "  --check           print the timing table and check the script, then exit\n"
      "  --cues            print each cue as id<TAB>English line (input for tools/narrate.py)\n"
      "  --data DIR        the film folder with fonts/ and narration/ (default: the source folder)\n"
      "  --validate        enable the Vulkan validation layer");
}

bool parse(int argc, char** argv, Options& o) {
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 >= argc) throw std::runtime_error(a + " needs a value");
      return argv[++i];
    };
    if (a == "--out") o.out = next();
    else if (a == "--out-dir") o.outDir = next();
    else if (a == "--zh") o.zh = true;
    else if (a == "--size") {
      std::string v = next();
      if (std::sscanf(v.c_str(), "%dx%d", &o.width, &o.height) != 2) throw std::runtime_error("--size wants WxH");
    } else if (a == "--fps") o.fps = std::stoi(next());
    else if (a == "--from") o.from = std::stof(next());
    else if (a == "--to") o.to = std::stof(next());
    else if (a == "--still") {
      std::stringstream ss(next());
      std::string tok;
      while (std::getline(ss, tok, ',')) o.stills.push_back(std::stof(tok));
    } else if (a == "--no-audio") o.audio = false;
    else if (a == "--crf") o.crf = std::stoi(next());
    else if (a == "--preset") o.preset = next();
    else if (a == "--samples") o.samples = std::stoi(next());
    else if (a == "--check") o.check = true;
    else if (a == "--cues") o.cues = true;
    else if (a == "--data") o.data = next();
    else if (a == "--validate") o.validation = true;
    else if (a == "-h" || a == "--help") { usage(); return false; }
    else throw std::runtime_error("unknown option " + a);
  }
  if (o.out.empty()) o.out = o.zh ? "out/emptying-bookshop.zh.mp4" : "out/emptying-bookshop.mp4";
  return true;
}

std::string fmt(float s) {
  char b[16];
  std::snprintf(b, sizeof b, "%d:%02d", (int)s / 60, (int)s % 60);
  return b;
}

// Narration lengths recorded by tools/narrate.py, one "id<TAB>seconds" per line.
std::map<std::string, float> readDurations(const fs::path& file) {
  std::map<std::string, float> m;
  std::ifstream in(file);
  std::string id;
  float s;
  while (in >> id >> s) m[id] = s;
  return m;
}

int check() {
  const float SPOKEN_WPS = 2.75f;  // a typical voice, for cues without a recording
  const int ZH_MAX = 48;           // longest Chinese line that still fits on two subtitle lines
  std::vector<std::string> problems;
  const auto& cues = script::cues();
  int total = 0;
  for (size_t i = 0; i < cues.size(); i++) {
    const Cue& q = cues[i];
    total += script::words(q.en);
    // Count characters, not bytes.
    int chars = 0;
    for (unsigned char ch : q.zh) chars += (ch & 0xC0) != 0x80;
    if (chars > ZH_MAX) problems.push_back(q.id + ": Chinese line has " + std::to_string(chars) + " characters (max " + std::to_string(ZH_MAX) + ")");
    float room = (i + 1 < cues.size() ? cues[i + 1].s : script::duration()) - q.s;
    float need = q.speech > 0 ? q.speech : script::words(q.en) / SPOKEN_WPS;
    if (need > room) problems.push_back(q.id + ": about " + std::to_string(need).substr(0, 4) + " s to speak but only " + std::to_string(room).substr(0, 4) + " s before the next line");
    if (i + 1 < cues.size() && cues[i + 1].s < q.e) problems.push_back(q.id + ": overlaps " + cues[i + 1].id);
  }
  float D = script::duration();
  std::printf("%zu chapters, %zu cues, %d words, %s (%.1f s)\n", script::chapters().size(), cues.size(), total, fmt(D).c_str(), D);
  std::printf("Average %.0f words per minute; layout pace %.1f words/s.\n", total / (D / 60), script::WPS);
  for (const Chapter& ch : script::chapters()) {
    int w = 0;
    for (const Cue* q : ch.cues) w += script::words(q->en);
    std::printf("  %d. %s–%s  %-30s %2zu cues %4d words\n", ch.n, fmt(ch.s).c_str(), fmt(ch.e).c_str(), ch.title.c_str(), ch.cues.size(), w);
  }
  int recorded = 0;
  for (const Cue& q : cues) recorded += q.speech > 0;
  std::printf("Narration recorded for %d of %zu cues.\n", recorded, cues.size());
  if (problems.empty()) {
    std::puts("\nNo problems found.");
    return 0;
  }
  std::printf("\n%zu problem(s):\n", problems.size());
  for (auto& p : problems) std::printf("  - %s\n", p.c_str());
  return 1;
}

}  // namespace

int main(int argc, char** argv) {
  Options o;
  try {
    if (!parse(argc, argv, o)) return 0;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "%s\n", e.what());
    usage();
    return 2;
  }
  const fs::path data = o.data;
  script::layout(readDurations(data / "narration" / "durations.tsv"));
  if (o.check) return check();
  if (o.cues) {
    for (const Cue& q : script::cues()) std::printf("%s\t%s\n", q.id.c_str(), q.en.c_str());
    return 0;
  }

  setFontFile(Font::Sans, (data / "fonts" / "Inter-Variable.ttf").string());
  setFontFile(Font::Serif, (data / "fonts" / "SourceSerif4-Variable.ttf").string());
  setFontFile(Font::SerifItalic, (data / "fonts" / "SourceSerif4-Italic-Variable.ttf").string());
  setFontFile(Font::CJK, (data / "fonts" / "NotoSansSC-Subset.otf").string());

  try {
    // Build the film.
    World* W = createWorld();
    W->resize(o.width, o.height);
    Labels L(*W, o.width, o.height, o.zh);
    Captions captions(o.width, o.height, o.zh);
    Ctx ctx{*W, L};
    std::vector<Update> mods = {buildC1(ctx), buildC2(ctx), buildC3(ctx), buildC4(ctx), buildC5(ctx), buildC6(ctx), buildC7(ctx)};
    // Chapter tag, top left, for the first seconds of every chapter (chapter 1 waits for the title card).
    for (const Chapter& ch : script::chapters()) {
      float start = ch.n == 1 ? c("1a").s + 0.4f : ch.s + 0.3f;
      L.add({.kind = Kind::Chapter, .screen = vec2{0.03f, 0}, .anchor = vec2{0, 0}, .stem = false,
             .chapter = std::make_tuple("Chapter " + std::to_string(ch.n), ch.title, ch.zh),
             .vis = [=](float t) { return win(t, start, start + 4.6f, 0.6f, 0.9f); }});
    }
    W->finalize();

    RendererOptions ro;
    ro.width = o.width;
    ro.height = o.height;
    ro.samples = o.samples;
    ro.validation = o.validation || std::getenv("ENG_VALIDATION");
    Renderer renderer(ro);
    std::fprintf(stderr, "Vulkan device: %s\n", renderer.deviceName().c_str());
    std::vector<uint8_t> frame((size_t)o.width * o.height * 4);
    const SafeArea safe = captions.safeArea();

    auto renderAt = [&](float t) {
      W->update(t);
      for (auto& m : mods) m(t);
      W->applyFx();
      renderer.render(W->scene, W->camera, W->post);
      std::memcpy(frame.data(), renderer.pixels(), frame.size());
      L.draw(frame.data(), t, safe);
      captions.draw(frame.data(), t);
    };

    if (!o.stills.empty()) {
      fs::create_directories(o.outDir);
      for (float t : o.stills) {
        renderAt(t);
        char name[64];
        std::snprintf(name, sizeof name, "t%06.2f%s.png", t, o.zh ? "-zh" : "");
        fs::path p = fs::path(o.outDir) / name;
        if (!savePng(p.string(), frame.data(), o.width, o.height)) throw std::runtime_error("could not write " + p.string());
        std::fprintf(stderr, "%s  (%d draws, %zu triangles)\n", p.string().c_str(), renderer.stats().draws, renderer.stats().triangles);
      }
      return 0;
    }

    // Video.
    const float D = script::duration();
    float to = o.to < 0 ? D : std::min(o.to, D);
    int f0 = (int)std::lround(o.from * o.fps), f1 = (int)std::lround(to * o.fps);
    fs::path outPath = o.out;
    if (outPath.has_parent_path()) fs::create_directories(outPath.parent_path());

    std::string audioPath;
    if (o.audio) {
      Audio mix;
      bool any = false;
      for (const Cue& q : script::cues()) {
        fs::path wav = data / "narration" / (q.id + ".wav");
        if (!fs::exists(wav)) continue;
        Audio a = Audio::loadWav(wav.string());
        if (!any) { mix.rate = a.rate; any = true; }
        mix.mix(a, q.s - o.from, 0.8f);  // Piper peaks at full scale; leave headroom for the AAC encoder
      }
      if (any) {
        mix.resize(to - o.from);
        audioPath = (outPath.parent_path() / (outPath.stem().string() + ".narration.wav")).string();
        mix.saveWav(audioPath);
      } else {
        std::fprintf(stderr, "No narration found in %s; the video will be silent. Run tools/narrate.py to record it.\n", (data / "narration").string().c_str());
      }
    }

    VideoWriter::Options vo;
    vo.path = outPath.string();
    vo.width = o.width;
    vo.height = o.height;
    vo.fps = o.fps;
    vo.crf = o.crf;
    vo.preset = o.preset;
    vo.audioPath = audioPath;
    VideoWriter writer(vo);
    auto start = std::chrono::steady_clock::now();
    for (int f = f0; f < f1; f++) {
      float t = (float)f / o.fps;
      renderAt(t);
      writer.write(frame.data());
      int done = f - f0 + 1;
      if (done % 30 == 0 || f + 1 == f1) {
        double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        double eta = el / done * (f1 - f - 1);
        std::fprintf(stderr, "\rframe %d/%d  t=%s  %.2f fps  eta %s   ", done, f1 - f0, fmt(t).c_str(), done / el, fmt((float)eta).c_str());
      }
    }
    std::fprintf(stderr, "\n");
    if (!writer.close()) throw std::runtime_error("ffmpeg reported an error");
    if (!audioPath.empty()) fs::remove(audioPath);
    std::fprintf(stderr, "wrote %s\n", outPath.string().c_str());
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 1;
  }
  return 0;
}
