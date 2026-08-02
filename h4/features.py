# The features sheet: what this firmware adds over the one the H4 ships with, and everything it
# can do, plus notes on buying hardware for a conversion.
#
# Written for someone deciding whether to use this, or working out what their lathe needs. Says
# what a feature lets you do, not how it is implemented.
#
# The "what is new" list is not from memory: it was established by comparing this branch against
# the stock firmware it is built on (commit 5de1419) and checking each feature against it.
#
# Style and machinery are in refsheet.py.  python features.py FEATURES.pdf

from refsheet import *

# ---------------------------------------------------------------------------
# Page 1 - what this adds over the stock firmware
# ---------------------------------------------------------------------------

CHANGES = [
    ("Set it up without a computer", [
        ("Settings on the panel",
         "The stock firmware is configured by editing the source and reflashing it. Here every "
         "figure — screw pitches, motor steps, speeds, backlash — is a menu item on the machine."),
        ("It measures itself",
         "Fourteen guided routines work out the numbers for you on the actual lathe: how far each "
         "screw really moves, how much slack it has, how fast the motors can go before they stall, "
         "and how finely the spindle sensor reads."),
        ("Save and restore",
         "One command over USB prints every setting as text you can keep. Paste it back to restore "
         "a machine, or to set up a second one the same way."),
        ("From a phone",
         "The controller can make its own WiFi network, so you can change settings and update the "
         "firmware from a phone standing at the lathe. Off unless you switch it on."),
    ]),
    ("New things it can cut", [
        ("Powered cross-feed",
         "The gearbox driving the cross slide instead of the carriage, so facing cuts are fed "
         "evenly rather than by hand."),
        ("Slotting",
         "Uses the lathe as a shaper. The spindle stays still and the tool strokes back and forth "
         "— keyways, flats and squares without a milling machine."),
        ("Tapered threading",
         "Threads cut on a taper, for pipe fittings such as NPT and BSPT."),
        ("Thread list",
         "52 common threads built in — metric, UNC/UNF, BSPP, trapezoidal, ACME and NPT. Choose "
         "one and the feed is set for you."),
    ]),
    ("Better cuts", [
        ("Spring passes",
         "Repeats the last pass without going deeper, to take out the spring in the tool and get "
         "the size you actually asked for."),
        ("Angled thread infeed",
         "Feeds the tool in at an angle so it cuts mainly on one side of the thread instead of "
         "both at once. Easier on the tool and leaves a better finish."),
        ("Peck parting",
         "Backs the tool out every so often while parting off, to clear the swarf before it packs "
         "in and jams."),
        ("One-key retract",
         "Pulls the tool clear of the work and puts it back on exactly the same number."),
    ]),
]

CHANGES2 = [
    ("Easier to read", [
        ("Big position display",
         "Z and X in digits you can read from a few feet away, instead of squinting at the panel."),
        ("Diameter readout",
         "Shows the cross slide as a diameter, which is how lathe work is measured, rather than as "
         "distance from centre."),
        ("Cutting speed helper",
         "Tell it what you are cutting and with what, and it works out the spindle speed the "
         "current diameter wants — and keeps telling you as the diameter changes."),
        ("Spindle indexing",
         "Divides a turn of the spindle into equal marks and beeps as you reach each one. With "
         "slotting, that gives you hexagons and keyway sets without an indexer."),
    ]),
    ("A more trustworthy spindle sensor", [
        ("Twice the resolution",
         "Reads the sensor four times per pulse instead of twice, so it knows the spindle angle "
         "twice as precisely from the same encoder."),
        ("Sorted out in the menu",
         "Counting direction, belt ratios and noise filtering are all settings now, rather than "
         "wires to swap or numbers to work out by hand."),
        ("It tells you when the wiring is bad",
         "A live figure for how clean the signal is. Electrical interference from the motors is "
         "the usual cause of a thread going wrong, and nothing else on the machine can see it — "
         "a sensor twitching in place looks like a stopped spindle to everything else."),
    ]),
    ("Small things that help", [
        ("Settings per job",
         "How many passes, how much clearance and so on are remembered separately for each kind of "
         "job, so setting up a thread does not disturb your turning settings."),
        ("Numbers typed as you would write them",
         "4mm is one keystroke, not four. The stock firmware takes every distance in whole microns, "
         "so the smallest useful figure sets the length of every figure; here the backspace key is "
         "a decimal point, and it refuses one where a fraction cannot be stored."),
        ("Sounds that mean something",
         "A refused key, a finished pass and a lost thread each sound different, so you can keep "
         "your eyes on the work."),
        ("Optional joystick",
         "A stick for jogging the axes, if you would rather not use the keypad."),
        ("Fewer ways to get it wrong",
         "Four different things can be wired to the same six spare terminals; switching on one "
         "that would clash with another is refused and says which."),
    ]),
]


def page1():
    story = [Paragraph("What this adds", S_H2)]
    story.append(P("This is the standard NanoEls H4 firmware with a good deal added. Everything the "
                   "original does is still here and behaves the same way — the list below is what "
                   "is new. Roughly half the additions are about setting the machine up and "
                   "checking it is right, rather than about cutting metal.", S_BODY))
    story.append(Spacer(1, 5))
    w = column_width()
    story.append(two_columns(
        stack([section_block(t, r, w) for t, r in CHANGES]),
        stack([section_block(t, r, w) for t, r in CHANGES2])))
    story.append(Spacer(1, 8))

    story.append(Paragraph("If you are already running the standard firmware", S_H2))
    story.append(P("Your settings survive the update and the machine will behave as it did. Five "
                   "things are worth doing once afterwards.", S_DIM))
    story.append(Spacer(1, 3))
    steps = [
        [P("1", S_CELL_B), P("Save your settings first. Send <b>$</b> over USB and keep the text "
                             "somewhere — there is no undo.", S_CELL)],
        [P("2", S_CELL_B), P("Leave the encoder <b>PPR</b> figure alone. It still means the same "
                             "thing; the controller simply reads the sensor more finely and works "
                             "the rest out itself.", S_CELL)],
        [P("3", S_CELL_B), P("Check the <b>glitch filter</b>. It is a noise filter, and because the "
                             "sensor is now read twice as often, a setting that was safe before can "
                             "start losing pulses at high speed. There is a formula in the manual.",
                             S_CELL)],
        [P("4", S_CELL_B), P("Run <b>Encoder signal</b> from the calibration menu during a real cut "
                             "and see what it reads. It should sit at 100. Anything less is "
                             "interference on the sensor wiring, which is worth fixing properly "
                             "rather than filtering out.", S_CELL)],
        [P("5", S_CELL_B), P("Anything you used to change by editing the source is now in the "
                             "settings menu. The source file only supplies the starting values for "
                             "a controller that has never been set up.", S_CELL)],
    ]
    story.append(numbered(steps))
    return story


# ---------------------------------------------------------------------------
# Page 2 - everything it does, and choosing hardware
# ---------------------------------------------------------------------------

FEATURES = [
    ("Feeding the tool", [
        ("Screw-cutting gearbox", "Any feed or thread pitch, metric or imperial, without change gears."),
        ("Powered cross-feed", "The same, driving the cross slide."),
        ("Steady feed", "A fixed speed with the spindle out of it altogether."),
        ("Taper turning", "The tool moves in as it travels along, at a ratio you set."),
        ("Jogging", "Arrow keys, with a step size down to a hundredth of a millimetre."),
        ("Handwheels", "Up to two, each assignable to whichever axis you like."),
    ]),
    ("Jobs it will do for you", [
        ("Turning and facing", "To a size, in as many passes as you ask for."),
        ("Parting and grooving", "With optional pecking to clear the swarf."),
        ("Threading", "Straight or tapered, single or multi-start, from a list of 52 threads."),
        ("Slotting", "Keyways and flats, using the lathe as a shaper."),
        ("Balls and dishes", "Rounded ends, convex or concave."),
        ("G-code", "Programs written on a computer and sent over."),
        ("A fourth axis", "If you fit one — rotary or linear."),
    ]),
    ("Getting the size right", [
        ("Backlash taken up", "Automatically, whenever an axis reverses."),
        ("Limits", "Four marks that define the cut and cannot be overrun."),
        ("Belt ratios", "On any axis and on the spindle sensor."),
        ("Spring passes", "A repeat at size to take out tool spring."),
        ("Angled thread infeed", "Cuts on one flank rather than plunging on both."),
    ]),
]

FEATURES2 = [
    ("What it will tell you", [
        ("Where the tool is", "Millimetres or inches, radius or diameter."),
        ("Big digits", "Readable from across the shop."),
        ("Spindle speed and angle", "Plus the cutting speed at the tool."),
        ("What speed to run", "For the material and tool you have told it about."),
        ("Indexing", "Equal marks around a turn, with a beep at each."),
        ("Sensor health", "Whether the spindle sensor is being interfered with."),
        ("How far through", "Which pass of how many, with a bar."),
    ]),
    ("Setting it up", [
        ("Everything on the panel", "No reflashing to change a number."),
        ("Self-measurement", "Fourteen routines that work out your lathe's figures for you."),
        ("Thread list", "52 threads across six families."),
        ("Backup and restore", "One text file, over USB or from the browser."),
        ("From a phone", "The same settings, easier to type, with live readouts."),
        ("Firmware updates", "Over WiFi, with no computer at the machine."),
    ]),
    ("Keeping you out of trouble", [
        ("Emergency stop", "If it loses track of position, or a key is stuck at power-up."),
        ("It says why", "A refused action explains itself on screen and sounds its own tone."),
        ("Wiring clashes", "Two devices cannot claim the same terminals."),
        ("No changes mid-cut", "Settings will not change underneath a move in progress."),
    ]),
]

HARDWARE = [
    ("Controller", "The NanoEls H4 board: two stepper outputs, a spindle sensor input, six spare "
                   "terminals and a 20×4 display. Boards and the circuit files come from the "
                   "original project."),
    ("Spindle sensor", "An incremental rotary encoder, 600 to 1000 pulses per revolution, with two "
                       "output channels (A and B). Mount it straight on the spindle if you can, or "
                       "on a 1:1 belt. Gearing it to spin faster than the spindle means more pulses "
                       "to keep up with, and there is a speed ceiling — the manual has the sum. "
                       "Higher resolution is not automatically better."),
    ("Sensor wiring", "Shielded cable with twisted pairs, the shield earthed at one end only, run "
                      "well away from the motor and VFD cables. This matters more than which "
                      "encoder you buy. Interference here is the most common cause of a thread "
                      "coming out wrong, which is why the firmware watches for it."),
    ("Stepper motors", "Size them to the axis rather than the lathe. The carriage is heavy and "
                       "wants the larger motor; the cross slide moves far less and needs less. "
                       "Closed-loop motors cost more but remove the one fault the controller cannot "
                       "detect: a step it asked for that never happened."),
    ("Drivers", "400 to 800 steps per turn of the screw is plenty. Finer microstepping sounds "
                "better but costs torque and buys nothing the screw can actually deliver. If a "
                "motor runs hot standing still, turn off <b>Hold when idle</b> for that axis."),
    ("Lead screws", "Ball screws remove nearly all the slack and are worth it on the cross slide "
                    "first. Ordinary trapezoidal screws work fine — backlash is taken up "
                    "automatically — but compensation cannot rescue a screw that is worn unevenly "
                    "along its length. The backlash routine will tell you what you have."),
    ("Power supply", "24 to 48 volts for the motors, rated for both running at once. The controller "
                     "itself runs from 5 volts."),
]


def page2():
    story = [Paragraph("Everything it does", S_H2)]
    w = column_width()
    story.append(two_columns(
        stack([section_block(t, r, w) for t, r in FEATURES]),
        stack([section_block(t, r, w) for t, r in FEATURES2])))
    story.append(Spacer(1, 8))

    story.append(Paragraph("Buying hardware for a conversion", S_H2))
    story.append(P("General guidance rather than a shopping list — the right answer depends on your "
                   "lathe, and a motor that suits a small hobby machine will not move the carriage "
                   "on a big one. Two things get less attention than they deserve: the sensor "
                   "wiring, and whether your motors can be trusted not to lose steps.", S_DIM))
    story.append(Spacer(1, 3))
    data = [[P("Part", S_KEY), P("What to look for", S_KEY)]]
    for name, desc in HARDWARE:
        data.append([P(name, S_CELL_B), P(desc, S_CELL)])
    story.append(zebra(data, [30 * mm, None]))
    story.append(Spacer(1, 4))
    story.append(P("Run the calibration routines again after any mechanical change. The controller "
                   "only knows what it has been told or has measured, and swapping a screw or a "
                   "motor turns every stored figure back into a guess.", S_NOTE))
    return story


if __name__ == "__main__":
    import sys
    build(sys.argv[1], "NanoEls H4  ·  What It Does",
          [page1(), page2()],
          ["What this adds to the standard firmware", "Full list, and choosing hardware"],
          "Based on the NanoEls H4 firmware by kachurovskiy")
