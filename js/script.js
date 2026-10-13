// Narrative data: chapters, timed narration cues (English + Simplified Chinese),
// and the on-screen wording shared by the scenes.
// Times are in seconds on the master timeline. Scene code references cues by id,
// so retiming a cue here moves the matching animation with it. onScreen: a card
// shows this line verbatim, so the caption steps aside while it is up.

export const DURATION = 420;

export const CHAPTERS = [
  { n: 1, start: 0,   end: 45,  title: 'One Tool, Two Approaches',            zh: '一个工具，两种用法' },
  { n: 2, start: 45,  end: 110, title: 'Efficiency and Independent Thinking', zh: '效率与独立思考' },
  { n: 3, start: 110, end: 190, title: 'Different Tasks, Different Uses',     zh: '不同任务，不同用法' },
  { n: 4, start: 190, end: 250, title: 'Find the Decisive Problem',           zh: '找到决定性的问题' },
  { n: 5, start: 250, end: 310, title: 'Learning Across Time',                zh: '跨越时间的学习' },
  { n: 6, start: 310, end: 380, title: 'Test, Reflect, Revise',               zh: '检验、反思、修正' },
  { n: 7, start: 380, end: 420, title: 'Ask a Better Question',               zh: '提出更好的问题' },
];

export const CUES = [
  // 01 One Tool, Two Approaches
  { id: '1a', start: 1.0,  end: 4.8,  en: 'A dramatization: two fictional students, one shared AI tool.', zh: '情景演绎：两位虚构的学生，共用一个 AI 工具。' },
  { id: '1b', onScreen: true, start: 5.4,  end: 12.2, en: 'Their question: if students who use AI receive higher grades, does that prove AI improves learning?', zh: '他们的问题是：如果使用 AI 的学生成绩更高，这能否证明 AI 促进了学习？' },
  { id: '1c', start: 13.0, end: 19.6, en: 'Student A asks the AI for a finished answer. It arrives fast, fluent, and well organized.', zh: '学生 A 让 AI 直接给出完整答案。答案来得很快，流畅而有条理。' },
  { id: '1d', start: 20.4, end: 26.2, en: 'Student B writes a rough first attempt, then asks the AI to critique it.', zh: '学生 B 先写出一个粗略的初稿，再请 AI 对它提出批评。' },
  { id: '1e', start: 27.0, end: 31.8, en: 'Then the teacher asks a follow-up: walk me through your reasoning.', zh: '接着，老师追问：请讲讲你的推理过程。' },
  { id: '1f', start: 32.4, end: 35.4, en: 'Both hesitate, but at different points.', zh: '两人都卡住了，但卡在不同的地方。' },
  { id: '1g', start: 36.0, end: 40.0, en: 'A can’t explain why a correlation isn’t a cause.', zh: 'A 说不清为什么相关不等于因果。' },
  { id: '1h', start: 40.6, end: 44.6, en: 'B has a plausible claim, but thin evidence behind it.', zh: 'B 的观点说得通，但背后的证据很单薄。' },

  // 02 Efficiency and Independent Thinking
  { id: '2a', start: 45.8,  end: 49.6,  en: 'Today’s choices are habits in progress, not fixed traits.', zh: '今天的选择只是正在形成的习惯，而不是固定的性格。' },
  { id: '2b', start: 50.2,  end: 53.8,  en: 'Now the shared workspace splits into two paths.', zh: '现在，共享的工作台分成了两条路径。' },
  { id: '2c', start: 54.6,  end: 59.6,  en: 'On one path, the time the AI saves flows into more thinking:', zh: '在一条路径上，AI 节省下来的时间流向了更多的思考：' },
  { id: '2d', start: 59.8,  end: 65.0,  en: 'testing an example, comparing explanations, putting the idea into your own words.', zh: '检验一个例子，比较不同的解释，用自己的话把想法说出来。' },
  { id: '2e', start: 65.8,  end: 71.6,  en: 'On the other, the process stops at a polished answer. The saved time just disappears.', zh: '在另一条路径上，过程止步于一个漂亮的答案，省下的时间就这样消失了。' },
  { id: '2f', start: 72.4,  end: 76.2,  en: 'It’s tempting to file this under pros and cons.', zh: '人们很容易把这归结为一张利弊清单。' },
  { id: '2g', start: 76.6,  end: 80.6,  en: 'But efficiency and independent thinking aren’t two separate columns.', zh: '但效率与独立思考并不是互不相干的两栏。' },
  { id: '2h', start: 81.2,  end: 83.4,  en: 'They act on each other.', zh: '它们相互作用。' },
  { id: '2i', start: 83.8,  end: 89.6,  en: 'Saved time can feed reasoning, and reasoning is what lets you judge the AI’s output.', zh: '节省的时间可以滋养推理，而推理正是你判断 AI 输出好坏的依据。' },
  { id: '2j', start: 90.2,  end: 93.6,  en: 'Which way the influence runs depends on conditions.', zh: '影响朝哪个方向发展，取决于条件。' },
  { id: '2k', start: 94.2,  end: 101.4, en: 'When a task demands explanation and the teacher checks the process, saved time tends to become thinking time.', zh: '当任务要求解释、老师检查过程时，节省的时间往往会变成思考的时间。' },
  { id: '2l', start: 102.0, end: 109.4, en: 'When only the final product is graded, efficiency can quietly replace the thinking it was meant to support.', zh: '当只有最终成果被评分时，效率可能悄悄取代它本应支持的思考。' },

  // 03 Different Tasks, Different Uses
  { id: '3a', onScreen: true, start: 110.8, end: 114.0, en: 'Different tasks ask different things of a learner.', zh: '不同的任务，对学习者的要求也不同。' },
  { id: '3b', start: 114.6, end: 121.0, en: 'The first station is about understanding a concept, such as correlation versus causation.', zh: '第一站是理解一个概念，比如相关与因果的区别。' },
  { id: '3c', start: 121.4, end: 129.4, en: 'AI can offer examples and analogies. The learner still has to explain the idea, and notice where an analogy breaks.', zh: 'AI 可以提供例子和类比。但学习者仍要自己解释这个概念，并看出类比在哪里失效。' },
  { id: '3d', start: 130.0, end: 134.6, en: 'The second station is about solving a problem.', zh: '第二站是解决一个问题。' },
  { id: '3e', start: 135.0, end: 138.6, en: 'AI can give a hint or check a step.', zh: 'AI 可以给出提示，或检查某一步。' },
  { id: '3f', start: 139.0, end: 145.4, en: 'The learner makes the first attempt, verifies each step, and later solves a similar problem alone.', zh: '学习者要先自己尝试，逐步验证，之后再独立解决一个类似的问题。' },
  { id: '3g', start: 146.0, end: 149.4, en: 'The third station: developing an argument.', zh: '第三站：构建一个论证。' },
  { id: '3h', start: 149.8, end: 154.0, en: 'AI can raise counterarguments and question the evidence.', zh: 'AI 可以提出反驳，质疑证据。' },
  { id: '3i', start: 154.4, end: 160.8, en: 'But choosing the claim, weighing the evidence, and standing behind the conclusion remain the learner’s work.', zh: '但选择论点、权衡证据、为结论负责，仍然是学习者自己的工作。' },
  { id: '3j', start: 161.6, end: 165.6, en: 'So “Should I use AI?” is too vague a question.', zh: '所以，“我该不该用 AI？”这个问题太笼统了。' },
  { id: '3k', onScreen: true, start: 166.0, end: 171.6, en: 'A better one: for this task, what must I be able to do myself?', zh: '更好的问题是：对于这项任务，哪些事我必须自己能做？' },
  { id: '3l', start: 172.4, end: 177.2, en: 'Pause for a moment. Think of a task you’re working on now.', zh: '停下来想一想。想一项你正在做的任务。' },
  { id: '3m', start: 184.2, end: 189.7, en: 'The answer depends on the task, and on what you’ll need to show later.', zh: '答案取决于任务本身，也取决于你之后需要展示什么。' },

  // 04 Find the Decisive Problem
  { id: '4a', start: 190.8, end: 194.6, en: 'Now picture each student’s reasoning as a construction model.', zh: '现在，把每个学生的推理想象成一个建筑模型。' },
  { id: '4b', start: 195.0, end: 201.6, en: 'Both have plenty of small flaws: a vague sentence, a missing citation. They don’t matter equally.', zh: '两个模型都有不少小毛病：一句含糊的话，一处缺失的引用。它们的分量并不相同。' },
  { id: '4c', start: 202.2, end: 207.8, en: 'So we test them under load: the same follow-up question the teacher asked.', zh: '于是我们给模型加上负荷：就是老师提出的那个追问。' },
  { id: '4d', start: 208.4, end: 214.0, en: 'A’s model fails at the foundation, the step from higher grades to better learning.', zh: 'A 的模型在地基处断裂——从“成绩更高”到“学得更好”的那一步。' },
  { id: '4e', start: 214.4, end: 218.2, en: 'A needs to understand correlation and causation.', zh: 'A 需要弄懂相关与因果。' },
  { id: '4f', start: 218.8, end: 222.4, en: 'B’s model stands, but it sways.', zh: 'B 的模型没有倒，但在摇晃。' },
  { id: '4g', start: 222.8, end: 227.4, en: 'The evidence is thin, and other explanations go unanswered.', zh: '证据单薄，其他解释也没有得到回应。' },
  { id: '4h', start: 227.8, end: 234.2, en: 'Maybe stronger students use AI more. Maybe AI-allowed tasks are easier to score well on.', zh: '也许是成绩更好的学生更常使用 AI；也许允许使用 AI 的任务本来就更容易拿高分。' },
  { id: '4i', start: 235.0, end: 242.0, en: 'The decisive problem is the one that, once addressed, changes how the others look.', zh: '决定性的问题，是那个一旦解决、就会改变其他问题面貌的问题。' },
  { id: '4j', start: 242.6, end: 249.6, en: 'That priority comes from what we observed. Not every weak point is a principal contradiction.', zh: '这个优先次序来自我们的观察。并不是每个薄弱点都是主要矛盾。' },

  // 05 Learning Across Time
  { id: '5a', start: 250.8, end: 257.6, en: 'Learning unfolds over time. Three linked islands: today’s assignment, a later assessment, and an unfamiliar problem.', zh: '学习在时间中展开。三座相连的岛屿：今天的作业、之后的测评，以及一个陌生的问题。' },
  { id: '5b', start: 258.2, end: 263.4, en: 'A strong result on the first island says little about the other two.', zh: '在第一座岛上取得好成绩，并不能说明另外两座岛上的情况。' },
  { id: '5c', start: 264.0, end: 270.8, en: 'After the follow-up, A changes strategy: attempt first, then ask for a hint, not an answer.', zh: '经过老师的追问，A 改变了策略：先自己尝试，再向 AI 要提示，而不是要答案。' },
  { id: '5d', start: 271.4, end: 278.8, en: 'B, meanwhile, meets a convincing suggestion: a confident summary citing a study that seems to settle the question.', zh: '与此同时，B 遇到了一个很有说服力的建议：一段自信的总结，引用了一项似乎能一锤定音的研究。' },
  { id: '5e', start: 279.4, end: 286.0, en: 'B checks the source. The study doesn’t exist, and the claim treats a correlation as a cause.', zh: 'B 核查了来源。那项研究根本不存在，而且这个说法把相关当成了因果。' },
  { id: '5f', start: 286.6, end: 291.6, en: 'Good habits on one day don’t guarantee good judgment on the next.', zh: '某一天的好习惯，并不能保证下一次的判断同样可靠。' },
  { id: '5g', start: 292.4, end: 297.0, en: 'These lines are possible trajectories, not measurements.', zh: '这些线条是可能的轨迹，而不是测量数据。' },
  { id: '5h', start: 297.6, end: 304.0, en: 'Each can bend with the students’ next decisions, and with how the next task is designed.', zh: '每一条都可能随着学生接下来的决定、以及下一项任务的设计而改变方向。' },
  { id: '5i', start: 304.8, end: 309.2, en: 'Both students have changed, and neither is finished.', zh: '两位学生都发生了变化，而且变化还没有结束。' },

  // 06 Test, Reflect, Revise
  { id: '6a', start: 310.8, end: 317.4, en: 'How can either student tell whether they’ve actually learned? Build a cycle that produces evidence.', zh: '两位学生怎样才能知道自己是否真的学会了？建立一个能产生证据的循环。' },
  { id: '6b', start: 318.0, end: 320.6, en: 'Start with an independent attempt.', zh: '从独立尝试开始。' },
  { id: '6c', start: 320.8, end: 323.4, en: 'Ask for targeted help.', zh: '寻求有针对性的帮助。' },
  { id: '6d', start: 323.6, end: 326.2, en: 'Verify what the AI said.', zh: '核实 AI 所说的内容。' },
  { id: '6e', start: 326.4, end: 329.2, en: 'Try the idea on a new case.', zh: '在新的情境中运用这个想法。' },
  { id: '6f', start: 329.4, end: 331.2, en: 'Then revise.', zh: '然后修正。' },
  { id: '6g', start: 332.0, end: 336.4, en: 'Now both face a new example, with the AI switched off.', zh: '现在，两人面对一个新的例子，AI 已经关闭。' },
  { id: '6h', start: 337.0, end: 341.4, en: 'A explains why a correlation alone can’t settle a causal question.', zh: 'A 解释了为什么仅凭相关性无法回答因果问题。' },
  { id: '6i', start: 341.8, end: 347.6, en: 'B names two alternative explanations, and the evidence that would rule them out.', zh: 'B 指出了两种替代解释，以及能够排除它们的证据。' },
  { id: '6j', start: 348.2, end: 355.4, en: 'A week later, they revisit it. Some of it held; some had faded. That’s useful information too.', zh: '一周后，他们再回头看这道题。有些内容记住了，有些已经淡忘。这同样是有用的信息。' },
  { id: '6k', start: 355.8, end: 363.2, en: 'Delayed, independent, transfer tasks provide evidence of understanding: stronger than one good grade, but still evidence, not proof.', zh: '延迟的、独立的、迁移性的任务能提供理解的证据：比一次好成绩更有力，但仍然是证据，而不是证明。' },
  { id: '6l', start: 363.8, end: 372.2, en: 'Social practice is a broader philosophical idea: the whole of human activity in which ideas are tested against the world.', zh: '社会实践是一个更宽广的哲学概念：人类在改造世界的全部活动中检验自己的认识。' },
  { id: '6m', start: 372.8, end: 379.4, en: 'A classroom check is only a small, designed part of it.', zh: '课堂上的检验，只是其中经过设计的一小部分。' },

  // 07 Ask a Better Question
  { id: '7a', start: 380.8, end: 385.6, en: 'The two workspaces reconnect, and the opening question changes shape.', zh: '两个工作台重新连接，最初的问题也改变了形状。' },
  { id: '7b', start: 386.2, end: 391.4, en: 'Smarter or more dependent? That either-or gives way to four questions.', zh: '更聪明，还是更依赖？这种二选一让位于四个问题。' },
  { id: '7c', start: 392.0, end: 394.2, en: 'Under what conditions?', zh: '在什么条件下？' },
  { id: '7d', start: 394.6, end: 397.0, en: 'What is the central difficulty?', zh: '核心难点是什么？' },
  { id: '7e', start: 397.4, end: 399.6, en: 'What changes over time?', zh: '随着时间推移，什么在变化？' },
  { id: '7f', start: 400.0, end: 402.6, en: 'How can we test our judgment?', zh: '我们如何检验自己的判断？' },
  { id: '7g', start: 403.4, end: 406.8, en: 'A higher grade alone can’t answer them.', zh: '单凭一次更高的成绩，回答不了这些问题。' },
  { id: '7h', start: 407.4, end: 414.4, en: 'What can is a learner who makes decisions about the tool, and then checks whether those decisions worked.', zh: '能回答它们的，是一个对如何使用工具做出决定、并检验这些决定是否有效的学习者。' },
];

export const TITLE = {
  en: 'Does AI Make Us Smarter—or More Dependent?',
  zh: 'AI 让我们更聪明，还是更依赖？',
};

export function chapterAt(t) {
  for (let i = CHAPTERS.length - 1; i >= 0; i--) if (t >= CHAPTERS[i].start) return i;
  return 0;
}

export function cueAt(t) {
  for (let i = 0; i < CUES.length; i++) {
    if (t >= CUES[i].start && t < CUES[i].end) return i;
  }
  return -1;
}

// Cue times relative to a chapter's start: { '2c': { s, e }, ... }
export function localCues(chapterIndex) {
  const ch = CHAPTERS[chapterIndex];
  const out = {};
  for (const c of CUES) {
    if (c.start >= ch.start && c.start < ch.end) out[c.id] = { s: c.start - ch.start, e: c.end - ch.start };
  }
  return out;
}
