// The narration script: seven chapters of timed cues in Simplified Chinese and English.
// The film is made in one language at a time (Chinese by default): narration, captions and labels.
// Cue times are laid out from the length of each line in that language, so editing a line retimes it
// and everything after it. Scene animation refers to cues by id (c("3c").s), so it follows the
// script automatically.
#pragma once
#include <map>
#include <string>
#include <vector>

enum class Lang { Zh, En };

struct Cue {
  std::string id;
  int ch = 0;
  std::string en, zh;
  float s = 0, e = 0;    // on screen from s to e (seconds)
  float speech = 0;      // length of the recorded narration, if any
  // The line in the film's language.
  const std::string& text() const;
};

struct Chapter {
  int n = 0;
  std::string title, zh;
  float s = 0, e = 0;
  std::vector<const Cue*> cues;
  const std::string& name() const;
};

namespace script {
// The film's language. Set it before layout().
void setLang(Lang l);
Lang lang();
inline bool zh() { return lang() == Lang::Zh; }
// Pick the string for the film's language.
inline std::string tr(const std::string& en, const std::string& zh) { return lang() == Lang::Zh ? zh : en; }
// The language's code, used for narration folders: "zh" or "en".
const char* code();

constexpr float WPS = 2.6f;   // English narration pace the layout allows, in words per second
constexpr float CPS = 4.4f;   // Chinese narration pace, in characters per second
constexpr float GAP = 0.6f;   // silence between cues
constexpr float LEAD = 1.2f;  // quiet beat at the start of each chapter
constexpr float MIN = 3.4f;   // shortest slot for any cue

int words(const std::string& s);
// Chinese characters in a string (punctuation and Latin letters are not counted).
int hanzi(const std::string& s);
// Seconds a line needs at the layout's pace in the film's language.
float readingTime(const Cue& q);
// Lay out the film. `speech` gives the narration length of each cue in seconds; a cue's slot grows
// to fit its narration, and never shrinks below the pace above.
void layout(const std::map<std::string, float>& speech = {});
const std::vector<Chapter>& chapters();
const std::vector<Cue>& cues();
float duration();
// The cue on screen at time t, or null in a gap.
const Cue* cueAt(float t);
const Chapter& chapterAt(float t);
}  // namespace script

// A cue by id. Unknown ids stop the program with a message.
const Cue& c(const std::string& id);
