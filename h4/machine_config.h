// Everything that describes YOUR machine. This is the only file you should need to edit to run
// this firmware on different hardware - the sketch itself contains no machine-specific numbers.
//
// Two kinds of setting live here:
//
//   * Wiring and topology - pins, which axes exist, whether a joystick or handwheels are fitted,
//     the keypad layout. These cannot change at runtime, so they are compile-time constants.
//
//   * Startup defaults for values that ARE editable in the settings menu - encoder PPR and
//     gearing, lead screw pitch, motor steps, backlash, speeds, max travel. Once a controller
//     has been set up, its stored values win and these are only used on a device that has never
//     been configured. Prefer the settings menu for these; change them here only to set the
//     defaults for a new build, or after a settings wipe.
//
// After changing anything here, re-run the calibration routines - see CALIBRATION.md.

#ifndef MACHINE_CONFIG_H
#define MACHINE_CONFIG_H

// ---------------------------------------------------------------------------
// Board revision
// ---------------------------------------------------------------------------

// Change whenever a different PCB / encoder / stepper / ... design is used.
#define HARDWARE_VERSION 4

// ---------------------------------------------------------------------------
// Pins
// ---------------------------------------------------------------------------

// Stepper drivers: enable, direction, step.
#define Z_ENA 16
#define Z_DIR 17
#define Z_STEP 18

#define X_ENA 8
#define X_DIR 19
#define X_STEP 20

// Spindle encoder channels. Swapping these reverses the counting direction, though the
// "Encoder direction" setting does the same thing without rewiring.
#define ENC_A 7
#define ENC_B 15

#define BUZZ 4 // Buzzer
#define SCL 5  // I2C clock, to the keypad controller
#define SDA 6  // I2C data

// Auxiliary terminals. Used by the A1 axis, the handwheels or the joystick - whichever of those
// is enabled below. They cannot be shared.
#define A11 9
#define A12 10
#define A13 11

#define A21 12
#define A22 13
#define A23 14

// Character LCD in 8-bit parallel mode: rs, enable, then d0-d7.
#define LCD_RS 21
#define LCD_EN 48
#define LCD_D0 47
#define LCD_D1 38
#define LCD_D2 39
#define LCD_D3 40
#define LCD_D4 41
#define LCD_D5 42
#define LCD_D6 2
#define LCD_D7 1
#define LCD_COLUMNS 20
#define LCD_ROWS 4

// ---------------------------------------------------------------------------
// Spindle encoder
// ---------------------------------------------------------------------------

// Pulses per revolution per channel, as the encoder is marked. The firmware reads both edges of
// both channels, so it sees 4x this many counts per encoder revolution.
const int ENCODER_PPR = 600;

// Belt or gear drive between spindle and encoder: the encoder turns SPINDLE_TEETH/PULLEY_TEETH
// times per spindle revolution. Both 1 means the encoder is on the spindle directly. Only the
// ratio matters, so a 1:2 belt can be 40 and 20, or 2 and 1.
const int ENCODER_SPINDLE_TEETH = 1;
const int ENCODER_PULLEY_TEETH = 1;

// Folds this many raw counts into one step, trading resolution for steadiness. Raise it if the
// carriage chases a fluttering encoder; movement smaller than the divider then never reaches
// the spindle position at all. 1 is off.
const int ENCODER_DIVIDER = 1;

// Dead-band in counts: the carriage only follows a spindle reversal once the spindle has gone
// back this far. Stops the carriage chasing small back-and-forth movement, at the cost of
// ignoring a genuine reversal for this many counts. Works downstream of the divider, so reach
// for the divider first.
const int ENCODER_BACKLASH = 3;

// Glitch filter in 12.5ns clock cycles, 1 - 1023. A transition is only accepted once the input
// has been stable this long, which is the main defence against electrical noise on the encoder
// cable. Too high and real pulses start being discarded at speed:
//
//   highest usable ENCODER rpm  =  2.4e9 / (ENCODER_PPR * ENCODER_FILTER)
//
// and remember a geared-up encoder spins faster than the spindle. At 1000 PPR and 200 that is
// 12000 encoder rpm - 6000 at the spindle through a 1:2 belt. The same value on a 600 PPR
// encoder allows 20000, so PPR and gearing both have to be accounted for before raising it.
const int ENCODER_FILTER = 2;

// ---------------------------------------------------------------------------
// Main lead screw (Z)
// ---------------------------------------------------------------------------

const long SCREW_Z_DU = 20000; // 2mm lead screw in deci-microns (10^-7 of a meter)
const long MOTOR_STEPS_Z = 800; // Motor steps per revolution of the MOTOR, microstepping included
// Belt or gear drive between motor and lead screw: the motor turns SCREW_TEETH/MOTOR_TEETH times
// per screw revolution. Both 1 is a motor coupled straight to the screw. Only the ratio matters.
const long MOTOR_TEETH_Z = 1;
const long SCREW_TEETH_Z = 1;
const long SPEED_START_Z = 2 * MOTOR_STEPS_Z; // Initial speed of a motor, steps / second.
const long ACCELERATION_Z = 30 * MOTOR_STEPS_Z; // Acceleration of a motor, steps / second ^ 2.
const long SPEED_MANUAL_MOVE_Z = 6 * MOTOR_STEPS_Z; // Maximum speed of a motor during manual move, steps / second.
const bool INVERT_Z = false; // change (true/false) if the carriage moves e.g. "left" when you press "right".
const bool NEEDS_REST_Z = false; // Set to false for closed-loop drivers, true for open-loop.
const long MAX_TRAVEL_MM_Z = 300; // Lathe bed doesn't allow to travel more than this in one go, 30cm / ~1 foot
const long BACKLASH_DU_Z = 6500; // 0.65mm backlash in deci-microns (10^-7 of a meter)
const char NAME_Z = 'Z'; // Text shown on screen before axis position value, GCode axis name

// ---------------------------------------------------------------------------
// Cross-slide lead screw (X)
// ---------------------------------------------------------------------------

const long SCREW_X_DU = 12500; // 1.25mm lead screw with 3x reduction in deci-microns (10^-7) of a meter
const long MOTOR_STEPS_X = 2400; // Motor steps per revolution of the MOTOR
// Could equally be written as 800 steps with MOTOR_TEETH_X 1 and SCREW_TEETH_X 3.
const long MOTOR_TEETH_X = 1;
const long SCREW_TEETH_X = 1;
const long SPEED_START_X = MOTOR_STEPS_X; // Initial speed of a motor, steps / second.
const long ACCELERATION_X = 10 * MOTOR_STEPS_X; // Acceleration of a motor, steps / second ^ 2.
const long SPEED_MANUAL_MOVE_X = 3 * MOTOR_STEPS_X; // Maximum speed of a motor during manual move, steps / second.
const bool INVERT_X = true; // change (true/false) if the carriage moves e.g. "left" when you press "right".
const bool NEEDS_REST_X = false; // Set to false for all kinds of drivers or X will be unlocked when not moving.
const long MAX_TRAVEL_MM_X = 100; // Cross slide doesn't allow to travel more than this in one go, 10cm
const long BACKLASH_DU_X = 1500; // 0.15mm backlash in deci-microns (10^-7 of a meter)
const char NAME_X = 'X'; // Text shown on screen before axis position value, GCode axis name

// ---------------------------------------------------------------------------
// Third axis on the A1 terminals (uncommon) - e.g. a dividing head
// ---------------------------------------------------------------------------

// Throughout this block 1mm = 1 degree of rotation, so 1du = 0.0001 degree.
const bool ACTIVE_A1 = false; // Whether the axis is connected
const bool ROTARY_A1 = true; // Whether the axis is rotary or linear
const long MOTOR_STEPS_A1 = 300; // Motor steps for 1 rotation of the worm gear screw
const long MOTOR_TEETH_A1 = 1;
const long SCREW_TEETH_A1 = 1;
const long SCREW_A1_DU = 20000; // Degrees multiplied by 10000 that the spindle travels per 1 turn of the worm gear. 2 degrees.
const long SPEED_START_A1 = 1600; // Initial speed of a motor, steps / second.
const long ACCELERATION_A1 = 16000; // Acceleration of a motor, steps / second ^ 2.
const long SPEED_MANUAL_MOVE_A1 = 3200; // Maximum speed of a motor during manual move, steps / second.
const bool INVERT_A1 = false; // change (true/false) if the carriage moves e.g. "left" when you press "right".
const bool NEEDS_REST_A1 = false; // Set to false for closed-loop drivers. Open-loop: true if you need holding torque, false otherwise.
const long MAX_TRAVEL_MM_A1 = 360; // Probably doesn't make sense to ask the dividin head to travel multiple turns.
const long BACKLASH_DU_A1 = 0; // Assuming no backlash on the worm gear
const char NAME_A1 = 'C'; // Text shown on screen before axis position value, GCode axis name

// ---------------------------------------------------------------------------
// Manual handwheels on the A1 and A2 terminals - ignore if not fitted
// ---------------------------------------------------------------------------

const bool PULSE_1_USE = false; // Whether there's a pulse generator connected on A11-A13 to be used for movement.
const char PULSE_1_AXIS = NAME_Z; // Set to NAME_X to make A11-A13 pulse generator control X instead.
const bool PULSE_1_INVERT = false; // Set to true to change the direction in which encoder moves the axis
const bool PULSE_2_USE = false; // Whether there's a pulse generator connected on A21-A23 to be used for movement.
const char PULSE_2_AXIS = NAME_X; // Set to NAME_Z to make A21-A23 pulse generator control Z instead.
const bool PULSE_2_INVERT = true; // Set to false to change the direction in which encoder moves the axis
const float PULSE_PER_REVOLUTION = 100; // PPR of handwheels used on A1 and/or A2.
const long PULSE_MIN_WIDTH_US = 1000; // Microseconds width of the pulse that is required for it to be registered. Prevents noise.
const long PULSE_HALF_BACKLASH = 2; // Prevents spurious reverses when moving using a handwheel. Raise to 3 or 4 if they still happen.

// ---------------------------------------------------------------------------
// Manual stepping feel
// ---------------------------------------------------------------------------

// How far the tool backs off the work when moving between cuts in an automated operation.
const long SAFE_DISTANCE_DU = 5000; // 0.5mm

// Only used when the step isn't the default continuous (1mm or 0.1").
const long STEP_TIME_MS = 500; // Time in milliseconds it should take to make 1 manual step.
const long DELAY_BETWEEN_STEPS_MS = 80; // Time in milliseconds to wait between steps.

// ---------------------------------------------------------------------------
// Keypad
// ---------------------------------------------------------------------------

// Key codes reported by the TCA8418 matrix controller. Change these only if your panel is wired
// to a different matrix layout - the "Input tester" calibration routine shows the code of
// whichever key you press, which is the easy way to remap a panel.
#define B_LEFT 57
#define B_RIGHT 37
#define B_UP 47
#define B_DOWN 67
#define B_MINUS 5
#define B_PLUS 64
#define B_ON 17
#define B_OFF 27
#define B_STOPL 7
#define B_STOPR 15
#define B_STOPU 6
#define B_STOPD 16
#define B_DISPL 14
#define B_STEP 24
#define B_SETTINGS 34
#define B_MEASURE 54
#define B_REVERSE 44
#define B_0 51
#define B_1 41
#define B_2 61
#define B_3 31
#define B_4 2
#define B_5 21
#define B_6 12
#define B_7 11
#define B_8 22
#define B_9 1
#define B_BACKSPACE 32
#define B_MODE_GEARS 42
#define B_MODE_TURN 52
#define B_MODE_FACE 62
#define B_MODE_CONE 3
#define B_MODE_CUT 13
#define B_MODE_THREAD 23
#define B_MODE_OTHER 33
#define B_X 53
#define B_Z 43
#define B_A 4
#define B_B 63

// ---------------------------------------------------------------------------
// Directional joystick on the A1 and A2 terminals - ignore if not fitted
// ---------------------------------------------------------------------------

// A digital stick with switches, not an analog one, plus two buttons. Deflecting the stick alone
// doesn't move anything: hold the move button to move the relevant axis in the deflected
// direction, observing any limits set. Releasing the button or centering the stick stops the
// move. The other button cycles the movement step exactly like the keypad step button.
//
// Not compatible with ACTIVE_A1, PULSE_1_USE or PULSE_2_USE - they need the same pins.
const bool JOYSTICK_USE = false;
const unsigned long JOYSTICK_DEBOUNCE_MS = 20; // Contact changes quicker than this are switch bounce

// Each switch shorts its pin to GND when active. Each row is {pin, key code of the keypad arrow
// the stick direction stands in for}. Remap to suit how your stick is mounted.
const int JOYSTICK_DIR_PIN_KEYS[4][2] = {
  {A11, B_LEFT},  // Stick left
  {A12, B_RIGHT}, // Stick right
  {A13, B_UP},    // Stick up
  {A21, B_DOWN},  // Stick down
};
#define JOYSTICK_MOVE_PIN A22 // Button that has to be held for the stick to move the axes
#define JOYSTICK_STEP_PIN A23 // Button cycling the movement step, same as the keypad step button

#endif
