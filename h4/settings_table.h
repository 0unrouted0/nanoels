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
  SETTING_ACTION,      // not a value at all: ON opens another screen
};

// Which axis a per-axis item acts on. Replaces the old trick of deriving the axis from the
// index being odd or even, which stopped working once items were grouped by section.
enum SettingAxis { SAX_NONE, SAX_Z, SAX_X, SAX_A1 };

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
  const char* prefKey;  // per-axis items are stored as the axis letter followed by this
  SettingSection section;
  SettingAxis axis;
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
  {"X readout",           SETTING_GLOBAL_BOOL, "xdd",  SEC_PREFS, SAX_NONE, "diameter", "radius", 0},
  {"Spring passes",       SETTING_GLOBAL_NUM,  "spp",  SEC_PREFS, SAX_NONE, 0, 0, "passes"},
  {"Parting peck depth",  SETTING_GLOBAL_DU,   "pck",  SEC_PREFS, SAX_NONE, 0, 0, 0},
  {"Thread flank infeed", SETTING_GLOBAL_BOOL, "fli",  SEC_PREFS, SAX_NONE, "on", "off", 0},
  {"Retract distance",    SETTING_GLOBAL_DU,   "rtd",  SEC_PREFS, SAX_NONE, 0, 0, 0},
  {"Clearance distance",  SETTING_GLOBAL_DU,   "safe", SEC_PREFS, SAX_NONE, 0, 0, 0},
  {"Slot left reduction", SETTING_GLOBAL_DU,   "slt",  SEC_PREFS, SAX_NONE, 0, 0, 0},
  {"Manual step time",    SETTING_GLOBAL_NUM,  "stm",  SEC_PREFS, SAX_NONE, 0, 0, "ms"},
  {"Step rest",           SETTING_GLOBAL_NUM,  "sdl",  SEC_PREFS, SAX_NONE, 0, 0, "ms"},

  // -- Z axis -------------------------------------------------------------
  {"Invert direction",   SETTING_AXIS_BOOL, "inv",  SEC_Z, SAX_Z, "inverted", "normal", 0},
  {"Backlash",           SETTING_AXIS_DU,   "bla",  SEC_Z, SAX_Z, 0, 0, 0},
  {"Lead screw pitch",   SETTING_AXIS_DU,   "scr",  SEC_Z, SAX_Z, 0, 0, 0},
  {"Motor steps/rev",    SETTING_AXIS_NUM,  "mst",  SEC_Z, SAX_Z, 0, 0, "steps"},
  {"Motor pulley",       SETTING_AXIS_NUM,  "mtt",  SEC_Z, SAX_Z, 0, 0, "teeth"},
  {"Lead screw pulley",  SETTING_AXIS_NUM,  "stt",  SEC_Z, SAX_Z, 0, 0, "teeth"},
  {"Start speed",        SETTING_AXIS_NUM,  "sst",  SEC_Z, SAX_Z, 0, 0, "steps/s"},
  {"Max speed",          SETTING_AXIS_NUM,  "spd",  SEC_Z, SAX_Z, 0, 0, "steps/s"},
  {"Acceleration",       SETTING_AXIS_NUM,  "acc",  SEC_Z, SAX_Z, 0, 0, "st/s2"},
  {"Max travel",         SETTING_AXIS_NUM,  "mtr",  SEC_Z, SAX_Z, 0, 0, "mm"},
  {"Hold when idle",     SETTING_AXIS_BOOL, "rst",  SEC_Z, SAX_Z, "no", "yes", 0},

  // -- X axis -------------------------------------------------------------
  {"Invert direction",   SETTING_AXIS_BOOL, "inv",  SEC_X, SAX_X, "inverted", "normal", 0},
  {"Backlash",           SETTING_AXIS_DU,   "bla",  SEC_X, SAX_X, 0, 0, 0},
  {"Lead screw pitch",   SETTING_AXIS_DU,   "scr",  SEC_X, SAX_X, 0, 0, 0},
  {"Motor steps/rev",    SETTING_AXIS_NUM,  "mst",  SEC_X, SAX_X, 0, 0, "steps"},
  {"Motor pulley",       SETTING_AXIS_NUM,  "mtt",  SEC_X, SAX_X, 0, 0, "teeth"},
  {"Lead screw pulley",  SETTING_AXIS_NUM,  "stt",  SEC_X, SAX_X, 0, 0, "teeth"},
  {"Start speed",        SETTING_AXIS_NUM,  "sst",  SEC_X, SAX_X, 0, 0, "steps/s"},
  {"Max speed",          SETTING_AXIS_NUM,  "spd",  SEC_X, SAX_X, 0, 0, "steps/s"},
  {"Acceleration",       SETTING_AXIS_NUM,  "acc",  SEC_X, SAX_X, 0, 0, "st/s2"},
  {"Max travel",         SETTING_AXIS_NUM,  "mtr",  SEC_X, SAX_X, 0, 0, "mm"},
  {"Hold when idle",     SETTING_AXIS_BOOL, "rst",  SEC_X, SAX_X, "no", "yes", 0},

  // -- A1 axis ------------------------------------------------------------
  // Whether the axis exists is read once at startup, so it needs a restart to take effect.
  {"Fitted (restart)",   SETTING_AXIS_BOOL, "act",  SEC_A1, SAX_A1, "yes", "no", 0},
  {"Rotary",             SETTING_AXIS_BOOL, "rot",  SEC_A1, SAX_A1, "yes", "no", 0},
  {"Invert direction",   SETTING_AXIS_BOOL, "inv",  SEC_A1, SAX_A1, "inverted", "normal", 0},
  {"Backlash",           SETTING_AXIS_DU,   "bla",  SEC_A1, SAX_A1, 0, 0, 0},
  {"Screw pitch",        SETTING_AXIS_DU,   "scr",  SEC_A1, SAX_A1, 0, 0, 0},
  {"Motor steps/rev",    SETTING_AXIS_NUM,  "mst",  SEC_A1, SAX_A1, 0, 0, "steps"},
  {"Motor pulley",       SETTING_AXIS_NUM,  "mtt",  SEC_A1, SAX_A1, 0, 0, "teeth"},
  {"Lead screw pulley",  SETTING_AXIS_NUM,  "stt",  SEC_A1, SAX_A1, 0, 0, "teeth"},
  {"Start speed",        SETTING_AXIS_NUM,  "sst",  SEC_A1, SAX_A1, 0, 0, "steps/s"},
  {"Max speed",          SETTING_AXIS_NUM,  "spd",  SEC_A1, SAX_A1, 0, 0, "steps/s"},
  {"Acceleration",       SETTING_AXIS_NUM,  "acc",  SEC_A1, SAX_A1, 0, 0, "st/s2"},
  {"Max travel",         SETTING_AXIS_NUM,  "mtr",  SEC_A1, SAX_A1, 0, 0, "mm"},
  {"Hold when idle",     SETTING_AXIS_BOOL, "rst",  SEC_A1, SAX_A1, "no", "yes", 0},

  // -- Spindle encoder ----------------------------------------------------
  // The divider reads as a ratio - "4 to 1" folds four raw counts into one step - so its unit is
  // the second half of that phrase rather than a quantity.
  {"Encoder PPR",          SETTING_GLOBAL_NUM,  "eppr", SEC_ENCODER, SAX_NONE, 0, 0, "pulses"},
  {"Direction",            SETTING_GLOBAL_BOOL, "einv", SEC_ENCODER, SAX_NONE, "inverted", "normal", 0},
  {"Spindle pulley",       SETTING_GLOBAL_NUM,  "est",  SEC_ENCODER, SAX_NONE, 0, 0, "teeth"},
  {"Encoder pulley",       SETTING_GLOBAL_NUM,  "ept",  SEC_ENCODER, SAX_NONE, 0, 0, "teeth"},
  {"Divider",              SETTING_GLOBAL_NUM,  "ediv", SEC_ENCODER, SAX_NONE, 0, 0, "to 1"},
  {"Dead-band",            SETTING_GLOBAL_NUM,  "ebl",  SEC_ENCODER, SAX_NONE, 0, 0, "counts"},
  // One-way is how the firmware has always behaved and models lead screw backlash. Symmetric
  // filters both directions equally, which is the one to reach for if the encoder is the problem.
  {"Dead-band shape",      SETTING_GLOBAL_BOOL, "ebls", SEC_ENCODER, SAX_NONE, "symmetric", "one-way", 0},
  {"Glitch filter",        SETTING_GLOBAL_NUM,  "eflt", SEC_ENCODER, SAX_NONE, 0, 0, "cycles"},

  // -- Handwheels ---------------------------------------------------------
  // Whether a handwheel is fitted is read once at startup to attach its interrupt, so that one
  // needs a restart. The rest take effect immediately.
  {"Fitted 1 (restart)", SETTING_GLOBAL_BOOL, "p1u",  SEC_HANDWHEEL, SAX_NONE, "yes", "no", 0},
  {"Invert 1",           SETTING_GLOBAL_BOOL, "p1i",  SEC_HANDWHEEL, SAX_NONE, "yes", "no", 0},
  {"Fitted 2 (restart)", SETTING_GLOBAL_BOOL, "p2u",  SEC_HANDWHEEL, SAX_NONE, "yes", "no", 0},
  {"Invert 2",           SETTING_GLOBAL_BOOL, "p2i",  SEC_HANDWHEEL, SAX_NONE, "yes", "no", 0},
  {"Pulses per rev",     SETTING_GLOBAL_NUM,  "hppr", SEC_HANDWHEEL, SAX_NONE, 0, 0, "pulses"},
  {"Min pulse width",    SETTING_GLOBAL_NUM,  "pmw",  SEC_HANDWHEEL, SAX_NONE, 0, 0, "us"},
  {"Dead-band",          SETTING_GLOBAL_NUM,  "phb",  SEC_HANDWHEEL, SAX_NONE, 0, 0, "counts"},

  // -- Joystick -----------------------------------------------------------
  // Which pin is which direction is wiring, so it stays in machine_config.h.
  {"Fitted (restart)",   SETTING_GLOBAL_BOOL, "joy",  SEC_JOYSTICK, SAX_NONE, "yes", "no", 0},
  {"Debounce",           SETTING_GLOBAL_NUM,  "jdb",  SEC_JOYSTICK, SAX_NONE, 0, 0, "ms"},

  // -- WiFi and updates ---------------------------------------------------
  // Both take effect immediately: the radio follows the toggle, and changing the PIN cycles a
  // running access point so it picks the new one up. The PIN is the WPA2 password and must be
  // exactly 8 digits - see machine_config.h. It is a code rather than a measurement, so it
  // genuinely has no unit.
  {"Enabled",            SETTING_GLOBAL_BOOL, "wfen", SEC_WIFI, SAX_NONE, "yes", "no", 0},
  {"Access PIN",         SETTING_GLOBAL_NUM,  "wfpw", SEC_WIFI, SAX_NONE, 0, 0, 0},

  // -- Calibration --------------------------------------------------------
  {"Open calibration",   SETTING_ACTION,      "",     SEC_CAL, SAX_NONE, 0, 0, 0},
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

// Storage key of an item, as both Preferences and the serial/HTTP interfaces name it: a per-axis
// item is the axis letter followed by the suffix, a global one is the suffix alone. out needs 16
// bytes, which is also the NVS key limit. The single definition of the key namespace - anything
// that reimplements it can disagree with what is actually stored, which is exactly how the axis
// letter came to be derived from index parity long after that stopped being true.
inline void settingKeyName(int index, char* out) {
  int n = 0;
  char letter = settingAxisLetter(index);
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
