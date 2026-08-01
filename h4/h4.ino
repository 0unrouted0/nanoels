// https://github.com/kachurovskiy/nanoels

// Everything describing YOUR machine - pins, axes, encoder, keypad, joystick - lives in
// machine_config.h. Nothing below this line should need changing to run on different hardware.
#include "machine_config.h"

// Arithmetic and the settings menu table, shared with the host test suite in test/.
#include "calibration_math.h"
#include "settings_table.h"
#include "pass_math.h"
#include "encoder_health.h"
#include "beeper.h"
#include "aux_pins.h"
#include "indexing.h"
#include "mode_settings.h"
#include "modes.h"

// Runtime copies of the machine_config.h defaults that the settings menu can change. The
// constants there are only the starting point for a device that has never been configured.
int encoderFilter = ENCODER_FILTER;
long safeDistanceDu = SAFE_DISTANCE_DU;
bool pulse1Use = PULSE_1_USE;
bool pulse1Invert = PULSE_1_INVERT;
bool pulse2Use = PULSE_2_USE;
bool pulse2Invert = PULSE_2_INVERT;
// Which axis each handwheel drives. Stored as a boolean - X when true, Z otherwise - because the
// settings table holds numbers, and these are the only two axes a handwheel can usefully turn.
bool pulse1DrivesX = (PULSE_1_AXIS == NAME_X);
bool pulse2DrivesX = (PULSE_2_AXIS == NAME_X);
float pulsePerRevolution = PULSE_PER_REVOLUTION;
long pulseMinWidthUs = PULSE_MIN_WIDTH_US;
long pulseHalfBacklash = PULSE_HALF_BACKLASH;
bool joystickUse = JOYSTICK_USE;
unsigned long joystickDebounceMs = JOYSTICK_DEBOUNCE_MS;
long stepTimeMs = STEP_TIME_MS;
long delayBetweenStepsMs = DELAY_BETWEEN_STEPS_MS;

int encoderPpr = ENCODER_PPR; // Currently active encoder PPR, can be changed in the settings menu
// Counts the pulse counter produces per revolution of the spindle. Recomputed by
// applyEncoderPpr() from the PPR, gearing and divider - see machine_config.h.
int ENCODER_STEPS_INT = ENCODER_PPR * ENCODER_COUNTS_PER_PULSE;
// Hardware counter span. On reaching +/-this the counter resets itself to zero; processSpindleCounter()
// detects that and corrects for it, so the counter is never cleared from software.
const int PCNT_LIM = 31000;
const long DUPR_MAX = 254000; // No more than 1 inch pitch
const int32_t STARTS_MAX = 124; // No more than 124-start thread
const long PASSES_MAX = MODE_PASSES_MAX; // No more turn or face passes than this
const long SAVE_DELAY_US = 5000000; // Wait 5s after last save and last change of saveable data before saving again
const long DIRECTION_SETUP_DELAY_US = 5; // Stepper driver needs some time to adjust to direction change
const long STEPPED_ENABLE_DELAY_MS = 100; // Delay after stepper is enabled and before issuing steps

// Version of the pref storage format, should be changed when non-backward-compatible
// changes are made to the storage logic, resulting in Preferences wipe on first start.
#define PREFERENCES_VERSION 1
#define PREF_NAMESPACE "h4"
#define GCODE_NAMESPACE "gc"

// GCode-related constants.
const float LINEAR_INTERPOLATION_PRECISION = 0.1; // 0 < x <= 1, smaller values make for quicker G0 and G1 moves
const long GCODE_WAIT_EPSILON_STEPS = 10;
const bool SPINDLE_PAUSES_GCODE = true; // pause GCode execution when spindle stops
const int GCODE_MIN_RPM = 30; // pause GCode execution if RPM is below this

// To be incremented whenever a measurable improvement is made.
#define SOFTWARE_VERSION 17




#define PREF_VERSION "v"
#define PREF_DUPR "d"
#define PREF_POS_Z "zp"
#define PREF_LEFT_STOP_Z "zls"
#define PREF_RIGHT_STOP_Z "zrs"
#define PREF_ORIGIN_POS_Z "zpo"
#define PREF_POS_GLOBAL_Z "zpg"
#define PREF_MOTOR_POS_Z "zpm"
#define PREF_DISABLED_Z "zd"
#define PREF_POS_X "xp"
#define PREF_LEFT_STOP_X "xls"
#define PREF_RIGHT_STOP_X "xrs"
#define PREF_ORIGIN_POS_X "xpo"
#define PREF_POS_GLOBAL_X "xpg"
#define PREF_MOTOR_POS_X "xpm"
#define PREF_DISABLED_X "xd"
#define PREF_POS_A1 "a1p"
#define PREF_LEFT_STOP_A1 "a1ls"
#define PREF_RIGHT_STOP_A1 "a1rs"
#define PREF_ORIGIN_POS_A1 "a1po"
#define PREF_POS_GLOBAL_A1 "a1pg"
#define PREF_MOTOR_POS_A1 "a1pm"
#define PREF_DISABLED_A1 "a1d"
#define PREF_SPINDLE_POS "sp"
#define PREF_SPINDLE_POS_AVG "spa"
#define PREF_OUT_OF_SYNC "oos"
#define PREF_SPINDLE_POS_GLOBAL "spg"
#define PREF_SHOW_ANGLE "ang"
#define PREF_SHOW_TACHO "rpm"
#define PREF_STARTS "sta"
#define PREF_MODE "mod"
#define PREF_MEASURE "mea"
#define PREF_CONE_RATIO "cr"
#define PREF_TURN_PASSES "tp"
#define PREF_MOVE_STEP "ms"
#define PREF_AUX_FORWARD "af"
#define PREF_SHOW_BDRO "bdro"
#define PREF_ENCODER_PPR "eppr"
#define PREF_SPRING_PASSES "spp"
#define PREF_PECK_DEPTH "pck"
#define PREF_FLANK_INFEED "fli"
#define PREF_RETRACT_DIST "rtd"
#define PREF_X_DIAMETER "xdd"
#define PREF_ENCODER_INVERT "einv"
#define PREF_ENCODER_BACKLASH "ebl"
#define PREF_ENCODER_SYMMETRIC "ebls"
#define PREF_ENCODER_SPINDLE_TEETH "est"
#define PREF_ENCODER_PULLEY_TEETH "ept"
#define PREF_ENCODER_DIVIDER "ediv"

#define MOVE_STEP_1 10000 // 1mm
#define MOVE_STEP_2 1000 // 0.1mm
#define MOVE_STEP_3 100 // 0.01mm

#define MOVE_STEP_IMP_1 25400 // 1/10"
#define MOVE_STEP_IMP_2 2540 // 1/100"
#define MOVE_STEP_IMP_3 254 // 1/1000" also known as 1 thou


#define MEASURE_METRIC 0
#define MEASURE_INCH 1
#define MEASURE_TPI 2

#define ESTOP_NONE 0
#define ESTOP_KEY 1
#define ESTOP_POS 2
#define ESTOP_MARK_ORIGIN 3
#define ESTOP_ON_OFF 4
#define ESTOP_OFF_MANUAL_MOVE 5

// For MEASURE_TPI, round TPI to the nearest integer if it's within this range of it.
// E.g. 80.02tpi would be shown as 80tpi but 80.04tpi would be shown as-is.
const float TPI_ROUND_EPSILON = 0.03;

float ENCODER_STEPS_FLOAT = ENCODER_STEPS_INT; // Convenience float version of ENCODER_STEPS_INT
long RPM_BULK = ENCODER_STEPS_INT; // Measure RPM averaged over this number of encoder pulses
const long RPM_UPDATE_INTERVAL_MICROS = 1000000; // Don't redraw RPM more often than once per second
// Minimum time between LCD redraws. taskDisplay is otherwise an unthrottled loop, which pushes
// redraws far faster than the glass can physically settle (~150ms on a typical HD44780) and
// leaves a half-rewritten line on screen much of the time - both of which read as smearing.
// Raise this if fast-moving digits are still hard to read, lower it if the screen feels laggy.
const int LCD_MIN_UPDATE_MS = 50; // 20 redraws per second

const long GCODE_FEED_DEFAULT_DU_SEC = 20000; // Default feed in du/sec in GCode mode
const float GCODE_FEED_MIN_DU_SEC = 167; // Minimum feed in du/sec in GCode mode - F1

#define DREAD(x) digitalRead(x)
#define DHIGH(x) digitalWrite(x, HIGH)
#define DLOW(x) digitalWrite(x, LOW)
#define DWRITE(x, y) digitalWrite(x, y)

#define DELAY(x) vTaskDelay(x / portTICK_PERIOD_MS);

// ESP32 hardware pulse counter library used to count spindle encoder pulses.
#include "driver/pcnt.h"

#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
#define LCD_HASH_INITIAL -3845709 // Random number that's unlikely to naturally occur as an actual hash
long lcdHashLine0 = LCD_HASH_INITIAL;
long lcdHashLine1 = LCD_HASH_INITIAL;
long lcdHashLine2 = LCD_HASH_INITIAL;
long lcdHashLine3 = LCD_HASH_INITIAL;
bool splashScreen = false;

#include <Preferences.h>

// Access point, config UI and firmware updates. Always compiled in, but nothing is started unless
// the "Enabled" item in Settings > WiFi & updates is on - see machine_config.h.
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include "web_page.h"
WebServer webServer(80);

#include <Adafruit_TCA8418.h>
Adafruit_TCA8418 keypad;
unsigned long keypadTimeUs = 0;

// Most buttons we only have "down" handling, holding them has no effect.
// Buttons with special "holding" logic have flags below.
bool buttonLeftPressed = false;
bool buttonRightPressed = false;
bool buttonUpPressed = false;
bool buttonDownPressed = false;
bool buttonOffPressed = false;
bool buttonGearsPressed = false;
bool buttonTurnPressed = false;
bool buttonAPressed = false; // We saw the press of B_A, so its release is ours to handle
unsigned long buttonAPressMs = 0; // Time B_A was pressed, to tell short and long presses apart
bool buttonSettingsPressed = false; // Same, for the settings button's short and long press
unsigned long buttonSettingsPressMs = 0;

// How long a key must be held to count as a long press. One value for every button that has two
// jobs, so the machine feels the same wherever the trick is used.
const unsigned long BUTTON_HOLD_MS = 500;

bool xRetracted = false; // X was moved away with the retract key, position to return to is stored below
long xRetractReturnPos = 0;

// Debounced state of the 6 joystick pins: indices 0-3 are JOYSTICK_DIR_PIN_KEYS rows, 4 move button, 5 step button.
bool joystickPinActive[6] = {false, false, false, false, false, false};
unsigned long joystickChangeMs[6] = {0, 0, 0, 0, 0, 0};
// Direction keys we've emitted a press for and not yet a release. Kept separate from pin state since
// a direction key is only considered pressed while the move button is also held.
bool joystickDirPressed[4] = {false, false, false, false};

bool inNumpad = false;
int numpadDigits[20];
int numpadIndex = 0;

bool isOn = false;
bool nextIsOn; // isOn value that should be applied asap
bool nextIsOnFlag; // whether nextIsOn requires attention
unsigned long resetMillis = 0;
int emergencyStop = 0;

// Which pattern the buzzer has been asked for, or BEEP_NONE. Written from any core - including
// the motion loop, which must not sit in tone() - and consumed by taskDisplay, which owns the
// buzzer and plays the pattern out step by step. A request arriving while one is playing replaces
// it: the newest event is the one worth hearing.
volatile int beepRequest = BEEP_NONE;
BeeperState beeper; // Only touched by taskDisplay

long dupr = 0; // pitch, tenth of a micron per rotation
long savedDupr = 0; // dupr saved in Preferences
long nextDupr = dupr; // dupr value that should be applied asap
bool nextDuprFlag = false; // whether nextDupr requires attention

SemaphoreHandle_t motionMutex; // controls blocks of code where variables affecting the motion loop() are changed

int starts = 1; // number of starts in a multi-start thread
int savedStarts = 0; // starts saved in Preferences
int nextStarts = starts; // number of starts that should be used asap
bool nextStartsFlag = false; // whether nextStarts requires attention

struct Axis {
  SemaphoreHandle_t mutex;

  char name;
  bool active;
  bool rotational;
  float motorSteps; // motor steps per revolution of the axis, after the pulley ratio
  float motorStepsPerTurn; // motor steps per revolution of the MOTOR, as set on the driver
  long motorTeeth; // teeth on the motor pulley
  long screwTeeth; // teeth on the lead screw pulley
  float screwPitch; // lead screw pitch in deci-microns (10^-7 of a meter)

  long pos; // relative position of the tool in stepper motor steps
  long savedPos; // value saved in Preferences
  float fractionalPos; // fractional distance in steps that we meant to travel but couldn't
  long originPos; // relative position of the stepper motor to origin, in steps
  long savedOriginPos; // originPos saved in Preferences
  long posGlobal; // global position of the motor in steps
  long savedPosGlobal; // posGlobal saved in Preferences
  int pendingPos; // steps of the stepper motor that we should make as soon as possible
  long motorPos; // position of the motor in stepper motor steps, same as pos unless moving back, then differs by backlashSteps
  long savedMotorPos; // motorPos saved in Preferences
  bool continuous; // whether current movement is expected to continue until an unknown position

  long leftStop; // left stop value of pos
  long savedLeftStop; // value saved in Preferences
  long nextLeftStop; // left stop value that should be applied asap
  bool nextLeftStopFlag; // whether nextLeftStop required attention

  long rightStop; // right stop value of pos
  long savedRightStop; // value saved in Preferences
  long nextRightStop; // right stop value that should be applied asap
  bool nextRightStopFlag; // whether nextRightStop requires attention

  long speed; // motor speed in steps / second
  long speedStart; // Initial speed of a motor, steps / second.
  long speedMax; // To limit max speed e.g. for manual moves
  long speedManualMove; // Maximum speed of a motor during manual move, steps / second.
  long acceleration; // Acceleration of a motor, steps / second ^ 2.
  long decelerateSteps; // Number of steps before the end position the deceleration should start.

  bool direction; // To reset speed when direction changes.
  bool directionInitialized;
  unsigned long stepStartUs;
  int stepperEnableCounter;
  bool disabled;
  bool savedDisabled;

  bool invertStepper; // change (true/false) if the carriage moves e.g. "left" when you press "right".
  bool needsRest; // set to false for closed-loop drivers, true for open-loop.
  bool movingManually; // whether stepper is being moved by left/right buttons
  long estopSteps; // amount of steps to exceed machine limits
  long backlashSteps; // amount of steps in reverse direction to re-engage the carriage
  long backlashDu; // backlash in deci-microns, used to re-derive backlashSteps when settings change
  long maxTravelMm; // max travel, used to re-derive estopSteps when settings change
  long gcodeRelativePos; // absolute position in steps that relative GCode refers to

  int ena; // Enable pin of this motor
  int dir; // Direction pin of this motor
  int step; // Step pin of this motor
};

// Re-calculates axis values that depend on the configurable hardware parameters.
// Must be called after motorSteps, screwPitch, backlashDu, speedManualMove or acceleration change.
void recomputeAxisDerived(Axis* a) {
  // Has to come first: everything below is in steps per revolution of the screw.
  a->motorSteps = calStepsPerScrewRev(a->motorStepsPerTurn, a->motorTeeth, a->screwTeeth);
  a->estopSteps = calEstopSteps(a->maxTravelMm, a->screwPitch, a->motorSteps);
  a->backlashSteps = calBacklashSteps(a->backlashDu, a->motorSteps, a->screwPitch);
  a->decelerateSteps = calDecelerateSteps(a->speedManualMove, a->speedStart, a->acceleration);
}

void initAxis(Axis* a, char name, bool active, bool rotational, float motorSteps, float screwPitch, long speedStart, long speedManualMove,
    long acceleration, bool invertStepper, bool needsRest, long maxTravelMm, long backlashDu, int ena, int dir, int step) {
  a->mutex = xSemaphoreCreateMutex();

  a->name = name;
  a->active = active;
  a->rotational = rotational;
  a->motorStepsPerTurn = motorSteps;
  a->motorTeeth = 1;
  a->screwTeeth = 1;
  a->motorSteps = motorSteps;
  a->screwPitch = screwPitch;

  a->pos = 0;
  a->savedPos = 0;
  a->fractionalPos = 0.0;
  a->originPos = 0;
  a->savedOriginPos = 0;
  a->posGlobal = 0;
  a->savedPosGlobal = 0;
  a->pendingPos = 0;
  a->motorPos = 0;
  a->savedMotorPos = 0;
  a->continuous = false;

  a->leftStop = 0;
  a->savedLeftStop = 0;
  a->nextLeftStopFlag = false;

  a->rightStop = 0;
  a->savedRightStop = 0;
  a->nextRightStopFlag = false;

  a->speed = speedStart;
  a->speedStart = speedStart;
  a->speedMax = LONG_MAX;
  a->speedManualMove = speedManualMove;
  a->acceleration = acceleration;

  a->direction = true;
  a->directionInitialized = false;
  a->stepStartUs = 0;
  a->stepperEnableCounter = 0;
  a->disabled = false;
  a->savedDisabled = false;

  a->invertStepper = invertStepper;
  a->needsRest = needsRest;
  a->movingManually = false;
  a->backlashDu = backlashDu;
  a->maxTravelMm = maxTravelMm;
  recomputeAxisDerived(a);
  a->gcodeRelativePos = 0;

  a->ena = ena;
  a->dir = dir;
  a->step = step;
}

Axis z;
Axis x;
Axis a1;

unsigned long saveTime = 0; // micros() of the previous Prefs write
unsigned long spindleEncTime = 0; // micros() of the previous spindle update
unsigned long spindleEncTimeDiffBulk = 0; // micros() between RPM_BULK spindle updates
unsigned long spindleEncTimeAtIndex0 = 0; // micros() when spindleEncTimeIndex was 0
int spindleEncTimeIndex = 0; // counter going between 0 and RPM_BULK - 1
bool encoderInvert = false; // Reverses the counting direction of the spindle encoder in software
int encoderBacklash = ENCODER_BACKLASH; // Dead-band in counts, see the constant for what it does
bool encoderSymmetric = ENCODER_SYMMETRIC; // Whether the dead-band filters both directions or one
EncHealth encHealth; // Signal-quality window, shown on the encoder calibration screen
int encoderSpindleTeeth = ENCODER_SPINDLE_TEETH; // Spindle pulley teeth, for a belt-driven encoder
int encoderPulleyTeeth = ENCODER_PULLEY_TEETH; // Encoder pulley teeth
int encoderDivider = ENCODER_DIVIDER; // Counts folded into one step to steady a fluttering encoder
int encoderDivRemainder = 0; // Counts not yet worth a step at the current divider
long encoderReversals = 0; // Spurious direction changes seen by the counter, reset from the calibration screen
int encoderLastDir = 0; // Sign of the last non-zero encoder delta, 0 until the first pulse
long encoderRunLength = 0; // Counts accumulated since the last direction change
long spindlePos = 0; // Spindle position
long spindlePosAvg = 0; // Spindle position accounting for encoder backlash
long savedSpindlePosAvg = 0; // spindlePosAvg saved in Preferences
long savedSpindlePos = 0; // spindlePos value saved in Preferences
int spindleCount = 0; // Last processed spindle encoder pulse counter value.
int spindlePosSync = 0; // Non-zero if gearbox is on and a soft limit was removed while axis was on it
int savedSpindlePosSync = 0; // spindlePosSync saved in Preferences
long spindlePosGlobal = 0; // global spindle position that is unaffected by e.g. zeroing
long savedSpindlePosGlobal = 0; // spindlePosGlobal saved in Preferences

volatile int pulse1Delta = 0; // Outstanding pulses generated by pulse generator on terminal A1.
volatile int pulse2Delta = 0; // Outstanding pulses generated by pulse generator on terminal A2.

long indexDivisions = INDEX_DIVISIONS; // Spindle divisions to show against, 0 or 1 = off
long indexToleranceTenths = INDEX_TOLERANCE_TENTHS_DEG; // How close counts as on the mark
long indexLastOnMark = -1; // Mark the buzzer last sounded for, so it sounds once per arrival
bool cssEnabled = CSS_ENABLED; // Whether the constant cutting speed readout replaces the tacho
long cssMaterial = CSS_MATERIAL; // Index into the material table, 0 = use the manual figure
bool cssCarbide = CSS_CARBIDE; // Tool the material speeds are read for
long surfaceSpeedMPerMin = SURFACE_SPEED_M_PER_MIN; // Manual target, used when material is Manual
long spindleMaxRpm = SPINDLE_MAX_RPM; // Highest rpm this lathe reaches, for capping the target

// The surface speed in force right now, or 0 when the feature is off. Everything that needs the
// figure goes through here, so the display, the web status and anything added later cannot
// disagree about whether the material or the typed number is in charge.
long cssActiveSpeed() {
  return cssEnabled ? cssSpeedFor(cssMaterial, cssCarbide, surfaceSpeedMPerMin) : 0;
}

bool showAngle = false; // Whether to show 0-359 spindle angle on screen
bool showTacho = false; // Whether to show spindle RPM and surface speed on screen
bool showBigDro = false; // Whether to show Z and X positions in large 2-row digits
bool savedShowAngle = false; // showAngle value saved in Preferences
bool savedShowTacho = false; // showTacho value saved in Preferences
bool savedShowBigDro = false; // showBigDro value saved in Preferences
int shownRpm = 0;
unsigned long shownRpmTime = 0; // micros() when shownRpm was set

long moveStep = 0; // thousandth of a mm
long savedMoveStep = 0; // moveStep saved in Preferences

volatile int mode = -1; // mode of operation (ELS, multi-start ELS, asynchronous)
int nextMode = 0; // mode value that should be applied asap
bool nextModeFlag = false; // whether nextMode needs attention
int savedMode = -1; // mode saved in Preferences

int measure = MEASURE_METRIC; // Whether to show distances in inches
int savedMeasure = MEASURE_METRIC; // measure value saved in Preferences

// How much X moves for 1 step of Z. Cone mode and tapered threading both use it and they used to
// share one value, so dialling an NPT 1:16 taper into TPR silently replaced whatever cone you had
// set - and taking the cone back out replaced the thread's taper. Mode-scoped like the rest of the
// per-operation settings now; this is the live copy for whichever mode is selected.
float coneRatio = 1;
float savedConeRatio = 0; // value of coneRatio saved in Preferences
float nextConeRatio = 0; // coneRatio that should be applied asap
bool nextConeRatioFlag = false; // whether nextConeRatio requires attention

int turnPasses = 3; // In turn mode, how many turn passes to make
int savedTurnPasses = 0; // value of turnPasses saved in Preferences

long setupIndex = 0; // Index of automation setup step
bool auxForward = true; // True for external, false for external thread
bool savedAuxForward = false; // value of auxForward saved in Preferences

long opIndex = 0; // Index of an automation operation
bool opIndexAdvanceFlag = false; // Whether user requested to move to the next pass
long opSubIndex = 0; // Sub-index of an automation operation
int opDuprSign = 1; // 1 if dupr was positive when operation started, -1 if negative
long opDupr = 0; // dupr that the multi-pass operation started with

const int customCharMmCode = 0;
byte customCharMm[] = {
  B11010,
  B10101,
  B10101,
  B00000,
  B11010,
  B10101,
  B10101,
  B00000
};
const int customCharLimUpCode = 1;
byte customCharLimUp[] = {
  B11111,
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00000,
  B00000
};
const int customCharLimDownCode = 2;
byte customCharLimDown[] = {
  B00000,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
  B11111,
  B00000
};
const int customCharLimLeftCode = 3;
byte customCharLimLeft[] = {
  B10000,
  B10010,
  B10100,
  B11111,
  B10100,
  B10010,
  B10000,
  B00000
};
const int customCharLimRightCode = 4;
byte customCharLimRight[] = {
  B00001,
  B01001,
  B00101,
  B11111,
  B00101,
  B01001,
  B00001,
  B00000
};
const int customCharLimUpDownCode = 5;
byte customCharLimUpDown[] = {
  B11111,
  B00100,
  B01110,
  B00000,
  B01110,
  B00100,
  B11111,
  B00000
};
const int customCharLimLeftRightCode = 6;
byte customCharLimLeftRight[] = {
  B00000,
  B10001,
  B10001,
  B11111,
  B10001,
  B10001,
  B00000,
  B00000
};
const int customCharDiaCode = 7;
byte customCharDia[] = {
  B00001,
  B01110,
  B10011,
  B10101,
  B11001,
  B01110,
  B10000,
  B00000
};

// Settings menu and thread database screens, both opened with the settings button.
bool inSettings = false; // Whether the settings menu is shown
// The settings menu is two levels. settingsSection is -1 while the directory of sections is
// shown, and the index of the open section once one has been entered.
int settingsSection = -1;
int settingsDirIndex = 0; // Highlighted row in the directory
bool inThreadPicker = false; // Whether the thread database is shown
int settingsIndex = 0; // Selected item in the settings menu
int threadPickerIndex = 0; // Selected item in the thread database
long settingsLcdHash = LCD_HASH_INITIAL; // Hash of the currently drawn settings/thread screen

// The menu items, their labels and their kinds live in settings_table.h, which also defines
// SETTINGS_COUNT. Only reading and writing the underlying variable is per-item code below.

// Settings that belong to the operation rather than to the machine. Each mode keeps its own copy
// in these arrays; the plain variables below are the live set for whichever mode is selected, and
// are what every motion and display path reads. Changing mode writes the live set back to the
// mode being left and loads the one being entered.
//
// The alternative was to make every reader index by mode, which would have touched the motion
// code in dozens of places to fix a storage problem.
ModeSettings modeSet[SMODE_COUNT];
bool modeScopedLoaded = false; // False until the arrays have been read from Preferences

long springPasses = 0; // Extra passes at final depth in turn/face/thread modes, 0 = off
long peckDepthDu = 0; // Peck parting: back off to break the chip every this much infeed in cut-off mode, 0 = off
bool flankInfeed = false; // Whether threading passes use 29.5-degree flank infeed instead of plunging
long retractDu = 20000; // One-key retract & return distance in deci-microns
long slotLeftReductionDu = 0; // Slotting: shorten each successive left stroke by this, 0 = full length
bool xDiameterDisplay = false; // Show and enter X values as diameter instead of radius

// WiFi access point and over-the-air updates. Read once during setup(), so changing wifiEnabled
// takes a restart - the settings item says so.
bool wifiEnabled = WIFI_ENABLED;
long wifiPin = WIFI_PIN_DEFAULT; // WPA2 password, always exactly 8 digits
bool wifiUp = false; // Whether the access point is actually running
bool wifiRestart = false; // Set when the PIN changes, so a running access point picks it up
// Set while firmware is being written. Flash writes disable the instruction cache and stall BOTH
// cores, so no step pulse can be trusted for the duration - loop() bails out on this the same way
// it does on an emergency stop.
bool otaInProgress = false;
int otaPercent = 0;

// Calibration screen. Guided routines that measure machine values on the actual lathe and store
// them under the same Preferences keys the settings menu uses, so calibrated and hand-entered
// values stay interchangeable. Opened from the last settings menu item.
#define CAL_PITCH_Z 0
#define CAL_PITCH_X 1
#define CAL_BACKLASH_Z 2
#define CAL_BACKLASH_X 3
#define CAL_DIR_Z 4
#define CAL_DIR_X 5
#define CAL_TRAVEL_Z 6
#define CAL_TRAVEL_X 7
#define CAL_SPEED_Z 8
#define CAL_SPEED_X 9
#define CAL_ENC_PPR 10
#define CAL_ENC_DIR 11
#define CAL_ENC_SIGNAL 12
#define CAL_INPUTS 13

// Routines 0-9 are axis-paired: even index is Z, odd is X, same convention as settingsAxis().
const char* CAL_ROUTINES[] = {
  "Z screw pitch", "X screw pitch", "Z backlash", "X backlash",
  "Z direction", "X direction", "Z travel limit", "X travel limit",
  "Z max speed", "X max speed", "Encoder PPR", "Encoder direction",
  "Encoder signal", "Input tester",
};
const int CAL_ROUTINES_COUNT = sizeof(CAL_ROUTINES) / sizeof(CAL_ROUTINES[0]);

// Nominal test distances for the screw pitch routine. Longer is more accurate.
const long CAL_TEST_DU[] = {10000, 50000, 100000, 250000, 500000, 1000000};
const int CAL_TEST_DU_COUNT = sizeof(CAL_TEST_DU) / sizeof(CAL_TEST_DU[0]);
// Revolutions to turn the spindle through when deriving encoder PPR.
const long CAL_REVS[] = {1, 5, 10, 20};
const int CAL_REVS_COUNT = sizeof(CAL_REVS) / sizeof(CAL_REVS[0]);
// The standard PPR table that measurements snap to lives in calibration_math.h.
#define CAL_PITCH_PRELOAD_DU 10000 // 1mm move that loads the axis before the pitch test move
#define CAL_BACKLASH_LOAD_DU 20000 // 2mm move used to take up the slack before measuring backlash
#define CAL_DIR_MOVE_DU 50000 // 5mm move used to show which way an axis travels
#define CAL_SPEED_STROKE_DU 200000 // 20mm back-and-forth stroke for the max speed ramp

bool inCal = false; // Whether the calibration screen is shown
int calRoutine = -1; // -1 = showing the routine list, otherwise the running CAL_* routine
int calListIndex = 0; // Selected routine in the list
int calStep = 0; // Step within the running routine
long calRefPos = 0; // Axis pos snapshot taken at the start of a measurement
long calRefSpindle = 0; // spindlePos snapshot for the encoder routines
int calParamIndex = 0; // Index into CAL_TEST_DU or CAL_REVS, whichever the routine uses
long calValue = 0; // Computed candidate value awaiting confirmation
long calPrev = 0; // Previous trial value, used by the max speed ramp
long calSaved = 0; // Original value stashed so aborting a routine can put it back
long calLcdHash = LCD_HASH_INITIAL; // Hash of the currently drawn calibration screen
int calLastKeyCode = -1; // Last key seen by the input tester
bool calLastKeyPress = false; // Whether that key was a press or a release
unsigned long calOffPressMs = 0; // millis() when OFF was pressed, for the hold-to-exit escape
bool calDirMoved = false; // Whether the direction routine has completed its test move

struct ThreadPreset {
  const char* name;
  long dupr; // pitch in deci-microns
  int measure; // measurement system this thread is normally expressed in
};

const ThreadPreset THREAD_PRESETS[] = {
  {"M3 coarse x0.5", 5000, MEASURE_METRIC},
  {"M4 coarse x0.7", 7000, MEASURE_METRIC},
  {"M5 coarse x0.8", 8000, MEASURE_METRIC},
  {"M6 coarse x1.0", 10000, MEASURE_METRIC},
  {"M8 coarse x1.25", 12500, MEASURE_METRIC},
  {"M8 fine x1.0", 10000, MEASURE_METRIC},
  {"M10 coarse x1.5", 15000, MEASURE_METRIC},
  {"M10 fine x1.25", 12500, MEASURE_METRIC},
  {"M12 coarse x1.75", 17500, MEASURE_METRIC},
  {"M12 fine x1.5", 15000, MEASURE_METRIC},
  {"M14 coarse x2.0", 20000, MEASURE_METRIC},
  {"M16 coarse x2.0", 20000, MEASURE_METRIC},
  {"M16 fine x1.5", 15000, MEASURE_METRIC},
  {"M20 coarse x2.5", 25000, MEASURE_METRIC},
  {"M24 coarse x3.0", 30000, MEASURE_METRIC},
  {"1/4-20 UNC", 12700, MEASURE_TPI},
  {"1/4-28 UNF", 9071, MEASURE_TPI},
  {"5/16-18 UNC", 14111, MEASURE_TPI},
  {"5/16-24 UNF", 10583, MEASURE_TPI},
  {"3/8-16 UNC", 15875, MEASURE_TPI},
  {"3/8-24 UNF", 10583, MEASURE_TPI},
  {"7/16-14 UNC", 18143, MEASURE_TPI},
  {"1/2-13 UNC", 19538, MEASURE_TPI},
  {"1/2-20 UNF", 12700, MEASURE_TPI},
  {"5/8-11 UNC", 23091, MEASURE_TPI},
  {"5/8-18 UNF", 14111, MEASURE_TPI},
  {"3/4-10 UNC", 25400, MEASURE_TPI},
  {"3/4-16 UNF", 15875, MEASURE_TPI},
  {"1-8 UNC", 31750, MEASURE_TPI},
  {"G1/8 BSPP 28tpi", 9071, MEASURE_TPI},
  {"G1/4 BSPP 19tpi", 13368, MEASURE_TPI},
  {"G3/8 BSPP 19tpi", 13368, MEASURE_TPI},
  {"G1/2 BSPP 14tpi", 18143, MEASURE_TPI},
  {"G3/4 BSPP 14tpi", 18143, MEASURE_TPI},
  {"G1 BSPP 11tpi", 23091, MEASURE_TPI},
  {"Tr8x1.5 trap", 15000, MEASURE_METRIC},
  {"Tr10x2 trap", 20000, MEASURE_METRIC},
  {"Tr12x3 trap", 30000, MEASURE_METRIC},
  {"Tr16x4 trap", 40000, MEASURE_METRIC},
  {"Tr20x4 trap", 40000, MEASURE_METRIC},
  {"1/4-16 ACME", 15875, MEASURE_TPI},
  {"5/16-14 ACME", 18143, MEASURE_TPI},
  {"3/8-12 ACME", 21167, MEASURE_TPI},
  {"1/2-10 ACME", 25400, MEASURE_TPI},
  {"5/8-8 ACME", 31750, MEASURE_TPI},
  {"3/4-6 ACME", 42333, MEASURE_TPI},
  {"1-5 ACME", 50800, MEASURE_TPI},
  {"1/8-27 NPT", 9407, MEASURE_TPI},
  {"1/4-18 NPT", 14111, MEASURE_TPI},
  {"3/8-18 NPT", 14111, MEASURE_TPI},
  {"1/2-14 NPT", 18143, MEASURE_TPI},
  {"3/4-14 NPT", 18143, MEASURE_TPI},
};
const int THREAD_PRESETS_COUNT = sizeof(THREAD_PRESETS) / sizeof(THREAD_PRESETS[0]);

// Thread database categories jumped to with numpad keys 1-6: the picker moves to the first
// preset whose name contains the marker.
const char* THREAD_CATEGORY_MARKERS[6] = {"M", "UN", "BSPP", "Tr", "ACME", "NPT"};

String gcodeCommand = "";
long gcodeFeedDuPerSec = GCODE_FEED_DEFAULT_DU_SEC;
bool gcodeInitialized = false;
bool gcodeAbsolutePositioning = true;
bool gcodeInBrace = false;
bool gcodeInSemicolon = false;
bool serialInKeycode = false;
int serialKeycode = 0;
String keycodeCommand = "";
bool serialInSetting = false; // Reading a "$..." settings command up to the end of the line
String settingCommand = "";
// Flash wear instrumentation, reported by "$". Since boot, not persisted. Each batch is one
// Preferences commit, so this is the number that maps to flash erase cycles.
unsigned long nvsSaveBatches = 0;
bool gcodeInSave = false;
bool gcodeInSaveFirstLine = false;
String gcodeSaveName = "";
String gcodeSaveValue = "";
int gcodeProgramIndex = 0;
int gcodeProgramCount = 0;
String gcodeProgram = "";
int gcodeProgramCharIndex = 0;

hw_timer_t *async_timer = timerBegin(80);
bool timerAttached = false;

int getApproxRpm() {
  unsigned long t = micros();
  if (t > spindleEncTime + 50000) {
    // RPM less than 10.
    spindleEncTimeDiffBulk = 0;
    return 0;
  }
  if (t < shownRpmTime + RPM_UPDATE_INTERVAL_MICROS) {
    // Don't update RPM too often to avoid flickering.
    return shownRpm;
  }
  int rpm = 0;
  if (spindleEncTimeDiffBulk > 0) {
    rpm = calRpmFromBulkMicros(spindleEncTimeDiffBulk);
    if (abs(rpm - shownRpm) > (rpm < 1000 ? 3 : 5)) {
      // Don't update RPM with insignificant differences.
      shownRpm = rpm;
      shownRpmTime = t;
    }
  }
  return rpm;
}

bool stepperIsRunning(Axis* a) {
  return micros() - a->stepStartUs < 50000;
}

// Returns number of letters printed.
int printDeciMicrons(long deciMicrons, int precisionPointsMax) {
  if (deciMicrons == 0) {
    return lcd.print("0");
  }
  bool imperial = measure != MEASURE_METRIC;
  long v = imperial ? round(deciMicrons / 25.4) : deciMicrons;
  int points = 0;
  if (v == 0 && precisionPointsMax >= 5) {
    points = 5;
  } else if ((v % 10) != 0 && precisionPointsMax >= 4) {
    points = 4;
  } else if ((v % 100) != 0 && precisionPointsMax >= 3) {
    points = 3;
  } else if ((v % 1000) != 0 && precisionPointsMax >= 2) {
    points = 2;
  } else if ((v % 10000) != 0 && precisionPointsMax >= 1) {
    points = 1;
  }
  int count = lcd.print(deciMicrons / (imperial ? 254000.0 : 10000.0), points);
  count += imperial ? lcd.print("\"") : lcd.write(customCharMmCode);
  return count;
}

int printDegrees(long degrees10000) {
  int points = 0;
  if ((degrees10000 % 100) != 0) {
    points = 3;
  } else if ((degrees10000 % 1000) != 0) {
    points = 2;
  } else if ((degrees10000 % 10000) != 0) {
    points = 1;
  }
  int count = lcd.print(degrees10000 / 10000.0, points);
  count += lcd.print(char(223)); // degree symbol
  return count;
}

int printDupr(long value) {
  int count = 0;
  if (measure != MEASURE_TPI) {
    count += printDeciMicrons(value, 5);
  } else {
    float tpi = 254000.0 / value;
    if (abs(tpi - round(tpi)) < TPI_ROUND_EPSILON) {
      count += lcd.print(int(round(tpi)));
    } else {
      int tpi100 = round(tpi * 100);
      int points = 0;
      if ((tpi100 % 10) != 0) {
        points = 2;
      } else if ((tpi100 % 100) != 0) {
        points = 1;
      }
      count += lcd.print(tpi, points);
    }
    count += lcd.print("tpi");
  }
  return count;
}

void printLcdSpaces(int charIndex) {
  // Our screen has width 20.
  for (; charIndex < 20; charIndex++) {
    lcd.print(" ");
  }
}

long stepsToDu(Axis* a, long steps) {
  return calStepsToDu(a->screwPitch, a->motorSteps, steps);
}

long getAxisPosDu(Axis* a) {
  return stepsToDu(a, a->pos + a->originPos);
}

long getAxisStopDiffDu(Axis* a) {
  if (a->leftStop == LONG_MAX || a->rightStop == LONG_MIN) return 0;
  return stepsToDu(a, a->leftStop - a->rightStop);
}

int printAxisPos(Axis* a) {
  if (a->rotational) {
    return printDegrees(getAxisPosDu(a));
  }
  if (a == &x && xDiameterDisplay) {
    return printDeciMicrons(getAxisPosDu(a) * 2, 3);
  }
  return printDeciMicrons(getAxisPosDu(a), 3);
}

int printAxisStopDiff(Axis* a, bool addTrailingSpace) {
  int count = 0;
  if (a->rotational) {
    count = printDegrees(getAxisStopDiffDu(a));
  } else {
    count = printDeciMicrons(getAxisStopDiffDu(a), 3);
  }
  if (addTrailingSpace) {
    count += lcd.print(' ');
  }
  return count;
}

int printAxisPosWithName(Axis* a, bool addTrailingSpace) {
  if (!a->active || a->disabled) return 0;
  int count = 0;
  if (a == &x && xDiameterDisplay && !a->rotational) {
    count += lcd.write(customCharDiaCode);
  } else {
    count += lcd.print(a->name);
  }
  count += printAxisPos(a);
  if (addTrailingSpace) {
    count += lcd.print(' ');
  }
  return count;
}

int printNoTrailing0(float value) {
  long v = round(value * 100000);
  int points = 0;
  if ((v % 10) != 0) {
    points = 5;
  } else if ((v % 100) != 0) {
    points = 4;
  } else if ((v % 1000) != 0) {
    points = 3;
  } else if ((v % 10000) != 0) {
    points = 2;
  } else if ((v % 100000) != 0) {
    points = 1;
  }
  return lcd.print(value, points);
}

// All of these are one-line adaptors onto modes.h, which is where the answers live so the test
// suite can reach them. They stay because the sketch reads them constantly and "isPassMode()" is
// easier to follow at a call site than "modeIsPass(mode)".
bool isThreadMode() { return modeIsThread(mode); }
bool isGearboxMode() { return modeIsGearbox(mode); }
bool needZStops() { return modeNeedsZStops(mode); }
bool isPassMode() { return modeIsPass(mode); }
bool manualMovesAllowedWhenOn() { return modeAllowsManualMovesWhenOn(mode); }
int getLastSetupIndex() { return modeLastSetupIndex(mode); }

// "2/3 " in front of a wizard question. Without it there is nothing to say the questions are a
// sequence with an end rather than an unbounded interrogation, and no way to tell how far in you
// are - which is most of what makes the pass modes hard to approach.
int printSetupStep() {
  int n = lcd.print(setupIndex);
  n += lcd.print("/");
  n += lcd.print(getLastSetupIndex());
  return n + lcd.print(" ");
}

Axis* getPitchAxis() {
  return modePitchOnX(mode) ? &x : &z;
}

long getPassModeZStart() {
  return modeZStart(mode, z.leftStop, z.rightStop, z.pos, dupr, auxForward);
}

long getPassModeXStart() {
  return modeXStart(mode, x.leftStop, x.rightStop, x.pos, dupr, auxForward);
}

const char* modeName() { return modeNameOf(mode); }
int settingModeOf(int m) { return modeSettingsOf(m); }

// Live set -> the mode's own copy, and back. Called either side of a mode change.
void modeScopedStore(int sm) {
  if (sm == SMODE_NONE) return;
  modeSet[sm].passes = turnPasses;
  modeSet[sm].springPasses = springPasses;
  modeSet[sm].clearanceDu = safeDistanceDu;
  modeSet[sm].peckDu = peckDepthDu;
  modeSet[sm].flankInfeed = flankInfeed;
  modeSet[sm].slotReductionDu = slotLeftReductionDu;
  modeSet[sm].taper = coneRatio;
}

void modeScopedLoad(int sm) {
  if (sm == SMODE_NONE) return;
  turnPasses = modeSet[sm].passes;
  savedTurnPasses = turnPasses; // it has not changed, it belongs to a different mode now
  springPasses = modeSet[sm].springPasses;
  safeDistanceDu = modeSet[sm].clearanceDu;
  peckDepthDu = modeSet[sm].peckDu;
  flankInfeed = modeSet[sm].flankInfeed;
  slotLeftReductionDu = modeSet[sm].slotReductionDu;
  coneRatio = modeSet[sm].taper;
  savedConeRatio = coneRatio; // it has not changed, it belongs to a different mode now
  // A pending taper from the mode being left must not land on the one being entered.
  nextConeRatio = coneRatio;
  nextConeRatioFlag = false;
}

// Reading and writing a mode-scoped item by its suffix. The live variables hold the current
// mode's values, so a row belonging to the mode you are in has to go through them - the array is
// only up to date for the modes you are not in.
long modeScopedRead(int sm, const char* k) {
  if (sm == settingModeOf(mode)) modeScopedStore(sm);
  return modeSettingRead(&modeSet[sm], k);
}

const char* modeScopedWrite(int sm, const char* k, long value) {
  const char* err = modeSettingWrite(&modeSet[sm], k, value);
  if (err != NULL) return err;
  // Editing the mode you are standing in has to reach the live set too, or the change would only
  // appear after switching away and back.
  if (sm == settingModeOf(mode)) modeScopedLoad(sm);
  return NULL;
}

// Whether this mode claims the short press of the settings button for a screen of its own.
//
// Threading is the only one so far: its short press opens the thread database. The long press
// reaches the settings menu from anywhere, which is what makes the short press free to give away
// - before that, thread mode had no route to the settings menu at all, so a threading setting
// like spring passes or flank infeed meant leaving the mode to change it.
//
// A mode that grows a preset list or a page of its own adds itself here and to
// enterSettingsScreen(), and nothing else has to move.
bool modeHasOwnScreen() {
  return settingModeCount(settingModeOf(mode)) > 0;
}

bool modeHasSibling() { return modeHasSiblingOf(mode); }

int printMode() {
  const char* name = modeName();
  if (name[0] == 0 && !modeHasSibling()) {
    return 0;
  }
  int n = 0;
  if (name[0] != 0) n += lcd.print(name);
  // Plain ASCII, not a custom glyph: all 8 CGRAM slots are already spoken for.
  if (modeHasSibling()) n += lcd.print("*");
  return n + lcd.print(" ");
}

// Transient message shown on the bottom display line, mostly explaining why an input was refused.
char splashBuf[21] = "";
unsigned long splashUntilMs = 0;
int splashId = 0; // Incremented per message so the display hash catches back-to-back splashes

void splash(const char* text) {
  strncpy(splashBuf, text, sizeof(splashBuf) - 1);
  splashBuf[sizeof(splashBuf) - 1] = 0;
  splashUntilMs = millis() + 1500;
  splashId++;
}

bool splashActive() {
  return millis() < splashUntilMs;
}

// A refused input: the message and the beep always travel together.
void splashError(const char* text) {
  splash(text);
  beepFor(BEEP_REFUSED);
}

// Deci-microns for a number just typed on the numpad, in the current measurement system.
long numpadRawToDu(long raw) {
  return calNumpadRawToDu(measure == MEASURE_INCH, raw);
}

// Big 3x2-cell digit font for the large DRO screen. Since the LCD only has 8 custom character
// slots, shared with the normal screen's icons, the character set is swapped when (de)entering it.
byte bigCharLT[] = {B00111, B01111, B11111, B11111, B11111, B11111, B11111, B11111};
byte bigCharUB[] = {B11111, B11111, B11111, B00000, B00000, B00000, B00000, B00000};
byte bigCharRT[] = {B11100, B11110, B11111, B11111, B11111, B11111, B11111, B11111};
byte bigCharLL[] = {B11111, B11111, B11111, B11111, B11111, B11111, B01111, B00111};
byte bigCharLB[] = {B00000, B00000, B00000, B00000, B00000, B11111, B11111, B11111};
byte bigCharLR[] = {B11111, B11111, B11111, B11111, B11111, B11111, B11110, B11100};
// Middle bar is 1px here and 2px in bigCharLMB below, so the crossbar totals 3px across the
// cell boundary - the same weight as the top and bottom bars. It used to be 2px+2px, which made
// the crossbar visibly fatter than every other stroke in the digit.
byte bigCharUMB[] = {B11111, B11111, B11111, B00000, B00000, B00000, B00000, B11111};
byte bigCharLMB[] = {B11111, B11111, B00000, B00000, B00000, B11111, B11111, B11111};

// Cells of each digit: 3 top row cells, then 3 bottom row cells. 32 = space, 255 = full block.
const byte BIG_DIGIT_CELLS[10][6] = {
  {0, 1, 2, 3, 4, 5},         // 0
  {32, 255, 32, 32, 255, 32}, // 1
  {6, 6, 2, 3, 7, 7},         // 2
  {6, 6, 2, 7, 7, 5},         // 3
  {255, 4, 255, 32, 32, 255}, // 4
  {255, 6, 6, 7, 7, 5},       // 5
  {255, 6, 6, 3, 7, 5},       // 6
  {1, 1, 2, 32, 32, 255},     // 7
  {0, 6, 2, 3, 7, 5},         // 8
  {0, 6, 2, 7, 7, 5},         // 9
};

bool lcdBigCharsLoaded = false;
long bigDroHash = LCD_HASH_INITIAL;
// Columns 0-13 of the large display hold the number, 14-19 the info strip.
#define BIG_DRO_NUM_COLS 14

void lcdLoadNormalChars() {
  lcd.createChar(customCharMmCode, customCharMm);
  lcd.createChar(customCharLimLeftCode, customCharLimLeft);
  lcd.createChar(customCharLimRightCode, customCharLimRight);
  lcd.createChar(customCharLimUpCode, customCharLimUp);
  lcd.createChar(customCharLimDownCode, customCharLimDown);
  lcd.createChar(customCharLimUpDownCode, customCharLimUpDown);
  lcd.createChar(customCharLimLeftRightCode, customCharLimLeftRight);
  lcd.createChar(customCharDiaCode, customCharDia);
}

void lcdLoadBigChars() {
  lcd.createChar(0, bigCharLT);
  lcd.createChar(1, bigCharUB);
  lcd.createChar(2, bigCharRT);
  lcd.createChar(3, bigCharLL);
  lcd.createChar(4, bigCharLB);
  lcd.createChar(5, bigCharLR);
  lcd.createChar(6, bigCharUMB);
  lcd.createChar(7, bigCharLMB);
}

void ensureLcdCharset(bool big) {
  if (big == lcdBigCharsLoaded) return;
  lcdBigCharsLoaded = big;
  if (big) lcdLoadBigChars();
  else lcdLoadNormalChars();
  // The glyphs on screen just changed meaning, force a full redraw.
  lcdHashLine0 = LCD_HASH_INITIAL;
  bigDroHash = LCD_HASH_INITIAL;
}

// Renders an axis value in 2-row-tall digits on rows row and row + 1, right-aligned into the
// columns left of the info strip. Always 4 digits so the strip never has to move, which means
// 0.01mm below 100mm and 0.1mm above it (0.001" and 0.01" in imperial). The normal screen and
// numpad entry keep full micron precision - only this screen trades the last digit for space,
// and it is the digit that changes fastest and is least readable while an axis is moving.
void printBigValue(int row, char label, long du) {
  char buf[12];
  float value = measure == MEASURE_METRIC ? du / 10000.0 : du / 254000.0;
  float mag = fabs(value);
  snprintf(buf, sizeof(buf), "%.*f", calBigDroPoints(measure == MEASURE_METRIC, mag), mag);
  int col = calBigDroStartCol(BIG_DRO_NUM_COLS, calBigDroWidth(buf));

  lcd.setCursor(0, row);
  lcd.print(label);
  lcd.setCursor(0, row + 1);
  lcd.print(du < 0 ? '-' : ' ');
  lcd.setCursor(1, row);
  for (int i = 1; i < col; i++) lcd.print(' ');
  lcd.setCursor(1, row + 1);
  for (int i = 1; i < col; i++) lcd.print(' ');

  for (int i = 0; buf[i] != 0 && col < BIG_DRO_NUM_COLS; i++) {
    char c = buf[i];
    if (c == '.') {
      lcd.setCursor(col, row);
      lcd.print(' ');
      lcd.setCursor(col, row + 1);
      lcd.print('.');
      col++;
    } else if (c >= '0' && c <= '9' && col + 3 <= BIG_DRO_NUM_COLS) {
      const byte* cells = BIG_DIGIT_CELLS[c - '0'];
      lcd.setCursor(col, row);
      for (int j = 0; j < 3; j++) lcd.write(cells[j]);
      lcd.setCursor(col, row + 1);
      for (int j = 3; j < 6; j++) lcd.write(cells[j]);
      col += 3;
    }
  }
  lcd.setCursor(col, row);
  for (int i = col; i < BIG_DRO_NUM_COLS; i++) lcd.print(' ');
  lcd.setCursor(col, row + 1);
  for (int i = col; i < BIG_DRO_NUM_COLS; i++) lcd.print(' ');
}

// Info strip down the right-hand side of the large position display.
//
// The disabled flags matter most. This screen shows commanded stepper travel, not a measured
// carriage position - there is no scale on the axes - so the moment an axis is disabled and
// hand-cranked, the big number beside it silently stops being true. The flag sits on that
// axis's own rows so it is impossible to misread which number has gone stale.
void printDroStrip() {
  int n;
  lcd.setCursor(BIG_DRO_NUM_COLS, 0);
  n = z.disabled ? lcd.print(" Z OFF") : lcd.print("   rpm");
  printLcdSpaces(BIG_DRO_NUM_COLS + n);

  lcd.setCursor(BIG_DRO_NUM_COLS, 1);
  n = lcd.print(" ");
  n += lcd.print(getApproxRpm());
  printLcdSpaces(BIG_DRO_NUM_COLS + n);

  lcd.setCursor(BIG_DRO_NUM_COLS, 2);
  n = x.disabled ? lcd.print(" X OFF") : lcd.print("  step");
  printLcdSpaces(BIG_DRO_NUM_COLS + n);

  lcd.setCursor(BIG_DRO_NUM_COLS, 3);
  n = lcd.print(" ");
  n += printDeciMicrons(moveStep, 5);
  printLcdSpaces(BIG_DRO_NUM_COLS + n);
}

void updateBigDroDisplay() {
  long zDu = getAxisPosDu(&z);
  long xDu = getAxisPosDu(&x) * (xDiameterDisplay ? 2 : 1);
  long newHash = zDu + xDu * 7 + measure * 3 + getApproxRpm() * 11L + moveStep * 13L
      + (z.disabled ? 101 : 0) + (x.disabled ? 202 : 0);
  if (newHash == bigDroHash) return;
  bigDroHash = newHash;
  printBigValue(0, 'Z', zDu);
  // All 8 custom character slots hold the big font here, so the diameter symbol the normal
  // screen uses isn't available. 'D' still says unambiguously that this is a diameter, which
  // matters: the value is doubled and an X label would read as a radius.
  printBigValue(2, xDiameterDisplay ? 'D' : 'X', xDu);
  printDroStrip();
}

// Startup screen: name, version and a summary of the active hardware settings so a wrong
// config (e.g. after a settings change) can be spotted before making chips.
void showStartupScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("NanoEls H" + String(HARDWARE_VERSION) + " V" + String(SOFTWARE_VERSION));
  lcd.setCursor(0, 1);
  lcd.print("Encoder PPR " + String(encoderPpr));
  lcd.setCursor(0, 2);
  lcd.print("Z ");
  printNoTrailing0(z.screwPitch / 10000.0);
  lcd.write(customCharMmCode);
  lcd.print(" " + String(long(round(z.motorSteps))) + "step");
  lcd.setCursor(0, 3);
  lcd.print("X ");
  printNoTrailing0(x.screwPitch / 10000.0);
  lcd.write(customCharMmCode);
  lcd.print(" " + String(long(round(x.motorSteps))) + "step");
  lcdHashLine0 = LCD_HASH_INITIAL;
  lcdHashLine1 = LCD_HASH_INITIAL;
  lcdHashLine2 = LCD_HASH_INITIAL;
  lcdHashLine3 = LCD_HASH_INITIAL;
  delay(2000);
}

void updateDisplay() {
  // Firmware is being written: nothing else may draw, and nothing else is worth showing.
  if (otaInProgress) {
    updateOtaDisplay();
    return;
  }
  if (splashScreen) {
    splashScreen = false;
    ensureLcdCharset(false);
    showStartupScreen();
  }
  if (inCal) {
    ensureLcdCharset(false);
    updateCalDisplay();
    return;
  }
  if (inSettings || inThreadPicker) {
    ensureLcdCharset(false);
    updateSettingsDisplay();
    return;
  }
  if (showBigDro && !inNumpad && !splashActive() && setupIndex == 0 && (!isOn || mode == MODE_NORMAL)) {
    ensureLcdCharset(true);
    updateBigDroDisplay();
    return;
  }
  ensureLcdCharset(false);
  int rpm = showTacho ? getApproxRpm() : 0;
  int charIndex = 0;
  if (lcdHashLine0 == LCD_HASH_INITIAL) {
    // First run after reset.
    lcd.clear();
    lcdHashLine1 = LCD_HASH_INITIAL;
    lcdHashLine2 = LCD_HASH_INITIAL;
    lcdHashLine3 = LCD_HASH_INITIAL;
  }

  long newHashLine0 = isOn + (z.leftStop - z.rightStop) + (x.leftStop - x.rightStop) + spindlePosSync + moveStep + mode + measure + setupIndex * 10;
  if (lcdHashLine0 != newHashLine0) {
    lcdHashLine0 = newHashLine0;
    charIndex = 0;
    lcd.setCursor(0, 0);
    if (setupIndex == 0 || !isPassMode()) {
      charIndex += printMode();
      charIndex += lcd.print(isOn ? "ON " : "off ");
      int beforeStops = charIndex;
      if (z.leftStop != LONG_MAX) {
        charIndex += lcd.write(customCharLimLeftCode);
      }
      if (x.leftStop != LONG_MAX && x.rightStop != LONG_MIN) {
        charIndex += lcd.write(customCharLimUpDownCode);
      } else if (x.leftStop != LONG_MAX) {
        charIndex += lcd.write(customCharLimUpCode);
      } else if (x.rightStop != LONG_MIN) {
        charIndex += lcd.write(customCharLimDownCode);
      }
      if (z.rightStop != LONG_MIN) {
        charIndex += lcd.write(customCharLimRightCode);
      }
      if (beforeStops != charIndex) {
        charIndex += lcd.print(" ");
      }

      if (spindlePosSync && !isPassMode()) {
        charIndex += lcd.print("SYN ");
      }
      if (mode == MODE_NORMAL && !spindlePosSync) {
        charIndex += lcd.print("step ");
      }
      charIndex += printDeciMicrons(moveStep, 5);
    } else {
      if (needZStops()) {
        charIndex += lcd.write(customCharLimLeftRightCode);
        charIndex += printAxisStopDiff(&z, true);
        while (charIndex < 10) charIndex += lcd.print(" ");
      } else {
        charIndex += printMode();
      }
      charIndex += lcd.write(customCharLimUpDownCode);
      charIndex += printAxisStopDiff(&x, false);
    }
    printLcdSpaces(charIndex);
  }

  long newHashLine1 = dupr + starts + mode + measure + setupIndex;
  if (lcdHashLine1 != newHashLine1) {
    lcdHashLine1 = newHashLine1;
    charIndex = 0;
    lcd.setCursor(0, 1);
    charIndex += lcd.print("Pitch ");
    charIndex += printDupr(dupr);
    if (starts != 1) {
      charIndex += lcd.print(" x");
      charIndex += lcd.print(starts);
    }
    printLcdSpaces(charIndex);
  }

  long zDisplayPos = z.pos + z.originPos;
  long xDisplayPos = x.pos + x.originPos;
  long a1DisplayPos = a1.pos + a1.originPos;
  long newHashLine2 = zDisplayPos + xDisplayPos + a1DisplayPos + measure + z.disabled + x.disabled + mode;
  if (lcdHashLine2 != newHashLine2) {
    lcdHashLine2 = newHashLine2;
    charIndex = 0;
    lcd.setCursor(0, 2);
    charIndex += printAxisPosWithName(&z, true);
    while (charIndex < 10) charIndex += lcd.print(" ");
    charIndex += printAxisPosWithName(&x, true);
    printLcdSpaces(charIndex);
  }

  long numpadResult = getNumpadResult();
  long gcodeCommandHash = 0;
  for (int i = 0; i < gcodeCommand.length(); i++) {
    gcodeCommandHash += gcodeCommand.charAt(i);
  }
  bool spindleStopped = micros() > spindleEncTime + 100000;
  // Indexing reads the spindle whether or not the angle display is on, so it has to put
  // spindlePos in the hash too - without it the line would not refresh as the chuck turns.
  long newHashLine3 = z.pos + ((showAngle || indexDivisions > 1) ? spindlePos : -1) + (showTacho ? rpm + x.originPos : -2) + (splashActive() ? splashId * 7919 : -3) +
      (joystickUse && joystickPinActive[4] ? 2000 + joystickDirPressed[0] + 2 * joystickDirPressed[1] + 4 * joystickDirPressed[2] + 8 * joystickDirPressed[3] : 0) + measure + (numpadResult > 0 ? numpadResult : -1) + mode * 5 + dupr +
      (mode == MODE_CONE || mode == MODE_TPR ? round(coneRatio * 10000) : 0) + turnPasses + opIndex + setupIndex + gcodeProgramIndex + gcodeProgramCount + spindleStopped * 3 + (isOn ? 139 : -117) + (inNumpad ? 10 : 0) + (auxForward ? 17 : -31) + (xRetracted ? 55 : 0) +
      (z.leftStop == LONG_MAX ? 123 : z.leftStop) + (z.rightStop == LONG_MIN ? 1234 : z.rightStop) +
      (x.leftStop == LONG_MAX ? 1235 : x.leftStop) + (x.rightStop == LONG_MIN ? 123456 : x.rightStop) + gcodeCommandHash +
      (mode == MODE_A1 ? a1.pos + a1.originPos + (a1.leftStop == LONG_MAX ? 123 : a1.leftStop) + (a1.rightStop == LONG_MIN ? 1234 : a1.rightStop) + a1.disabled : 0) + x.pos + z.pos;
  if (lcdHashLine3 != newHashLine3) {
    lcdHashLine3 = newHashLine3;
    charIndex = 0;
    lcd.setCursor(0, 3);
    if (splashActive()) {
      charIndex += lcd.print(splashBuf);
    } else if (mode == MODE_A1 && !inNumpad) {
      if (a1.leftStop != LONG_MAX && a1.rightStop != LONG_MIN) {
        charIndex += lcd.write(customCharLimUpDownCode);
        charIndex += lcd.print(" ");
      } else if (a1.leftStop != LONG_MAX) {
        charIndex += lcd.write(customCharLimDownCode);
        charIndex += lcd.print(" ");
      } else if (a1.rightStop != LONG_MIN) {
        charIndex += lcd.write(customCharLimUpCode);
        charIndex += lcd.print(" ");
      }
      charIndex += printAxisPosWithName(&a1, false);
    } else if (mode == MODE_GCODE) {
      if (setupIndex == 1 && gcodeProgramCount == 0) {
        charIndex += lcd.print("No stored programs");
      } else if (setupIndex == 1) {
        Preferences pref;
        pref.begin(GCODE_NAMESPACE);
        if (gcodeProgramIndex >= gcodeProgramCount) {
          charIndex += lcd.print("Program deleted");
        } else {
          String programName = pref.getString(String(gcodeProgramIndex).c_str());
          if (programName.length() == 0) charIndex += lcd.print("(empty name)");
          else charIndex += lcd.print(programName.substring(0, 20));
        }
        pref.end();
      } else if (setupIndex == 2) {
        if (spindleStopped) charIndex += lcd.print("Turn on the spindle!");
        else charIndex += lcd.print("Spindle on. Go?");
      } else if (isOn) {
        charIndex += lcd.print(gcodeCommand.substring(0, 20));
      }
    } else if (isPassMode()) {
      bool missingZStops = needZStops() && (z.leftStop == LONG_MAX || z.rightStop == LONG_MIN);
      bool missingStops = missingZStops || x.leftStop == LONG_MAX || x.rightStop == LONG_MIN;
      if (!inNumpad && missingStops) {
        charIndex += lcd.print(needZStops() ? "Set all stops" : "Set X stops");
      } else if (numpadResult != 0 && setupIndex == 1) {
        charIndex += printSetupStep();
        long passes = min(PASSES_MAX, numpadResult);
        charIndex += lcd.print(passes);
        if (passes == 1) charIndex += lcd.print(" pass?");
        else charIndex += lcd.print(" passes?");
      } else if (!isOn && setupIndex == 1) {
        charIndex += printSetupStep();
        charIndex += lcd.print(turnPasses);
        if (turnPasses == 1) charIndex += lcd.print(" pass?");
        else charIndex += lcd.print(" passes?");
      } else if (!isOn && setupIndex == 2) {
        charIndex += printSetupStep();
        if (mode == MODE_FACE) {
          charIndex += lcd.print(auxForward ? "Right-left" : "Left-right");
        } else if (mode == MODE_CUT) {
          // Cut-off takes its direction from the sign of the pitch rather than from this
          // question, so there is nothing here to change with the arrows.
          charIndex += lcd.print(dupr >= 0 ? "Ext, pitch>0" : "Int, pitch<0");
        } else {
          charIndex += lcd.print(auxForward ? "External" : "Internal");
        }
        // Nothing else on the panel says the arrows change the answer, and the question mark on
        // its own reads as a yes/no that ON would answer.
        if (mode != MODE_CUT) charIndex += lcd.print(" <>");
      } else if (!isOn && mode == MODE_TPR && setupIndex == 3) {
        // Only the tapered thread asks for this; a straight thread never sees the step.
        charIndex += printSetupStep();
        if (numpadResult != 0) {
          charIndex += lcd.print("Taper ");
          charIndex += lcd.print(numpadToConeRatio(), 5);
          charIndex += lcd.print("?");
        } else {
          charIndex += lcd.print("Taper ");
          charIndex += printNoTrailing0(coneRatio);
          charIndex += lcd.print("?");
        }
      } else if (!isOn && setupIndex == getLastSetupIndex()) {
        charIndex += printSetupStep();
        long zOffset = getPassModeZStart() - z.pos;
        long xOffset = getPassModeXStart() - x.pos;
        charIndex += lcd.print("Go");
        // The offsets say the tool is about to move before it does, which is what the
        // confirmation is for. But an offset can be ten characters with its sign and unit, and
        // two of them plus the step counter overrun the line - which wraps onto the position
        // line below and corrupts it. Print each only while there is room, and keep the "?".
        if (zOffset != 0 && charIndex + 10 <= 19) {
          charIndex += lcd.print(" ");
          charIndex += lcd.print(z.name);
          charIndex += printDeciMicrons(stepsToDu(&z, zOffset), 2);
        }
        if (xOffset != 0 && charIndex + 10 <= 19) {
          charIndex += lcd.print(" ");
          charIndex += lcd.print(x.name);
          charIndex += printDeciMicrons(stepsToDu(&x, xOffset), 2);
        }
        charIndex += lcd.print("?");
      } else if (isOn && numpadResult == 0) {
        long springTotal = (mode == MODE_TURN || mode == MODE_FACE || isThreadMode()) ? springPasses * starts : 0;
        long total = max(opIndex, long(turnPasses * starts) + springTotal);
        charIndex += lcd.print(opIndex > turnPasses * starts ? "Spring " : "Pass ");
        charIndex += lcd.print(opIndex);
        charIndex += lcd.print(" of ");
        charIndex += lcd.print(total);
        // Completed passes as a progress bar in the remaining space.
        charIndex += lcd.print(" ");
        long barCols = 20 - charIndex;
        long filled = total > 0 ? min(barCols, barCols * (opIndex - 1) / total) : 0;
        for (long i = 0; i < filled; i++) {
          charIndex += lcd.write(byte(255));
        }
      } else if (!isOn && !inNumpad && setupIndex == 0) {
        // The way in. With the stops set and nothing running there was nothing here at all, so
        // the wizard behind the ON key was invisible - you had to already know it existed. This
        // takes the line ahead of the angle and rpm readouts on purpose: in a pass mode, about to
        // start, the next keypress matters more than the tacho.
        //
        // Not while the numpad is open: the line below echoes what is being typed, and that
        // matters more than an instruction for something you have not asked for yet.
        charIndex += lcd.print("ON to set up ");
        charIndex += lcd.print(getLastSetupIndex());
        charIndex += lcd.print(" steps");
      }
    } else if (mode == MODE_CONE) {
      if (numpadResult != 0 && setupIndex == 1) {
        charIndex += printSetupStep();
        charIndex += lcd.print("Ratio ");
        charIndex += lcd.print(numpadToConeRatio(), 5);
        charIndex += lcd.print("?");
      } else if (!isOn && setupIndex == 1) {
        charIndex += printSetupStep();
        charIndex += lcd.print("Ratio ");
        charIndex += printNoTrailing0(coneRatio);
        charIndex += lcd.print("?");
      } else if (!isOn && setupIndex == 2) {
        charIndex += printSetupStep();
        charIndex += lcd.print(auxForward ? "External" : "Internal");
        charIndex += lcd.print(" <>");
      } else if (!isOn && setupIndex == 3) {
        charIndex += printSetupStep();
        charIndex += lcd.print("Go?");
      } else if (isOn && numpadResult == 0) {
        charIndex += lcd.print("Cone ratio ");
        charIndex += printNoTrailing0(coneRatio);
      } else if (!isOn && !inNumpad && setupIndex == 0) {
        charIndex += lcd.print("ON to set up ");
        charIndex += lcd.print(getLastSetupIndex());
        charIndex += lcd.print(" steps");
      }
    }

    if (charIndex == 0 && inNumpad) { // Also show for 0 input to allow setting limits to 0.
      charIndex += lcd.print("Use ");
      charIndex += printDupr(numpadToDeciMicrons());
      charIndex += lcd.print("?");
    }

    if (charIndex > 0) {
      // No space for shared RPM/angle text.
    } else if (joystickUse && joystickPinActive[4]) {
      // The joystick move button is held - show what the pendant is driving.
      charIndex += lcd.print("Jog");
      bool anyDir = false;
      for (int i = 0; i < 4; i++) {
        if (!joystickDirPressed[i]) continue;
        anyDir = true;
        int key = JOYSTICK_DIR_PIN_KEYS[i][1];
        charIndex += lcd.print(key == B_LEFT ? " Z<" : key == B_RIGHT ? " Z>" : key == B_UP ? " X^" : " Xv");
      }
      if (!anyDir) {
        charIndex += lcd.print(" ready");
      }
    } else if (xRetracted) {
      charIndex += lcd.print("X retracted");
    } else if (indexDivisions > 1) {
      // Indexing takes the line ahead of the plain angle: it is the one you act on while your
      // hand is on the chuck, and the angle it replaces is still the same number underneath.
      IndexPosition ip = indexNearest(spindleModulo(spindlePos), ENCODER_STEPS_INT, indexDivisions);
      long tol = indexToleranceCounts(ENCODER_STEPS_INT, indexToleranceTenths);
      bool onMark = indexOnMark(ip.errCounts, tol);
      charIndex += lcd.print("Idx ");
      charIndex += lcd.print(ip.index + 1); // 1-based: the marks are counted, not addressed
      charIndex += lcd.print("/");
      charIndex += lcd.print(indexDivisions);
      if (onMark) {
        charIndex += lcd.print(" ON");
      } else {
        charIndex += lcd.print(" ");
        charIndex += lcd.print(indexErrorDeg(ip.errCounts, ENCODER_STEPS_INT), 1);
        charIndex += lcd.print(char(223));
      }
      // Sound once per arrival rather than for every refresh spent sitting on the mark, and rearm
      // only after leaving it - otherwise a chuck wobbling on the tolerance edge chatters.
      if (onMark && indexLastOnMark != ip.index) {
        beepFor(BEEP_INDEX);
        indexLastOnMark = ip.index;
      } else if (!onMark) {
        indexLastOnMark = -1;
      }
    } else if (showAngle) {
      charIndex += lcd.print("Angle ");
      charIndex += lcd.print(spindleModulo(spindlePos) * 360 / ENCODER_STEPS_FLOAT, 2);
      charIndex += lcd.print(char(223));
    } else if (showTacho) {
      charIndex += lcd.print(rpm);
      charIndex += lcd.print("rpm");
      // Diameter of the work at the tool. Only right if X was zeroed on the lathe centerline.
      long diameterDu = abs(getAxisPosDu(&x)) * 2;
      long wanted = cssActiveSpeed();
      if (wanted > 0) {
        // Constant surface speed: what the spindle should be doing at this diameter, and how far
        // off it is. The controller has no way to set it - see indexing.h - so it says, you dial.
        long target = cssTargetRpm(diameterDu, wanted);
        long capped = cssCappedRpm(target, spindleMaxRpm);
        charIndex += lcd.print(capped < target ? " ^" : " >"); // ^ means the lathe cannot go faster
        charIndex += lcd.print(capped);
        int pct = cssDeviationPercent(capped, rpm);
        charIndex += lcd.print(pct >= 0 ? " +" : " ");
        charIndex += lcd.print(pct);
        charIndex += lcd.print("%");
      } else {
        // Unrounded metres per minute, converted at the last moment: rounding first and scaling
        // afterwards shifts the imperial figure by up to two feet per minute.
        double speed = cssActualSpeed(diameterDu, rpm);
        bool inchMode = measure != MEASURE_METRIC;
        charIndex += lcd.print(" ");
        charIndex += lcd.print(inchMode ? speed * CSS_FEET_PER_METRE : speed, 0);
        charIndex += lcd.print(cssUnitName(inchMode));
      }
    }
    printLcdSpaces(charIndex);
  }
}

unsigned long pulse1HighMicros = 0;
unsigned long pulse2HighMicros = 0;

// Called on a FALLING interrupt for the first axis rotary encoder pin.
void IRAM_ATTR pulse1Enc() {
  unsigned long now = micros();
  if (DREAD(A12)) {
    pulse1HighMicros = now;
  } else if (now > pulse1HighMicros + pulseMinWidthUs) {
    pulse1Delta += (DREAD(A13) ? -1 : 1) * (pulse1Invert ? -1 : 1);
  }
}

// Called on a FALLING interrupt for the second axis rotary encoder pin.
void IRAM_ATTR pulse2Enc() {
  unsigned long now = micros();
  if (DREAD(A22)) {
    pulse2HighMicros = now;
  } else if (now > pulse2HighMicros + pulseMinWidthUs) {
    pulse2Delta += (DREAD(A23) ? -1 : 1) * (pulse2Invert ? -1 : 1);
  }
}

void setAsyncTimerEnable(bool value) {
  if (value) {
    timerStart(async_timer);
  } else {
    timerStop(async_timer);
  }
}

void taskDisplay(void *param) {
  unsigned long lastLcdMs = 0;
  while (emergencyStop == ESTOP_NONE) {
    // Rate-limited rather than free-running: see LCD_MIN_UPDATE_MS. The per-line hashes already
    // skip unchanged content, but without this a value that changes every loop gets rewritten
    // faster than it can be read or the display can settle.
    unsigned long nowMs = millis();
    if (nowMs - lastLcdMs >= LCD_MIN_UPDATE_MS) {
      lastLcdMs = nowMs;
      updateDisplay();
    }
    // Calling Preferences.commit() blocks all interrupts for 30ms, don't call saveIfChanged() if
    // encoder is likely to move soon.
    unsigned long now = micros();
    if (!stepperIsRunning(&z) && !stepperIsRunning(&x) && (now > spindleEncTime + SAVE_DELAY_US) && (now < saveTime || now > saveTime + SAVE_DELAY_US) && (now < keypadTimeUs || now > keypadTimeUs + SAVE_DELAY_US)) {
      if (saveIfChanged()) {
        saveTime = now;
      }
    }
    // The buzzer lives here because playing a multi-step pattern means coming back to it over and
    // over, which the motion loop on core 1 cannot afford to do.
    if (beepRequest != BEEP_NONE) {
      beeperStart(&beeper, beepRequest, nowMs);
      beepRequest = BEEP_NONE;
    }
    int beepFreq = 0;
    if (beeperTick(&beeper, nowMs, &beepFreq)) {
      if (beepFreq > 0) {
        tone(BUZZ, beepFreq);
      } else {
        noTone(BUZZ);
      }
    }
    if (abs(z.pendingPos) > z.estopSteps || abs(x.pendingPos) > x.estopSteps) {
      setEmergencyStop(ESTOP_POS);
    }
    taskYIELD();
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("EMERGENCY STOP");
  lcd.setCursor(0, 1);
  if (emergencyStop == ESTOP_KEY) {
    lcd.print("Key down at power-up");
    lcd.setCursor(0, 2);
    lcd.print("Hardware failure?");
  } else if (emergencyStop == ESTOP_POS) {
    lcd.print("Requested position");
    lcd.setCursor(0, 2);
    lcd.print("outside machine");
  } else if (emergencyStop == ESTOP_MARK_ORIGIN) {
    lcd.print("Unable to");
    lcd.setCursor(0, 2);
    lcd.print("mark origin");
  } else if (emergencyStop == ESTOP_ON_OFF) {
    lcd.print("Unable to");
    lcd.setCursor(0, 2);
    lcd.print("turn on/off");
  } else if (emergencyStop == ESTOP_OFF_MANUAL_MOVE) {
    lcd.print("Off during");
    lcd.setCursor(0, 2);
    lcd.print("manual move");
  }
  vTaskDelete(NULL);
}

void taskKeypad(void *param) {
  while (emergencyStop == ESTOP_NONE) {
    processKeypadEvent();
    taskYIELD();
  }
  vTaskDelete(NULL);
}

void waitForPendingPosNear0(Axis* a) {
  while (abs(a->pendingPos) > a->motorSteps / 3) {
    taskYIELD();
  }
}

void waitForPendingPos0(Axis* a) {
  while (a->pendingPos != 0) {
    taskYIELD();
  }
}

bool isContinuousStep() {
  return moveStep == (measure == MEASURE_METRIC ? MOVE_STEP_1 : MOVE_STEP_IMP_1);
}

// For rotational axis the moveStep of 0.1" means 0.1°.
long getMoveStepForAxis(Axis* a) {
  return (a->rotational && measure != MEASURE_METRIC) ? (moveStep / 25.4) : moveStep;
}

long getStepMaxSpeed(Axis* a) {
  return isContinuousStep() ? a->speedManualMove : min(long(a->speedManualMove), abs(getMoveStepForAxis(a)) * 1000 / stepTimeMs);
}

void waitForStep(Axis* a) {
  if (isContinuousStep()) {
    // Move continuously for default step.
    waitForPendingPosNear0(a);
  } else {
    // Move with tiny pauses allowing to stop precisely.
    a->continuous = false;
    waitForPendingPos0(a);
    DELAY(delayBetweenStepsMs);
  }
}

int getAndResetPulses(Axis* a) {
  int delta = 0;
  if ((pulse1DrivesX ? NAME_X : NAME_Z) == a->name) {
    if (pulse1Delta < -pulseHalfBacklash) {
      noInterrupts();
      delta = pulse1Delta + pulseHalfBacklash;
      pulse1Delta = -pulseHalfBacklash;
      interrupts();
    } else if (pulse1Delta > pulseHalfBacklash) {
      noInterrupts();
      delta = pulse1Delta - pulseHalfBacklash;
      pulse1Delta = pulseHalfBacklash;
      interrupts();
    }
  } else if ((pulse2DrivesX ? NAME_X : NAME_Z) == a->name) {
    if (pulse2Delta < -pulseHalfBacklash) {
      noInterrupts();
      delta = pulse2Delta + pulseHalfBacklash;
      pulse2Delta = -pulseHalfBacklash;
      interrupts();
    } else if (pulse2Delta > pulseHalfBacklash) {
      noInterrupts();
      delta = pulse2Delta - pulseHalfBacklash;
      pulse2Delta = pulseHalfBacklash;
      interrupts();
    }
  }
  return delta;
}

void taskMoveZ(void *param) {
  while (emergencyStop == ESTOP_NONE) {
    int pulseDelta = getAndResetPulses(&z);
    bool left = buttonLeftPressed;
    bool right = buttonRightPressed;
    if (!left && !right && pulseDelta == 0) {
      taskYIELD();
      continue;
    }
    if (spindlePosSync != 0) {
      // Edge case.
      taskYIELD();
      continue;
    }
    if (isOn && !manualMovesAllowedWhenOn()) {
      setIsOnFromTask(false);
    }
    int sign = pulseDelta == 0 ? (left ? 1 : -1) : (pulseDelta > 0 ? 1 : -1);
    bool stepperOn = true;
    stepperEnable(&z, true);
    z.movingManually = true;
    if (isOn && dupr != 0 && mode == MODE_NORMAL) {
      // Move by moveStep in the desired direction but stay in the thread by possibly traveling a little more.
      int diff = ceil(moveStep * 1.0 / abs(dupr * starts)) * ENCODER_STEPS_FLOAT * sign * (dupr > 0 ? 1 : -1);
      long prevSpindlePos = spindlePos;
      bool resting = false;
      do {
        z.speedMax = z.speedManualMove;
        if (xSemaphoreTake(motionMutex, 100) == pdTRUE) {
          if (!resting) {
            spindlePos += diff;
            spindlePosAvg += diff;
          }
          // If spindle is moving, it will be changing spindlePos at the same time. Account for it.
          while (diff > 0 ? (spindlePos < prevSpindlePos) : (spindlePos > prevSpindlePos)) {
            spindlePos += diff;
            spindlePosAvg += diff;
          };
          prevSpindlePos = spindlePos;
          xSemaphoreGive(motionMutex);
        }

        long newPos = posFromSpindle(&z, prevSpindlePos, true);
        if (newPos != z.pos) {
          stepToContinuous(&z, newPos);
          waitForPendingPosNear0(&z);
        } else if (z.pos == (left ? z.leftStop : z.rightStop)) {
          // We're standing on a stop with the L/R move button pressed.
          resting = true;
          if (stepperOn) {
            stepperEnable(&z, false);
            stepperOn = false;
          }
          DELAY(200);
        }
      } while (left ? buttonLeftPressed : buttonRightPressed);
    } else {
      z.speedMax = getStepMaxSpeed(&z);
      int delta = 0;
      do {
        float fractionalDelta = (pulseDelta == 0 ? moveStep * sign / z.screwPitch : pulseDelta / pulsePerRevolution) * z.motorSteps + z.fractionalPos;
        delta = round(fractionalDelta);
        // Don't lose fractional steps when moving by 0.01" or 0.001".
        z.fractionalPos = fractionalDelta - delta;
        if (delta == 0) {
          // When moveStep is e.g. 1 micron and MOTOR_STEPS_Z is 200, make delta non-zero.
          delta = sign;
        }

        long posCopy = z.pos + z.pendingPos;
        // Don't left-right move out of stops.
        if (posCopy + delta > z.leftStop) {
          delta = z.leftStop - posCopy;
        } else if (posCopy + delta < z.rightStop) {
          delta = z.rightStop - posCopy;
        }
        z.speedMax = getStepMaxSpeed(&z);
        stepToContinuous(&z, posCopy + delta);
        waitForStep(&z);
      } while (delta != 0 && (left ? buttonLeftPressed : buttonRightPressed));
      z.continuous = false;
      waitForPendingPos0(&z);
      if (isOn && mode == MODE_CONE) {
        if (xSemaphoreTake(motionMutex, 100) != pdTRUE) {
          setEmergencyStop(ESTOP_MARK_ORIGIN);
        } else {
          markOrigin();
          xSemaphoreGive(motionMutex);
        }
      } else if (isOn && mode == MODE_ASYNC) {
        // Restore async direction.
        updateAsyncTimerSettings();
      }
    }
    z.movingManually = false;
    if (stepperOn) {
      stepperEnable(&z, false);
    }
    z.speedMax = LONG_MAX;
    taskYIELD();
  }
  vTaskDelete(NULL);
}

void taskMoveX(void *param) {
  while (emergencyStop == ESTOP_NONE) {
    int pulseDelta = getAndResetPulses(&x);
    bool up = buttonUpPressed || pulseDelta > 0;
    bool down = buttonDownPressed || pulseDelta < 0;
    if (!up && !down) {
      taskYIELD();
      continue;
    }
    if (isOn && !manualMovesAllowedWhenOn()) {
      setIsOnFromTask(false);
    }
    x.movingManually = true;
    x.speedMax = getStepMaxSpeed(&x);
    stepperEnable(&x, true);

    int delta = 0;
    int sign = up ? 1 : -1;
    do {
      float fractionalDelta = (pulseDelta == 0 ? moveStep * sign / x.screwPitch : pulseDelta / pulsePerRevolution) * x.motorSteps + x.fractionalPos;
      delta = round(fractionalDelta);
      // Don't lose fractional steps when moving by 0.01" or 0.001".
      x.fractionalPos = fractionalDelta - delta;
      if (delta == 0) {
        // When moveStep is e.g. 1 micron and MOTOR_STEPS_Z is 200, make delta non-zero.
        delta = sign;
      }

      long posCopy = x.pos + x.pendingPos;
      if (posCopy + delta > x.leftStop) {
        delta = x.leftStop - posCopy;
      } else if (posCopy + delta < x.rightStop) {
        delta = x.rightStop - posCopy;
      }
      stepToContinuous(&x, posCopy + delta);
      waitForStep(&x);
      pulseDelta = getAndResetPulses(&x);
    } while (delta != 0 && (pulseDelta != 0 || (up ? buttonUpPressed : buttonDownPressed)));
    x.continuous = false;
    waitForPendingPos0(&x);
    if (isOn && mode == MODE_CONE) {
      if (xSemaphoreTake(motionMutex, 100) != pdTRUE) {
        setEmergencyStop(ESTOP_MARK_ORIGIN);
      } else {
        markOrigin();
        xSemaphoreGive(motionMutex);
      }
    }
    x.movingManually = false;
    x.speedMax = LONG_MAX;
    stepperEnable(&x, false);

    taskYIELD();
  }
  vTaskDelete(NULL);
}

void taskMoveA1(void *param) {
  while (emergencyStop == ESTOP_NONE) {
    bool plus = buttonTurnPressed;
    bool minus = buttonGearsPressed;
    if (mode != MODE_A1 || (!plus && !minus)) {
      taskYIELD();
      continue;
    }
    a1.movingManually = true;
    a1.speedMax = getStepMaxSpeed(&a1);
    stepperEnable(&a1, true);

    int delta = 0;
    int sign = plus ? 1 : -1;
    do {
      float fractionalDelta = getMoveStepForAxis(&a1) * sign / a1.screwPitch * a1.motorSteps + a1.fractionalPos;
      delta = round(fractionalDelta);
      a1.fractionalPos = fractionalDelta - delta;
      if (delta == 0) delta = sign;

      long posCopy = a1.pos + a1.pendingPos;
      if (posCopy + delta > a1.leftStop) {
        delta = a1.leftStop - posCopy;
      } else if (posCopy + delta < a1.rightStop) {
        delta = a1.rightStop - posCopy;
      }
      stepToContinuous(&a1, posCopy + delta);
      waitForStep(&a1);
    } while (plus ? buttonTurnPressed : buttonGearsPressed);
    a1.continuous = false;
    waitForPendingPos0(&a1);
    // Restore async direction.
    if (isOn && mode == MODE_A1) updateAsyncTimerSettings();
    a1.movingManually = false;
    a1.speedMax = LONG_MAX;
    stepperEnable(&a1, false);
    taskYIELD();
  }
  vTaskDelete(NULL);
}

void taskGcode(void *param) {
  while (emergencyStop == ESTOP_NONE) {
    if (mode != MODE_GCODE) {
      gcodeInitialized = false;
    } else if (!gcodeInitialized) {
      gcodeInitialized = true;
      gcodeCommand = "";
      gcodeAbsolutePositioning = true;
      gcodeFeedDuPerSec = GCODE_FEED_DEFAULT_DU_SEC;
      gcodeInBrace = false;
      gcodeInSemicolon = false;
    }
    // Implementing a relevant subset of RS274 (Gcode) and GRBL (state management) covering basic use cases.
    char receivedChar = '\0';
    bool isSerial = false;
    if (mode == MODE_GCODE && isOn && gcodeProgramCharIndex < gcodeProgram.length()) {
      receivedChar = gcodeProgram.charAt(gcodeProgramCharIndex);
      gcodeProgramCharIndex++;
    } else if (Serial.available() > 0) {
      isSerial = true;
      receivedChar = Serial.read();
    }
    int charCode = int(receivedChar);
    if (charCode > 0) {
      if (gcodeInBrace) {
        if (receivedChar == ')') gcodeInBrace = false;
      } else if (serialInSetting) {
        // "$" then an optional key=value, up to the end of the line.
        if (charCode < 32) {
          serialInSetting = false;
          applySettingCommand(settingCommand);
          settingCommand = "";
        } else {
          settingCommand += receivedChar;
        }
      } else if (serialInKeycode) {
        if (charCode < 32) {
          if (serialKeycode == 0) {
            serialKeycode = keycodeCommand.toInt();
            Serial.println(serialKeycode);
          } else {
            Serial.println("slower");
          }
          serialInKeycode = false;
          keycodeCommand = "";
        } else {
          keycodeCommand += receivedChar;
        }
      } else if (receivedChar == '(') {
        gcodeInBrace = true;
      } else if (receivedChar == ';' /* start of comment till end of line */) {
        gcodeInSemicolon = true;
      } else if (gcodeInSemicolon && charCode >= 32) {
        // Ignoring comment.
      } else if (receivedChar == '!' /* stop */) {
        setIsOnFromTask(false);
      } else if (receivedChar == '~' /* resume */) {
        setIsOnFromTask(true);
      } else if (receivedChar == '$' /* dump settings, or set one with $key=value */) {
        serialInSetting = true;
        settingCommand = "";
      } else if (receivedChar == '%' /* start/end marker */) {
        // Not using % markers in this implementation.
      } else if (receivedChar == '?' /* status */) {
        Serial.print("<");
        Serial.print(isOn ? "Run" : "Idle");
        Serial.print("|WPos:");
        float divisor = measure == MEASURE_METRIC ? 10000.0 : 254000.0;
        Serial.print(getAxisPosDu(&x) / divisor, 3);
        Serial.print(",0.000,");
        Serial.print(getAxisPosDu(&z) / divisor, 3);
        Serial.print("|FS:");
        Serial.print(round(gcodeFeedDuPerSec * 60 / 10000.0));
        Serial.print(",");
        Serial.print(getApproxRpm());
        Serial.print("|Id:");
        Serial.print("H" + String(HARDWARE_VERSION) + "V" + String(SOFTWARE_VERSION));
        Serial.print(">"); // no new line to allow client to easily cut out the status response
      } else if (gcodeInSave && receivedChar == '"' /* end of saved program */) {
        gcodeInSave = false;
        if (gcodeSaveName.length() == 0) {
          if (removeAllGcode()) Serial.println("ok");
        } else if (gcodeSaveValue.length() > 1) {
          if (saveGcode()) Serial.println("ok");
        } else if (gcodeSaveName.length() == 1) {
          Serial.println("error: name must be at least 2 chars");
        } else {
          Preferences pref;
          pref.begin(GCODE_NAMESPACE);
          bool found = false;
          for (int i = 0; i < 256; i++) {
            if (!pref.isKey(String(i).c_str())) break;
            if (gcodeSaveName.equals(pref.getString(String(i).c_str()))) {
              found = true;
              if (removeGcode(i)) Serial.println("ok");
              break;
            }
          }
          if (!found) Serial.println("error: name not found");
          pref.end();
        }
        gcodeSaveName = "";
        gcodeSaveValue = "";
      } else if (!gcodeInSave && receivedChar == '"' /* start of save program */) {
        gcodeInSave = true;
        gcodeInSaveFirstLine = true;
      } else if (gcodeInSaveFirstLine && receivedChar >= 32) {
        gcodeSaveName += receivedChar;
      } else if (gcodeInSaveFirstLine && receivedChar < 32) {
        gcodeInSaveFirstLine = false;
        Serial.println("ok");
      } else if (gcodeInSave) {
        gcodeSaveValue += receivedChar;
        if (receivedChar < 32) {
          gcodeInBrace = false;
          gcodeInSemicolon = false;
          Serial.println("ok");
        }
      } else if (isOn) {
        if (gcodeInBrace && charCode < 32) {
          Serial.println("error: comment not closed");
          setIsOnFromTask(false);
        } else if (charCode < 32 && gcodeCommand.length() > 1) {
          if (handleGcodeCommand(gcodeCommand) && isSerial) Serial.println("ok");
          gcodeCommand = "";
          gcodeInSemicolon = false;
        } else if (charCode < 32) {
          if (isSerial) Serial.println("ok");
          gcodeCommand = "";
        } else if (charCode >= 32 && (charCode == 'G' || charCode == 'M')) {
          // Split consequent G and M commands on one line.
          // No "ok" for commands in the middle of the line.
          handleGcodeCommand(gcodeCommand);
          gcodeCommand = receivedChar;
        } else if (charCode >= 32) {
          gcodeCommand += receivedChar;
        }
      } else if (receivedChar == '=' /* start of keycode command */) {
        serialInKeycode = true;
        keycodeCommand = "";
      } else {
        // ignoring non-realtime command input when off
        // to flush any commands coming after an error
      }
    }
    if (mode == MODE_GCODE && isOn && gcodeProgramCharIndex > 0 && gcodeProgramCharIndex == gcodeProgram.length()) {
      setIsOnFromTask(false);
    }
    taskYIELD();
  }
  vTaskDelete(NULL);
}

bool saveGcode() {
  Preferences pref;
  pref.begin(GCODE_NAMESPACE);
  bool success = false;
  if (gcodeSaveName.length() < 2) {
    Serial.println("error: name must be at least 2 chars");
  } else if (gcodeSaveValue.length() < 2) {
    Serial.println("error: program too short");
  } else if (pref.freeEntries() < 2 || gcodeProgramCount >= 256) {
    Serial.println("error: memory full");
  } else if (pref.isKey(gcodeSaveName.c_str())) {
    if (pref.putString(gcodeSaveName.c_str(), gcodeSaveValue) != gcodeSaveValue.length()) {
      Serial.println("error: failed to overwrite");
    } else {
      success = true;
    }
  } else {
    if (pref.putString(String(gcodeProgramCount).c_str(), gcodeSaveName) == 0) {
      Serial.println("error: not enough memory for program name");
    } else if (pref.putString(gcodeSaveName.c_str(), gcodeSaveValue) != gcodeSaveValue.length()) {
      pref.remove(String(gcodeProgramCount).c_str());
      Serial.println("error: not enough memory for program text");
    } else {
      gcodeProgramCount++;
      success = true;
    }
  }
  pref.end();
  return success;
}

bool removeGcode(int indexToRemove) {
  Preferences pref;
  pref.begin(GCODE_NAMESPACE);
  bool success = false;
  if (indexToRemove >= 0 && indexToRemove < 256 && pref.isKey(String(indexToRemove).c_str())) {
    success = true;
    String programName = pref.getString(String(indexToRemove).c_str());
    pref.remove(String(indexToRemove).c_str());
    if (programName.length() > 0) {
      pref.remove(programName.c_str());
    }
    // Move all the following program names down to avoid holes.
    for (int i = indexToRemove + 1; pref.isKey(String(i).c_str()); i++) {
      pref.putString(String(i - 1).c_str(), pref.getString(String(i).c_str()));
      pref.remove(String(i).c_str());
    }
    if (gcodeProgramCount > 0) gcodeProgramCount--;
    if (gcodeProgramCount > 0 && gcodeProgramIndex >= gcodeProgramCount) {
      gcodeProgramIndex = gcodeProgramCount - 1;
    }
  } else {
    Serial.print("error: program to delete not found at index ");
    Serial.println(indexToRemove);
  }
  pref.end();
  return success;
}

bool removeAllGcode() {
  Preferences pref;
  pref.begin(GCODE_NAMESPACE);
  bool success = pref.clear();
  if (!success) Serial.println("error: failed clearing GCODE_NAMESPACE");
  pref.end();
  return true;
}

// Full 4x quadrature decode, using both channels of the PCNT unit.
//
// This replaces a single-channel 2x decode that counted only channel A's edges and took the
// direction from channel B's *level* at that instant. Direction sampled from a level rather
// than derived from the state sequence has a real failure mode: an A edge arriving while B sits
// near its threshold can be counted either way, so a shaft parked in the transition region
// walks the count in one direction instead of dithering harmlessly around it. That is exactly
// where hand-turning leaves the spindle - moving slowly, in the transition region, with
// vibration - which is why the carriage twitched by hand but threaded cleanly under power.
//
// Counting both channels' edges means every transition is accounted for, so an excursion that
// returns to the same state nets to zero no matter which order the edges arrived in.
void startPulseCounter(pcnt_unit_t unit, int gpioA, int gpioB) {
  // Channel 0 counts A's edges, using B to tell which way the shaft turned.
  pcnt_config_t chA;
  chA.pulse_gpio_num = gpioA;
  chA.ctrl_gpio_num = gpioB;
  chA.channel = PCNT_CHANNEL_0;
  chA.unit = unit;
  chA.pos_mode = PCNT_COUNT_DEC;
  chA.neg_mode = PCNT_COUNT_INC;
  chA.lctrl_mode = PCNT_MODE_REVERSE;
  chA.hctrl_mode = PCNT_MODE_KEEP;
  chA.counter_h_lim = PCNT_LIM;
  chA.counter_l_lim = -PCNT_LIM;
  pcnt_unit_config(&chA);

  // Channel 1 does the mirror image: B's edges, direction from A. Together the two cover all
  // four transitions of the quadrature cycle.
  pcnt_config_t chB;
  chB.pulse_gpio_num = gpioB;
  chB.ctrl_gpio_num = gpioA;
  chB.channel = PCNT_CHANNEL_1;
  chB.unit = unit;
  chB.pos_mode = PCNT_COUNT_INC;
  chB.neg_mode = PCNT_COUNT_DEC;
  chB.lctrl_mode = PCNT_MODE_REVERSE;
  chB.hctrl_mode = PCNT_MODE_KEEP;
  chB.counter_h_lim = PCNT_LIM;
  chB.counter_l_lim = -PCNT_LIM;
  pcnt_unit_config(&chB);

  // The filter applies to the unit's inputs, so its RPM ceiling is unchanged by the decode mode.
  pcnt_set_filter_value(unit, encoderFilter);
  pcnt_filter_enable(unit);
  pcnt_counter_pause(unit);
  pcnt_counter_clear(unit);
  pcnt_counter_resume(unit);
}

// Attaching interrupt on core 0 to have more time on core 1 where axes are moved.
void taskAttachInterrupts(void *param) {
  encHealthReset(&encHealth, micros());
  startPulseCounter(PCNT_UNIT_0, ENC_A, ENC_B);
  // The handwheel interrupts are attached by applyAuxPins(), which owns the aux terminals and
  // runs again whenever a device is enabled or disabled.
  vTaskDelete(NULL);
}

void setEmergencyStop(int kind) {
  emergencyStop = kind;
  setAsyncTimerEnable(false);
  xSemaphoreTake(z.mutex, 10);
  xSemaphoreTake(x.mutex, 10);
  xSemaphoreTake(a1.mutex, 10);
}

// Configures the six auxiliary terminals from the current device flags, and is the only code that
// touches them. Called at startup and again whenever one of the flags changes, so enabling a
// handwheel or the joystick takes effect immediately instead of at the next restart.
//
// It also fixes something that never worked: the startup pin setup used to run before the stored
// settings were read, so it always configured whatever machine_config.h said. A handwheel or
// joystick switched on from the settings menu was saved, survived the reboot, and still had no
// pins configured for it.
//
// Every pin is released to a plain input first. A device being switched off has to stop driving
// before another claims the same terminals, and the two passes mean the order the flags happen to
// be in cannot leave a pin configured by its previous owner. Callers must ensure the machine is
// idle: the A1 enable line floats for the moment between the two passes.
// aux_pins.h describes which device claims which terminal, and it has to be told rather than
// derived because the host tests build it without machine_config.h. These keep the two in step:
// remap the joystick onto different pins and the claim table stops describing reality, which
// would let a conflicting device through the check.
static_assert(JOYSTICK_DIR_PIN_KEYS[0][0] == A11 && JOYSTICK_DIR_PIN_KEYS[1][0] == A12 &&
    JOYSTICK_DIR_PIN_KEYS[2][0] == A13 && JOYSTICK_DIR_PIN_KEYS[3][0] == A21,
    "Joystick direction pins moved - update auxClaimMask() in aux_pins.h to match");
static_assert(JOYSTICK_MOVE_PIN == A22 && JOYSTICK_STEP_PIN == A23,
    "Joystick button pins moved - update auxClaimMask() in aux_pins.h to match");

void applyAuxPins() {
  detachInterrupt(digitalPinToInterrupt(A12));
  detachInterrupt(digitalPinToInterrupt(A22));
  pinMode(A11, INPUT);
  pinMode(A12, INPUT);
  pinMode(A13, INPUT);
  pinMode(A21, INPUT);
  pinMode(A22, INPUT);
  pinMode(A23, INPUT);

  if (a1.active) {
    pinMode(A12, OUTPUT);
    pinMode(A13, OUTPUT);
    pinMode(A11, OUTPUT);
    DHIGH(A13);
  }

  if (pulse1Use) {
    pinMode(A11, OUTPUT);
    pinMode(A12, INPUT);
    pinMode(A13, INPUT);
    DLOW(A11);
    // Anything counted while the pins were someone else's is not handwheel movement.
    pulse1Delta = 0;
    attachInterrupt(digitalPinToInterrupt(A12), pulse1Enc, CHANGE);
  }

  if (pulse2Use) {
    pinMode(A21, OUTPUT);
    pinMode(A22, INPUT);
    pinMode(A23, INPUT);
    DLOW(A21);
    pulse2Delta = 0;
    attachInterrupt(digitalPinToInterrupt(A22), pulse2Enc, CHANGE);
  }

  if (joystickUse) {
    for (int i = 0; i < 6; i++) {
      pinMode(joystickPin(i), INPUT_PULLUP);
    }
  } else {
    // Held directions have to be dropped, or an axis carries on moving after the stick that was
    // driving it has been switched off.
    for (int i = 0; i < 6; i++) {
      joystickPinActive[i] = false;
    }
    for (int i = 0; i < 4; i++) {
      joystickDirPressed[i] = false;
    }
  }
}

// Refuses to switch an auxiliary device on over terminals another one already holds. See
// aux_pins.h for the claim table.
const char* auxConflictMessage(int device) {
  int other = auxConflict(device, auxEnabledMask(a1.active, pulse1Use, pulse2Use, joystickUse));
  if (other < 0) {
    return NULL;
  }
  static char msg[24];
  snprintf(msg, sizeof(msg), "Used by %s", auxDeviceName(other));
  return msg;
}

void setup() {
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  pinMode(Z_DIR, OUTPUT);
  pinMode(Z_STEP, OUTPUT);
  pinMode(Z_ENA, OUTPUT);
  DHIGH(Z_STEP);

  pinMode(X_DIR, OUTPUT);
  pinMode(X_STEP, OUTPUT);
  pinMode(X_ENA, OUTPUT);
  DHIGH(X_STEP);

  pinMode(BUZZ, OUTPUT);
  beeperReset(&beeper);

  // The auxiliary terminals are configured further down, once the stored settings have been read
  // and the axes exist - applyAuxPins() needs both.

  Preferences pref;
  pref.begin(PREF_NAMESPACE);
  if (pref.getInt(PREF_VERSION) != PREFERENCES_VERSION) {
    pref.clear();
    pref.putInt(PREF_VERSION, PREFERENCES_VERSION);
  }

  initAxis(&z, NAME_Z, true, false, MOTOR_STEPS_Z, SCREW_Z_DU, SPEED_START_Z, SPEED_MANUAL_MOVE_Z, ACCELERATION_Z, INVERT_Z, NEEDS_REST_Z, MAX_TRAVEL_MM_Z, BACKLASH_DU_Z, Z_ENA, Z_DIR, Z_STEP);
  initAxis(&x, NAME_X, true, false, MOTOR_STEPS_X, SCREW_X_DU, SPEED_START_X, SPEED_MANUAL_MOVE_X, ACCELERATION_X, INVERT_X, NEEDS_REST_X, MAX_TRAVEL_MM_X, BACKLASH_DU_X, X_ENA, X_DIR, X_STEP);
  initAxis(&a1, NAME_A1, ACTIVE_A1, ROTARY_A1, MOTOR_STEPS_A1, SCREW_A1_DU, SPEED_START_A1, SPEED_MANUAL_MOVE_A1, ACCELERATION_A1, INVERT_A1, NEEDS_REST_A1, MAX_TRAVEL_MM_A1, BACKLASH_DU_A1, A11, A12, A13);
  z.motorTeeth = MOTOR_TEETH_Z; z.screwTeeth = SCREW_TEETH_Z; recomputeAxisDerived(&z);
  x.motorTeeth = MOTOR_TEETH_X; x.screwTeeth = SCREW_TEETH_X; recomputeAxisDerived(&x);
  a1.motorTeeth = MOTOR_TEETH_A1; a1.screwTeeth = SCREW_TEETH_A1; recomputeAxisDerived(&a1);

  // Hardware parameters changed in the settings menu override the compiled-in defaults.
  loadAxisSettings(&z, &pref);
  loadAxisSettings(&x, &pref);
  encoderPpr = pref.getLong(PREF_ENCODER_PPR, ENCODER_PPR);
  encoderSpindleTeeth = pref.getLong(PREF_ENCODER_SPINDLE_TEETH, ENCODER_SPINDLE_TEETH);
  encoderPulleyTeeth = pref.getLong(PREF_ENCODER_PULLEY_TEETH, ENCODER_PULLEY_TEETH);
  encoderDivider = pref.getLong(PREF_ENCODER_DIVIDER, ENCODER_DIVIDER);
  encoderBacklash = pref.getLong(PREF_ENCODER_BACKLASH, ENCODER_BACKLASH);
  encoderSymmetric = pref.getBool(PREF_ENCODER_SYMMETRIC, ENCODER_SYMMETRIC);
  encoderFilter = pref.getLong("eflt", ENCODER_FILTER);
  // Per-mode settings, read straight from the table so a mode or an item added there needs no
  // matching change here. Defaults are the old single global values, so a controller upgrading
  // from before this starts with every mode set the way the one global was.
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (!settingIsModeScoped(i) || settingIsAction(i)) continue;
    char key[16];
    settingKeyName(i, key);
    int sm = SETTINGS[i].mode;
    const char* k = SETTINGS[i].prefKey;
    if (!strcmp(k, "tps")) modeSet[sm].passes = pref.getLong(key, pref.getInt(PREF_TURN_PASSES, 3));
    else if (!strcmp(k, "spp")) modeSet[sm].springPasses = pref.getLong(key, pref.getLong("spp", 0));
    else if (!strcmp(k, "safe")) modeSet[sm].clearanceDu = pref.getLong(key, pref.getLong("safe", SAFE_DISTANCE_DU));
    else if (!strcmp(k, "pck")) modeSet[sm].peckDu = pref.getLong(key, pref.getLong("pck", 0));
    else if (!strcmp(k, "fli")) modeSet[sm].flankInfeed = pref.getBool(key, pref.getBool("fli", false));
    else if (!strcmp(k, "slt")) modeSet[sm].slotReductionDu = pref.getLong(key, pref.getLong("slt", 0));
  }
  // The taper has no table row - see ModeSettings - so it is read directly. Both modes fall back to
  // the old shared value, which is what they were both using before they were separated.
  {
    float shared = pref.getFloat(PREF_CONE_RATIO, 1);
    modeSet[SMODE_CONE].taper = pref.getFloat("ccr", shared);
    // A tapered thread wants a taper, and 1:16 is NPT and BSPT - what the mode was built for.
    // Cone keeps 1, which is the ratio it has always started at.
    modeSet[SMODE_TPR].taper = pref.getFloat("rcr", shared != 1 ? shared : 0.0625f);
  }
  modeScopedLoaded = true;
  pulse1Use = pref.getBool("p1u", PULSE_1_USE);
  pulse1Invert = pref.getBool("p1i", PULSE_1_INVERT);
  pulse2Use = pref.getBool("p2u", PULSE_2_USE);
  pulse2Invert = pref.getBool("p2i", PULSE_2_INVERT);
  pulsePerRevolution = pref.getLong("hppr", (long) PULSE_PER_REVOLUTION);
  pulseMinWidthUs = pref.getLong("pmw", PULSE_MIN_WIDTH_US);
  pulseHalfBacklash = pref.getLong("phb", PULSE_HALF_BACKLASH);
  joystickUse = pref.getBool("joy", JOYSTICK_USE);
  joystickDebounceMs = pref.getLong("jdb", (long) JOYSTICK_DEBOUNCE_MS);
  stepTimeMs = pref.getLong("stm", STEP_TIME_MS);
  delayBetweenStepsMs = pref.getLong("sdl", DELAY_BETWEEN_STEPS_MS);
  indexDivisions = pref.getLong("idiv", INDEX_DIVISIONS);
  indexToleranceTenths = pref.getLong("itol", INDEX_TOLERANCE_TENTHS_DEG);
  cssEnabled = pref.getBool("cson", CSS_ENABLED);
  cssMaterial = pref.getLong("cmat", CSS_MATERIAL);
  cssCarbide = pref.getBool("ctol", CSS_CARBIDE);
  surfaceSpeedMPerMin = pref.getLong("css", SURFACE_SPEED_M_PER_MIN);
  spindleMaxRpm = pref.getLong("smax", SPINDLE_MAX_RPM);
  pulse1DrivesX = pref.getBool("p1a", PULSE_1_AXIS == NAME_X);
  pulse2DrivesX = pref.getBool("p2a", PULSE_2_AXIS == NAME_X);
  applyEncoderPpr();
  springPasses = pref.getLong(PREF_SPRING_PASSES, springPasses);
  peckDepthDu = pref.getLong(PREF_PECK_DEPTH, peckDepthDu);
  flankInfeed = pref.getBool(PREF_FLANK_INFEED, flankInfeed);
  retractDu = pref.getLong(PREF_RETRACT_DIST, retractDu);
  slotLeftReductionDu = pref.getLong("slt", slotLeftReductionDu);
  xDiameterDisplay = pref.getBool(PREF_X_DIAMETER, xDiameterDisplay);
  encoderInvert = pref.getBool(PREF_ENCODER_INVERT, encoderInvert);
  wifiEnabled = pref.getBool("wfen", WIFI_ENABLED);
  wifiPin = pref.getLong("wfpw", WIFI_PIN_DEFAULT);

  // Now that both the axes and the stored device flags exist, wire up the aux terminals. A stored
  // setup that predates the conflict check can have two devices claiming the same pins, so drop
  // the losers rather than configuring the terminals twice and letting the last one win silently.
  // Only the in-memory flags change - nothing is written back, so correcting the setting that
  // caused it restores the device rather than leaving a stored value quietly rewritten.
  int auxGranted = auxResolveConflicts(auxEnabledMask(a1.active, pulse1Use, pulse2Use, joystickUse));
  a1.active = (auxGranted & (1 << AUX_A1_AXIS)) != 0;
  pulse1Use = (auxGranted & (1 << AUX_HANDWHEEL_1)) != 0;
  pulse2Use = (auxGranted & (1 << AUX_HANDWHEEL_2)) != 0;
  joystickUse = (auxGranted & (1 << AUX_JOYSTICK)) != 0;
  applyAuxPins();

  isOn = false;
  savedDupr = dupr = pref.getLong(PREF_DUPR);
  motionMutex = xSemaphoreCreateMutex();
  savedStarts = starts = min(STARTS_MAX, max(static_cast<int32_t>(1), pref.getInt(PREF_STARTS)));
  z.savedPos = z.pos = pref.getLong(PREF_POS_Z);
  z.savedPosGlobal = z.posGlobal = pref.getLong(PREF_POS_GLOBAL_Z);
  z.savedOriginPos = z.originPos = pref.getLong(PREF_ORIGIN_POS_Z);
  z.savedMotorPos = z.motorPos = pref.getLong(PREF_MOTOR_POS_Z);
  z.savedLeftStop = z.leftStop = pref.getLong(PREF_LEFT_STOP_Z, LONG_MAX);
  z.savedRightStop = z.rightStop = pref.getLong(PREF_RIGHT_STOP_Z, LONG_MIN);
  z.savedDisabled = z.disabled = pref.getBool(PREF_DISABLED_Z, false);
  x.savedPos = x.pos = pref.getLong(PREF_POS_X);
  x.savedPosGlobal = x.posGlobal = pref.getLong(PREF_POS_GLOBAL_X);
  x.savedOriginPos = x.originPos = pref.getLong(PREF_ORIGIN_POS_X);
  x.savedMotorPos = x.motorPos = pref.getLong(PREF_MOTOR_POS_X);
  x.savedLeftStop = x.leftStop = pref.getLong(PREF_LEFT_STOP_X, LONG_MAX);
  x.savedRightStop = x.rightStop = pref.getLong(PREF_RIGHT_STOP_X, LONG_MIN);
  x.savedDisabled = x.disabled = pref.getBool(PREF_DISABLED_X, false);
  a1.savedPos = a1.pos = pref.getLong(PREF_POS_A1);
  a1.savedPosGlobal = a1.posGlobal = pref.getLong(PREF_POS_GLOBAL_A1);
  a1.savedOriginPos = a1.originPos = pref.getLong(PREF_ORIGIN_POS_A1);
  a1.savedMotorPos = a1.motorPos = pref.getLong(PREF_MOTOR_POS_A1);
  a1.savedLeftStop = a1.leftStop = pref.getLong(PREF_LEFT_STOP_A1, LONG_MAX);
  a1.savedRightStop = a1.rightStop = pref.getLong(PREF_RIGHT_STOP_A1, LONG_MIN);
  a1.savedDisabled = a1.disabled = pref.getBool(PREF_DISABLED_A1, false);
  savedSpindlePos = spindlePos = pref.getLong(PREF_SPINDLE_POS);
  savedSpindlePosAvg = spindlePosAvg = pref.getLong(PREF_SPINDLE_POS_AVG);
  savedSpindlePosSync = spindlePosSync = pref.getInt(PREF_OUT_OF_SYNC);
  savedSpindlePosGlobal = spindlePosGlobal = pref.getLong(PREF_SPINDLE_POS_GLOBAL);
  savedShowAngle = showAngle = pref.getBool(PREF_SHOW_ANGLE);
  savedShowTacho = showTacho = pref.getBool(PREF_SHOW_TACHO);
  savedShowBigDro = showBigDro = pref.getBool(PREF_SHOW_BDRO);
  savedMoveStep = moveStep = pref.getLong(PREF_MOVE_STEP, MOVE_STEP_1);
  setModeFromLoop(savedMode = pref.getInt(PREF_MODE));
  // setModeFromLoop returns early when the stored mode is the one already selected, so the live
  // set would still hold defaults. Load it explicitly for whatever mode we ended up in.
  modeScopedLoad(settingModeOf(mode));
  savedMeasure = measure = pref.getInt(PREF_MEASURE);
  // The taper is loaded per mode further up and handed to the live copy by modeScopedLoad(), so
  // there is nothing to read from the old shared key here.
  savedTurnPasses = turnPasses = pref.getInt(PREF_TURN_PASSES, turnPasses);
  savedAuxForward = auxForward = pref.getBool(PREF_AUX_FORWARD, true);
  pref.end();

  if (!z.needsRest && !z.disabled) {
    DHIGH(z.ena);
  }
  if (!x.needsRest && !x.disabled) {
    DHIGH(x.ena);
  }
  if (a1.active && !a1.needsRest && !a1.disabled) {
    DHIGH(a1.ena);
  }

  pref.begin(GCODE_NAMESPACE);
  gcodeProgramCount = 0;
  for (int i = 0; i < 256; i++) {
    if (pref.isKey(String(i).c_str())) {
      gcodeProgramCount++;
    } else {
      break;
    }
  }
  pref.end();

  lcd.begin(LCD_COLUMNS, LCD_ROWS);
  lcdLoadNormalChars();

  Serial.begin(115200);

  if (!Wire.begin(SDA, SCL)) {
    Serial.println("I2C initialization failed");
  } else if (!keypad.begin(TCA8418_DEFAULT_ADDR, &Wire)) {
    Serial.println("TCA8418 key controller not found");
  } else {
    keypad.matrix(7, 7);
    keypad.flush();
  }

  // Non-time-sensitive tasks on core 0.
  xTaskCreatePinnedToCore(taskDisplay, "taskDisplay", 10000 /* stack size */, NULL, 0 /* priority */, NULL, 0 /* core */);

  delay(100);
  if (keypad.available()) {
    setEmergencyStop(ESTOP_KEY);
    return;
  } else {
    xTaskCreatePinnedToCore(taskKeypad, "taskKeypad", 10000 /* stack size */, NULL, 0 /* priority */, NULL, 0 /* core */);
  }

  xTaskCreatePinnedToCore(taskMoveZ, "taskMoveZ", 10000 /* stack size */, NULL, 0 /* priority */, NULL, 0 /* core */);
  xTaskCreatePinnedToCore(taskMoveX, "taskMoveX", 10000 /* stack size */, NULL, 0 /* priority */, NULL, 0 /* core */);
  if (a1.active) xTaskCreatePinnedToCore(taskMoveA1, "taskMoveA1", 10000 /* stack size */, NULL, 0 /* priority */, NULL, 0 /* core */);
  xTaskCreatePinnedToCore(taskAttachInterrupts, "taskAttachInterrupts", 10000 /* stack size */, NULL, 0 /* priority */, NULL, 0 /* core */);
  xTaskCreatePinnedToCore(taskGcode, "taskGcode", 10000 /* stack size */, NULL, 0 /* priority */, NULL, 0 /* core */);
  // Always created, but it does nothing at all until the WiFi setting is switched on - that is
  // what lets the radio be started and stopped from the panel without a power cycle.
  xTaskCreatePinnedToCore(taskWeb, "taskWeb", 10000 /* stack size */, NULL, 0 /* priority */, NULL, 0 /* core */);
}

bool saveIfChanged() {
  // Should avoid calling Preferences whenever possible to reduce memory wear and avoid ~20ms write delay that blocks interrupts.
  if (dupr == savedDupr && starts == savedStarts && z.pos == z.savedPos && z.originPos == z.savedOriginPos && z.posGlobal == z.savedPosGlobal && z.motorPos == z.savedMotorPos && z.leftStop == z.savedLeftStop && z.rightStop == z.savedRightStop && z.disabled == z.savedDisabled &&
      spindlePos == savedSpindlePos && spindlePosAvg == savedSpindlePosAvg && spindlePosSync == savedSpindlePosSync && savedSpindlePosGlobal == spindlePosGlobal && showAngle == savedShowAngle && showTacho == savedShowTacho && showBigDro == savedShowBigDro && moveStep == savedMoveStep &&
      mode == savedMode && measure == savedMeasure && x.pos == x.savedPos && x.originPos == x.savedOriginPos && x.posGlobal == x.savedPosGlobal && x.motorPos == x.savedMotorPos && x.leftStop == x.savedLeftStop && x.rightStop == x.savedRightStop && x.disabled == x.savedDisabled &&
      a1.pos == a1.savedPos && a1.originPos == a1.savedOriginPos && a1.posGlobal == a1.savedPosGlobal && a1.motorPos == a1.savedMotorPos && a1.leftStop == a1.savedLeftStop && a1.rightStop == a1.savedRightStop && a1.disabled == a1.savedDisabled &&
      coneRatio == savedConeRatio && turnPasses == savedTurnPasses && savedAuxForward == auxForward) return false;

  Preferences pref;
  pref.begin(PREF_NAMESPACE);
  if (dupr != savedDupr) pref.putLong(PREF_DUPR, savedDupr = dupr);
  if (starts != savedStarts) pref.putInt(PREF_STARTS, savedStarts = starts);
  if (z.pos != z.savedPos) pref.putLong(PREF_POS_Z, z.savedPos = z.pos);
  if (z.posGlobal != z.savedPosGlobal) pref.putLong(PREF_POS_GLOBAL_Z, z.savedPosGlobal = z.posGlobal);
  if (z.originPos != z.savedOriginPos) pref.putLong(PREF_ORIGIN_POS_Z, z.savedOriginPos = z.originPos);
  if (z.motorPos != z.savedMotorPos) pref.putLong(PREF_MOTOR_POS_Z, z.savedMotorPos = z.motorPos);
  if (z.leftStop != z.savedLeftStop) pref.putLong(PREF_LEFT_STOP_Z, z.savedLeftStop = z.leftStop);
  if (z.rightStop != z.savedRightStop) pref.putLong(PREF_RIGHT_STOP_Z, z.savedRightStop = z.rightStop);
  if (z.disabled != z.savedDisabled) pref.putBool(PREF_DISABLED_Z, z.savedDisabled = z.disabled);
  if (spindlePos != savedSpindlePos) pref.putLong(PREF_SPINDLE_POS, savedSpindlePos = spindlePos);
  if (spindlePosAvg != savedSpindlePosAvg) pref.putLong(PREF_SPINDLE_POS_AVG, savedSpindlePosAvg = spindlePosAvg);
  if (spindlePosSync != savedSpindlePosSync) pref.putInt(PREF_OUT_OF_SYNC, savedSpindlePosSync = spindlePosSync);
  if (spindlePosGlobal != savedSpindlePosGlobal) pref.putLong(PREF_SPINDLE_POS_GLOBAL, savedSpindlePosGlobal = spindlePosGlobal);
  if (showAngle != savedShowAngle) pref.putBool(PREF_SHOW_ANGLE, savedShowAngle = showAngle);
  if (showTacho != savedShowTacho) pref.putBool(PREF_SHOW_TACHO, savedShowTacho = showTacho);
  if (showBigDro != savedShowBigDro) pref.putBool(PREF_SHOW_BDRO, savedShowBigDro = showBigDro);
  if (moveStep != savedMoveStep) pref.putLong(PREF_MOVE_STEP, savedMoveStep = moveStep);
  if (mode != savedMode) pref.putInt(PREF_MODE, savedMode = mode);
  if (measure != savedMeasure) pref.putInt(PREF_MEASURE, savedMeasure = measure);
  if (x.pos != x.savedPos) pref.putLong(PREF_POS_X, x.savedPos = x.pos);
  if (x.posGlobal != x.savedPosGlobal) pref.putLong(PREF_POS_GLOBAL_X, x.savedPosGlobal = x.posGlobal);
  if (x.originPos != x.savedOriginPos) pref.putLong(PREF_ORIGIN_POS_X, x.savedOriginPos = x.originPos);
  if (x.motorPos != x.savedMotorPos) pref.putLong(PREF_MOTOR_POS_X, x.savedMotorPos = x.motorPos);
  if (x.leftStop != x.savedLeftStop) pref.putLong(PREF_LEFT_STOP_X, x.savedLeftStop = x.leftStop);
  if (x.rightStop != x.savedRightStop) pref.putLong(PREF_RIGHT_STOP_X, x.savedRightStop = x.rightStop);
  if (x.disabled != x.savedDisabled) pref.putBool(PREF_DISABLED_X, x.savedDisabled = x.disabled);
  if (a1.pos != a1.savedPos) pref.putLong(PREF_POS_A1, a1.savedPos = a1.pos);
  if (a1.posGlobal != a1.savedPosGlobal) pref.putLong(PREF_POS_GLOBAL_A1, a1.savedPosGlobal = a1.posGlobal);
  if (a1.originPos != a1.savedOriginPos) pref.putLong(PREF_ORIGIN_POS_A1, a1.savedOriginPos = a1.originPos);
  if (a1.motorPos != a1.savedMotorPos) pref.putLong(PREF_MOTOR_POS_A1, a1.savedMotorPos = a1.motorPos);
  if (a1.leftStop != a1.savedLeftStop) pref.putLong(PREF_LEFT_STOP_A1, a1.savedLeftStop = a1.leftStop);
  if (a1.rightStop != a1.savedRightStop) pref.putLong(PREF_RIGHT_STOP_A1, a1.savedRightStop = a1.rightStop);
  if (a1.disabled != a1.savedDisabled) pref.putBool(PREF_DISABLED_A1, a1.savedDisabled = a1.disabled);
  // Under the mode's own key, so the cone and the tapered thread stop overwriting each other.
  if (coneRatio != savedConeRatio) {
    savedConeRatio = coneRatio;
    int sm = settingModeOf(mode);
    if (sm != SMODE_NONE) {
      modeSet[sm].taper = coneRatio;
      char key[16] = {'\0'};
      key[0] = settingModeLetterOf(sm);
      strcpy(key + 1, "cr");
      pref.putFloat(key, coneRatio);
    }
  }
  // Passes belong to the mode now, so it goes under the mode's own key. The wizard changes this
  // constantly and the old global key would have every mode overwriting the others.
  if (turnPasses != savedTurnPasses) {
    savedTurnPasses = turnPasses;
    int sm = settingModeOf(mode);
    if (sm != SMODE_NONE) {
      modeSet[sm].passes = turnPasses;
      char key[16] = {'\0'};
      key[0] = settingModeLetterOf(sm);
      strcpy(key + 1, "tps");
      pref.putLong(key, turnPasses);
    }
  }
  if (auxForward != savedAuxForward) pref.putBool(PREF_AUX_FORWARD, savedAuxForward = auxForward);
  pref.end();
  nvsSaveBatches++;
  return true;
}

void markAxisOrigin(Axis* a) {
  bool hasSemaphore = xSemaphoreTake(a->mutex, 10) == pdTRUE;
  if (!hasSemaphore) {
    beepFor(BEEP_REFUSED);
  }
  if (a->leftStop != LONG_MAX) {
    a->leftStop -= a->pos;
  }
  if (a->rightStop != LONG_MIN) {
    a->rightStop -= a->pos;
  }
  a->motorPos -= a->pos;
  a->originPos += a->pos;
  a->pos = 0;
  a->fractionalPos = 0;
  a->pendingPos = 0;
  if (hasSemaphore) {
    xSemaphoreGive(a->mutex);
  }
}

void zeroSpindlePos() {
  spindlePos = 0;
  spindlePosAvg = 0;
  spindlePosSync = 0;
}

// Loose the thread and mark current physical positions of
// encoder and stepper as a new 0. To be called when dupr changes
// or ELS is turned on/off. Without this, changing dupr will
// result in stepper rushing across the lathe to the new position.
// Must be called while holding motionMutex.
void markOrigin() {
  markAxisOrigin(&z);
  markAxisOrigin(&x);
  markAxisOrigin(&a1);
  zeroSpindlePos();
}

void markAxis0(Axis* a) {
  a->originPos = -a->pos;
}

Axis* getAsyncAxis() {
  return mode == MODE_A1 ? &a1 : &z;
}

void updateAsyncTimerSettings() {
  // dupr and therefore direction can change while we're in async mode.
  setDir(getAsyncAxis(), dupr > 0);

  // dupr can change while we're in async mode, keep updating timer frequency.
  timerAlarm(async_timer, getTimerLimit(), true, 0);
  // without this timer stops working if already above new limit
  timerWrite(async_timer, 0);
}

void setDupr(long value) {
  // Can't apply changes right away since we might be in the middle of motion logic.
  nextDupr = value;
  nextDuprFlag = true;
}

// Must be called while holding motionMutex.
void applyDupr() {
  if (nextDupr == dupr) {
    return;
  }
  dupr = nextDupr;
  markOrigin();
  if (mode == MODE_ASYNC || mode == MODE_A1) {
    updateAsyncTimerSettings();
  }
}

void setStarts(int value) {
  // Can't apply changes right away since we might be in the middle of motion logic.
  nextStarts = value;
  nextStartsFlag = true;
}

// Must be called while holding motionMutex.
void applyStarts() {
  if (starts == nextStarts) {
    return;
  }
  starts = nextStarts;
  markOrigin();
}

void setMeasure(int value) {
  if (measure == value) {
    return;
  }
  measure = value;
  moveStep = measure == MEASURE_METRIC ? MOVE_STEP_1 : MOVE_STEP_IMP_1;
}

unsigned int getTimerLimit() {
  if (dupr == 0) {
    return 65535;
  }
  return min(long(65535), long(1000000 / (z.motorSteps * abs(dupr) / z.screwPitch)) - 1); // 1000000/Hz - 1
}

// Only used for async movement in ASYNC and A1 modes.
// Keep code in this method to absolute minimum to achieve high stepper speeds.
void IRAM_ATTR onAsyncTimer() {
  Axis* a = getAsyncAxis();
  if (!isOn || a->movingManually || (mode != MODE_ASYNC && mode != MODE_A1)) {
    return;
  } else if (dupr > 0 && a->pos < a->leftStop) {
    if (a->pos <= a->motorPos) {
      a->pos++;
    }
    a->motorPos++;
    a->posGlobal++;
  } else if (dupr < 0 && a->pos > a->rightStop) {
    if (a->pos >= a->motorPos + a->backlashSteps) {
      a->pos--;
    }
    a->motorPos--;
    a->posGlobal--;
  } else {
    return;
  }

  DLOW(a->step);
  a->stepStartUs = micros();
  delayMicroseconds(10);
  DHIGH(a->step);
}

void setModeFromTask(int value) {
  nextMode = value;
  nextModeFlag = true;
}

void setModeFromLoop(int value) {
  if (mode == value) {
    return;
  }
  if (isOn) {
    setIsOnFromLoop(false);
  }
  if (isThreadMode()) {
    setStarts(1);
  } else if (mode == MODE_ASYNC || mode == MODE_A1) {
    setAsyncTimerEnable(false);
  }
  // Hand the per-mode settings over: the mode being left keeps what it was using, and the one
  // being entered gets its own back. Only once the arrays have been read, or the first mode
  // change during startup would store defaults over what is stored.
  if (modeScopedLoaded) {
    modeScopedStore(settingModeOf(mode));
    modeScopedLoad(settingModeOf(value));
  }
  mode = value;
  setupIndex = 0;
  if (mode == MODE_ASYNC || mode == MODE_A1) {
    if (!timerAttached) {
      timerAttached = true;
      timerAttachInterrupt(async_timer, &onAsyncTimer);
    }
    updateAsyncTimerSettings();
    setAsyncTimerEnable(true);
  }
}

void setTurnPasses(int value) {
  if (isOn) {
    beep();
  } else {
    turnPasses = value;
  }
}

void setConeRatio(float value) {
  // Can't apply changes right away since we might be in the middle of motion logic.
  nextConeRatio = value;
  nextConeRatioFlag = true;
}

void applyConeRatio() {
  if (nextConeRatio == coneRatio) {
    return;
  }
  coneRatio = nextConeRatio;
  markOrigin();
}

void reset() {
  z.leftStop = LONG_MAX;
  z.nextLeftStopFlag = false;
  z.rightStop = LONG_MIN;
  z.nextRightStopFlag = false;
  z.originPos = 0;
  z.posGlobal = 0;
  z.motorPos = 0;
  z.pendingPos = 0;
  z.disabled = false;
  x.leftStop = LONG_MAX;
  x.nextLeftStopFlag = false;
  x.rightStop = LONG_MIN;
  x.nextRightStopFlag = false;
  x.originPos = 0;
  x.posGlobal = 0;
  x.motorPos = 0;
  x.pendingPos = 0;
  x.disabled = false;
  a1.leftStop = LONG_MAX;
  a1.nextLeftStopFlag = false;
  a1.rightStop = LONG_MIN;
  a1.nextRightStopFlag = false;
  a1.originPos = 0;
  a1.posGlobal = 0;
  a1.motorPos = 0;
  a1.pendingPos = 0;
  a1.disabled = false;
  setDupr(0);
  setStarts(1);
  moveStep = MOVE_STEP_1;
  setModeFromTask(MODE_NORMAL);
  measure = MEASURE_METRIC;
  showTacho = false;
  showAngle = false;
  showBigDro = false;
  setConeRatio(1);
  auxForward = true;
  xRetracted = false;
}

long normalizePitch(long pitch) {
  int scale = 1;
  if (measure == MEASURE_METRIC) {
    // Drop the 3rd and 4th precision point if any.
    scale = 100;
  } else if (measure == MEASURE_INCH) {
    // Always drop the 4th precision point in inch representation if any.
    scale = 254;
  }
  return round(pitch / scale) * scale;
}

void buttonPlusMinusPress(bool plus) {
  // Mutex is aquired in setDupr() and setStarts().
  bool minus = !plus;
  if (isThreadMode() && setupIndex == 2) {
    if (minus && starts > 2) {
      setStarts(starts - 1);
    } else if (plus && starts < STARTS_MAX) {
      setStarts(starts + 1);
    }
  } else if (isPassMode() && setupIndex == 1 && getNumpadResult() == 0) {
    if (minus && turnPasses > 1) {
      setTurnPasses(turnPasses - 1);
    } else if (plus && turnPasses < PASSES_MAX) {
      setTurnPasses(turnPasses + 1);
    }
  } else if (measure != MEASURE_TPI) {
    int delta = measure == MEASURE_METRIC ? MOVE_STEP_3 : MOVE_STEP_IMP_3;
    // Switching between mm/inch/tpi often results in getting non-0 3rd and 4th
    // precision points that can't be easily controlled. Remove them.
    long normalizedDupr = normalizePitch(dupr);
    if (minus && dupr > -DUPR_MAX) {
      setDupr(max(-DUPR_MAX, normalizedDupr - delta));
    } else if (plus && dupr < DUPR_MAX) {
      setDupr(min(DUPR_MAX, normalizedDupr + delta));
    }
  } else { // TPI
    if (dupr == 0) {
      setDupr(plus ? 1 : -1);
    } else {
      long currentTpi = round(254000.0 / dupr);
      long tpi = currentTpi + (plus ? 1 : -1);
      long newDupr = tpi == 0 ? (plus ? DUPR_MAX : -DUPR_MAX) : round(254000.0 / tpi);
      // Happens for small pitches like 0.01mm.
      if (newDupr == dupr) {
        newDupr += plus ? -1 : 1;
      }
      if (newDupr != dupr && newDupr < DUPR_MAX && newDupr > -DUPR_MAX) {
        setDupr(newDupr);
      }
    }
  }
}

// Asks for a pattern. Safe from any task and from the motion loop - it only sets a flag, and
// taskDisplay does the work. NOT safe from an interrupt: this function lives in flash like
// everything else here, so calling it from one of the IRAM_ATTR handlers would fault the moment a
// settings write had the flash cache disabled.
void beepFor(int pattern) {
  beepRequest = pattern;
}

// The general acknowledgement, and what every existing caller means by a beep.
void beep() {
  beepFor(BEEP_ACK);
}

// One-key retract & return: first press moves X away from the workpiece by retractDu remembering
// the current position, second press moves it back. Direction is away from the cut given auxForward.
void xRetractToggle() {
  if (isOn || x.movingManually || x.disabled) {
    splashError(x.disabled ? "X is disabled" : "Turn off first");
    return;
  }
  if (!xRetracted) {
    long target = x.pos + (auxForward ? -1 : 1) * long(retractDu * x.motorSteps / x.screwPitch);
    if (target > x.leftStop) target = x.leftStop;
    else if (target < x.rightStop) target = x.rightStop;
    if (target == x.pos) {
      splashError("Limited by stop");
      return;
    }
    xRetractReturnPos = x.pos;
    xRetracted = true;
    x.speedMax = x.speedManualMove;
    stepToFinal(&x, target);
  } else {
    xRetracted = false;
    x.speedMax = x.speedManualMove;
    stepToFinal(&x, xRetractReturnPos);
  }
}

void buttonOnOffPress(bool on) {
  resetMillis = millis();
  // Automated moves invalidate the stored retract return position.
  if (on) xRetracted = false;
  bool missingZStops = needZStops() && (z.leftStop == LONG_MAX || z.rightStop == LONG_MIN);
  if (on && isPassMode() && (missingZStops || x.leftStop == LONG_MAX || x.rightStop == LONG_MIN)) {
    splashError(needZStops() ? "Set all stops first" : "Set X stops first");
  } else if (!isOn && on && mode == MODE_GCODE && gcodeProgramIndex >= gcodeProgramCount && setupIndex == 1) {
    beep();
  } else if (!isOn && on && setupIndex < getLastSetupIndex()) {
    // Move to the next setup step.
    setupIndex++;
  } else if (isOn && on && (mode == MODE_TURN || mode == MODE_FACE || isThreadMode())) {
    // Move to the next pass.
    opIndexAdvanceFlag = true;
  } else if (!on && (z.movingManually || x.movingManually || x.movingManually)) {
    setEmergencyStop(ESTOP_OFF_MANUAL_MOVE);
  } else if (!isOn && on && mode == MODE_GCODE && gcodeProgramIndex >= gcodeProgramCount) {
    beep();
  } else if (!isOn && on && mode == MODE_GCODE) {
    Preferences pref;
    pref.begin(GCODE_NAMESPACE);
    if (!pref.isKey(String(gcodeProgramIndex).c_str())) {
      beep();
    } else {
      String programName = pref.getString(String(gcodeProgramIndex).c_str());
      if (programName.length() == 0) {
        beep();
      } else {
        gcodeProgramCharIndex = 0;
        gcodeProgram = pref.getString(programName.c_str());
        gcodeProgram += '\n'; // ensures the last line is executed
        setIsOnFromTask(on);
      }
    }
    pref.end();
  } else {
    setIsOnFromTask(on);
  }
}

void setIsOnFromTask(bool on) {
  nextIsOn = on;
  nextIsOnFlag = true;
}

void setIsOnFromLoop(bool on) {
  if (isOn && on) {
    return;
  }
  if (!on) {
    isOn = false;
    setupIndex = 0;
  }
  stepperEnable(&z, on);
  stepperEnable(&x, on);
  stepperEnable(&a1, on);
  markOrigin();
  if (on) {
    isOn = true;
    opDuprSign = dupr >= 0 ? 1 : -1;
    opDupr = dupr;
    opIndex = 0;
    opIndexAdvanceFlag = false;
    opSubIndex = 0;
    setupIndex = 0;
  }
}

void buttonOffRelease() {
  if (millis() - resetMillis > 3000) {
    reset();
    splashScreen = true;
  }
}

void setLeftStop(Axis* a, long value) {
  // Can't apply changes right away since we might be in the middle of motion logic.
  a->nextLeftStop = value;
  a->nextLeftStopFlag = true;
}

void leaveStop(Axis* a, long oldStop) {
  if (mode == MODE_CONE) {
    // To avoid rushing to a far away position if standing on limit.
    markOrigin();
  } else if (isGearboxMode() && a == getPitchAxis() && a->pos == oldStop) {
    // Spindle is most likely out of sync with the stepper because
    // it was spinning while the lead screw was on the stop. Worth its own sound: the thread being
    // cut is wrong from here on, and nothing else about the machine looks different.
    spindlePosSync = spindleModulo(spindlePos - spindleFromPos(a, a->pos));
    beepFor(BEEP_SYNC_LOST);
  }
}

void applyLeftStop(Axis* a) {
  // Accept left stop even if it's lower than pos.
  // Stop button press processing takes time during which motor could have moved.
  long oldStop = a->leftStop;
  a->leftStop = a->nextLeftStop;
  leaveStop(a, oldStop);
}

void setRightStop(Axis* a, long value) {
  // Can't apply changes right away since we might be in the middle of motion logic.
  a->nextRightStop = value;
  a->nextRightStopFlag = true;
}

void applyRightStop(Axis* a) {
  // Accept right stop even if it's higher than pos.
  // Stop button press processing takes time during which motor could have moved.
  long oldStop = a->rightStop;
  a->rightStop = a->nextRightStop;
  leaveStop(a, oldStop);
}

void buttonLeftStopPress(Axis* a) {
  setLeftStop(a, a->leftStop == LONG_MAX ? a->pos : LONG_MAX);
}

void buttonRightStopPress(Axis* a) {
  setRightStop(a, a->rightStop == LONG_MIN ? a->pos : LONG_MIN);
}

void buttonDisplayPress() {
  if (!showAngle && !showTacho && !showBigDro) {
    showAngle = true;
  } else if (showAngle) {
    showAngle = false;
    showTacho = true;
  } else if (showTacho) {
    showTacho = false;
    showBigDro = true;
  } else {
    showBigDro = false;
  }
}

void buttonMoveStepPress() {
  if (measure == MEASURE_METRIC) {
    if (moveStep == MOVE_STEP_1) {
      moveStep = MOVE_STEP_2;
    } else if (moveStep == MOVE_STEP_2) {
      moveStep = MOVE_STEP_3;
    } else {
      moveStep = MOVE_STEP_1;
    }
  } else {
    if (moveStep == MOVE_STEP_IMP_1) {
      moveStep = MOVE_STEP_IMP_2;
    } else if (moveStep == MOVE_STEP_IMP_2) {
      moveStep = MOVE_STEP_IMP_3;
    } else {
      moveStep = MOVE_STEP_IMP_1;
    }
  }
}

void setDir(Axis* a, bool dir) {
  // Start slow if direction changed.
  if (a->direction != dir || !a->directionInitialized) {
    a->speed = a->speedStart;
    a->direction = dir;
    a->directionInitialized = true;
    DWRITE(a->dir, dir ^ a->invertStepper);
    delayMicroseconds(DIRECTION_SETUP_DELAY_US);
  }
}

void buttonModePress() {
  if (mode == MODE_NORMAL) {
    setModeFromTask(ACTIVE_A1 ? MODE_A1 : MODE_ELLIPSE);
  } else if (mode == MODE_A1) {
    setModeFromTask(MODE_ELLIPSE);
  } else if (mode == MODE_ELLIPSE) {
    setModeFromTask(MODE_GCODE);
  } else if (mode == MODE_GCODE) {
    setModeFromTask(MODE_ASYNC);
  } else if (mode == MODE_ASYNC) {
    setModeFromTask(MODE_SLOT);
  } else {
    setModeFromTask(MODE_NORMAL);
  }
}

void buttonMeasurePress() {
  if (measure == MEASURE_METRIC) {
    setMeasure(MEASURE_INCH);
  } else if (measure == MEASURE_INCH) {
    setMeasure(MEASURE_TPI);
  } else {
    setMeasure(MEASURE_METRIC);
  }
}

void buttonReversePress() {
  setDupr(-dupr);
}

void numpadPress(int digit) {
  // Typing a digit means a number is being entered. The settings and calibration screens call
  // this directly instead of going through processNumpad(), which is the only place that used
  // to set the flag - so without setting it here the branch below reset the index on every
  // keystroke and only the most recent digit survived.
  if (!inNumpad) {
    numpadIndex = 0;
    inNumpad = true;
  }
  numpadDigits[numpadIndex] = digit;
  if (numpadIndex < 7) {
    numpadIndex++;
  } else {
    numpadIndex = 0;
  }
}

void numpadBackspace() {
  if (inNumpad && numpadIndex > 0) {
    numpadIndex--;
  }
}

void resetNumpad() {
  numpadIndex = 0;
  // No entry is in progress any more, so the next digit starts a fresh number rather than
  // appending to the one just consumed.
  inNumpad = false;
}

long getNumpadResult() {
  long result = 0;
  for (int i = 0; i < numpadIndex; i++) {
    result += numpadDigits[i] * pow(10, numpadIndex - 1 - i);
  }
  return result;
}

void numpadPlusMinus(bool plus) {
  if (numpadDigits[numpadIndex - 1] < 9 && plus) {
    numpadDigits[numpadIndex - 1]++;
  } else if (numpadDigits[numpadIndex - 1] > 1 && !plus) {
    numpadDigits[numpadIndex - 1]--;
  }
  // TODO: implement going over 9 and below 1.
}

long numpadToDeciMicrons() {
  long result = getNumpadResult();
  if (result == 0) {
    return 0;
  }
  if (measure == MEASURE_INCH) {
    result = result * 254;
  } else if (measure == MEASURE_TPI) {
    result = round(254000.0 / result);
  } else { // Metric
    result = result * 10;
  }
  return result;
}

float numpadToConeRatio() {
  return getNumpadResult() / 100000.0;
}

bool processNumpad(int keyCode) {
  if (keyCode == B_0) {
    numpadPress(0);
    inNumpad = true;
  } else if (keyCode == B_1) {
    numpadPress(1);
    inNumpad = true;
  } else if (keyCode == B_2) {
    numpadPress(2);
    inNumpad = true;
  } else if (keyCode == B_3) {
    numpadPress(3);
    inNumpad = true;
  } else if (keyCode == B_4) {
    numpadPress(4);
    inNumpad = true;
  } else if (keyCode == B_5) {
    numpadPress(5);
    inNumpad = true;
  } else if (keyCode == B_6) {
    numpadPress(6);
    inNumpad = true;
  } else if (keyCode == B_7) {
    numpadPress(7);
    inNumpad = true;
  } else if (keyCode == B_8) {
    numpadPress(8);
    inNumpad = true;
  } else if (keyCode == B_9) {
    numpadPress(9);
    inNumpad = true;
  } else if (keyCode == B_BACKSPACE) {
    numpadBackspace();
    inNumpad = true;
  } else if (inNumpad && (keyCode == B_PLUS || keyCode == B_MINUS)) {
    numpadPlusMinus(keyCode == B_PLUS);
    return true;
  } else if (inNumpad) {
    inNumpad = false;
    return processNumpadResult(keyCode);
  }
  return inNumpad;
}

bool processNumpadResult(int keyCode) {
  long newDu = numpadToDeciMicrons();
  float newConeRatio = numpadToConeRatio();
  long numpadResult = getNumpadResult();
  resetNumpad();
  // Ignore numpad input unless confirmed with ON.
  if (keyCode == B_ON) {
    if (isPassMode() && setupIndex == 1) {
      setTurnPasses(int(min(PASSES_MAX, numpadResult)));
      setupIndex++;
    } else if (mode == MODE_CONE && setupIndex == 1) {
      setConeRatio(newConeRatio);
      setupIndex++;
    } else if (mode == MODE_TPR && setupIndex == 3) {
      // Taper for the thread being cut, e.g. 0.0625 for NPT's 1:16.
      setConeRatio(newConeRatio);
      setupIndex++;
    } else {
      if (abs(newDu) <= DUPR_MAX) {
        setDupr(newDu);
      }
    }
    // Don't use this ON press for starting the motion.
    return true;
  }

  // Shared piece for stops and moves.
  Axis* a = (keyCode == B_STOPL || keyCode == B_STOPR || keyCode == B_LEFT || keyCode == B_RIGHT || keyCode == B_Z) ? &z : &x;
  int sign = ((keyCode == B_STOPL || keyCode == B_STOPU || keyCode == B_LEFT || keyCode == B_UP || keyCode == B_Z || keyCode == B_X || keyCode == B_A) ? 1 : -1);
  if (mode == MODE_A1 && (keyCode == B_MODE_GEARS || keyCode == B_MODE_TURN || keyCode == B_MODE_FACE || keyCode == B_MODE_CONE || keyCode == B_MODE_THREAD)) {
    a = &a1;
    sign = (keyCode == B_MODE_GEARS || keyCode == B_MODE_FACE) ? -1 : 1;
  }
  // In diameter mode, typed X distances and coordinates are diameters, so the physical travel is half of that.
  if (xDiameterDisplay && (keyCode == B_UP || keyCode == B_DOWN || keyCode == B_STOPU || keyCode == B_STOPD || keyCode == B_X)) {
    newDu /= 2;
  }
  long pos = a->pos + (a->rotational ? numpadResult * 10 : newDu) / a->screwPitch * a->motorSteps * sign;

  // Potentially assign a new value to a limit. Treat newDu as a relative distance from current position.
  if (keyCode == B_STOPL) {
    setLeftStop(&z, pos);
    return true;
  } else if (keyCode == B_STOPR) {
    setRightStop(&z, pos);
    return true;
  } else if (keyCode == B_STOPU) {
    setLeftStop(&x, pos);
    return true;
  } else if (keyCode == B_STOPD) {
    setRightStop(&x, pos);
    return true;
  } else if (mode == MODE_A1) {
    if (keyCode == B_MODE_CONE) {
      setLeftStop(&a1, pos);
      return true;
    } else if (keyCode == B_MODE_FACE) {
      setRightStop(&a1, pos);
      return true;
    }
  }

  // Potentially move by newDu in the given direction.
  // We don't support precision manual moves when ON yet. Can't stay in the thread for most modes.
  if (!isOn && (keyCode == B_LEFT || keyCode == B_RIGHT || keyCode == B_UP || keyCode == B_DOWN || (mode == MODE_A1 && (keyCode == B_MODE_GEARS || keyCode == B_MODE_TURN)))) {
    if (pos < a->rightStop) {
      pos = a->rightStop;
      splashError("Limited by stop");
    } else if (pos > a->leftStop) {
      pos = a->leftStop;
      splashError("Limited by stop");
    } else if (abs(pos - a->pos) > a->estopSteps) {
      splashError("Too far, ignored");
      return true;
    }
    a->speedMax = a->speedManualMove;
    stepToFinal(a, pos);
    return true;
  }

  // Set A1 axis 0 newDu ahead.
  if (mode == MODE_A1 && keyCode == B_MODE_THREAD) {
    a->originPos = -pos;
    return true;
  }

  // Move the axis to the typed coordinate as it would be shown on the display, observing the stops.
  if (keyCode == B_Z || keyCode == B_X) {
    if (isOn) {
      beep();
      return true;
    }
    long target = newDu / a->screwPitch * a->motorSteps - a->originPos;
    if (target < a->rightStop) {
      target = a->rightStop;
      splashError("Limited by stop");
    } else if (target > a->leftStop) {
      target = a->leftStop;
      splashError("Limited by stop");
    } else if (abs(target - a->pos) > a->estopSteps) {
      splashError("Too far, ignored");
      return true;
    }
    a->speedMax = a->speedManualMove;
    stepToFinal(a, target);
    return true;
  }

  // Set X axis 0 from diameter.
  if (keyCode == B_A) {
    a->originPos = -(a->pos + pos) / 2;
    return true;
  }

  if (keyCode == B_STEP) {
    if (newDu > 0) {
      moveStep = newDu;
    } else {
      beep();
    }
    return true;
  }

  return false;
}

void applyEncoderPpr() {
  // Counts per revolution of the SPINDLE, which is what everything downstream means by a
  // revolution. A belt-driven encoder turns spindleTeeth/pulleyTeeth times per spindle turn,
  // and the divider folds that many raw counts into one step.
  ENCODER_STEPS_INT = calCountsPerSpindleRev(encoderPpr, encoderSpindleTeeth, encoderPulleyTeeth, encoderDivider);
  ENCODER_STEPS_FLOAT = ENCODER_STEPS_INT;
  RPM_BULK = ENCODER_STEPS_INT;
}

// Overrides compiled-in axis defaults with values changed via the settings menu, if any.
void loadAxisSettings(Axis* a, Preferences* pref) {
  String n = String(a->name);
  a->invertStepper = pref->getBool((n + "inv").c_str(), a->invertStepper);
  a->backlashDu = pref->getLong((n + "bla").c_str(), a->backlashDu);
  a->screwPitch = pref->getLong((n + "scr").c_str(), (long) round(a->screwPitch));
  a->motorStepsPerTurn = pref->getLong((n + "mst").c_str(), (long) round(a->motorStepsPerTurn));
  a->motorTeeth = pref->getLong((n + "mtt").c_str(), a->motorTeeth);
  a->screwTeeth = pref->getLong((n + "stt").c_str(), a->screwTeeth);
  a->speedManualMove = pref->getLong((n + "spd").c_str(), a->speedManualMove);
  a->acceleration = pref->getLong((n + "acc").c_str(), a->acceleration);
  a->maxTravelMm = pref->getLong((n + "mtr").c_str(), a->maxTravelMm);
  a->speedStart = pref->getLong((n + "sst").c_str(), a->speedStart);
  a->needsRest = pref->getBool((n + "rst").c_str(), a->needsRest);
  a->active = pref->getBool((n + "act").c_str(), a->active);
  a->rotational = pref->getBool((n + "rot").c_str(), a->rotational);
  recomputeAxisDerived(a);
}

void settingsPutLong(Axis* a, const char* suffix, long value) {
  Preferences pref;
  pref.begin(PREF_NAMESPACE);
  pref.putLong((String(a->name) + suffix).c_str(), value);
  pref.end();
}

void settingsPutBool(Axis* a, const char* suffix, bool value) {
  Preferences pref;
  pref.begin(PREF_NAMESPACE);
  pref.putBool((String(a->name) + suffix).c_str(), value);
  pref.end();
}

// Axis that the current settings menu item refers to, NULL for the global items.
Axis* settingsAxis() {
  return settingsAxisOf(settingsIndex);
}

bool settingsUsesDu() {
  return settingUsesDu(settingsIndex);
}

bool settingsIsToggle() {
  return settingIsToggle(settingsIndex);
}

// The names behind a list item's value. They live with the thing they describe rather than in
// SETTINGS[], which has no room for a list of labels per row - so this is the one place that maps
// a storage key to its list.
const char* settingListName(int index, long value) {
  if (!strcmp(SETTINGS[index].prefKey, "cmat")) return materialName((int) value);
  return "?";
}

int settingListCount(int index) {
  if (!strcmp(SETTINGS[index].prefKey, "cmat")) return materialCount();
  return 1;
}

void settingsPutGlobalLong(const char* key, long value) {
  Preferences pref;
  pref.begin(PREF_NAMESPACE);
  pref.putLong(key, value);
  pref.end();
}

void settingsPutGlobalInt(const char* key, int value) {
  Preferences pref;
  pref.begin(PREF_NAMESPACE);
  pref.putInt(key, value);
  pref.end();
}

void settingsPutGlobalBool(const char* key, bool value) {
  Preferences pref;
  pref.begin(PREF_NAMESPACE);
  pref.putBool(key, value);
  pref.end();
}

int keyCodeToDigit(int keyCode) {
  if (keyCode == B_0) return 0;
  if (keyCode == B_1) return 1;
  if (keyCode == B_2) return 2;
  if (keyCode == B_3) return 3;
  if (keyCode == B_4) return 4;
  if (keyCode == B_5) return 5;
  if (keyCode == B_6) return 6;
  if (keyCode == B_7) return 7;
  if (keyCode == B_8) return 8;
  if (keyCode == B_9) return 9;
  return -1;
}

// Opens the current mode's own page directly, skipping the directory it is not part of.
void enterModeScreen() {
  enterSettingsScreen(false);
  settingsSection = SEC_MODE;
  settingsIndex = settingModeFirst(settingModeOf(mode));
}

void enterSettingsScreen(bool threadPicker) {
  inSettings = !threadPicker;
  inThreadPicker = threadPicker;
  settingsSection = -1; // Always open on the directory
  resetNumpad();
  inNumpad = false;
  settingsLcdHash = LCD_HASH_INITIAL;
  // Ensure no manual move continues while the settings screen swallows key releases.
  buttonLeftPressed = false;
  buttonRightPressed = false;
  buttonUpPressed = false;
  buttonDownPressed = false;
  buttonGearsPressed = false;
  buttonTurnPressed = false;
  buttonAPressed = false;
}

void exitSettingsScreens() {
  inSettings = false;
  inThreadPicker = false;
  resetNumpad();
  inNumpad = false;
  lcdHashLine0 = LCD_HASH_INITIAL; // Force full redraw of the normal screen.
  // When exiting with the off button, its release event goes through normal processing.
  // Without this, buttonOffRelease() would see a stale resetMillis and trigger a reset().
  resetMillis = millis();
}

// Axis a table entry acts on, NULL for the global items.
Axis* settingsAxisOf(int index) {
  switch (SETTINGS[index].axis) {
    case SAX_Z: return &z;
    case SAX_X: return &x;
    case SAX_A1: return &a1;
    default: return NULL;
  }
}

// Current stored value of a settings menu item, in the units it is persisted in: deci-microns
// for distances, 0 or 1 for toggles.
//
// Dispatched on the table's own storage key rather than on the menu index, so there is one
// branch per distinct parameter instead of one per menu row. Three axes share the same handful
// of branches, and re-ordering the menu cannot silently point a row at the wrong variable.
long settingsReadValue(int index) {
  const char* k = SETTINGS[index].prefKey;
  if (settingIsModeScoped(index)) {
    return modeScopedRead(SETTINGS[index].mode, k);
  }
  Axis* a = settingsAxisOf(index);
  if (a != NULL) {
    if (!strcmp(k, "inv")) return a->invertStepper ? 1 : 0;
    if (!strcmp(k, "bla")) return a->backlashDu;
    if (!strcmp(k, "scr")) return (long) round(a->screwPitch);
    if (!strcmp(k, "mst")) return (long) round(a->motorStepsPerTurn);
    if (!strcmp(k, "mtt")) return a->motorTeeth;
    if (!strcmp(k, "stt")) return a->screwTeeth;
    if (!strcmp(k, "sst")) return a->speedStart;
    if (!strcmp(k, "spd")) return a->speedManualMove;
    if (!strcmp(k, "acc")) return a->acceleration;
    if (!strcmp(k, "mtr")) return a->maxTravelMm;
    if (!strcmp(k, "rst")) return a->needsRest ? 1 : 0;
    if (!strcmp(k, "act")) return a->active ? 1 : 0;
    if (!strcmp(k, "rot")) return a->rotational ? 1 : 0;
    return 0;
  }
  if (!strcmp(k, "eppr")) return encoderPpr;
  if (!strcmp(k, "einv")) return encoderInvert ? 1 : 0;
  if (!strcmp(k, "est")) return encoderSpindleTeeth;
  if (!strcmp(k, "ept")) return encoderPulleyTeeth;
  if (!strcmp(k, "ediv")) return encoderDivider;
  if (!strcmp(k, "ebl")) return encoderBacklash;
  if (!strcmp(k, "ebls")) return encoderSymmetric ? 1 : 0;
  if (!strcmp(k, "eflt")) return encoderFilter;
  if (!strcmp(k, "rtd")) return retractDu;
  if (!strcmp(k, "p1u")) return pulse1Use ? 1 : 0;
  if (!strcmp(k, "p1i")) return pulse1Invert ? 1 : 0;
  if (!strcmp(k, "p2u")) return pulse2Use ? 1 : 0;
  if (!strcmp(k, "p2i")) return pulse2Invert ? 1 : 0;
  if (!strcmp(k, "hppr")) return (long) round(pulsePerRevolution);
  if (!strcmp(k, "pmw")) return pulseMinWidthUs;
  if (!strcmp(k, "phb")) return pulseHalfBacklash;
  if (!strcmp(k, "joy")) return joystickUse ? 1 : 0;
  if (!strcmp(k, "jdb")) return joystickDebounceMs;
  if (!strcmp(k, "xdd")) return xDiameterDisplay ? 1 : 0;
  if (!strcmp(k, "stm")) return stepTimeMs;
  if (!strcmp(k, "sdl")) return delayBetweenStepsMs;
  if (!strcmp(k, "idiv")) return indexDivisions;
  if (!strcmp(k, "itol")) return indexToleranceTenths;
  if (!strcmp(k, "cson")) return cssEnabled ? 1 : 0;
  if (!strcmp(k, "cmat")) return cssMaterial;
  if (!strcmp(k, "ctol")) return cssCarbide ? 1 : 0;
  if (!strcmp(k, "css")) return surfaceSpeedMPerMin;
  if (!strcmp(k, "smax")) return spindleMaxRpm;
  if (!strcmp(k, "p1a")) return pulse1DrivesX ? 1 : 0;
  if (!strcmp(k, "p2a")) return pulse2DrivesX ? 1 : 0;
  if (!strcmp(k, "wfen")) return wifiEnabled ? 1 : 0;
  if (!strcmp(k, "wfpw")) return wifiPin;
  return 0;
}

// Validates and applies a settings menu item, persisting it and re-deriving anything that
// depends on it. Returns an error string to show the user, or NULL on success. The single place
// a setting is written, so the menu and the serial restore command cannot disagree.
const char* settingsWriteValue(int index, long value) {
  const char* k = SETTINGS[index].prefKey;
  if (settingIsModeScoped(index)) {
    const char* err = modeScopedWrite(SETTINGS[index].mode, k, value);
    if (err != NULL) return err;
    char key[16];
    settingKeyName(index, key);
    if (settingIsToggle(index)) {
      settingsPutGlobalBool(key, value != 0);
    } else {
      settingsPutGlobalLong(key, value);
    }
    return NULL;
  }
  Axis* a = settingsAxisOf(index);
  if (a != NULL) {
    if (!strcmp(k, "inv")) {
      a->invertStepper = value != 0;
      a->directionInitialized = false;
    } else if (!strcmp(k, "bla")) {
      a->backlashDu = value;
    } else if (!strcmp(k, "scr")) {
      if (value <= 0) return "Must be above 0";
      a->screwPitch = value;
    } else if (!strcmp(k, "mst")) {
      if (value <= 0) return "Must be above 0";
      a->motorStepsPerTurn = value;
    } else if (!strcmp(k, "mtt")) {
      if (value < 1 || value > 1000) return "Teeth must be 1-1000";
      a->motorTeeth = value;
    } else if (!strcmp(k, "stt")) {
      if (value < 1 || value > 1000) return "Teeth must be 1-1000";
      a->screwTeeth = value;
    } else if (!strcmp(k, "sst")) {
      if (value <= 0) return "Must be above 0";
      if (value > a->speedManualMove) return "Above max speed";
      a->speedStart = value;
    } else if (!strcmp(k, "spd")) {
      if (value < a->speedStart) return "Below start speed";
      a->speedManualMove = value;
    } else if (!strcmp(k, "acc")) {
      if (value <= 0) return "Must be above 0";
      a->acceleration = value;
    } else if (!strcmp(k, "mtr")) {
      if (value <= 0) return "Must be above 0";
      a->maxTravelMm = value;
    } else if (!strcmp(k, "rst")) {
      a->needsRest = value != 0;
    } else if (!strcmp(k, "act")) {
      // Only A1 lives on the shared terminals; Z and X have pins of their own.
      if (a == &a1 && value != 0) {
        const char* clash = auxConflictMessage(AUX_A1_AXIS);
        if (clash != NULL) return clash;
      }
      a->active = value != 0;
      if (a == &a1) applyAuxPins();
    } else if (!strcmp(k, "rot")) {
      a->rotational = value != 0;
    } else {
      return "Not a stored setting";
    }
    if (settingIsToggle(index)) {
      settingsPutBool(a, k, value != 0);
    } else {
      settingsPutLong(a, k, value);
    }
    if (xSemaphoreTake(a->mutex, 100) == pdTRUE) {
      recomputeAxisDerived(a);
      xSemaphoreGive(a->mutex);
    } else {
      recomputeAxisDerived(a);
    }
    return NULL;
  }

  if (!strcmp(k, "eppr")) {
    if (!calPprValid(value)) return "PPR must be 24-10000";
    encoderPpr = value;
  } else if (!strcmp(k, "einv")) {
    encoderInvert = value != 0;
  } else if (!strcmp(k, "est")) {
    if (value < 1 || value > 1000) return "Teeth must be 1-1000";
    encoderSpindleTeeth = value;
  } else if (!strcmp(k, "ept")) {
    if (value < 1 || value > 1000) return "Teeth must be 1-1000";
    encoderPulleyTeeth = value;
  } else if (!strcmp(k, "ediv")) {
    if (value < 1 || value > 100) return "Divider must be 1-100";
    encoderDivider = value;
    encoderDivRemainder = 0;
  } else if (!strcmp(k, "ebl")) {
    if (value < 0 || value > 1000) return "Must be 0 to 1000";
    encoderBacklash = value;
  } else if (!strcmp(k, "ebls")) {
    encoderSymmetric = value != 0;
    // The two shapes hold the follower in different bands - [pos, pos+band] against
    // [pos-band, pos+band] - so switching leaves it outside the new one by up to a full band.
    // Re-centre now rather than letting the first count of the next cut take up the difference.
    spindlePosAvg = spindlePos;
  } else if (!strcmp(k, "eflt")) {
    // 1023 is the counter's own ceiling. The RPM this allows depends on PPR and gearing -
    // machine_config.h carries the formula.
    if (value < 1 || value > 1023) return "Filter must be 1-1023";
    encoderFilter = value;
    pcnt_set_filter_value(PCNT_UNIT_0, encoderFilter);
  } else if (!strcmp(k, "rtd")) {
    if (value <= 0) return "Must be above 0";
    retractDu = value;
  } else if (!strcmp(k, "p1u")) {
    if (value != 0) {
      const char* clash = auxConflictMessage(AUX_HANDWHEEL_1);
      if (clash != NULL) return clash;
    }
    pulse1Use = value != 0;
    applyAuxPins();
  } else if (!strcmp(k, "p1a")) {
    pulse1DrivesX = value != 0;
  } else if (!strcmp(k, "p2a")) {
    pulse2DrivesX = value != 0;
  } else if (!strcmp(k, "p1i")) {
    pulse1Invert = value != 0;
  } else if (!strcmp(k, "p2u")) {
    if (value != 0) {
      const char* clash = auxConflictMessage(AUX_HANDWHEEL_2);
      if (clash != NULL) return clash;
    }
    pulse2Use = value != 0;
    applyAuxPins();
  } else if (!strcmp(k, "p2i")) {
    pulse2Invert = value != 0;
  } else if (!strcmp(k, "hppr")) {
    if (value <= 0) return "Must be above 0";
    pulsePerRevolution = value;
  } else if (!strcmp(k, "pmw")) {
    if (value < 0) return "Must be 0 or above";
    pulseMinWidthUs = value;
  } else if (!strcmp(k, "phb")) {
    if (value < 0) return "Must be 0 or above";
    pulseHalfBacklash = value;
  } else if (!strcmp(k, "joy")) {
    if (value != 0) {
      const char* clash = auxConflictMessage(AUX_JOYSTICK);
      if (clash != NULL) return clash;
    }
    joystickUse = value != 0;
    applyAuxPins();
  } else if (!strcmp(k, "jdb")) {
    if (value < 0) return "Must be 0 or above";
    joystickDebounceMs = value;
  } else if (!strcmp(k, "xdd")) {
    xDiameterDisplay = value != 0;
  } else if (!strcmp(k, "stm")) {
    if (value <= 0) return "Must be above 0";
    stepTimeMs = value;
  } else if (!strcmp(k, "sdl")) {
    if (value < 0) return "Must be 0 or above";
    delayBetweenStepsMs = value;
  } else if (!strcmp(k, "idiv")) {
    // The ceiling is what the encoder can actually separate: dividing a revolution more finely
    // than the tolerance band is wide would put two marks inside one "on the mark" reading.
    if (value < 0 || value > 360) return "Must be 0 to 360";
    indexDivisions = value;
    indexLastOnMark = -1;
  } else if (!strcmp(k, "itol")) {
    if (value < 1 || value > 100) return "Must be 1 to 100";
    indexToleranceTenths = value;
    indexLastOnMark = -1;
  } else if (!strcmp(k, "cson")) {
    cssEnabled = value != 0;
  } else if (!strcmp(k, "cmat")) {
    if (value < 0 || value >= materialCount()) return "Not a material";
    cssMaterial = value;
  } else if (!strcmp(k, "ctol")) {
    cssCarbide = value != 0;
  } else if (!strcmp(k, "css")) {
    if (value < 0 || value > 2000) return "Must be 0 to 2000";
    surfaceSpeedMPerMin = value;
  } else if (!strcmp(k, "smax")) {
    if (value < 0 || value > 20000) return "Must be 0 to 20000";
    spindleMaxRpm = value;
  } else if (!strcmp(k, "wfen")) {
    wifiEnabled = value != 0;
  } else if (!strcmp(k, "wfpw")) {
    if (!calWifiPinValid(value)) return "PIN must be 8 digits";
    wifiPin = value;
    // A live access point keeps the password it was started with, so cycle it to pick up the new
    // one. Anyone currently connected is dropped, which is the point of changing it.
    wifiRestart = true;
  } else {
    return "Not a stored setting";
  }
  if (settingIsToggle(index)) {
    settingsPutGlobalBool(k, value != 0);
  } else {
    settingsPutGlobalLong(k, value);
  }
  // PPR, gearing and the divider all feed the counts-per-revolution figure.
  applyEncoderPpr();
  return NULL;
}

// Commits the numpad value to the currently selected settings menu item and persists it.
void commitSetting() {
  long raw = getNumpadResult();
  resetNumpad();
  // Distances are typed in microns or thou, everything else as a plain number.
  long value = raw;
  if (settingsUsesDu()) {
    value = numpadRawToDu(raw);
  } else if (settingUsesSpeed(settingsIndex)) {
    value = cssFromDisplay(raw, measure != MEASURE_METRIC);
  }
  const char* err = settingsWriteValue(settingsIndex, value);
  if (err != NULL) {
    splashError(err);
  }
}

// settingAxisLetter() spells the axis letters out so settings_table.h stays independent of the
// machine configuration. If the two ever disagree, stored keys stop matching what the axes are
// called, so tie them together at compile time.
static_assert(NAME_Z == 'Z', "NAME_Z must match settingAxisLetter");
static_assert(NAME_X == 'X', "NAME_X must match settingAxisLetter");
static_assert(NAME_A1 == 'C', "NAME_A1 must match settingAxisLetter");

// Storage key of a settings item as the serial and HTTP interfaces name it. Thin wrapper over the
// pure settingKeyName() so the host tests exercise the same key composition the firmware uses.
String settingsKeyName(int index) {
  char buf[16];
  settingKeyName(index, buf);
  return String(buf);
}

// Every stored setting as lines that can be pasted straight back in to restore it, followed by
// diagnostics. Machine config only - positions and stops are working state. One definition, so the
// serial dump and the file the web UI offers for download are the same backup byte for byte.
String settingsDumpText() {
  String out = "; NanoEls H" + String(HARDWARE_VERSION) + " V" + String(SOFTWARE_VERSION) + " settings\n";
  out += "; paste these lines back to restore, distances are in deci-microns\n";
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (settingIsAction(i)) continue;
    out += "$" + settingsKeyName(i) + "=" + String(settingsReadValue(i)) + "\n";
  }
  Preferences pref;
  pref.begin(PREF_NAMESPACE, true);
  out += "; nvs free entries " + String(pref.freeEntries()) + "\n";
  pref.end();
  out += "; nvs save batches since boot " + String(nvsSaveBatches) + "\n";
  out += "; uptime seconds " + String(millis() / 1000) + "\n";
  return out;
}

void dumpSettings() {
  Serial.print(settingsDumpText());
  Serial.println("ok");
}

// Whether it is safe to change a machine parameter right now. The settings menu can't be open
// while an axis moves - entering it clears the jog flags and it swallows the keypad. Serial and
// HTTP have no such protection, and changing screwPitch or motorSteps underneath a move in
// progress would corrupt it, so they have to ask.
bool machineIsBusy() {
  return isOn || z.movingManually || x.movingManually || stepperIsRunning(&z) || stepperIsRunning(&x);
}

// Applies one setting by its storage key, with the same validation the settings menu uses.
// Returns NULL on success or a message to show the user. The single entry point for every
// transport that isn't the keypad, so serial and the web UI cannot drift apart in what they accept.
const char* applySettingByKey(const String& key, long value) {
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (settingIsAction(i) || !key.equals(settingsKeyName(i))) continue;
    if (machineIsBusy()) {
      return "machine is busy";
    }
    // Hold the motion loop off while the value lands: settingsWriteValue() re-derives step counts
    // and commits to NVS, and loop() must not read a half-updated axis in between.
    if (xSemaphoreTake(motionMutex, 100) != pdTRUE) {
      return "machine is busy";
    }
    const char* err = settingsWriteValue(i, value);
    xSemaphoreGive(motionMutex);
    return err;
  }
  return "unknown setting";
}

// Applies one "key=value" settings command from the serial port.
void applySettingCommand(String command) {
  command.trim();
  if (command.length() == 0) {
    dumpSettings();
    return;
  }
  int eq = command.indexOf('=');
  if (eq < 1) {
    Serial.println("error: expected key=value");
    return;
  }
  String key = command.substring(0, eq);
  String value = command.substring(eq + 1);
  key.trim();
  value.trim();
  const char* err = applySettingByKey(key, value.toInt());
  Serial.println(err == NULL ? "ok" : "error: " + String(err));
}

// ---------------------------------------------------------------------------
// WiFi access point, configuration UI and over-the-air updates
// ---------------------------------------------------------------------------
//
// Nothing here runs unless the "Enabled" item in Settings > WiFi & updates is on. The page is
// generated from the same SETTINGS[] table the LCD menu walks, and every write goes through
// applySettingByKey(), so the two front ends cannot disagree about labels, ordering, units or
// what values are legal.

String otaError = ""; // Set by the upload handler, reported by the handler that answers the POST
bool otaWritten = false; // Whether a complete image actually landed, so an empty POST can't reboot
long otaKb = 0; // Kilobytes written so far, for the LCD
long otaLcdHash = LCD_HASH_INITIAL;

// Labels and error strings are ASCII and under our control, but one stray quote would produce a
// document the page silently fails to parse, which is a miserable thing to debug over WiFi.
String jsonEscape(const char* s) {
  String out = "";
  for (int i = 0; s[i] != 0; i++) {
    char c = s[i];
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (c >= 32) {
      out += c;
    }
  }
  return out;
}

// How the page should render an item's value. Mirrors the three ways the LCD menu treats them.
const char* settingKindName(int index) {
  if (settingIsToggle(index)) return "bool";
  if (settingIsList(index)) return "list";
  if (settingUsesSpeed(index)) return "speed";
  if (settingUsesDu(index)) return "du";
  return "num";
}

// Items a section shows. The calibration section holds only the action that opens a screen on the
// controller itself, which has no value to edit and no meaning over HTTP.
int settingSectionValueCount(int section) {
  int n = 0;
  int first = settingSectionFirst(section);
  int count = settingSectionCount(section);
  for (int i = first; i >= 0 && i < first + count; i++) {
    if (!settingIsAction(i)) n++;
  }
  return n;
}

void handleRoot() {
  webServer.send_P(200, "text/html", WEB_PAGE);
}

// Streamed rather than built as one String: 60-odd items is only a few KB, but chunking keeps peak
// allocation flat and matches how the page itself is served.
void handleSettingsJson() {
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.send(200, "application/json", "");
  webServer.sendContent("{\"version\":\"H" + String(HARDWARE_VERSION) + " V" + String(SOFTWARE_VERSION) +
      "\",\"metric\":" + String(measure == MEASURE_METRIC ? 1 : 0) +
      ",\"busy\":" + String(machineIsBusy() ? 1 : 0) + ",\"sections\":[");
  bool firstGroup = true;
  // Directory sections first, then one group per mode. The panel keeps the mode pages behind the
  // settings button because offering the threading page while set up to face is noise; a browser
  // has room to show the lot, and hiding them here would make the page the poorer front end.
  for (int g = 0; g < SECTION_COUNT + SMODE_COUNT; g++) {
    bool isMode = g >= SECTION_COUNT;
    int m = g - SECTION_COUNT;
    int first = isMode ? settingModeFirst(m) : settingSectionFirst(g);
    int count = isMode ? settingModeCount(m) : settingSectionCount(g);
    const char* name = isMode ? settingModeName(m) : SECTION_NAMES[g];
    if (first < 0 || count < 1 || name[0] == 0) continue;
    // A group of nothing but actions has nothing to edit over HTTP.
    int editable = 0;
    for (int i = first; i < first + count; i++) if (!settingIsAction(i)) editable++;
    if (editable < 1) continue;

    if (!firstGroup) webServer.sendContent(",");
    firstGroup = false;
    webServer.sendContent("{\"name\":\"" + jsonEscape(name) + "\",\"items\":[");
    bool firstItem = true;
    for (int i = first; i < first + count; i++) {
      if (settingIsAction(i)) continue;
      if (!firstItem) webServer.sendContent(",");
      firstItem = false;
      // The unit is resolved here rather than on the page: a distance follows the metric/inch
      // setting, and only the firmware knows which is selected.
      const char* unit = settingUsesDu(i)
          ? (measure == MEASURE_METRIC ? "mm" : "in")
          : SETTINGS[i].unit;
      String item = "{\"key\":\"" + settingsKeyName(i) +
          "\",\"label\":\"" + jsonEscape(SETTINGS[i].label) +
          "\",\"kind\":\"" + String(settingKindName(i)) +
          "\",\"unit\":\"" + (unit == 0 ? String("") : jsonEscape(unit)) +
          "\",\"value\":" + String(settingsReadValue(i));
      if (settingIsToggle(i) && SETTINGS[i].onLabel != 0) {
        item += ",\"on\":\"" + jsonEscape(SETTINGS[i].onLabel) +
            "\",\"off\":\"" + jsonEscape(SETTINGS[i].offLabel) + "\"";
      }
      // A list item carries its own labels, so the page can offer the same choices the panel does
      // without knowing what any of them mean.
      if (settingIsList(i)) {
        item += ",\"options\":[";
        for (int o = 0; o < settingListCount(i); o++) {
          if (o > 0) item += ",";
          item += "\"" + jsonEscape(settingListName(i, o)) + "\"";
        }
        item += "]";
      }
      webServer.sendContent(item + "}");
    }
    webServer.sendContent("]}");
  }
  webServer.sendContent("]}");
  webServer.sendContent("");
}

void handleStatusJson() {
  String out = "{\"on\":" + String(isOn ? 1 : 0);
  out += ",\"busy\":" + String(machineIsBusy() ? 1 : 0);
  out += ",\"mode\":\"" + String(modeName()) + "\"";
  out += ",\"rpm\":" + String(getApproxRpm());
  out += ",\"z\":" + String(getAxisPosDu(&z));
  out += ",\"x\":" + String(getAxisPosDu(&x) * (xDiameterDisplay ? 2 : 1));
  out += ",\"a1\":" + String(a1.active ? getAxisPosDu(&a1) : 0);
  // Surface speed at the tool, and the rpm the target asks for. Both fall out of numbers already
  // here, and a phone propped on the lathe is a better place to watch them than the LCD line they
  // share with everything else.
  {
    long diameterDu = abs(getAxisPosDu(&x)) * 2;
    out += ",\"diameterDu\":" + String(diameterDu);
    long wanted = cssActiveSpeed();
    out += ",\"surface\":" + String(cssActualMPerMin(diameterDu, getApproxRpm()));
    out += ",\"wanted\":" + String(wanted);
    out += ",\"material\":\"" + String(cssEnabled ? materialName(cssMaterial) : "off") + "\"";
    out += ",\"targetRpm\":" + String(wanted > 0
        ? cssCappedRpm(cssTargetRpm(diameterDu, wanted), spindleMaxRpm) : 0);
  }
  // Encoder signal quality. Live, and polled, because the windows worth catching happen mid-cut -
  // which is exactly when nobody is looking at the LCD.
  out += ",\"coherence\":" + String(encHealth.coherence);
  out += ",\"worstCoherence\":" + String(encHealth.worstCoherence);
  out += ",\"dirtyWindows\":" + String(encHealth.dirtyWindows);
  out += ",\"flips\":" + String(encoderReversals);
  out += ",\"coherenceFloor\":" + String(ENCODER_COHERENCE_FLOOR);
  out += ",\"a1active\":" + String(a1.active ? 1 : 0);
  out += ",\"dia\":" + String(xDiameterDisplay ? 1 : 0);
  out += ",\"measure\":" + String(measure);
  out += ",\"pitch\":" + String(dupr);
  out += ",\"uptime\":" + String(millis() / 1000);
  out += "}";
  webServer.send(200, "application/json", out);
}

// One axis worth of consequences. Everything here falls out of values already stored - none of it
// is settable, and that is the point: a setting's effect is often not visible in the setting.
String derivedAxisJson(Axis* a) {
  String out = "{\"name\":\"";
  out += a->name;
  out += "\",\"fitted\":" + String(a->active ? 1 : 0);
  out += ",\"resDu\":" + String(calStepResolutionDu(a->screwPitch, a->motorSteps), 4);
  out += ",\"stepsPerMm\":" + String(calStepsPerMm(a->screwPitch, a->motorSteps), 2);
  out += ",\"feedDuMin\":" + String(calMaxFeedDuPerMin(a->speedManualMove, a->screwPitch, a->motorSteps));
  out += ",\"stopDu\":" + String(calStopDistanceDu(a->speedManualMove, a->speedStart, a->acceleration,
                                                   a->screwPitch, a->motorSteps));
  out += ",\"backlashSteps\":" + String(a->backlashSteps);
  out += ",\"maxRpm\":" + String(calMaxRpmForPitch(a->speedManualMove, a->screwPitch, a->motorSteps, dupr));
  out += "}";
  return out;
}

void handleDerivedJson() {
  String out = "{\"pitch\":" + String(dupr);
  out += ",\"metric\":" + String(measure == MEASURE_METRIC ? 1 : 0);
  out += ",\"axes\":[" + derivedAxisJson(&z) + "," + derivedAxisJson(&x);
  if (a1.active) {
    out += "," + derivedAxisJson(&a1);
  }
  out += "],\"encoder\":{";
  out += "\"counts\":" + String(ENCODER_STEPS_INT);
  out += ",\"deg\":" + String(calAngularResolutionDeg(ENCODER_STEPS_INT), 4);
  out += ",\"maxEncoderRpm\":" + String(calMaxEncoderRpm(encoderPpr, encoderFilter));
  out += ",\"maxSpindleRpm\":" + String(calMaxSpindleRpm(encoderPpr, encoderFilter,
                                                         encoderSpindleTeeth, encoderPulleyTeeth));
  out += ",\"divider\":" + String(encoderDivider);
  out += ",\"symmetric\":" + String(encoderSymmetric ? 1 : 0);
  // The live signal-quality figures are in /api/status, not here: this endpoint is fetched once
  // when the page loads, and a coherence reading that never updates is worse than none - it would
  // read as a clean signal for the rest of the session however noisy the machine became.
  out += "}}";
  webServer.send(200, "application/json", out);
}

void handleSettingWrite() {
  if (!webServer.hasArg("key") || !webServer.hasArg("value")) {
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"expected key and value\"}");
    return;
  }
  const char* err = applySettingByKey(webServer.arg("key"), webServer.arg("value").toInt());
  if (err == NULL) {
    webServer.send(200, "application/json", "{\"ok\":true}");
  } else {
    // 409 rather than 400: "machine is busy" is a state problem, not a malformed request, and the
    // page retries on it rather than showing the value as rejected.
    webServer.send(409, "application/json", "{\"ok\":false,\"error\":\"" + jsonEscape(err) + "\"}");
  }
}

void handleDump() {
  webServer.sendHeader("Content-Disposition", "attachment; filename=nanoels-settings.txt");
  webServer.send(200, "text/plain", settingsDumpText());
}

// Answers the upload POST once the body has been consumed by handleUpdateUpload().
void handleUpdateDone() {
  if (otaError.length() > 0) {
    webServer.send(409, "application/json", "{\"ok\":false,\"error\":\"" + otaError + "\"}");
    otaInProgress = false;
    return;
  }
  if (!otaWritten) {
    // A POST that carried no file at all never reaches the upload handler, so without this it
    // would fall through to the restart below on an empty error string.
    webServer.send(400, "application/json", "{\"ok\":false,\"error\":\"no firmware in request\"}");
    otaInProgress = false;
    return;
  }
  webServer.send(200, "application/json", "{\"ok\":true}");
  webServer.client().stop();
  delay(500); // let the response actually leave before the radio goes down
  ESP.restart();
}

void handleUpdateUpload() {
  HTTPUpload& up = webServer.upload();
  if (up.status == UPLOAD_FILE_START) {
    otaError = "";
    otaWritten = false;
    otaKb = 0;
    if (machineIsBusy()) {
      otaError = "machine is busy";
      return;
    }
    // Set before the first write, not after. Writing to flash disables the instruction cache and
    // stalls BOTH cores for the duration, so no step pulse train can survive it - loop() has to be
    // out of the picture first. The stepper enable lines are deliberately left alone: dropping them
    // would release the holding torque and let a heavy cross slide drift under its own weight.
    otaInProgress = true;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      otaError = String(Update.errorString());
      otaInProgress = false;
    }
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (otaError.length() > 0) return;
    if (Update.write(up.buf, up.currentSize) != up.currentSize) {
      otaError = String(Update.errorString());
    }
    otaKb = up.totalSize / 1024;
  } else if (up.status == UPLOAD_FILE_END) {
    if (otaError.length() > 0) {
      Update.abort();
      otaInProgress = false;
      return;
    }
    if (!Update.end(true)) {
      otaError = String(Update.errorString());
      otaInProgress = false;
      return;
    }
    // Deliberately leaves otaInProgress set: the image is written but not yet booted, and if the
    // response never reaches the browser the safe outcome is a controller sitting still until it
    // is power-cycled, not one that resumes stepping.
    otaWritten = true;
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    otaInProgress = false;
    otaError = "upload aborted";
  }
}

void updateOtaDisplay() {
  long newHash = otaKb * 7L + (otaError.length() > 0 ? 3 : 1);
  if (newHash == otaLcdHash) {
    return;
  }
  otaLcdHash = newHash;
  ensureLcdCharset(false);
  lcd.setCursor(0, 0);
  printLcdSpaces(lcd.print("Updating firmware"));
  lcd.setCursor(0, 1);
  int n = lcd.print(otaKb);
  n += lcd.print(" KB written");
  printLcdSpaces(n);
  lcd.setCursor(0, 2);
  printLcdSpaces(lcd.print("Do not power off"));
  lcd.setCursor(0, 3);
  printLcdSpaces(0);
}

// WebServer keeps its routes in a list that begin() does not clear, so registering them again on
// every start would stack up duplicates each time the radio is cycled.
bool webRoutesRegistered = false;

void startWebServer() {
  WiFi.mode(WIFI_AP);
  // softAP() takes the password as text and silently opens the network if it is under 8
  // characters, which is why the PIN is validated as exactly 8 digits when it is stored.
  wifiUp = WiFi.softAP(WIFI_SSID, String(wifiPin).c_str(), WIFI_CHANNEL);
  if (!wifiUp) {
    splashError("WiFi failed to start");
    // Leave the setting on but stop retrying every pass through the task: a failing softAP() in a
    // tight loop would starve everything else on this core.
    wifiEnabled = false;
    return;
  }
  if (!webRoutesRegistered) {
    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/api/settings", HTTP_GET, handleSettingsJson);
    webServer.on("/api/status", HTTP_GET, handleStatusJson);
    webServer.on("/api/derived", HTTP_GET, handleDerivedJson);
    webServer.on("/api/setting", HTTP_POST, handleSettingWrite);
    webServer.on("/api/dump", HTTP_GET, handleDump);
    webServer.on("/update", HTTP_POST, handleUpdateDone, handleUpdateUpload);
    webRoutesRegistered = true;
  }
  webServer.begin();
  splash(WIFI_SSID);
}

void stopWebServer() {
  webServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  wifiUp = false;
  splash("WiFi off");
}

void taskWeb(void *param) {
  while (emergencyStop == ESTOP_NONE) {
    // The radio follows the setting rather than the boot state, so it can be switched on and off
    // at the panel without a power cycle. While it is off nothing here touches WiFi at all, so a
    // machine that never enables it pays only for an idle task.
    if (wifiRestart && wifiUp) {
      // The PIN changed underneath a running access point; it only takes effect on a fresh softAP.
      stopWebServer();
    }
    wifiRestart = false;
    if (wifiEnabled && !wifiUp) {
      startWebServer();
    } else if (!wifiEnabled && wifiUp) {
      stopWebServer();
    }
    if (wifiUp) {
      webServer.handleClient();
    }
    taskYIELD();
  }
  vTaskDelete(NULL);
}

// Moves to another item in the open section, wrapping inside it. delta of +-1 steps one item,
// larger jumps a page.
// First index and length of whatever list the settings screen is showing. The mode page is not a
// section - it is the current mode's run of the table - but it is navigated identically, so
// everything that walks a list goes through these two rather than knowing which it has.
int settingsListFirst() {
  return settingsSection == SEC_MODE ? settingModeFirst(settingModeOf(mode))
                                     : settingSectionFirst(settingsSection);
}

int settingsListCount() {
  return settingsSection == SEC_MODE ? settingModeCount(settingModeOf(mode))
                                     : settingSectionCount(settingsSection);
}

const char* settingsListName() {
  return settingsSection == SEC_MODE ? settingModeName(settingModeOf(mode))
                                     : SECTION_NAMES[settingsSection];
}

void settingsMove(int delta) {
  int first = settingsListFirst();
  int count = settingsListCount();
  if (first < 0 || count < 1) return;
  int at = settingsIndex - first;
  at = ((at + delta) % count + count) % count;
  settingsIndex = first + at;
  resetNumpad();
}

void processSettingsKeypress(int keyCode) {
  if (keyCode == B_SETTINGS) {
    exitSettingsScreens();
    return;
  }
  if (inThreadPicker) {
    int digit = keyCodeToDigit(keyCode);
    if (digit >= 1 && digit <= 6) {
      for (int i = 0; i < THREAD_PRESETS_COUNT; i++) {
        if (strstr(THREAD_PRESETS[i].name, THREAD_CATEGORY_MARKERS[digit - 1]) != NULL) {
          threadPickerIndex = i;
          break;
        }
      }
    } else if (keyCode == B_UP) {
      threadPickerIndex = (threadPickerIndex + THREAD_PRESETS_COUNT - 1) % THREAD_PRESETS_COUNT;
    } else if (keyCode == B_DOWN) {
      threadPickerIndex = (threadPickerIndex + 1) % THREAD_PRESETS_COUNT;
    } else if (keyCode == B_LEFT) {
      threadPickerIndex = (threadPickerIndex + THREAD_PRESETS_COUNT - 10) % THREAD_PRESETS_COUNT;
    } else if (keyCode == B_RIGHT) {
      threadPickerIndex = (threadPickerIndex + 10) % THREAD_PRESETS_COUNT;
    } else if (keyCode == B_ON) {
      setMeasure(THREAD_PRESETS[threadPickerIndex].measure);
      setDupr(THREAD_PRESETS[threadPickerIndex].dupr);
      setStarts(1);
      exitSettingsScreens();
    } else if (keyCode == B_OFF) {
      exitSettingsScreens();
    }
    return;
  }

  // Directory of sections.
  if (settingsSection < 0) {
    if (keyCode == B_UP) {
      settingsDirIndex = (settingsDirIndex + SECTION_COUNT - 1) % SECTION_COUNT;
    } else if (keyCode == B_DOWN) {
      settingsDirIndex = (settingsDirIndex + 1) % SECTION_COUNT;
    } else if (keyCode == B_LEFT) {
      settingsDirIndex = (settingsDirIndex + SECTION_COUNT - 3) % SECTION_COUNT;
    } else if (keyCode == B_RIGHT) {
      settingsDirIndex = (settingsDirIndex + 3) % SECTION_COUNT;
    } else if (keyCode == B_ON) {
      int first = settingSectionFirst(settingsDirIndex);
      if (first < 0) {
        beep();
        return;
      }
      settingsSection = settingsDirIndex;
      settingsIndex = first;
      resetNumpad();
    } else if (keyCode == B_OFF) {
      exitSettingsScreens();
    }
    return;
  }

  // Inside a section, or on the mode page.
  if (keyCode == B_OFF) {
    if (settingsSection == SEC_MODE) {
      // The mode page is not in the directory, so there is nothing above it to go back to.
      exitSettingsScreens();
    } else {
      // Back to the directory rather than straight out, so a wrong turn costs one keypress.
      settingsSection = -1;
      resetNumpad();
    }
    return;
  }
  int digit = keyCodeToDigit(keyCode);
  // Only items that take a typed value collect digits. On a toggle, a list or an action the
  // numpad would fill up and the bottom line would offer to use a number the item cannot hold,
  // while ON - which those items bind to something else - would ignore it and do that instead.
  bool typed = !settingsIsToggle() && !settingIsList(settingsIndex) && !settingIsAction(settingsIndex);
  if (digit >= 0) {
    if (typed) numpadPress(digit);
  } else if (keyCode == B_BACKSPACE) {
    if (typed) numpadBackspace();
  } else if (keyCode == B_UP) {
    settingsMove(-1);
  } else if (keyCode == B_DOWN) {
    settingsMove(1);
  } else if (keyCode == B_LEFT) {
    settingsMove(-3);
  } else if (keyCode == B_RIGHT) {
    settingsMove(3);
  } else if (settingIsAction(settingsIndex)) {
    if (keyCode == B_ON) {
      if (isOn) {
        splashError("Turn off first");
      } else if (!strcmp(SETTINGS[settingsIndex].prefKey, "thr")) {
        enterSettingsScreen(true); // the thread database
      } else {
        enterCalScreen();
      }
    }
  } else if (settingsIsToggle()) {
    if (keyCode == B_ON || keyCode == B_PLUS || keyCode == B_MINUS) {
      const char* err = settingsWriteValue(settingsIndex, settingsReadValue(settingsIndex) == 0 ? 1 : 0);
      if (err != NULL) splashError(err);
    }
  } else if (settingIsList(settingsIndex)) {
    // Same keys a toggle uses, since a list is a toggle with more than two positions. The arrows
    // are not free here - they move between items and jump pages.
    if (keyCode == B_ON || keyCode == B_PLUS || keyCode == B_MINUS) {
      long n = settingListCount(settingsIndex);
      long step = keyCode == B_MINUS ? n - 1 : 1; // wrap backwards without going negative
      const char* err = settingsWriteValue(settingsIndex, (settingsReadValue(settingsIndex) + step) % n);
      if (err != NULL) splashError(err);
    }
  } else if (keyCode == B_ON && numpadIndex > 0) {
    commitSetting();
  } else if (keyCode == B_ON) {
    beep();
  }
}

long getSettingsValueHash() {
  if (settingsSection < 0) return settingsDirIndex * 31L + 7L;
  if (settingsIndex < 0 || settingsIndex >= SETTINGS_COUNT) return settingsSection * 7919L;
  // wifiUp is in here because the WiFi section prints the address beside the value, and the access
  // point can finish coming up after the screen has already been drawn.
  return settingsReadValue(settingsIndex) * 3L + settingsIndex * 101L + (wifiUp ? 977L : 0L);
}

void updateSettingsDisplay() {
  long newHash = (inThreadPicker
      ? (2000000 + threadPickerIndex)
      : (settingsSection * 1000003L + numpadIndex * 7 + getNumpadResult() * 13 + getSettingsValueHash() + measure))
      + (splashActive() ? splashId * 7919 : 0);
  if (newHash == settingsLcdHash) {
    return;
  }
  settingsLcdHash = newHash;
  int charIndex = 0;
  if (inThreadPicker) {
    lcd.setCursor(0, 0);
    charIndex = lcd.print("Thread ");
    charIndex += lcd.print(threadPickerIndex + 1);
    charIndex += lcd.print(" of ");
    charIndex += lcd.print(THREAD_PRESETS_COUNT);
    printLcdSpaces(charIndex);
    lcd.setCursor(0, 1);
    charIndex = lcd.print(THREAD_PRESETS[threadPickerIndex].name);
    printLcdSpaces(charIndex);
    lcd.setCursor(0, 2);
    charIndex = lcd.print("Pitch ");
    if (THREAD_PRESETS[threadPickerIndex].measure == MEASURE_METRIC) {
      charIndex += printNoTrailing0(THREAD_PRESETS[threadPickerIndex].dupr / 10000.0);
      charIndex += lcd.write(customCharMmCode);
    } else {
      charIndex += lcd.print(int(round(254000.0 / THREAD_PRESETS[threadPickerIndex].dupr)));
      charIndex += lcd.print("tpi");
    }
    // This screen is where the settings button lands in thread mode, so it is where someone
    // looking for the settings menu ends up. Say how to get there. Only when the pitch leaves
    // room - a five-decimal metric pitch does not, and a wrapped line would overwrite the hints
    // below it.
    if (charIndex + 10 <= 20) {
      while (charIndex < 10) charIndex += lcd.print(" ");
      charIndex += lcd.print("hold=menu");
    }
    printLcdSpaces(charIndex);
    lcd.setCursor(0, 3);
    charIndex = lcd.print("ON use, 1-6 groups");
    printLcdSpaces(charIndex);
    return;
  }

  // Directory: three sections at a time with a cursor, scrolled to keep the selection visible.
  if (settingsSection < 0) {
    lcd.setCursor(0, 0);
    charIndex = lcd.print("Settings ");
    charIndex += lcd.print(settingsDirIndex + 1);
    charIndex += lcd.print("/");
    charIndex += lcd.print((int) SECTION_COUNT);
    printLcdSpaces(charIndex);
    int top = settingsDirIndex - 1;
    if (top < 0) top = 0;
    if (top > SECTION_COUNT - 3) top = SECTION_COUNT - 3;
    if (top < 0) top = 0;
    for (int row = 0; row < 3; row++) {
      lcd.setCursor(0, row + 1);
      int at = top + row;
      charIndex = 0;
      if (at < SECTION_COUNT) {
        charIndex = lcd.print(at == settingsDirIndex ? ">" : " ");
        charIndex += lcd.print(SECTION_NAMES[at]);
      }
      printLcdSpaces(charIndex);
    }
    return;
  }

  // Inside a section, or on a mode's page: one item at a time, since a value has to be typed in.
  int first = settingsListFirst();
  int count = settingsListCount();
  lcd.setCursor(0, 0);
  charIndex = lcd.print(settingsListName());
  charIndex += lcd.print(" ");
  charIndex += lcd.print(settingsIndex - first + 1);
  charIndex += lcd.print("/");
  charIndex += lcd.print(count);
  printLcdSpaces(charIndex);

  lcd.setCursor(0, 1);
  charIndex = lcd.print(SETTINGS[settingsIndex].label);
  printLcdSpaces(charIndex);

  lcd.setCursor(0, 2);
  charIndex = 0;
  if (!settingIsAction(settingsIndex)) {
    charIndex = lcd.print("Now ");
    long value = settingsReadValue(settingsIndex);
    if (settingIsToggle(settingsIndex)) {
      const char* on = SETTINGS[settingsIndex].onLabel;
      const char* off = SETTINGS[settingsIndex].offLabel;
      charIndex += lcd.print(value ? (on ? on : "on") : (off ? off : "off"));
    } else if (settingIsList(settingsIndex)) {
      charIndex += lcd.print(settingListName(settingsIndex, value));
    } else if (settingUsesSpeed(settingsIndex)) {
      bool inchMode = measure != MEASURE_METRIC;
      charIndex += lcd.print(cssToDisplay(value, inchMode));
      charIndex += lcd.print(" ");
      charIndex += lcd.print(cssUnitName(inchMode));
    } else if (settingUsesDu(settingsIndex)) {
      // A distance has no fixed unit - it is typed and shown in whichever system is selected.
      charIndex += printDeciMicrons(value, 5);
      charIndex += lcd.print(measure == MEASURE_METRIC ? " mm" : "\"");
    } else {
      charIndex += lcd.print(value);
      const char* unit = SETTINGS[settingsIndex].unit;
      // Only if it fits: a value wide enough to crowd the unit out would otherwise wrap onto the
      // next line and overwrite it.
      if (unit != 0 && charIndex + 1 + (int) strlen(unit) <= 20) {
        charIndex += lcd.print(" ");
        charIndex += lcd.print(unit);
      }
    }
  } else {
    charIndex = lcd.print(CAL_ROUTINES_COUNT);
    charIndex += lcd.print(" routines");
  }
  // The whole point of the WiFi section is to get you to a browser, so the address goes where you
  // are already looking rather than in a manual. Only beside the on/off item: "Now yes" padded to
  // column 9 leaves exactly the 11 columns an address needs, whereas "Now 13572468" beside the PIN
  // would leave three and wrap onto the line below.
  if (SETTINGS[settingsIndex].section == SEC_WIFI && wifiUp && settingIsToggle(settingsIndex)) {
    String ip = WiFi.softAPIP().toString();
    if (charIndex + 1 + (int) ip.length() <= 20) {
      while (charIndex < 20 - (int) ip.length()) {
        charIndex += lcd.print(" ");
      }
      charIndex += lcd.print(ip);
    }
  }
  printLcdSpaces(charIndex);

  lcd.setCursor(0, 3);
  if (splashActive()) {
    charIndex = lcd.print(splashBuf);
  } else if (settingIsAction(settingsIndex)) {
    charIndex = lcd.print("ON to open");
  } else if (settingsIsToggle()) {
    charIndex = lcd.print("ON to toggle");
  } else if (settingIsList(settingsIndex)) {
    charIndex = lcd.print("+/- to change");
  } else if (numpadIndex > 0) {
    charIndex = lcd.print("Use ");
    if (settingsUsesDu()) {
      charIndex += printDeciMicrons(numpadRawToDu(getNumpadResult()), 5);
      charIndex += lcd.print(measure == MEASURE_METRIC ? " mm" : "\"");
    } else if (settingUsesSpeed(settingsIndex)) {
      // Echoed as typed rather than converted: the figure being confirmed has to be the one that
      // was entered, or the confirmation looks like a rejection.
      charIndex += lcd.print(getNumpadResult());
      charIndex += lcd.print(" ");
      charIndex += lcd.print(cssUnitName(measure != MEASURE_METRIC));
    } else {
      charIndex += lcd.print(getNumpadResult());
      // Confirming a bare number invites entering it in the wrong unit, so name it here too.
      const char* unit = SETTINGS[settingsIndex].unit;
      if (unit != 0 && charIndex + 2 + (int) strlen(unit) <= 20) {
        charIndex += lcd.print(" ");
        charIndex += lcd.print(unit);
      }
    }
    charIndex += lcd.print("?");
  } else if (settingsUsesDu()) {
    charIndex = lcd.print(measure == MEASURE_INCH ? "Type thou, then ON" : "Type microns, ON");
  } else if (settingUsesSpeed(settingsIndex)) {
    charIndex = lcd.print(measure == MEASURE_METRIC ? "Type m/min, then ON" : "Type ft/min, ON");
  } else {
    charIndex = lcd.print("Type number, ON");
  }
  printLcdSpaces(charIndex);
}

int joystickPin(int i) {
  if (i < 4) return JOYSTICK_DIR_PIN_KEYS[i][0];
  return i == 4 ? JOYSTICK_MOVE_PIN : JOYSTICK_STEP_PIN;
}

// Polls the joystick and returns a keypad-style event (keyCode with bit 7 set on press) or 0 if nothing changed.
// Stick deflection alone produces no events: arrow key presses are emitted while the move button is held and the
// stick is deflected, releases when either ends. The step button maps directly to the keypad step button.
int getJoystickEvent() {
  unsigned long now = millis();
  for (int i = 0; i < 6; i++) {
    bool active = DREAD(joystickPin(i)) == LOW;
    if (active != joystickPinActive[i] && now - joystickChangeMs[i] >= joystickDebounceMs) {
      joystickPinActive[i] = active;
      joystickChangeMs[i] = now;
      if (i == 5) {
        int event = B_STEP;
        bitWrite(event, 7, active ? 1 : 0);
        return event;
      }
    }
  }
  for (int i = 0; i < 4; i++) {
    bool pressed = joystickPinActive[i] && joystickPinActive[4];
    if (pressed != joystickDirPressed[i]) {
      joystickDirPressed[i] = pressed;
      int event = JOYSTICK_DIR_PIN_KEYS[i][1];
      bitWrite(event, 7, pressed ? 1 : 0);
      return event;
    }
  }
  return 0;
}

// Axis that the running calibration routine refers to, NULL for the encoder and diagnostic ones.
Axis* calAxis() {
  if (calRoutine < 0 || calRoutine > CAL_SPEED_X) return NULL;
  return calRoutine % 2 == 0 ? &z : &x;
}

long axisDuToSteps(Axis* a, long du) {
  return calDuToSteps(a->screwPitch, a->motorSteps, du);
}

// Same mutex dance commitSetting() uses when re-deriving values behind the motion tasks' back.
void calRecompute(Axis* a) {
  if (xSemaphoreTake(a->mutex, 100) == pdTRUE) {
    recomputeAxisDerived(a);
    xSemaphoreGive(a->mutex);
  } else {
    recomputeAxisDerived(a);
  }
}

// Commands a validated relative move using the same path as manual numpad moves. Unlike those,
// an out-of-range move is refused rather than clamped: a silently shortened test move would
// corrupt the measurement it feeds.
bool calRequestMove(Axis* a, long deltaSteps) {
  if (isOn || a->movingManually) {
    beep();
    return false;
  }
  // A commanded move is still outstanding. Issuing another now would retarget from wherever the
  // axis has got to, compounding the two into a longer move than either - which is exactly what
  // an impatient second press of ON or plus would otherwise do.
  if (a->pendingPos != 0) {
    beep();
    return false;
  }
  long target = a->pos + deltaSteps;
  if (target > a->leftStop || target < a->rightStop) {
    splashError("Limited by stop");
    return false;
  }
  if (abs(deltaSteps) > a->estopSteps) {
    splashError("Too far, ignored");
    return false;
  }
  a->speedMax = a->speedManualMove;
  stepToFinal(a, target);
  return true;
}

void calClearJogFlags() {
  buttonLeftPressed = false;
  buttonRightPressed = false;
  buttonUpPressed = false;
  buttonDownPressed = false;
}

// Undoes everything the running routine changed for the duration of the measurement. Must be
// reachable from every exit path, aborts included, or backlash and speed stay clobbered.
void calStopRoutine() {
  Axis* a = calAxis();
  if (a != NULL) {
    if ((calRoutine == CAL_SPEED_Z || calRoutine == CAL_SPEED_X) && calSaved > 0) {
      a->speedManualMove = calSaved;
    }
    a->speedMax = LONG_MAX;
    // Backlash compensation is suppressed while it is being measured; always restore it.
    calRecompute(a);
  }
  calClearJogFlags();
  calRoutine = -1;
  calStep = 0;
  calValue = 0;
  calPrev = 0;
  calSaved = 0;
  calRefPos = 0;
  calDirMoved = false;
  resetNumpad();
}

void enterCalScreen() {
  inSettings = false;
  inThreadPicker = false;
  inCal = true;
  calRoutine = -1;
  calListIndex = 0;
  calStep = 0;
  calLastKeyCode = -1;
  resetNumpad();
  inNumpad = false;
  calLcdHash = LCD_HASH_INITIAL;
  calClearJogFlags();
  buttonGearsPressed = false;
  buttonTurnPressed = false;
  buttonAPressed = false;
}

// Calibration is opened from the settings menu, so leaving it returns there.
void exitCalScreen() {
  calStopRoutine();
  inCal = false;
  inSettings = true;
  settingsLcdHash = LCD_HASH_INITIAL;
  resetNumpad();
  inNumpad = false;
}

void calStartRoutine(int r) {
  calRoutine = r;
  calStep = 0;
  calValue = 0;
  calPrev = 0;
  calSaved = 0;
  calRefPos = 0;
  calDirMoved = false;
  calParamIndex = 2; // 10mm test move, or 10 revolutions
  calRefSpindle = spindlePos;
  resetNumpad();
  Axis* a = calAxis();
  if (r == CAL_BACKLASH_Z || r == CAL_BACKLASH_X) {
    // Measuring backlash needs the compensation out of the way or it just measures itself.
    a->backlashSteps = 0;
  } else if (r == CAL_SPEED_Z || r == CAL_SPEED_X) {
    calSaved = a->speedManualMove;
    calValue = a->speedManualMove;
  } else if (r == CAL_ENC_SIGNAL) {
    encoderReversals = 0;
    encHealthReset(&encHealth, micros());
  }
}

void calOnPress() {
  Axis* a = calAxis();
  switch (calRoutine) {
    case CAL_PITCH_Z:
    case CAL_PITCH_X:
      if (calStep == 0) {
        // Pre-load in the same direction as the test move so the slack is already taken up
        // when the indicator is zeroed. Without this the test move would silently include
        // however wrong the current backlash setting is - see calStep 1.
        if (calRequestMove(a, axisDuToSteps(a, CAL_PITCH_PRELOAD_DU))) calStep = 1;
      } else if (calStep == 1) {
        // Same direction as the pre-load, so no backlash compensation is applied at all and
        // the measurement does not depend on backlashDu being correct yet.
        if (calRequestMove(a, axisDuToSteps(a, CAL_TEST_DU[calParamIndex]))) calStep = 2;
      } else if (calStep == 2) {
        if (numpadIndex == 0) {
          beep();
          break;
        }
        long raw = getNumpadResult();
        long actualDu = numpadRawToDu(raw);
        long nominalDu = CAL_TEST_DU[calParamIndex];
        resetNumpad();
        if (actualDu <= 0) {
          splashError("Must be above 0");
          break;
        }
        long newPitch = calCorrectedPitch(a->screwPitch, actualDu, nominalDu);
        if (!calPitchPlausible(a->screwPitch, newPitch)) {
          splashError("Off by >20%, check");
          break;
        }
        calValue = newPitch;
        calStep = 3;
      } else {
        a->screwPitch = calValue;
        settingsPutLong(a, "scr", calValue);
        calRecompute(a);
        splash("Screw pitch saved");
        calStopRoutine();
      }
      break;

    case CAL_BACKLASH_Z:
    case CAL_BACKLASH_X:
      if (calStep == 0) {
        if (calRequestMove(a, axisDuToSteps(a, CAL_BACKLASH_LOAD_DU))) calStep = 1;
      } else if (calStep == 1) {
        calRefPos = a->pos;
        calStep = 2;
      } else if (calStep == 2) {
        calValue = stepsToDu(a, abs(calRefPos - a->pos));
        calStep = 3;
      } else {
        a->backlashDu = calValue;
        settingsPutLong(a, "bla", calValue);
        calRecompute(a);
        splash("Backlash saved");
        calStopRoutine();
      }
      break;

    case CAL_DIR_Z:
    case CAL_DIR_X:
      if (!calDirMoved) {
        if (calRequestMove(a, axisDuToSteps(a, CAL_DIR_MOVE_DU))) calDirMoved = true;
      } else {
        splash("Direction kept");
        calStopRoutine();
      }
      break;

    case CAL_TRAVEL_Z:
    case CAL_TRAVEL_X:
      if (calStep == 0) {
        calRefPos = a->pos;
        calStep = 1;
      } else if (calStep == 1) {
        long mm = calTravelMm(stepsToDu(a, abs(a->pos - calRefPos)));
        if (mm < 1) {
          splashError("Span too small");
          break;
        }
        calValue = mm;
        calStep = 2;
        // Leaving the jog steps while an arrow is still held would strand its flag set and
        // leave the axis running, since the release no longer reaches the jog passthrough.
        calClearJogFlags();
      } else {
        a->maxTravelMm = calValue;
        settingsPutLong(a, "mtr", calValue);
        calRecompute(a);
        splash("Max travel saved");
        calStopRoutine();
      }
      break;

    case CAL_SPEED_Z:
    case CAL_SPEED_X:
      if (calStep == 0) {
        calValue = a->speedManualMove;
        calStep = 1;
      } else if (calStep == 1) {
        // One stroke at the current trial speed, alternating direction, then step it up 10%.
        a->speedManualMove = calValue;
        calRecompute(a);
        long steps = axisDuToSteps(a, CAL_SPEED_STROKE_DU);
        if (calRequestMove(a, calRefPos == 0 ? steps : -steps)) {
          calRefPos = calRefPos == 0 ? 1 : 0;
          calPrev = calValue;
          calValue = calSpeedNext(calValue);
        }
      } else {
        a->speedManualMove = calValue;
        settingsPutLong(a, "spd", calValue);
        calRecompute(a);
        // Keep the new value: tell calStopRoutine there is nothing to restore.
        calSaved = 0;
        splash("Max speed saved");
        calStopRoutine();
      }
      break;

    case CAL_ENC_PPR:
      if (calStep == 0) {
        calRefSpindle = spindlePos;
        calStep = 1;
      } else if (calStep == 1) {
        // You turn the SPINDLE, but PPR is a property of the encoder, so undo the gearing and
        // the divider to get back to raw encoder counts before deriving it.
        long rawCounts = (spindlePos - calRefSpindle) * encoderDivider * encoderPulleyTeeth;
        long ppr = calPprFromCounts(rawCounts, CAL_REVS[calParamIndex] * encoderSpindleTeeth);
        if (!calPprValid(ppr)) {
          splashError("PPR must be 24-10000");
          break;
        }
        calValue = calSnapPpr(ppr);
        calStep = 2;
      } else {
        encoderPpr = calValue;
        applyEncoderPpr();
        Preferences pref;
        pref.begin(PREF_NAMESPACE);
        pref.putInt(PREF_ENCODER_PPR, encoderPpr);
        pref.end();
        splash("Encoder PPR saved");
        calStopRoutine();
      }
      break;

    case CAL_ENC_DIR:
      splash(encoderInvert ? "Encoder inverted" : "Encoder normal");
      calStopRoutine();
      break;
  }
}

// Repeats the move belonging to the current step without advancing. Bound to the plus key, the
// mirror of minus: minus goes back a step, plus runs where you are again. For taking up more
// slack, watching a direction check a second time, or hearing the same speed again before
// deciding it stalled.
void calRepeatMove() {
  Axis* a = calAxis();
  switch (calRoutine) {
    case CAL_PITCH_Z:
    case CAL_PITCH_X:
      // Only before the test move. Repeating that one would add a second test distance while
      // the indicator still reads from the first, so the total would no longer match the
      // nominal it gets compared against. Step back with minus and re-zero instead.
      if (calStep <= 1) {
        calRequestMove(a, axisDuToSteps(a, CAL_PITCH_PRELOAD_DU));
        return;
      }
      break;

    case CAL_BACKLASH_Z:
    case CAL_BACKLASH_X:
      // Same reasoning: only while still loading the axis, before the indicator is zeroed.
      if (calStep <= 1) {
        calRequestMove(a, axisDuToSteps(a, CAL_BACKLASH_LOAD_DU));
        return;
      }
      break;

    case CAL_DIR_Z:
    case CAL_DIR_X:
      if (calRequestMove(a, axisDuToSteps(a, CAL_DIR_MOVE_DU))) {
        calDirMoved = true;
      }
      return;

    case CAL_SPEED_Z:
    case CAL_SPEED_X:
      if (calStep == 1) {
        // The same speed again rather than stepping up 10%. calPrev has to follow, or calling
        // the stall afterwards would back off from the speed before this one.
        a->speedManualMove = calValue;
        calRecompute(a);
        long steps = axisDuToSteps(a, CAL_SPEED_STROKE_DU);
        if (calRequestMove(a, calRefPos == 0 ? steps : -steps)) {
          calRefPos = calRefPos == 0 ? 1 : 0;
          calPrev = calValue;
        }
        return;
      }
      break;
  }
  beep();
}

// Steps a routine back so a wrong entry can be redone without aborting and repeating the
// physical moves. Bound to the minus key, which is the only one free at every step: the arrows
// are taken by jogging and parameter adjustment, and backspace edits the numpad.
//
// Stepping back never un-does a move, it only returns to the earlier prompt. That is safe here
// because every repeatable move in these routines goes the same direction as the one before it,
// so running one again leaves the axis loaded exactly as it was.
void calBack() {
  switch (calRoutine) {
    case CAL_DIR_Z:
    case CAL_DIR_X:
      // Run the test move again if you missed which way it went.
      if (calDirMoved) {
        calDirMoved = false;
        return;
      }
      break;

    case CAL_SPEED_Z:
    case CAL_SPEED_X:
      // Only from the confirm screen. Going back further would re-baseline the ramp from a
      // speed the routine has already been changing, and lose the original in calSaved.
      if (calStep == 2) {
        calValue = calPrev > 0 ? calPrev : calSaved;
        calStep = 1;
        return;
      }
      break;

    case CAL_PITCH_Z:
    case CAL_PITCH_X:
    case CAL_BACKLASH_Z:
    case CAL_BACKLASH_X:
    case CAL_TRAVEL_Z:
    case CAL_TRAVEL_X:
    case CAL_ENC_PPR:
      if (calStep > 0) {
        calValue = 0;
        calStep--;
        resetNumpad();
        // Stepping back into a jogging step must not inherit a stale held-arrow flag.
        calClearJogFlags();
        return;
      }
      break;
  }
  beep();
}

// Left/right while a routine is running. dir is -1 for left, +1 for right.
void calAdjust(int dir) {
  Axis* a = calAxis();
  switch (calRoutine) {
    case CAL_PITCH_Z:
    case CAL_PITCH_X:
      if (calStep == 0) calParamIndex = (calParamIndex + CAL_TEST_DU_COUNT + dir) % CAL_TEST_DU_COUNT;
      break;

    case CAL_BACKLASH_Z:
    case CAL_BACKLASH_X:
      // Nudge back against the loaded direction one moveStep at a time. Right nudges forward again
      // so an overshoot can be walked back instead of restarting the routine; the measurement is
      // the distance from calRefPos either way, so it stays correct.
      if (calStep == 2) calRequestMove(a, axisDuToSteps(a, moveStep) * dir);
      break;

    case CAL_DIR_Z:
    case CAL_DIR_X:
      if (calDirMoved && dir < 0) {
        a->invertStepper = !a->invertStepper;
        a->directionInitialized = false;
        settingsPutBool(a, "inv", a->invertStepper);
        splash("Direction flipped");
        calDirMoved = false; // move again so the user can confirm the new direction
      }
      break;

    case CAL_SPEED_Z:
    case CAL_SPEED_X:
      if (calStep == 1 && dir < 0 && calPrev > 0) {
        calValue = calSpeedBackoff(calPrev, a->speedStart);
        calStep = 2;
      }
      break;

    case CAL_ENC_PPR:
      if (calStep == 0) calParamIndex = (calParamIndex + CAL_REVS_COUNT + dir) % CAL_REVS_COUNT;
      break;

    case CAL_ENC_DIR:
      if (dir < 0) {
        encoderInvert = !encoderInvert;
        settingsPutGlobalBool(PREF_ENCODER_INVERT, encoderInvert);
        calRefSpindle = spindlePos;
      }
      break;

    case CAL_ENC_SIGNAL:
      if (dir < 0) {
        encoderReversals = 0;
        encHealthReset(&encHealth, micros());
        calRefSpindle = spindlePos;
      }
      break;
  }
}

void processCalKeypress(int keyCode, bool isPress) {
  // The travel limit routines are jogged by hand, so their arrow keys drive the normal manual
  // move flags that taskMoveZ / taskMoveX already act on. This is why we need releases here.
  if ((calRoutine == CAL_TRAVEL_Z || calRoutine == CAL_TRAVEL_X) && calStep < 2) {
    if (calRoutine == CAL_TRAVEL_Z && (keyCode == B_LEFT || keyCode == B_RIGHT)) {
      if (keyCode == B_LEFT) buttonLeftPressed = isPress;
      else buttonRightPressed = isPress;
      return;
    }
    if (calRoutine == CAL_TRAVEL_X && (keyCode == B_UP || keyCode == B_DOWN)) {
      if (keyCode == B_UP) buttonUpPressed = isPress;
      else buttonDownPressed = isPress;
      return;
    }
  }

  // The input tester swallows every key on purpose, so it needs a hold-to-exit escape.
  if (calRoutine == CAL_INPUTS) {
    calLastKeyCode = keyCode;
    calLastKeyPress = isPress;
    if (keyCode == B_OFF) {
      if (isPress) calOffPressMs = millis();
      else if (millis() - calOffPressMs >= 1000) calStopRoutine();
    }
    return;
  }

  if (!isPress) return;

  if (keyCode == B_OFF) {
    calRoutine < 0 ? exitCalScreen() : calStopRoutine();
    return;
  }

  if (calRoutine < 0) {
    if (keyCode == B_UP) calListIndex = (calListIndex + CAL_ROUTINES_COUNT - 1) % CAL_ROUTINES_COUNT;
    else if (keyCode == B_DOWN) calListIndex = (calListIndex + 1) % CAL_ROUTINES_COUNT;
    else if (keyCode == B_LEFT) calListIndex = (calListIndex + CAL_ROUTINES_COUNT - 4) % CAL_ROUTINES_COUNT;
    else if (keyCode == B_RIGHT) calListIndex = (calListIndex + 4) % CAL_ROUTINES_COUNT;
    else if (keyCode == B_ON) calStartRoutine(calListIndex);
    return;
  }

  int digit = keyCodeToDigit(keyCode);
  if (digit >= 0) numpadPress(digit);
  else if (keyCode == B_BACKSPACE) numpadBackspace();
  else if (keyCode == B_LEFT) calAdjust(-1);
  else if (keyCode == B_RIGHT) calAdjust(1);
  else if (keyCode == B_MINUS) calBack();
  else if (keyCode == B_PLUS) calRepeatMove();
  // Same 1mm / 0.1mm / 0.01mm ladder the main screen uses, so the backlash nudge and the travel
  // limit jog can both be taken down to 0.01mm without the user leaving the routine.
  else if (keyCode == B_STEP) buttonMoveStepPress();
  else if (keyCode == B_ON) calOnPress();
}

// Current value of the machine parameter a routine calibrates, shown under it in the list.
int calPrintNow(int r) {
  if (r == CAL_ENC_SIGNAL || r == CAL_INPUTS) {
    return lcd.print("Diagnostic, no moves");
  }
  Axis* a = r <= CAL_SPEED_X ? (r % 2 == 0 ? &z : &x) : NULL;
  int n = lcd.print("Now ");
  switch (r) {
    case CAL_PITCH_Z: case CAL_PITCH_X: n += printDeciMicrons((long) round(a->screwPitch), 5); break;
    case CAL_BACKLASH_Z: case CAL_BACKLASH_X: n += printDeciMicrons(a->backlashDu, 5); break;
    case CAL_DIR_Z: case CAL_DIR_X: n += lcd.print(a->invertStepper ? "inverted" : "normal"); break;
    case CAL_TRAVEL_Z: case CAL_TRAVEL_X: n += lcd.print(a->maxTravelMm); n += lcd.print("mm"); break;
    case CAL_SPEED_Z: case CAL_SPEED_X: n += lcd.print(a->speedManualMove); n += lcd.print(" st/s"); break;
    case CAL_ENC_PPR: n += lcd.print(encoderPpr); n += lcd.print(" ppr"); break;
    case CAL_ENC_DIR: n += lcd.print(encoderInvert ? "inverted" : "normal"); break;
  }
  return n;
}

long getCalHash() {
  long h = (calRoutine + 2) * 1000003L + calStep * 10007L + calListIndex * 101L
      + calParamIndex * 7L + calValue * 3L + calPrev + measure + (calDirMoved ? 5 : 0);
  if (numpadIndex > 0) h += numpadIndex * 31L + getNumpadResult() * 13L;
  if (splashActive()) h += splashId * 7919L;
  switch (calRoutine) {
    case CAL_ENC_SIGNAL:
      // Coherence has to be in here in its own right: a spindle sitting still with a noisy
      // encoder moves the figure while spindlePos stays put, which is exactly the case the
      // screen exists to show.
      h += spindlePos * 3L + encoderReversals * 11L + getApproxRpm() * 13L
          + encHealth.coherence * 17L + encHealth.worstCoherence * 19L;
      break;
    case CAL_ENC_PPR:
    case CAL_ENC_DIR:
      h += (spindlePos - calRefSpindle) * 3L;
      break;
    case CAL_INPUTS:
      h += calLastKeyCode * 37L + (calLastKeyPress ? 1 : 0);
      // Only meaningful when the pins have been configured - see updateCalDisplay.
      if (joystickUse) {
        for (int i = 0; i < 6; i++) {
          if (DREAD(joystickPin(i)) == LOW) h += 1L << i;
        }
      }
      break;
    case CAL_BACKLASH_Z: case CAL_BACKLASH_X:
    case CAL_TRAVEL_Z: case CAL_TRAVEL_X:
      // moveStep is in the hash because both routines now let the STEP key change it in place.
      h += calAxis()->pos * 3L + moveStep * 17L;
      break;
  }
  return h;
}

void updateCalDisplay() {
  long newHash = getCalHash();
  if (newHash == calLcdHash) {
    return;
  }
  calLcdHash = newHash;
  Axis* a = calAxis();
  int charIndex = 0;
  const char* hint = "";

  if (calRoutine < 0) {
    lcd.setCursor(0, 0);
    charIndex = lcd.print("Calibrate ");
    charIndex += lcd.print(calListIndex + 1);
    charIndex += lcd.print(" of ");
    charIndex += lcd.print(CAL_ROUTINES_COUNT);
    printLcdSpaces(charIndex);
    lcd.setCursor(0, 1);
    charIndex = lcd.print(CAL_ROUTINES[calListIndex]);
    printLcdSpaces(charIndex);
    lcd.setCursor(0, 2);
    charIndex = calPrintNow(calListIndex);
    printLcdSpaces(charIndex);
    lcd.setCursor(0, 3);
    charIndex = splashActive() ? lcd.print(splashBuf) : lcd.print("ON start, OFF exit");
    printLcdSpaces(charIndex);
    return;
  }

  lcd.setCursor(0, 0);
  charIndex = lcd.print(CAL_ROUTINES[calRoutine]);
  printLcdSpaces(charIndex);

  switch (calRoutine) {
    case CAL_PITCH_Z:
    case CAL_PITCH_X:
      lcd.setCursor(0, 1);
      if (calStep == 0) {
        charIndex = lcd.print("Test ");
        charIndex += printDeciMicrons(CAL_TEST_DU[calParamIndex], 5);
        charIndex += lcd.print(measure == MEASURE_INCH ? "\"" : "mm");
        hint = "<> size, ON load";
      } else if (calStep == 1) {
        charIndex = lcd.print("Slack taken up");
        hint = "Zero dial, ON move";
      } else if (calStep == 2) {
        charIndex = lcd.print("Actual travel?");
        hint = "Type + ON, - back";
      } else {
        charIndex = printDeciMicrons((long) round(a->screwPitch), 5);
        charIndex += lcd.print(" > ");
        charIndex += printDeciMicrons(calValue, 5);
        hint = "- redo, ON save";
      }
      printLcdSpaces(charIndex);
      lcd.setCursor(0, 2);
      if (calStep == 0) {
        charIndex = lcd.print(calRoutine == CAL_PITCH_X ? "Slide travel not dia" : "Loads axis 1mm first");
      } else if (calStep == 1) {
        charIndex = lcd.print("Do not move it back");
      } else if (calStep == 2) {
        charIndex = lcd.print(measure == MEASURE_INCH ? "Thou: " : "Microns: ");
        if (numpadIndex > 0) {
          charIndex += printDeciMicrons(numpadRawToDu(getNumpadResult()), 5);
        }
      } else {
        charIndex = lcd.print("New screw pitch");
      }
      printLcdSpaces(charIndex);
      break;

    case CAL_BACKLASH_Z:
    case CAL_BACKLASH_X:
      lcd.setCursor(0, 1);
      if (calStep == 0) {
        charIndex = lcd.print("Take up slack +2mm");
        hint = "ON move, + again";
      } else if (calStep == 1) {
        charIndex = lcd.print("Zero the indicator");
        hint = "+ more, ON when set";
      } else if (calStep == 2) {
        charIndex = lcd.print("Back off ");
        charIndex += printDeciMicrons(stepsToDu(a, abs(calRefPos - a->pos)), 5);
        hint = "<> nudge, ON if mvd";
      } else {
        charIndex = printDeciMicrons(a->backlashDu, 5);
        charIndex += lcd.print(" > ");
        charIndex += printDeciMicrons(calValue, 5);
        hint = "- redo, ON save";
      }
      printLcdSpaces(charIndex);
      lcd.setCursor(0, 2);
      if (calStep == 2) {
        // The nudge size decides the resolution of the whole measurement, so it has to be visible
        // and changeable here rather than assumed from whatever the main screen was left on.
        charIndex = lcd.print("STEP key: ");
        charIndex += printDeciMicrons(moveStep, 5);
        charIndex += lcd.print(measure == MEASURE_INCH ? "\"" : "mm");
      }
      else if (calStep == 3) charIndex = lcd.print("New backlash");
      else charIndex = 0;
      printLcdSpaces(charIndex);
      break;

    case CAL_DIR_Z:
    case CAL_DIR_X:
      lcd.setCursor(0, 1);
      if (!calDirMoved) {
        charIndex = lcd.print("Test move +5mm");
        hint = "ON to move";
      } else {
        charIndex = lcd.print(calRoutine == CAL_DIR_Z ? "To the tailstock?" : "Away from you?");
        hint = "ON yes,< no,+ again";
      }
      printLcdSpaces(charIndex);
      lcd.setCursor(0, 2);
      charIndex = lcd.print(a->invertStepper ? "Now inverted" : "Now normal");
      printLcdSpaces(charIndex);
      break;

    case CAL_TRAVEL_Z:
    case CAL_TRAVEL_X:
      lcd.setCursor(0, 1);
      if (calStep == 0) {
        charIndex = lcd.print("Jog to one end");
      } else if (calStep == 1) {
        charIndex = lcd.print("Span ");
        charIndex += printDeciMicrons(stepsToDu(a, abs(a->pos - calRefPos)), 5);
      } else {
        charIndex = lcd.print(a->maxTravelMm);
        charIndex += lcd.print("mm > ");
        charIndex += lcd.print(calValue);
        charIndex += lcd.print("mm");
      }
      hint = calStep == 2 ? "- redo, ON save" : (calRoutine == CAL_TRAVEL_Z ? "<> jog, ON mark" : "^v jog, ON mark");
      printLcdSpaces(charIndex);
      lcd.setCursor(0, 2);
      if (calStep == 1) charIndex = lcd.print("Now jog to other end");
      else if (calStep == 2) charIndex = lcd.print("New max travel");
      else {
        // Tapping an arrow moves one of these, so show the size while creeping up on a hard stop.
        charIndex = lcd.print("STEP key: ");
        charIndex += printDeciMicrons(moveStep, 5);
        charIndex += lcd.print(measure == MEASURE_INCH ? "\"" : "mm");
      }
      printLcdSpaces(charIndex);
      break;

    case CAL_SPEED_Z:
    case CAL_SPEED_X:
      lcd.setCursor(0, 1);
      if (calStep == 0) {
        charIndex = lcd.print("Ramp from ");
        charIndex += lcd.print(a->speedManualMove);
        hint = "ON to start";
      } else if (calStep == 1) {
        charIndex = lcd.print("Try ");
        charIndex += lcd.print(calValue);
        charIndex += lcd.print(" steps/s");
        hint = "ON up,+ same,< stall";
      } else {
        charIndex = lcd.print(calSaved);
        charIndex += lcd.print(" > ");
        charIndex += lcd.print(calValue);
        hint = "- redo, ON save";
      }
      printLcdSpaces(charIndex);
      lcd.setCursor(0, 2);
      if (calStep == 1) charIndex = lcd.print("20mm strokes");
      else if (calStep == 2) charIndex = lcd.print("80% of last good");
      else charIndex = 0;
      printLcdSpaces(charIndex);
      break;

    case CAL_ENC_PPR: {
      long counts = abs(spindlePos - calRefSpindle);
      lcd.setCursor(0, 1);
      if (calStep == 0) {
        charIndex = lcd.print("Turn ");
        charIndex += lcd.print(CAL_REVS[calParamIndex]);
        charIndex += lcd.print(" revolutions");
        hint = "<> revs, ON start";
      } else if (calStep == 1) {
        charIndex = lcd.print("Counted ");
        charIndex += lcd.print(counts);
        hint = "ON when done";
      } else {
        charIndex = lcd.print(encoderPpr);
        charIndex += lcd.print(" > ");
        charIndex += lcd.print(calValue);
        hint = "- redo, ON save";
      }
      printLcdSpaces(charIndex);
      lcd.setCursor(0, 2);
      if (calStep == 0) {
        charIndex = lcd.print("Mark the chuck first");
      } else if (calStep == 1) {
        charIndex = lcd.print("PPR so far ");
        charIndex += lcd.print(long(round(counts / (2.0 * CAL_REVS[calParamIndex]))));
      } else {
        charIndex = lcd.print("New encoder PPR");
      }
      printLcdSpaces(charIndex);
      break;
    }

    case CAL_ENC_DIR:
      lcd.setCursor(0, 1);
      charIndex = lcd.print("Turn spindle fwd");
      printLcdSpaces(charIndex);
      lcd.setCursor(0, 2);
      charIndex = lcd.print(spindlePos - calRefSpindle);
      charIndex += lcd.print(encoderInvert ? " inverted" : " normal");
      printLcdSpaces(charIndex);
      hint = "Rising? ON. Else <";
      break;

    case CAL_ENC_SIGNAL:
      lcd.setCursor(0, 1);
      charIndex = lcd.print("Ang ");
      charIndex += lcd.print(spindleModulo(spindlePos) * 360 / ENCODER_STEPS_FLOAT, 1);
      charIndex += lcd.print(" Rpm ");
      charIndex += lcd.print(getApproxRpm());
      printLcdSpaces(charIndex);
      lcd.setCursor(0, 2);
      // Coherence now and the worst seen since the reset, then the spurious flip count. A clean
      // encoder sits at 100 while the spindle turns; the low-water mark is the number that
      // matters, because noise comes in bursts you will not be looking at the screen for. The
      // filter value lives one menu away - these three are what you watch while changing it.
      charIndex = lcd.print("Coh ");
      charIndex += encHealth.coherence < 0 ? lcd.print("--") : lcd.print(encHealth.coherence);
      charIndex += lcd.print(" Lo ");
      charIndex += encHealth.worstCoherence < 0 ? lcd.print("--") : lcd.print(encHealth.worstCoherence);
      charIndex += lcd.print(" Flp ");
      charIndex += lcd.print(encoderReversals);
      printLcdSpaces(charIndex);
      hint = "< reset, OFF exit";
      break;

    case CAL_INPUTS:
      lcd.setCursor(0, 1);
      charIndex = lcd.print("Key ");
      if (calLastKeyCode < 0) {
        charIndex += lcd.print("none yet");
      } else {
        charIndex += lcd.print(calLastKeyCode);
        charIndex += lcd.print(calLastKeyPress ? " down" : " up");
      }
      printLcdSpaces(charIndex);
      lcd.setCursor(0, 2);
      // The joystick pins only get pinMode()d when joystickUse is set. Reading them otherwise
      // returns whatever a floating input picks up, which would look like a live but erratic
      // stick and send you chasing wiring faults that aren't there.
      if (joystickUse) {
        charIndex = lcd.print("Joystick ");
        for (int i = 0; i < 6; i++) {
          charIndex += lcd.print(DREAD(joystickPin(i)) == LOW ? "1" : "0");
        }
      } else {
        charIndex = lcd.print("Joystick disabled");
      }
      printLcdSpaces(charIndex);
      hint = "Hold OFF 1s to exit";
      break;
  }

  lcd.setCursor(0, 3);
  charIndex = splashActive() ? lcd.print(splashBuf) : lcd.print(hint);
  printLcdSpaces(charIndex);
}

void processKeypadEvent() {
  int event = 0;
  if (serialKeycode != 0) {
    event = serialKeycode;
    serialKeycode = 0;
  } else if (keypad.available() > 0) {
    event = keypad.getEvent();
  } else if (joystickUse) {
    event = getJoystickEvent();
  }
  if (event == 0) return;
  int keyCode = event;
  bitWrite(keyCode, 7, 0);
  bool isPress = bitRead(event, 7) == 1; // 1 - press, 0 - release
  keypadTimeUs = micros();

  // Calibration screen swallows all keys, and unlike the settings screens it needs releases
  // too: the travel routines jog with the arrow keys and the input tester exits on a hold.
  if (inCal) {
    processCalKeypress(keyCode, isPress);
    return;
  }

  // Settings menu or thread database screen swallows all keys while shown.
  if (inSettings || inThreadPicker) {
    if (isPress) processSettingsKeypress(keyCode);
    return;
  }

  // Off button always gets handled.
  if (keyCode == B_OFF) {
    buttonOffPressed = isPress;
    isPress ? buttonOnOffPress(false) : buttonOffRelease();
  }

  if (mode == MODE_GCODE && isOn) {
    // Not allowed to interfere other than turn off.
    if (isPress && keyCode != B_OFF) beep();
    return;
  }

  // Releases don't matter in numpad but it has to run before LRUD since it might handle those keys.
  if (isPress && processNumpad(keyCode)) {
    return;
  }

  // Setup wizard navigation.
  if (isPress && setupIndex == 2 && (keyCode == B_LEFT || keyCode == B_RIGHT)) {
    auxForward = !auxForward;
  } else if (isPress && mode == MODE_GCODE && setupIndex == 1 && (keyCode == B_UP || keyCode == B_DOWN)) {
    if (gcodeProgramIndex > 0 && keyCode == B_UP) gcodeProgramIndex--;
    else if (gcodeProgramIndex == 0 && gcodeProgramCount > 0 && keyCode == B_UP) gcodeProgramIndex = gcodeProgramCount - 1;
    else if ((gcodeProgramIndex < gcodeProgramCount - 1) && keyCode == B_DOWN) gcodeProgramIndex++;
    else if (keyCode == B_DOWN) gcodeProgramIndex = 0;
  } else if (isPress && mode == MODE_GCODE && setupIndex == 1 && keyCode == B_MINUS) {
    removeGcode(gcodeProgramIndex);
    return;
  } else if (keyCode == B_LEFT) { // Make sure isPress=false propagates to motion flags.
    buttonLeftPressed = isPress;
  } else if (keyCode == B_RIGHT) {
    buttonRightPressed = isPress;
  } else if (keyCode == B_UP) {
    buttonUpPressed = isPress;
  } else if (keyCode == B_DOWN) {
    buttonDownPressed = isPress;
  } else if (keyCode == B_MODE_GEARS) {
    buttonGearsPressed = isPress;
  } else if (keyCode == B_MODE_TURN) {
    buttonTurnPressed = isPress;
  } else if (keyCode == B_A) {
    // Short press retracts / returns X, long press toggles the X stepper e.g. for hand-cranking.
    if (isPress) {
      buttonAPressed = true;
      buttonAPressMs = millis();
    } else if (buttonAPressed) {
      buttonAPressed = false;
      if (millis() - buttonAPressMs >= BUTTON_HOLD_MS) {
        x.disabled = !x.disabled;
        updateEnable(&x);
      } else {
        xRetractToggle();
      }
    }
  } else if (keyCode == B_SETTINGS) {
    // Handled on release, because which of the two screens is wanted cannot be known until the
    // key comes back up. The flag matters: pressing this button to leave the settings menu is
    // consumed by the menu, and the release then arrives here with no press to match it.
    if (isPress) {
      buttonSettingsPressed = true;
      buttonSettingsPressMs = millis();
    } else if (buttonSettingsPressed) {
      buttonSettingsPressed = false;
      if (isOn) {
        beep();
      } else {
        // Long press always reaches the settings menu. That is what frees the short press for
        // each mode to claim - see modeHasOwnScreen().
        bool held = millis() - buttonSettingsPressMs >= BUTTON_HOLD_MS;
        if (!held && modeHasOwnScreen()) {
          enterModeScreen();
        } else {
          enterSettingsScreen(false);
        }
      }
    }
  }

  // For all other keys we have no "release" logic.
  if (!isPress) {
    return;
  }

  // Rest of the buttons.
  if (keyCode == B_PLUS) {
    buttonPlusMinusPress(true);
  } else if (keyCode == B_MINUS) {
    buttonPlusMinusPress(false);
  } else if (keyCode == B_ON) {
    buttonOnOffPress(true);
  } else if (keyCode == B_STOPL) {
    buttonLeftStopPress(&z);
  } else if (keyCode == B_STOPR) {
    buttonRightStopPress(&z);
  } else if (keyCode == B_STOPU) {
    buttonLeftStopPress(&x);
  } else if (keyCode == B_STOPD) {
    buttonRightStopPress(&x);
  } else if (keyCode == B_MODE_OTHER) {
    buttonModePress();
  } else if (keyCode == B_DISPL) {
    buttonDisplayPress();
  } else if (keyCode == B_X) {
    markAxis0(&x);
  } else if (keyCode == B_Z) {
    markAxis0(&z);
  } else if (keyCode == B_B) {
    z.disabled = !z.disabled;
    updateEnable(&z);
  } else if (keyCode == B_STEP) {
    buttonMoveStepPress();
  } else if (keyCode == B_REVERSE) {
    buttonReversePress();
  } else if (keyCode == B_MEASURE) {
    buttonMeasurePress();
  } else if (keyCode == B_MODE_GEARS && mode != MODE_A1) {
    // Pressing gear again swaps which axis the spindle drives.
    setModeFromTask(mode == MODE_NORMAL ? MODE_XGEAR : MODE_NORMAL);
  } else if (keyCode == B_MODE_TURN && mode != MODE_A1) {
    setModeFromTask(MODE_TURN);
  } else if (keyCode == B_MODE_FACE) {
    mode == MODE_A1 ? buttonRightStopPress(&a1) : setModeFromTask(MODE_FACE);
  } else if (keyCode == B_MODE_CONE) {
    mode == MODE_A1 ? buttonLeftStopPress(&a1) : setModeFromTask(MODE_CONE);
  } else if (keyCode == B_MODE_CUT) {
    if (mode == MODE_A1) {
      a1.disabled = !a1.disabled;
      updateEnable(&a1);
    } else {
      setModeFromTask(MODE_CUT);
    }
  } else if (keyCode == B_MODE_THREAD) {
    // Pressing thread again switches between a straight and a tapered thread.
    mode == MODE_A1 || (mode == MODE_GCODE && ACTIVE_A1) ? markAxis0(&a1)
        : setModeFromTask(mode == MODE_THREAD ? MODE_TPR : MODE_THREAD);
  }
}

// Moves the stepper so that the tool is located at the newPos.
bool stepToContinuous(Axis* a, long newPos) {
  return stepTo(a, newPos, true);
}

bool stepToFinal(Axis* a, long newPos) {
  return stepTo(a, newPos, false);
}

bool stepTo(Axis* a, long newPos, bool continuous) {
  if (xSemaphoreTake(a->mutex, 10) == pdTRUE) {
    a->continuous = continuous;
    if (newPos == a->pos) {
      a->pendingPos = 0;
    } else {
      a->pendingPos = newPos - a->motorPos - (newPos > a->pos ? 0 : a->backlashSteps);
    }
    xSemaphoreGive(a->mutex);
    return true;
  }
  return false;
}

// Calculates stepper position from spindle position.
long posFromSpindle(Axis* a, long s, bool respectStops) {
  long newPos = s * a->motorSteps / a->screwPitch / ENCODER_STEPS_FLOAT * dupr * starts;

  // Respect left/right stops.
  if (respectStops) {
    if (newPos < a->rightStop) {
      newPos = a->rightStop;
    } else if (newPos > a->leftStop) {
      newPos = a->leftStop;
    }
  }

  return newPos;
}

// Calculates spindle position from stepper position.
long spindleFromPos(Axis* a, long p) {
  return p * a->screwPitch * ENCODER_STEPS_FLOAT / a->motorSteps / (dupr * starts);
}

void stepperEnable(Axis* a, bool value) {
  if (!a->needsRest || !a->active) {
    return;
  }
  if (value) {
    a->stepperEnableCounter++;
    if (value == 1) {
      updateEnable(a);
    }
  } else if (a->stepperEnableCounter > 0) {
    a->stepperEnableCounter--;
    if (a->stepperEnableCounter == 0) {
      updateEnable(a);
    }
  }
}

void updateEnable(Axis* a) {
  if (!a->disabled && (!a->needsRest || a->stepperEnableCounter > 0)) {
    DHIGH(a->ena);
    // Stepper driver needs some time before it will react to pulses.
    DELAY(STEPPED_ENABLE_DELAY_MS);
  } else {
    DLOW(a->ena);
  }
}

void moveAxis(Axis* a) {
  // Most of the time a step isn't needed.
  if (a->pendingPos == 0) {
    if (a->speed > a->speedStart) {
      a->speed--;
    }
    return;
  }

  unsigned long nowUs = micros();
  float delayUs = 1000000.0 / a->speed;
  if (nowUs - a->stepStartUs < delayUs - 5) {
    // Not enough time has passed to issue this step.
    return;
  }

  if (xSemaphoreTake(a->mutex, 1) == pdTRUE) {
    // Check pendingPos again now that we have the mutex.
    if (a->pendingPos != 0) {
      bool dir = a->pendingPos > 0;
      setDir(a, dir);

      DLOW(a->step);
      int delta = dir ? 1 : -1;
      a->pendingPos -= delta;
      if (dir && a->motorPos >= a->pos) {
        a->pos++;
      } else if (!dir && a->motorPos <= (a->pos - a->backlashSteps)) {
        a->pos--;
      }
      a->motorPos += delta;
      a->posGlobal += delta;

      bool accelerate = a->continuous || a->pendingPos >= a->decelerateSteps || a->pendingPos <= -a->decelerateSteps;
      a->speed += (accelerate ? 1 : -1) * a->acceleration * delayUs / 1000000.0;
      if (a->speed > a->speedMax) {
        a->speed = a->speedMax;
      } else if (a->speed < a->speedStart) {
        a->speed = a->speedStart;
      }
      a->stepStartUs = nowUs;

      DHIGH(a->step);
    }
    xSemaphoreGive(a->mutex);
  }
}

// One pitch of axis travel per spindle revolution. MODE_NORMAL passes Z, MODE_XGEAR passes X.
void modeGearbox(Axis* a) {
  if (a->movingManually) {
    return;
  }
  a->speedMax = LONG_MAX;
  stepToContinuous(a, posFromSpindle(a, spindlePosAvg, true));
}

// Feed rate for modes that move an axis at a distance per second rather than per spindle turn.
long duPerSecondToStepsPerSecond(Axis* a, long duPerSecond) {
  long speed = round(abs(duPerSecond) * a->motorSteps / a->screwPitch);
  return speed < 1 ? 1 : speed;
}

long spindleModulo(long value) {
  return calSpindleModulo(value, ENCODER_STEPS_INT);
}

long auxSafeDistance, startOffset;
long peckNextPos = 0; // Cut-off mode: x.pos at which the next chip-breaking peck retract happens
long peckReturnPos = 0; // Cut-off mode: depth to return to after the peck retract
void modeTurn(Axis* main, Axis* aux) {
  if (main->movingManually || aux->movingManually || turnPasses <= 0 ||
      main->leftStop == LONG_MAX || main->rightStop == LONG_MIN ||
      aux->leftStop == LONG_MAX || aux->rightStop == LONG_MIN ||
      dupr == 0 || (dupr * opDuprSign < 0) || starts < 1) {
    setIsOnFromLoop(false);
    return;
  }

  // Variables below have to be re-calculated every time because origin can change
  // while TURN is running e.g. due to dupr change.
  long mainStartStop = opDuprSign > 0 ? main->rightStop : main->leftStop;
  long mainEndStop = opDuprSign > 0 ? main->leftStop : main->rightStop;
  long auxStartStop = auxForward ? aux->rightStop : aux->leftStop;
  long auxEndStop = auxForward ? aux->leftStop : aux->rightStop;

  // opIndex 0 is only executed once, do setup calculations here.
  if (opIndex == 0) {
    auxSafeDistance = (auxForward ? -1 : 1) * safeDistanceDu * aux->motorSteps / aux->screwPitch;
    startOffset = passStartOffset(starts, ENCODER_STEPS_FLOAT);

    // Move to right-bottom limit.
    main->speedMax = main->speedManualMove;
    aux->speedMax = aux->speedManualMove;
    long auxPos = auxStartStop;
    // Overstep by 1 so that "main" backlash is taken out before "opSubIndex == 1".
    long mainPos = mainStartStop + (opDuprSign > 0 ? -1 : 1);
    stepToFinal(main, mainPos);
    stepToFinal(aux, auxPos);
    if (main->pos == mainPos && aux->pos == auxPos) {
      stepToFinal(main, mainStartStop);
      opIndex = 1;
      opSubIndex = 0;
    }
  } else if (opIndex <= passTotalSteps(turnPasses, springPasses, starts)) {
    if (opIndexAdvanceFlag && (opIndex + starts) < turnPasses * starts) {
      opIndexAdvanceFlag = false;
      opIndex += starts;
    }
    // Passes beyond turnPasses are spring passes repeating the last pass at full depth.
    long passNumber = passNumberForIndex(opIndex, starts, turnPasses);
    long auxPos = passDepthPos(auxStartStop, auxEndStop, turnPasses, passNumber);
    // Bringing X to starting position.
    if (opSubIndex == 0) {
      stepToFinal(aux, auxPos);
      if (aux->pos == auxPos) {
        opSubIndex = 1;
        long flankShift = 0;
        if (isThreadMode() && flankInfeed && turnPasses > 1) {
          flankShift = passFlankShift(abs(stepsToDu(aux, auxEndStop - auxStartStop)),
              turnPasses, passNumber, ENCODER_STEPS_FLOAT, dupr);
        }
        spindlePosSync = spindleModulo(spindlePosGlobal - spindleFromPos(main, main->posGlobal) + startOffset * (opIndex - 1) + flankShift);
        return; // Instead of jumping to the next step, let spindlePosSync get to 0 first.
      }
    }
    // spindlePosSync counted down to 0, start thread from here.
    if (opSubIndex == 1) {
      markOrigin();
      main->speedMax = LONG_MAX;
      opSubIndex = 2;
      // markOrigin() changed Start/EndStop values, re-calculate them.
      return;
    }
    // Doing the pass cut.
    if (opSubIndex == 2) {
      // In case we were pushed to the next opIndex before finishing the current one.
      long mainTargetPos = posFromSpindle(main, spindlePosAvg, true);
      long auxTargetPos = auxPos;
      // Tapered threading: drift aux across as main advances, so the thread gets progressively
      // shallower along its length. Same ratio arithmetic modeCone() uses.
      if (mode == MODE_TPR && coneRatio != 0) {
        float coneEffectRatio = -coneRatio / 2 / main->motorSteps * aux->motorSteps /
            aux->screwPitch * main->screwPitch * (auxForward ? 1 : -1);
        auxTargetPos = auxPos + round(mainTargetPos * coneEffectRatio);
        if (auxTargetPos > aux->leftStop) auxTargetPos = aux->leftStop;
        if (auxTargetPos < aux->rightStop) auxTargetPos = aux->rightStop;
      }
      stepToFinal(aux, auxTargetPos);
      stepToContinuous(main, mainTargetPos);
      if (main->pos == mainEndStop ||
          (mode == MODE_TPR && coneRatio != 0 &&
           aux->pos == (opDuprSign > 0 ? auxStartStop : auxEndStop))) {
        opSubIndex = 3;
      }
    }
    // Retracting the tool
    if (opSubIndex == 3) {
      long auxPos = auxStartStop + auxSafeDistance;
      stepToFinal(aux, auxPos);
      if (aux->pos == auxPos) {
        opSubIndex = 4;
      }
    }
    // Returning to start of main.
    if (opSubIndex == 4) {
      main->speedMax = main->speedManualMove;
      // Overstep by 1 so that "main" backlash is taken out before "opSubIndex == 2".
      long mainPos = mainStartStop + (opDuprSign > 0 ? -1 : 1);
      stepToFinal(main, mainPos);
      if (main->pos == mainPos) {
        stepToFinal(main, mainStartStop);
        opSubIndex = 0;
        opIndex++;
      }
    }
  } else {
    // Move to right-bottom limit.
    main->speedMax = main->speedManualMove;
    long auxPos = auxStartStop;
    long mainPos = mainStartStop;
    stepToFinal(main, mainPos);
    stepToFinal(aux, auxPos);
    if (main->pos == mainPos && aux->pos == auxPos) {
      setIsOnFromLoop(false);
      beepFor(BEEP_DONE);
    }
  }
}

void modeCone() {
  if (z.movingManually || x.movingManually || coneRatio == 0) {
    return;
  }

  float zToXRatio = -coneRatio / 2 / z.motorSteps * x.motorSteps / x.screwPitch * z.screwPitch * (auxForward ? 1 : -1);
  if (zToXRatio == 0) {
    return;
  }

  // TODO: calculate maximum speeds and accelerations to avoid potential desync.
  x.speedMax = LONG_MAX;
  z.speedMax = LONG_MAX;

  // Respect limits of both axis by translating them into limits on spindlePos value.
  long spindle = spindlePosAvg;
  long spindleMin = LONG_MIN;
  long spindleMax = LONG_MAX;
  if (z.leftStop != LONG_MAX) {
    (dupr > 0 ? spindleMax : spindleMin) = spindleFromPos(&z, z.leftStop);
  }
  if (z.rightStop != LONG_MIN) {
    (dupr > 0 ? spindleMin: spindleMax) = spindleFromPos(&z, z.rightStop);
  }
  if (x.leftStop != LONG_MAX) {
    long lim = spindleFromPos(&z, round(x.leftStop / zToXRatio));
    if (zToXRatio < 0) {
      (dupr > 0 ? spindleMin: spindleMax) = lim;
    } else {
      (dupr > 0 ? spindleMax : spindleMin) = lim;
    }
  }
  if (x.rightStop != LONG_MIN) {
    long lim = spindleFromPos(&z, round(x.rightStop / zToXRatio));
    if (zToXRatio < 0) {
      (dupr > 0 ? spindleMax : spindleMin) = lim;
    } else {
      (dupr > 0 ? spindleMin: spindleMax) = lim;
    }
  }
  if (spindle > spindleMax) {
    spindle = spindleMax;
  } else if (spindle < spindleMin) {
    spindle = spindleMin;
  }

  stepToContinuous(&z, posFromSpindle(&z, spindle, true));
  stepToContinuous(&x, round(z.pos * zToXRatio));
}

// Slotting: the lathe used as a shaper. The spindle is not involved at all - X feeds in to depth,
// Z strokes to the left, X retracts and Z returns, one stroke per pass, going deeper each time.
// Cutting an internal keyway is the usual reason to want it.
//
// Pitch means feed distance per second here rather than per spindle turn, so it can be adjusted
// while running and a pitch of 0 parks the stroke until it is raised again.
void modeSlot() {
  if (z.movingManually || x.movingManually || turnPasses <= 0 ||
      z.leftStop == LONG_MAX || z.rightStop == LONG_MIN ||
      x.leftStop == LONG_MAX || x.rightStop == LONG_MIN) {
    setIsOnFromLoop(false);
    return;
  }

  long zStartStop = z.rightStop;
  long zEndStop = z.leftStop;
  long xStartStop = auxForward ? x.rightStop : x.leftStop;
  long xEndStop = auxForward ? x.leftStop : x.rightStop;

  if (opIndex == 0) {
    z.speedMax = z.speedManualMove;
    x.speedMax = x.speedManualMove;
    stepToFinal(&z, zStartStop);
    stepToFinal(&x, xStartStop);
    if (z.pos == zStartStop && x.pos == xStartStop) {
      opIndex = 1;
      opSubIndex = 0;
    }
  } else if (opIndex <= turnPasses) {
    long xPos = slotDepthPos(xStartStop, xEndStop, turnPasses, opIndex);
    long zEndPos = slotStrokeEnd(zStartStop, zEndStop, axisDuToSteps(&z, slotLeftReductionDu), opIndex);

    if (opSubIndex == 0) {
      x.speedMax = x.speedManualMove;
      stepToFinal(&x, xPos);
      if (x.pos == xPos) {
        opSubIndex = 1;
      }
    } else if (opSubIndex == 1) {
      if (dupr == 0) {
        stepToFinal(&z, z.pos);
        return;
      }
      z.speedMax = duPerSecondToStepsPerSecond(&z, dupr);
      stepToContinuous(&z, zEndPos);
      if (z.pos == zEndPos) {
        opSubIndex = 2;
      }
    } else if (opSubIndex == 2) {
      x.speedMax = x.speedManualMove;
      stepToFinal(&x, xStartStop);
      if (x.pos == xStartStop) {
        opSubIndex = 3;
      }
    } else if (opSubIndex == 3) {
      z.speedMax = z.speedManualMove;
      stepToFinal(&z, zStartStop);
      if (z.pos == zStartStop) {
        opSubIndex = 0;
        opIndex++;
      }
    }
  } else {
    setIsOnFromLoop(false);
    beepFor(BEEP_DONE);
  }
}

void modeCut() {
  if (x.movingManually || turnPasses <= 0 || x.leftStop == LONG_MAX || x.rightStop == LONG_MIN || dupr == 0 || dupr * opDuprSign < 0) {
    setIsOnFromLoop(false);
    return;
  }

  long startStop = opDuprSign > 0 ? x.rightStop : x.leftStop;
  long endStop = opDuprSign > 0 ? x.leftStop : x.rightStop;

  if (opIndex == 0) {
    // Move to back limit.
    x.speedMax = x.speedManualMove;
    long xPos = startStop;
    stepToFinal(&x, xPos);
    if (x.pos == xPos) {
      opIndex = 1;
      opSubIndex = 0;
    }
  } else if (opIndex <= turnPasses) {
    long peckSteps = peckDepthDu * x.motorSteps / x.screwPitch;
    // Set spindlePos and x.pos in sync.
    if (opSubIndex == 0) {
      spindlePosAvg = spindlePos = spindleFromPos(&x, x.pos);
      peckNextPos = peckNextDepth(x.pos, peckSteps, dupr > 0);
      opSubIndex = 1;
    }
    // Doing the pass cut.
    if (opSubIndex == 1) {
      x.speedMax = LONG_MAX;
      long endPos = passDepthPos(startStop, endStop, turnPasses, opIndex);
      long xPos = posFromSpindle(&x, spindlePosAvg, true);
      if (dupr > 0 && xPos > endPos) xPos = endPos;
      else if (dupr < 0 && xPos < endPos) xPos = endPos;
      stepToContinuous(&x, xPos);
      if (x.pos == endPos) {
        opSubIndex = 2;
      } else if (peckSteps > 0 && (dupr > 0 ? x.pos >= peckNextPos : x.pos <= peckNextPos)) {
        // Peck: back off to break the chip, then resume from the same depth.
        peckReturnPos = x.pos;
        opSubIndex = 3;
      }
    }
    // Returning to start.
    if (opSubIndex == 2) {
      x.speedMax = x.speedManualMove;
      stepToFinal(&x, startStop);
      if (x.pos == startStop) {
        opSubIndex = 0;
        opIndex++;
      }
    }
    // Peck retract to break the chip.
    if (opSubIndex == 3) {
      x.speedMax = x.speedManualMove;
      long target = peckRetractPos(peckReturnPos, long(safeDistanceDu * x.motorSteps / x.screwPitch), startStop, dupr > 0);
      stepToFinal(&x, target);
      if (x.pos == target) {
        opSubIndex = 4;
      }
    }
    // Returning to the depth the peck started from and resuming the cut.
    if (opSubIndex == 4) {
      x.speedMax = x.speedManualMove;
      stepToFinal(&x, peckReturnPos);
      if (x.pos == peckReturnPos) {
        spindlePosAvg = spindlePos = spindleFromPos(&x, x.pos);
        peckNextPos = peckNextDepth(x.pos, peckSteps, dupr > 0);
        opSubIndex = 1;
      }
    }
  } else {
    setIsOnFromLoop(false);
    beepFor(BEEP_DONE);
  }
}

void modeEllipse(Axis* main, Axis* aux) {
  if (main->movingManually || aux->movingManually || turnPasses <= 0 ||
      main->leftStop == LONG_MAX || main->rightStop == LONG_MIN ||
      aux->leftStop == LONG_MAX || aux->rightStop == LONG_MIN ||
      main->leftStop == main->rightStop ||
      aux->leftStop == aux->rightStop ||
      dupr == 0 || dupr != opDupr) {
    setIsOnFromLoop(false);
    return;
  }

  // Start from left or right depending on the pitch.
  long mainStartStop = opDuprSign > 0 ? main->rightStop : main->leftStop;
  long mainEndStop = opDuprSign > 0 ? main->leftStop : main->rightStop;
  long auxStartStop = aux->rightStop;
  long auxEndStop = aux->leftStop;

  main->speedMax = main->speedManualMove;
  aux->speedMax = aux->speedManualMove;

  if (opIndex == 0) {
    opIndex = 1;
    opSubIndex = 0;
    spindlePos = 0;
    spindlePosAvg = 0;
  } else if (opIndex <= turnPasses) {
    float pass0to1 = opIndex / float(turnPasses);
    long mainDelta = round(pass0to1 * (mainEndStop - mainStartStop));
    long auxDelta = round(pass0to1 * (auxEndStop - auxStartStop));
    long spindleDelta = spindleFromPos(main, mainDelta);

    // Move to starting position.
    if (opSubIndex == 0) {
      long auxPos = auxStartStop;
      stepToFinal(aux, auxPos);
      if (aux->pos == auxPos) {
        opSubIndex = 1;
      }
    } else if (opSubIndex == 1) {
      long mainPos = mainEndStop - mainDelta;
      stepToFinal(main, mainPos);
      if (main->pos == mainPos) {
        opSubIndex = 2;
        spindlePos = 0;
        spindlePosAvg = 0;
      }
    } else if (opSubIndex == 2) {
      float progress0to1 = 0;
      if ((spindleDelta > 0 && spindlePosAvg >= spindleDelta) || (spindleDelta < 0 && spindlePosAvg <= spindleDelta)) {
        progress0to1 = 1;
      } else {
        progress0to1 = spindlePosAvg / float(spindleDelta);
      }
      float mainCoeff = auxForward ? cos(HALF_PI * (3 + progress0to1)) : (1 + sin(HALF_PI * (progress0to1 - 1)));
      long mainPos = mainEndStop - mainDelta + round(mainDelta * mainCoeff);
      float auxCoeff = auxForward ? (1 + sin(HALF_PI * (3 + progress0to1))) : sin(HALF_PI * progress0to1);
      long auxPos = auxStartStop + round(auxDelta * auxCoeff);
      stepToContinuous(main, mainPos);
      stepToContinuous(aux, auxPos);
      if (progress0to1 == 1 && main->pos == mainPos && aux->pos == auxPos) {
        opIndex++;
        opSubIndex = 0;
      }
    }
  } else if (opIndex == turnPasses + 1) {
    stepToFinal(aux, auxStartStop);
    if (aux->pos == auxStartStop) {
      setIsOnFromLoop(false);
      beepFor(BEEP_DONE);
    }
  }
}

long mmOrInchToAbsolutePos(Axis* a, float mmOrInch) {
  long scaleToDu = measure == MEASURE_METRIC ? 10000 : 254000;
  long part1 = a->gcodeRelativePos;
  long part2 = round(mmOrInch * scaleToDu / a->screwPitch * a->motorSteps);
  return part1 + part2;
}

String getValueString(const String& command, char letter) {
  int index = command.indexOf(letter);
  if (index == -1) {
    return "";
  }
  String valueString;
  for (int i = index + 1; i < command.length(); i++) {
    char c = command.charAt(i);
    if (isDigit(c) || c == '.' || c == '-') {
      valueString += c;
    } else {
      break;
    }
  }
  return valueString;
}

float getFloat(const String& command, char letter) {
  return getValueString(command, letter).toFloat();
}

int getInt(const String& command, char letter) {
  return getValueString(command, letter).toInt();
}

void updateAxisSpeeds(long diffX, long diffZ, long diffA1) {
  if (diffX == 0 && diffZ == 0 && diffA1 == 0) return;
  long absX = abs(diffX);
  long absZ = abs(diffZ);
  long absC = abs(diffA1);
  float stepsPerSecX = gcodeFeedDuPerSec * x.motorSteps / x.screwPitch;
  float minStepsPerSecX = GCODE_FEED_MIN_DU_SEC * x.motorSteps / x.screwPitch;
  if (stepsPerSecX > x.speedManualMove) stepsPerSecX = x.speedManualMove;
  else if (stepsPerSecX < minStepsPerSecX) stepsPerSecX = minStepsPerSecX;
  float stepsPerSecZ = gcodeFeedDuPerSec * z.motorSteps / z.screwPitch;
  float minStepsPerSecZ = GCODE_FEED_MIN_DU_SEC * z.motorSteps / z.screwPitch;
  if (stepsPerSecZ > z.speedManualMove) stepsPerSecZ = z.speedManualMove;
  else if (stepsPerSecZ < minStepsPerSecZ) stepsPerSecZ = minStepsPerSecZ;
  float stepsPerSecA1 = gcodeFeedDuPerSec * a1.motorSteps / a1.screwPitch;
  float minStepsPerSecA1 = GCODE_FEED_MIN_DU_SEC * a1.motorSteps / a1.screwPitch;
  if (stepsPerSecA1 > a1.speedManualMove) stepsPerSecA1 = a1.speedManualMove;
  else if (stepsPerSecA1 < minStepsPerSecA1) stepsPerSecA1 = minStepsPerSecA1;
  float secX = absX / stepsPerSecX;
  float secZ = absZ / stepsPerSecZ;
  float secA1 = absC / stepsPerSecA1;
  float sec = ACTIVE_A1 ? max(max(secX, secZ), secA1) : max(secX, secZ);
  x.speedMax = sec > 0 ? absX / sec : x.speedManualMove;
  z.speedMax = sec > 0 ? absZ / sec : z.speedManualMove;
  a1.speedMax = sec > 0 ? absC / sec : a1.speedManualMove;
}

void setFeedRate(const String& command) {
  float feed = getFloat(command, 'F');
  if (feed <= 0) return;
  gcodeFeedDuPerSec = round(feed * (measure == MEASURE_METRIC ? 10000 : 254000) / 60.0);
}

void gcodeWaitEpsilon(int epsilon) {
  while (abs(x.pendingPos) > epsilon || abs(z.pendingPos) > epsilon || abs(a1.pendingPos) > epsilon || (SPINDLE_PAUSES_GCODE && getApproxRpm() < GCODE_MIN_RPM)) {
    taskYIELD();
  }
}

void gcodeWaitNear() {
  gcodeWaitEpsilon(GCODE_WAIT_EPSILON_STEPS);
}

void gcodeWaitStop() {
  gcodeWaitEpsilon(0);
}

// Rapid positioning / linear interpolation.
void G00_01(const String& command) {
  long xStart = x.pos;
  long zStart = z.pos;
  long a1Start = a1.pos;
  long xEnd = command.indexOf(x.name) >= 0 ? mmOrInchToAbsolutePos(&x, getFloat(command, x.name)) : xStart;
  long zEnd = command.indexOf(z.name) >= 0 ? mmOrInchToAbsolutePos(&z, getFloat(command, z.name)) : zStart;
  long a1End = command.indexOf(a1.name) >= 0 ? mmOrInchToAbsolutePos(&a1, getFloat(command, a1.name)) : a1Start;
  long xDiff = xEnd - xStart;
  long zDiff = zEnd - zStart;
  long a1Diff = a1End - a1Start;
  updateAxisSpeeds(xDiff, zDiff, a1Diff);
  long chunks = round(max(max(abs(xDiff), abs(zDiff)), abs(a1Diff)) * LINEAR_INTERPOLATION_PRECISION);
  for (long i = 0; i < chunks; i++) {
    if (!isOn) return;
    float scale = i / float(chunks);
    stepToContinuous(&x, xStart + xDiff * scale);
    stepToContinuous(&z, zStart + zDiff * scale);
    if (ACTIVE_A1) stepToContinuous(&a1, a1Start + a1Diff * scale);
    gcodeWaitNear();
  }
  // To avoid any rounding error, move to precise position.
  stepToFinal(&x, xEnd);
  stepToFinal(&z, zEnd);
  if (ACTIVE_A1) stepToFinal(&a1, a1End);
  gcodeWaitStop();
}

bool handleGcode(const String& command) {
  int op = getInt(command, 'G');
  if (op == 0 || op == 1) { // 0 also covers X and Z commands without G.
    G00_01(command);
  } else if (op == 20 || op == 21) {
    setMeasure(op == 20 ? MEASURE_INCH : MEASURE_METRIC);
  } else if (op == 90 || op == 91) {
    gcodeAbsolutePositioning = op == 90;
  } else if (op == 94) {
    /* no-op feed per minute */
  } else if (op == 18) {
    /* no-op ZX plane selection */
  } else {
    Serial.print("error: unsupported command ");
    Serial.println(command);
    return false;
  }
  return true;
}

bool handleMcode(const String& command) {
  int op = getInt(command, 'M');
  if (op == 0 || op == 1 || op == 2 || op == 30) {
    setIsOnFromTask(false);
  } else {
    setIsOnFromTask(false);
    Serial.print("error: unsupported command ");
    Serial.println(command);
    return false;
  }
  return true;
}

// Process one command, return ok flag.
bool handleGcodeCommand(String command) {
  command.trim();
  if (command.length() == 0) return false;

  // Trim N.. prefix.
  char code = command.charAt(0);
  int spaceIndex = command.indexOf(' ');
  if (code == 'N' && spaceIndex > 0) {
    command = command.substring(spaceIndex + 1);
    code = command.charAt(0);
  }

  // Update position for relative calculations right before performing them.
  z.gcodeRelativePos = gcodeAbsolutePositioning ? -z.originPos : z.pos;
  x.gcodeRelativePos = gcodeAbsolutePositioning ? -x.originPos : x.pos;
  a1.gcodeRelativePos = gcodeAbsolutePositioning ? -a1.originPos : a1.pos;

  setFeedRate(command);
  switch (code) {
    case 'G':
    case NAME_Z:
    case NAME_X:
    case NAME_A1: return handleGcode(command);
    case 'F': return true; /* feed already handled above */
    case 'M': return handleMcode(command);
    case 'T': return true; /* ignoring tool changes */
    default: Serial.print("error: unsupported command "); Serial.println(code); return false;
  }
  return false;
}

void discountFullSpindleTurns() {
  // When standing at the stop, ignore full spindle turns.
  // This allows to avoid waiting when spindle direction reverses
  // and reduces the chance of the skipped stepper steps since
  // after a reverse the spindle starts slow.
  if (dupr != 0 && !stepperIsRunning(&z) && (mode == MODE_NORMAL || mode == MODE_CONE)) {
    int spindlePosDiff = 0;
    if (z.pos == z.rightStop) {
      long stopSpindlePos = spindleFromPos(&z, z.rightStop);
      if (dupr > 0) {
        if (spindlePos < stopSpindlePos - ENCODER_STEPS_INT) {
          spindlePosDiff = ENCODER_STEPS_INT;
        }
      } else {
        if (spindlePos > stopSpindlePos + ENCODER_STEPS_INT) {
          spindlePosDiff = -ENCODER_STEPS_INT;
        }
      }
    } else if (z.pos == z.leftStop) {
      long stopSpindlePos = spindleFromPos(&z, z.leftStop);
      if (dupr > 0) {
        if (spindlePos > stopSpindlePos + ENCODER_STEPS_INT) {
          spindlePosDiff = -ENCODER_STEPS_INT;
        }
      } else {
        if (spindlePos < stopSpindlePos - ENCODER_STEPS_INT) {
          spindlePosDiff = ENCODER_STEPS_INT;
        }
      }
    }
    if (spindlePosDiff != 0) {
      spindlePos += spindlePosDiff;
      spindlePosAvg += spindlePosDiff;
    }
  }
}

void processSpindleCounter() {
  int16_t count;
  pcnt_get_counter_value(PCNT_UNIT_0, &count);
  int delta = count - spindleCount;
  spindleCount = count;
  if (delta == 0) {
    return;
  }
  // The counter resets itself to zero on reaching +/-PCNT_LIM, which reads as a jump of nearly
  // a full span in the wrong direction. Taking that at face value injected a huge false
  // reversal, which is what made the RPM readout drop to zero at speed: several RPM
  // measurements would complete in the same microsecond and the interval between them was 0.
  //
  // Undo it arithmetically rather than clearing the counter ourselves. A manual clear throws
  // away whatever arrived between reading the value and clearing it, and at a quarter of a
  // million counts a second - 1000 PPR, 4x, geared up 1:2, at 2000 spindle rpm - that is real
  // position drift accumulating through a threading pass. This stays exact as long as less than
  // half a counter span arrives between two polls: 15500 counts, about 58ms at that rate.
  if (delta > PCNT_LIM / 2) {
    delta -= PCNT_LIM;
  } else if (delta < -PCNT_LIM / 2) {
    delta += PCNT_LIM;
  }
  // Software equivalent of swapping the encoder A and B wires.
  if (encoderInvert) {
    delta = -delta;
  }
  // Fold several raw counts into one step. Movement smaller than the divider never reaches the
  // spindle position at all, so an encoder that flutters on a transition is quietened at the
  // source rather than being masked further downstream by the dead-band.
  if (encoderDivider > 1) {
    encoderDivRemainder += delta;
    delta = encoderDivRemainder / encoderDivider;
    encoderDivRemainder -= delta * encoderDivider;
    if (delta == 0) {
      return;
    }
  }
  // Count only *spurious* direction changes, so the calibration screen shows electrical noise
  // rather than normal use. A reversal after a long run in one direction is you turning the
  // spindle back, which is not a fault; a reversal after barely any movement is the counter
  // dithering on a transition, which is. The same span the motion deadband uses is the
  // dividing line, because movement inside it can't reach the axes anyway.
  int encDir = delta > 0 ? 1 : -1;
  if (encoderLastDir != 0 && encDir != encoderLastDir) {
    if (encoderRunLength <= encoderBacklash) {
      encoderReversals++;
    }
    encoderRunLength = 0;
  }
  encoderLastDir = encDir;
  encoderRunLength += delta > 0 ? delta : -delta;

  unsigned long microsNow = micros();
  // A gap this long means the spindle stopped, so the part-finished bulk measurement is stale.
  // Carrying it over would time the next one from an old timestamp and report a spindle that
  // has just started as nearly stationary. Same threshold getApproxRpm() calls stopped.
  if (microsNow - spindleEncTime > 50000) {
    spindleEncTimeIndex = 0;
    spindleEncTimeAtIndex0 = microsNow;
    // Same reasoning for the health window, so it has to be dropped before this count goes in
    // rather than after: counts from before the pause scored against the whole elapsed time
    // read as a nearly stationary spindle, for movement that had already finished.
    encHealthRestartWindow(&encHealth, microsNow);
  }
  // Signal quality, measured on the same counts that drive the axes - after the divider, so it
  // reports what the machine actually follows rather than what the encoder emits. Always on:
  // the windows worth seeing are the ones that happen mid-cut, not while you stand at the
  // calibration screen. Two adds and a compare per count, a division every 20ms.
  encHealthAdd(&encHealth, delta, microsNow, ENCODER_HEALTH_WINDOW_US,
               encRateFromRpm(ENCODER_STEPS_INT, ENCODER_HEALTH_BUSY_RPM), ENCODER_COHERENCE_FLOOR);
  // The large display's info strip and the encoder signal calibration screen both read RPM,
  // so the bulk timing has to keep running for them as well as for the tacho readout.
  if (showTacho || mode == MODE_GCODE || showBigDro || (inCal && calRoutine == CAL_ENC_SIGNAL)) {
    if (calRpmAccumulate(&spindleEncTimeIndex, delta, RPM_BULK)) {
      spindleEncTimeDiffBulk = microsNow - spindleEncTimeAtIndex0;
      spindleEncTimeAtIndex0 = microsNow;
    }
  } else {
    // Nothing is reading RPM. Reset here too, or switching a readout on mid-rotation would
    // time its first bulk from whenever the accumulator last ran.
    spindleEncTimeDiffBulk = 0;
    spindleEncTimeIndex = 0;
    spindleEncTimeAtIndex0 = microsNow;
  }

  spindlePos += delta;
  spindlePosGlobal += delta;
  if (spindlePosGlobal > ENCODER_STEPS_INT) {
    spindlePosGlobal -= ENCODER_STEPS_INT;
  } else if (spindlePosGlobal < 0) {
    spindlePosGlobal += ENCODER_STEPS_INT;
  }
  spindlePosAvg = encFollowDeadband(spindlePos, spindlePosAvg, encoderBacklash, encoderSymmetric);
  spindleEncTime = microsNow;

  if (spindlePosSync != 0) {
    spindlePosSync += delta;
    if (spindlePosSync % ENCODER_STEPS_INT == 0) {
      spindlePosSync = 0;
      Axis* a = getPitchAxis();
      spindlePosAvg = spindlePos = spindleFromPos(a, a->pos);
    }
  }
}

// Apply changes requested by the keyboard thread.
void applySettings() {
  if (nextDuprFlag) {
    applyDupr();
    nextDuprFlag = false;
  }
  if (nextStartsFlag) {
    applyStarts();
    nextStartsFlag = false;
  }
  if (z.nextLeftStopFlag) {
    applyLeftStop(&z);
    z.nextLeftStopFlag = false;
  }
  if (z.nextRightStopFlag) {
    applyRightStop(&z);
    z.nextRightStopFlag = false;
  }
  if (x.nextLeftStopFlag) {
    applyLeftStop(&x);
    x.nextLeftStopFlag = false;
  }
  if (x.nextRightStopFlag) {
    applyRightStop(&x);
    x.nextRightStopFlag = false;
  }
  if (a1.nextLeftStopFlag) {
    applyLeftStop(&a1);
    a1.nextLeftStopFlag = false;
  }
  if (a1.nextRightStopFlag) {
    applyRightStop(&a1);
    a1.nextRightStopFlag = false;
  }
  if (nextConeRatioFlag) {
    applyConeRatio();
    nextConeRatioFlag = false;
  }
  if (nextIsOnFlag) {
    setIsOnFromLoop(nextIsOn);
    nextIsOnFlag = false;
  }
  if (nextModeFlag) {
    setModeFromLoop(nextMode);
    nextModeFlag = false;
  }
}

void loop() {
  if (emergencyStop != ESTOP_NONE) {
    return;
  }
  // Flash writes stall the instruction cache on both cores, so step timing is meaningless while
  // firmware is being written. Nothing was moving when the upload was accepted and nothing may
  // start now.
  if (otaInProgress) {
    return;
  }
  if (xSemaphoreTake(motionMutex, 1) != pdTRUE) {
    return;
  }
  applySettings();
  processSpindleCounter();
  discountFullSpindleTurns();
  if (!isOn || (dupr == 0 && mode != MODE_SLOT) || spindlePosSync != 0) {
    // None of the modes work.
  } else if (mode == MODE_NORMAL) {
    modeGearbox(&z);
  } else if (mode == MODE_XGEAR) {
    modeGearbox(&x);
  } else if (mode == MODE_TURN) {
    modeTurn(&z, &x);
  } else if (mode == MODE_FACE) {
    modeTurn(&x, &z);
  } else if (mode == MODE_CUT) {
    modeCut();
  } else if (mode == MODE_SLOT) {
    modeSlot();
  } else if (mode == MODE_CONE) {
    modeCone();
  } else if (isThreadMode()) {
    modeTurn(&z, &x);
  } else if (mode == MODE_ELLIPSE) {
    modeEllipse(&z, &x);
  }
  moveAxis(&z);
  moveAxis(&x);
  if (ACTIVE_A1) moveAxis(&a1);
  xSemaphoreGive(motionMutex);
}
