# The features sheet: what this firmware adds over the stock H4, and everything it does.
#
# Style and machinery are in refsheet.py.  python features.py FEATURES.pdf
#
# The "new" column is not from memory: it was established by diffing this branch against the
# upstream commit it is built on (5de1419) and checking whether each identifier exists there.
# Upstream h4.ino is 3,074 lines; this branch adds 9,438 across 21 files.

from refsheet import *

# ---------------------------------------------------------------------------
# Page 1 - what is different
# ---------------------------------------------------------------------------

CHANGES = [
    ("Spindle encoder", [
        ("Four times the resolution",
         "Stock reads one channel at two edges per pulse. This reads both channels at four, so a "
         "1000 PPR encoder resolves 4000 positions instead of 2000 — 0.09° rather than 0.18°."),
        ("Direction in software",
         "Reverses the count from the menu instead of swapping the A and B wires."),
        ("Belt and gear ratios",
         "Spindle-to-encoder teeth are a setting, so a geared encoder no longer has to be folded "
         "into the PPR figure by hand."),
        ("Divider and dead-band shape",
         "Two ways to quieten a fluttering encoder. The dead-band can filter both directions "
         "rather than only the reverse."),
        ("Live signal quality",
         "Coherence compares how far the counter travelled against how far it moved. A dithering "
         "encoder reads as a stopped spindle on every other readout; this is the one that sees it."),
    ]),
    ("New modes", [
        ("XGEAR — power cross-feed",
         "The electronic gearbox driving the cross slide instead of the carriage, for powered facing."),
        ("SLOT — the lathe as a shaper",
         "The spindle is not involved. Z strokes back and forth while X steps down, for keyways and flats."),
        ("TPR — tapered threading",
         "X drifts across as Z advances, so the thread grows on a cone. NPT and BSPT."),
    ]),
    ("Cutting", [
        ("Spring passes", "Repeats the final pass at depth to take out deflection."),
        ("Flank infeed",
         "Phases the tool along the helix so it cuts mostly on the leading flank instead of "
         "plunging on both."),
        ("Peck parting", "Retracts periodically to break the chip."),
        ("Thread database", "52 presets — metric, UN, BSPP, trapezoidal, ACME and NPT."),
        ("One-key X retract", "Pulls the tool clear and puts it back on the same number."),
    ]),
]

CHANGES2 = [
    ("Readouts", [
        ("Large position display", "Z and X in two-row digits, readable across the shop."),
        ("Diameter mode for X", "Type and read diameters rather than radii."),
        ("Spindle indexing", "Divides a revolution into marks and beeps as you reach each one."),
        ("Constant cutting speed",
         "Says what rpm the current diameter wants, from a material and tool table. Advisory: "
         "there is no spindle output on this board."),
    ]),
    ("Configuration", [
        ("Settings menu", "90 items on the panel. Stock has none — you edited the sketch and reflashed."),
        ("Per-mode settings", "Passes, spring passes, clearance and the rest belong to the operation."),
        ("Calibration routines",
         "Fourteen guided measurements — screw pitch, backlash, travel, max speed, encoder PPR "
         "and direction — that measure rather than assume."),
        ("machine_config.h", "Every pin and machine constant in one file, out of the sketch."),
        ("Serial interface", "$ dumps every setting for backup; $key=value writes one."),
    ]),
    ("Connectivity", [
        ("WiFi access point", "The controller makes its own network. Off by default: no radio at all."),
        ("Web configuration", "Every setting from a phone at the lathe, plus a live status strip."),
        ("Firmware over the air", "Update without a laptop. Refused unless the machine is stopped."),
    ]),
    ("Inputs and safety", [
        ("Joystick", "Optional stick on the auxiliary terminals for jogging."),
        ("Terminal conflict checking",
         "Four devices want the same six pins. Enabling one over another's is refused rather than "
         "quietly configuring the pins twice."),
        ("Runtime enable", "Handwheels and the stick switch on without a restart."),
        ("Distinct buzzer patterns", "A refused key, a finished pass and a lost thread sound different."),
    ]),
    ("Underneath", [
        ("Host test suite",
         "777 checks on the arithmetic and 42 on the web page, run on a PC without a board. "
         "The firmware calls the same code the tests do."),
    ]),
]


def page1():
    story = [Paragraph("What this firmware adds", S_H2)]
    story.append(P("Built on kachurovskiy/nanoels H4 at commit <b>5de1419</b>. Everything the stock "
                   "firmware does is still here and works the same way — the list below is what is "
                   "new. Upstream is 3,074 lines; this adds 9,438 across 21 files, about half of it "
                   "tests and documentation.", S_BODY))
    story.append(Spacer(1, 5))
    w = column_width()
    story.append(two_columns(
        stack([section_block(t, r, w) for t, r in CHANGES]),
        stack([section_block(t, r, w) for t, r in CHANGES2])))
    story.append(Spacer(1, 8))

    story.append(Paragraph("Coming from the stock firmware", S_H2))
    story.append(P("Nothing here changes how the stock features behave, and your stored settings "
                   "survive the update. Five things are worth doing once.", S_DIM))
    story.append(Spacer(1, 3))
    steps = [
        [P("1", S_CELL_B), P("Take a backup first. Send <b>$</b> over USB and keep the text — there "
                             "is no undo in the settings menu.", S_CELL)],
        [P("2", S_CELL_B), P("Leave the encoder <b>PPR</b> as it is. It still means pulses per "
                             "revolution as marked; the firmware reads four counts from each rather "
                             "than two, and works the rest out itself.", S_CELL)],
        [P("3", S_CELL_B), P("Check the <b>glitch filter</b>. The counter now sees twice the traffic, "
                             "and the usable ceiling is <b>2.4e9 / (PPR × filter)</b> encoder rpm — "
                             "a geared-up encoder spins faster than the spindle, so account for the "
                             "belt too.", S_CELL)],
        [P("4", S_CELL_B), P("Run <b>Encoder signal</b> from the calibration menu and read the "
                             "coherence figure during a real cut. It is the one readout that can "
                             "tell you the cable is the problem.", S_CELL)],
        [P("5", S_CELL_B), P("Everything that used to mean editing the sketch is now in the settings "
                             "menu. <b>machine_config.h</b> only sets the defaults for a controller "
                             "that has never been configured.", S_CELL)],
    ]
    story.append(numbered(steps))
    return story


# ---------------------------------------------------------------------------
# Page 2 - everything it does, and choosing hardware
# ---------------------------------------------------------------------------

FEATURES = [
    ("Feeding", [
        ("Electronic lead screw", "Carriage feeds from the spindle at any pitch, metric or imperial."),
        ("Power cross-feed", "The same, driving X."),
        ("Async feed", "A fixed feed rate with no spindle involved."),
        ("Cone", "Constant ratio between X and Z for continuous taper turning."),
        ("Manual jogging", "Arrow keys, with a selectable step down to 0.01mm."),
        ("Handwheels", "Up to two pulse generators, each assignable to an axis."),
    ]),
    ("Automated operations", [
        ("Turning and facing", "Multi-pass to a diameter or a face, returning between passes."),
        ("Parting and grooving", "Feeds X in, with optional pecking."),
        ("Threading", "Single-point, multi-pass, multi-start, straight or tapered."),
        ("Slotting", "Shaper strokes for keyways and flats."),
        ("Half-spheres and ellipses", "Convex or concave, over multiple passes."),
        ("G-code", "Stored programs sent from the browser or over USB."),
        ("Fourth axis", "An optional A1 axis, rotary or linear."),
    ]),
    ("Precision", [
        ("Backlash compensation", "Per axis, taken up automatically on a reversal."),
        ("Soft limits", "Four stops that define the cut and cannot be passed."),
        ("Gear and pulley ratios", "Belt drives on any axis and on the encoder."),
        ("Spring passes", "Repeat at depth to take out deflection."),
        ("Flank infeed", "Cut on the leading flank rather than plunging."),
    ]),
]

FEATURES2 = [
    ("What you can see", [
        ("Position readout", "Z and X, metric or inch, radius or diameter."),
        ("Large display", "Two-row digits for reading across the shop."),
        ("Spindle angle and rpm", "Plus surface speed at the tool."),
        ("Constant speed target", "The rpm the current diameter wants for the material set."),
        ("Spindle indexing", "Divide a revolution into marks, with a beep at each."),
        ("Encoder signal health", "Coherence and its low-water mark, live."),
        ("Pass progress", "Which pass of how many, with a bar."),
    ]),
    ("Setting it up", [
        ("Settings menu", "Everything adjustable on the panel — no reflashing."),
        ("Calibration routines", "Fourteen guided measurements on the actual lathe."),
        ("Thread database", "52 presets across six thread families."),
        ("Backup and restore", "One text dump over USB or from the browser."),
        ("Web interface", "The same settings from a phone, with live status."),
        ("Firmware over the air", "No laptop at the machine."),
    ]),
    ("Looking after itself", [
        ("Emergency stop", "On a lost position, a key stuck at power-up, or a demanded overtravel."),
        ("Refusals with reasons", "A rejected input says why on screen and sounds its own tone."),
        ("Terminal conflicts", "Two devices cannot claim the same pins."),
        ("Motion lock on writes", "A setting cannot change underneath a move in progress."),
    ]),
]

HARDWARE = [
    ("Controller", "The NanoEls H4 board — an ESP32-S3 with two stepper outputs, an encoder input, "
                   "six auxiliary terminals and a 20×4 display. Boards and the PCB files are on the "
                   "upstream project."),
    ("Spindle encoder", "An incremental A/B quadrature encoder, 600–1000 PPR. Mount it on the "
                        "spindle directly or on a 1:1 belt if you can. Push-pull output is easier "
                        "than open-collector. Gearing it up multiplies the pulse rate, and the "
                        "usable ceiling is <b>2.4e9 / (PPR × glitch filter)</b> encoder rpm — check "
                        "that figure before choosing a high-count encoder."),
    ("Encoder cable", "Shielded, twisted pairs, shield grounded at one end only, run away from the "
                      "VFD and motor leads. This matters more than the encoder does: the coherence "
                      "readout exists because noise here is the usual cause of a bad thread."),
    ("Stepper motors", "Size to the axis, not the lathe. Z has to pull the carriage and wants the "
                       "larger motor; X moves far less mass. Closed-loop steppers cost more and "
                       "remove the one failure this firmware cannot detect — a step commanded and "
                       "not taken."),
    ("Drivers", "Microstepping of 400–800 steps per screw revolution is plenty; more costs torque "
                "and buys nothing the lead screw can deliver. Set <b>Hold when idle</b> to off for "
                "open-loop drivers that run hot holding position."),
    ("Lead screws", "Ball screws remove most backlash and are worth it on X. Trapezoidal screws "
                    "work — backlash is compensated in software, but compensation cannot help a "
                    "screw that is also worn unevenly. Measure it with the backlash routine."),
    ("Power", "24–48V for the steppers, sized for both at once. The controller runs from 5V."),
]


def page2():
    story = [Paragraph("Everything it does", S_H2)]
    w = column_width()
    story.append(two_columns(
        stack([section_block(t, r, w) for t, r in FEATURES]),
        stack([section_block(t, r, w) for t, r in FEATURES2])))
    story.append(Spacer(1, 8))

    story.append(Paragraph("Choosing hardware for a conversion", S_H2))
    story.append(P("General guidance rather than a parts list — the right answer depends on the "
                   "lathe. Two things are worth more attention than they usually get: the encoder "
                   "cable, and whether your steppers can be trusted not to lose steps.", S_DIM))
    story.append(Spacer(1, 3))
    data = [[P("Part", S_KEY), P("What to look for", S_KEY)]]
    for name, desc in HARDWARE:
        data.append([P(name, S_CELL_B), P(desc, S_CELL)])
    story.append(zebra(data, [30 * mm, None]))
    story.append(Spacer(1, 4))
    story.append(P("Run the calibration routines after any mechanical change — the firmware only "
                   "knows what it has been told or has measured, and a screw or motor swap makes "
                   "every stored figure a guess.", S_NOTE))
    return story


if __name__ == "__main__":
    import sys
    build(sys.argv[1], "NanoEls H4  ·  Features",
          [page1(), page2()],
          ["What is different from the stock firmware", "Full feature list and hardware notes"],
          "Built on kachurovskiy/nanoels H4 at 5de1419")
