# Video Review and Animation Improvement Recommendations

**Project:** *Does AI Make Us Smarter—or More Dependent?*  
**Review date:** October 13, 2026  
**Scope:** Narrative, educational clarity, philosophical framing, visual design, animation, and playback.

The film has a coherent story and a recognizable miniature-world aesthetic. Its strongest choices are the changing behavior of both students, the consistent learning question, and the explicit distinction between fictional illustrations and measured evidence. The next revision should improve reading comfort, make the reasoning visible, and give the animation clearer visual priorities.

This document combines the earlier review with further animation beautification suggestions. It proposes changes; it does not implement them.

**Review basis and limitations**

- The complete bilingual script and relevant player and chapter code were inspected. Representative frames from all seven chapters were checked in the browser, including bilingual layouts and a 390 × 844 viewport.
- The narration checker reported **73 cues, 827 English words, 118 words per minute averaged across seven minutes, and zero problems**. These are script statistics, not measurements of actual speech duration or comprehension.
- The review browser had no available English system voice. Voice quality, pronunciation, and continuous audio synchronization were therefore not assessed by listening.
- Layout findings below were observed in rendered frames. Playback and packet-direction findings are based on code inspection. Artistic proposals are recommendations, not claims that a particular technique has already been tested.

**1. Highest-priority changes**

1. **Make every important label readable.** At **4:16**, captions cover the “Today’s assignment” label. At **6:44**, the four question cards overlap even in the sampled landscape view; on a phone-sized viewport, the outer cards also extend beyond the screen. Narrower views additionally clip the opening critique label and the chapter-five AI suggestion. Reserve space for captions and controls, constrain labels to safe screen bounds, and use a separate narrow-screen layout.
2. **Separate two different inference problems.** At **3:28–3:38**, the narration moves from “higher grades mean better learning” to “correlation versus causation.” Distinguish whether AI caused a grade difference from whether grades demonstrate lasting independent understanding. Both questions matter, but they need different evidence.
3. **Show an actual transfer task.** At **5:32–5:48**, the narration announces that the students can reason through a new example, while the visual shows generic labels such as “rival 1” and “rival 2.” Show the new problem, their explanations, and the evidence they would seek.
4. **Strengthen the course-concept connections.** If submitted for a Marxist philosophy course, explain the relationship between the example and the concepts being used, especially principal contradiction and social practice.
5. **Stabilize the presentation before adding effects.** Fix layout and motion-direction issues, then refine lighting, materials, camera work, and character action. For a final seven-minute video, use a fixed narration track.

**2. Narrative and educational recommendations**

- **Clarify the central reasoning chain — 3:28–3:38.** Display “AI use → higher grades → lasting independent ability,” with a separate question mark on each arrow. Ask whether other factors could explain the grades, then whether the assessment measures understanding that lasts. Connect the second question to the later independent and transfer tests. Relevant source: `js/script.js`, cues `4d` and `4e`.
- **Demonstrate learning — 5:32–5:48.** Introduce a clearly fictional example: “Students who join a study group receive higher grades.” A explains why this alone does not establish causation. B proposes prior knowledge and study effort as alternative explanations and identifies what comparisons would help investigate them. Do not present a simple observational comparison as conclusive proof. Relevant sources: `js/script.js`, cues `6g`–`6i`; `js/chapters/c6.js`.
- **Make B’s development visible — 4:31–4:52.** B currently succeeds in identifying a fabricated study, immediately followed by a warning that good habits do not guarantee good judgment. Add a brief moment in which B is tempted to accept the suggestion before checking it. Alternatively, change the commentary to emphasize that good judgment must be renewed on each occasion. This better matches the behavior already shown.
- **Tighten the middle.** Consider reallocating approximately 10–15 seconds from the three general task descriptions in chapter three to the concrete example in chapter six. Preserve the quiet reflection interval around **2:57–3:04**. This is an editorial target, not a required cut.
- **End with an action.** Keep the four reflective questions, then finish with a practical instruction: “On your next assignment, attempt it yourself, ask AI for feedback, then close the tool and explain your reasoning.” Shorten the surrounding conclusion as needed to preserve the seven-minute target.
- **Align the two languages.** At **4:18**, the English “says little” is more cautious than the Chinese wording. Prefer a shared meaning equivalent to “is insufficient to judge the later outcomes.” Review other revisions in both languages together.

**3. Philosophical framing for coursework**

- **Principal contradiction — 3:55–4:09.** The construction test effectively illustrates how to identify a consequential weakness. It does not yet identify the opposing sides of a contradiction or explain why that contradiction governs other problems under the stated conditions. Either keep the claim at “identify the decisive problem,” or add that missing analysis. Avoid treating the largest mistake as automatically equivalent to the principal contradiction. This recommendation follows the distinction drawn in [*On Contradiction*](https://news.12371.cn/2017/08/29/ARTI1504001209326173.shtml), which discusses the leading or determining role of the principal contradiction within a complex process.
- **Social practice — 6:03–6:19.** The English explanation emphasizes testing ideas against the world; the Chinese explanation also refers to transforming the world. Align their scope. Show a concrete extension into investigation, an experiment, or a real task with consequences and feedback. An expanding ring of buildings provides scale, but does not by itself explain the philosophical relationship.
- **Avoid adding terminology without explanation.** A short sentence connecting a depicted action to a concept will be more useful than a new floating label. Preserve the existing caution that classroom checks are limited evidence, and that the students and trajectories are illustrative.

**4. Animation beautification and visual direction**

The recommended direction is a carefully lit miniature learning world: sculptural ivory objects, a restrained teal environment, clear amber and mint signals, and deliberate camera movement. Preserve the handmade model quality while giving each shot one dominant action.

1. **Create a stronger lighting hierarchy.** Some sampled scenes contain bright mint rings and glowing surfaces that compete with the reasoning objects. Reduce the intensity of persistent background glows and reserve the brightest accents for the current action. Keep a gentle warm key light and a subtle cool edge light so ivory objects retain readable shape. The project already uses bloom and shadows; adjust them before introducing another effect. Relevant sources: `js/main.js`, `js/world.js`, `js/kit.js`.

2. **Improve contact and material separation.** Distinguish paper, furniture, structural models, glass, and light-emitting elements through modest differences in roughness and color. Keep paper matte, glass restrained, and the plinth slightly darker than the models. Strengthen soft contact cues beneath feet, blocks, and desks so objects feel grounded. If testing ambient occlusion, evaluate its visual benefit and performance before adopting it. Avoid noisy textures that obscure the abstract teaching style.

3. **Compose shots around the current idea.** Use a wide view to establish a chapter, a medium view for comparison, and a close view for a decisive detail. In chapter four, frame the failing joint and the thin support clearly before returning to the two models. In chapter six, give the new example enough screen space to read. Set the camera’s target with the label and subtitle areas in mind, rather than fitting only the 3D objects.

4. **Design portrait composition explicitly.** The current narrow-screen approach mainly moves the camera farther away, making the models smaller without solving the label layout. Use alternate camera targets and screen-space text arrangements for portrait views. Present A and B sequentially when a side-by-side comparison cannot remain legible. The conclusion can show one question at a time, followed by a compact summary. Relevant sources: `js/world.js`, `js/labels.js`, `css/style.css`.

5. **Give typography a clear hierarchy.** Separate chapter titles, current teaching points, short object labels, and captions through size and placement. Replace long floating paragraphs with a stable explanation panel and short labels near the relevant object. Use a consistent translucent backing behind both language lines. Reduce decorative borders and shadows where several cards appear together. Prioritize reading at normal playback speed rather than fitting all text into a still frame.

6. **Introduce motion that communicates weight and intent.** The shared toolkit already provides rising and overshooting scale entrances. Use a calmer settle for heavy platforms and structural blocks, a lighter arrival for information packets, and a short emphasis pulse for verified reasoning. Let a crack open under load, a support brace slide into position, and the model visibly settle afterward. Vary entrances by meaning instead of giving every object the same bounce.

7. **Refine camera timing.** Let viewers register an object before its explanation begins, then hold the composition through the key sentence. Smooth the approach and departure of camera moves, and check chapter boundaries for distracting speed changes. Use the existing inter-chapter travel where spatial continuity helps; consider a shorter, quieter transition where the journey competes with a new idea. Avoid continuous decorative camera movement during dense bilingual reading.

8. **Extend the students’ existing gestures.** Keep the abstract figures, but make gaze and posture support the story: attention shifts to a draft, a head turns toward a source, a hesitation precedes a revision, and both students inspect the result. The opening already includes head tilts; build on these with a few purposeful actions. Give both students comparable expressive detail so one is not visually coded as inherently better.

9. **Make visual transformations explain the argument.** A draft can gain annotations, a highlighted claim can connect to supporting evidence, and a repaired joint can restore a broken reasoning chain. Reuse a small number of recognizable motifs across chapters. These transformations should let the viewer understand what changed even before reading the caption.

10. **Preserve the color meaning.** Amber represents AI output; mint represents the learner’s reasoning and checking. In chapter one, the AI’s critique packets and critique label currently use mint. Keep returned AI material amber, then introduce mint when the learner checks, selects, or revises it. Use motion, shape, and wording as additional cues so the distinction does not depend on color alone.

11. **Simplify the background when the foreground becomes complex.** Reduce peripheral particles, bright world-ring segments, and decorative motion during dense explanations. Preserve depth with restrained fog and lighting. If particle placement is revised, use a fixed seed so review frames remain consistent across reloads; master-time animation should remain deterministic.

12. **Give the ending a clean visual resolution.** After the four questions, consolidate attention on the learner’s decision and its evaluation. Fade secondary labels before the end card arrives. Hold the final message long enough to read comfortably in both languages, using time reclaimed elsewhere if necessary. Avoid ending on another crowded arrangement of explanatory cards.

**5. Chapter-specific visual priorities**

- **Chapter 1, 0:00–0:45:** Clarify the request and response directions; use amber for all incoming AI feedback; show B examining the feedback before a mint revision appears. Keep speech and critique labels within safe bounds.
- **Chapter 2, 0:45–1:50:** Make the active route the brightest element. Dim the inactive path and background rings. Give each condition weight a readable arrival and a clear relationship to the changing balance.
- **Chapter 3, 1:50–3:10:** Give each station a distinct silhouette and reveal its explanation in two beats: what AI offers, then what the learner must do. Keep inactive stations visually quiet and preserve the reflection interval.
- **Chapter 4, 3:10–4:10:** Emphasize the sequence of load, failure, diagnosis, repair, and stabilization. Use close framing and subtle settling motion to make the structural metaphor convincing. Keep minor flaws visible without competing for attention.
- **Chapter 5, 4:10–5:10:** Move island labels clear of captions. Place the time arrow in its intended lower band. For B’s source check, show a specific change from a confident claim to an unresolved or invalid source, rather than relying on a new label alone.
- **Chapter 6, 5:10–6:20:** Introduce the five cycle stages progressively, then simplify the scene for the concrete transfer example. During the wider-world reveal, emphasize a few understandable activities instead of many equally weighted miniature props.
- **Chapter 7, 6:20–7:00:** Highlight one question at a time, then arrange the summary in a stable screen-space layout. Clear that summary before the final action and end card.

**6. Implementation findings and delivery recommendations**

- **AI response direction:** In `js/chapters/c1.js`, the curves run from the AI panel to the laptops, but the answer and critique animations sample them with reversed progress. Use forward progress for responses and reverse progress for requests. Verify the complete departure, travel, and landing sequence around **0:15–0:18** and **0:24–0:26**.
- **Narration after seeking:** In `js/main.js`, seeking cancels speech and marks a mid-cue destination as already spoken. Keyboard seeking in `js/ui.js` does not restart that cue, while pointer dragging pauses and resumes playback. Define one consistent behavior for both interactions. Verify with a browser that has an English voice.
- **Time-arrow placement:** At **4:16**, the time arrow appears against the top edge. The `.ov.bottom-band` rule adds a bottom offset while retaining `.lbl`’s `top: 0`. Clear the inherited top positioning when positioning this overlay from the bottom.
- **Recorded narration:** For a final video, use a fixed English audio track and align the cues and animation to it. The current speech-synthesis fallback can wait up to six extra seconds per cue and restarts the current sentence after resuming. Keep it as an optional web fallback if useful, but do not assume its wall-clock runtime is exactly seven minutes.
- **Performance:** Profile representative scenes before adding post-processing or more geometry. Keep a lower-cost visual option if necessary. Performance has not been benchmarked in this review, so no frame-rate target is claimed as achieved.

**7. Suggested implementation order and acceptance checks**

1. Fix label collisions, edge clipping, caption overlap, the time-arrow placement, and response direction.
2. Revise the reasoning distinction, transfer example, and philosophical connections; keep the English and Chinese synchronized.
3. Refine lighting, material separation, camera framing, and motion, starting with one representative scene before applying the treatment across all chapters.
4. Add or finalize the narration track, retime the sequence to the intended duration, and complete playback and visual verification.

After implementation:

- Run `node tools/check-script.mjs` after script changes and resolve all reported issues.
- Recheck **0:26, 1:35, 2:05, 3:35, 4:16, 4:42, 5:44, 6:10, and 6:44**, with English-only and bilingual captions. Use URL options such as `/?t=95&still&zh=1` for reproducible frames.
- Check at least 1280 × 720, a larger desktop view, and 390 × 844. No essential label should overlap another label, sit beneath captions, or extend beyond the viewport.
- Inspect both paused and playing states: auto-hiding controls change the available caption area.
- Verify play/pause, keyboard and pointer seeking, chapter navigation, playback speed, captions, narration, fullscreen, and resizing; check the browser console for errors.
- Seek backward and forward repeatedly. Every object, label, and effect should return to the correct state without accumulated drift. Derive motion from master time and reference narration cue IDs when timing chapter actions.
- Compare revised frames with the current version. The active concept should be easier to locate, the relevant text easier to read, and the motion easier to interpret.

The first implementation pass should deliver three visible improvements: readable layouts, a clearly demonstrated learning example, and more selective lighting and motion.
