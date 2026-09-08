// The settings that belong to an operation rather than to the machine, and the validation that
// guards them. Shared with the host test suite in test/ - no Arduino here.
//
// Each mode keeps one of these. The sketch holds an array of them and hands the values to and
// from the plain variables the motion code reads, so nothing on the motion path has to know that
// a setting is mode-scoped at all.
//
// The read and write below dispatch on the table's storage suffix rather than on a menu index,
// the same way the global settings do: one branch per parameter, and every mode reuses it.

#ifndef MODE_SETTINGS_H
#define MODE_SETTINGS_H

#include <string.h>

// Shared with the numpad, which clamps a typed pass count to the same ceiling.
#define MODE_PASSES_MAX 999
#define MODE_SPRING_PASSES_MAX 99

// What a mode's default pass count itself starts at, on a controller that has never been set up.
#define MODE_PASSES_DEFAULT 3

struct ModeSettings {
  // The count this mode is using now. Kept per mode so switching away to face something and back
  // does not lose the count dialled in for threading, but never written to flash: how many cuts it
  // takes to reach depth belongs to the job, and a count carried over from last week looks like a
  // considered answer while being nothing of the kind. A restart brings it back to defaultPasses.
  long passes;
  // The count this mode starts from. This one is stored, and is what the settings menu edits.
  long defaultPasses;
  long springPasses;
  long clearanceDu;
  long peckDu;
  long slotReductionDu;
  bool flankInfeed;
  // No table row: the menu holds whole numbers and a taper is a ratio. The setup wizard asks for
  // it every run, so it is carried here only to be handed over on a mode change.
  float taper;
};

inline long modeSettingRead(const ModeSettings* s, const char* key) {
  if (!strcmp(key, "tps")) return s->defaultPasses;
  if (!strcmp(key, "spp")) return s->springPasses;
  if (!strcmp(key, "safe")) return s->clearanceDu;
  if (!strcmp(key, "pck")) return s->peckDu;
  if (!strcmp(key, "slt")) return s->slotReductionDu;
  if (!strcmp(key, "fli")) return s->flankInfeed ? 1 : 0;
  return 0;
}

// Returns NULL on success or the message to show. The ranges are the point of this function
// living where a test can reach it: a clearance of zero would put the tool back into the work on
// the return stroke, and a pass count of zero would divide by it.
inline const char* modeSettingWrite(ModeSettings* s, const char* key, long value) {
  if (!strcmp(key, "tps")) {
    if (value < 1 || value > MODE_PASSES_MAX) return "Must be 1 or above";
    s->defaultPasses = value;
    // Take effect now rather than at the next restart. Setting a default and watching the mode go
    // on using the old count would read as the setting not having been saved.
    s->passes = value;
  } else if (!strcmp(key, "spp")) {
    if (value < 0 || value > MODE_SPRING_PASSES_MAX) return "Must be 0 to 99";
    s->springPasses = value;
  } else if (!strcmp(key, "safe")) {
    if (value <= 0) return "Must be above 0";
    s->clearanceDu = value;
  } else if (!strcmp(key, "pck")) {
    if (value < 0) return "Must be 0 or above";
    s->peckDu = value;
  } else if (!strcmp(key, "slt")) {
    if (value < 0) return "Must be 0 or above";
    s->slotReductionDu = value;
  } else if (!strcmp(key, "fli")) {
    s->flankInfeed = value != 0;
  } else {
    return "Not a stored setting";
  }
  return NULL;
}

#endif // MODE_SETTINGS_H
