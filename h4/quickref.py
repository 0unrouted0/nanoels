# The modes sheet: how to reach every mode, what each is for, and what it can be told.
#
# Style and machinery are in refsheet.py.  python quickref.py QUICKREF.pdf

from refsheet import *

# --------------------------------------------------------------------------
# Page 1 - modes
# --------------------------------------------------------------------------

def key_badge_table(rows):
    """A row of key -> what it selects, showing the panel's own icon."""
    data = []
    for icons, what in rows:
        data.append([icon_row(icons, 13), P("→", S_DIM), P(what, S_CELL)])
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
     "Electronic lead screw. Carriage feeds from the spindle at the set pitch.",
     "none", "–"),
    ("XGEAR", "XGEAR", ("IconGears", True),
     "Power cross-feed. Same gearbox, driving the cross slide instead.",
     "none", "–"),
    ("Turning", "TURN", ("IconTurning", False),
     "Turns to a diameter over several passes, returning between each.",
     "all four", "Passes · Spring passes · Clearance"),
    ("Facing", "FACE", ("IconFacing", False),
     "Faces the end over several passes. Direction is set in the wizard, not by pitch sign.",
     "all four", "Passes · Spring passes · Clearance"),
    ("Parting", "CUT", ("IconParting", False),
     "Parting and grooving: feeds X in, optionally pecking to break the chip.",
     "X only", "Passes · Peck depth · Clearance"),
    ("Threading", "THRD", ("IconThread", False),
     "Single-point threading, multi-pass and multi-start. Up limit is full thread depth.",
     "all four", "Passes · Spring passes · Flank infeed · Clearance · Thread database"),
    ("Tapered thread", "TPR", ("IconThread", True),
     "Threading on a cone — NPT, BSPT. Asks for the taper as an extra wizard step.",
     "all four", "as threading, plus its own taper"),
    ("Cone", "CONE", ("IconCone", False),
     "Continuous taper turning at a set ratio. Feed in by hand between passes.",
     "none", "taper ratio, asked in the wizard"),
    ("Ellipse", "ELLI", ("IconM", False),
     "Convex or concave half-spheres and half-ellipses over multiple passes.",
     "all four", "Passes"),
    ("Slotting", "SLOT", ("IconM", False),
     "The lathe as a shaper. Spindle is not involved: Z strokes while X steps down.",
     "all four", "Passes · Left reduction"),
    ("Async", "ASY", ("IconM", False),
     "Feed at a fixed rate, not synchronised to the spindle. Pitch means mm per second.",
     "none", "–"),
    ("G-code", "GCODE", ("IconM", False),
     "Runs a stored program sent from the browser or over USB.",
     "none", "–"),
    ("A1 axis", "A1", ("IconM", False),
     "Jogs and positions the fourth axis. Only in the cycle when A1 is fitted.",
     "none", "–"),
]

def page1():
    story = []
    story.append(Paragraph("Getting to a mode", S_H2))
    story.append(P("Six buttons select a mode directly. Pressing the same button again reaches its "
                   "hidden sibling where it has one.", S_DIM))
    story.append(Spacer(1, 3))
    story.append(key_badge_table([
        (["IconGears"], "Gearbox  ·  press again for XGEAR (cross-feed)"),
        (["IconTurning"], "Turning"),
        (["IconFacing"], "Facing"),
        (["IconParting"], "Parting"),
        (["IconCone"], "Cone"),
        (["IconThread"], "Threading  ·  press again for TPR (tapered)"),
    ]))
    story.append(Spacer(1, 5))
    story.append(Table([[icon("IconM", 13), P("cycles the rest:  "
                   "<font color='#0b6ec9'>A1</font> (if fitted) → "
                   "<font color='#0b6ec9'>ELLI</font> → "
                   "<font color='#0b6ec9'>GCODE</font> → "
                   "<font color='#0b6ec9'>ASY</font> → "
                   "<font color='#0b6ec9'>SLOT</font> → back to the gearbox", S_BODY)]], colWidths=[16, None], style=TableStyle([("VALIGN",(0,0),(-1,-1),"MIDDLE"),("LEFTPADDING",(0,0),(-1,-1),0),("TOPPADDING",(0,0),(-1,-1),0),("BOTTOMPADDING",(0,0),(-1,-1),0)])))
    story.append(Spacer(1, 7))

    story.append(Paragraph("The modes", S_H2))
    head = [P("Mode", S_KEY), P("On screen", S_KEY), P("Key", S_KEY),
            P("What it is for", S_KEY), P("Stops", S_KEY), P("Its own settings", S_KEY)]
    data = [head]
    for name, shown, key, what, stops, settings in MODES:
        data.append([P(name, S_CELL_B), P(shown, S_CELL), icon_key(key[0], key[1]),
                     P(what, S_CELL), P(stops, S_CELL), P(settings, S_CELL)])
    t = Table(data, colWidths=[22 * mm, 14 * mm, 17 * mm, None, 12 * mm, 46 * mm], repeatRows=1)
    st = [
        ("BACKGROUND", (0, 0), (-1, 0), INK),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 3.5),
        ("RIGHTPADDING", (0, 0), (-1, -1), 3.5),
        ("TOPPADDING", (0, 0), (-1, -1), 3),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
        ("LINEBELOW", (0, 0), (-1, -1), 0.4, LINE),
        ("BOX", (0, 0), (-1, -1), 0.6, LINE),
    ]
    for i in range(1, len(data)):
        if i % 2 == 0:
            st.append(("BACKGROUND", (0, i), (-1, i), SOFT2))
    t.setStyle(TableStyle(st))
    story.append(t)
    story.append(Spacer(1, 7))

    story.append(Paragraph("Running an automated mode", S_H2))
    wiz = [
        [P("1", S_CELL_B),
         icon_row(["IconLimitLeft", "IconLimitRight", "IconLimitUp", "IconLimitDown"], 10, gap=0.8, joiner=""),
         P("Set the pitch, then the soft limits. The limits are the cut — for a "
           "thread the up limit is full depth.", S_CELL)],
        [P("2", S_CELL_B), icon_row(["IconPlay"], 10),
         P("The bottom line reads <b>ON to set up 3 steps</b>. Press play.", S_CELL)],
        [P("3", S_CELL_B), icon_row(["IconArrowLeft", "IconArrowRight", "IconPlay"], 10, gap=0.8, joiner=""),
         P("Answer each step. <b>1/3</b> is where you are and how many there are. "
           "<b>&lt;&gt;</b> means the arrows change the answer; a number is typed "
           "on the numpad. Play accepts.", S_CELL)],
        [P("4", S_CELL_B), icon_row(["IconPlay", "IconStop"], 10, gap=0.8, joiner=""),
         P("The last step is always <b>Go?</b>, showing how far the tool moves to "
           "reach the starting corner. Play starts it; stop returns to the beginning.", S_CELL)],
        [P("5", S_CELL_B), icon_row(["IconPlay", "IconStop"], 10, gap=0.8, joiner=""),
         P("While running the line reads <b>Pass 2 of 6</b> with a progress bar. "
           "Play skips to the next pass; stop or a jog button ends it.", S_CELL)],
    ]
    tw = Table(wiz, colWidths=[7 * mm, 23 * mm, None])
    tw.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 3.5),
        ("TOPPADDING", (0, 0), (-1, -1), 2.4),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 2.4),
        ("BACKGROUND", (0, 0), (-1, -1), SOFT),
        ("BOX", (0, 0), (-1, -1), 0.6, LINE),
        ("LINEBELOW", (0, 0), (-1, -2), 0.4, colors.white),
    ]))
    story.append(tw)
    story.append(Spacer(1, 5))
    story.append(P("Thread depth is yours to set: the controller sets the pitch from the database "
                   "but cannot know how deep. For a 60° thread the depth on the <b>radius</b> is "
                   "0.6134 × pitch — M10×1.5 is 0.92mm. In diameter readout mode, enter twice that.",
                   S_NOTE))
    story.append(Spacer(1, 9))

    story.append(Paragraph("Reading the screen", S_H2))
    lcd_rows = [
        [P("THRD* off  step 1.00", S_LCD)],
        [P("Pitch 1.50mm x2", S_LCD)],
        [P("Z 12.345  X 25.400", S_LCD)],
        [P("3/3 Go Z-5.00mm?", S_LCD)],
    ]
    lcd = Table(lcd_rows, colWidths=[62 * mm])
    lcd.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, -1), LCDBG),
        ("LEFTPADDING", (0, 0), (-1, -1), 6),
        ("RIGHTPADDING", (0, 0), (-1, -1), 6),
        ("TOPPADDING", (0, 0), (-1, -1), 3),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
        ("BOX", (0, 0), (-1, -1), 1.2, INK),
    ]))

    legend = [
        [P("Line 1", S_CELL_B), P("Mode, on/off, then a glyph for each soft limit that is set, and the jog step. "
                                  "<b>*</b> means the key has a second mode behind it. During setup "
                                  "it shows the distance between the stops instead.", S_CELL)],
        [P("Line 2", S_CELL_B), P("Pitch, and the number of starts if more than one.", S_CELL)],
        [P("Line 3", S_CELL_B), P("Z and X positions. A slashed X means the readout is diameter, not radius.", S_CELL)],
        [P("Line 4", S_CELL_B), P("Everything else: the setup wizard, pass progress, refusal messages, "
                                  "and whichever readout the display button has selected.", S_CELL)],
    ]
    lt = Table(legend, colWidths=[13 * mm, None])
    lt.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 3.5),
        ("TOPPADDING", (0, 0), (-1, -1), 2.2),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 2.2),
        ("LINEBELOW", (0, 0), (-1, -2), 0.35, LINE),
    ]))

    pair = Table([[lcd, lt]], colWidths=[66 * mm, None])
    pair.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (0, 0), 0),
        ("RIGHTPADDING", (0, 0), (0, 0), 6 * mm),
        ("LEFTPADDING", (1, 0), (1, 0), 0),
        ("RIGHTPADDING", (1, 0), (1, 0), 0),
        ("TOPPADDING", (0, 0), (-1, -1), 0),
    ]))
    story.append(pair)
    story.append(Spacer(1, 4))
    story.append(P("The <b>display</b> button cycles what line 4 shows when nothing else needs it: "
                   "spindle angle → rpm with cutting speed → large two-line position readout → off. "
                   "Spindle divisions replaces the angle with an index readout; constant speed "
                   "replaces the cutting speed with the rpm you should be at.", S_DIM))
    return story

# --------------------------------------------------------------------------
# Page 2 - setup and global settings
# --------------------------------------------------------------------------

SETUP = [
    ("1", "Input tester", "no movement", "Every key reports a code. Confirms the panel before anything moves."),
    ("2", "Encoder signal", "no movement", "Coherence should read 100 while the spindle turns. Fix the cable, not the filter."),
    ("3", "Encoder PPR", "you hand-turn", "Counts through a known number of turns and derives the pulses per revolution."),
    ("4", "Encoder direction", "no movement", "Turn the spindle forward; the count must rise."),
    ("5", "Z / X direction", "axis moves", "Confirms which way each axis travels before any measured move."),
    ("6", "Z / X screw pitch", "axis moves", "Moves a known distance, you measure it, the pitch is corrected."),
    ("7", "Z / X backlash", "axis moves", "Takes up the slack against an indicator to find the lost motion."),
    ("8", "Z / X travel limit", "you jog", "Jog to each end to record the usable travel."),
    ("9", "Z / X max speed", "axis moves", "Ramps until the motor stalls, then backs off. Do this last."),
]

SECTIONS = [
    ("Preferences", [
        ("X readout", "Radius or diameter. Diameter doubles every X figure you type and see."),
        ("Retract distance", "How far the one-key X retract pulls the tool clear."),
        ("Manual step time", "How long one tap of a jog key takes."),
        ("Step rest", "Pause between stepped moves."),
        ("Spindle divisions", "Turns the angle readout into an indexer. 0 is off, 6 gives hex flats."),
        ("Index tolerance", "How close counts as on the mark, in tenths of a degree."),
        ("Constant speed", "The single switch for the cutting-speed readout."),
        ("Material", "Picks the speed from a table. Manual means use the figure below."),
        ("Tool", "HSS or carbide. Carbide runs roughly three times as fast."),
        ("Manual speed", "Surface speed when Material is Manual. Follows metric/inch."),
        ("Spindle max rpm", "Your lathe's ceiling, so an unreachable target is shown capped."),
    ]),
    ("Spindle encoder", [
        ("Encoder PPR", "Pulses per revolution as marked. Read four times over."),
        ("Direction", "Reverses the count without swapping wires."),
        ("Spindle / Encoder pulley", "Teeth either end of the belt, if the encoder is geared."),
        ("Divider", "Folds several counts into one step to steady a fluttering encoder."),
        ("Dead-band", "How far the count must reverse before the axes follow."),
        ("Dead-band shape", "One-way models screw backlash; symmetric filters both directions."),
        ("Glitch filter", "Noise rejection. Too high and real pulses are dropped at speed."),
    ]),
    ("Z axis  /  X axis  /  A1 axis", [
        ("Invert direction", "If the axis moves the wrong way."),
        ("Backlash", "Lost motion, taken up automatically on a reversal."),
        ("Lead screw pitch", "Travel per screw revolution."),
        ("Motor steps/rev", "Including microstepping."),
        ("Motor / Lead screw pulley", "Teeth either end, for a belt-driven axis."),
        ("Start speed", "Where the acceleration ramp begins."),
        ("Max speed", "Ceiling for rapid and manual moves."),
        ("Acceleration", "Steps per second squared."),
        ("Max travel", "Used for the emergency stop limit."),
        ("Hold when idle", "Off for open-loop drivers that get hot holding position."),
        ("Fitted / Rotary", "A1 only: whether it exists, and whether it turns rather than slides."),
    ]),
    ("Handwheels", [
        ("Fitted 1 / 2", "Whether each pulse generator is connected."),
        ("Drives 1 / 2", "Which axis it turns."),
        ("Invert 1 / 2", "If it drives the wrong way."),
        ("Pulses per rev", "Of the handwheel itself."),
        ("Min pulse width", "Noise rejection, in microseconds."),
        ("Dead-band", "Stops a reversal being read from a wobble."),
    ]),
    ("Joystick", [
        ("Fitted", "Needs all six auxiliary terminals, so it conflicts with everything else on them."),
        ("Debounce", "Contact changes quicker than this are switch bounce."),
    ]),
    ("WiFi and updates", [
        ("Enabled", "Brings up the machine's own access point. Off means no radio at all."),
        ("Access PIN", "The WPA2 password, exactly 8 digits. Change it from the default."),
    ]),
]

def page2():
    story = []
    story.append(Paragraph("First-time setup", S_H2))
    story.append(P("Flash the firmware, then pick metric or inch with the measure button. Open the "
                   "calibration routines: <b>hold</b> the settings button → <b>Calibration</b> → play. "
                   "Work down this list — it runs from zero risk to most, and each routine is more "
                   "trustworthy once the ones above it are right.", S_BODY))
    story.append(Spacer(1, 3))

    head = [P("#", S_KEY), P("Routine", S_KEY), P("Moves?", S_KEY), P("What it does", S_KEY)]
    data = [head]
    for n, name, moves, what in SETUP:
        colour = GOOD if moves == "no movement" else WARM
        data.append([P(n, S_CELL_B), P(name, S_CELL_B),
                     P("<font color='#%s'>%s</font>" % (colour.hexval()[2:], moves), S_CELL),
                     P(what, S_CELL)])
    t = Table(data, colWidths=[7 * mm, 34 * mm, 24 * mm, None])
    st = [
        ("BACKGROUND", (0, 0), (-1, 0), INK),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 3.5),
        ("TOPPADDING", (0, 0), (-1, -1), 2.4),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 2.4),
        ("BOX", (0, 0), (-1, -1), 0.6, LINE),
        ("LINEBELOW", (0, 0), (-1, -1), 0.4, LINE),
    ]
    for i in range(1, len(data)):
        if i % 2 == 0:
            st.append(("BACKGROUND", (0, i), (-1, i), SOFT2))
    t.setStyle(TableStyle(st))
    story.append(t)
    story.append(Spacer(1, 4))
    story.append(P("Then back the settings up — there is no undo. Send <b>$</b> over USB to dump every "
                   "setting as key=value lines, or download the same file from the web page. Paste the "
                   "lines back to restore.", S_NOTE))
    story.append(Spacer(1, 8))

    story.append(Paragraph("Global settings", S_H2))
    story.append(P("<b>Hold</b> the settings button to open the menu. These belong to the machine; "
                   "anything belonging to an operation lives on that mode's own page — see page 1.", S_DIM))
    story.append(Spacer(1, 3))

    avail = W - 2 * MARGIN
    gap = 5 * mm
    colw = (avail - gap) / 2.0
    left = [SECTIONS[0], SECTIONS[1]]
    right = [SECTIONS[2], SECTIONS[3], SECTIONS[4], SECTIONS[5]]

    def stack(items):
        out = []
        for i, (title, rows) in enumerate(items):
            out.append(section_block(title, rows, colw))
            if i != len(items) - 1:
                out.append(Spacer(1, 4))
        return out

    cols = Table([[stack(left), stack(right)]], colWidths=[colw, colw])
    cols.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (0, 0), 0),
        ("RIGHTPADDING", (0, 0), (0, 0), gap),
        ("LEFTPADDING", (1, 0), (1, 0), 0),
        ("RIGHTPADDING", (1, 0), (1, 0), 0),
        ("TOPPADDING", (0, 0), (-1, -1), 0),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 0),
    ]))
    story.append(cols)
    story.append(Spacer(1, 9))

    story.append(Paragraph("From a phone or laptop", S_H2))
    wifi = [
        [P("1", S_CELL_B), P("Settings → WiFi and updates → <b>Enabled</b> = yes. Change the "
                             "<b>Access PIN</b> from the default first: anyone who can join the "
                             "network can reflash the machine.", S_CELL)],
        [P("2", S_CELL_B), P("Join the <b>NanoEls-H4</b> network using that 8-digit PIN. The panel "
                             "shows the address beside the Enabled setting.", S_CELL)],
        [P("3", S_CELL_B), P("Open <b>192.168.4.1</b>. Every setting is editable, grouped as on the "
                             "panel, with each mode's own settings shown too.", S_CELL)],
        [P("4", S_CELL_B), P("The strip along the top is live: position, rpm, cutting speed, the "
                             "constant-speed target, and the encoder <b>signal</b> figure. Leave it "
                             "open through a job and check the low-water mark afterwards.", S_CELL)],
        [P("5", S_CELL_B), P("<b>Derived figures</b> shows what your settings actually mean — step "
                             "resolution, max feed, and the spindle rpm ceiling at the current pitch.", S_CELL)],
        [P("6", S_CELL_B), P("Firmware updates upload from the same page. Refused unless the machine "
                             "is stopped; the panel shows progress, not the browser.", S_CELL)],
    ]
    tw = Table(wifi, colWidths=[7 * mm, None])
    tw.setStyle(TableStyle([
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 3.5),
        ("TOPPADDING", (0, 0), (-1, -1), 2.4),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 2.4),
        ("BACKGROUND", (0, 0), (-1, -1), SOFT),
        ("BOX", (0, 0), (-1, -1), 0.6, LINE),
        ("LINEBELOW", (0, 0), (-1, -2), 0.4, colors.white),
    ]))
    story.append(tw)
    story.append(Spacer(1, 4))
    story.append(P("Writes are refused while the machine is running, from the panel, USB and the "
                   "browser alike — stop it first.", S_NOTE))
    return story


if __name__ == "__main__":
    import sys
    build(sys.argv[1], "NanoEls H4  ·  Quick Reference",
          [page1(), page2()],
          ["Modes", "First-time setup and global settings"],
          "Settings button: short press = this mode's page   ·   hold 0.5s = settings menu")
