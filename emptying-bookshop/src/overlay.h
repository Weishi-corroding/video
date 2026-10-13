// The overlay drawn over each rendered frame: labels anchored to 3D points or to the screen, the
// chapter tags, title cards and panels, and the captions. Sizes follow the original player's CSS at a
// 1280 × 720 design size and scale with the output.
#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "world.h"

namespace film {

enum class Kind { Tag, Fact, Flaw, Principle, Shop, Need, Note, Card, Title, Panel, Chapter };

// What a label points at: a world point, a node (followed as it moves), or a function of time.
struct Anchor {
  std::optional<vec3> point;
  Node* node = nullptr;
  std::function<vec3(float)> fn;
  Anchor() = default;
  Anchor(vec3 p) : point(p) {}
  Anchor(Node* n) : node(n) {}
  Anchor(const Figure& f) : node(f.root) {}
  Anchor(std::function<vec3(float)> f) : fn(std::move(f)) {}
  bool empty() const { return !point && !node && !fn; }
};

using Pair = std::pair<std::string, std::string>;  // English, Chinese

struct PanelItem {
  std::string en, p, zh;  // p: a highlighted tail in the accent colour
};
struct PanelSpec {
  Pair h4;
  std::vector<PanelItem> items;
  std::vector<Pair> chips;
  std::optional<Pair> foot;
  // Opacity of list item i at time t (items can be dimmed until they are reached).
  std::function<float(int, float)> itemOpacity;
};

struct LabelSpec {
  Kind kind = Kind::Tag;
  std::string cls;  // extra classes: small plain stamp strike quote gold big snapframe teal coral summary
  Anchor at;
  float lift = 0;                  // world y added to the anchor
  std::optional<vec2> screen;      // fractions of the safe area instead of a 3D anchor
  std::optional<vec2> anchor;      // which point of the box sits on the anchor (fractions of the box)
  vec2 offset{0, 0};               // design pixels
  std::optional<bool> stem;        // a pointer line under the box (3D labels by default)
  std::string en, zh;
  std::optional<Pair> kicker;      // small caps line above the text
  bool noKicker = false;           // suppress the kind's default kicker
  std::optional<Pair> sub;         // title: a subtitle
  std::optional<Pair> attr;        // card: an attribution line
  std::optional<PanelSpec> panel;  // panel: heading, list, chips, footnote
  std::optional<std::tuple<std::string, std::string, std::string>> chapter;  // chapter tag: number line, title, Chinese title
  std::function<float(float)> vis;  // opacity over time
};

struct SafeArea {
  float top, left, right, bottom;  // output pixels
};

class Labels {
 public:
  Labels(World& W, int width, int height, bool zh);
  ~Labels();
  struct Label;
  Label* add(LabelSpec spec);
  void set(Label* l, const std::string& en, const std::string& zh);
  // Place every label for time t and draw them onto an RGBA8 frame.
  void draw(uint8_t* frame, float t, const SafeArea& safe);
  float scale() const { return S_; }

 private:
  World& W_;
  int w_, h_;
  bool zh_;
  float S_;
  std::vector<std::unique_ptr<Label>> items_;
};

class Captions {
 public:
  Captions(int width, int height, bool zh);
  // Draw the caption for the cue on screen at time t.
  void draw(uint8_t* frame, float t);
  // The safe area for labels, which stays clear of the caption band.
  SafeArea safeArea() const;

 private:
  int w_, h_;
  bool zh_;
  float S_;
  struct Cache;
  std::shared_ptr<Cache> cache_;
};

}  // namespace film
