# The getting-started sheet: what the controller can do, how to ask for it, and how to set a
# lathe up from scratch.
#
# Written for someone who has not used this controller before and may not have used a lathe much
# either. Plain language, the job before the mechanism, and nothing assumed beyond knowing which
# end of the machine the chuck is on.
#
# Style and machinery are in refsheet.py.  python quickref.py QUICKREF.pdf

import os
import re

from refsheet import *

HERE = os.path.dirname(os.path.abspath(__file__))


# ---------------------------------------------------------------------------
# Reference data, read out of the firmware itself
# ---------------------------------------------------------------------------
# Parsed at build time rather than copied, so the sheet cannot quietly disagree with the machine.
# Distances in the firmware are deci-microns: ten-thousandths of a millimetre.

def read_threads():
    src = open(os.path.join(HERE, "h4.ino"), encoding="utf-8", errors="replace").read()
    block = re.search(r"const ThreadPreset THREAD_PRESETS\[\] = \{(.*?)\n\};", src, re.S)
    out = []
    for name, du, measure in re.findall(r'\{"([^"]+)",\s*(\d+),\s*(MEASURE_\w+)\}', block.group(1)):
        out.append((name, int(du) / 10000.0, measure == "MEASURE_METRIC"))
    return out


def read_materials():
    src = open(os.path.join(HERE, "indexing.h"), encoding="utf-8", errors="replace").read()
    block = re.search(r"static const MaterialPreset materials\[\d+\] = \{(.*?)\n  \};", src, re.S)
    out = []
    for name, hss, carbide in re.findall(r'\{"([^"]+)",\s*(\d+),\s*(\d+)\}', block.group(1)):
        if name == "Manual":
            continue
        out.append((name, int(hss), int(carbide)))
    return out


# Thread depth on the radius, by form. The included angle decides it: 60 degrees for metric and
# Unified, 55 for Whitworth-form BSPP, and the flat-topped forms are half the pitch plus a
# clearance. NPT is a truncated 60 degree thread and deeper than it looks.
FORMS = [
    ("NPT", "60&#176; taper", lambda p: 0.800 * p),
    ("ACME", "29&#176;", lambda p: 0.5 * p + 0.254),
    ("Tr", "30&#176;", lambda p: 0.5 * p + 0.25),
    ("BSPP", "55&#176;", lambda p: 0.6403 * p),
    ("UN", "60&#176;", lambda p: 0.6134 * p),
    ("M", "60&#176;", lambda p: 0.6134 * p),
]

FAMILY_NAMES = {
    "M": "Metric",
    "UN": "Unified  ·  UNC and UNF",
    "BSPP": "British parallel pipe",
    "Tr": "Trapezoidal",
    "ACME": "ACME",
    "NPT": "American taper pipe",
}


def thread_form(name):
    """Which form a preset belongs to. Order matters - ACME contains an M."""
    for key, angle, depth in FORMS:
        if key in name:
            return key, angle, depth
    return "M", "60&#176;", FORMS[-1][2]


# ---------------------------------------------------------------------------
# Page 1 - what the machine can do and how to ask for it
# ---------------------------------------------------------------------------

def key_badge_table(rows):
    """A row of key -> what it selects, showing the panel's own icon."""
    data = []
    for icons, what in rows:
        data.append([icon_row(icons, 13), P("&#8594;", S_DIM), P(what, S_CELL)])
    t = Table(data, colWidths=[24 * mm, 6 * mm, None])
    t.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
        ("ALIGN", (0, 0), (0, -1), "CENTRE"),
        ("LEFTPADDING", (0, 0), (-1, -1), 4),
        ("RIGHTPADDING", (0, 0), (-1, -1), 4),
        ("TOPPADDING", (0, 0), (-1, -1), 2.6),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 2.6),
        ("LINEBELOW", (0, 0), (-1, -2), 0.4, LINE),
        ("BOX", (0, 0), (-1, -1), 0.6, LINE),
        ("BACKGROUND", (0, 0), (-1, -1), SOFT2),
    ]))
    return t


MODES = [
    ("Gearbox", "—", ("IconGears", False),
     "The carriage feeds along by itself as the spindle turns — what replaces change gears.",
     "not needed", "–"),
    ("Cross-feed", "XGEAR", ("IconGears", True),
     "The same, driving the cross slide across the face. Powered facing.",
     "not needed", "–"),
    ("Turning", "TURN", ("IconTurning", False),
     "Reduces a diameter over several passes, lifting the tool clear on each return.",
     "all four", "Passes · Spring passes · Clearance"),
    ("Facing", "FACE", ("IconFacing", False),
     "Cleans up or shortens the end of the work, in several passes.",
     "all four", "Passes · Spring passes · Clearance"),
    ("Parting", "CUT", ("IconParting", False),
     "Cuts the work off, or cuts a groove. Can back out to clear the swarf.",
     "up, down", "Passes · Peck depth · Clearance"),
    ("Threading", "THRD", ("IconThread", False),
     "Cuts a screw thread, deeper each pass. Pick it from the list of 52 and the feed is set.",
     "all four", "Passes · Spring passes · Flank infeed · Clearance · Thread list"),
    ("Tapered thread", "TPR", ("IconThread", True),
     "A thread on a taper rather than a parallel bar — pipe fittings, NPT and BSPT.",
     "all four", "as threading, plus its own taper"),
    ("Taper", "CONE", ("IconCone", False),
     "The tool moves in as it travels along, at a ratio you set. Feed in by hand.",
     "not needed", "taper ratio, asked at the start"),
    ("Ball and dish", "ELLI", ("IconM", False),
     "Rounds the end into a ball, or hollows it into a dish.",
     "all four", "Passes"),
    ("Slotting", "SLOT", ("IconM", False),
     "The lathe as a shaper: spindle still, tool stroking. Keyways and flats.",
     "all four", "Passes · Stroke shortening"),
    ("Steady feed", "ASY", ("IconM", False),
     "A set feed rate with the spindle out of it. Handy for polishing.",
     "not needed", "–"),
    ("G-code", "GCODE", ("IconM", False),
     "Runs a program written on a computer and sent over.",
     "not needed", "–"),
    ("Fourth axis", "A1", ("IconM", False),
     "Drives a fourth motor, if you have fitted one.",
     "not needed", "–"),
]


def page1():
    story = []

    story.append(Paragraph("What this is", S_H2))
    story.append(P("A controller that drives the carriage and the cross slide with stepper motors, "
                   "keeping them in step with the spindle. It does the job of change gears and a "
                   "lead screw, and adds automatic operations on top: mark the corners of the cut "
                   "and it will make the passes for you.", S_BODY))
    story.append(Spacer(1, 4))
    intro = [
        [P("Limits", S_CELL_B),
         P("Most jobs need you to mark the corners of the cut first — how far along, and how deep "
           "in — with the four limit keys. They are not guard rails: they <i>are</i> the cut.", S_CELL)],
        [P("Passes", S_CELL_B),
         P("A cut taken in stages. You say how many; it divides the depth between them and lifts "
           "the tool clear on the way back.", S_CELL)],
        [P("Feed", S_CELL_B),
         P("How far the tool moves per turn of the spindle. On a thread that is the pitch; on a "
           "turning cut it sets the finish.", S_CELL)],
        [P("It moves itself", S_CELL_B),
         P("Keep clear, know where the stop key is, and try anything new with the tool well away "
           "from the work.", S_CELL)],
        [P("Typing a distance", S_CELL_B),
         P("In millimetres, or inches in inch mode. <b>The backspace key is the decimal point</b> — "
           "type <b>4</b> for 4mm, <b>.5</b> for half a millimetre, <b>25.4</b> for an inch. Hold "
           "that key to clear the number and start again.", S_CELL)],
    ]
    story.append(numbered(intro, numw=20 * mm))
    story.append(Spacer(1, 7))

    story.append(Paragraph("Choosing what it does", S_H2))
    story.append(P("Each job has a key, shown in the table below. Two keys have a second job behind "
                   "them: press the same key again to reach it, marked <b>&#215;2</b>.", S_DIM))
    story.append(Spacer(1, 3))
    story.append(Table([[icon("IconM", 13),
                         P("steps through the rest:  "
                           "<font color='#0b6ec9'>fourth axis</font> (if fitted) &#8594; "
                           "<font color='#0b6ec9'>ball and dish</font> &#8594; "
                           "<font color='#0b6ec9'>G-code</font> &#8594; "
                           "<font color='#0b6ec9'>steady feed</font> &#8594; "
                           "<font color='#0b6ec9'>slotting</font> &#8594; back to the gearbox",
                           S_BODY)]],
                       colWidths=[16, None],
                       style=TableStyle([("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
                                         ("LEFTPADDING", (0, 0), (-1, -1), 0),
                                         ("TOPPADDING", (0, 0), (-1, -1), 0),
                                         ("BOTTOMPADDING", (0, 0), (-1, -1), 0)])))
    story.append(Spacer(1, 7))

    head = [P("Job", S_KEY), P("Shows", S_KEY), P("Key", S_KEY),
            P("What it is for", S_KEY), P("Limits", S_KEY), P("What you can adjust", S_KEY)]
    data = [head]
    for name, shown, key, what, stops, settings in MODES:
        data.append([P(name, S_CELL_B), P(shown, S_CELL), icon_key(key[0], key[1]),
                     P(what, S_CELL), P(stops, S_CELL), P(settings, S_CELL)])
    story.append(zebra(data, [22 * mm, 13 * mm, 15 * mm, None, 15 * mm, 43 * mm]))
    story.append(Spacer(1, 7))

    story.append(Paragraph("Running one of the automatic jobs", S_H2))
    wiz = [
        [P("1", S_CELL_B),
         icon_row(["IconLimitLeft", "IconLimitRight", "IconLimitUp", "IconLimitDown"], 10, gap=0.8),
         P("Set the feed, then move the tool to each corner of the cut and press the matching "
           "limit key. On a thread the <b>up</b> limit is full thread depth — see below.", S_CELL)],
        [P("2", S_CELL_B), icon_row(["IconPlay"], 10),
         P("The bottom line reads <b>ON to set up 3 steps</b>. Press play to begin.", S_CELL)],
        [P("3", S_CELL_B), icon_row(["IconArrowLeft", "IconArrowRight", "IconPlay"], 10, gap=0.8),
         P("It asks a short series of questions. <b>1/3</b> tells you which one you are on and how "
           "many there are. <b>&lt;&gt;</b> means the arrow keys change the answer; otherwise type "
           "a number. Play accepts and moves on.", S_CELL)],
        [P("4", S_CELL_B), icon_row(["IconPlay", "IconStop"], 10, gap=0.8),
         P("The last question is <b>Go?</b>, and it shows how far the tool will travel to reach "
           "the starting corner before it cuts anything. Play starts the job; stop takes you back "
           "to the beginning.", S_CELL)],
        [P("5", S_CELL_B), icon_row(["IconPlay", "IconStop"], 10, gap=0.8),
         P("While it runs the line reads <b>Pass 2 of 6</b>. Play skips to the next pass if you "
           "have taken enough off; stop, or any jog key, ends the job.", S_CELL)],
    ]
    story.append(numbered(wiz, iconw=23 * mm))
    story.append(Spacer(1, 5))
    story.append(P("<b>How deep should a thread go?</b> The controller sets the pitch from its "
                   "thread list, but it cannot know the depth — that is what the up limit tells it, "
                   "and getting it wrong is the usual reason a first thread comes out badly. For an "
                   "ordinary 60° thread the depth is <b>0.6134 × pitch</b> measured inwards from "
                   "the surface, so an M10×1.5 goes 0.92mm deep. If the cross slide is set to read "
                   "diameters rather than distance from centre, put in twice that.", S_NOTE))
    story.append(Spacer(1, 8))

    story.append(Paragraph("Reading the screen", S_H2))
    lcd_rows = [[P("THRD* off step 1.00", S_LCD)], [P("Pitch 1.50mm x2", S_LCD)],
                [P("Z 12.345 X 25.400", S_LCD)], [P("3/3 Go Z-5.00mm?", S_LCD)]]
    lcd = Table(lcd_rows, colWidths=[62 * mm])
    lcd.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, -1), LCDBG),
        ("LEFTPADDING", (0, 0), (-1, -1), 6), ("RIGHTPADDING", (0, 0), (-1, -1), 6),
        ("TOPPADDING", (0, 0), (-1, -1), 3), ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
        ("BOX", (0, 0), (-1, -1), 1.2, INK),
    ]))
    legend = [
        [P("Top", S_CELL_B), P("Which job is selected, whether it is running, a small mark for each "
                               "limit you have set, and how far one tap of a jog key moves.", S_CELL)],
        [P("Second", S_CELL_B), P("The feed, and the number of starts if the thread has more than "
                                  "one.", S_CELL)],
        [P("Third", S_CELL_B), P("Where the tool is. A slashed X means that figure is a diameter.",
                                 S_CELL)],
        [P("Bottom", S_CELL_B), P("Everything else — the setup questions, how many passes are done, "
                                  "why something was refused, and whichever readout you have turned "
                                  "on.", S_CELL)],
    ]
    lt = Table(legend, colWidths=[13 * mm, None])
    lt.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 3.5),
        ("TOPPADDING", (0, 0), (-1, -1), 2.2), ("BOTTOMPADDING", (0, 0), (-1, -1), 2.2),
        ("LINEBELOW", (0, 0), (-1, -2), 0.35, LINE),
    ]))
    pair = Table([[lcd, lt]], colWidths=[66 * mm, None])
    pair.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 0),
        ("RIGHTPADDING", (0, 0), (0, 0), 6 * mm), ("TOPPADDING", (0, 0), (-1, -1), 0),
    ]))
    story.append(pair)
    story.append(Spacer(1, 4))
    story.append(P("The <b>display</b> key changes what the bottom line shows when it is otherwise "
                   "idle: spindle angle, then spindle speed alongside the cutting speed at the "
                   "tool, then big position digits you can read from a few feet away, then nothing.",
                   S_DIM))
    return story


# ---------------------------------------------------------------------------
# Page 2 - setting the machine up
# ---------------------------------------------------------------------------

SETUP = [
    ("1", "Input tester", "nothing moves",
     "Press every key and check it registers. Confirms the panel works before anything is asked to move."),
    ("2", "Encoder signal", "nothing moves",
     "Watches the spindle sensor for electrical interference. Should sit at 100 while the spindle turns."),
    ("3", "Encoder PPR", "you turn the chuck",
     "Turn the spindle a set number of times by hand; it works out how finely the sensor reads."),
    ("4", "Encoder direction", "nothing moves",
     "Turn the spindle forwards. If the number counts down instead of up, this flips it."),
    ("5", "Z / X direction", "the axis moves",
     "Checks each axis travels the way you expect, before anything is measured against it."),
    ("6", "Z / X screw pitch", "the axis moves",
     "Moves a set distance; you measure what it actually moved, and it corrects itself to match."),
    ("7", "Z / X backlash", "the axis moves",
     "Finds the slack in each screw against a dial indicator, so it can be taken up automatically."),
    ("8", "Z / X travel limit", "you jog it",
     "Wind each axis to its ends so the controller knows how far it is allowed to go."),
    ("9", "Z / X max speed", "the axis moves",
     "Speeds up until the motor stalls, then backs off. Leave this one until last."),
]

SECTIONS = [
    ("How things behave", [
        ("X readout", "Whether the cross slide shows how far it is from centre, or the diameter "
                      "that gives — twice as much. Lathe work is usually quoted as a diameter."),
        ("Retract distance", "How far the tool jumps clear when you press the retract key."),
        ("Manual step time", "How long one tap of a jog key takes."),
        ("Step rest", "A pause between stepped moves."),
        ("Spindle divisions", "Turns the angle display into an indexer for cutting flats or "
                              "keyways. 0 is off; 6 gives you a hexagon."),
        ("Index tolerance", "How close to a mark counts as being on it."),
        ("Constant speed", "The single switch for the cutting-speed helper below."),
        ("Material", "What you are cutting. Sets a sensible cutting speed for you."),
        ("Tool", "High speed steel or carbide. Carbide takes roughly three times the speed."),
        ("Manual speed", "Your own figure, used when Material is set to Manual."),
        ("Spindle max rpm", "The fastest your lathe runs, so it never suggests more."),
    ]),
    ("Spindle sensor", [
        ("Encoder PPR", "How many pulses it gives per turn. Usually printed on the body."),
        ("Direction", "Flips the counting direction without rewiring anything."),
        ("Spindle / sensor pulley", "Tooth counts, if the sensor is belt driven rather than "
                                    "straight off the spindle."),
        ("Divider", "Ignores very small movements, for a sensor that twitches at rest."),
        ("Dead-band", "How far the spindle must turn back before the tool follows it."),
        ("Dead-band shape", "One-way suits a worn lead screw; symmetric suits a noisy sensor."),
        ("Glitch filter", "Ignores electrical spikes. Set too high it starts missing real pulses."),
    ]),
]

SECTIONS2 = [
    ("Each axis  ·  Z, X and the fourth", [
        ("Invert direction", "If the axis travels the wrong way."),
        ("Backlash", "The slack in the screw, taken up automatically whenever it reverses."),
        ("Lead screw pitch", "How far the axis moves for one turn of its screw."),
        ("Motor steps/rev", "From the motor and its driver settings together."),
        ("Motor / screw pulley", "Tooth counts, if the motor drives through a belt."),
        ("Start speed", "How gently it sets off before speeding up."),
        ("Max speed", "The fastest it will travel."),
        ("Acceleration", "How briskly it gets up to that speed."),
        ("Max travel", "How far the axis can physically go, used as a safety net."),
        ("Hold when idle", "Whether the motor stays locked when stopped. Turn off if it runs hot."),
        ("Fitted / Rotary", "Fourth axis only: whether it exists, and whether it turns or slides."),
    ]),
    ("Handwheels", [
        ("Fitted 1 / 2", "Whether each handwheel is connected."),
        ("Drives 1 / 2", "Which axis it winds."),
        ("Invert 1 / 2", "If it winds the wrong way."),
        ("Pulses per rev", "Of the handwheel itself."),
        ("Min pulse width", "Ignores electrical noise on its wiring."),
        ("Dead-band", "Stops a wobble being read as a change of direction."),
    ]),
    ("Joystick", [
        ("Fitted", "Uses all six spare terminals, so nothing else can share them."),
        ("Debounce", "Ignores the brief chatter a switch makes as it closes."),
    ]),
    ("WiFi", [
        ("Enabled", "Makes the controller its own small network. Off means the radio is not on at all."),
        ("Access PIN", "The password, exactly 8 digits. Change it from the default."),
    ]),
]


def page2():
    story = [Paragraph("Setting up a new machine", S_H2)]
    story.append(P("Flash the firmware, then choose millimetres or inches with the measure key. The "
                   "controller has to be told about your lathe before it can be accurate, and it "
                   "can measure most of that itself. <b>Hold</b> the settings key, choose "
                   "<b>Calibration</b>, press play, and work down this list — it begins with the "
                   "routines that move nothing and ends with the one that deliberately stalls a "
                   "motor.", S_BODY))
    story.append(Spacer(1, 3))
    head = [P("#", S_KEY), P("Routine", S_KEY), P("Does it move?", S_KEY), P("What it is for", S_KEY)]
    data = [head]
    for n, name, moves, what in SETUP:
        colour = GOOD if moves.startswith("nothing") else WARM
        data.append([P(n, S_CELL_B), P(name, S_CELL_B),
                     P("<font color='#%s'>%s</font>" % (colour.hexval()[2:], moves), S_CELL),
                     P(what, S_CELL)])
    story.append(zebra(data, [7 * mm, 33 * mm, 26 * mm, None]))
    story.append(Spacer(1, 4))
    story.append(P("<b>Then save what you have done — there is no undo.</b> Connect a computer over "
                   "USB, send a single <b>$</b>, and it prints every setting as a list of lines you "
                   "can keep in a text file. Paste them back to restore. The same file can be "
                   "downloaded from the web page.", S_NOTE))
    story.append(Spacer(1, 8))

    story.append(Paragraph("Settings that describe your lathe", S_H2))
    story.append(P("<b>Hold</b> the settings key to open the menu. Everything here describes the "
                   "machine itself, and you will mostly set it once. Anything to do with one "
                   "particular job — how many passes, how much clearance — lives on that job's own "
                   "page instead, which is a <b>short</b> press of the same key.", S_DIM))
    story.append(Spacer(1, 3))
    w = column_width()
    story.append(two_columns(
        stack([section_block(t, r, w) for t, r in SECTIONS]),
        stack([section_block(t, r, w) for t, r in SECTIONS2])))
    return story


# ---------------------------------------------------------------------------
# Page 3 - the tables worth having at the machine
# ---------------------------------------------------------------------------

def thread_columns():
    """The whole thread list, grouped by form, split into three balanced columns."""
    threads = read_threads()
    families = []
    for key in ("M", "UN", "BSPP", "Tr", "ACME", "NPT"):
        rows = [(n, p, m) for (n, p, m) in threads if thread_form(n)[0] == key]
        if rows:
            families.append((key, rows))

    # Balance by row count, keeping each family whole.
    total = sum(len(r) + 1 for _, r in families)
    target = total / 3.0
    cols, cur, used = [], [], 0
    for key, rows in families:
        if used and used + len(rows) + 1 > target and len(cols) < 2:
            cols.append(cur)
            cur, used = [], 0
        cur.append((key, rows))
        used += len(rows) + 1
    cols.append(cur)
    while len(cols) < 3:
        cols.append([])

    out = []
    for col in cols:
        data = [[P("Thread", S_KEY), P("Pitch", S_KEY), P("Depth", S_KEY)]]
        spans = []
        for key, rows in col:
            spans.append(len(data))
            data.append([P(FAMILY_NAMES[key] + "  &#183;  " + thread_form(rows[0][0])[1], S_CELL_B),
                         "", ""])
            for name, pitch, metric in rows:
                _, _, depth = thread_form(name)
                shown = "%.2fmm" % pitch if metric else "%dtpi" % round(25.4 / pitch)
                data.append([P(name, S_CELL), P(shown, S_CELL),
                             P("%.2f" % depth(pitch), S_CELL)])
        t = Table(data, colWidths=[None, 13 * mm, 10 * mm])
        st = [
            ("BACKGROUND", (0, 0), (-1, 0), INK),
            ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
            ("LEFTPADDING", (0, 0), (-1, -1), 3),
            ("RIGHTPADDING", (0, 0), (-1, -1), 3),
            ("TOPPADDING", (0, 0), (-1, -1), 1.5),
            ("BOTTOMPADDING", (0, 0), (-1, -1), 1.5),
            ("BOX", (0, 0), (-1, -1), 0.6, LINE),
            ("LINEBELOW", (0, 0), (-1, 0), 0.4, LINE),
        ]
        for r in spans:
            st.append(("SPAN", (0, r), (-1, r)))
            st.append(("BACKGROUND", (0, r), (-1, r), SOFT))
            st.append(("LINEABOVE", (0, r), (-1, r), 0.4, LINE))
        t.setStyle(TableStyle(st))
        out.append(t)
    return out


def page3():
    story = [Paragraph("Every thread it knows", S_H2)]
    story.append(P("Choose one from the machine and the feed is set for you. <b>Depth</b> is how far "
                   "in from the surface the tool has to go, in millimetres, measured on the radius "
                   "— that is what the up limit is for, and it is the one thing the controller "
                   "cannot work out for itself. If the cross slide is set to read diameters, put in "
                   "twice the figure shown.", S_BODY))
    story.append(Spacer(1, 3))

    cols = thread_columns()
    gap = 4 * mm
    colw = (W - 2 * MARGIN - 2 * gap) / 3.0
    t = Table([cols], colWidths=[colw, colw, colw])
    t.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 0),
        ("RIGHTPADDING", (0, 0), (1, 0), gap),
        ("RIGHTPADDING", (2, 0), (2, 0), 0),
        ("TOPPADDING", (0, 0), (-1, -1), 0),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 0),
    ]))
    story.append(t)
    story.append(Spacer(1, 4))
    story.append(P("Depths are for a full-form thread on the outside of a bar. An internal thread "
                   "is cut to the same depth outwards from the bore. Trapezoidal and ACME include "
                   "the usual root clearance; taper pipe threads are deeper than they look because "
                   "the form is truncated rather than pointed.", S_DIM))
    story.append(Spacer(1, 7))

    story.append(Paragraph("Cutting one of them", S_H2))
    tips = [
        [P("1", S_CELL_B), P("Pick the thread from the machine's own list — press the settings key "
                             "briefly while in threading, then choose it. That sets the feed.", S_CELL)],
        [P("2", S_CELL_B), P("Touch the tool on the outside of the bar and zero the cross slide. "
                             "Set the <b>down</b> limit there: that is the surface.", S_CELL)],
        [P("3", S_CELL_B), P("Wind in by the depth from the table and set the <b>up</b> limit. That "
                             "is full thread depth. In diameter readout, wind in twice the figure.", S_CELL)],
        [P("4", S_CELL_B), P("Set the two length limits, leaving room at the far end for the tool "
                             "to come out — a run-out groove if the thread ends against a shoulder.",
                             S_CELL)],
        [P("5", S_CELL_B), P("Six to ten passes suits most threads, plus a spring pass or two. More "
                             "passes means a lighter cut each time and a better finish.", S_CELL)],
    ]
    story.append(numbered(tips))
    return story


def page4():
    mats = read_materials()
    story = []
    story.append(Paragraph("Cutting speeds", S_H2))
    story.append(P("For setting the spindle yourself. Metres per minute measured at the surface of "
                   "the work, not the spindle speed — the two are only the same on a one-metre bar. "
                   "Conservative starting points for turning rather than limits: depth of cut, how "
                   "rigid the setup is and whether you are using coolant all matter as well. Work "
                   "up from these while the finish and the swarf still look happy, and drop to "
                   "roughly half for parting off.", S_BODY))
    story.append(Spacer(1, 3))

    half = (len(mats) + 1) // 2
    tables = []
    for chunk in (mats[:half], mats[half:]):
        data = [[P("Material", S_KEY), P("HSS", S_KEY), P("Carbide", S_KEY)]]
        for name, hss, carbide in chunk:
            data.append([P(name, S_CELL_B), P("%d m/min" % hss, S_CELL),
                         P("%d m/min" % carbide, S_CELL)])
        tables.append(zebra(data, [None, 20 * mm, 20 * mm]))
    story.append(two_columns(tables[0], tables[1]))
    story.append(Spacer(1, 6))

    # The grid does the arithmetic for you, which is the whole point when you are standing at the
    # machine with a bar in the chuck and a speed chart on the wall.
    story.append(Paragraph("Spindle speed for that cutting speed", S_H2))
    story.append(P("rpm = 318 &#215; cutting speed &#247; diameter in mm. Read it off here instead: "
                   "the diameter you are cutting down the side, the speed from the table above "
                   "across the top. Round down to whatever your lathe actually offers.", S_BODY))
    story.append(Spacer(1, 3))
    speeds = [20, 30, 60, 90, 120, 200]
    diams = [6, 10, 16, 20, 25, 32, 40, 50, 63, 80, 100]
    head = [P("&#216; mm", S_KEY)] + [P("%d" % s, S_KEY) for s in speeds]
    data = [head]
    for d in diams:
        row = [P("%d" % d, S_CELL_B)]
        for v in speeds:
            rpm = 318.3 * v / d
            row.append(P("%d" % (int(round(rpm / 10.0)) * 10 if rpm >= 100 else int(round(rpm / 5.0)) * 5),
                         S_CELL))
        data.append(row)
    colw = [16 * mm] + [None] * len(speeds)
    story.append(zebra(data, colw))
    story.append(Spacer(1, 3))
    story.append(P("Top row is metres per minute. Anything above your lathe's top speed simply "
                   "means run it flat out — small diameters usually want more than the machine has.",
                   S_DIM))
    story.append(Spacer(1, 8))

    # Taper ratios feed the cone and tapered-thread modes directly, and are the sort of number
    # nobody remembers.
    story.append(Paragraph("Taper ratios", S_H2))
    story.append(P("What to type when the taper or tapered-thread setup asks for a ratio. It is "
                   "the change in <b>diameter</b> per unit of length — (large &#8722; small) &#247; "
                   "length — so a taper quoted as 1 in 16 is entered as 0.0625.", S_BODY))
    story.append(Spacer(1, 3))
    import math
    tapers = [
        ("Morse 1", 0.04988, "tailstock and spindle tooling"),
        ("Morse 2", 0.04995, "the commonest on small lathes"),
        ("Morse 3", 0.05020, ""),
        ("Morse 4", 0.05194, ""),
        ("Morse 5", 0.05263, ""),
        ("NPT and BSPT", 0.0625, "taper pipe threads, 1 in 16"),
        ("1 in 10", 0.1000, ""),
        ("1 in 20", 0.0500, ""),
    ]
    half = (len(tapers) + 1) // 2
    tt = []
    for chunk in (tapers[:half], tapers[half:]):
        data = [[P("Taper", S_KEY), P("Enter", S_KEY), P("Included", S_KEY), P("Where", S_KEY)]]
        for name, ratio, where in chunk:
            deg = math.degrees(2 * math.atan(ratio / 2.0))
            data.append([P(name, S_CELL_B), P("%.4f" % ratio, S_CELL),
                         P("%d&#176; %d'" % (int(deg), round((deg - int(deg)) * 60)), S_CELL),
                         P(where or "&#8211;", S_CELL)])
        tt.append(zebra(data, [22 * mm, 14 * mm, 15 * mm, None]))
    story.append(two_columns(tt[0], tt[1]))
    story.append(Spacer(1, 3))
    story.append(P("Working from an angle instead: ratio = 2 &#215; tan(half the included angle). "
                   "Cutting a taper on the outside and a matching socket needs the same ratio for "
                   "both — the mode does not care which way round you are cutting.", S_DIM))
    story.append(Spacer(1, 8))

    story.append(Paragraph("Using it from a phone", S_H2))
    wifi = [
        [P("1", S_CELL_B), P("Settings &#8594; WiFi &#8594; <b>Enabled</b> = yes. Change the "
                             "<b>Access PIN</b> from the default first — anyone who can join the "
                             "network can change the machine's settings.", S_CELL)],
        [P("2", S_CELL_B), P("Join the <b>NanoEls-H4</b> network with that PIN, then open "
                             "<b>192.168.4.1</b>. The panel shows the address.", S_CELL)],
        [P("3", S_CELL_B), P("Every setting is there, quicker to type than on the panel, with "
                             "position and speed live. It also shows how clean the spindle sensor's "
                             "signal is — leave it open through a job and check afterwards.", S_CELL)],
        [P("4", S_CELL_B), P("New firmware installs from the same page, with no computer at the "
                             "machine. Only while it is stopped.", S_CELL)],
    ]
    story.append(numbered(wifi))
    return story


if __name__ == "__main__":
    import sys
    build(sys.argv[1], "NanoEls H4  ·  Getting Started",
          [page1(), page2(), page3(), page4()],
          ["What it does, and how to ask for it",
           "Setting up your lathe",
           "Threads it knows, and how to cut one",
           "Speeds, tapers and the web page"],
          "Settings key: short press = this job's settings   ·   hold it for the main menu")
