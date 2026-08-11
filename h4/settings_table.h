// The settings menu described as data instead of as parallel switch statements.
//
// The menu is two levels: a directory of sections, and the items within a section. Everything
// that is pure classification - which section an item belongs to, which axis it acts on, whether
// it is a toggle, whether it takes a distance, what it is called - lives in the table below.
// Only reading and writing the underlying variable is per-item code, because each item touches
// a different variable.
//
// Items must stay grouped by section: a section is a contiguous run of the table, which is what
// makes navigating one a matter of a first index and a count. test/ checks that.
//
// No Arduino dependencies, so test/ can check the table's structure.

#ifndef SETTINGS_TABLE_H
#define SETTINGS_TABLE_H

enum SettingKind {
  SETTING_AXIS_BOOL,   // per-axis on/off, toggled with ON
  SETTING_AXIS_DU,     // per-axis distance, typed in microns or thou
  SETTING_AXIS_NUM,    // per-axis plain number
  SETTING_GLOBAL_BOOL, // global on/off, toggled with ON
  SETTING_GLOBAL_DU,   // global distance
  SETTING_GLOBAL_NUM,  // global plain number
  SETTING_GLOBAL_LIST, // global choice from a named list, stored as its index
  SETTING_GLOBAL_SPEED,// global surface speed, stored in m/min and shown in ft/min in inch mode
  SETTING_ACTION,      // not a value at all: ON opens another screen
};

// Which axis a per-axis item acts on. Replaces the old trick of deriving the axis from the
// index being odd or even, which stopped working once items were grouped by section.
enum SettingAxis { SAX_NONE, SAX_Z, SAX_X, SAX_A1 };

// Which operating mode an item belongs to, or SMODE_NONE for one that belongs to the machine.
//
// A mode-scoped item has one stored value per mode: the spring passes that suit a thread are not
// the ones that suit a facing cut, and before this they were the same number. The storage key
// carries a mode letter exactly as a per-axis key carries an axis letter.
enum SettingMode {
  SMODE_NONE,
  SMODE_TURN,
  SMODE_FACE,
  SMODE_CUT,
  SMODE_THREAD,
  SMODE_TPR,
  SMODE_ELLIPSE,
  SMODE_SLOT,
  SMODE_CONE,
  SMODE_COUNT,
};

// Title of the mode's own settings screen, and the heading the web page groups them under.
inline const char* settingModeName(int m) {
  switch (m) {
    case SMODE_TURN: return "Turning";
    case SMODE_FACE: return "Facing";
    case SMODE_CUT: return "Parting";
    case SMODE_THREAD: return "Threading";
    case SMODE_TPR: return "Tapered thread";
    case SMODE_ELLIPSE: return "Ellipse";
    case SMODE_SLOT: return "Slotting";
    case SMODE_CONE: return "Cone";
    default: return "";
  }
}

enum SettingSection {
  SEC_PREFS,
  SEC_Z,
  SEC_X,
  SEC_A1,
  SEC_ENCODER,
  SEC_HANDWHEEL,
  SEC_JOYSTICK,
  SEC_WIFI,
  SEC_CAL,
  SECTION_COUNT,
  // Mode-scoped items live outside the directory: they are reached by a short press of the
  // settings button, which opens the current mode's page directly. Numbering it at SECTION_COUNT
  // keeps it out of the directory's arithmetic without that code needing to know it exists.
  SEC_MODE = SECTION_COUNT,
};

// Shown in the directory, so at most 18 characters to leave room for the cursor.
const char* SECTION_NAMES[SECTION_COUNT] = {
  "Preferences",
  "Z axis",
  "X axis",
  "A1 axis",
  "Spindle encoder",
  "Handwheels",
  "Joystick",
  "WiFi & updates",
  "Calibration",
};

struct SettingDesc {
  const char* label;    // at most 20 characters
  SettingKind kind;
  // Per-axis items are stored as the axis letter followed by this, mode-scoped ones as the mode
  // letter followed by it. Several modes therefore share a suffix - "spp" is spring passes for
  // all of them - and the prefix is what keeps their values apart.
  const char* prefKey;
  SettingSection section;
  SettingAxis axis;
  SettingMode mode;     // SMODE_NONE for an item that belongs to the machine, not to a mode
  const char* onLabel;  // toggles only; 0 falls back to on/off
  const char* offLabel;
  // Shown after the value, at most 8 characters so it still fits the LCD beside a 6-digit number.
  // 0 means the item has no unit, which is the honest answer for a toggle, for a ratio and for the
  // WiFi PIN. Distances also carry 0 because their unit is not fixed - it follows the metric/inch
  // setting, so whatever renders one supplies mm or inches itself.
  const char* unit;
};

// Grouped by section, and each section's items must be contiguous.
const SettingDesc SETTINGS[] = {
  // -- Preferences --------------------------------------------------------
  // How the readout behaves, then how automated operations cut, then how manual jogging feels.
  // Spring passes, peck depth, flank infeed, clearance and the slot reduction used to live here
  // as one global value each. They belong to the operation rather than to the machine and have
  // moved to the per-mode pages further down.
  {"X readout",           SETTING_GLOBAL_BOOL, "xdd",  SEC_PREFS, SAX_NONE, SMODE_NONE, "diameter", "radius", 0},
  // Whether a commanded move longer than the axis's max travel trips the emergency stop. It is a
  // guard against a move that would run off the end of the machine, and it is only as good as the
  // max travel figures it compares against - so it can be switched off on a machine whose travels
  // have not been measured, where it fires on perfectly good moves instead.
  {"Travel E-stop",       SETTING_GLOBAL_BOOL, "estp", SEC_PREFS, SAX_NONE, SMODE_NONE, "on", "off", 0},
  {"Retract distance",    SETTING_GLOBAL_DU,   "rtd",  SEC_PREFS, SAX_NONE, SMODE_NONE, 0, 0, 0},
  {"Manual step time",    SETTING_GLOBAL_NUM,  "stm",  SEC_PREFS, SAX_NONE, SMODE_NONE, 0, 0, "ms"},
  {"Step rest",           SETTING_GLOBAL_NUM,  "sdl",  SEC_PREFS, SAX_NONE, SMODE_NONE, 0, 0, "ms"},
  // Indexing replaces the angle line on the main screen, constant speed replaces the tacho line.
  {"Spindle divisions",   SETTING_GLOBAL_NUM,  "idiv", SEC_PREFS, SAX_NONE, SMODE_NONE, 0, 0, "marks"},
  {"Index tolerance",     SETTING_GLOBAL_NUM,  "itol", SEC_PREFS, SAX_NONE, SMODE_NONE, 0, 0, "0.1deg"},

  // Constant cutting speed. The switch comes first and is the only thing that has to be touched
  // to turn the whole feature off - the three items under it keep their values while it is off,
  // so switching back on returns to the setup you had rather than to nothing.
  //
  // Material picks the speed from a table for the fitted tool; Manual means use the typed figure
  // instead, which is why it sits in the same list rather than being a separate mode.
  {"Constant speed",      SETTING_GLOBAL_BOOL, "cson", SEC_PREFS, SAX_NONE, SMODE_NONE, "on", "off", 0},
  {"Material",            SETTING_GLOBAL_LIST, "cmat", SEC_PREFS, SAX_NONE, SMODE_NONE, 0, 0, 0},
  {"Tool",                SETTING_GLOBAL_BOOL, "ctol", SEC_PREFS, SAX_NONE, SMODE_NONE, "carbide", "HSS", 0},
  // Unit is 0 for the same reason a distance carries 0: it is not fixed, it follows the
  // metric/inch setting, so whatever renders one supplies m/min or ft/min itself.
  {"Manual speed",        SETTING_GLOBAL_SPEED,"css",  SEC_PREFS, SAX_NONE, SMODE_NONE, 0, 0, 0},
  {"Spindle max rpm",     SETTING_GLOBAL_NUM,  "smax", SEC_PREFS, SAX_NONE, SMODE_NONE, 0, 0, "rpm"},

  // -- Z axis -------------------------------------------------------------
  {"Invert direction",   SETTING_AXIS_BOOL, "inv",  SEC_Z, SAX_Z, SMODE_NONE, "inverted", "normal", 0},
  {"Backlash",           SETTING_AXIS_DU,   "bla",  SEC_Z, SAX_Z, SMODE_NONE, 0, 0, 0},
  {"Lead screw pitch",   SETTING_AXIS_DU,   "scr",  SEC_Z, SAX_Z, SMODE_NONE, 0, 0, 0},
  {"Motor steps/rev",    SETTING_AXIS_NUM,  "mst",  SEC_Z, SAX_Z, SMODE_NONE, 0, 0, "steps"},
  {"Motor pulley",       SETTING_AXIS_NUM,  "mtt",  SEC_Z, SAX_Z, SMODE_NONE, 0, 0, "teeth"},
  {"Lead screw pulley",  SETTING_AXIS_NUM,  "stt",  SEC_Z, SAX_Z, SMODE_NONE, 0, 0, "teeth"},
  {"Start speed",        SETTING_AXIS_NUM,  "sst",  SEC_Z, SAX_Z, SMODE_NONE, 0, 0, "steps/s"},
  {"Max speed",          SETTING_AXIS_NUM,  "spd",  SEC_Z, SAX_Z, SMODE_NONE, 0, 0, "steps/s"},
  {"Acceleration",       SETTING_AXIS_NUM,  "acc",  SEC_Z, SAX_Z, SMODE_NONE, 0, 0, "st/s2"},
  {"Max travel",         SETTING_AXIS_NUM,  "mtr",  SEC_Z, SAX_Z, SMODE_NONE, 0, 0, "mm"},
  // Whether the driver stays energised between moves. "No" lets an open-loop motor cool, and costs
  // position for it: de-energising frees the rotor to be pulled to the nearest detent by the load,
  // and energising snaps it to wherever the driver's phase counter restarts. That is a small fixed
  // error at each end of every move - invisible over a long move, a real fraction of a short one,
  // and cumulative over a lot of jogging. Say yes on any axis whose position you care about.
  {"Hold when idle",     SETTING_AXIS_BOOL, "rst",  SEC_Z, SAX_Z, SMODE_NONE, "no", "yes", 0},

  // -- X axis -------------------------------------------------------------
  {"Invert direction",   SETTING_AXIS_BOOL, "inv",  SEC_X, SAX_X, SMODE_NONE, "inverted", "normal", 0},
  {"Backlash",           SETTING_AXIS_DU,   "bla",  SEC_X, SAX_X, SMODE_NONE, 0, 0, 0},
  {"Lead screw pitch",   SETTING_AXIS_DU,   "scr",  SEC_X, SAX_X, SMODE_NONE, 0, 0, 0},
  {"Motor steps/rev",    SETTING_AXIS_NUM,  "mst",  SEC_X, SAX_X, SMODE_NONE, 0, 0, "steps"},
  {"Motor pulley",       SETTING_AXIS_NUM,  "mtt",  SEC_X, SAX_X, SMODE_NONE, 0, 0, "teeth"},
  {"Lead screw pulley",  SETTING_AXIS_NUM,  "stt",  SEC_X, SAX_X, SMODE_NONE, 0, 0, "teeth"},
  {"Start speed",        SETTING_AXIS_NUM,  "sst",  SEC_X, SAX_X, SMODE_NONE, 0, 0, "steps/s"},
  {"Max speed",          SETTING_AXIS_NUM,  "spd",  SEC_X, SAX_X, SMODE_NONE, 0, 0, "steps/s"},
  {"Acceleration",       SETTING_AXIS_NUM,  "acc",  SEC_X, SAX_X, SMODE_NONE, 0, 0, "st/s2"},
  {"Max travel",         SETTING_AXIS_NUM,  "mtr",  SEC_X, SAX_X, SMODE_NONE, 0, 0, "mm"},
  {"Hold when idle",     SETTING_AXIS_BOOL, "rst",  SEC_X, SAX_X, SMODE_NONE, "no", "yes", 0},

  // -- A1 axis ------------------------------------------------------------
  // The axis shares its terminals with the handwheels and the joystick, so switching it on is
  // refused while one of those holds the pins - see aux_pins.h.
  {"Fitted",             SETTING_AXIS_BOOL, "act",  SEC_A1, SAX_A1, SMODE_NONE, "yes", "no", 0},
  {"Rotary",             SETTING_AXIS_BOOL, "rot",  SEC_A1, SAX_A1, SMODE_NONE, "yes", "no", 0},
  {"Invert direction",   SETTING_AXIS_BOOL, "inv",  SEC_A1, SAX_A1, SMODE_NONE, "inverted", "normal", 0},
  {"Backlash",           SETTING_AXIS_DU,   "bla",  SEC_A1, SAX_A1, SMODE_NONE, 0, 0, 0},
  {"Screw pitch",        SETTING_AXIS_DU,   "scr",  SEC_A1, SAX_A1, SMODE_NONE, 0, 0, 0},
  {"Motor steps/rev",    SETTING_AXIS_NUM,  "mst",  SEC_A1, SAX_A1, SMODE_NONE, 0, 0, "steps"},
  {"Motor pulley",       SETTING_AXIS_NUM,  "mtt",  SEC_A1, SAX_A1, SMODE_NONE, 0, 0, "teeth"},
  {"Lead screw pulley",  SETTING_AXIS_NUM,  "stt",  SEC_A1, SAX_A1, SMODE_NONE, 0, 0, "teeth"},
  {"Start speed",        SETTING_AXIS_NUM,  "sst",  SEC_A1, SAX_A1, SMODE_NONE, 0, 0, "steps/s"},
  {"Max speed",          SETTING_AXIS_NUM,  "spd",  SEC_A1, SAX_A1, SMODE_NONE, 0, 0, "steps/s"},
  {"Acceleration",       SETTING_AXIS_NUM,  "acc",  SEC_A1, SAX_A1, SMODE_NONE, 0, 0, "st/s2"},
  {"Max travel",         SETTING_AXIS_NUM,  "mtr",  SEC_A1, SAX_A1, SMODE_NONE, 0, 0, "mm"},
  {"Hold when idle",     SETTING_AXIS_BOOL, "rst",  SEC_A1, SAX_A1, SMODE_NONE, "no", "yes", 0},

  // -- Spindle encoder ----------------------------------------------------
  // The divider reads as a ratio - "4 to 1" folds four raw counts into one step - so its unit is
  // the second half of that phrase rather than a quantity.
  {"Encoder PPR",          SETTING_GLOBAL_NUM,  "eppr", SEC_ENCODER, SAX_NONE, SMODE_NONE, 0, 0, "pulses"},
  {"Direction",            SETTING_GLOBAL_BOOL, "einv", SEC_ENCODER, SAX_NONE, SMODE_NONE, "inverted", "normal", 0},
  {"Spindle pulley",       SETTING_GLOBAL_NUM,  "est",  SEC_ENCODER, SAX_NONE, SMODE_NONE, 0, 0, "teeth"},
  {"Encoder pulley",       SETTING_GLOBAL_NUM,  "ept",  SEC_ENCODER, SAX_NONE, SMODE_NONE, 0, 0, "teeth"},
  {"Divider",              SETTING_GLOBAL_NUM,  "ediv", SEC_ENCODER, SAX_NONE, SMODE_NONE, 0, 0, "to 1"},
  {"Dead-band",            SETTING_GLOBAL_NUM,  "ebl",  SEC_ENCODER, SAX_NONE, SMODE_NONE, 0, 0, "counts"},
  // One-way is how the firmware has always behaved and models lead screw backlash. Symmetric
  // filters both directions equally, which is the one to reach for if the encoder is the problem.
  {"Dead-band shape",      SETTING_GLOBAL_BOOL, "ebls", SEC_ENCODER, SAX_NONE, SMODE_NONE, "symmetric", "one-way", 0},
  {"Glitch filter",        SETTING_GLOBAL_NUM,  "eflt", SEC_ENCODER, SAX_NONE, SMODE_NONE, 0, 0, "cycles"},

  // -- Handwheels ---------------------------------------------------------
  // Handwheel 1 shares its terminals with the A1 axis and the joystick, handwheel 2 with the
  // joystick, so switching one on is refused while another device holds the pins. Interrupts are
  // attached and released as these change - see aux_pins.h.
  {"Fitted 1",           SETTING_GLOBAL_BOOL, "p1u",  SEC_HANDWHEEL, SAX_NONE, SMODE_NONE, "yes", "no", 0},
  {"Drives 1",           SETTING_GLOBAL_BOOL, "p1a",  SEC_HANDWHEEL, SAX_NONE, SMODE_NONE, "X", "Z", 0},
  {"Invert 1",           SETTING_GLOBAL_BOOL, "p1i",  SEC_HANDWHEEL, SAX_NONE, SMODE_NONE, "yes", "no", 0},
  {"Fitted 2",           SETTING_GLOBAL_BOOL, "p2u",  SEC_HANDWHEEL, SAX_NONE, SMODE_NONE, "yes", "no", 0},
  {"Drives 2",           SETTING_GLOBAL_BOOL, "p2a",  SEC_HANDWHEEL, SAX_NONE, SMODE_NONE, "X", "Z", 0},
  {"Invert 2",           SETTING_GLOBAL_BOOL, "p2i",  SEC_HANDWHEEL, SAX_NONE, SMODE_NONE, "yes", "no", 0},
  {"Pulses per rev",     SETTING_GLOBAL_NUM,  "hppr", SEC_HANDWHEEL, SAX_NONE, SMODE_NONE, 0, 0, "pulses"},
  {"Min pulse width",    SETTING_GLOBAL_NUM,  "pmw",  SEC_HANDWHEEL, SAX_NONE, SMODE_NONE, 0, 0, "us"},
  {"Dead-band",          SETTING_GLOBAL_NUM,  "phb",  SEC_HANDWHEEL, SAX_NONE, SMODE_NONE, 0, 0, "counts"},

  // -- Joystick -----------------------------------------------------------
  // Which pin is which direction is wiring, so it stays in machine_config.h. The stick needs all
  // six terminals, so it conflicts with every other device that uses them - see aux_pins.h.
  {"Fitted",             SETTING_GLOBAL_BOOL, "joy",  SEC_JOYSTICK, SAX_NONE, SMODE_NONE, "yes", "no", 0},
  {"Debounce",           SETTING_GLOBAL_NUM,  "jdb",  SEC_JOYSTICK, SAX_NONE, SMODE_NONE, 0, 0, "ms"},

  // -- WiFi and updates ---------------------------------------------------
  // Both take effect immediately: the radio follows the toggle, and changing the PIN cycles a
  // running access point so it picks the new one up. The PIN is the WPA2 password and must be
  // exactly 8 digits - see machine_config.h. It is a code rather than a measurement, so it
  // genuinely has no unit.
  {"Enabled",            SETTING_GLOBAL_BOOL, "wfen", SEC_WIFI, SAX_NONE, SMODE_NONE, "yes", "no", 0},
  {"Access PIN",         SETTING_GLOBAL_NUM,  "wfpw", SEC_WIFI, SAX_NONE, SMODE_NONE, 0, 0, 0},

  // -- Per-mode ------------------------------------------------------------
  // Reached by a short press of the settings button, which opens the page for whichever mode is
  // selected. Not in the settings directory: there is no point offering the threading page while
  // the machine is set up to face.
  //
  // Each mode's items must stay contiguous, for the same reason a section's do. Several modes
  // share a suffix - every one of these has "Default passes" under "tps" - and the mode letter in
  // front is what keeps the stored values apart.
  //
  // Note what "tps" stores: the count a mode STARTS from, not the count it is using. The working
  // count belongs to the job in front of you - how many cuts it takes to reach depth follows from
  // the material, the tool and how hard you are pushing - so it is never written to flash, and a
  // restart brings every mode back to its default. Within a session each mode keeps its own, so
  // stepping over to face something and back does not lose the count you dialled in for threading.
  {"Default passes",     SETTING_GLOBAL_NUM,  "tps",  SEC_MODE, SAX_NONE, SMODE_TURN, 0, 0, "passes"},
  {"Spring passes",      SETTING_GLOBAL_NUM,  "spp",  SEC_MODE, SAX_NONE, SMODE_TURN, 0, 0, "passes"},
  {"Clearance",          SETTING_GLOBAL_DU,   "safe", SEC_MODE, SAX_NONE, SMODE_TURN, 0, 0, 0},

  {"Default passes",     SETTING_GLOBAL_NUM,  "tps",  SEC_MODE, SAX_NONE, SMODE_FACE, 0, 0, "passes"},
  {"Spring passes",      SETTING_GLOBAL_NUM,  "spp",  SEC_MODE, SAX_NONE, SMODE_FACE, 0, 0, "passes"},
  {"Clearance",          SETTING_GLOBAL_DU,   "safe", SEC_MODE, SAX_NONE, SMODE_FACE, 0, 0, 0},

  {"Default passes",     SETTING_GLOBAL_NUM,  "tps",  SEC_MODE, SAX_NONE, SMODE_CUT, 0, 0, "passes"},
  {"Peck depth",         SETTING_GLOBAL_DU,   "pck",  SEC_MODE, SAX_NONE, SMODE_CUT, 0, 0, 0},
  {"Clearance",          SETTING_GLOBAL_DU,   "safe", SEC_MODE, SAX_NONE, SMODE_CUT, 0, 0, 0},

  {"Default passes",     SETTING_GLOBAL_NUM,  "tps",  SEC_MODE, SAX_NONE, SMODE_THREAD, 0, 0, "passes"},
  {"Spring passes",      SETTING_GLOBAL_NUM,  "spp",  SEC_MODE, SAX_NONE, SMODE_THREAD, 0, 0, "passes"},
  {"Flank infeed",       SETTING_GLOBAL_BOOL, "fli",  SEC_MODE, SAX_NONE, SMODE_THREAD, "on", "off", 0},
  {"Clearance",          SETTING_GLOBAL_DU,   "safe", SEC_MODE, SAX_NONE, SMODE_THREAD, 0, 0, 0},
  // The database used to be the whole of what the settings button did in thread mode. It is an
  // entry on the page now, so the page can hold the threading settings as well.
  {"Thread database",    SETTING_ACTION,      "thr",  SEC_MODE, SAX_NONE, SMODE_THREAD, 0, 0, 0},

  {"Default passes",     SETTING_GLOBAL_NUM,  "tps",  SEC_MODE, SAX_NONE, SMODE_TPR, 0, 0, "passes"},
  {"Spring passes",      SETTING_GLOBAL_NUM,  "spp",  SEC_MODE, SAX_NONE, SMODE_TPR, 0, 0, "passes"},
  {"Flank infeed",       SETTING_GLOBAL_BOOL, "fli",  SEC_MODE, SAX_NONE, SMODE_TPR, "on", "off", 0},
  {"Clearance",          SETTING_GLOBAL_DU,   "safe", SEC_MODE, SAX_NONE, SMODE_TPR, 0, 0, 0},
  {"Thread database",    SETTING_ACTION,      "thr",  SEC_MODE, SAX_NONE, SMODE_TPR, 0, 0, 0},

  {"Default passes",     SETTING_GLOBAL_NUM,  "tps",  SEC_MODE, SAX_NONE, SMODE_ELLIPSE, 0, 0, "passes"},

  {"Default passes",     SETTING_GLOBAL_NUM,  "tps",  SEC_MODE, SAX_NONE, SMODE_SLOT, 0, 0, "passes"},
  {"Left reduction",     SETTING_GLOBAL_DU,   "slt",  SEC_MODE, SAX_NONE, SMODE_SLOT, 0, 0, 0},

  // -- Calibration --------------------------------------------------------
  {"Open calibration",   SETTING_ACTION,      "",     SEC_CAL, SAX_NONE, SMODE_NONE, 0, 0, 0},
};

// Derived from the table so the two can never drift apart.
const int SETTINGS_COUNT = sizeof(SETTINGS) / sizeof(SETTINGS[0]);

inline bool settingIsAxis(int index) {
  SettingKind k = SETTINGS[index].kind;
  return k == SETTING_AXIS_BOOL || k == SETTING_AXIS_DU || k == SETTING_AXIS_NUM;
}

// Letter a per-axis item's storage key is prefixed with, or 0 for a global one. These must match
// NAME_Z / NAME_X / NAME_A1 in machine_config.h, which the sketch static_asserts; they are spelled
// out here rather than included so this header stays free of machine configuration.
inline char settingAxisLetter(int index) {
  switch (SETTINGS[index].axis) {
    case SAX_Z: return 'Z';
    case SAX_X: return 'X';
    case SAX_A1: return 'C';
    default: return 0;
  }
}

// Letter a mode's storage keys are prefixed with, or 0 for SMODE_NONE. Lower case so it can never
// collide with an axis letter, which is what lets one key namespace hold both - the cone taper
// lives at "ccr" and a C axis item would be "Ccr".
//
// One definition, taken by mode rather than by row, because the save path has a mode and no row.
// It was briefly written out twice and two switches that must agree is one too many.
inline char settingModeLetterOf(int m) {
  switch (m) {
    case SMODE_TURN: return 't';
    case SMODE_FACE: return 'f';
    case SMODE_CUT: return 'p';
    case SMODE_THREAD: return 'h';
    case SMODE_TPR: return 'r';
    case SMODE_ELLIPSE: return 'e';
    case SMODE_SLOT: return 's';
    case SMODE_CONE: return 'c';
    default: return 0;
  }
}

inline char settingModeLetter(int index) {
  return settingModeLetterOf(SETTINGS[index].mode);
}

inline bool settingIsModeScoped(int index) {
  return SETTINGS[index].mode != SMODE_NONE;
}

// Storage key of an item, as both Preferences and the serial/HTTP interfaces name it: a per-axis
// item is the axis letter followed by the suffix, a global one is the suffix alone. out needs 16
// bytes, which is also the NVS key limit. The single definition of the key namespace - anything
// that reimplements it can disagree with what is actually stored, which is exactly how the axis
// letter came to be derived from index parity long after that stopped being true.
inline void settingKeyName(int index, char* out) {
  int n = 0;
  char letter = settingAxisLetter(index);
  if (letter == 0) letter = settingModeLetter(index);
  if (letter != 0) out[n++] = letter;
  const char* k = SETTINGS[index].prefKey;
  for (int i = 0; k[i] != 0 && n < 15; i++) out[n++] = k[i];
  out[n] = 0;
}

// Whether the item is a distance entered in microns / thou.
inline bool settingUsesDu(int index) {
  SettingKind k = SETTINGS[index].kind;
  return k == SETTING_AXIS_DU || k == SETTING_GLOBAL_DU;
}

// Whether the item is an on/off value toggled with the ON key.
inline bool settingIsToggle(int index) {
  SettingKind k = SETTINGS[index].kind;
  return k == SETTING_AXIS_BOOL || k == SETTING_GLOBAL_BOOL;
}

// Whether ON opens another screen rather than committing a typed value.
inline bool settingIsAction(int index) {
  return SETTINGS[index].kind == SETTING_ACTION;
}

// Whether the item is a choice from a named list. Stored and typed as a plain index, so nothing
// about persistence or the wire format changes - only how the value is shown, and that the arrow
// keys step through it. The names themselves live with the thing they describe rather than in
// this table, which has no room for a list per row.
inline bool settingIsList(int index) {
  return SETTINGS[index].kind == SETTING_GLOBAL_LIST;
}

// Whether the item is a surface speed. Stored in m/min throughout, the way a distance is stored in
// deci-microns throughout, and converted to feet per minute only where it meets the operator - so
// a shop working in inches never has to turn 100 m/min into 328 SFM in its head.
inline bool settingUsesSpeed(int index) {
  return SETTINGS[index].kind == SETTING_GLOBAL_SPEED;
}

// Whether a decimal point can be typed into this item. Distances are held in deci-microns and take
// one. Everything else is a whole number where it is stored - a pass count, a motor's steps per
// revolution, a surface speed in whole m/min - and a point typed into one of those would be dropped
// somewhere between the screen and the write, which is worse than the key refusing it.
inline bool settingAcceptsPoint(int index) {
  return settingUsesDu(index);
}

// A mode's items, navigated exactly as a section's are: a first index and a count, which only
// works because each mode's run is contiguous. Returns -1 and 0 for a mode with no settings of
// its own, which the caller uses to decide there is no page worth opening.
inline int settingModeFirst(int m) {
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (SETTINGS[i].mode == m) return i;
  }
  return -1;
}

inline int settingModeCount(int m) {
  int n = 0;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (SETTINGS[i].mode == m) n++;
  }
  return n;
}

// Index of a section's first item, or -1 if it has none.
inline int settingSectionFirst(int section) {
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (SETTINGS[i].section == section) return i;
  }
  return -1;
}

inline int settingSectionCount(int section) {
  int n = 0;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (SETTINGS[i].section == section) n++;
  }
  return n;
}

#endif
