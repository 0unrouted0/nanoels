// Everything that describes YOUR machine. This is the only file you should need to edit to run
// this firmware on different hardware - the sketch itself contains no machine-specific numbers.
//
// Moving these out of h4.ino means updating the firmware no longer means re-applying your own
// numbers by hand: keep this file, take the new sketch.
//
// The values here are the stock NanoEls H4 configuration, unchanged.

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

// Spindle rotary encoder pins. Swap values if the rotation direction is wrong.
#define ENC_A 7
#define ENC_B 15

#define BUZZ 4 // Buzzer
#define SCL 5  // I2C clock, to the keypad controller
#define SDA 6  // I2C data

// Auxiliary terminals, used by the A1 axis or the handwheels - whichever is enabled below.
// They cannot be shared.
#define A11 9
#define A12 10
#define A13 11

#define A21 12
#define A22 13
#define A23 14

// Character LCD in parallel mode: rs, enable, then the data lines.
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

const int ENCODER_PPR = 600; // 600 step spindle optical rotary encoder. Fractional values not supported.
const int ENCODER_BACKLASH = 3; // Numer of impulses encoder can issue without movement of the spindle
const int ENCODER_FILTER = 2; // Encoder pulses shorter than this will be ignored. Clock cycles, 1 - 1023.

// ---------------------------------------------------------------------------
// Main lead screw (Z)
// ---------------------------------------------------------------------------

const long SCREW_Z_DU = 20000; // 2mm lead screw in deci-microns (10^-7 of a meter)
const long MOTOR_STEPS_Z = 800;
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
const long MOTOR_STEPS_X = 2400; // 800 steps at 3x reduction
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

// Throughout this block we assume 1mm = 1degree of rotation, so 1du = 0.0001degree.
const bool ACTIVE_A1 = false; // Whether the axis is connected
const bool ROTARY_A1 = true; // Whether the axis is rotary or linear
const long MOTOR_STEPS_A1 = 300; // Number of motor steps for 1 rotation of the the worm gear screw (full step with 20:30 reduction)
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
const long SAFE_DISTANCE_DU = 5000; // Step back 0.5mm from the material when moving between cuts in automated modes

// Only used when step isn't default continuous (1mm or 0.1").
const long STEP_TIME_MS = 500; // Time in milliseconds it should take to make 1 manual step.
const long DELAY_BETWEEN_STEPS_MS = 80; // Time in milliseconds to wait between steps.

// ---------------------------------------------------------------------------
// Keypad
// ---------------------------------------------------------------------------

// Key codes reported by the TCA8418 matrix controller. Change these only if your panel is wired
// to a different matrix layout.
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

#endif
