#include "script.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <sstream>

namespace {

struct RawCue { const char* id; float pre, hold; const char* en; const char* zh; };
struct RawChapter { const char* title; const char* zh; float tail; std::vector<RawCue> cues; };

const std::vector<RawChapter> RAW = {
    {"An Emptying Shop", "日渐冷清的书店", 1.2f, {
      {"1a", 4.6f, 0.0f,
       "This is Jia, who runs the student second-hand bookshop on a university campus.",
       "这是小嘉，在一所大学的校园里，经营着一家学生二手书店。"},
      {"1b", 0.0f, 0.0f,
       "It began years ago as a free swap shelf in a dormitory lounge, and grew into a proper shop.",
       "几年前，它只是宿舍休息室里的一个免费换书架，后来才发展成一家正式的书店。"},
      {"1c", 0.0f, 1.2f,
       "For years it was busy. This term, fewer students come through the door each week.",
       "多年来，这里一直很热闹。可这个学期，上门的学生一周比一周少。"},
      {"1d", 0.0f, 0.0f,
       "Jia has to act. But first comes a quieter question: how to think about the problem at all.",
       "小嘉必须采取行动。但在此之前，还有一个更根本的问题：该怎样思考这个问题？"},
      {"1e", 0.0f, 0.0f,
       "Following Jia's choices, we'll see how a few Marxist principles become practical thinking skills.",
       "跟随小嘉的选择，我们来看几条马克思主义基本原理，如何变成实用的思维能力。"},
    }},
    {"Three Quick Reactions", "三种第一反应", 0.8f, {
      {"2a", 0.0f, 0.0f,
       "Jia's first reactions are quick, and very common. Let's look at three of them.",
       "小嘉的第一反应来得很快，也很常见。我们来看其中三种。"},
      {"2b", 0.0f, 0.0f,
       "First reaction: 'Nobody reads anymore.' One sweeping cause, so nothing can be done.",
       "第一种反应：“现在没人读书了。”一个笼统的原因，于是什么也做不了。"},
      {"2c", 0.0f, 0.0f,
       "That answer was assumed, not checked. And it looks at the shop in isolation, cut off from the campus around it.",
       "这个答案是臆断出来的，而不是查证出来的。它把书店孤立起来看，割断了书店与周围校园的联系。"},
      {"2d", 0.4f, 0.0f,
       "Second reaction: copy a fix that worked elsewhere. Another campus ran a half-price week, so Jia runs one too.",
       "第二种反应：照搬别处的成功做法。另一所学校搞过“半价周”，小嘉也照着办一次。"},
      {"2e", 0.0f, 0.0f,
       "Prices fall, but the shop stays quiet. That fix answered another campus's conditions, not this one's.",
       "价格降了，书店依旧冷清。那个办法针对的是别的学校的条件，而不是这里的条件。"},
      {"2f", 0.0f, 0.0f,
       "Lenin called a concrete analysis of a concrete situation 'the living soul of Marxism.'",
       "列宁把“对具体情况作具体分析”称为“马克思主义的活的灵魂”。"},
      {"2g", 0.4f, 0.0f,
       "Third reaction: after one quiet week, give up. 'Nothing changed, so nothing will.'",
       "第三种反应：冷清了一周，就打算放弃。“什么都没变，以后也不会变。”"},
      {"2h", 0.0f, 0.0f,
       "This takes one snapshot for the whole story, as if the shop and its students were fixed in place.",
       "这是用一张快照代替整个过程，好像书店和学生都是一成不变的。"},
      {"2i", 0.4f, 0.0f,
       "Marxist philosophy calls this way of thinking metaphysical: seeing things as isolated, static, and one-sided.",
       "马克思主义哲学把这种思维方式称为形而上学：用孤立、静止、片面的观点看问题。"},
      {"2j", 0.0f, 0.0f,
       "Its alternative, dialectics, starts somewhere less dramatic: going out to look.",
       "与之相对的辩证法，起点却很朴素：走出去，看一看。"},
    }},
    {"Go and Find Out", "走出去调查", 0.8f, {
      {"3a", 0.6f, 0.0f,
       "Mao Zedong put it bluntly: 'No investigation, no right to speak.'",
       "毛泽东说得很直接：“没有调查，没有发言权。”"},
      {"3b", 0.0f, 0.0f,
       "So Jia closes the shop for an afternoon and asks students what they actually do.",
       "于是小嘉关店一个下午，去问问同学们实际上是怎么做的。"},
      {"3c", 0.0f, 0.0f,
       "The first finding is about place. Last year the shop moved behind the library, off the main path.",
       "第一个发现关于位置：去年书店搬到了图书馆后面，离开了主路。"},
      {"3d", 0.0f, 0.0f,
       "The second is about time. The shop closes at five, just as most classes end.",
       "第二个发现关于时间：书店五点关门，而大多数课正好在这时结束。"},
      {"3e", 0.0f, 0.0f,
       "The third: many courses now post readings online, so fewer printed books change hands.",
       "第三个发现：很多课程把阅读材料放到了网上，纸质书的买卖自然少了。"},
      {"3f", 0.0f, 0.0f,
       "And a fourth, which Jia never thought to ask about: there's nowhere to study together in the evening.",
       "还有第四个，是小嘉事先根本没想到要问的：晚上找不到一起学习的地方。"},
      {"3g", 0.8f, 0.0f,
       "None of these is inside the shop. Its quiet is tied to paths, timetables, courses, and habits.",
       "这些原因都不在书店内部。书店的冷清，与道路、课表、课程和习惯联系在一起。"},
      {"3h", 0.0f, 0.0f,
       "That's universal connection. But connected doesn't mean equally important: some threads are thicker.",
       "这就是普遍联系。但有联系，不等于同样重要：有些联系更粗、更强。"},
      {"3i", 0.0f, 0.0f,
       "Facts like these don't settle the question. They give Jia something real to think with.",
       "这些事实还不能直接给出答案，但它们让小嘉的思考有了真实的依据。"},
    }},
    {"The Pull Inside the Problem", "问题内部的矛盾", 0.8f, {
      {"4a", 0.4f, 0.0f,
       "Next, Jia looks for the tension inside the problem: what pulls the shop two ways?",
       "接下来，小嘉寻找问题内部的张力：是什么在把书店往两个方向拉？"},
      {"4b", 0.0f, 0.0f,
       "On one side, what the shop is: a daytime counter for buying and selling.",
       "一方面，是书店现在的样子：一个白天营业、只管买卖的柜台。"},
      {"4c", 0.0f, 0.0f,
       "On the other, what students now need: cheap materials, evening hours, and a place to work together.",
       "另一方面，是学生现在的需要：便宜的资料、晚上的时间，以及一起学习的地方。"},
      {"4d", 0.0f, 0.0f,
       "The two sides depend on each other: the shop exists for students, and students once relied on it.",
       "双方相互依存：书店为学生而存在，学生也曾经依靠书店。"},
      {"4e", 0.0f, 0.0f,
       "Yet they pull apart. Dialectics calls this a contradiction: a unity of opposites.",
       "可它们又在相互背离。辩证法把这叫作矛盾：对立面的统一。"},
      {"4f", 0.4f, 0.0f,
       "There are smaller problems too: a faded sign, an old website, prices. Each one is real.",
       "还有一些小问题：褪色的招牌、过时的网站、价格。每一个都真实存在。"},
      {"4g", 0.0f, 0.0f,
       "But a brighter sign only brings people to a shop that still doesn't fit their day.",
       "但招牌再亮，也只是把人引到一家仍然不合他们作息的书店。"},
      {"4h", 0.0f, 0.0f,
       "This contradiction shapes the others, so under these conditions it is the principal one.",
       "这个矛盾影响和制约着其他问题，所以在这些条件下，它是主要矛盾。"},
      {"4i", 0.8f, 0.0f,
       "And 'nobody reads anymore'? The move online is real, but it is a condition from outside.",
       "那“现在没人读书了”呢？资料转到网上是真的，但它是外部条件。"},
      {"4j", 0.0f, 0.0f,
       "External causes work through internal causes. What happens next depends on how the shop meets its own contradiction.",
       "外因通过内因起作用。接下来会怎样，取决于书店如何解决自身的矛盾。"},
    }},
    {"Test, Accumulate, Revise", "检验、积累、修正", 0.8f, {
      {"5a", 0.0f, 0.0f,
       "An analysis is still only an idea. Jia turns it into small tests.",
       "分析再好，也还只是想法。小嘉把它变成几个小小的试验。"},
      {"5b", 0.0f, 0.0f,
       "Test one: stay open until nine. Test two: a swap shelf, where a book costs another book.",
       "试验一：营业到晚上九点。试验二：设一个换书架，用一本书换一本书。"},
      {"5c", 0.0f, 0.0f,
       "Test three: four borrowed tables by the window, and a sign that says 'Study here.'",
       "试验三：在窗边摆上四张借来的桌子，挂一块“欢迎来学习”的牌子。"},
      {"5d", 0.4f, 0.0f,
       "The first weeks look disappointing. Two regulars, then five. On the surface, little has changed.",
       "头几周看上去令人失望。先是两个常客，后来是五个。表面上，变化不大。"},
      {"5e", 0.0f, 0.0f,
       "But something is adding up: people who know the shop, and who bring their friends.",
       "但有些东西正在积累：熟悉这家店的人，以及他们带来的朋友。"},
      {"5f", 0.0f, 0.0f,
       "Then a study group starts meeting there every Tuesday. Other groups follow.",
       "后来，一个学习小组每周二都来这里活动，其他小组也跟着来了。"},
      {"5g", 0.5f, 0.8f,
       "At some point the shop becomes a different kind of place: not somewhere you pass through, but somewhere you meet.",
       "到了某个时刻，书店变成了另一种地方：不再是路过的地方，而是相聚的地方。"},
      {"5h", 0.0f, 0.0f,
       "Many small, quantitative changes have added up to a qualitative one.",
       "许多细小的量变，积累成了质变。"},
      {"5i", 0.8f, 0.0f,
       "Then a setback: the library complains about noise. Practice reveals something the analysis missed.",
       "接着出现了挫折：图书馆投诉太吵。实践暴露出分析中遗漏的问题。"},
      {"5j", 0.0f, 0.0f,
       "So Jia revises: quiet study by the window, a talk corner on the far side, and a timetable on the door.",
       "于是小嘉作出修正：窗边安静自习，远侧设讨论角，门上贴出时间表。"},
      {"5k", 0.4f, 0.0f,
       "Mao summed up the cycle: 'practice, knowledge, again practice, and again knowledge,' each round at a higher level.",
       "毛泽东概括过这个循环：“实践、认识、再实践、再认识”，每一轮都更进一步。"},
    }},
    {"Back, but Higher", "回到起点，却更高一层", 0.8f, {
      {"6a", 0.6f, 0.0f,
       "Step back, and the shop's whole history comes into view.",
       "退后一步，书店的整个历史就展现在眼前。"},
      {"6b", 0.0f, 0.0f,
       "First, a free swap shelf in a dorm lounge: shared and friendly, but disorganised. Books went missing.",
       "最初，是宿舍休息室里的免费换书架：大家共享，气氛友好，却杂乱无序，书常常不见。"},
      {"6c", 0.0f, 0.0f,
       "Then a proper shop: reliable, catalogued, with regular hours. It overcame the chaos, but lost the gathering.",
       "后来，成了正式的书店：可靠、有目录、按时营业。它克服了混乱，却失去了相聚的氛围。"},
      {"6d", 0.0f, 0.0f,
       "Now, a swap-and-study room. Sharing and gathering return, and the shop's organisation stays.",
       "现在，它是一个换书加自习的空间。共享与相聚回来了，书店的条理也保留了下来。"},
      {"6e", 0.5f, 2.2f,
       "Not a straight line, and not a closed circle. From above it looks like a return. From the side, it climbs.",
       "这不是一条直线，也不是一个封闭的圆。从上面看，像是回到了起点；从侧面看，它在上升。"},
      {"6f", 0.0f, 0.0f,
       "This is the negation of the negation: each stage overcomes the last, while keeping what was valuable in it.",
       "这就是否定之否定：每个阶段都克服前一阶段，同时保留其中有价值的东西。"},
    }},
    {"Your Own Bookshop", "你自己的“书店”", 7.5f, {
      {"7a", 0.6f, 0.0f,
       "Look back at Jia's turning points.",
       "回顾一下小嘉的几个转折点。"},
      {"7b", 0.0f, 0.0f,
       "Jia went and looked before judging, traced the connections, and asked which ones mattered most.",
       "小嘉先调查再下判断，理清了各种联系，并追问哪些联系最重要。"},
      {"7c", 0.0f, 0.0f,
       "Jia found the principal contradiction, tested ideas in practice, watched small changes add up, and expected the process to keep moving.",
       "小嘉找到了主要矛盾，在实践中检验想法，看到细小的变化不断积累，也预料到事情还会继续发展。"},
      {"7d", 0.0f, 0.0f,
       "None of this needs a bookshop. It's a way into any problem that refuses to stay simple.",
       "这一切并不只适用于书店。面对任何复杂的问题，都可以这样入手。"},
      {"7e", 1.0f, 1.0f,
       "So: what's the emptying bookshop in your own life?",
       "那么，你生活中那家“日渐冷清的书店”是什么？"},
      {"7f", 0.4f, 0.0f,
       "This week, pick one problem. Before you judge it, ask three people who live with it.",
       "这周，选一个问题。在下判断之前，先问问三个身处其中的人。"},
    }},
};

std::vector<Chapter> gChapters;
std::vector<Cue> gCues;
float gDuration = 0;
Lang gLang = Lang::Zh;

float round2(float x) { return std::round(x * 100) / 100; }

}  // namespace

const std::string& Cue::text() const { return script::lang() == Lang::Zh ? zh : en; }
const std::string& Chapter::name() const { return script::lang() == Lang::Zh ? zh : title; }

namespace script {

void setLang(Lang l) { gLang = l; }
Lang lang() { return gLang; }
const char* code() { return gLang == Lang::Zh ? "zh" : "en"; }

int hanzi(const std::string& s) {
  // Decode UTF-8 and count code points in the CJK Unified Ideographs block.
  int n = 0;
  for (size_t i = 0; i < s.size();) {
    unsigned char c = s[i];
    uint32_t cp = c;
    int len = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
    if (len > 1) {
      cp = c & (0xFF >> (len + 1));
      for (int k = 1; k < len && i + k < s.size(); k++) cp = (cp << 6) | (s[i + k] & 0x3F);
    }
    if (cp >= 0x4E00 && cp <= 0x9FFF) n++;
    i += len;
  }
  return n;
}

float readingTime(const Cue& q) { return gLang == Lang::Zh ? hanzi(q.zh) / CPS : words(q.en) / WPS; }

int words(const std::string& s) {
  std::istringstream in(s);
  std::string w;
  int n = 0;
  while (in >> w) n++;
  return n;
}

void layout(const std::map<std::string, float>& speech) {
  gChapters.clear();
  gCues.clear();
  size_t total = 0;
  for (const auto& ch : RAW) total += ch.cues.size();
  gCues.reserve(total);  // chapters keep pointers into this vector
  float t = 0;
  for (size_t i = 0; i < RAW.size(); i++) {
    const RawChapter& rc = RAW[i];
    Chapter chapter;
    chapter.n = (int)i + 1;
    chapter.title = rc.title;
    chapter.zh = rc.zh;
    chapter.s = t;
    t += LEAD;
    for (size_t j = 0; j < rc.cues.size(); j++) {
      const RawCue& q = rc.cues[j];
      t += q.pre;
      Cue cue;
      cue.id = q.id;
      cue.ch = (int)i + 1;
      cue.en = q.en;
      cue.zh = q.zh;
      auto it = speech.find(q.id);
      cue.speech = it == speech.end() ? 0 : it->second;
      float slot = std::max<float>({MIN, readingTime(cue) + 0.4f, cue.speech + 0.4f}) + q.hold;
      cue.s = round2(t);
      cue.e = round2(t + slot);
      t = cue.e + (j + 1 < rc.cues.size() ? GAP : 0);
      gCues.push_back(cue);
      chapter.cues.push_back(&gCues.back());
    }
    t += rc.tail;
    chapter.e = round2(t);
    t = chapter.e;
    gChapters.push_back(chapter);
  }
  gDuration = round2(t);
}

const std::vector<Chapter>& chapters() { return gChapters; }
const std::vector<Cue>& cues() { return gCues; }
float duration() { return gDuration; }

const Cue* cueAt(float time) {
  for (const Cue& q : gCues)
    if (time >= q.s && time < q.e) return &q;
  return nullptr;
}

const Chapter& chapterAt(float time) {
  for (int i = (int)gChapters.size() - 1; i >= 0; i--)
    if (time >= gChapters[i].s) return gChapters[i];
  return gChapters[0];
}

}  // namespace script

const Cue& c(const std::string& id) {
  for (const Cue& q : script::cues())
    if (q.id == id) return q;
  std::fprintf(stderr, "unknown cue '%s'\n", id.c_str());
  std::exit(2);
}
