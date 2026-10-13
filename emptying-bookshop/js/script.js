// The narration script: seven chapters of timed cues in English and Simplified Chinese.
// Cue times are laid out from word counts, so editing a line retimes it and everything after it.
// Scene animation refers to cues by id (c['3c'].s), so it follows the script automatically.

export const WPS = 2.6;   // narration pace the layout allows, in words per second
const GAP = 0.6;          // silence between cues
const LEAD = 1.2;         // quiet beat at the start of each chapter
const MIN = 3.4;          // shortest slot for any cue

const RAW = [
  {
    title: 'An Emptying Shop', zh: '日渐冷清的书店', tail: 1.2,
    cues: [
      { id: '1a', pre: 4.6, en: 'This is Jia, who runs the student second-hand bookshop on a university campus.',
        zh: '这是小嘉，在一所大学校园里经营学生二手书店。' },
      { id: '1b', en: 'It began years ago as a free swap shelf in a dormitory lounge, and grew into a proper shop.',
        zh: '几年前，它只是宿舍休息室里的一个免费换书架，后来才发展成一家正式的书店。' },
      { id: '1c', hold: 1.2, en: 'For years it was busy. This term, fewer students come through the door each week.',
        zh: '多年来，这里一直很热闹。可这个学期，上门的学生一周比一周少。' },
      { id: '1d', en: 'Jia has to act. But first comes a quieter question: how to think about the problem at all.',
        zh: '小嘉必须采取行动。但在此之前，还有一个更根本的问题：该怎样思考这个问题？' },
      { id: '1e', en: "Following Jia's choices, we'll see how a few Marxist principles become practical thinking skills.",
        zh: '跟随小嘉的选择，我们来看几条马克思主义基本原理，如何变成实用的思维能力。' },
    ],
  },
  {
    title: 'Three Quick Reactions', zh: '三种第一反应', tail: 0.8,
    cues: [
      { id: '2a', en: "Jia's first reactions are quick, and very common. Let's look at three of them.",
        zh: '小嘉的第一反应来得很快，也很常见。我们来看其中三种。' },
      { id: '2b', en: "First reaction: 'Nobody reads anymore.' One sweeping cause, so nothing can be done.",
        zh: '第一种反应：“现在没人读书了。”一个笼统的原因，于是什么也做不了。' },
      { id: '2c', en: 'That answer was assumed, not checked. And it looks at the shop in isolation, cut off from the campus around it.',
        zh: '这个答案是臆断出来的，而不是查证出来的。它把书店孤立起来看，割断了书店与周围校园的联系。' },
      { id: '2d', pre: 0.4, en: 'Second reaction: copy a fix that worked elsewhere. Another campus ran a half-price week, so Jia runs one too.',
        zh: '第二种反应：照搬别处的成功做法。另一所学校搞过“半价周”，小嘉也照着办一次。' },
      { id: '2e', en: "Prices fall, but the shop stays quiet. That fix answered another campus's conditions, not this one's.",
        zh: '价格降了，书店依旧冷清。那个办法针对的是别的学校的条件，而不是这里的条件。' },
      { id: '2f', en: "Lenin called a concrete analysis of a concrete situation 'the living soul of Marxism.'",
        zh: '列宁把“对具体情况作具体分析”称为“马克思主义的活的灵魂”。' },
      { id: '2g', pre: 0.4, en: "Third reaction: after one quiet week, give up. 'Nothing changed, so nothing will.'",
        zh: '第三种反应：冷清了一周，就打算放弃。“什么都没变，以后也不会变。”' },
      { id: '2h', en: 'This takes one snapshot for the whole story, as if the shop and its students were fixed in place.',
        zh: '这是用一张快照代替整个过程，好像书店和学生都是一成不变的。' },
      { id: '2i', pre: 0.4, en: 'Marxist philosophy calls this way of thinking metaphysical: seeing things as isolated, static, and one-sided.',
        zh: '马克思主义哲学把这种思维方式称为形而上学：用孤立、静止、片面的观点看问题。' },
      { id: '2j', en: 'Its alternative, dialectics, starts somewhere less dramatic: going out to look.',
        zh: '与之相对的辩证法，起点却很朴素：走出去，看一看。' },
    ],
  },
  {
    title: 'Go and Find Out', zh: '走出去调查', tail: 0.8,
    cues: [
      { id: '3a', pre: 0.6, en: "Mao Zedong put it bluntly: 'No investigation, no right to speak.'",
        zh: '毛泽东说得很直接：“没有调查，没有发言权。”' },
      { id: '3b', en: 'So Jia closes the shop for an afternoon and asks students what they actually do.',
        zh: '于是小嘉关店一个下午，去问问同学们实际上是怎么做的。' },
      { id: '3c', en: 'The first finding is about place. Last year the shop moved behind the library, off the main path.',
        zh: '第一个发现关于位置：去年书店搬到了图书馆后面，离开了主路。' },
      { id: '3d', en: 'The second is about time. The shop closes at five, just as most classes end.',
        zh: '第二个发现关于时间：书店五点关门，而大多数课正好在这时结束。' },
      { id: '3e', en: 'The third: many courses now post readings online, so fewer printed books change hands.',
        zh: '第三个发现：很多课程把阅读材料放到了网上，纸质书的买卖自然少了。' },
      { id: '3f', en: "And a fourth, which Jia never thought to ask about: there's nowhere to study together in the evening.",
        zh: '还有第四个，是小嘉事先根本没想到要问的：晚上找不到一起学习的地方。' },
      { id: '3g', pre: 0.8, en: 'None of these is inside the shop. Its quiet is tied to paths, timetables, courses, and habits.',
        zh: '这些原因都不在书店内部。书店的冷清，与道路、课表、课程和习惯联系在一起。' },
      { id: '3h', en: "That's universal connection. But connected doesn't mean equally important: some threads are thicker.",
        zh: '这就是普遍联系。但有联系，不等于同样重要：有些联系更粗、更强。' },
      { id: '3i', en: "Facts like these don't settle the question. They give Jia something real to think with.",
        zh: '这些事实还不能直接给出答案，但它们让小嘉的思考有了真实的依据。' },
    ],
  },
  {
    title: 'The Pull Inside the Problem', zh: '问题内部的矛盾', tail: 0.8,
    cues: [
      { id: '4a', pre: 0.4, en: 'Next, Jia looks for the tension inside the problem: what pulls the shop two ways?',
        zh: '接下来，小嘉寻找问题内部的张力：是什么在把书店往两个方向拉？' },
      { id: '4b', en: 'On one side, what the shop is: a daytime counter for buying and selling.',
        zh: '一方面，是书店现在的样子：一个白天营业、只管买卖的柜台。' },
      { id: '4c', en: 'On the other, what students now need: cheap materials, evening hours, and a place to work together.',
        zh: '另一方面，是学生现在的需要：便宜的资料、晚上的时间，以及一起学习的地方。' },
      { id: '4d', en: 'The two sides depend on each other: the shop exists for students, and students once relied on it.',
        zh: '双方相互依存：书店为学生而存在，学生也曾经依靠书店。' },
      { id: '4e', en: 'Yet they pull apart. Dialectics calls this a contradiction: a unity of opposites.',
        zh: '可它们又在相互背离。辩证法把这叫作矛盾：对立面的统一。' },
      { id: '4f', pre: 0.4, en: 'There are smaller problems too: a faded sign, an old website, prices. Each one is real.',
        zh: '还有一些小问题：褪色的招牌、过时的网站、价格。每一个都真实存在。' },
      { id: '4g', en: "But a brighter sign only brings people to a shop that still doesn't fit their day.",
        zh: '但招牌再亮，也只是把人引到一家仍然不合他们作息的书店。' },
      { id: '4h', en: 'This contradiction shapes the others, so under these conditions it is the principal one.',
        zh: '这个矛盾影响和制约着其他问题，所以在这些条件下，它是主要矛盾。' },
      { id: '4i', pre: 0.8, en: "And 'nobody reads anymore'? The move online is real, but it is a condition from outside.",
        zh: '那“现在没人读书了”呢？资料转到网上是真的，但它是外部条件。' },
      { id: '4j', en: 'External causes work through internal causes. What happens next depends on how the shop meets its own contradiction.',
        zh: '外因通过内因起作用。接下来会怎样，取决于书店如何解决自身的矛盾。' },
    ],
  },
  {
    title: 'Test, Accumulate, Revise', zh: '检验、积累、修正', tail: 0.8,
    cues: [
      { id: '5a', en: 'An analysis is still only an idea. Jia turns it into small tests.',
        zh: '分析再好，也还只是想法。小嘉把它变成几个小小的试验。' },
      { id: '5b', en: 'Test one: stay open until nine. Test two: a swap shelf, where a book costs another book.',
        zh: '试验一：营业到晚上九点。试验二：设一个换书架，用一本书换一本书。' },
      { id: '5c', en: "Test three: four borrowed tables by the window, and a sign that says 'Study here.'",
        zh: '试验三：在窗边摆上四张借来的桌子，挂一块“欢迎来学习”的牌子。' },
      { id: '5d', pre: 0.4, en: 'The first weeks look disappointing. Two regulars, then five. On the surface, little has changed.',
        zh: '头几周看上去令人失望。先是两个常客，后来是五个。表面上，变化不大。' },
      { id: '5e', en: 'But something is adding up: people who know the shop, and who bring their friends.',
        zh: '但有些东西正在积累：熟悉这家店的人，以及他们带来的朋友。' },
      { id: '5f', en: 'Then a study group starts meeting there every Tuesday. Other groups follow.',
        zh: '后来，一个学习小组每周二都来这里活动，其他小组也跟着来了。' },
      { id: '5g', pre: 0.5, hold: 0.8, en: 'At some point the shop becomes a different kind of place: not somewhere you pass through, but somewhere you meet.',
        zh: '到了某个时刻，书店变成了另一种地方：不再是路过的地方，而是相聚的地方。' },
      { id: '5h', en: 'Many small, quantitative changes have added up to a qualitative one.',
        zh: '许多细小的量变，积累成了质变。' },
      { id: '5i', pre: 0.8, en: 'Then a setback: the library complains about noise. Practice reveals something the analysis missed.',
        zh: '接着出现了挫折：图书馆投诉太吵。实践暴露出分析中遗漏的问题。' },
      { id: '5j', en: 'So Jia revises: quiet study by the window, a talk corner on the far side, and a timetable on the door.',
        zh: '于是小嘉作出修正：窗边安静自习，远侧设讨论角，门上贴出时间表。' },
      { id: '5k', pre: 0.4, en: "Mao summed up the cycle: 'practice, knowledge, again practice, and again knowledge,' each round at a higher level.",
        zh: '毛泽东概括过这个循环：“实践、认识、再实践、再认识”，每一轮都更进一步。' },
    ],
  },
  {
    title: 'Back, but Higher', zh: '回到起点，却更高一层', tail: 0.8,
    cues: [
      { id: '6a', pre: 0.6, en: "Step back, and the shop's whole history comes into view.",
        zh: '退后一步，书店的整个历史就展现在眼前。' },
      { id: '6b', en: 'First, a free swap shelf in a dorm lounge: shared and friendly, but disorganised. Books went missing.',
        zh: '最初，是宿舍休息室里的免费换书架：大家共享，气氛友好，却杂乱无序，书常常不见。' },
      { id: '6c', en: 'Then a proper shop: reliable, catalogued, with regular hours. It overcame the chaos, but lost the gathering.',
        zh: '后来，成了正式的书店：可靠、有目录、按时营业。它克服了混乱，却失去了相聚的氛围。' },
      { id: '6d', en: "Now, a swap-and-study room. Sharing and gathering return, and the shop's organisation stays.",
        zh: '现在，它是一个换书加自习的空间。共享与相聚回来了，书店的条理也保留了下来。' },
      { id: '6e', pre: 0.5, hold: 2.2, en: 'Not a straight line, and not a closed circle. From above it looks like a return. From the side, it climbs.',
        zh: '这不是一条直线，也不是一个封闭的圆。从上面看，像是回到了起点；从侧面看，它在上升。' },
      { id: '6f', en: 'This is the negation of the negation: each stage overcomes the last, while keeping what was valuable in it.',
        zh: '这就是否定之否定：每个阶段都克服前一阶段，同时保留其中有价值的东西。' },
    ],
  },
  {
    title: 'Your Own Bookshop', zh: '你自己的“书店”', tail: 7.5,
    cues: [
      { id: '7a', pre: 0.6, en: "Look back at Jia's turning points.",
        zh: '回顾一下小嘉的几个转折点。' },
      { id: '7b', en: 'Jia went and looked before judging, traced the connections, and asked which ones mattered most.',
        zh: '小嘉先调查再下判断，理清了各种联系，并追问哪些联系最重要。' },
      { id: '7c', en: 'Jia found the principal contradiction, tested ideas in practice, watched small changes add up, and expected the process to keep moving.',
        zh: '小嘉找到了主要矛盾，在实践中检验想法，看到细小的变化不断积累，也预料到事情还会继续发展。' },
      { id: '7d', en: "None of this needs a bookshop. It's a way into any problem that refuses to stay simple.",
        zh: '这一切并不只适用于书店。面对任何复杂的问题，都可以这样入手。' },
      { id: '7e', pre: 1.0, hold: 1.0, en: "So: what's the emptying bookshop in your own life?",
        zh: '那么，你生活中那家“日渐冷清的书店”是什么？' },
      { id: '7f', pre: 0.4, en: 'This week, pick one problem. Before you judge it, ask three people who live with it.',
        zh: '这周，选一个问题。在下判断之前，先问问三个身处其中的人。' },
    ],
  },
];

export const words = (s) => s.trim().split(/\s+/).filter(Boolean).length;

export const chapters = [];
export const cues = [];
export const c = {};

let t = 0;
RAW.forEach((ch, i) => {
  const chapter = { n: i + 1, title: ch.title, zh: ch.zh, s: t, cues: [] };
  t += LEAD;
  ch.cues.forEach((q, j) => {
    t += q.pre || 0;
    const slot = Math.max(MIN, words(q.en) / WPS + 0.4) + (q.hold || 0);
    const cue = { id: q.id, ch: i + 1, en: q.en, zh: q.zh, s: round(t), e: round(t + slot) };
    t = cue.e + (j < ch.cues.length - 1 ? GAP : 0);
    chapter.cues.push(cue);
    cues.push(cue);
    c[q.id] = cue;
  });
  t += ch.tail;
  chapter.e = round(t);
  t = chapter.e;
  chapters.push(chapter);
});

export const DURATION = round(t);

function round(x) { return Math.round(x * 100) / 100; }

// The cue on screen at time t (or null in a gap).
export function cueAt(time) {
  for (const q of cues) if (time >= q.s && time < q.e) return q;
  return null;
}

export function chapterAt(time) {
  for (let i = chapters.length - 1; i >= 0; i--) if (time >= chapters[i].s) return chapters[i];
  return chapters[0];
}
