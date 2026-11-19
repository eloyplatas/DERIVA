from figures import main_panel, beads_panel, signal_flow

FLOW = dict(
    **{"in": "IN", "in_sub": "mono sum", "gate": "GATE", "gate_sub": "normal / sens", "rec_sub": "group 1 / 2 / all",
       "comp_sub": "compressor", "lim": "LIMITER", "lim_sub": "tape-like", "mem": "MEMORY 44.7 s",
       "mem_sub": "prime-length lines · REVERSE · 1/2 speed", "blur_sub": "line mixing", "filt": "FILTERS",
       "fb_sub": "x suppressor", "feedback": "feedback", "drift_sub": "pan LFO + chaos", "beads_sub": "WET position",
       "mix_sub": "dry / memory", "dry": "dry signal", "loop": "LOOP return",
       "caption": "Signal flow. Everything written to memory passes through the limiter; the dry signal goes straight to MIX. "
                  "The BEADS stage sits on the memory output (WET) or feeds the loop (LOOP, dashed)."})

TOC = [
    ("intro", "Welcome"), ("history", "A short history"), ("concept", "How Deriva works"),
    ("install", "Installation"), ("quick", "Quick start"), ("tour", "Panel tour"),
    ("algorithms", "The twelve algorithms"), ("controls", "Controls in detail"),
    ("memory", "Working with the memory"), ("beads", "The BEADS stage"), ("midi", "MIDI control"),
    ("recipes", "Recipes"), ("technical", "Technical notes"), ("cosmos", "Deriva and the COSMOS"),
    ("faq", "Troubleshooting and FAQ"), ("credits", "Credits and licences"), ("params", "Appendix: parameter list"),
]

BODY = f"""
<h1 id="intro"><span class="num">01</span> Welcome</h1>
<p class="lead">Deriva is a memory that drifts. Everything you play is recorded into 44.7 seconds of sound memory. The memory is split into delay lines whose lengths are different prime numbers. Because those lengths never share a common rhythm, the repetitions slide against each other forever, so the texture keeps changing without ever settling into a loop.</p>
<p>The plugin follows the behaviour of the main firmware of the SOMA Laboratory COSMOS, the "drifting memory station" designed by Vlad Kreimer. Deriva has its own name, code and interface, and adds things a pedal cannot do: it works at any sample rate, follows the host tempo, responds to MIDI, can store its memory inside your project, and has an optional granular stage built on the open-source DSP of Mutable Instruments Beads.</p>
<p>Deriva works in two ways:</p>
<ul>
<li><strong>As an instrument for real-time playing:</strong> guitar, synthesizer, voice or a whole mix go in, and you sculpt the memory with the controls and footswitches while you play over it.</li>
<li><strong>As a studio effect:</strong> long evolving tails, infinite washes, broken granular echoes and lo-fi memories for any track.</li>
</ul>
<div class="tip"><b>In this manual</b>Chapters 2 and 3 tell the story and explain the idea behind the instrument. Chapters 4 to 6 get you running. Chapters 7 to 11 describe every control. Chapter 12 has ready-to-use recipes. The last chapters cover technical details, how Deriva compares with the hardware, and the licences.</div>

<h1 id="history"><span class="num">02</span> A short history</h1>
<h2>Tape that remembers</h2>
<p>The idea behind Deriva is older than any digital delay. In the early 1960s, composers working with tape discovered that two reel-to-reel recorders placed side by side could build music on their own. The first recorder records, the tape travels to the playback head of the second, and the signal is sent back to be recorded again. Each pass adds a new layer while the older ones slowly fade. Terry Riley and Pauline Oliveros used this "Time-Lag Accumulator" in the San Francisco tape music scene. Riley's 1963 piece <em>Music for The Gift</em> fed a jazz quartet into such a loop.</p>
<h2>Fripp &amp; Eno</h2>
<p>On 8 September 1972, in his home studio in London, Brian Eno set up two modified Revox A77 tape recorders in this feedback arrangement and invited Robert Fripp to play guitar through them. The result, <em>The Heavenly Music Corporation</em>, opened the album <em>(No Pussyfooting)</em>, released in November 1973. Fripp kept refining the technique, later with digital delays, and it became known as <strong>Frippertronics</strong>: long, slowly decaying layers built live by a single performer.</p>
<h2>SOMA Laboratory and the COSMOS</h2>
<p>SOMA Laboratory is the workshop of Vlad Kreimer, known for "organismic" instruments such as the LYRA-8 drone synthesizer and the PULSAR-23 drum machine. In 2021 SOMA announced the COSMOS, a large floor unit described as a drifting memory station designed to work with meditative states through music. Its manual names the Fripp &amp; Eno system as its inspiration. The COSMOS replaces two tape decks with a digital memory of 44.7 seconds, split into delay lines whose lengths are large prime numbers and modulated by an asynchronous LFO and a slow chaotic signal. A large twelve-position switch selects two-delay, four-delay, giant-reverb and granular algorithms. Alternative firmware added rhythmic looping, pitch and a quad mode for two units.</p>
<h2>Mutable Instruments Beads</h2>
<p>Mutable Instruments, founded by Émilie Gillet, made some of the most influential Eurorack modules of the 2010s. They were all released as open source. Its granular "texture synthesizer" Clouds became a modular classic. In February 2021 Gillet released its successor, <strong>Beads</strong>, redesigned from the ground up with a crisper sound, randomisable parameters, four recording qualities and a tempo-synced delay mode. Beads was the last module Gillet designed, and Mutable Instruments closed in December 2022. Its DSP code remains available under the MIT licence, which is what makes Deriva's BEADS stage possible.</p>
<div class="timeline">
<div class="ev"><span class="yr">1963</span>Terry Riley uses a two-recorder feedback loop in <em>Music for The Gift</em>.</div>
<div class="ev"><span class="yr">1972</span>Eno and Fripp record <em>The Heavenly Music Corporation</em> with two Revox A77s.</div>
<div class="ev"><span class="yr">1973</span><em>(No Pussyfooting)</em> is released; "Frippertronics" is born.</div>
<div class="ev"><span class="yr">2021</span>Mutable Instruments releases Beads (February). SOMA Laboratory announces the COSMOS (spring).</div>
<div class="ev"><span class="yr">2022</span>Mutable Instruments closes; its code stays open.</div>
<div class="ev"><span class="yr">2026</span>Deriva brings the drifting memory and Beads together in one plugin.</div>
</div>

<h1 id="concept"><span class="num">03</span> How Deriva works</h1>
<h2>One memory, many prime-length lines</h2>
<p>Deriva owns a single block of memory that holds 44.7 seconds of sound. Each algorithm divides that block into two, four or eight delay lines. Every line behaves like a loop of tape with one head that plays back what is on the tape and records the new mix in the same place. When a line comes round, you hear what was recorded one length earlier.</p>
<p>All line lengths are <strong>different prime numbers of samples</strong>. A sound recorded into two lines at once comes back from each at slightly different moments. On every pass the gap between the copies changes, and because the lengths share no common divisor, the pattern never repeats. This is the "drift": no new material is added, yet the music keeps changing.</p>
<h2>Three sources of movement</h2>
<ul>
<li><strong>Prime offsets:</strong> the lines slowly slide against each other.</li>
<li><strong>DRIFT:</strong> an asynchronous LFO for each line moves the line across the stereo field. Its speed is pushed around by a slow chaotic signal (a Lorenz attractor).</li>
<li><strong>BLUR:</strong> cross-feeds the lines into each other, so the copies gradually merge into one blurred body.</li>
</ul>
<h2>The signal flow</h2>
{signal_flow(FLOW)}
<p>The input passes through the IN gain, an optional mono sum, a noise gate and the DRIVE saturation. RECORD decides which lines receive it. The new sound is added to the feedback, goes through the compressor if one is active, then through the tape-like limiter, and is written into memory. What comes out of the memory is mixed between lines (BLUR), filtered, scaled by FB and the suppressor, and fed back. The memory output is spread across the stereo field by DRIFT and mixed with the dry signal by MIX.</p>
<h2>The orbits</h2>
<p>Around the algorithm selector, Deriva draws one ring for each delay line. The small bars show what is stored along the line and the bright dot is its head. With prime lengths, the heads run at slightly different speeds, so you can literally watch them drift apart. The most recently written part of each ring glows in violet.</p>

<h1 id="install"><span class="num">04</span> Installation</h1>
<p>Download the zip for your system from the <strong>Releases</strong> page of the DERIVA repository and copy the folder <code>Deriva.vst3</code> to the VST3 folder of your system:</p>
<table><tr><th>System</th><th>Folder</th><th>Requirements</th></tr>
<tr><td>Linux</td><td><code>~/.vst3/</code> or <code>/usr/lib/vst3/</code></td><td>x86_64, glibc 2.35 or newer (Ubuntu 22.04+, Debian 12+, Fedora 35+, Arch), fontconfig and freetype.</td></tr>
<tr><td>Windows</td><td><code>C:\\Program Files\\Common Files\\VST3\\</code></td><td>Windows 10 or 11, 64-bit. No extra runtime needed.</td></tr>
<tr><td>macOS</td><td><code>~/Library/Audio/Plug-Ins/VST3/</code></td><td>macOS 10.13 or newer. Universal binary for Apple Silicon and Intel.</td></tr></table>
<p>Then rescan plugins in your DAW. In REAPER: <em>Options › Preferences › Plug-ins › VST › Re-scan</em>. Deriva appears as an effect from <em>Eloy Platas</em> in the Delay and Reverb categories, with a stereo output and a mono or stereo input. It also accepts MIDI (chapter 11).</p>
<div class="note"><b>macOS</b>The plugin is not notarised by Apple. If macOS says it is damaged or cannot be opened, run this once in Terminal and rescan:<br/><code>xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Deriva.vst3</code></div>

<h1 id="quick"><span class="num">05</span> Quick start</h1>
<ol>
<li><strong>Insert Deriva</strong> on a track with a guitar, synth or voice, or on an FX send.</li>
<li><strong>Set IN</strong> so the green LED next to it moves with your playing and only rarely turns red.</li>
<li><strong>Choose an algorithm</strong> with the big selector. The default, 2 DELAYS · M, repeats every 9.5 seconds.</li>
<li><strong>Play a phrase.</strong> RECORD is on by default (red LEDs). The phrase returns after the line length and starts to drift.</li>
<li><strong>Shape it.</strong> Raise DRIFT for slow stereo movement and BLUR to melt the repetitions together. Keep FB just below the 1.0 mark so the oldest layers fade while new ones arrive.</li>
<li><strong>Play over it.</strong> Turn RECORD off to solo over the memory without adding to it, or keep recording to build layers.</li>
<li><strong>Clean up.</strong> Hold ERASE to erase under the heads, or shift-click ERASE (or the ERASE ALL chip) to clear everything.</li>
</ol>
<div class="tip"><b>Tip</b>With FB above 1.0 the memory grows until the limiter holds it, like tape saturating. This is a valid sound, not a fault: watch the red LED of the level meter.</div>

<h1 id="tour"><span class="num">06</span> Panel tour</h1>
{main_panel("The main panel. Numbers refer to the table below.")}
<table><tr><th>#</th><th>Element</th><th>What it does</th></tr>
<tr><td>1</td><td>IN</td><td>Input gain, ±24 dB. The LED shows input level (green) and A/D overload (red).</td></tr>
<tr><td>2</td><td>BLUR</td><td>Cross-feedback between lines (delays) or grain position noise (granular).</td></tr>
<tr><td>3</td><td>DRIFT</td><td>Chaotic LFO movement: stereo position of the lines, or grain position.</td></tr>
<tr><td>4</td><td>DRIVE</td><td>Gentle saturation of the sound entering the memory.</td></tr>
<tr><td>5</td><td>Selector and orbits</td><td>Twelve algorithms in four families. The rings show each line's content and head.</td></tr>
<tr><td>6</td><td>SUP | COM</td><td>Left: suppressor. Centre: off. Right: compressor.</td></tr>
<tr><td>7</td><td>FB</td><td>Feedback from 0 to 1.15, with a mark at unity (1.0).</td></tr>
<tr><td>8</td><td>MIX</td><td>Balance between dry signal and memory; both at full level in the centre.</td></tr>
<tr><td>9</td><td>OUT</td><td>Output level. The green LED is the power light.</td></tr>
<tr><td>10–11</td><td>HPF, LPF</td><td>Filters inside the feedback loop, each with three cutoffs.</td></tr>
<tr><td>12</td><td>Loop meter</td><td>Level inside the memory; red when the limiter is working.</td></tr>
<tr><td>13</td><td>ERASE</td><td>Hold to erase under the heads; shift-click to erase all.</td></tr>
<tr><td>14</td><td>REVERSE</td><td>Plays the memory backwards, like turning the tape round.</td></tr>
<tr><td>15</td><td>RECORD</td><td>Recording on/off, target groups, and half speed with a long press.</td></tr>
<tr><td>16</td><td>BEADS</td><td>Opens the BEADS panel and switches the granular stage on.</td></tr>
<tr><td>17</td><td>Option chips</td><td>MONO IN, GATE SENS, 1/2 SPEED, SAVE MEMORY, ERASE ALL.</td></tr>
<tr><td>18</td><td>Algorithm caption</td><td>Current algorithm, number of lines and length of the longest line.</td></tr>
</table>
<p>Hover over any knob to read its exact value in place of its name, and pause over any control for a short explanation. Double-click a knob to return it to its default. The window can be resized from the corner; it keeps its proportions.</p>

<h1 id="algorithms"><span class="num">07</span> The twelve algorithms</h1>
<p>The selector has twelve positions grouped in four families. Within each family, the three dots grow with the size of the memory. You can click a dot, drag the pointer or use the mouse wheel. When you change algorithm, the memory is <strong>not erased</strong>: the new algorithm takes over the same material and starts processing it its own way. A short fade-out and fade-in, plus tiny fades at the splice points, keep the change click-free, so switching becomes a performance gesture.</p>
<table><tr><th>Pos.</th><th>Algorithm</th><th>Lines</th><th>Size</th><th>Character</th></tr>
<tr><td>1–3</td><td>2 DELAYS · S / M / L</td><td>2 (L and R)</td><td>2.5 / 9.5 / 22 s</td><td>Classic Frippertronics. Two lines whose repeats drift apart, one per side.</td></tr>
<tr><td>4–6</td><td>4 DELAYS · S / M / L</td><td>4</td><td>2.5 / 8.5 / 11.5 s</td><td>Denser and more polyrhythmic; four drifting copies spread across the stereo field.</td></tr>
<tr><td>7–9</td><td>REVERB · HUGE / SUPER / INSANE</td><td>8</td><td>longest line 1.1 / 2.6 / 5.3 s</td><td>A hall kilometres wide. Early reflections arrive as separate echoes and merge very slowly into a diffuse cloud.</td></tr>
<tr><td>10–12</td><td>GRAIN · S / M / L</td><td>2 (+1 central)</td><td>grains of 45 / 140 / 380 ms</td><td>Asynchronous granular stereo delay. The large variant adds a central granular line.</td></tr>
</table>
<div class="two"><div><figure><img src="IMG/dial_reverb.png"/><figcaption>REVERB · SUPER: eight rings, one per line of the feedback network.</figcaption></figure></div>
<div><figure><img src="IMG/dial_grain.png"/><figcaption>GRAIN · M: two rings; grains read the memory behind each head.</figcaption></figure></div></div>
<h2>2 DELAYS</h2>
<p>Line A records the left input and plays on the left; line B records the right input and plays on the right. B is about 2.7 % shorter than A, so a phrase recorded in both comes back as two copies that move apart on every pass. BLUR rotates sound between the lines; at full BLUR each line receives half of the other. Half speed is available.</p>
<h2>4 DELAYS</h2>
<p>Four lines (two per input) with lengths spread over about 10 %, panned hard left, hard right and half-way. BLUR mixes them in pairs and then across pairs, until at full BLUR each line hears all the others equally. Half speed is available.</p>
<h2>REVERB</h2>
<p>An eight-line feedback delay network with a Householder matrix. This matrix spreads energy gently, so the first reflections stay audible as individual echoes. Lossless allpass diffusers inside each line make the echoes melt a little more on every pass. FB sets the decay: at 1.0 the space sustains forever. BLUR and DRIFT have no effect here, as on the hardware. Unlike the hardware, the tail never drifts out of tune.</p>
<h2>GRAIN</h2>
<p>Each line is written like a delay, but instead of a single playback head, short grains with smooth windows are read from behind the head at irregular moments. Each grain gets its own stereo position. BLUR adds noise to where grains are taken from; DRIFT moves that position with the chaotic LFO. The feedback carries the granulated sound back into the memory, so every repetition is granulated again.</p>

<h1 id="controls"><span class="num">08</span> Controls in detail</h1>
<h2>Knobs</h2>
<table><tr><th>Control</th><th>Range</th><th>Description</th></tr>
<tr><td>IN</td><td>−24 … +24 dB</td><td>Input gain before everything else. Aim for a green LED with occasional red only on peaks.</td></tr>
<tr><td>BLUR</td><td>0 … 100 %</td><td>Delays: orthogonal cross-feedback between lines. It conserves energy, so FB 1.0 still holds the level. Granular: random jitter of the grain position. Reverb: no effect.</td></tr>
<tr><td>DRIFT</td><td>0 … 100 %</td><td>Delays: depth of the per-line stereo LFOs (periods around 30–50 s, rates bent by a Lorenz attractor). At 0 the lines stay at their home positions. Granular: LFO movement of the grain position. Reverb: no effect.</td></tr>
<tr><td>DRIVE</td><td>0 … 100 %</td><td>Soft tanh saturation of the sound entering the memory, with antialiasing. It does not accumulate on every pass.</td></tr>
<tr><td>SUP | COM</td><td>SUP 100 % … OFF … COM 100 %</td><td>Left half: <strong>suppressor</strong>. The louder you play, the more the existing memory is turned down, so new phrases replace old ones. Right half: <strong>compressor</strong> on the sound written to memory, driven by memory plus input. Soft sounds grow and loud ones are held back, for an even, sustained memory.</td></tr>
<tr><td>FB</td><td>0 … 1.15</td><td>Feedback. Below 1.0 the memory fades; at 1.0 it holds; above 1.0 it grows until the tape-like limiter holds it. A tick marks 1.0. Just below it is the sweet spot for layering.</td></tr>
<tr><td>MIX</td><td>dry 100 / 0 … 0 / 100 memory</td><td>In the centre, both dry and memory are at full level, as on the hardware. Fully clockwise: memory only (send/return use).</td></tr>
<tr><td>OUT</td><td>−inf … +12 dB</td><td>Final output level.</td></tr>
</table>
<h2>Footswitches</h2>
<table><tr><th>Switch</th><th>Click</th><th>Right-click / wheel</th><th>Other</th></tr>
<tr><td>HPF</td><td>Filter on/off</td><td>Cutoff MIN 60 Hz · MED 150 Hz · MAX 400 Hz</td><td>Inside the loop: each pass loses more bass.</td></tr>
<tr><td>LPF</td><td>Filter on/off</td><td>Cutoff MIN 1.5 kHz · MED 4 kHz · MAX 9 kHz</td><td>MIN is the darkest. Each pass gets darker, like old tape.</td></tr>
<tr><td>ERASE</td><td>Hold to erase</td><td>—</td><td>Shift-click: erase all. In delays you hear the erasure immediately; in reverb and granular it shows on the next pass.</td></tr>
<tr><td>REVERSE</td><td>On/off</td><td>—</td><td>Reverses the memory and everything recorded afterwards.</td></tr>
<tr><td>RECORD</td><td>Recording on/off</td><td>Target: GROUP 1 · GROUP 2 · ALL</td><td>Hold 2 s: half speed (delays only).</td></tr>
</table>
<p>The two LEDs above HPF, LPF and RECORD show the selected option: left = first, right = second, both = third. They are bright when the function is on and dim when it is off. During half speed the RECORD LEDs blink.</p>
<h2>Option chips</h2>
<table><tr><th>Chip</th><th>Function</th></tr>
<tr><td>BEADS</td><td>Opens the BEADS panel below and switches the granular stage on (chapter 10). Click again to close and bypass.</td></tr>
<tr><td>MONO IN</td><td>Sums left and right inputs and sends the result to both groups. Useful to treat two mono sources as one.</td></tr>
<tr><td>GATE SENS</td><td>Lowers the noise gate threshold by 20 dB (from −60 to −80 dBFS) so soft attacks are not cut.</td></tr>
<tr><td>1/2 SPEED</td><td>Same as a long press on RECORD.</td></tr>
<tr><td>SAVE MEMORY</td><td>Stores the memory content inside your project (chapter 9).</td></tr>
<tr><td>ERASE ALL</td><td>Clears the whole memory with a short fade.</td></tr>
</table>
<h2>Meters</h2>
<p>The five LEDs under the selector show the level <em>inside</em> the memory: two green (from −36 and −24 dBFS), two yellow (−12 and −6 dBFS) and a red one that lights when the limiter is shaping the sound. Even with a constant red LED nothing breaks: you hear soft tape-like saturation, never digital clipping.</p>

<h1 id="memory"><span class="num">09</span> Working with the memory</h1>
<h2>Recording groups</h2>
<p>The left input feeds group 1 and the right input feeds group 2. In 2 DELAYS, group 1 is line A and group 2 is line B. In 4 DELAYS, lines A and C form group 1 and B and D group 2. Right-click RECORD to record only into one group while the other keeps playing untouched. You can feed two different sources into Deriva and decide which one is captured, or keep half of the memory as a frozen bed. With RECORD off, the input is still heard through MIX but nothing is written.</p>
<h2>Erasing</h2>
<p>Holding ERASE writes silence under the heads. In the delay algorithms you also hear the memory go quiet immediately; release the switch to keep the rest. ERASE ALL clears everything with a short fade-out and fade-in.</p>
<h2>Reverse and half speed</h2>
<p>REVERSE changes the direction of the heads, so what was recorded plays backwards and new material is recorded backwards too. Reverse again and that new material plays reversed. Half speed (delays only) slows playback and recording to half: the memory drops an octave and lasts twice as long. Material recorded at half speed comes back at its original pitch when you return to normal speed. The change takes about 150 ms, like a tape machine slowing down.</p>
<h2>Switching algorithms</h2>
<p>All algorithms share the same block of memory. When you switch, the new layout reads what is already there, so a phrase recorded in 2 DELAYS can become the raw material of the giant reverb or the granular engine. Try changing algorithm in the middle of a piece.</p>
<h2>Saving the memory</h2>
<p>With SAVE MEMORY on, the memory is stored inside your project in compressed 16-bit form, about 3–4 MB at 48 kHz, and comes back when you reopen it. If the project is opened at another sample rate, the memory is resampled. Changing the sample rate while the plugin is running clears the memory.</p>

<h1 id="beads"><span class="num">10</span> The BEADS stage</h1>
<p class="lead">BEADS adds the granular processor of Mutable Instruments Beads to Deriva, using the original DSP code by Émilie Gillet. Click the BEADS chip on the header: a second panel opens below the main one and the stage is switched on with a short crossfade. Click BEADS again, or CLOSE in the panel, to close it and bypass the stage.</p>
{beads_panel("The BEADS panel. Numbers refer to the table below.")}
<table><tr><th>#</th><th>Control</th><th>Function</th></tr>
<tr><td>1</td><td>Activity string</td><td>The beads light up with the output level of the stage.</td></tr>
<tr><td>2</td><td>FREEZE</td><td>Stops recording into the Beads buffer; grains keep reading the same material.</td></tr>
<tr><td>3</td><td>SEED</td><td>Latched (pulsing LED): grains are generated continuously according to DENSITY. Press: manual mode, each press triggers grains. Hold 1 s: latch again.</td></tr>
<tr><td>4</td><td>TIME</td><td>Where grains are read in the 4-second buffer, from the most recent to the oldest material.</td></tr>
<tr><td>5</td><td>SIZE</td><td>Grain length. Below one third, grains play backwards. Fully clockwise, Beads becomes a delay (see below).</td></tr>
<tr><td>6</td><td>SHAPE</td><td>Grain envelope, from percussive to smooth and reversed.</td></tr>
<tr><td>7</td><td>PITCH</td><td>Transposition up to ±24 semitones, with resting points at 1, 3, 4, 7, 12 and 24.</td></tr>
<tr><td>8</td><td>RANDOM ×4</td><td>Randomness for TIME, SIZE, SHAPE and PITCH. Clockwise: a new random value for each grain. Counter-clockwise: slow random drift.</td></tr>
<tr><td>9</td><td>DENSITY</td><td>Centre: no grains (only SEED). Left: random grains, denser as you turn. Right: regular grains, faster as you turn.</td></tr>
<tr><td>10</td><td>FEEDBACK</td><td>Beads' own feedback into its buffer.</td></tr>
<tr><td>11</td><td>DRY / WET</td><td>WET position: balance between Deriva's memory and the grains. LOOP position: how much of Deriva's feedback goes through Beads.</td></tr>
<tr><td>12</td><td>REVERB</td><td>Beads' plate-like reverb (Griesinger/Dattorro topology).</td></tr>
<tr><td>13</td><td>QUALITY</td><td>DIRTY TAPE (8-bit µ-law at 24 kHz with wow and flutter), CLEAN TAPE (12-bit, 24 kHz), VINTAGE (12-bit, 32 kHz) or DIGITAL (16-bit, 48 kHz). Changing it restarts the Beads buffer with a short fade.</td></tr>
<tr><td>14</td><td>WET / LOOP</td><td>Where the stage sits (see below).</td></tr>
<tr><td>15</td><td>MONO, SYNC</td><td>MONO: mono buffer with twice the time. SYNC: host quarter notes drive the SEED input.</td></tr>
<tr><td>16</td><td>CLOSE</td><td>Closes the panel and bypasses the stage.</td></tr>
</table>
<h2>WET and LOOP</h2>
<p>In <strong>WET</strong> (default), Beads processes the memory output before MIX. Deriva's memory is untouched; you add a granular layer on top of what comes out. In <strong>LOOP</strong>, Deriva's feedback goes through Beads, in the amount set by DRY/WET, before being written back. Every repetition is grained, transposed or crushed again, so the memory evolves into something new. In LOOP, Beads' own FEEDBACK and REVERB are disabled so that two feedback loops never pile up.</p>
<h2>Delay mode and SYNC</h2>
<p>With SIZE fully clockwise, Beads works as a delay instead of a granulator. With SYNC on and the host playing, Beads receives a pulse on every quarter note. With SEED latched, DENSITY then chooses a divider and probability of those pulses, which gives tempo-locked grains or echoes. When the transport stops, Beads goes back to its internal timing.</p>
<div class="tip"><b>Latency</b>Beads always runs at 48 kHz, its native rate. At other sample rates Deriva converts in and out with a high-quality resampler. This adds about 5 ms at 48 kHz and around 30 ms at 44.1 or 96 kHz, only on the memory path, where it is inaudible.</div>

<h1 id="midi"><span class="num">11</span> MIDI control</h1>
<p>Deriva accepts MIDI on any channel, so a MIDI foot controller can play it like the pedal. In REAPER, route a MIDI track or your controller to the track that holds Deriva.</p>
<div class="two"><div><h3>Notes (footswitches)</h3><table><tr><th>Note</th><th>Function</th></tr>
<tr><td>60</td><td>HPF on/off</td></tr><tr><td>61</td><td>LPF on/off</td></tr><tr><td>62</td><td>ERASE (held)</td></tr>
<tr><td>63</td><td>REVERSE</td></tr><tr><td>64</td><td>RECORD on/off</td></tr><tr><td>65</td><td>ERASE ALL</td></tr>
<tr><td>66</td><td>HALF SPEED</td></tr><tr><td>67</td><td>Beads FREEZE</td></tr><tr><td>68</td><td>Beads SEED (held)</td></tr>
<tr><td>69</td><td>BEADS on/off</td></tr></table></div>
<div><h3>Control change</h3><table><tr><th>CC</th><th>Parameter</th></tr>
<tr><td>20</td><td>IN</td></tr><tr><td>21</td><td>BLUR</td></tr><tr><td>22</td><td>DRIFT</td></tr><tr><td>23</td><td>DRIVE</td></tr>
<tr><td>24</td><td>SUP | COM</td></tr><tr><td>25</td><td>FB</td></tr><tr><td>26</td><td>MIX</td></tr><tr><td>27</td><td>OUT</td></tr>
<tr><td>28</td><td>Algorithm</td></tr><tr><td>29–32</td><td>Beads TIME, SIZE, SHAPE, PITCH</td></tr>
<tr><td>33–36</td><td>Beads DENSITY, FEEDBACK, DRY/WET, REVERB</td></tr></table></div></div>
<p>Every parameter can also be automated from the host (see the appendix for the full list).</p>

<h1 id="recipes"><span class="num">12</span> Recipes</h1>
<div class="recipe"><h3>Frippertronics</h3><p class="set">2 DELAYS · M · FB just below 1.0 · DRIFT 30 % · BLUR 0 · LPF MED on</p><p>Play long guitar notes and let them come back. Each pass loses a little top end, like the Revox system. Turn RECORD off to solo over the bed, and back on to add another layer.</p></div>
<div class="recipe"><h3>Drifting choir</h3><p class="set">4 DELAYS · L · FB 0.98 · DRIFT 70 % · BLUR 50 % · SUP|COM: COM 40 %</p><p>Sing or play pads. The compressor keeps the memory even and the four lines wander across the stereo field.</p></div>
<div class="recipe"><h3>Endless hall</h3><p class="set">REVERB · INSANE · FB 1.0 · MIX centre · HPF MIN on</p><p>Feed short sounds: they bloom as individual echoes and slowly turn into a still cloud. The HPF stops the low end from building up. Use ERASE ALL to reset.</p></div>
<div class="recipe"><h3>New phrase replaces the old</h3><p class="set">2 DELAYS · S · FB 1.0 · SUP|COM: SUP 70 %</p><p>Each time you play, the stored loop ducks and is replaced, so the memory always holds your latest phrase. It works well for rhythmic or melodic cells.</p></div>
<div class="recipe"><h3>Shimmering memory (LOOP)</h3><p class="set">2 DELAYS · M · FB 0.97 · BEADS on · LOOP · PITCH +12 · SIZE 60 % · DENSITY 70 % · DRY/WET 35 %</p><p>Every pass climbs an octave in grains while the original stays underneath: a slowly rising shimmer. Lower DRY/WET to slow down the transformation.</p></div>
<div class="recipe"><h3>Broken cassette</h3><p class="set">Any delay · BEADS on · WET · QUALITY DIRTY TAPE · SIZE fully clockwise (delay mode) · SYNC on · REVERB 30 %</p><p>A tempo-locked, warped echo on top of the drifting memory, full of wow, flutter and µ-law grit.</p></div>
<div class="recipe"><h3>Frozen bed and live grains</h3><p class="set">GRAIN · L · record a texture · RECORD off · BEADS on · FREEZE · TIME RANDOM 40 %</p><p>Deriva holds the bed while Beads scatters frozen grains across it. Play over the top with the dry signal.</p></div>

<h1 id="technical"><span class="num">13</span> Technical notes</h1>
<table><tr><th>Element</th><th>Implementation</th></tr>
<tr><td>Delay lines</td><td>One fractional pointer per line reads and writes (Rocchesso, <em>Fractionally-addressed delay lines</em>, 2000). At normal speed heads sit on whole samples, so playback is exact and no filtering accumulates.</td></tr>
<tr><td>Interpolation</td><td>4-point Hermite (Niemitalo) for half speed, reverse and grains.</td></tr>
<tr><td>BLUR</td><td>Givens rotations (orthogonal, energy-preserving). At 45° they equal a normalised Hadamard matrix (Gerzon; Jot and Chaigne; Smith).</td></tr>
<tr><td>Giant reverb</td><td>8-line FDN with a Householder matrix and two lossless Schroeder allpasses per line.</td></tr>
<tr><td>Filters</td><td>Zero-delay-feedback state variable filters (Zavalishin, <em>The Art of VA Filter Design</em>).</td></tr>
<tr><td>DRIVE</td><td>tanh with first-order antiderivative antialiasing (Parker, Zavalishin and Le Bivic, DAFx-16).</td></tr>
<tr><td>DRIFT</td><td>LFOs with irrational rate ratios (1, √2, φ, √3…), rates modulated by a slow Lorenz attractor.</td></tr>
<tr><td>Limiter</td><td>Static soft curve: linear up to −6 dBFS, tanh knee to a ceiling of 0 dBFS, inside the loop.</td></tr>
<tr><td>BEADS</td><td>Original Mutable Instruments code at 48 kHz in 32-sample blocks; r8brain resampling (6 % transition band, 120 dB).</td></tr>
</table>
<p><strong>Performance:</strong> about 0.4–1.2 % of one CPU core for Deriva and around 1 % for the BEADS stage (measured on a desktop x86 CPU at 48 kHz). Deriva adds no latency; the BEADS stage delays only the memory path.</p>
<p><strong>Verification:</strong> offline tests run on every build. They check that FB 1.0 conserves the loop energy within ±0.04 dB over 150 s, that the limiter contains FB 1.15, that 100 algorithm changes are click-free, the exact octave of half speed, all twelve algorithms, ERASE ALL, the sample rates, and stable resampling at 44.1–96 kHz with any block size. The plugin passes <em>pluginval</em> at its strictest level.</p>

<h1 id="cosmos"><span class="num">14</span> Deriva and the COSMOS</h1>
<p>Deriva follows what SOMA's manual and FAQ describe: the 44.7-second memory of prime-length lines, the four families with their three sizes and documented times, and the behaviour of every control in each algorithm. It also reproduces the four recording targets, erase, reverse, half speed, the noise gate, the mono sum and the memory carried over between algorithms.</p>
<p>What SOMA has not published had to be estimated. That includes the exact primes, the curves of the limiter and drive, the filter frequencies, the suppressor and compressor timings, the reverb topology and the granular details. Deriva therefore behaves like the COSMOS, but it is not a measured clone and its sound will differ, most of all in the reverb and granular families. Other differences are deliberate:</p>
<ul>
<li>The reverb never drifts out of tune.</li>
<li>Hidden switch combinations are visible chips.</li>
<li>A long press on RECORD takes 2 s instead of 5.</li>
<li>The rhythmic, pitch and quad firmwares are not included.</li>
<li>Deriva adds things the pedal does not have: the BEADS stage, tempo sync, MIDI and saving the memory.</li>
</ul>

<h1 id="faq"><span class="num">15</span> Troubleshooting and FAQ</h1>
<h3>I hear nothing from the memory.</h3><p>Check that RECORD is on (bright red LEDs), that MIX is not fully counter-clockwise and that FB is above 0. Very quiet inputs may be held by the gate: turn on GATE SENS or raise IN.</p>
<h3>The memory keeps getting louder and distorted.</h3><p>FB is above 1.0 or the compressor is pushing it. Lower FB just under the 1.0 mark, or use the suppressor.</p>
<h3>The BEADS stage is silent.</h3><p>DENSITY in the centre produces no grains unless SEED is pressed. Turn DENSITY away from the centre, check that SEED is latched (pulsing LED) and that DRY/WET is not fully counter-clockwise. With SYNC on, grains need the host transport running.</p>
<h3>The memory is gone after reopening the project.</h3><p>Turn on SAVE MEMORY before saving the project.</p>
<h3>Changing the sample rate erased the memory.</h3><p>That is expected: the memory is rebuilt for the new rate.</p>
<h3>macOS says the plugin is damaged.</h3><p>See the note in chapter 4 about removing the quarantine flag.</p>

<h1 id="credits"><span class="num">16</span> Credits and licences</h1>
<ul>
<li><strong>Deriva</strong> is built with JUCE 8. JUCE is available under AGPLv3 or a commercial licence, so a version distributed without a commercial JUCE licence must keep its source code open under AGPLv3.</li>
<li><strong>Beads DSP and stmlib:</strong> © Émilie Gillet, Mutable Instruments, MIT licence. Local changes are listed in <code>ThirdParty/mi/README-Deriva.txt</code>.</li>
<li><strong>r8brain-free-src:</strong> © Aleksey Vaneev, MIT licence.</li>
<li><strong>Montserrat font:</strong> SIL Open Font License.</li>
<li>SOMA Laboratory, COSMOS, Mutable Instruments and Beads are names of their respective owners, used here only as references. Deriva is not affiliated with them.</li>
</ul>
<p class="small">References: Rocchesso, D. "Fractionally-addressed delay lines", IEEE TSAP 8(6), 2000 · Niemitalo, O. "Polynomial interpolators for high-quality resampling of oversampled audio", 2001 · Jot, J.-M. and Chaigne, A. "Digital delay networks for designing artificial reverberators", AES 1991 · Smith, J. O. <em>Physical Audio Signal Processing</em> · Luff, G. "Let's write a reverb", 2021 · Zavalishin, V. <em>The Art of VA Filter Design</em> 2.1 · Parker, J., Zavalishin, V. and Le Bivic, E. "Reducing the aliasing of nonlinear waveshaping using continuous-time convolution", DAFx-16 · SOMA Laboratory, COSMOS user manual v1.3 and FAQ · Mutable Instruments, Beads source code.</p>

<h1 id="params"><span class="num">A</span> Appendix: parameter list</h1>
<p>Names as they appear in your host's automation lanes.</p>
<table><tr><th>Parameter</th><th>Range / values</th><th>Default</th></tr>
<tr><td>Algorithm</td><td>1–12</td><td>2 (2 DELAYS · M)</td></tr>
<tr><td>In</td><td>−24 … +24 dB</td><td>0 dB</td></tr>
<tr><td>Blur / Drift / Drive</td><td>0–100 %</td><td>0 / 30 / 0 %</td></tr>
<tr><td>Sup|Com</td><td>SUP 100 % … OFF … COM 100 %</td><td>OFF</td></tr>
<tr><td>Feedback</td><td>0 … 1.15</td><td>0.97</td></tr>
<tr><td>Mix</td><td>dry/memory 100/0 … 0/100</td><td>centre</td></tr>
<tr><td>Out</td><td>−inf … +12 dB</td><td>0 dB</td></tr>
<tr><td>HPF, LPF</td><td>OFF / ON</td><td>OFF</td></tr>
<tr><td>HPF Cutoff, LPF Cutoff</td><td>MIN / MED / MAX</td><td>MIN, MED</td></tr>
<tr><td>Record</td><td>OFF / ON</td><td>ON</td></tr>
<tr><td>Record Target</td><td>GROUP 1 / GROUP 2 / ALL</td><td>ALL</td></tr>
<tr><td>Reverse, Half Speed, Erase, Erase All</td><td>OFF / ON</td><td>OFF</td></tr>
<tr><td>Input</td><td>STEREO / MONO SUM</td><td>STEREO</td></tr>
<tr><td>Gate</td><td>NORMAL / SENSITIVE</td><td>NORMAL</td></tr>
<tr><td>Save Memory</td><td>OFF / ON</td><td>OFF</td></tr>
<tr><td>Beads</td><td>OFF / ON</td><td>OFF</td></tr>
<tr><td>Beads Place</td><td>WET / LOOP</td><td>WET</td></tr>
<tr><td>Beads Time / Size / Shape</td><td>0–100 %</td><td>40 / 55 / 50 %</td></tr>
<tr><td>Beads Pitch</td><td>−24 … +24 st</td><td>0 st</td></tr>
<tr><td>Beads … Random (×4)</td><td>−100 … +100 %</td><td>0</td></tr>
<tr><td>Beads Density</td><td>RANDOM … MANUAL … REGULAR</td><td>REGULAR 38 %</td></tr>
<tr><td>Beads Feedback / Dry/Wet / Reverb</td><td>0–100 %</td><td>0 / 60 / 25 %</td></tr>
<tr><td>Beads Freeze, Seed</td><td>OFF / ON</td><td>OFF</td></tr>
<tr><td>Beads Seed Latch</td><td>OFF / ON</td><td>ON</td></tr>
<tr><td>Beads Quality</td><td>DIRTY TAPE / CLEAN TAPE / VINTAGE / DIGITAL</td><td>DIGITAL</td></tr>
<tr><td>Beads Mono, Beads Sync</td><td>OFF / ON</td><td>OFF</td></tr>
</table>
"""

CONTENT = dict(lang="en", cover_title="USER MANUAL", cover_meta="VERSION {version} · VST3 · LINUX · WINDOWS · MACOS",
               toc_title="Contents", toc=TOC, body=BODY)
