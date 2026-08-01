# NanoEls H4 calibration guide

Step-by-step instructions for the calibration routines in `NanoEls H4 V15`.

These routines **measure** your machine instead of asking you to know its numbers in advance. Everything they save goes to the same places the settings menu edits, so you can always check or override a result afterwards.

---

## Before you start

**What you need**

- A dial indicator (DTI) with a magnetic base. 0.01mm resolution is enough; 0.002mm is better.
- Something to mark the chuck with — a chalk line or a bit of tape.
- Roughly an hour for a full first-time calibration. Individual routines take a few minutes.

**Safety**

- The controller must be **off** (not running an operation). If it isn't, opening calibration shows `Turn off first`.
- **Nothing moves without you pressing play.** Every test move is shown on screen first and only happens when you confirm it. The stop button aborts and puts back anything the routine changed.
- Remove the tool from the work, or move the carriage somewhere with clear travel, before running any routine that moves an axis.
- Moves that would cross a soft limit or exceed the axis travel are **refused outright**, not shortened — a shortened test move would silently corrupt the measurement it feeds.

**Opening the calibration screen**

1. With the controller off, press the **settings** button.

   > The settings button is the **hexagon with a dot in the middle** (a hex nut seen end-on), on the right-hand side of the panel in the inner of the two right columns — directly below the rectangle (display) button and directly above the caliper (measure) button. It's easy to miss because it's the one key the manual refers to by name rather than by symbol. Don't confuse it with the double-arrow ⇄ in the outer column, which is reverse.

2. Press the down arrow until you reach `Settings 22 of 22` — `Calibration`.
3. Press play.

You're now in the routine list:

```
Calibrate 1 of 14
Z screw pitch
Now 2
ON start, OFF exit
```

- **Up / down** — move one routine at a time
- **Left / right** — jump four at a time
- **Play** — start the selected routine
- **Stop** — go back to the settings menu

The third line shows the value that routine currently holds, so you can see what you're about to change. Diagnostic routines show `Diagnostic, no moves` instead.

**Inside a routine**, throughout:

- **Play** — confirm and advance
- **Minus** — step *back* one, to redo an entry you got wrong
- **Plus** — run the current step's move *again*, without advancing
- **Step** — cycle the move step between 1mm, 0.1mm and 0.01mm (0.1", 0.01", 0.001" in imperial). This sets how far one arrow press moves the axis in the backlash and travel limit routines, and the screen shows the current size while it matters
- **Stop** — abort the whole routine and restore anything it changed

### Redoing a step, and repeating a move

Two keys, mirroring each other: **minus goes back, plus repeats where you are.** Between them you should never need to abort and start over because of one bad entry or one move you want to see again.

Neither ever *undoes* a move — they return you to a prompt or run a move a second time. That's safe because every repeatable move in these routines goes the *same direction* as the one before it, so running one again leaves the axis loaded exactly as it was and backlash stays out of the result.

| Routine | Minus (back a step) | Plus (repeat the move) |
|---|---|---|
| Screw pitch | confirm → retype measurement → repeat test move → change test distance | Another 1mm slack take-up, before the test move only |
| Backlash | confirm → keep nudging → re-zero indicator → redo the 2mm load | Another 2mm load, before the indicator is zeroed only |
| Direction | Runs the test move again | Runs the test move again |
| Travel limit | confirm → re-jog second end → re-mark first end | — (jogging is already repeatable) |
| Max speed | From the confirm screen, back to the ramp at the last speed that ran | **Another stroke at the same speed, without stepping it up** |
| Encoder PPR | confirm → keep turning → restart the count | — (no moves) |

Where there's nothing to go back to or repeat, the controller beeps.

Two deliberate limits:

- **Plus won't repeat the pitch test move or the backlash measuring nudge.** Running the test move twice would add a second test distance while the indicator still reads from the first, so the total would no longer match what it's compared against. Step back with minus and re-zero instead.
- **A move already in progress won't be interrupted.** Pressing play or plus again while an axis is still travelling beeps rather than retargeting, which would otherwise compound the two into a longer move than either.

---

## Recommended order

Work down this list the first time. It goes from zero risk to most, and later routines are more trustworthy once the earlier ones are right.

| # | Routine | Moves? |
|---|---|---|
| 1 | Input tester | No |
| 2 | Encoder signal | No |
| 3 | Encoder PPR | No (you turn the spindle by hand) |
| 4 | Encoder direction | No |
| 5 | Z direction, X direction | Yes |
| 6 | Z screw pitch, X screw pitch | Yes |
| 7 | Z backlash, X backlash | Yes |
| 8 | Z travel limit, X travel limit | Yes, you jog |
| 9 | Z max speed, X max speed | Yes |

**After calibrating, back up your settings** — see [Saving your results](#saving-your-results) at the end. There is no undo in the settings menu.

---

## 1. Input tester

Checks that every key registers and, if fitted, that the joystick is wired correctly. Nothing moves.

1. Select `Input tester`, press play.
2. Press each key on the panel in turn. The screen shows the code of the last key and whether it's `down` or `up`:
   ```
   Input tester
   Key 17 down
   Joystick disabled
   Hold OFF 1s to exit
   ```
3. Confirm every key produces a code, and that the code returns to `up` when you release it. A key that stays `down` is stuck.
4. If you have a joystick fitted **and** `JOYSTICK_USE` is set to `true` in the firmware, line 3 shows six digits — one per joystick pin, `1` when active. Move the stick and press its buttons and watch them change. If `JOYSTICK_USE` is `false` the pins aren't configured, so the tester says `Joystick disabled` rather than showing meaningless values.
5. **To leave, hold the stop button for a full second.** This screen deliberately swallows every key, so a quick tap won't exit.

---

## 2. Encoder signal

A live health check on the spindle encoder. This is the screen to come back to any time threading goes strange. Nothing moves.

1. Select `Encoder signal`, press play.
   ```
   Encoder signal
   Ang 214.5 Rpm 0
   Coh 100 Lo 100 Flp 0
   < reset, OFF exit
   ```
2. **`Coh` is coherence, as a percentage, and it should read 100 whenever the spindle is turning.**

   It compares how far the counter *travelled* against how far it actually *moved*, over a 20ms window. A cleanly rotating encoder travels exactly as far as it moves, so the two match. An encoder dithering on a quadrature transition racks up travel that never becomes movement, and coherence falls.

   This is worth watching precisely because the RPM readout cannot show it: RPM is accumulated signed, so a counter jittering back and forth cancels itself out and reads as a *stopped* spindle rather than a noisy one.

3. **`Lo` is the lowest coherence seen since the last reset.** This is the number that matters. Noise arrives in bursts you won't be standing at the screen for, so leave the machine running, go and cut something, then come back and read `Lo`.

   Two kinds of window are shown but not recorded, so `Lo` stays meaningful. Anything below 30 rpm is ignored — starting, stopping and hand-turning all produce low coherence honestly. So is any window carrying fewer than 8 counts, because coherence resolves to 100/counts percent and a window holding two counts can only ever read 100, 50 or 0. **On a coarse encoder that second rule matters:** below roughly 250 counts per revolution you may find `Lo` stays at `--` at low speeds simply because no window was dense enough to judge. Run faster, or raise the PPR, before concluding the signal is clean.
4. **`Flp` counts spurious reversals** — a direction change after barely any movement, which is the counter dithering rather than you turning the spindle back. It should stay at 0.
5. Turn the spindle slowly by hand. `Ang` should sweep smoothly through 0–360° and back to 0 without jumping. Run the spindle under power and check `Rpm` reads steadily rather than wandering.
6. Press the left arrow to reset the counters and watch again.
7. Press stop to exit.

The web page's derived-figures panel shows the same numbers plus `dirtyWindows`, a running count of windows that came in under 95%. That's the one to leave open on a phone through a whole job.

### Reading the result

| `Lo` | What it means |
|---|---|
| 100 | Clean. If threading is still wrong, the encoder is not the cause. |
| 95–99 | Occasional dithering. Usually harmless, worth watching. |
| 50–95 | Real noise reaching the counter. Fix it before trusting a thread. |
| Near 0 with `Rpm` reading 0 | The counter is moving but going nowhere — classic dither, and the case that silently looks like a stopped spindle. |

**If `Lo` falls or `Flp` climbs**, in order of what to try. The first four are causes; the last two are masks:

- Route the encoder cable away from the stepper and VFD wiring rather than bundled with it. Coherence that tracks VFD frequency is coupling, not a bad encoder.
- Make sure the encoder ground returns to the controller ground at a single point — a ground loop through the lathe frame injects noise no filter setting will fix.
- Ground the cable shield at one end only.
- Add a 1nF capacitor from each of the A and B lines to ground at the controller end.
- Raise `Glitch filter`, but check the ceiling first — it depends on your encoder and gearing, not just the filter value:

  ```
  highest usable ENCODER rpm  =  2.4e9 / (ENCODER_PPR * ENCODER_FILTER)
  ```

  and a geared-up encoder spins faster than the spindle. At 1000 PPR with `ENCODER_FILTER = 200` that's 12,000 encoder rpm, or 6,000 at the spindle through a 1:2 belt. Raising it to 400 would halve that to 3,000 spindle rpm, which is inside a lathe's working range — so a value that was safe on a direct-driven 600 PPR encoder can silently start dropping pulses on a geared 1000 PPR one.
- Switch `Dead-band shape` to `symmetric` — see below.

### Dead-band shape

The dead-band is how far the count has to move before the axes follow it. Its **shape** decides whether that applies in both directions:

- **`one-way`** (the default, and how the firmware has always behaved) holds the follower in `[count, count + band]`. It snaps forward the instant the count rises and lags only on the way back. That models mechanical backlash in a lead screw, which is what the setting is named after.

  It is **not** a noise filter and does not act as one. A spurious count in the rising direction is adopted immediately, and the offset it leaves is permanent — the follower simply waits at the higher value for real movement to catch up. Blips in one direction therefore accumulate across a pass, which looks like a thread drifting rather than a carriage twitching.

- **`symmetric`** holds the follower in `[count - band, count + band]`, moving only once the count pushes past the band on either side. Both directions are filtered equally, so a blip forward and back nets to nothing.

  The price is that the rising direction now lags by up to `Dead-band` counts where it previously did not. On a machine with a clean encoder that is a cost with no benefit — check `Lo` first.

Raising `Dead-band` treats the symptom rather than the cause either way, and the cost is that a genuine spindle reversal is ignored for that many counts. At 600 PPR, `16` counts is 4.8°. It is still the fastest way to stop a twitching carriage while you chase the wiring.

---

## 3. Encoder PPR

Derives your encoder's pulses-per-revolution by counting while you turn the spindle a known number of turns.

1. **Mark the chuck** and pick a fixed reference point on the lathe body to line it up against. Accuracy depends entirely on stopping at exactly the right place.
2. Select `Encoder PPR`, press play.
   ```
   Encoder PPR
   Turn 10 revolutions
   Mark the chuck first
   <> revs, ON start
   ```
3. Use left/right to choose how many revolutions: **1, 5, 10 or 20**. More turns means a more accurate result — 10 is a good default.
4. Line the mark up with your reference point, then press play to start counting.
5. Turn the spindle **by hand** through exactly that many revolutions, finishing with the mark back on the reference point. The screen counts as you go and shows its running estimate:
   ```
   Counted 23994
   PPR so far 600
   ```
   The raw count is four per encoder pulse — the controller reads both edges of both channels — so a 600 PPR encoder gives 2400 counts per revolution.
6. Press play when you're back at the mark.
7. The result is shown next to the current value. If it landed within 2% of a standard encoder resolution it snaps to that exact figure. Press play to save, stop to discard.

**If you stopped short of the mark**, press **minus** to go back to counting and keep turning to the right place — the count carries on rather than restarting. Press minus again to restart the count from scratch.

**If it's rejected** with `PPR must be 24-10000`, the count was far off — usually a miscount of turns, or the encoder isn't producing clean pulses. Run **Encoder signal** first.

---

## 4. Encoder direction

Makes the controller count the spindle the right way round. Previously this needed the A and B wires physically swapped.

1. Select `Encoder direction`, press play.
   ```
   Encoder direction
   Turn spindle fwd
   142 normal
   Rising? ON. Else <
   ```
2. Turn the spindle **forwards** — the direction you cut in.
3. Watch the number on line 3. If it **counts up**, the direction is correct: press play to accept.
4. If it **counts down**, press the left arrow. The direction flips and saves immediately, and the count resets so you can confirm it now rises.
5. Press play to finish.

---

## 5. Axis direction (Z and X)

Confirms each axis moves the way the controller thinks it does.

1. Make sure the axis has at least 5mm of clear travel in the positive direction.
2. Select `Z direction` (or `X direction`), press play.
   ```
   Z direction
   Test move +5mm
   Now normal
   ON to move
   ```
3. Press play. The axis moves 5mm.
4. Answer the question on screen:
   - **Z:** `To the tailstock?`
   - **X:** `Away from you?`
5. Press **play for yes** — the direction is correct and the routine ends. Press the **left arrow for no** — the inversion flips, saves, and the test move runs again so you can confirm.

**If you missed which way it moved**, press **plus** to run the same test move again before answering. Watch out for running out of travel if you repeat it several times — each press moves another 5mm the same way.

Repeat for the other axis.

---

## 6. Screw pitch (Z and X)

**The routine that makes parts come out the right size.** It commands a known move, you measure what actually happened, and it corrects the stored lead screw pitch.

1. Mount a dial indicator so it reads travel along the axis you're calibrating. On X this must read **actual cross-slide travel**, not a diameter — the routine works in slide travel regardless of whether your X readout is set to diameter mode.
2. Position the axis so it has room for the pre-load plus your chosen test distance, in the positive direction.
3. Select `Z screw pitch` (or `X screw pitch`), press play.
   ```
   Z screw pitch
   Test 10mm
   Zero indicator now
   <> size, ON load
   ```
4. Use left/right to choose the test distance: **1, 5, 10, 25, 50 or 100mm**. **Longer is more accurate** — use the longest your indicator and setup allow. 50mm is a good target on Z.
5. Press play. The axis moves **1mm** to take up the slack.
   ```
   Slack taken up
   Do not move it back
   Zero dial, ON move
   ```
   If you're not confident the slack is fully out — you bumped the handwheel, or the screw is badly worn — press **plus** for another 1mm. Repeat as many times as you like; it only ever moves further in the same direction.
6. **Now zero your indicator.** Do not jog the axis backwards from here — see the note below.
7. Press play. The axis moves your chosen test distance.
8. Read the indicator and type what it **actually** travelled, then press play:
   - Metric mode: type **microns**. A reading of 50.02mm is `50020`.
   - Inch mode: type **thou**. A reading of 1.9685" is `1969`.
9. The corrected pitch is shown next to the old one. Press play to save, stop to discard.
10. **Re-zero your axes afterwards** — the physical meaning of the stored positions has changed.

**If you mistyped the measurement**, press **minus** at step 9 to go back and retype it. The test move you already made is still valid, so nothing needs re-running. Press minus again to repeat the test move itself — useful if the indicator slipped. The axis stays loaded in the same direction, so a repeated test move is still backlash-free.

**Why the 1mm pre-load matters.** Both moves go the same direction, so by the time you zero the indicator the axis is already loaded and every step of the test move produces real travel. Without it, an axis you'd last jogged *backwards* would spend the start of the test move taking up slack, the controller would add however much backlash compensation is currently configured, and any error in that setting would land straight in your measured pitch. This is also why pitch can be calibrated **before** backlash — the routine doesn't depend on backlash being right yet.

**If you get `Off by >20%, check`**, the correction was too large to be credible and was rejected. Almost always a mistyped measurement — check you used microns and not millimetres. If the number really is that far out, your motor steps setting is probably wrong; fix that in the settings menu first.

---

## 7. Backlash (Z and X)

Measures the slack in the lead screw and nut.

1. Mount the dial indicator to read travel along the axis.
2. Select `Z backlash` (or `X backlash`), press play.
   ```
   Z backlash
   Take up slack +2mm
   ON to move
   ```
3. Press play. The axis moves 2mm to load the screw in one direction. Press **plus** for another 2mm if you want to be certain the slack is fully out before zeroing.
4. **Zero your indicator**, then press play.
5. Now press the **left arrow repeatedly**. Each press backs the axis off by one step, and the accumulated distance is shown along with the step size currently in use:
   ```
   Back off 0.05
   STEP key: 0.01mm
   <> nudge, ON if mvd
   ```
6. **Press the step button to change the nudge size** — it cycles 1mm → 0.1mm → 0.01mm exactly as it does on the main screen (0.1" → 0.01" → 0.001" in imperial). This is your measurement resolution, so drop it to 0.01mm before the needle gets close. Typical backlash is 0.02–0.20mm, so starting at 0.1mm to cover ground and switching to 0.01mm for the last part is the quickest way in.
7. **Press play the instant the indicator needle starts to move.** The distance travelled up to that point is the backlash.
8. The result is shown next to the current value. Press play to save.

**If you overshot**, press the **right arrow** to nudge forward again by the same step. The measurement is the distance from where the indicator was zeroed, so walking back and forth over the last few hundredths is fine and does not corrupt the result.

**If you pressed play too early**, press **minus** to go back to nudging and carry on from where you were — the accumulated distance is kept, so you can keep stepping until the needle really moves.

Backlash compensation is switched off for the duration of the routine, so it can't measure itself.

---

## 8. Travel limit (Z and X)

Records how far the axis can physically travel. This is what the "too far" safety check is based on.

This is the **only routine you drive yourself** — an axis can't be safely auto-driven toward an unknown hard stop.

1. Select `Z travel limit` (or `X travel limit`), press play.
   ```
   Z travel limit
   Jog to one end
   STEP key: 1mm
   <> jog, ON mark
   ```
2. Jog to one extreme of travel using the arrows — **left/right for Z, up/down for X**. Stop short of crashing; you're recording usable travel, not hard-stop to hard-stop. A tap moves one step and the **step button** cycles that between 1mm, 0.1mm and 0.01mm, so you can creep the last bit in without overshooting.
3. Press play to mark that end.
4. Jog to the other extreme. The span updates live:
   ```
   Span 285.4
   Now jog to other end
   ```
5. Press play. The span in whole millimetres is offered next to the current value.
6. Press play to save.

The value is truncated down to a whole millimetre, which errs on the safe side.

---

## 9. Max speed (Z and X)

Finds the fastest the axis can be driven without the stepper stalling, then backs off for a margin.

1. Make sure the axis has at least **20mm of clear travel** and nothing is in the way — this routine runs repeated strokes at increasing speed.
2. Select `Z max speed` (or `X max speed`), press play.
   ```
   Z max speed
   Ramp from 3000
   ON to start
   ```
3. Press play to begin. The screen shows the speed it's about to try:
   ```
   Try 3000 steps/s
   20mm strokes
   ON faster, < stalled
   ```
4. Press **play** to run one 20mm stroke at that speed. Each stroke alternates direction, and the trial speed goes up 10% afterwards.
5. Keep pressing play, **listening carefully**. A stalling stepper makes a harsh buzzing or grinding sound and stops moving properly.

   **If you're not sure about a speed, press plus to run it again at the same rate** instead of stepping up. This is the one place repeating really matters — every play press raises the speed, so without it you can't hear a borderline speed twice before committing.
6. **The moment it stalls or sounds wrong, press the left arrow.** The routine takes the last speed that completed cleanly and offers 80% of it.
7. Press play to save.

**If you called the stall too early**, press **minus** to return to the ramp at the last speed that ran, and carry on.

If you press stop at any point, the original speed is put back.

---

## Saving your results

Calibration values live in the controller's flash and survive reboots and firmware updates — but **there is no undo in the settings menu**, and one mistyped value overwrites the old one permanently. Take a backup once you're happy.

1. Connect the controller to a computer over USB.
2. Open a serial terminal at **115200 baud**.
3. Send `$` on its own line. The controller prints every stored setting:
   ```
   ; NanoEls H4 V15 settings
   ; paste these lines back to restore, distances are in deci-microns
   $eppr=600
   $Zinv=0
   $Zbla=6500
   $Zscr=20000
   ...
   ```
4. **Save that text somewhere.** To restore, paste the lines back into the terminal.

To change one setting directly, send a single line like `$Zbla=250`. Values use the same units as storage — deci-microns for distances, `0`/`1` for on/off — and go through exactly the same checks as the settings menu. Settings can't be changed while the controller is on or an axis is moving.

---

## Message reference

| Message | Meaning |
|---|---|
| `Turn off first` | Calibration can't be opened while the controller is running |
| `Limited by stop` | The test move would cross a soft limit. Clear the stop or move the axis first |
| `Too far, ignored` | The move exceeds the axis max travel. Calibrate the travel limit, or pick a shorter test distance |
| `Off by >20%, check` | The screw pitch correction is too large to be credible — usually a mistyped measurement |
| `Must be above 0` | A value that has to be positive was entered as zero or blank |
| `PPR must be 24-10000` | The encoder count was far outside any plausible resolution |
| `Span too small` | The travel limit span came to less than 1mm — the two marks were too close |
| `Below start speed` | A manual move speed below the axis start speed was entered |
| `machine is busy` (serial) | A setting was sent over USB while the controller was on or an axis was moving |

---

## If something goes wrong

**I made a wrong entry.** Press **minus** to step back one and redo it — you don't need to abort and repeat the physical moves. See [Redoing a step, and repeating a move](#redoing-a-step-and-repeating-a-move).

**I want to run that move again.** Press **plus**. See the same section for which routines allow it.

**Play or plus just beeps.** Either the axis is still moving — wait for it to stop — or there's nothing to advance/repeat at this step.

**A routine left the machine in a strange state.** Press stop. Every routine restores what it changed on abort, including backlash compensation and speed settings.

**A saved value looks wrong.** Every calibrated value also appears in the normal settings menu, where you can read and retype it. Screw pitch and backlash are items 6/7 and 4/5; max travel is 20/21.

**Positions are wrong after calibrating screw pitch.** Expected — the physical meaning of stored positions changed. Re-zero both axes.

**The readout doesn't match reality after hand-cranking.** This is normal and not a calibration fault. The position display shows commanded stepper travel, not a measured carriage position — there's no scale on the axes. Disable an axis and hand-crank it and the controller has no way to know. The large position display flags this with `Z OFF` or `X OFF` beside the affected axis. Re-zero after hand-cranking.
