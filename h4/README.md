# NanoEls H4

4-axis Electronic Lead Screw (ELS) based on ESP32-S3 for metal lathes.

![h4-600px](https://github.com/kachurovskiy/nanoels/assets/517919/86289a74-f70e-4d16-873f-0954dd297f77)

Controller is no longer available for purchase on https://kachurovskiy.com/ but you can DIY with the hardware design files provided.

## Features

- Metric, inch and TPI support, cut any thread with utmost precision
- Controls 2 axes - lead screw (Z) and cross-slide (X)
- Manual movement of both axes: by eye, in precise steps or by a precise amount
- Disabling/enabling of stepper motors allowing to switch to manual operations in 1 click
- Soft limits allowing to cut e.g. close to the chuck
- Automatic threads: just set length, depth, pitch and number of passes
- Tapered threading for NPT / BSPT pipe fittings
- Power cross-feed (`XGEAR`) for consistent facing finish
- Automatic slotting for internal keyways, using the lathe as a shaper
- Automatic multi-pass turning and facing
- Automatic multi-start threads
- Automatic half-spheres and ellipses
- Cone mode to cut internal or external tapers
- Cut-off mode allowing for smooth in-feed
- RPM and angle indication
- Internal/external and left-to-right versions of each automatic operation
- Backlash compensation
- Guided calibration routines that measure screw pitch, backlash, travel, max speed and encoder PPR on the machine
- Encoder signal health and input test screens for diagnosing wiring problems
- Rotary encoders to move axes
- Rotary or linear 3rd axis
- Optional WiFi access point with a browser-based settings page and over-the-air firmware updates

Advanced CNC features via [lathecode online CAD](https://github.com/kachurovskiy/lathecode):

- Running GCode over USB
- Saving GCode to device to run later

## Hardware

[Datasheet PDF](https://raw.githubusercontent.com/kachurovskiy/nanoels/main/h4/h4.pdf) contains schematic, screen, terminals and other hardware details that aren't specific do the electronic lead screw application.

See [parts.md](https://github.com/kachurovskiy/nanoels/blob/main/h4/parts.md) for Gerber, BOM, STL and other info required for DIY assembly.

See [hardware.md](https://github.com/kachurovskiy/nanoels/blob/main/hardware.md) for mounts and adapters for encoders and motors. For attaching a cross-slide motor, consider [belt-based](https://www.thingiverse.com/thing:6058899) or [gear-based](https://www.thingiverse.com/thing:4714722) NEMA 17 mounts.

## Wiring

![h4-back-cropped](https://github.com/kachurovskiy/nanoels/assets/517919/bd0fd006-5bda-4b00-a66b-ec7d7cab502e)

Encoder terminal:

- GND - connect to encoder power-in line (usually black) and wire shielding if there's any
- 5V - connect to encoder power-in line (usually red)
- ENCB - connect to one of the encoder signal lines
- ENCA - connect to one of the encoder signal lines

Z axis - stepper terminal for the main lead screw:

- 5V - connect to stepper driver PUL+, DIR+, ENA+ and wire shielding if there's any
- ENA - connect to stepper driver ENA-
- DIR - connect to stepper driver DIR-
- STEP - connect to stepper driver PUL-

X axis - stepper terminal for the cross-slide:

- 5V - connect to stepper driver PUL+, DIR+, ENA+ and wire shielding if there's any
- ENA - connect to stepper driver ENA-
- DIR - connect to stepper driver DIR-
- STEP - connect to stepper driver PUL-

Note: depending on your particular encoder, stepper driver and signal wire, H4 might not be able to provide sufficient voltage/current on the 5V output lines above due to the presence of a fuse and current-limiting resistors on the PCB. If your encoder isn't functioning or steppers are working erratically, consider supplying 5V from an external power source to the 5V lines above.

### Joystick (optional)

A directional stick with switches (digital, not analog) plus 2 buttons can be connected to the `A1` and `A2` terminals and used as a movement pendant. Each direction and button is a switch that connects its pin to the terminal's `GND` when active.

Operation is deadman-style for safety: deflecting the stick alone does nothing. Move the stick in a direction, then hold the move button — while it's held, the relevant axis moves in that direction, observing any limits set. Releasing the button or centering the stick stops the move. The second button cycles the movement step (1mm, 0.1mm, 0.01mm and so on) exactly like the step button on the front panel. While the move button is held, the bottom screen line shows what the pendant is doing, e.g. `Jog Z<` or `Jog ready` when the stick is centered.

Default wiring (edit `JOYSTICK_DIR_PIN_KEYS`, `JOYSTICK_MOVE_PIN` and `JOYSTICK_STEP_PIN` in `h4.ino` to remap):

- `A1` terminal: `A11` = left, `A12` = right, `A13` = up
- `A2` terminal: `A21` = down, `A22` = move button, `A23` = step button

To enable, set `JOYSTICK_USE = true` near the top of `h4.ino` and re-upload the sketch. Set it back to `false` to disable. The joystick can't be used together with a dividing head (`ACTIVE_A1`) or handwheel pulse generators (`PULSE_1_USE` / `PULSE_2_USE`) since they use the same terminals.

## Programming the controller

- Install the [Arduino IDE](https://docs.arduino.cc/software/ide-v2)
- Add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` in [Preferences as "Additional board manager URLs"](https://github.com/kachurovskiy/nanoels/assets/517919/dcc023e6-20fc-4284-ba56-d466dbe4ce53)
- Install `esp32` [via Board Manager](https://github.com/kachurovskiy/nanoels/assets/517919/094d00ff-1e51-4f26-bb81-aa4ad42bde2a)
- Install Adafruit `TCA8418` library [via Library Manager](https://github.com/kachurovskiy/nanoels/assets/517919/90326e0d-6600-4b47-aa66-1177c4b9cc27)
- Download [this repository](https://github.com/kachurovskiy/nanoels/archive/refs/heads/main.zip), unzip, go to `h4` directory and open `h4.ino` file in the Arduino IDE
- Check the top constants (e.g. encoder steps, motor steps, display offset) and adjust if needed
- Select "ESP32S3 Dev Module" as device at the top, pick COM port that appears when you connect the device with a USB cable
- Upload the sketch to your H4 controller

A few things to check after upload:

- Spindle direction: show angle on screen using ![IconDisplay](https://github.com/kachurovskiy/nanoels/assets/517919/60bb723d-4c2d-45af-9208-95c2b26a42d1). Rotate the chuck forward manually - angle should increase. If it decreases, swap `ENCB` and `ENCA` wires in the terminal
- Motor direction: try ![IconArrowLeft](https://github.com/kachurovskiy/nanoels/assets/517919/d110519a-6cf4-491e-b81a-c09aefa49e49) ![IconArrowRight](https://github.com/kachurovskiy/nanoels/assets/517919/48a0327e-0cc4-465e-b371-644f153f3288) ![IconArrowUp](https://github.com/kachurovskiy/nanoels/assets/517919/ac350635-4424-4438-bcfb-3cb0431345f8) ![IconArrowDown](https://github.com/kachurovskiy/nanoels/assets/517919/897b4005-45d0-46ed-977e-b25a98983961) buttons - if motor is moving in the wrong direction, change `INVERT_Z` or `INVERT_X` in the code, re-upload the sketch (or swap the motor leads `A+` and `A-` in the stepper driver)

After the first cable upload you can install later versions over WiFi instead — see
[WIFI.md](WIFI.md). The partition layout already has two application slots, so no change to the
board settings is needed. The very first upload has to be over USB.

Troubleshooting:

- Arduino IDE doesn't detect NanoEls H4: try a different USB cable
- Not sure which COM port is NanoEls H4: unplug it, check the list of available ports in Arduino IDE, plug H4 in, see new port that appeared is H4
- `ImportError: No module named serial` error on Linux: try `sudo apt install python3-serial`

## Running this on different hardware

Everything that describes a particular machine lives in **`machine_config.h`**. The sketch itself contains no machine-specific numbers, so porting to a different lathe, encoder, screw or panel means editing one file.

It's organised into sections: board revision, pins, spindle encoder, Z axis, X axis, optional third axis, handwheels, manual stepping feel, WiFi, keypad codes, joystick.

Two kinds of setting live there, and the difference matters:

- **Wiring and topology** — pins, which axes exist, whether a joystick or handwheels are fitted, the keypad matrix codes. These can't change while running, so they're compile-time only.
- **Startup defaults for values you can also edit in the settings menu** — encoder PPR and gearing, lead screw pitch, motor steps, backlash, speeds, max travel. Once a controller has been set up its stored values win, and these are only used by a device that has never been configured. Use the settings menu for these day to day; change them here to set the defaults for a new build, or after a settings wipe.

A few notes for a new setup:

- The **Input tester** calibration routine prints the code of whichever key you press, which is the easy way to remap `B_*` for a differently-wired panel.
- `ENCODER_FILTER` has an RPM ceiling that depends on your encoder and gearing — the formula is in the file, and a value that's safe on one encoder can silently drop pulses on another.
- The A1 terminals are shared between the third axis, the handwheels and the joystick. Only one of those can be enabled.
- After changing anything, work through [CALIBRATION.md](CALIBRATION.md) — most of these values can be measured on the machine rather than guessed.

## Tests

The arithmetic that decides whether a part comes out the right size - screw pitch correction, backlash and travel conversions, encoder PPR derivation, spindle angle wrapping and the RPM accumulator - lives in `calibration_math.h`, which has no Arduino dependencies. The firmware includes it and calls it directly, so the tests exercise the shipped code rather than a copy of it.

Run them on a PC with Visual Studio (or the standalone C++ Build Tools) installed:

```
.\test\run_tests.ps1
```

No board required. The suite is 69 assertions and takes a second. `test/` is ignored by the Arduino build, so it doesn't affect the firmware.

## Usage manual

**This manual corresponds to `NanoEls H4 V15`. To see your software version, hold ![IconStop](https://github.com/kachurovskiy/nanoels/assets/517919/9ed2da6c-7461-419f-827d-781980c9ddde) for 5 seconds. If you have a different version - e.g. `V5`, please update to the latest one.**

### Switching between metric and imperial

Press ![IconMeasure](https://github.com/kachurovskiy/nanoels/assets/517919/c4b72922-82c7-445a-9954-7bd08abcc6b5) to change the system of measurement between:

- Metric (mm)
- Inch
- TPI

It affects how the pitch and position is displayed and changes, move steps for precision moves.

### Setting pitch

https://github.com/kachurovskiy/nanoels/assets/517919/58da0f90-8adc-4035-a18c-198c88a73c22

Pitch is the distance that carriage will move when the spindle makes the full turn. For example, M14 thread uses 2mm pitch.

Use numpad buttons ![Icon0](https://github.com/kachurovskiy/nanoels/assets/517919/67f660f1-c6fa-4922-bf03-9f6571023806) to ![Icon9](https://github.com/kachurovskiy/nanoels/assets/517919/6ad6b4e4-5bf1-473f-93ed-65f6ee478d8f) to enter the pitch value. For example, pressing ![Icon1](https://github.com/kachurovskiy/nanoels/assets/517919/f0a4eb3c-1e57-441b-96a8-d76dcf0a6cf4) ![Icon5](https://github.com/kachurovskiy/nanoels/assets/517919/2ca23018-d54f-4754-a8f4-aff00ba655e8) ![Icon0](https://github.com/kachurovskiy/nanoels/assets/517919/1f71946a-14a9-41b0-9c1c-454fb935015c) followed by ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/1bf41a47-f4b5-4896-96a6-72c97ae472e2) will set the pitch to `0.15mm`, `0.15"` or `150tpi` depending on the current measurement system.

Adjust the desired pitch using ![IconMinus](https://github.com/kachurovskiy/nanoels/assets/517919/ed05b417-5714-4e31-9f17-9ff646c85214) and ![IconPlus](https://github.com/kachurovskiy/nanoels/assets/517919/0a5e045c-f670-4e45-8b26-e4e27de9ce7a) buttons.

Use negative pitch values e.g. `-2mm` to cut left-to-right threads. To change the pitch sign, press ![IconReverse](https://github.com/kachurovskiy/nanoels/assets/517919/8eee528a-b628-4207-99b1-6f61b85c8be7).

### Setting step

https://github.com/kachurovskiy/nanoels/assets/517919/c31d04b4-958b-48af-9717-09322885c12c

Step setting is used for manual carriage moves in `off` state - i.e. when no mode is active and no thread is being cut. Current step setting is normally shown on the 1st screen line as e.g. `step 1mm` or `step 0.1"` but can be hidden if there's no space.

Pressing ![IconSteps](https://github.com/kachurovskiy/nanoels/assets/517919/52ce78e4-4202-4642-9433-e61ca39de9d5) cycles through the list of predefined steps:

- Metric: `1mm`, `0.1mm`, `0.01mm`
- Imperial: `0.1"`, `0.01"`, `0.001"`

Use numpad buttons ![Icon0](https://github.com/kachurovskiy/nanoels/assets/517919/67f660f1-c6fa-4922-bf03-9f6571023806) to ![Icon9](https://github.com/kachurovskiy/nanoels/assets/517919/6ad6b4e4-5bf1-473f-93ed-65f6ee478d8f) to enter a custom step value. Pressing ![IconSteps](https://github.com/kachurovskiy/nanoels/assets/517919/52ce78e4-4202-4642-9433-e61ca39de9d5) when the screen bottom line shows `Use 1.234mm?` will make the step equal to `1.234mm`.

### Settings menu

The settings button is the **hexagon with a dot in the middle** - a hex nut seen end-on - on the right-hand side of the panel, in the inner of the two right columns, between the display button above it and the measure button below it. It's the one button in this manual named in words rather than shown as a symbol, so it's worth pointing out.

Press the settings button when the controller is `off` to open the settings menu. It opens on a directory of sections; move with the arrows and press play to enter one, and press stop to come back out.

| # | Section | Holds |
|---|---|---|
| 1 | Preferences | X readout, spring passes, peck depth, flank infeed, retract and clearance distances, slot left reduction, manual step feel, spindle divisions, constant cutting speed |
| 2 | Z axis | direction, backlash, screw pitch, motor steps, pulley teeth, speeds, acceleration, max travel |
| 3 | X axis | as Z |
| 4 | A1 axis | as Z, plus whether it's fitted and whether it's rotary |
| 5 | Spindle encoder | PPR, direction, pulley teeth, divider, dead-band and its shape, glitch filter |
| 6 | Handwheels | which are fitted, which axis each drives, direction, PPR, pulse width, dead-band |
| 7 | Joystick | fitted, debounce |
| 8 | WiFi & updates | radio on/off, access PIN — see [WIFI.md](WIFI.md) |
| 9 | Calibration | opens the guided measuring routines described below |

Inside a section, up and down move one item at a time, left and right jump a page, the numpad enters a new value and play confirms. Values are stored on the device and survive reboots and firmware updates. Anything editable here can be changed without re-flashing.

Every value shows its unit next to it, both on the screen and in the web interface — `Now 800 steps`, `Now 3400 steps/s`, `Now 20 teeth`. Distances follow whichever measurement system is selected, so the same setting reads `2 mm` or `0.0787"` depending on the mode. The confirmation line does the same while you type, so `Use 300 ms?` can't be mistaken for microseconds.

Distances (backlash, screw pitch) are entered in microns when in metric mode or thousandths of an inch when in inch mode. Speed and acceleration are entered in motor steps per second and steps per second squared - same units as the constants at the top of `h4.ino`. Press the settings button or off button to exit.

After changing screw pitch or motor steps, re-zero the axes since the physical meaning of the stored positions changes.

### Belt-driven axes

If a motor drives its lead screw through a belt or gears rather than a direct coupling, set `Motor pulley` and `Lead screw pulley` to their tooth counts in that axis's settings section. `Motor steps/rev` is then what the driver is actually set to - the steps for one turn of the **motor** - and the firmware works out the steps per turn of the screw itself.

Only the ratio matters, so a 3:1 reduction can be entered as 1 and 3, or 20 and 60. Both 1 is a direct coupling, which is the default.

You can still fold the ratio into the step count by hand if you prefer - 800 steps through a 3:1 reduction is the same as 2400 steps with 1 and 1 - but entering the teeth keeps the driver setting and the mechanics separate, so changing a pulley means changing one number instead of recomputing.

### Belt-driven encoders

If the encoder is driven off the spindle by a belt or gears rather than mounted on the spindle directly, set `Spindle pulley` and `Encoder pulley` to the tooth counts. Only the ratio matters, so a 1:2 belt can be entered as 40 and 20, or as 2 and 1. Direct drive is 1 and 1, which is the default.

Everything downstream works in spindle revolutions, so once the ratio is set the pitch, threading and RPM readout are all correct regardless of the gearing. The encoder PPR calibration routine accounts for it too - you turn the spindle, and it undoes the ratio to report the encoder's own PPR.

Note that a belt-driven encoder spins faster than the spindle when it's geared up, so a 1:2 belt at 2000 spindle rpm is running the encoder at 4000. Check that against the encoder's rated speed.

### Encoder divider and backlash

Two settings for an encoder that flutters - typically one on a belt, or one whose signal is marginal.

`Encoder divider` folds several raw counts into one step: at `2`, two counts become one. Movement smaller than the divider never reaches the spindle position at all, so flutter is quietened at the source. The cost is resolution - threading sync gets coarser by the same factor - so raise it only as far as you need. `1` is off.

`Encoder backlash` is a dead-band in counts. The carriage only follows a spindle reversal once the spindle has gone back this far, which stops it chasing small back-and-forth movement. The cost is that a genuine reversal is ignored for that many counts. This works downstream of the divider, so it's the coarser of the two tools - prefer the divider first.

### Backing up settings over USB

Connect over USB at 115200 baud and send `$` on its own line. The controller prints every stored machine setting as a line you can paste straight back to restore it:

```
; NanoEls H4 V16 settings
; paste these lines back to restore, distances are in deci-microns
$eppr=600
$Zinv=0
$Zbla=6500
$Zscr=20000
$Cscr=20000
...
; nvs free entries 476
; nvs save batches since boot 12
; uptime seconds 843
```

Send a single `$key=value` line to change one setting - `$Zbla=250` sets Z backlash to 25 microns. Values use the same units as storage (deci-microns for distances, `0`/`1` for on/off), and go through exactly the same validation as the settings menu, so serial can't store a value the menu would have refused. Settings can't be changed while the controller is `on`.

This is machine configuration only - positions, stops and the current mode are working state and aren't included. Worth saving a copy somewhere after calibrating: there's no undo in the settings menu, and a mistyped value overwrites the old one permanently.

The three `;` lines at the end are diagnostics. `nvs free entries` is how much settings storage remains, and `nvs save batches` counts how many times the controller has committed to flash since power-on - useful if you ever suspect it's writing more often than it should.

Axis settings are named by the axis letter followed by the suffix: `Z`, `X` and `C` for the third axis. **Backups taken with V15 or earlier are not reliable** - a bug named axis settings by their position in the menu rather than by their axis, so those files can contain the wrong key for a setting, the same key twice, and no third-axis settings at all. Take a fresh backup after upgrading and discard the old one.

### Settings over WiFi

The same settings can be edited from a phone or laptop, and firmware installed without a cable. The controller brings up its own WiFi network, so no router is involved. It's off unless you turn it on from `Settings > WiFi & updates`, it takes effect immediately without a restart, and while off there's no radio and no change in behaviour at all.

Edits made in the browser are staged and applied together when you press **Save**, so a half-typed number never reaches the machine.

See **[WIFI.md](WIFI.md)** for setup, what the page can and can't do, and why the access PIN has to be eight digits.

The web page is generated from the same settings table the LCD menu walks and writes through the same validation, so the two can't disagree about what exists or what's allowed. It deliberately has no way to move the machine: motion is only ever commanded from the keypad.

### Calibration

The last settings menu item, `Calibration`, opens a set of guided routines that *measure* the machine values on the actual lathe instead of asking you to know them in advance. Everything they save goes to the same places the settings menu edits, so calibrated and hand-entered values are interchangeable and you can always check or override a result in the menu afterwards.

Use up and down to browse the routines, left and right to jump by 4. The second line names the routine and the third shows the value it currently holds, so you can see what you're about to change. Press play to start one, off to go back. Throughout, play confirms and moves to the next step, minus steps back one so a wrong entry can be redone without repeating the physical moves, plus runs the current step's move again without advancing, and off aborts and puts back anything the routine changed.

**Nothing moves without you asking for it.** Every test move is shown on screen first and only happens when you press play, and off aborts. Moves that would cross a soft limit or exceed the axis travel are refused outright rather than shortened, since a silently truncated test move would corrupt the measurement it feeds.

- **Z / X screw pitch** - the one that makes parts come out the right size. Pick a test distance with left and right (1 to 100mm; longer is more accurate) and press play: the axis first moves 1mm to take up the slack, *then* you zero the dial indicator, press play again for the test move, and type what the indicator actually read. The corrected lead screw pitch is shown next to the old one before you save it. A correction larger than ±20% is rejected as a probable typo. On X this always means the actual cross-slide travel, not a diameter, even when the X readout is in diameter mode.

  That 1mm pre-load is not padding - it is what keeps backlash out of the result. Both moves go the same way, so the axis is already loaded when you zero the indicator and every step of the test move produces real travel. Without it, an axis you had last jogged *backwards* would spend the start of the test move taking up slack, the firmware would add however much backlash compensation is currently configured, and any error in that setting would land straight in the measured pitch. This is also why pitch can be calibrated **before** backlash: the routine does not depend on backlash being right yet. Just don't nudge the axis backwards between zeroing the dial and the test move.
- **Z / X backlash** - moves 2mm to take up the slack, waits for you to zero the indicator, then nudges back one step at a time with the left arrow. Press play the instant the indicator starts moving and the distance travelled becomes the backlash. Backlash compensation is switched off while the routine runs so it can't measure itself.
- **Z / X direction** - moves the axis 5mm and asks in plain words which way it went. Answer with play if correct, left arrow if not, and the direction inversion is flipped and re-tested.
- **Z / X travel limit** - the only routine you drive yourself: jog to one extreme with the arrows, press play, jog to the other, press play. The span becomes the axis max travel, which is what the "too far" safety check is based on. This value used to be compile-time only.
- **Z / X max speed** - runs 20mm strokes, raising the speed 10% each time you press play. Press the left arrow the moment the motor stalls or sounds wrong and the routine saves 80% of the last speed that worked.
- **Encoder PPR** - mark the chuck, choose how many revolutions to turn with left and right, press play and turn the spindle by hand. The count and the derived PPR update live. Press play at exactly that many turns and the result snaps to the nearest standard value if it's within 2%.
- **Encoder direction** - shows the count changing as you turn the spindle forwards. If it counts down instead of up, press the left arrow to reverse the encoder in software - previously this needed the A and B wires physically swapped.
- **Encoder signal** - a read-only health screen showing angle, RPM, the glitch filter setting, and a `Flips` counter of direction reversals. **With the spindle stopped this must stay at zero.** Anything else is electrical noise on the encoder cable, and the fix is a larger `ENCODER_FILTER` at the top of the sketch, better cable routing away from the stepper and VFD wiring, or a solid single-point ground. Left arrow resets the counters.
- **Input tester** - a read-only screen showing the code of whatever key you press and whether it's down or up, plus the live state of all six joystick pins. Useful for finding a stuck key or checking joystick wiring. Because it deliberately swallows every key, you leave it by *holding* off for a second.

Work through them in this order when setting up a machine: input tester and encoder signal first (nothing moves), then encoder PPR and direction, then the axis direction checks, then screw pitch, backlash, travel limits and finally max speed. Re-zero the axes after calibrating screw pitch, since the physical meaning of the stored positions changes.

**See [CALIBRATION.md](CALIBRATION.md) for a full step-by-step walkthrough of every routine**, including what to measure, what to type, what the messages mean and how to back the results up.

### Thread database

When in threading mode and `off`, press the settings button to open the thread database instead of the settings menu. Use up and down arrows to browse one at a time, left and right to jump by 10, or jump straight to a thread family with the numpad: `1` metric, `2` UNC/UNF, `3` BSPP, `4` trapezoidal, `5` ACME, `6` NPT. The database reopens on the last used thread. Press play to apply the selected thread - pitch and measurement system are set automatically and starts are reset to 1. Includes common metric coarse and fine, UNC, UNF, BSPP, metric trapezoidal (Tr), ACME and NPT threads. Note that trapezoidal, ACME and NPT presets only set the pitch - the correct form tool and, for NPT, a taper setup are still needed.

### Surface speed readout

Press the display button to cycle the extra readouts: spindle angle, then RPM with surface speed (e.g. `1250rpm 98m/min`), then the large position display, then off. Surface speed shows the current cutting speed in m/min (or ft/min in inch and TPI modes) calculated from the spindle RPM and the tool diameter position. For accurate readings, X must be zeroed on the lathe centerline - see "Zeroing the axes" below.

### Constant cutting speed

Cutting speed is what the tool actually feels, and it falls with diameter. The RPM that's right on 50mm stock gives a third of the speed on 16mm, and facing towards centre takes it to zero — which is why the finish goes off as you approach the middle of a faced surface.

Turn **Constant speed** on in Preferences and the RPM readout changes from showing what the speed *is* to showing what the spindle *should be doing*:

```
850rpm >620 +37%
```

850 RPM now, 620 wanted at this diameter, running 37% fast. Turn the spindle down until the percentage reads near zero. It updates live as the diameter changes, so it tracks a facing cut inward and steps between turning passes on its own.

`^` in place of `>` means the target is above **Spindle max rpm** and has been capped — the lathe can't reach the speed this diameter wants, which is normal on small diameters and not a fault.

**Constant speed** is the only item you need to touch to turn the whole thing on or off. The three below it keep their values while it's off, so switching back on returns to the setup you had.

#### Picking the speed

Set **Material** and **Tool** and the speed is looked up for you:

| Material | HSS | Carbide |
|---|---:|---:|
| Aluminium | 70 | 200 |
| Brass | 60 | 180 |
| Bronze | 40 | 120 |
| Copper | 50 | 150 |
| Cast iron | 25 | 90 |
| Mild steel | 30 | 120 |
| Alloy steel | 20 | 90 |
| Tool steel | 15 | 60 |
| Stainless | 15 | 60 |
| Titanium | 10 | 40 |
| Plastic | 100 | 250 |

All in m/min. Press **+** or **−** on the Material item to step through the list; the name is shown rather than a number. The web page shows the same list as a dropdown.

These are conservative starting points for turning, not gospel — the right speed also depends on depth of cut, feed, how rigid the setup is, and whether there's coolant. They're here so the machine can suggest something sane rather than leave you a number with no way to pick it. Treat them as a safe first cut and adjust from what the chip and the finish tell you.

Set **Material** to `Manual` to use the **Manual speed** figure instead. That's the one to use for a material that isn't listed, or when you've worked out what a particular job actually wants.

**The controller can't set the speed for you.** There's no spindle output on this board and no free pin for one — every terminal is taken by the steppers, encoder, display and keypad. This tells you what to dial and you dial it.

### Spindle indexing

The encoder resolves a revolution far more finely than a chuck can be positioned by hand — 4000 counts is 0.09° — so it can stand in for the dividing head the lathe doesn't have.

Set **Spindle divisions** in Preferences to the number of positions you want and the angle line becomes an index readout:

```
Idx 3/6 -2.4°
```

You're nearest mark 3 of 6, and 2.4° short of it. Turn the chuck until it reads `ON` and the buzzer sounds. Cut, then turn to the next mark and repeat. **Index tolerance** sets how close counts as arrived, in tenths of a degree; 0.5° is a sensible default, since a chuck turned by hand settles to a few tenths at best whatever the encoder can resolve.

Paired with [automatic slotting](#automatic-slotting) this cuts hex flats, square drives, keyway sets and splines with no indexer — index, slot, index, slot. It replaces the plain angle display while it's on. Set divisions to 0 to turn it off.

### Large position display

The third press of the display button switches to a full-screen readout showing the Z and X positions in large 2-row-tall digits readable from across the shop. It steps aside automatically whenever there's something else to show - numpad entry, operation setup, automated passes or a message - and comes back afterwards. In modes other than the normal gearbox mode it's only shown while `off`.

The big digits show four figures, right-aligned so the value always ends in the same column: 0.01mm below 100mm and 0.1mm above it, or 0.001" and 0.01" in inch mode. That's one digit less than the normal screen, which keeps full micron precision - the trade buys the info strip on the right, and the digit given up is the fastest-changing one, the one that's unreadable while an axis is moving anyway.

The strip beside the digits shows spindle RPM, the current jog step, and - on the rows belonging to each axis - a `Z OFF` or `X OFF` flag when that axis's stepper is disabled. **That flag is the important one.** This screen shows commanded stepper travel, not a measured carriage position: there is no scale on the axes, so the firmware only knows where it has told the motors to go. Disable an axis and hand-crank it and the number beside it silently stops being true until you re-zero. The flag sits on that axis's own rows so there's no ambiguity about which number has gone stale.

A note on the digit shapes: the display has only 8 custom character slots, and they're all spent on this font. Thinner strokes would need outer-cell glyphs combining a vertical bar with a crossbar, which needs roughly 19 shapes - so the chunky verticals are a hard limit of the hardware, not a style choice.

### Screen messages

When an input is refused - a value out of range in the settings, a move that would pass a limit, turning on without stops set - a short message explaining why appears on the bottom line for a moment along with the beep, e.g. `Limited by stop`, `Below start speed` or `Set all stops first`. During automated operations the bottom line also shows a progress bar filling up as passes complete. On power-up the screen briefly shows the firmware version, encoder PPR and the active screw/motor configuration of both axes so a wrong setup can be spotted before making chips.

### Diameter readout for X

By default X shows the tool position as a radius - distance from X0. Switch the `X readout` setting to `diameter` to show it doubled, the way lathe work is usually measured. In diameter mode the `X` in front of the position is replaced with a `ø` symbol so the two readouts can't be confused. In diameter mode, X values entered on the numpad - moves, X limits and go-to coordinates - are also treated as diameters, so entering `0.5mm` on the numpad and pressing the down arrow removes 0.5mm from the diameter by moving the tool 0.25mm. Zeroing X from a measured diameter with ![IconA](https://github.com/kachurovskiy/nanoels/assets/517919/3059b6ed-0197-4e48-91a7-80a7e1317176) already expects a diameter and works the same in both modes. The surface speed readout is unaffected.

### Spring passes

The `Spring passes` setting (0 = off) adds that many extra passes at final depth to the end of automatic turning, facing and threading operations. Spring passes repeat the last pass without adding infeed, removing the material left behind by tool and part deflection for a better finish and a more accurate size. The display shows `Spring x of y` while they run.

### Peck parting

The `Parting peck depth` setting (0 = off) makes the automatic cut-off mode back the tool off by 0.5mm every time it advances by the set depth, breaking the chip and letting coolant in, then return and continue the cut. Useful on deep parting cuts where a continuous chip tends to jam in the groove.

### Thread flank infeed

With the `Thread flank infeed` setting `on`, multi-pass threading feeds the tool in along the thread flank - the electronic equivalent of setting a manual lathe compound to 29.5 degrees - instead of plunging straight in. Each pass shifts sideways along the thread by 29.5 degrees worth of the remaining depth so the tool cuts mostly with its leading edge, which cuts cleaner and reduces chatter on deeper threads. The final pass (and any spring passes) run with no shift, finishing the thread in its true position.

### Retract and return

Press ![IconA](https://github.com/kachurovskiy/nanoels/assets/517919/3059b6ed-0197-4e48-91a7-80a7e1317176) briefly when `off` to retract X away from the workpiece by the `Retract distance` setting (default 2mm) - to inspect the work, measure or clear chips. The bottom line shows `X retracted`. Press it briefly again to return exactly to the remembered position. The retract direction follows the internal/external operation setting and observes X limits. Holding ![IconA](https://github.com/kachurovskiy/nanoels/assets/517919/3059b6ed-0197-4e48-91a7-80a7e1317176) for half a second performs its old function - enabling/disabling the X stepper for manual operation.

### Zeroing the axes

Z and X position can be counted from any location you'd like. By pressing ![IconZ](https://github.com/kachurovskiy/nanoels/assets/517919/32d95cce-d8be-4f8c-8a9c-399d278a2115) or ![IconX](https://github.com/kachurovskiy/nanoels/assets/517919/fd870901-4cc0-469e-ad88-e558998928d0) you can take the current position on the respective axis as `0`.

You can also move an axis to an exact coordinate: when `off`, enter the target position using the numpad buttons ![Icon0](https://github.com/kachurovskiy/nanoels/assets/517919/67f660f1-c6fa-4922-bf03-9f6571023806) to ![Icon9](https://github.com/kachurovskiy/nanoels/assets/517919/6ad6b4e4-5bf1-473f-93ed-65f6ee478d8f) and then press ![IconZ](https://github.com/kachurovskiy/nanoels/assets/517919/32d95cce-d8be-4f8c-8a9c-399d278a2115) or ![IconX](https://github.com/kachurovskiy/nanoels/assets/517919/fd870901-4cc0-469e-ad88-e558998928d0) - the axis moves to that position as it would be shown on the display. Use plus / minus while typing to enter negative coordinates. Typing `0` and pressing the axis button returns the tool to the axis zero. Limits are observed - the move clamps at a limit and beeps. Note: in older firmware versions this key combination set the axis `0` ahead of the current position, that function was replaced by go-to-position in V14.

You can set X0 on the lathe centerline by making a light cut, measuring diameter, entering it on the numpad and then pressing ![IconA](https://github.com/kachurovskiy/nanoels/assets/517919/3059b6ed-0197-4e48-91a7-80a7e1317176).

### Moving left and right, in and out

https://github.com/kachurovskiy/nanoels/assets/517919/6b6dfb87-ee3a-4e2c-b272-3af0c08e09fa

Press ![IconArrowLeft](https://github.com/kachurovskiy/nanoels/assets/517919/d110519a-6cf4-491e-b81a-c09aefa49e49) and ![IconArrowRight](https://github.com/kachurovskiy/nanoels/assets/517919/48a0327e-0cc4-465e-b371-644f153f3288) buttons to move the carriage, ![IconArrowUp](https://github.com/kachurovskiy/nanoels/assets/517919/ac350635-4424-4438-bcfb-3cb0431345f8) and ![IconArrowDown](https://github.com/kachurovskiy/nanoels/assets/517919/897b4005-45d0-46ed-977e-b25a98983961) to move the cross-slide.

Depending on the state of the controller, pressing the move button will result in traveling the following distance:

- `off` - carriage will move by the step distance but at least 1 stepper motor step
- `ON` - carriage will move in pitch increments (stay in the thread) but at least the step distance

If step is set to `1mm` or `0.1"`, pressing and holding move buttons results in continuous movement allowing for quick tool positioning. If step is set to values other than `1mm` or `0.1"`, there's a short delay betwen steps when manual moves are triggered allowing to precisely position the tool.

Changing direction will result in automatic backlash compensation, for example with backlash `0.65mm` and step `0.1mm`, first move in the opposite direction will result in lead screw turning `0.75mm`. Tool should still only move `0.1mm` assuming backlash is uniform and is specified correclty.

When `off`, use numpad buttons ![Icon0](https://github.com/kachurovskiy/nanoels/assets/517919/67f660f1-c6fa-4922-bf03-9f6571023806) to ![Icon9](https://github.com/kachurovskiy/nanoels/assets/517919/6ad6b4e4-5bf1-473f-93ed-65f6ee478d8f) to enter a custom move distance. Pressing a move button when the screen bottom line shows `Use 1.234mm?` will move `1.234mm` in the corresponding direction. If there's an automatic stop limiting travel, it will move up to the limit and beep.

### Soft limits / automatic stops

https://github.com/kachurovskiy/nanoels/assets/517919/8b29bba1-941e-4e69-8598-6cb8aa0c6929

Soft limits allow to cut close to the lathe chuck, part shoulder or rear support without hitting them.

One way to set a soft limit is to move the carriage or cross-slide to the desired position and press one of the ![IconLimitLeft](https://github.com/kachurovskiy/nanoels/assets/517919/0e3d005e-b917-4389-84b3-76bddbd989a4) ![IconLimitRight](https://github.com/kachurovskiy/nanoels/assets/517919/9890c846-c7af-4082-a862-39bb0a1ffcd7) ![IconLimitUp](https://github.com/kachurovskiy/nanoels/assets/517919/fd552c49-7f50-4e25-b78f-b0d565c0b96c) ![IconLimitDown](https://github.com/kachurovskiy/nanoels/assets/517919/b18703de-9408-45aa-83e0-812069a3a674) limit buttons. Pressing the button again removes the limit regardless of the current position.

Another way to set a soft limit is to use numpad buttons ![Icon0](https://github.com/kachurovskiy/nanoels/assets/517919/67f660f1-c6fa-4922-bf03-9f6571023806) to ![Icon9](https://github.com/kachurovskiy/nanoels/assets/517919/6ad6b4e4-5bf1-473f-93ed-65f6ee478d8f) to enter a distance relative to the current position. Pressing a limit button when the screen bottom line shows `Use 1.234mm?` will set corresponding limit `1.234mm` away from the current position.

Soft limits are used when preparing an automatic operation like turning, facing, threading and cutting off.

### Gearbox mode / electronic lead screw

https://github.com/kachurovskiy/nanoels/assets/517919/df9fa150-0284-4aec-ab81-8e37f4a747bc

Press ![IconGears](https://github.com/kachurovskiy/nanoels/assets/517919/d926a776-79a8-4abe-9589-734ef546daf5) to switch to the gearbox mode. After turning the controller `ON` with the ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/250b0641-6d5c-4ffb-a88c-b483e98c04c5) button, lead screw will move the distance of 1 pitch for 1 spindle turn.

With a small pitch like `0.07mm` this mode is useful for turning - gradually taking off material from the outer diameter of the part.

With a large pitch like `0.5"` one can cut a corresponding thread. Moving with ![IconArrowLeft](https://github.com/kachurovskiy/nanoels/assets/517919/e75e40a9-8a75-4375-90e7-4dbcf1305ab0) and ![IconArrowRight](https://github.com/kachurovskiy/nanoels/assets/517919/47cd6d04-ca6f-4af9-83f3-b15af942c588) buttons is supported and does not result in losing the thread.

Direction of movement can be quickly reversed with ![IconReverse](https://github.com/kachurovskiy/nanoels/assets/517919/d820c1cd-8ce3-4d02-91f9-d240f4ea51cc) button which toggles the pitch sign. Negative pitch can also be used to cut left threads.

Soft limits are respected in this mode allowing to finish the cut in a precise position.

Cross-slide can be moved with manual move buttons or disabled by holding ![IconA](https://github.com/kachurovskiy/nanoels/assets/517919/3059b6ed-0197-4e48-91a7-80a7e1317176) for half a second and be operated manually. A short press of that button retracts / returns the cross-slide instead - see "Retract and return".

Press ![IconStop](https://github.com/kachurovskiy/nanoels/assets/517919/9ed2da6c-7461-419f-827d-781980c9ddde) to turn gearbox mode `off` and decouple lead screw movements from spindle turns.

### Modes hidden behind a button

Some buttons hold more than one mode. When the screen shows a `*` after the mode name, pressing that same button again switches to another mode:

| Button | First press | Press again |
|---|---|---|
| gears | gearbox (Z) — shown as `*` | `XGEAR` |
| thread | `THRD*` | `TPR` (tapered thread) |

The `*` only appears on the first of the pair, so if you can see it there's something else behind that key.

### XGEAR — power cross-feed

Press the gears button twice. Where gearbox mode feeds `Z` along the bed, `XGEAR` feeds `X` across the face — the cross-slide moves one pitch per spindle revolution.

This is what you want for **facing**. Surface finish on a faced surface depends almost entirely on feed per revolution, and hand-cranking gives a visibly uneven finish because your hand speed varies. Set `0.05mm` and the finish is consistent right across the face with no arm ache.

Everything works as it does in gearbox mode: soft limits are respected, reverse flips the direction, and the carriage can be moved by hand or disabled and cranked manually.

The one difference: manual `Z` moves during `XGEAR` don't preserve the spindle phase the way manual `X` moves do in gearbox mode. That only matters if you're using `XGEAR` to cut a spiral, which is unusual.

### Automatic slotting

Press the mode button until `SLOT` is shown.

This one doesn't use the spindle at all — it uses the lathe as a **shaper**. `X` feeds in to depth, `Z` strokes to the left, `X` retracts, `Z` returns to the right, and the next pass goes deeper. Lock the spindle before starting.

**What it's for:** cutting an **internal keyway** in a bore — a pulley, gear or sprocket. Otherwise you need a broach set (expensive, one size per keyway, wants a press) or a mill with a slotting head. Doing it by hand means cranking the carriage back and forth for fifty passes. Also works for external keyways, flats and splines.

All four soft limits must be set: `Z` left and right give the length of the slot, `X` up and down give the start and finish depth.

Pressing play guides through the same steps as the other automatic modes — number of passes, then `External?`/`Internal?` to choose which `X` limit to start from, then `Go?`.

Two things behave differently from the spindle-driven modes:

- **Pitch is the feed rate in distance per second**, not per revolution, since there is no spindle to reference. Adjust it while running with plus and minus. Setting pitch to `0` parks the stroke until you raise it again, which is a convenient pause.
- `X` moves and the return stroke use the configured manual move speed, not the pitch.

For **blind** slots, set `Slot left reduction` in `Settings > Preferences` to something small. Each successive stroke then stops a little shorter than the last, leaving somewhere for chips to accumulate near the closed end instead of packing solid. `0` — the default — gives full-length strokes for a through slot.

### Tapered threading

Press the thread button twice; the mode reads `TPR`.

Identical to normal threading except that `X` drifts steadily across as `Z` advances, so the thread gets progressively shallower along its length.

**What it's for: NPT and BSPT pipe threads**, which are tapered by definition and turn up in every hydraulic, pneumatic, gas and plumbing fitting. Making your own adapter beats waiting a week for one.

Setup adds one step between `External?`/`Internal?` and `Go?`:

```
Taper ratio 0.0625?
```

The ratio is `(major diameter - minor diameter) / length`, the same figure cone mode uses. NPT is 1:16, so `0.0625`. Type a new one on the numpad and press play, or press play to keep what's shown.

Everything else — multi-start, spring passes, flank infeed, skipping a pass with play — works exactly as it does for a straight thread.

Note that the taper ratio is the **same stored value cone mode uses**, so setting one changes the other.

### Automatic turning

https://github.com/kachurovskiy/nanoels/assets/517919/30d98d1e-6ddc-4f40-b637-48f76003cd33

Press ![IconTurning](https://github.com/kachurovskiy/nanoels/assets/517919/00abcae7-0cc3-430d-be56-c8ec22d1d9d0) to switch to the automatic turning mode which is usually used to take large amount of material from the inside or outside diameter of the part in multiple passes.

Set the desired pitch to a suitable value e.g. `0.05mm`. Negative pitch will make turning start from the left limit, positive pitch will make it start from the right. All soft limits (left, right, up, down) must be set before the operation can be started.

Pressing ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d) guides through remaining steps:

- Entering the numer of passes - use numpad or ![IconPlus](https://github.com/kachurovskiy/nanoels/assets/517919/3c0e1c27-820a-4d34-ae97-4b362b537e72) and ![IconMinus](https://github.com/kachurovskiy/nanoels/assets/517919/75db2ae9-97d6-4d39-9fff-e98e889ee84b), confirm with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)
- Selecting `External` or `Internal` operation - use ![IconArrowLeft](https://github.com/kachurovskiy/nanoels/assets/517919/28a59458-0f91-42a5-9ba4-412d050dc462) and ![IconArrowRight](https://github.com/kachurovskiy/nanoels/assets/517919/0ff9ab80-0ce1-45fc-bc00-86d34ecac9f1) to change selection. Internal operations start from the ![IconLimitUp](https://github.com/kachurovskiy/nanoels/assets/517919/fd552c49-7f50-4e25-b78f-b0d565c0b96c) limit, external from the ![IconLimitDown](https://github.com/kachurovskiy/nanoels/assets/517919/b18703de-9408-45aa-83e0-812069a3a674) limit. Confirm with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)
- Confirm the final `Go?` question with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)

Turn on the spindle. Operation will proceed fully automatically and the cutter will return to the starting position when done. Pitch can be adjusted on the fly using ![IconPlus](https://github.com/kachurovskiy/nanoels/assets/517919/3c0e1c27-820a-4d34-ae97-4b362b537e72) and ![IconMinus](https://github.com/kachurovskiy/nanoels/assets/517919/75db2ae9-97d6-4d39-9fff-e98e889ee84b).

You can skip to the next pass using ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d) while the operation is running.

Operation can be stopped at any time by pressing ![IconStop](https://github.com/kachurovskiy/nanoels/assets/517919/cf4b9b31-dda3-4469-9667-1d1c44ea39b4) or using manual move buttons.

### Automatic facing

Press ![IconFacing](https://github.com/kachurovskiy/nanoels/assets/517919/f37cfd0e-74cc-4e70-80ab-620ab769afb8) to switch to the automatic facing mode which is usually used to take large amount of material from the face of the part in 1 or more passes.

It's setup in the same way as automatic turning above. When running, passes are made along the face instead of the side of the part. Negative pitch will make facing start from the inside.

### Cone

https://github.com/kachurovskiy/nanoels/assets/517919/6dd4a5e6-12ef-45a5-a24e-4a538f206546

Press ![IconCone](https://github.com/kachurovskiy/nanoels/assets/517919/d6fb049a-d93e-4dd4-aec9-4a9ed840ffde) to switch to the cone mode which maintains a constant ratio of movement between the X and Z axes. Cone mode doesn't require soft limits to be set but if they are set, they are respected.

Pressing ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d) guides through the setup:

- Enter cone ratio which is calculated as `(major_diameter - minor_diameter) / length`, see [this spreadsheet for most used values](https://docs.google.com/spreadsheets/d/1l0FUMtlWUjPywN9j94DOL84lB8dFrvdRWZIffT2NoHA/edit?usp=sharing). Confirm with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)
- Select `External` or `Internal` cone - use ![IconArrowLeft](https://github.com/kachurovskiy/nanoels/assets/517919/28a59458-0f91-42a5-9ba4-412d050dc462) and ![IconArrowRight](https://github.com/kachurovskiy/nanoels/assets/517919/0ff9ab80-0ce1-45fc-bc00-86d34ecac9f1) to change selection. Confirm with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)

Turn on the spindle. Pitch can be adjusted on the fly using ![IconPlus](https://github.com/kachurovskiy/nanoels/assets/517919/3c0e1c27-820a-4d34-ae97-4b362b537e72) and ![IconMinus](https://github.com/kachurovskiy/nanoels/assets/517919/75db2ae9-97d6-4d39-9fff-e98e889ee84b). It's convenient to cut multiple cone passes with ![IconReverse](https://github.com/kachurovskiy/nanoels/assets/517919/d820c1cd-8ce3-4d02-91f9-d240f4ea51cc) moving the tool in using manual move buttons.

### Automatic cut-off

https://github.com/kachurovskiy/nanoels/assets/517919/fe5245b8-4bf4-4001-8762-c786e85a2369

Press ![IconParting](https://github.com/kachurovskiy/nanoels/assets/517919/ba1501b3-89df-4198-9f8d-3ff862add69f) to switch to the automatic parting mode which gradually feeds the cross-slide into the part from outside or inside in one or multiple passes.

Set the desired pitch to a suitable value e.g. `0.05mm`. Negative pitch will make turning start from the inside, positive pitch will make it start from the outside. ![IconArrowUp](https://github.com/kachurovskiy/nanoels/assets/517919/0a1320df-5ec8-4f6d-93f2-f8c1bdfe93b8) and ![IconLimitDown](https://github.com/kachurovskiy/nanoels/assets/517919/b18703de-9408-45aa-83e0-812069a3a674) soft limits must be set before the operation can be started.

Pressing ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d) guides through remaining steps:

- Entering the numer of passes - use numpad or ![IconPlus](https://github.com/kachurovskiy/nanoels/assets/517919/3c0e1c27-820a-4d34-ae97-4b362b537e72) and ![IconMinus](https://github.com/kachurovskiy/nanoels/assets/517919/75db2ae9-97d6-4d39-9fff-e98e889ee84b), confirm with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)
- Check that pitch corresponds to the desired direction of cutting, click ![IconReverse](https://github.com/kachurovskiy/nanoels/assets/517919/d820c1cd-8ce3-4d02-91f9-d240f4ea51cc) to change it
- Confirm the final `Go?` question with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)

Turn on the spindle. Operation will proceed fully automatically and the cutter will return to the starting position when done. Pitch can be adjusted on the fly using ![IconPlus](https://github.com/kachurovskiy/nanoels/assets/517919/3c0e1c27-820a-4d34-ae97-4b362b537e72) and ![IconMinus](https://github.com/kachurovskiy/nanoels/assets/517919/75db2ae9-97d6-4d39-9fff-e98e889ee84b).

Operation can be stopped at any time by pressing ![IconStop](https://github.com/kachurovskiy/nanoels/assets/517919/cf4b9b31-dda3-4469-9667-1d1c44ea39b4) or using manual move buttons.

### Automatic threading

Press ![IconThread](https://github.com/kachurovskiy/nanoels/assets/517919/8f07c5bc-fdf5-4eaa-91c4-32a5d656e04a) to switch to the automatic threading which will cut a thread (optionally multi-start one) in multiple passes.

Set the desired pitch to a suitable value e.g. `2mm` or `20tpi`. Negative pitch will result in a left thread and will make the operation start from the left limit, positive pitch will make the operation start from the right. All soft limits (left, right, up, down) must be set before the operation can be started.

Pressing ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d) guides through remaining steps:

- Entering the numer of passes - use numpad or ![IconPlus](https://github.com/kachurovskiy/nanoels/assets/517919/3c0e1c27-820a-4d34-ae97-4b362b537e72) and ![IconMinus](https://github.com/kachurovskiy/nanoels/assets/517919/75db2ae9-97d6-4d39-9fff-e98e889ee84b), confirm with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)
- Selecting `External` or `Internal` operation - use ![IconArrowLeft](https://github.com/kachurovskiy/nanoels/assets/517919/28a59458-0f91-42a5-9ba4-412d050dc462) and ![IconArrowRight](https://github.com/kachurovskiy/nanoels/assets/517919/0ff9ab80-0ce1-45fc-bc00-86d34ecac9f1) to change selection. Internal operations start from the ![IconLimitUp](https://github.com/kachurovskiy/nanoels/assets/517919/fd552c49-7f50-4e25-b78f-b0d565c0b96c) limit, external from the ![IconLimitDown](https://github.com/kachurovskiy/nanoels/assets/517919/b18703de-9408-45aa-83e0-812069a3a674) limit. For multi-start thread, press ![IconPlus](https://github.com/kachurovskiy/nanoels/assets/517919/3c0e1c27-820a-4d34-ae97-4b362b537e72) to add more starts. Confirm with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)
- Confirm the final `Go?` question with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)

Turn on the spindle. Operation will proceed fully automatically and the cutter will return to the starting position when done.

You can skip to the next pass using ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d) while the operation is running.

Operation can be stopped at any time by pressing ![IconStop](https://github.com/kachurovskiy/nanoels/assets/517919/cf4b9b31-dda3-4469-9667-1d1c44ea39b4) or using manual move buttons.

### Automatic half-spheres and half-ellipses

Press ![IconM](https://github.com/kachurovskiy/nanoels/assets/517919/902cc062-3b85-4335-8e6c-077ec956f410) several times until `ELLI` is shown on screen. In this mode, cutter will form convex or concave half-ellipses in multiple passes.

Starting operation with positive pitch forms left hemisphere, with negative pitch - right hemisphere. This ensures that X backlash doesn't affect the cut.

`External` mode forms convex hemisphere (curved outward), `Internal` forms concave hemisphere (curved inward). Start and end points are the same in both cases.

Set the desired pitch to a suitable value e.g. `0.07mm`. All soft limits (left, right, up, down) must be set before the operation can be started.

Pressing ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d) guides through remaining steps:

- Entering the numer of passes - use numpad or ![IconPlus](https://github.com/kachurovskiy/nanoels/assets/517919/3c0e1c27-820a-4d34-ae97-4b362b537e72) and ![IconMinus](https://github.com/kachurovskiy/nanoels/assets/517919/75db2ae9-97d6-4d39-9fff-e98e889ee84b), confirm with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)
- Selecting `External` or `Internal` operation - use ![IconArrowLeft](https://github.com/kachurovskiy/nanoels/assets/517919/28a59458-0f91-42a5-9ba4-412d050dc462) and ![IconArrowRight](https://github.com/kachurovskiy/nanoels/assets/517919/0ff9ab80-0ce1-45fc-bc00-86d34ecac9f1) to change selection. Confirm with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)
- Confirm the final `Go?` question with ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d)

Turn on the spindle. Operation will proceed fully automatically and the cutter will return to the starting position when done. Adjusting the pitch on the fly is not supported.

Operation can be stopped at any time by pressing ![IconStop](https://github.com/kachurovskiy/nanoels/assets/517919/cf4b9b31-dda3-4469-9667-1d1c44ea39b4) or using manual move buttons.

### Async mode

In this mode the carriage disregards spindle movements and instead moves with constant speed of `pitch` per second. When pitch is positive, `Z` axis moves to the left. When pitch is negative, `Z` axis moves to the right. If pitch is 0, `Z` axis doesn't move.

You can adjust the pitch using ![IconMinus](https://github.com/kachurovskiy/nanoels/assets/517919/ed05b417-5714-4e31-9f17-9ff646c85214) or ![IconPlus](https://github.com/kachurovskiy/nanoels/assets/517919/0a5e045c-f670-4e45-8b26-e4e27de9ce7a) to adjust the movement speed.

Async mode respects limits if they are set.

### GCode

![image](https://github.com/kachurovskiy/nanoels/assets/517919/36c18783-e229-4531-a2e8-bc68c17727fb)

**WARNING:** GCode mode ignores automatic stops. Use ![IconStop](https://github.com/kachurovskiy/nanoels/assets/517919/cf4b9b31-dda3-4469-9667-1d1c44ea39b4) or emergency stop. Clicking `Stop` in the Web UI has a delay.

This mode allows to control tool movement from the PC. Programs can be generated automatically using textual part description - see [lathecode README](https://github.com/kachurovskiy/lathecode) for more info.

Before the program runs, it's critical to zero the tool:

- Set `Z=0` at the point where the left edge of the cutting tool touches the right face of the stock - e.g. by taking a light facing cut and pressing ![IconZ](https://github.com/kachurovskiy/nanoels/assets/517919/32d95cce-d8be-4f8c-8a9c-399d278a2115)
- Set `X=0` at the point where the front edge of the cutting tool is on the centerline - e.g. take of a small amount of material from the outside diameter, enter the measured diameter using the numpad, press ![IconA](https://github.com/kachurovskiy/nanoels/assets/517919/3059b6ed-0197-4e48-91a7-80a7e1317176)

Before cutting your first part, zero the tool "in the air" and check that your program is doing what you expect.

Use [**lathecode online editor**](https://kachurovskiy.com/lathecode/) to define your part and run or save the GCode program to H4.

#### Saved programs

Switch to `GCODE` mode and press ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/1bf41a47-f4b5-4896-96a6-72c97ae472e2).

Use ![IconArrowUp](https://github.com/kachurovskiy/nanoels/assets/517919/ac350635-4424-4438-bcfb-3cb0431345f8) and ![IconArrowDown](https://github.com/kachurovskiy/nanoels/assets/517919/897b4005-45d0-46ed-977e-b25a98983961) to select the program, ![IconMinus](https://github.com/kachurovskiy/nanoels/assets/517919/ed05b417-5714-4e31-9f17-9ff646c85214) to delete the program, ![IconPlay](https://github.com/kachurovskiy/nanoels/assets/517919/c9fb0ef5-94d7-4b42-b1a3-4c85c704e80d) to run it.

#### Technical details of the GCode mode

This section is intended for the few people sending raw serial commands to H4 - not for regular lathecode users.

Commands can be sent to device e.g. using [our Web-based GCode sender](https://kachurovskiy.github.io/nanoels/h4/sender.html) while connected over USB. The following commands are currently implemented:

- G0, G1 - linear move
- G20, G21 - inch or metric mode
- F - setting feed as mm/min or inch/min
- X, Z - single axis move
- G90, G91 - absolute or relative positioning
- G18 - ZX plane

Sample command `G1 X5 Z2 F100` will move the cutter to X=5mm, Z=2mm at 100 mm/sec in absolute metric mode.

To save a program `X0\n\Z10` under a name `Part1`, send `"Part1\nX0\n\Z10"`. To remove it, send `"Part1"`. To remove all saved programs, send `""`.
