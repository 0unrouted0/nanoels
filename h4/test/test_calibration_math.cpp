// Host tests for the pure arithmetic in ../calibration_math.h, which the firmware calls
// directly - these exercise the shipped code, not a copy of it.
//
// Run with test/run_tests.ps1 (needs Visual Studio's cl.exe, no other dependencies).

#include <cstdio>
#include <cstring>
#include "../calibration_math.h"
#include "../settings_table.h"
#include "../pass_math.h"
#include "../encoder_health.h"
#include "../beeper.h"
#include "../aux_pins.h"
#include "../indexing.h"
#include "../mode_settings.h"
#include "../modes.h"
#include "../lcd_line.h"
#include "../setup_line.h"

static int checks = 0;
static int failures = 0;
static const char* section = "";

static void group(const char* name) {
  section = name;
  printf("\n%s\n", name);
}

static void expectL(const char* what, long got, long want) {
  checks++;
  if (got == want) {
    printf("  ok   %s\n", what);
  } else {
    failures++;
    printf("  FAIL %s: got %ld, want %ld\n", what, got, want);
  }
}

static void expectB(const char* what, bool got, bool want) {
  checks++;
  if (got == want) {
    printf("  ok   %s\n", what);
  } else {
    failures++;
    printf("  FAIL %s: got %s, want %s\n", what, got ? "true" : "false", want ? "true" : "false");
  }
}

// Several derived figures are genuine fractions - an X axis at 2400 steps on a 2mm screw resolves
// 8.33 deci-microns - so rounding them to compare would hide exactly the precision being checked.
static void expectF(const char* what, double got, double want, double tol) {
  checks++;
  if (got >= want - tol && got <= want + tol) {
    printf("  ok   %s\n", what);
  } else {
    failures++;
    printf("  FAIL %s: got %.5f, want %.5f +-%.5f\n", what, got, want, tol);
  }
}

// Whether the item carrying this suffix in this section stores itself under the expected key.
static bool keyIs(int section, const char* suffix, const char* want) {
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (SETTINGS[i].section != section || strcmp(SETTINGS[i].prefKey, suffix) != 0) continue;
    char buf[16];
    settingKeyName(i, buf);
    return strcmp(buf, want) == 0;
  }
  return false; // no such item, which is itself a failure worth reporting
}

// A representative Z axis: 2mm lead screw, 800 steps/rev, so one step is 25 deci-microns.
static const float Z_PITCH = 20000.0f;
static const float Z_STEPS = 800.0f;

static void testGeometry() {
  group("axis geometry");
  expectL("one screw revolution is one pitch", calStepsToDu(Z_PITCH, Z_STEPS, 800), 20000);
  expectL("one step is 25du", calStepsToDu(Z_PITCH, Z_STEPS, 1), 25);
  expectL("negative travel keeps its sign", calStepsToDu(Z_PITCH, Z_STEPS, -800), -20000);
  expectL("du back to steps", calDuToSteps(Z_PITCH, Z_STEPS, 20000), 800);
  expectL("10mm of travel", calDuToSteps(Z_PITCH, Z_STEPS, 100000), 4000);

  // Round trip on exact multiples of one step must be lossless, or positions would drift.
  bool roundTripClean = true;
  for (long steps = -5000; steps <= 5000; steps += 137) {
    if (calDuToSteps(Z_PITCH, Z_STEPS, calStepsToDu(Z_PITCH, Z_STEPS, steps)) != steps) {
      roundTripClean = false;
    }
  }
  expectB("steps -> du -> steps round trips", roundTripClean, true);

  // Motor to lead screw pulley ratio.
  expectL("direct coupled motor", (long) calStepsPerScrewRev(800, 1, 1), 800);
  expectL("3:1 reduction triples the steps", (long) calStepsPerScrewRev(800, 1, 3), 2400);
  expectL("2:1 reduction doubles them", (long) calStepsPerScrewRev(800, 10, 20), 1600);
  expectL("geared the other way", (long) calStepsPerScrewRev(800, 2, 1), 400);
  expectL("only the ratio matters", (long) calStepsPerScrewRev(800, 20, 60), 2400);
  // Nonsense teeth must not zero the axis, which would divide by zero downstream.
  expectL("zero teeth is ignored", (long) calStepsPerScrewRev(800, 0, 3), 800);

  expectL("backlash 0.65mm in steps", calBacklashSteps(6500, Z_STEPS, Z_PITCH), 260);
  expectL("no backlash is no steps", calBacklashSteps(0, Z_STEPS, Z_PITCH), 0);
  expectL("300mm of travel in steps", calEstopSteps(300, Z_PITCH, Z_STEPS), 120000);

  // Only needs to be positive and monotonic in speed; the exact count is an implementation detail.
  long slow = calDecelerateSteps(2000, 1000, 30000);
  long fast = calDecelerateSteps(4000, 1000, 30000);
  expectB("deceleration ramp is non-empty", slow > 0, true);
  expectB("faster moves decelerate over more steps", fast > slow, true);
}

static void testPitchCalibration() {
  group("screw pitch calibration");
  expectL("perfect measurement leaves pitch alone", calCorrectedPitch(Z_PITCH, 100000, 100000), 20000);
  expectL("axis overtravelled 1% -> pitch up 1%", calCorrectedPitch(Z_PITCH, 101000, 100000), 20200);
  expectL("axis undertravelled 1% -> pitch down 1%", calCorrectedPitch(Z_PITCH, 99000, 100000), 19800);
  expectL("holds precision over a 100mm test move", calCorrectedPitch(Z_PITCH, 1000500, 1000000), 20010);
  expectL("zero nominal cannot divide", calCorrectedPitch(Z_PITCH, 100000, 0), 0);

  expectB("unchanged pitch is plausible", calPitchPlausible(Z_PITCH, 20000), true);
  expectB("+20% exactly is still accepted", calPitchPlausible(Z_PITCH, 24000), true);
  expectB("just past +20% is rejected", calPitchPlausible(Z_PITCH, 24001), false);
  expectB("-20% exactly is still accepted", calPitchPlausible(Z_PITCH, 16000), true);
  expectB("just past -20% is rejected", calPitchPlausible(Z_PITCH, 15999), false);
  // A decimal point in the wrong place is the mistake this guard exists to catch.
  expectB("10x typo is rejected", calPitchPlausible(Z_PITCH, 200000), false);
}

static void testTravelAndSpeed() {
  group("travel limits and speed ramp");
  expectL("300mm span", calTravelMm(3000000), 300);
  // Truncating rather than rounding keeps the stored limit inside the real span.
  expectL("partial mm truncates down", calTravelMm(2999999), 299);
  expectL("sub-millimetre span is zero", calTravelMm(9999), 0);

  expectL("ramp steps up 10%", calSpeedNext(1000), 1100);
  expectL("ramp steps up 10% again", calSpeedNext(1100), 1210);
  // Below 10, speed/10 truncates to zero and the ramp would never advance.
  expectL("tiny speeds still advance", calSpeedNext(5), 6);
  expectL("speed 1 still advances", calSpeedNext(1), 2);

  expectL("backs off to 80%", calSpeedBackoff(1000, 100), 800);
  expectB("backoff is always below the stall", calSpeedBackoff(1000, 100) < 1000, true);
  expectL("never drops below start speed", calSpeedBackoff(1000, 900), 900);
  expectL("clamps when already at start speed", calSpeedBackoff(100, 100), 100);
}

static void testEncoderPpr() {
  group("encoder PPR");
  // 4x quadrature: both edges of both channels, so 600 PPR is 2400 counts per revolution.
  expectL("counts per pulse is 4x quadrature", ENCODER_COUNTS_PER_PULSE, 4);
  expectL("600ppr from 10 clean revolutions", calPprFromCounts(600 * 4 * 10, 10), 600);
  expectL("600ppr from a single revolution", calPprFromCounts(2400, 1), 600);
  expectL("backwards rotation counts the same", calPprFromCounts(-24000, 10), 600);
  expectL("one count short still rounds to 600", calPprFromCounts(23999, 10), 600);
  expectL("zero revolutions cannot divide", calPprFromCounts(24000, 0), 0);

  expectB("24 is the lowest accepted", calPprValid(24), true);
  expectB("below 24 rejected", calPprValid(23), false);
  expectB("10000 is the highest accepted", calPprValid(10000), true);
  expectB("above 10000 rejected", calPprValid(10001), false);

  group("encoder gearing");
  // Direct drive: 600 PPR at 4x is 2400 counts per spindle revolution.
  expectL("direct drive", calCountsPerSpindleRev(600, 1, 1, 1), 2400);
  // 1:2 belt - spindle pulley twice the encoder pulley, so the encoder turns twice per
  // spindle revolution and the controller sees twice as many counts.
  expectL("1:2 belt doubles the counts", calCountsPerSpindleRev(600, 40, 20, 1), 4800);
  expectL("teeth only matter as a ratio", calCountsPerSpindleRev(600, 2, 1, 1), 4800);
  // Geared the other way, encoder slower than the spindle.
  expectL("2:1 belt halves the counts", calCountsPerSpindleRev(600, 20, 40, 1), 1200);
  // The divider trades resolution for steadiness, and cancels a 1:2 belt exactly.
  expectL("divider halves the counts", calCountsPerSpindleRev(600, 1, 1, 2), 1200);
  expectL("belt and divider cancel out", calCountsPerSpindleRev(600, 40, 20, 2), 2400);
  expectL("divider of 4", calCountsPerSpindleRev(600, 1, 1, 4), 600);
  // A 1000 PPR encoder on a 1:2 belt, which is 8000 counts per spindle revolution before any
  // divider - the setup the RPM and filter limits were sized against.
  expectL("1000ppr geared 1:2", calCountsPerSpindleRev(1000, 2, 1, 1), 8000);
  expectL("1000ppr geared 1:2, divider 2", calCountsPerSpindleRev(1000, 2, 1, 2), 4000);
  // Nonsense settings must not produce a zero or negative revolution, which would divide by
  // zero everywhere downstream.
  expectL("zero pulley teeth is ignored", calCountsPerSpindleRev(600, 1, 0, 1), 2400);
  expectB("never returns less than one", calCountsPerSpindleRev(24, 1, 100, 1) >= 1, true);
  expectB("huge divider still returns one", calCountsPerSpindleRev(600, 1, 1, 100) >= 1, true);

  group("encoder PPR");
  expectL("exact value is left alone", calSnapPpr(600), 600);
  expectL("small error snaps to 600", calSnapPpr(597), 600);
  expectL("2% error exactly still snaps", calSnapPpr(588), 600);
  expectL("beyond 2% is kept as measured", calSnapPpr(587), 587);
  expectL("1024 line encoders snap", calSnapPpr(1022), 1024);
  expectL("nothing near 777 to snap to", calSnapPpr(777), 777);
}

static void testSpindleModulo() {
  group("spindle angle wrapping");
  const int steps = 600 * ENCODER_COUNTS_PER_PULSE; // one revolution at 600 PPR
  expectL("zero stays zero", calSpindleModulo(0, steps), 0);
  expectL("a full turn wraps to zero", calSpindleModulo(steps, steps), 0);
  expectL("just past a full turn", calSpindleModulo(steps + 1, steps), 1);
  expectL("many turns wrap", calSpindleModulo(steps * 7 + 42, steps), 42);
  expectL("negative wraps forward", calSpindleModulo(-1, steps), steps - 1);
  expectL("negative full turn is zero", calSpindleModulo(-steps, steps), 0);
  expectL("negative beyond a turn", calSpindleModulo(-steps - 1, steps), steps - 1);

  bool alwaysInRange = true;
  for (long v = -5000; v <= 5000; v += 7) {
    long m = calSpindleModulo(v, steps);
    if (m < 0 || m >= steps) {
      alwaysInRange = false;
    }
  }
  expectB("result is always within one turn", alwaysInRange, true);
}

static void testRpmAccumulator() {
  group("RPM accumulator");
  const long bulk = 1200;
  int index = 0;

  expectB("a partial revolution does not complete", calRpmAccumulate(&index, 500, bulk), false);
  expectB("reaching the bulk completes", calRpmAccumulate(&index, 700, bulk), true);
  expectL("and leaves nothing behind", index, 0);

  // Regression: the overshoot used to be discarded by zeroing the index, losing counts at
  // speed where several arrive per poll.
  index = 1199;
  expectB("overshooting completes", calRpmAccumulate(&index, 5, bulk), true);
  expectL("overshoot is carried, not dropped", index, 4);

  // Regression: accumulating abs(delta) made a spindle dithering on an encoder edge look
  // like it was spinning, inflating the RPM readout.
  index = 0;
  bool anyCompleted = false;
  for (int i = 0; i < 4000; i++) {
    if (calRpmAccumulate(&index, (i % 2 == 0) ? 1 : -1, bulk)) {
      anyCompleted = true;
    }
  }
  expectB("jitter never completes a revolution", anyCompleted, false);
  expectB("jitter leaves the index near zero", calAbsL(index) <= 1, true);

  // A spindle genuinely running in reverse must still produce an RPM.
  index = 0;
  expectB("reverse rotation completes", calRpmAccumulate(&index, -1200, bulk), true);
  expectL("reverse leaves nothing behind", index, 0);

  // Many small deltas must total the same as one big one.
  index = 0;
  int completions = 0;
  for (int i = 0; i < 240; i++) {
    if (calRpmAccumulate(&index, 5, bulk)) {
      completions++;
    }
  }
  expectL("240 x 5 counts is exactly one revolution", completions, 1);

  group("RPM from timing");
  expectL("60ms per revolution is 1000rpm", calRpmFromBulkMicros(60000), 1000);
  expectL("600ms per revolution is 100rpm", calRpmFromBulkMicros(600000), 100);
  expectL("no timing yet reads zero", calRpmFromBulkMicros(0), 0);
}

static void testBigDroLayout() {
  group("large display layout");
  const int numCols = 14; // columns 14-19 are the info strip

  expectL("metric under 100mm gets 0.01", calBigDroPoints(true, 12.345f), 2);
  expectL("metric at 100mm steps to 0.1", calBigDroPoints(true, 100.0f), 1);
  expectL("metric over 100mm stays at 0.1", calBigDroPoints(true, 287.5f), 1);
  expectL("imperial under 10in gets 0.001", calBigDroPoints(false, 1.234f), 3);
  expectL("imperial at 10in steps to 0.01", calBigDroPoints(false, 10.0f), 2);

  expectL("four digits and a point", calBigDroWidth("12.34"), 13);
  expectL("three digits and a point", calBigDroWidth("5.25"), 10);
  expectL("hundreds still four digits", calBigDroWidth("123.4"), 13);
  expectL("zero", calBigDroWidth("0.00"), 10);

  // The whole point of the fixed 4-digit format: the number can never reach column 14.
  const char* samples[] = {"0.00", "5.25", "12.34", "99.99", "100.0", "287.5", "1.234", "10.00"};
  bool alwaysClear = true;
  bool onesColumnStable = true;
  int firstOnesEnd = -1;
  for (int i = 0; i < 8; i++) {
    int width = calBigDroWidth(samples[i]);
    int start = calBigDroStartCol(numCols, width);
    if (start + width > numCols) {
      alwaysClear = false;
    }
    // Right-aligned, so every value ends on the same column.
    if (firstOnesEnd < 0) {
      firstOnesEnd = start + width;
    } else if (start + width != firstOnesEnd) {
      onesColumnStable = false;
    }
  }
  expectB("digits never run into the info strip", alwaysClear, true);
  expectB("every value ends on the same column", onesColumnStable, true);

  expectL("normal value starts at column 1", calBigDroStartCol(numCols, 13), 1);
  expectL("short value is pushed right", calBigDroStartCol(numCols, 10), 4);
  // An over-wide value clamps rather than writing left of the sign column.
  expectL("oversized value clamps to column 1", calBigDroStartCol(numCols, 19), 1);
}

static void testDerivedFigures() {
  group("derived figures");
  // Z: 2mm screw, 800 steps -> 25 deci-microns, exactly 2.5 microns, per step.
  expectF("Z resolution in du", calStepResolutionDu(Z_PITCH, Z_STEPS), 25.0, 0.001);
  expectF("Z steps per mm", calStepsPerMm(Z_PITCH, Z_STEPS), 400.0, 0.001);
  // X through a 3:1 reduction resolves three times finer, and no longer divides evenly.
  expectF("X resolution in du", calStepResolutionDu(20000.0f, 2400.0f), 8.3333, 0.001);
  expectF("X steps per mm", calStepsPerMm(20000.0f, 2400.0f), 1200.0, 0.001);
  // A finer screw resolves finer for the same motor.
  expectF("1mm screw halves the step", calStepResolutionDu(10000.0f, 800.0f), 12.5, 0.001);
  expectF("a zero motor cannot resolve", calStepResolutionDu(Z_PITCH, 0), 0.0, 0.0001);
  expectF("a zero screw has no steps per mm", calStepsPerMm(0, Z_STEPS), 0.0, 0.0001);

  // 2000 steps/s on the Z above is 50000 du/s, so 3,000,000 du (300mm) per minute.
  expectL("max feed", calMaxFeedDuPerMin(2000, Z_PITCH, Z_STEPS), 3000000);
  expectL("no motor, no feed", calMaxFeedDuPerMin(2000, Z_PITCH, 0), 0);

  // The number that matters when threading: 300mm/min of Z at a 1mm pitch is 300 rpm, and a
  // coarser pitch lowers the ceiling proportionally.
  expectL("max rpm at 1mm pitch", calMaxRpmForPitch(2000, Z_PITCH, Z_STEPS, 10000), 300);
  expectL("a 3mm pitch thirds it", calMaxRpmForPitch(2000, Z_PITCH, Z_STEPS, 30000), 100);
  expectL("a fine 0.5mm pitch doubles it", calMaxRpmForPitch(2000, Z_PITCH, Z_STEPS, 5000), 600);
  // Left-hand threads are the same speed problem as right-hand ones.
  expectL("sign of the pitch is irrelevant", calMaxRpmForPitch(2000, Z_PITCH, Z_STEPS, -30000), 100);
  expectL("no pitch means no limit to report", calMaxRpmForPitch(2000, Z_PITCH, Z_STEPS, 0), 0);

  // The worked example from the ENCODER_FILTER comment in machine_config.h.
  expectL("1000 PPR at filter 200", calMaxEncoderRpm(1000, 200), 12000);
  expectL("600 PPR allows more", calMaxEncoderRpm(600, 200), 20000);
  expectL("a heavier filter allows less", calMaxEncoderRpm(1000, 400), 6000);
  expectL("nonsense PPR reports nothing", calMaxEncoderRpm(0, 200), 0);
  // Geared up 1:2 the encoder spins twice as fast, so the spindle hits the ceiling at half.
  expectL("belted encoder halves the spindle ceiling",
          calMaxSpindleRpm(1000, 200, 2, 1), 6000);
  expectL("direct drive is unchanged", calMaxSpindleRpm(1000, 200, 1, 1), 12000);
  expectL("geared down raises it", calMaxSpindleRpm(1000, 200, 1, 2), 24000);

  // 1000 PPR read 4x is 4000 counts, so a shade under a tenth of a degree.
  expectF("angular resolution", calAngularResolutionDeg(4000), 0.09, 0.0001);
  expectF("a coarse encoder sees less", calAngularResolutionDeg(400), 0.9, 0.0001);
  expectF("no counts, no resolution", calAngularResolutionDeg(0), 0.0, 0.0001);

  // Stopping distance is the deceleration ramp measured in millimetres rather than steps.
  long rampSteps = calDecelerateSteps(2000, 800, 25000);
  expectL("stop distance follows the ramp",
          calStopDistanceDu(2000, 800, 25000, Z_PITCH, Z_STEPS),
          calRoundL(rampSteps * 25.0));
  expectL("no motor, no distance", calStopDistanceDu(2000, 800, 25000, Z_PITCH, 0), 0);
}

static void testSettingsTable() {
  group("settings table");
  expectL("item count", SETTINGS_COUNT, 90);
  expectL("section count", (int)SECTION_COUNT, 9);

  // Navigating a section is a first index and a count, which only works if a section's items
  // are one unbroken run of the table.
  bool contiguous = true;
  bool allPopulated = true;
  int covered = 0;
  for (int s = 0; s < SECTION_COUNT; s++) {
    int first = settingSectionFirst(s);
    int count = settingSectionCount(s);
    if (first < 0 || count < 1) { allPopulated = false; continue; }
    covered += count;
    for (int i = first; i < first + count; i++) {
      if (i >= SETTINGS_COUNT || SETTINGS[i].section != s) contiguous = false;
    }
  }
  expectB("every section has items", allPopulated, true);
  expectB("each section is one unbroken run", contiguous, true);

  // Mode-scoped items are not in any directory section - they live on the mode's own page - so
  // between them the sections and the modes have to account for every row and no row twice.
  bool modesContiguous = true;
  int modeCovered = 0;
  for (int m = SMODE_NONE + 1; m < SMODE_COUNT; m++) {
    int first = settingModeFirst(m);
    int count = settingModeCount(m);
    if (first < 0 || count < 1) continue; // a mode is allowed to have no settings of its own
    modeCovered += count;
    for (int i = first; i < first + count; i++) {
      if (i >= SETTINGS_COUNT || SETTINGS[i].mode != m) modesContiguous = false;
    }
  }
  expectB("each mode is one unbroken run", modesContiguous, true);
  expectL("sections and modes cover every item exactly once", covered + modeCovered, SETTINGS_COUNT);

  // The two prefixes share one key namespace, so an item must not claim both.
  bool scopeExclusive = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (settingIsModeScoped(i) && settingIsAxis(i)) scopeExclusive = false;
    // A mode-scoped row must be on the mode page, and only those rows may be.
    if (settingIsModeScoped(i) != (SETTINGS[i].section == SEC_MODE)) scopeExclusive = false;
  }
  expectB("an item is scoped to an axis or a mode, never both", scopeExclusive, true);

  // Several modes deliberately share a suffix - every pass mode has "tps" - so the mode letter is
  // the only thing keeping their stored values apart. If it ever returned 0 they would collide.
  bool modeLetters = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (settingIsModeScoped(i) && settingModeLetter(i) == 0) modeLetters = false;
  }
  expectB("every mode-scoped item has a letter", modeLetters, true);

  // Axis and mode prefixes share one key namespace, and case is the only thing separating them:
  // the cone taper is stored under "ccr" while the C axis would claim "Ccr". If a mode letter
  // ever went upper case the two could collide, and the collision would be silent - the loser
  // simply reads back the winner's value.
  bool casesDistinct = true;
  for (int m = SMODE_NONE + 1; m < SMODE_COUNT; m++) {
    int i = settingModeFirst(m);
    if (i < 0) continue;
    char c = settingModeLetter(i);
    if (c < 'a' || c > 'z') casesDistinct = false;
  }
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    char c = settingAxisLetter(i);
    if (c != 0 && (c < 'A' || c > 'Z')) casesDistinct = false;
  }
  expectB("mode letters are lower case, axis letters upper", casesDistinct, true);

  // Two modes sharing a letter would share their whole key namespace.
  bool lettersUnique = true;
  for (int a = SMODE_NONE + 1; a < SMODE_COUNT; a++) {
    int ia = settingModeFirst(a);
    if (ia < 0) continue;
    for (int b = a + 1; b < SMODE_COUNT; b++) {
      int ib = settingModeFirst(b);
      if (ib < 0) continue;
      if (settingModeLetter(ia) == settingModeLetter(ib)) lettersUnique = false;
    }
  }
  expectB("no two modes share a letter", lettersUnique, true);

  // A per-axis item has to say which axis, and a global one must not claim an axis - the axis
  // used to be inferred from the index being odd or even, which grouping by section broke.
  bool axisConsistent = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    bool named = SETTINGS[i].axis != SAX_NONE;
    if (named != settingIsAxis(i)) axisConsistent = false;
  }
  expectB("axis field agrees with the kind", axisConsistent, true);

  // Units. A toggle and the calibration action have nothing to measure; a distance follows the
  // metric/inch setting, so its unit cannot be a fixed string in the table.
  bool unitsPlaced = true;
  bool unitsFit = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    const char* u = SETTINGS[i].unit;
    if ((settingIsToggle(i) || settingIsAction(i) || settingIsList(i) || settingUsesDu(i) ||
        settingUsesSpeed(i)) && u != 0) unitsPlaced = false;
    if (u != 0 && (strlen(u) > 8 || u[0] == 0 || u[0] == ' ')) unitsFit = false;
  }
  expectB("only measurable items carry a unit", unitsPlaced, true);
  expectB("units are short enough for the LCD", unitsFit, true);

  // Every plain number should say what it is counting. The WiFi PIN is the one real exception:
  // it is a code, not a quantity. A list is not a quantity either - its value is shown as a name -
  // and a speed's unit follows the metric/inch setting, so neither belongs here.
  int unitless = 0;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (settingIsToggle(i) || settingIsAction(i) || settingIsList(i) || settingUsesDu(i) ||
        settingUsesSpeed(i)) continue;
    if (SETTINGS[i].unit == 0) unitless++;
  }
  expectL("only the PIN is a number without a unit", unitless, 1);

  // A unit named in the label as well as the field would print twice - "Max travel mm ... 300 mm".
  bool labelsUnitFree = true;
  const char* trailing[] = {" mm", " ms", " us", " s", " counts", " steps", " teeth"};
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    const char* label = SETTINGS[i].label;
    size_t n = strlen(label);
    for (int t = 0; t < 7; t++) {
      size_t m = strlen(trailing[t]);
      if (n >= m && strcmp(label + n - m, trailing[t]) == 0) labelsUnitFree = false;
    }
  }
  expectB("no label repeats its own unit", labelsUnitFree, true);

  bool labelsSane = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    const char* label = SETTINGS[i].label;
    if (label == 0 || label[0] == 0 || label[0] == ' ') labelsSane = false;
    if ((int)strlen(label) > 20) labelsSane = false;
  }
  expectB("labels are present and fit the screen", labelsSane, true);

  // Every row's kind has to be one the panel knows how to draw and edit. Adding a kind to the
  // enum without teaching updateSettingsDisplay() and processSettingsKeypress() about it produces
  // an item that shows a bare number and cannot be changed - visible on the web page, which falls
  // back to a text box, but dead on the machine. This fails the build instead.
  bool kindsHandled = true;
  bool everyItemEditable = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    switch (SETTINGS[i].kind) {
      case SETTING_AXIS_BOOL: case SETTING_AXIS_DU: case SETTING_AXIS_NUM:
      case SETTING_GLOBAL_BOOL: case SETTING_GLOBAL_DU: case SETTING_GLOBAL_NUM:
      case SETTING_GLOBAL_LIST: case SETTING_GLOBAL_SPEED: case SETTING_ACTION:
        break;
      default:
        kindsHandled = false;
    }
    // Exactly one way to change each item: toggles and lists take ON or +/-, actions open a
    // screen, everything else is typed. An item matching none of those cannot be edited at all.
    int ways = (settingIsToggle(i) ? 1 : 0) + (settingIsList(i) ? 1 : 0) + (settingIsAction(i) ? 1 : 0)
             + (settingUsesDu(i) ? 1 : 0) + (settingUsesSpeed(i) ? 1 : 0);
    bool plainNumber = SETTINGS[i].kind == SETTING_AXIS_NUM || SETTINGS[i].kind == SETTING_GLOBAL_NUM;
    if (ways != (plainNumber ? 0 : 1)) everyItemEditable = false;
  }
  expectB("every kind is one the panel can draw", kindsHandled, true);
  expectB("every item has exactly one way to edit it", everyItemEditable, true);

  bool sectionNamesFit = true;
  for (int s = 0; s < SECTION_COUNT; s++) {
    if (SECTION_NAMES[s] == 0 || strlen(SECTION_NAMES[s]) == 0) sectionNamesFit = false;
    if (strlen(SECTION_NAMES[s]) > 18) sectionNamesFit = false; // room for the cursor
  }
  expectB("section names fit beside the cursor", sectionNamesFit, true);

  // A toggle shows its own wording, so it needs both halves or neither.
  bool togglesLabelled = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    bool both = SETTINGS[i].onLabel != 0 && SETTINGS[i].offLabel != 0;
    bool neither = SETTINGS[i].onLabel == 0 && SETTINGS[i].offLabel == 0;
    if (!both && !neither) togglesLabelled = false;
    if (!settingIsToggle(i) && both) togglesLabelled = false;
  }
  expectB("toggle wording is complete", togglesLabelled, true);

  // Actions open another screen rather than holding a value. Their key is not storage, it is
  // which screen: the sketch dispatches on it, so two actions sharing one would open the wrong
  // thing, and an unknown one falls through to calibration - which moves an axis.
  int actionCount = 0;
  bool actionKeysKnown = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (!settingIsAction(i)) continue;
    actionCount++;
    const char* k = SETTINGS[i].prefKey;
    if (strcmp(k, "") != 0 && strcmp(k, "thr") != 0) actionKeysKnown = false;
  }
  expectL("the actions are calibration and the two thread databases", actionCount, 3);
  expectB("every action names a screen the sketch knows", actionKeysKnown, true);

  bool kindsExclusive = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (settingUsesDu(i) && (settingIsToggle(i) || settingIsAction(i))) kindsExclusive = false;
  }
  expectB("kinds are mutually exclusive", kindsExclusive, true);

  // Anything that holds a value needs somewhere to store it. Actions are exempt: their key names
  // a screen instead, and may be empty.
  bool keysPresent = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (settingIsAction(i)) continue;
    if (SETTINGS[i].prefKey == 0 || SETTINGS[i].prefKey[0] == 0) keysPresent = false;
  }
  expectB("every storable item has a key", keysPresent, true);

  // Every per-axis item must resolve to a letter, or its stored key collides with the same suffix
  // on another axis. This is what index-parity derivation silently got wrong for A1, which has no
  // parity that could ever have produced 'C'.
  bool lettersPresent = true;
  bool lettersKnown = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    char letter = settingAxisLetter(i);
    if (settingIsAxis(i) != (letter != 0)) lettersPresent = false;
    if (letter != 0 && letter != 'Z' && letter != 'X' && letter != 'C') lettersKnown = false;
  }
  expectB("axis items have a letter, globals have none", lettersPresent, true);
  expectB("letters are only Z, X or C", lettersKnown, true);

  bool a1Lettered = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (SETTINGS[i].axis == SAX_A1 && settingAxisLetter(i) != 'C') a1Lettered = false;
  }
  expectB("A1 items are reachable as C", a1Lettered, true);

  // Two items sharing a storage key would silently overwrite each other. Per-axis items are
  // qualified by the axis letter, which is how three axes reuse the same suffixes. Built with the
  // firmware's own settingKeyName() - restating the composition here is what let the parity bug
  // live behind a passing uniqueness check.
  // Sized from the table rather than from a number someone has to remember to raise: at 80 this
  // overran its stack the moment the per-mode rows landed, and a buffer overrun is a much worse
  // way to learn the table grew than a failed assertion.
  char qualified[SETTINGS_COUNT][20];
  int count = 0;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (settingIsAction(i)) continue;
    settingKeyName(i, qualified[count]);
    count++;
  }
  bool keysUnique = true;
  bool keysFit = true;
  for (int i = 0; i < count; i++) {
    if (strlen(qualified[i]) > 15) keysFit = false; // NVS key limit
    for (int j = i + 1; j < count; j++) {
      if (strcmp(qualified[i], qualified[j]) == 0) keysUnique = false;
    }
  }
  expectB("no two items share a storage key", keysUnique, true);
  expectB("storage keys fit the NVS key limit", keysFit, true);

  // Spot-check the actual strings. The properties above would all still hold if the letters were
  // simply shuffled, so pin down what a few keys really spell - these are the names a saved backup
  // contains and what a restore has to match.
  // The WiFi PIN doubles as the WPA2 password, where too short means an open network rather than
  // an error, so the boundaries are the whole safety mechanism.
  expectB("8 digits is accepted", calWifiPinValid(13572468), true);
  expectB("the smallest 8-digit PIN", calWifiPinValid(10000000), true);
  expectB("the largest 8-digit PIN", calWifiPinValid(99999999), true);
  expectB("7 digits is refused", calWifiPinValid(9999999), false);
  expectB("9 digits is refused", calWifiPinValid(100000000), false);
  expectB("a leading zero would be lost", calWifiPinValid(1234567), false);
  expectB("zero is refused", calWifiPinValid(0), false);
  expectB("negative is refused", calWifiPinValid(-13572468), false);

  expectB("Z screw pitch is Zscr", keyIs(SEC_Z, "scr", "Zscr"), true);
  expectB("X screw pitch is Xscr", keyIs(SEC_X, "scr", "Xscr"), true);
  expectB("A1 screw pitch is Cscr", keyIs(SEC_A1, "scr", "Cscr"), true);
  expectB("a global keeps its bare suffix", keyIs(SEC_ENCODER, "eppr", "eppr"), true);

  // Calibration is the last thing in the menu, as the only item that opens a screen.
  expectB("the last item is the calibration action", settingIsAction(SETTINGS_COUNT - 1), true);
  // Preferences is what the directory opens on, so it has to be the first section.
  expectL("preferences comes first", (int)SEC_PREFS, 0);
  expectL("preferences holds what belongs to the machine", settingSectionCount(SEC_PREFS), 11);
}

static void testSlotting() {
  group("slotting");
  // Four passes from X=0 to X=-2mm of depth: every stroke cuts, and the last lands on the stop.
  expectL("first pass is already cutting", slotDepthPos(0, -2000, 4, 1), -500);
  expectL("second pass", slotDepthPos(0, -2000, 4, 2), -1000);
  expectL("third pass", slotDepthPos(0, -2000, 4, 3), -1500);
  expectL("last pass lands on the stop", slotDepthPos(0, -2000, 4, 4), -2000);
  // A single pass goes straight to full depth.
  expectL("one pass cuts it all", slotDepthPos(0, -2000, 1, 1), -2000);
  // Cutting the other way, e.g. an internal slot fed outwards.
  expectL("outward first pass", slotDepthPos(0, 2000, 4, 1), 500);
  expectL("outward last pass", slotDepthPos(0, 2000, 4, 4), 2000);
  expectL("no passes cannot move", slotDepthPos(100, -2000, 0, 1), 100);

  // Blind-slot shortening. The stroke runs from the right stop to the left stop, and left is the
  // larger coordinate on this machine, so the far end is above the near end and the reduction
  // pulls it back down.
  expectL("full length when off", slotStrokeEnd(0, 5000, 0, 1), 5000);
  expectL("still full length on pass 9", slotStrokeEnd(0, 5000, 0, 9), 5000);
  // With reduction on, pass 1 is full length and each later one stops 100 steps earlier.
  expectL("first stroke is full length", slotStrokeEnd(0, 5000, 100, 1), 5000);
  expectL("second stops short", slotStrokeEnd(0, 5000, 100, 2), 4900);
  expectL("third stops shorter", slotStrokeEnd(0, 5000, 100, 3), 4800);
  // Enough passes and the shortening would reach past the start - it has to clamp, or the stroke
  // would reverse and cut backwards into the near stop.
  expectL("clamped at the near stop", slotStrokeEnd(0, 5000, 100, 60), 0);
  expectL("clamped well past it too", slotStrokeEnd(0, 5000, 100, 500), 0);
}

static void testPassDepth() {
  group("pass depth");
  // 4 passes from the surface (0) down to 1mm of infeed.
  expectL("first pass of four", passDepthPos(0, 10000, 4, 1), 2500);
  expectL("second pass of four", passDepthPos(0, 10000, 4, 2), 5000);
  expectL("third pass of four", passDepthPos(0, 10000, 4, 3), 7500);
  expectL("last pass reaches full depth", passDepthPos(0, 10000, 4, 4), 10000);
  expectL("a single pass goes straight to depth", passDepthPos(0, 10000, 1, 1), 10000);

  // The invariant that matters most: whatever the per-pass division truncates to, the final
  // pass must land exactly on the end stop or the part finishes oversize.
  bool finalExact = true;
  bool monotonic = true;
  bool withinStops = true;
  for (long total = 1; total <= 40; total++) {
    for (long start = -3000; start <= 3000; start += 1500) {
      long end = start + 7333; // indivisible by most pass counts
      if (passDepthPos(start, end, total, total) != end) finalExact = false;
      long prev = start;
      for (long p = 1; p <= total; p++) {
        long d = passDepthPos(start, end, total, p);
        if (d < prev) monotonic = false;
        if (d < start || d > end) withinStops = false;
        prev = d;
      }
    }
  }
  expectB("final pass always lands exactly on the end stop", finalExact, true);
  expectB("depth never goes backwards between passes", monotonic, true);
  expectB("depth never overshoots the stops", withinStops, true);

  // Cutting the other way round, which is how the aux axis runs when auxForward is false.
  expectL("reversed direction first pass", passDepthPos(0, -10000, 4, 1), -2500);
  expectL("reversed direction final pass", passDepthPos(0, -10000, 4, 4), -10000);
  bool reversedExact = true;
  for (long total = 1; total <= 40; total++) {
    if (passDepthPos(500, 500 - 7333, total, total) != 500 - 7333) reversedExact = false;
  }
  expectB("reversed final pass is exact too", reversedExact, true);

  expectL("degenerate pass count returns the end stop", passDepthPos(0, 10000, 0, 0), 10000);
}

static void testPassSequencing() {
  group("pass sequencing");
  expectL("plain four pass operation", passTotalSteps(4, 0, 1), 4);
  expectL("spring passes extend it", passTotalSteps(4, 2, 1), 6);
  expectL("two start thread doubles every pass", passTotalSteps(4, 0, 2), 8);
  expectL("starts and spring passes together", passTotalSteps(4, 2, 2), 12);

  expectL("single start maps index to pass", passNumberForIndex(1, 1, 4), 1);
  expectL("single start pass three", passNumberForIndex(3, 1, 4), 3);
  // With 2 starts, indices 1 and 2 are the same depth cut on opposite thread starts.
  expectL("two starts share pass one", passNumberForIndex(1, 2, 4), 1);
  expectL("two starts share pass one again", passNumberForIndex(2, 2, 4), 1);
  expectL("two starts move to pass two", passNumberForIndex(3, 2, 4), 2);
  expectL("two starts still pass two", passNumberForIndex(4, 2, 4), 2);

  // Spring passes clamp, which is what makes them re-cut at final depth instead of deeper.
  expectL("spring pass reports the last pass", passNumberForIndex(5, 1, 4), 4);
  expectL("later spring pass still clamps", passNumberForIndex(6, 1, 4), 4);
  expectB("index four is not a spring pass", passIsSpring(4, 1, 4), false);
  expectB("index five is a spring pass", passIsSpring(5, 1, 4), true);
  expectB("two start spring pass detected", passIsSpring(9, 2, 4), true);
  expectB("two start last real pass is not", passIsSpring(8, 2, 4), false);

  // A spring pass must cut at exactly the same depth as the final pass, not deeper.
  long finalDepth = passDepthPos(0, 10000, 4, passNumberForIndex(4, 1, 4));
  long springDepth = passDepthPos(0, 10000, 4, passNumberForIndex(6, 1, 4));
  expectL("spring pass repeats the final depth", springDepth, finalDepth);

  expectL("single start needs no offset", passStartOffset(1, 1200.0f), 0);
  expectL("two starts are half a turn apart", passStartOffset(2, 1200.0f), 600);
  expectL("three starts split the turn", passStartOffset(3, 1200.0f), 400);
  expectL("four starts split the turn", passStartOffset(4, 1200.0f), 300);
}

static void testFlankInfeed() {
  group("thread flank infeed");
  const float encSteps = 1200.0f;
  const long pitch = 10000; // 1mm

  // The shift is proportional to remaining depth, so the finishing pass must not be shifted at
  // all - that is what leaves the last cut symmetric on both flanks.
  expectL("final pass has no flank shift", passFlankShift(10000, 4, 4, encSteps, pitch), 0);
  expectB("earlier passes are shifted", passFlankShift(10000, 4, 1, encSteps, pitch) != 0, true);

  bool shrinking = true;
  long prev = passFlankShift(10000, 6, 1, encSteps, pitch);
  for (long p = 2; p <= 6; p++) {
    long s = passFlankShift(10000, 6, p, encSteps, pitch);
    if (s > prev) shrinking = false;
    prev = s;
  }
  expectB("shift shrinks as depth increases", shrinking, true);
  expectL("and reaches zero on the last pass", prev, 0);

  // tan(29.5) * 10000du * 1200 steps / 10000du per rev = 678 for a full-depth first pass.
  expectL("first of two passes shifts by half depth", passFlankShift(10000, 2, 1, encSteps, pitch), 339);
  expectL("negative pitch reverses the shift", passFlankShift(10000, 2, 1, encSteps, -pitch), -339);
  expectL("zero pitch cannot divide", passFlankShift(10000, 2, 1, encSteps, 0), 0);
}

static void testPeck() {
  group("peck parting");
  expectL("next break is one peck deeper", peckNextDepth(1000, 250, true), 1250);
  expectL("cutting the other way", peckNextDepth(1000, 250, false), 750);
  expectL("disabled peck does not advance", peckNextDepth(1000, 0, true), 1000);

  expectL("retracts by the safe distance", peckRetractPos(5000, 200, 0, true), 4800);
  expectL("retracts the other way", peckRetractPos(5000, 200, 10000, false), 5200);
  // Never back out past where the cut started, or the tool leaves the work entirely.
  expectL("clamped at the start of the cut", peckRetractPos(100, 500, 0, true), 0);
  expectL("clamped the other way", peckRetractPos(9900, 500, 10000, false), 10000);
  expectL("exactly at the start stays put", peckRetractPos(500, 500, 0, true), 0);
}

// The dead-band shapes, and the signal-quality window that tells you which one you need.
static void testEncoderHealth() {
  group("encoder dead-band");
  // One-way: the follower snaps forward with the count and lags only on the way back. This is
  // the long-standing behaviour, so it is pinned here rather than merely described.
  expectL("one-way follows a rising count exactly", encFollowDeadband(1000, 990, 8, false), 1000);
  expectL("one-way holds inside the band falling", encFollowDeadband(995, 1000, 8, false), 1000);
  expectL("one-way lags once past the band", encFollowDeadband(980, 1000, 8, false), 988);
  // The reason the shape matters: a single spurious count forward is adopted immediately and the
  // follower then sits ahead of the truth, waiting for real movement to catch up. Two blips in
  // the same direction leave twice the error, which is how it accumulates over a pass.
  long avg = 1000;
  avg = encFollowDeadband(1001, avg, 8, false); // noise count forward
  avg = encFollowDeadband(1000, avg, 8, false); // and back to the truth
  expectL("one-way keeps the offset a forward blip left", avg, 1001);
  avg = encFollowDeadband(1001, avg, 8, false);
  avg = encFollowDeadband(1000, avg, 8, false);
  expectL("one-way blips accumulate", avg, 1001);

  // Symmetric: the same blip is inside the band in both directions, so nothing moves at all.
  avg = 1000;
  avg = encFollowDeadband(1001, avg, 8, true);
  avg = encFollowDeadband(1000, avg, 8, true);
  expectL("symmetric absorbs a forward blip", avg, 1000);
  expectL("symmetric holds inside the band rising", encFollowDeadband(1007, 1000, 8, true), 1000);
  expectL("symmetric moves once past the band rising", encFollowDeadband(1009, 1000, 8, true), 1001);
  expectL("symmetric holds inside the band falling", encFollowDeadband(993, 1000, 8, true), 1000);
  expectL("symmetric moves once past the band falling", encFollowDeadband(991, 1000, 8, true), 999);
  // A zero band has to be a straight pass-through in both shapes, or turning the dead-band off
  // would quietly leave a one-count offset behind.
  expectL("zero band passes through rising", encFollowDeadband(1005, 1000, 0, true), 1005);
  expectL("zero band passes through falling", encFollowDeadband(995, 1000, 0, true), 995);
  expectL("zero band passes through one-way", encFollowDeadband(995, 1000, 0, false), 995);

  group("encoder signal quality");
  EncHealth h;
  encHealthReset(&h, 0);
  expectL("no coherence before the first window", h.coherence, -1);
  expectL("no low-water mark before the first window", h.worstCoherence, -1);

  // 4000 counts/rev at 600 rpm is 40000 counts/sec: 800 counts in a 20ms window, one every 25us.
  // A clean signal travels exactly as far as it moves.
  const long busy = encRateFromRpm(4000, 30);
  expectL("busy threshold in counts per second", busy, 2000);
  unsigned long t = 0;
  bool completed = false;
  for (int i = 0; i < 800; i++) {
    t += 25;
    completed = encHealthAdd(&h, 1, t, 20000, busy, 95);
  }
  expectB("a full window completes", completed, true);
  expectL("clean rotation is fully coherent", h.coherence, 100);
  expectL("clean rotation reports its rate", h.pathRate, 40000);
  expectL("net rate matches path rate when clean", h.netRate, 40000);
  expectL("a clean window is not counted dirty", h.dirtyWindows, 0);
  expectL("low-water mark recorded once busy", h.worstCoherence, 100);

  // Pure dither: the counter travels the same distance but arrives nowhere. This is the case the
  // signed RPM figure the rest of the firmware uses cannot see, because the deltas cancel.
  encHealthReset(&h, 0);
  t = 0;
  for (int i = 0; i < 800; i++) {
    t += 25;
    encHealthAdd(&h, i % 2 == 0 ? 1 : -1, t, 20000, busy, 95);
  }
  expectL("dither is loud", h.pathRate, 40000);
  expectL("dither goes nowhere", h.netRate, 0);
  expectL("dither reads as no coherence", h.coherence, 0);
  expectL("dither is counted dirty", h.dirtyWindows, 1);

  // Real movement with noise on top: 800 counts of travel producing 600 counts of movement.
  encHealthReset(&h, 0);
  t = 0;
  for (int i = 0; i < 800; i++) {
    t += 25;
    encHealthAdd(&h, i % 8 == 7 ? -1 : 1, t, 20000, busy, 95);
  }
  expectL("noisy rotation is partly coherent", h.coherence, 75);
  expectL("noisy rotation is counted dirty", h.dirtyWindows, 1);

  // A spindle turning too slowly to judge: measured and shown, but kept out of the record, or
  // every start and stop would peg the low-water mark at zero.
  encHealthReset(&h, 0);
  t = 0;
  for (int i = 0; i < 20; i++) {
    t += 1000; // one count per millisecond, 1000 counts/sec, well under the threshold
    encHealthAdd(&h, i % 2 == 0 ? 1 : -1, t, 20000, busy, 95);
  }
  expectL("a slow window still reports coherence", h.coherence, 0);
  expectL("a slow window is not counted dirty", h.dirtyWindows, 0);
  expectL("a slow window leaves the low-water mark alone", h.worstCoherence, -1);

  // A coarse encoder gives few counts per window even at speed, and coherence resolves to
  // 100/path percent - so two counts can only ever read 100, 50 or 0. Recording those would tell
  // a 24 PPR machine its signal was flawless however noisy it really was, which is the worst kind
  // of wrong: a diagnostic reporting health because it cannot see.
  encHealthReset(&h, 0);
  t = 0;
  const long coarseBusy = encRateFromRpm(96, 30); // 24 PPR at 4x, the coarsest the firmware takes
  for (int i = 0; i < 60; i++) {
    t += 20000; // one count per window, still well above the busy rate for this encoder
    encHealthAdd(&h, 1, t, 20000, coarseBusy, 95);
  }
  expectB("a sparse window is above the busy rate", h.pathRate >= coarseBusy, true);
  expectL("and still reports its coherence", h.coherence, 100);
  expectL("but is not recorded as flawless", h.worstCoherence, -1);
  expectL("nor counted either way", h.dirtyWindows, 0);

  // The low-water mark has to survive the good windows that follow the bad one, or it would only
  // ever show whatever happened most recently.
  encHealthReset(&h, 0);
  t = 0;
  for (int i = 0; i < 800; i++) {
    t += 25;
    encHealthAdd(&h, i % 8 == 7 ? -1 : 1, t, 20000, busy, 95);
  }
  for (int i = 0; i < 800; i++) {
    t += 25;
    encHealthAdd(&h, 1, t, 20000, busy, 95);
  }
  expectL("current coherence recovers", h.coherence, 100);
  expectL("the low-water mark does not", h.worstCoherence, 75);

  // micros() wraps every 71.6 minutes. Unsigned arithmetic has to carry the window across it,
  // because a threading pass can easily straddle one.
  encHealthReset(&h, 0xFFFFFF00UL);
  t = 0xFFFFFF00UL;
  for (int i = 0; i < 800; i++) {
    t += 25;
    completed = encHealthAdd(&h, 1, t, 20000, busy, 95);
  }
  expectB("a window straddling the rollover completes", completed, true);
  expectL("and measures the same rate", h.pathRate, 40000);

  // Dropping a stale window must not disturb what has already been recorded.
  encHealthReset(&h, 0);
  t = 0;
  for (int i = 0; i < 800; i++) {
    t += 25;
    encHealthAdd(&h, i % 8 == 7 ? -1 : 1, t, 20000, busy, 95);
  }
  encHealthRestartWindow(&h, t + 5000000UL);
  expectL("restarting keeps the low-water mark", h.worstCoherence, 75);
  expectL("restarting keeps the dirty count", h.dirtyWindows, 1);
}

// Plays a pattern through to its end, collecting every tone change. Returns the number of
// changes, which is one per step plus the silence that ends it.
static int beeperRun(int pattern, int* freqs, int max) {
  BeeperState s;
  beeperReset(&s);
  beeperStart(&s, pattern, 1000);
  int n = 0;
  int freq = 0;
  for (unsigned long t = 1000; t < 4000 && n < max; t++) {
    if (beeperTick(&s, t, &freq)) {
      freqs[n++] = freq;
    }
  }
  return n;
}

static void testBeeper() {
  group("buzzer patterns");
  int f[16];

  // The acknowledgement is one tone and then silence - the same single beep every existing
  // caller has always produced.
  expectL("ack is one tone", beeperRun(BEEP_ACK, f, 16), 2);
  expectL("ack tone", f[0], 1000);
  expectL("ack ends silent", f[1], 0);

  // Refused is two low tones with a gap, so a rejected keypress cannot be mistaken for an
  // operation finishing.
  expectL("refused has three steps plus silence", beeperRun(BEEP_REFUSED, f, 16), 4);
  expectL("refused first tone", f[0], 400);
  expectL("refused gap is silent", f[1], 0);
  expectL("refused second tone", f[2], 400);
  expectL("refused ends silent", f[3], 0);

  // Done rises, sync-lost falls. Direction is the cue that survives a noisy shop.
  int n = beeperRun(BEEP_DONE, f, 16);
  expectL("done has two tones plus silence", n, 3);
  expectB("done rises", f[1] > f[0], true);
  n = beeperRun(BEEP_SYNC_LOST, f, 16);
  expectL("sync lost has five steps plus silence", n, 6);
  expectB("sync lost falls", f[4] < f[0], true);

  // A pattern must always end silent, or the buzzer stays on until the next event.
  for (int p = BEEP_ACK; p < BEEP_PATTERN_COUNT; p++) {
    n = beeperRun(p, f, 16);
    expectL("pattern ends silent", f[n - 1], 0);
  }

  // Starting a pattern mid-play replaces it rather than queueing, so the newest event is heard.
  BeeperState s;
  beeperReset(&s);
  beeperStart(&s, BEEP_SYNC_LOST, 0);
  int freq = 0;
  beeperTick(&s, 0, &freq);
  expectL("playing the first pattern", freq, 1200);
  beeperStart(&s, BEEP_INDEX, 10);
  beeperTick(&s, 10, &freq);
  expectL("replaced by the newer one", freq, 1600);

  // Silence has to be reported as a change so the caller knows to call noTone(), and the machine
  // must then go quiet rather than repeating.
  beeperReset(&s);
  beeperStart(&s, BEEP_INDEX, 0);
  beeperTick(&s, 0, &freq);
  expectB("nothing to do mid-tone", beeperTick(&s, 30, &freq), false);
  expectB("the end of the tone is a change", beeperTick(&s, 60, &freq), true);
  expectL("and it is silence", freq, 0);
  expectB("finished patterns stay quiet", beeperTick(&s, 5000, &freq), false);

  // An unknown pattern must be inert rather than reading off the end of the table.
  beeperReset(&s);
  beeperStart(&s, BEEP_PATTERN_COUNT + 7, 0);
  expectB("an unknown pattern does nothing", beeperTick(&s, 0, &freq), false);
  expectB("as does BEEP_NONE", beeperTick(&s, 0, &freq), false);

  // millis() wraps every 49 days. A pattern straddling it must finish, not hang on its step.
  beeperReset(&s);
  beeperStart(&s, BEEP_ACK, 0xFFFFFF00UL);
  beeperTick(&s, 0xFFFFFF00UL, &freq);
  expectB("a pattern straddling the rollover ends", beeperTick(&s, 0xFFFFFF00UL + 300, &freq), true);
  expectL("and ends silent", freq, 0);
}

static void testAuxPins() {
  group("auxiliary terminal conflicts");
  // The claim table, stated independently of the header so a wrong edit to it fails here rather
  // than only showing up as a device that quietly stops working.
  expectL("A1 axis takes the first three", auxClaimMask(AUX_A1_AXIS), 0x07);
  expectL("handwheel 1 takes the same three", auxClaimMask(AUX_HANDWHEEL_1), 0x07);
  expectL("handwheel 2 takes the second three", auxClaimMask(AUX_HANDWHEEL_2), 0x38);
  expectL("the joystick takes all six", auxClaimMask(AUX_JOYSTICK), 0x3F);

  int none = auxEnabledMask(false, false, false, false);
  expectL("nothing conflicts on a bare machine", auxConflict(AUX_JOYSTICK, none), -1);

  // The pairs that overlap.
  int a1Only = auxEnabledMask(true, false, false, false);
  expectL("handwheel 1 clashes with the A1 axis", auxConflict(AUX_HANDWHEEL_1, a1Only), AUX_A1_AXIS);
  expectL("the joystick clashes with the A1 axis", auxConflict(AUX_JOYSTICK, a1Only), AUX_A1_AXIS);
  int hw2Only = auxEnabledMask(false, false, true, false);
  expectL("the joystick clashes with handwheel 2", auxConflict(AUX_JOYSTICK, hw2Only), AUX_HANDWHEEL_2);
  int joyOnly = auxEnabledMask(false, false, false, true);
  expectL("the A1 axis clashes with the joystick", auxConflict(AUX_A1_AXIS, joyOnly), AUX_JOYSTICK);
  expectL("handwheel 2 clashes with the joystick", auxConflict(AUX_HANDWHEEL_2, joyOnly), AUX_JOYSTICK);

  // The pairs that do not. These have to stay allowed or the check would be worse than useless.
  expectL("handwheel 2 is fine beside the A1 axis", auxConflict(AUX_HANDWHEEL_2, a1Only), -1);
  int hw1Only = auxEnabledMask(false, true, false, false);
  expectL("handwheel 2 is fine beside handwheel 1", auxConflict(AUX_HANDWHEEL_2, hw1Only), -1);
  expectL("the A1 axis is fine beside handwheel 2", auxConflict(AUX_A1_AXIS, hw2Only), -1);

  // Re-enabling something already on must not report itself as its own conflict, or a settings
  // restore that writes every key back would refuse the ones that are already correct.
  expectL("a device does not conflict with itself", auxConflict(AUX_JOYSTICK, joyOnly), -1);
  expectL("nor does the A1 axis", auxConflict(AUX_A1_AXIS, a1Only), -1);

  // Resolving a stored setup where several devices claim the same terminals. Order matters: the
  // A1 axis is part of the machine, so it must not be the one silently dropped to keep an
  // accessory alive.
  expectL("nothing enabled resolves to nothing", auxResolveConflicts(none), 0);
  expectL("a lone device survives", auxResolveConflicts(joyOnly), joyOnly);
  expectL("the axis wins over the stick",
      auxResolveConflicts(auxEnabledMask(true, false, false, true)), a1Only);
  expectL("the axis wins over handwheel 1",
      auxResolveConflicts(auxEnabledMask(true, true, false, false)), a1Only);
  expectL("handwheel 1 wins over the stick",
      auxResolveConflicts(auxEnabledMask(false, true, false, true)), hw1Only);
  // Compatible devices must both survive - dropping one of a pair that never collided would be
  // worse than not checking at all.
  expectL("the axis and handwheel 2 coexist",
      auxResolveConflicts(auxEnabledMask(true, false, true, false)),
      auxEnabledMask(true, false, true, false));
  expectL("both handwheels coexist",
      auxResolveConflicts(auxEnabledMask(false, true, true, false)),
      auxEnabledMask(false, true, true, false));
  // Everything on at once: the axis and handwheel 2 fit together, the other two do not fit at all.
  expectL("all four resolves to the two that fit",
      auxResolveConflicts(auxEnabledMask(true, true, true, true)),
      auxEnabledMask(true, false, true, false));

  // Every device needs a name short enough to fit "Used by " plus the name on a 20-column line.
  for (int d = 0; d < AUX_DEVICE_COUNT; d++) {
    checks++;
    size_t len = strlen(auxDeviceName(d)) + 8;
    if (len <= 20) {
      printf("  ok   %s fits the error line\n", auxDeviceName(d));
    } else {
      failures++;
      printf("  FAIL %s makes a %d character message\n", auxDeviceName(d), (int)len);
    }
  }
}

static void testIndexing() {
  group("spindle indexing");
  const long CPR = 4000; // 1000 PPR, 4x decode

  // Six divisions of 4000 counts is 666.67 - deliberately not a whole number, because that is the
  // case where an implementation that adds a truncated step drifts.
  IndexPosition ip = indexNearest(0, CPR, 6);
  expectL("dead on the first mark", ip.index, 0);
  expectL("with no error", ip.errCounts, 0);

  ip = indexNearest(667, CPR, 6);
  expectL("on the second mark", ip.index, 1);
  expectL("within a count of it", ip.errCounts, 0);

  // The last mark must not have accumulated the truncation of the five before it.
  ip = indexNearest(3333, CPR, 6);
  expectL("on the sixth mark", ip.index, 5);
  expectB("last mark is still exact", calAbsL(ip.errCounts) <= 1, true);

  // Just short of coming back round: nearest is mark 0 a whole turn on, reported as 0 with a
  // negative error rather than as a seventh mark.
  ip = indexNearest(3990, CPR, 6);
  expectL("wraps to the first mark", ip.index, 0);
  expectL("and reads as short of it", ip.errCounts, -10);

  // Sign convention: positive means the spindle has gone past the mark.
  ip = indexNearest(680, CPR, 6);
  expectB("past the mark reads positive", ip.errCounts > 0, true);
  ip = indexNearest(650, CPR, 6);
  expectB("short of the mark reads negative", ip.errCounts < 0, true);

  // Every mark of a fine division must still be reachable and distinct.
  for (long d = 2; d <= 360; d *= 3) {
    for (long i = 0; i < d; i++) {
      long pos = (i * CPR + d / 2) / d;
      ip = indexNearest(pos, CPR, d);
      checks++;
      if (ip.index == i % d && calAbsL(ip.errCounts) <= 1) {
        continue;
      }
      failures++;
      printf("  FAIL %ld divisions, mark %ld: got index %ld err %ld\n", d, i, ip.index, ip.errCounts);
    }
  }
  printf("  ok   every mark of 2, 6, 18, 54, 162 divisions lands on itself\n");

  expectF("error in degrees", indexErrorDeg(10, CPR), 0.9, 0.001);
  expectF("negative error in degrees", indexErrorDeg(-10, CPR), -0.9, 0.001);

  expectL("half a degree is five counts", indexToleranceCounts(CPR, 5), 5);
  // A tolerance finer than the encoder resolves would never be reachable, so it floors at one
  // count rather than at zero.
  expectL("tolerance never reaches zero", indexToleranceCounts(CPR, 0), 1);
  expectB("inside tolerance is on the mark", indexOnMark(4, 5), true);
  expectB("outside is not", indexOnMark(6, 5), false);
  expectB("tolerance is inclusive", indexOnMark(-5, 5), true);

  // A position outside one revolution is normalised rather than trusted. Truncation towards zero
  // would otherwise hand back a negative mark number, which reads on the display as an encoder
  // fault rather than an arithmetic one.
  ip = indexNearest(-10, CPR, 6);
  expectL("a negative position wraps to a real mark", ip.index, 0);
  expectL("and keeps its sign against that mark", ip.errCounts, -10);
  ip = indexNearest(-667, CPR, 6);
  expectL("a negative position lands on the last mark", ip.index, 5);
  expectB("exactly on it", calAbsL(ip.errCounts) <= 1, true);
  ip = indexNearest(CPR + 667, CPR, 6);
  expectL("more than a turn wraps too", ip.index, 1);
  expectB("and is exact", calAbsL(ip.errCounts) <= 1, true);
  bool everyIndexInRange = true;
  for (long p = -3 * CPR; p < 3 * CPR; p += 37) {
    IndexPosition q = indexNearest(p, CPR, 6);
    if (q.index < 0 || q.index >= 6) everyIndexInRange = false;
  }
  expectB("no position produces a mark outside the list", everyIndexInRange, true);

  // Nonsense settings must be inert rather than dividing by zero.
  ip = indexNearest(100, CPR, 0);
  expectL("zero divisions is inert", ip.errCounts, 0);
  ip = indexNearest(100, 0, 6);
  expectL("no encoder is inert", ip.errCounts, 0);
}

static void testSurfaceSpeed() {
  group("constant surface speed");
  // 100 m/min on 50mm stock: 1000 * 100 / (pi * 50) = 636.6 rpm.
  expectL("target rpm at 50mm", cssTargetRpm(500000, 100), 637);
  // The same speed on a third of the diameter needs three times the rpm - the whole reason the
  // figure is worth showing.
  expectL("three times the rpm at a third the diameter", cssTargetRpm(166667, 100), 1910);
  expectL("actual speed is the inverse", cssActualMPerMin(500000, 637), 100);

  // Facing runs the diameter to zero, where the required rpm runs to infinity. Both directions
  // have to stay finite rather than dividing by zero at the centre of the work.
  expectL("no diameter, no target", cssTargetRpm(0, 100), 0);
  expectL("no diameter, no surface speed", cssActualMPerMin(0, 1000), 0);
  expectL("no target speed, no target rpm", cssTargetRpm(500000, 0), 0);

  // Capping: a target the lathe cannot reach is reported as the machine's ceiling, so the display
  // can say the speed is unreachable instead of showing an rpm nobody can dial in.
  expectL("under the ceiling passes through", cssCappedRpm(637, 2000), 637);
  expectL("over the ceiling is capped", cssCappedRpm(5000, 2000), 2000);
  expectL("no ceiling means no cap", cssCappedRpm(5000, 0), 5000);

  group("cutting speed by material");
  // Manual is index 0 and means "use the number I typed", which is what lets one setting cover
  // both sources instead of needing a separate mode.
  expectL("manual uses the typed figure", cssSpeedFor(0, true, 140), 140);
  expectL("manual ignores the tool", cssSpeedFor(0, false, 140), 140);
  expectL("a negative manual figure reads as off", cssSpeedFor(0, true, -5), 0);
  // Out of range must fall back to the manual figure rather than reading off the end of the table.
  expectL("an unknown material falls back", cssSpeedFor(999, true, 140), 140);
  expectL("a negative material falls back", cssSpeedFor(-1, true, 140), 140);

  // A material overrides the typed figure entirely, and carbide runs faster than HSS in every
  // entry - if that ever inverts, the table has been mistyped.
  expectB("a material overrides the manual figure", cssSpeedFor(6, true, 140) != 140, true);
  bool carbideAlwaysFaster = true;
  bool namesFit = true;
  for (int m = 1; m < materialCount(); m++) {
    const MaterialPreset* p = materialAt(m);
    if (p->carbide <= p->hss) carbideAlwaysFaster = false;
    if (strlen(p->name) > 12 || p->name[0] == 0) namesFit = false;
    checks++;
    if (p->hss > 0 && p->carbide > 0) continue;
    failures++;
    printf("  FAIL %s has a zero speed\n", p->name);
  }
  printf("  ok   every material carries a speed for both tools\n");
  expectB("carbide is faster than HSS throughout", carbideAlwaysFaster, true);
  expectB("names fit the LCD value line", namesFit, true);
  expectL("mild steel with carbide", cssSpeedFor(6, true, 0), 120);
  expectL("mild steel with HSS", cssSpeedFor(6, false, 0), 30);

  // The whole point: material plus diameter gives an rpm without the operator looking anything up.
  // Mild steel, carbide, 50mm stock -> 120 m/min -> 764 rpm.
  expectL("mild steel on 50mm stock", cssTargetRpm(500000, cssSpeedFor(6, true, 0)), 764);

  expectL("out of range names do not crash", (long) strlen(materialName(999)), (long) strlen(materialName(0)));

  group("metric and imperial surface speed");
  // Stored in m/min throughout, converted only where it meets the operator - the same rule
  // distances follow in deci-microns. A shop working in inches thinks in surface feet per minute.
  expectL("metric passes straight through", cssToDisplay(100, false), 100);
  expectL("100 m/min is 328 ft/min", cssToDisplay(100, true), 328);
  expectL("and back again", cssFromDisplay(328, true), 100);
  expectL("metric round trips exactly", cssFromDisplay(cssToDisplay(137, false), false), 137);
  expectL("zero stays zero", cssToDisplay(0, true), 0);
  expectB("the unit follows the mode", strcmp(cssUnitName(true), "ft/min") == 0, true);
  expectB("and in metric", strcmp(cssUnitName(false), "m/min") == 0, true);

  // Every material has to survive being shown in feet and typed back in without drifting, or a
  // shop working in inches would find its speeds creeping every time it opened the menu.
  bool imperialRoundTrips = true;
  for (int m = 1; m < materialCount(); m++) {
    for (int tool = 0; tool < 2; tool++) {
      long metric = cssSpeedFor(m, tool != 0, 0);
      if (cssFromDisplay(cssToDisplay(metric, true), true) != metric) imperialRoundTrips = false;
    }
  }
  expectB("every material survives a trip through feet", imperialRoundTrips, true);

  // Rounding before converting is the trap: whole m/min scaled to feet afterwards moves the
  // figure by up to two, which shows on a readout with no decimals.
  expectB("the actual speed is not pre-rounded",
      cssActualSpeed(500000, 100) != (double)cssActualMPerMin(500000, 100), true);
  expectF("and still agrees to within a unit",
      cssActualSpeed(500000, 100), cssActualMPerMin(500000, 100), 1.0);

  group("constant surface speed");
  expectL("on target reads zero", cssDeviationPercent(600, 600), 0);
  expectL("too fast reads positive", cssDeviationPercent(600, 900), 50);
  expectL("too slow reads negative", cssDeviationPercent(600, 300), -50);
  expectL("no target, no deviation", cssDeviationPercent(0, 900), 0);
  // Near centre the target collapses and the deviation would otherwise be a number too wide for
  // the line it shares with the rpm.
  expectL("clamped high", cssDeviationPercent(1, 100000), 999);
  // The low side needs no clamp: a stopped spindle is 100% below target and nothing is further
  // below than that.
  expectL("a stopped spindle is 100% under", cssDeviationPercent(600, 0), -100);
  expectL("and that is the floor", cssDeviationPercent(100000, 1), -99);
}

// Everything the sketch asks about a mode. These are read all over it and a wrong answer is
// quiet: a mode missing from needsZStops lets an operation start with no limits set, and a wrong
// start corner sends the tool to the far end of the work before the first cut.
static const int ALL_MODES[] = {
  MODE_NORMAL, MODE_XGEAR, MODE_ASYNC, MODE_CONE, MODE_TURN, MODE_FACE, MODE_CUT,
  MODE_THREAD, MODE_ELLIPSE, MODE_GCODE, MODE_A1, MODE_SLOT, MODE_TPR,
};
static const int ALL_MODES_COUNT = sizeof(ALL_MODES) / sizeof(ALL_MODES[0]);

static void testModePredicates() {
  group("what each mode is");
  expectB("thread is a thread mode", modeIsThread(MODE_THREAD), true);
  expectB("so is the tapered one", modeIsThread(MODE_TPR), true);
  expectB("turning is not", modeIsThread(MODE_TURN), false);
  expectB("the gearbox drives Z", modeIsGearbox(MODE_NORMAL), true);
  expectB("and XGEAR is the same mode on X", modeIsGearbox(MODE_XGEAR), true);
  expectB("async is not a gearbox - it is not spindle synced", modeIsGearbox(MODE_ASYNC), false);

  // Anything that runs passes needs the wizard, and anything with a wizard runs passes - except
  // cone and gcode, which have setup of their own without being pass modes.
  bool wizardMatchesPasses = true;
  for (int i = 0; i < ALL_MODES_COUNT; i++) {
    int m = ALL_MODES[i];
    bool wizard = modeLastSetupIndex(m) > 0;
    bool expected = modeIsPass(m) || m == MODE_CONE || m == MODE_GCODE;
    if (wizard != expected) wizardMatchesPasses = false;
  }
  expectB("every mode that runs passes has a setup wizard", wizardMatchesPasses, true);

  // Needing Z stops implies being a pass mode; parting is the one that runs passes on X alone.
  bool zStopsSubset = true;
  for (int i = 0; i < ALL_MODES_COUNT; i++) {
    int m = ALL_MODES[i];
    if (modeNeedsZStops(m) && !modeIsPass(m)) zStopsSubset = false;
  }
  expectB("needing Z stops implies running passes", zStopsSubset, true);
  expectB("parting runs passes without Z stops",
      modeIsPass(MODE_CUT) && !modeNeedsZStops(MODE_CUT), true);

  // Jogging while running is only safe where the axes are not being driven to a target.
  bool jogSafe = true;
  for (int i = 0; i < ALL_MODES_COUNT; i++) {
    int m = ALL_MODES[i];
    if (modeAllowsManualMovesWhenOn(m) && modeIsPass(m)) jogSafe = false;
  }
  expectB("no pass mode allows jogging while it runs", jogSafe, true);

  group("setup wizard length");
  // Every mode with a wizard ends on the Go? confirmation. Cone returned 2 here while its display
  // drew Go? at step 3, so the step never arrived and it began cutting straight off a question.
  expectL("turning asks three", modeLastSetupIndex(MODE_TURN), 3);
  expectL("facing asks three", modeLastSetupIndex(MODE_FACE), 3);
  expectL("parting asks three", modeLastSetupIndex(MODE_CUT), 3);
  expectL("threading asks three", modeLastSetupIndex(MODE_THREAD), 3);
  expectL("the tapered thread asks a fourth for the taper", modeLastSetupIndex(MODE_TPR), 4);
  expectL("cone asks three", modeLastSetupIndex(MODE_CONE), 3);
  expectL("gcode asks two", modeLastSetupIndex(MODE_GCODE), 2);
  expectL("the gearbox asks nothing", modeLastSetupIndex(MODE_NORMAL), 0);
  expectL("nor does A1", modeLastSetupIndex(MODE_A1), 0);

  group("mode names");
  // The name shares line 0 with the on/off word, the limit glyphs and the step, so it has to stay
  // short. Five is what the longest already is.
  bool namesFit = true;
  for (int i = 0; i < ALL_MODES_COUNT; i++) {
    if (strlen(modeNameOf(ALL_MODES[i])) > 5) namesFit = false;
  }
  expectB("mode names fit the top line", namesFit, true);
  expectB("the plain gearbox shows nothing", modeNameOf(MODE_NORMAL)[0] == 0, true);
  // ...which is why it needs the sibling marker, or the screen would say nothing at all about
  // which of the two gearbox modes is selected.
  expectB("but it does carry the sibling marker", modeHasSiblingOf(MODE_NORMAL), true);
  expectB("as does thread, which hides TPR", modeHasSiblingOf(MODE_THREAD), true);
  expectB("XGEAR does not - it is the sibling", modeHasSiblingOf(MODE_XGEAR), false);

  group("which corner an operation starts from");
  // leftStop is the larger coordinate on both axes - the convention throughout the firmware.
  const long L = 100, R = 0, POS = 55;
  expectL("turning a right-hand thread starts at the right", modeZStart(MODE_TURN, L, R, POS, 1, true), R);
  expectL("a left-hand one starts at the left", modeZStart(MODE_TURN, L, R, POS, -1, true), L);
  expectL("threading follows the same rule", modeZStart(MODE_THREAD, L, R, POS, 1, true), R);
  expectL("and so does the tapered thread", modeZStart(MODE_TPR, L, R, POS, 1, true), R);
  // Facing takes its direction from external/internal, not from the sign of the pitch - the pitch
  // there is depth per revolution and says nothing about which way along the face it travels.
  expectL("facing outward starts at the right", modeZStart(MODE_FACE, L, R, POS, 1, true), R);
  expectL("facing inward starts at the left", modeZStart(MODE_FACE, L, R, POS, 1, false), L);
  expectL("the ellipse starts opposite turning", modeZStart(MODE_ELLIPSE, L, R, POS, 1, true), L);
  expectL("slotting always strokes from the right", modeZStart(MODE_SLOT, L, R, POS, 1, true), R);
  expectL("slotting ignores the pitch sign", modeZStart(MODE_SLOT, L, R, POS, -1, false), R);
  // A mode with no passes has no start corner, so it stays where it is rather than moving.
  expectL("the gearbox does not move to a corner", modeZStart(MODE_NORMAL, L, R, POS, 1, true), POS);
  expectL("nor does async", modeZStart(MODE_ASYNC, L, R, POS, 1, true), POS);

  expectL("an external thread starts at the near X stop", modeXStart(MODE_TURN, L, R, POS, 1, true), R);
  expectL("an internal one starts at the far side", modeXStart(MODE_TURN, L, R, POS, 1, false), L);
  expectL("parting outside in", modeXStart(MODE_CUT, L, R, POS, 1, true), R);
  expectL("parting inside out", modeXStart(MODE_CUT, L, R, POS, -1, true), L);
  expectL("the gearbox stays put on X too", modeXStart(MODE_NORMAL, L, R, POS, 1, true), POS);

  group("modes and their settings");
  // A mode that runs passes must have a pass count to run, which means a settings group.
  bool passModesHaveSettings = true;
  for (int i = 0; i < ALL_MODES_COUNT; i++) {
    int m = ALL_MODES[i];
    if (modeIsPass(m) && settingModeCount(modeSettingsOf(m)) < 1) passModesHaveSettings = false;
  }
  expectB("every pass mode has settings of its own", passModesHaveSettings, true);
  expectL("the gearbox has none", modeSettingsOf(MODE_NORMAL), SMODE_NONE);
  expectL("nor gcode", modeSettingsOf(MODE_GCODE), SMODE_NONE);
  expectL("nor A1", modeSettingsOf(MODE_A1), SMODE_NONE);

  // Two modes mapping to one settings group would have them silently sharing values, which is
  // the whole thing this arrangement exists to prevent.
  bool mappingUnique = true;
  for (int i = 0; i < ALL_MODES_COUNT; i++) {
    for (int j = i + 1; j < ALL_MODES_COUNT; j++) {
      int a = modeSettingsOf(ALL_MODES[i]);
      if (a == SMODE_NONE) continue;
      if (a == modeSettingsOf(ALL_MODES[j])) mappingUnique = false;
    }
  }
  expectB("no two modes share one settings group", mappingUnique, true);

  expectB("the pitch drives X when facing", modePitchOnX(MODE_FACE), true);
  expectB("and when cross-feeding", modePitchOnX(MODE_XGEAR), true);
  expectB("but Z when turning", modePitchOnX(MODE_TURN), false);
}

// Compares a built line against what should be on the display. The millimetre glyph is character
// 1 in the buffer - it cannot be its real code 0, which would terminate the string - so tests
// write it as ~ and it is swapped in here.
static void expectLine(const char* what, const LcdLine* l, const char* want) {
  char expanded[64];
  int n = 0;
  for (int i = 0; want[i] != 0 && n < 63; i++) {
    expanded[n++] = want[i] == '~' ? LCD_GLYPH_MM : want[i];
  }
  expanded[n] = 0;
  checks++;
  if (strcmp(l->buf, expanded) == 0) {
    printf("  ok   %s\n", what);
  } else {
    failures++;
    printf("  FAIL %s: got \"%s\", want \"%s\"\n", what, l->buf, want);
  }
}

static void testLcdLine() {
  group("building a display line");
  LcdLine l;
  lcdLineClear(&l);
  lcdLineStr(&l, "Pitch ");
  lcdLineFixed(&l, 1.25, 2);
  expectLine("text and a number", &l, "Pitch 1.25");
  expectL("and its length", l.len, 10);

  // A line longer than the display used to wrap onto the next one and overwrite it. It cannot
  // now: the builder stops, and says it stopped.
  lcdLineClear(&l);
  lcdLineStr(&l, "012345678901234567890123456789");
  expectL("a long line stops at the display width", l.len, LCD_LINE_MAX);
  expectB("and reports that it did", l.truncated, true);
  expectLine("keeping the front of it", &l, "01234567890123456789");

  lcdLineClear(&l);
  lcdLineStr(&l, "01234567890123456789");
  expectB("exactly full is not truncated", l.truncated, false);
  expectL("no room left", lcdLineRoom(&l), 0);

  group("numbers on the display");
  lcdLineClear(&l); lcdLineLong(&l, 0);
  expectLine("zero", &l, "0");
  lcdLineClear(&l); lcdLineLong(&l, -42);
  expectLine("negative", &l, "-42");
  lcdLineClear(&l); lcdLineLong(&l, 123456789L);
  expectLine("large", &l, "123456789");

  // Arduino rounds half away from zero and the C library rounds half to even, so this is a
  // transcription of Print::printFloat rather than a call to snprintf. If it drifted, the tests
  // would pass while the display showed something else.
  // 0.125 is exactly representable, so this really is the halfway case: Arduino gives 0.13 and
  // the C library's round-half-to-even gives 0.12. Picking 1.005 would prove nothing - it is
  // stored as 1.00499... and both would print 1.00.
  lcdLineClear(&l); lcdLineFixed(&l, 0.125, 2);
  expectLine("half rounds away from zero, as Arduino does", &l, "0.13");
  lcdLineClear(&l); lcdLineFixed(&l, 1.999, 2);
  expectLine("and carries into the integer part", &l, "2.00");
  lcdLineClear(&l); lcdLineFixed(&l, -2.5, 0);
  expectLine("negative rounds away too", &l, "-3");
  lcdLineClear(&l); lcdLineFixed(&l, 0.0625, 5);
  expectLine("a taper at full precision", &l, "0.06250");

  group("distances in the selected units");
  lcdLineClear(&l); lcdLineDeciMicrons(&l, 0, 5, true);
  expectLine("zero needs no unit", &l, "0");
  lcdLineClear(&l); lcdLineDeciMicrons(&l, 20000, 5, true);
  expectLine("a whole millimetre drops its decimals", &l, "2~");
  lcdLineClear(&l); lcdLineDeciMicrons(&l, 6500, 5, true);
  expectLine("and keeps the ones it needs", &l, "0.65~");
  lcdLineClear(&l); lcdLineDeciMicrons(&l, -50000, 2, true);
  expectLine("negative", &l, "-5~");
  lcdLineClear(&l); lcdLineDeciMicrons(&l, 254000, 5, false);
  expectLine("an inch in inch mode", &l, "1\"");
}

static SetupLineInput turnSetup() {
  SetupLineInput in;
  memset(&in, 0, sizeof(in));
  in.mode = MODE_TURN;
  in.passes = 3;
  in.metric = true;
  in.auxForward = true;
  in.dupr = 2000;
  return in;
}

static void testSetupLine() {
  group("the setup wizard's line");
  LcdLine l;
  SetupLineInput in = turnSetup();

  // Before the wizard starts there was nothing here at all, so the wizard behind the play button
  // was invisible unless you already knew it existed.
  buildSetupLine(&l, &in);
  expectLine("the way in", &l, "ON to set up 3 steps");

  in.setupIndex = 1;
  buildSetupLine(&l, &in);
  expectLine("step one counts itself", &l, "1/3 3 passes?");
  in.passes = 1;
  buildSetupLine(&l, &in);
  expectLine("one pass is singular", &l, "1/3 1 pass?");
  in.numpadPasses = 12;
  buildSetupLine(&l, &in);
  expectLine("what is being typed wins", &l, "1/3 12 passes?");
  in.numpadPasses = 5000; // above the ceiling
  buildSetupLine(&l, &in);
  expectLine("clamped to the ceiling", &l, "1/3 999 passes?");
  in.numpadPasses = 0;
  in.passes = 3;

  // The question mark used to read as a yes/no that play would answer, when play accepts what is
  // shown and the arrows are what change it.
  in.setupIndex = 2;
  buildSetupLine(&l, &in);
  expectLine("step two says the arrows change it", &l, "2/3 External <>");
  in.auxForward = false;
  buildSetupLine(&l, &in);
  expectLine("and the other way", &l, "2/3 Internal <>");

  in.mode = MODE_FACE;
  in.auxForward = true;
  buildSetupLine(&l, &in);
  expectLine("facing asks about direction along the face", &l, "2/3 Right-left <>");

  // Parting has nothing for the arrows to change, so it must not claim they do.
  in.mode = MODE_CUT;
  buildSetupLine(&l, &in);
  expectLine("parting takes its direction from the pitch", &l, "2/3 Ext, pitch>0");
  in.dupr = -2000;
  buildSetupLine(&l, &in);
  expectLine("and says so the other way", &l, "2/3 Int, pitch<0");

  group("the Go confirmation");
  in = turnSetup();
  in.setupIndex = 3;
  buildSetupLine(&l, &in);
  expectLine("already at the corner, nothing to move", &l, "3/3 Go?");

  in.zOffsetDu = -50000;
  buildSetupLine(&l, &in);
  expectLine("says how far Z moves first", &l, "3/3 Go Z-5~?");
  in.xOffsetDu = -20000;
  buildSetupLine(&l, &in);
  expectLine("and X", &l, "3/3 Go Z-5~ X-2~?");
  expectB("which still fits", l.len <= LCD_LINE_MAX, true);

  // This is the case that used to reach 23 characters and wrap onto the position line below,
  // corrupting it. Both offsets cannot fit, so the second is dropped and the line stays whole.
  in.zOffsetDu = -1234500;
  in.xOffsetDu = -987600;
  buildSetupLine(&l, &in);
  expectB("two large offsets still fit the line", l.len <= LCD_LINE_MAX, true);
  expectB("without truncating anything", l.truncated, false);
  expectB("and the question mark survives", l.buf[l.len - 1] == '?', true);

  group("the tapered thread's extra step");
  in = turnSetup();
  in.mode = MODE_TPR;
  in.taper = 0.0625;
  in.setupIndex = 3;
  buildSetupLine(&l, &in);
  expectLine("asks for the taper", &l, "3/4 Taper 0.06250?");
  in.setupIndex = 4;
  buildSetupLine(&l, &in);
  expectLine("then confirms", &l, "4/4 Go?");
  // A straight thread never sees that step - its step 3 is the confirmation.
  in.mode = MODE_THREAD;
  in.setupIndex = 3;
  buildSetupLine(&l, &in);
  expectLine("a straight thread goes at step three", &l, "3/3 Go?");

  group("while it runs");
  in = turnSetup();
  in.isOn = true;
  in.opIndex = 2;
  in.opTotal = 6;
  in.springStart = 4;
  buildSetupLine(&l, &in);
  expectLine("which pass of how many", &l, "Pass 2 of 6");
  in.opIndex = 5;
  buildSetupLine(&l, &in);
  expectLine("and names the spring passes", &l, "Spring 5 of 6");

  group("stops not set");
  in = turnSetup();
  in.missingStops = true;
  buildSetupLine(&l, &in);
  expectLine("turning needs all four", &l, "Set all stops");
  in.mode = MODE_CUT;
  buildSetupLine(&l, &in);
  expectLine("parting only needs X", &l, "Set X stops");
  // While a number is being typed the line below echoes it, and that matters more.
  in.inNumpad = true;
  in.setupIndex = 0;
  buildSetupLine(&l, &in);
  expectL("the numpad gets the line", l.len, 0);

  // Nothing may ever exceed the display, whatever the inputs.
  group("no input overruns the display");
  bool everFits = true;
  const long offsets[] = {0, -50, 50000, -1234500, 99999900L, -99999900L};
  for (int mi = 0; mi < ALL_MODES_COUNT; mi++) {
    if (!modeIsPass(ALL_MODES[mi])) continue;
    for (long step = 0; step <= 4; step++) {
      for (int o = 0; o < 6; o++) {
        for (int metric = 0; metric < 2; metric++) {
          SetupLineInput t = turnSetup();
          t.mode = ALL_MODES[mi];
          t.setupIndex = step;
          t.zOffsetDu = offsets[o];
          t.xOffsetDu = offsets[5 - o];
          t.metric = metric != 0;
          t.passes = 999;
          t.opTotal = 9999;
          t.opIndex = 9999;
          LcdLine probe;
          buildSetupLine(&probe, &t);
          if (probe.len > LCD_LINE_MAX) everFits = false;
        }
      }
    }
  }
  expectB("every mode, step, offset and unit fits", everFits, true);
}

// Per-mode settings: the values that used to be one global each, and the validation that guards
// them. The ranges matter more than they look - a clearance of zero puts the tool back into the
// work on the return stroke, and a pass count of zero is divided by.
static void testModeSettings() {
  group("per-mode settings");
  ModeSettings s;
  memset(&s, 0, sizeof(s));

  expectB("a fresh write succeeds", modeSettingWrite(&s, "tps", 6) == NULL, true);
  expectL("and reads back", modeSettingRead(&s, "tps"), 6);
  expectB("spring passes", modeSettingWrite(&s, "spp", 2) == NULL, true);
  expectL("read back", modeSettingRead(&s, "spp"), 2);
  expectB("clearance", modeSettingWrite(&s, "safe", 5000) == NULL, true);
  expectL("read back", modeSettingRead(&s, "safe"), 5000);
  expectB("peck depth", modeSettingWrite(&s, "pck", 2000) == NULL, true);
  expectL("read back", modeSettingRead(&s, "pck"), 2000);
  expectB("slot reduction", modeSettingWrite(&s, "slt", 500) == NULL, true);
  expectL("read back", modeSettingRead(&s, "slt"), 500);
  expectB("flank infeed", modeSettingWrite(&s, "fli", 1) == NULL, true);
  expectL("read back as a flag", modeSettingRead(&s, "fli"), 1);
  expectB("and off again", modeSettingWrite(&s, "fli", 0) == NULL, true);
  expectL("reads back off", modeSettingRead(&s, "fli"), 0);

  // Rejections. Each has to leave the stored value alone, or a refused edit would still land.
  expectB("no passes is refused", modeSettingWrite(&s, "tps", 0) != NULL, true);
  expectL("and the count is untouched", modeSettingRead(&s, "tps"), 6);
  expectB("negative passes refused", modeSettingWrite(&s, "tps", -1) != NULL, true);
  expectB("the ceiling is accepted", modeSettingWrite(&s, "tps", MODE_PASSES_MAX) == NULL, true);
  expectB("one past it is not", modeSettingWrite(&s, "tps", MODE_PASSES_MAX + 1) != NULL, true);
  expectL("still the ceiling", modeSettingRead(&s, "tps"), MODE_PASSES_MAX);

  expectB("zero spring passes is fine - it means none", modeSettingWrite(&s, "spp", 0) == NULL, true);
  expectB("99 spring passes accepted", modeSettingWrite(&s, "spp", MODE_SPRING_PASSES_MAX) == NULL, true);
  expectB("100 refused", modeSettingWrite(&s, "spp", MODE_SPRING_PASSES_MAX + 1) != NULL, true);

  // Zero clearance is the dangerous one: the tool would return through the cut it just made.
  expectB("zero clearance refused", modeSettingWrite(&s, "safe", 0) != NULL, true);
  expectB("negative clearance refused", modeSettingWrite(&s, "safe", -1) != NULL, true);
  expectL("clearance survives both", modeSettingRead(&s, "safe"), 5000);

  // Zero peck and zero slot reduction both legitimately mean "off".
  expectB("zero peck is off, not invalid", modeSettingWrite(&s, "pck", 0) == NULL, true);
  expectB("negative peck refused", modeSettingWrite(&s, "pck", -1) != NULL, true);
  expectB("zero slot reduction is off", modeSettingWrite(&s, "slt", 0) == NULL, true);
  expectB("negative slot reduction refused", modeSettingWrite(&s, "slt", -1) != NULL, true);

  expectB("an unknown key is refused", modeSettingWrite(&s, "nope", 1) != NULL, true);
  expectL("and reads as zero", modeSettingRead(&s, "nope"), 0);

  // The whole point: two modes hold different values through the same functions.
  group("modes keep their own values");
  ModeSettings thread, face;
  memset(&thread, 0, sizeof(thread));
  memset(&face, 0, sizeof(face));
  modeSettingWrite(&thread, "spp", 2);
  modeSettingWrite(&face, "spp", 0);
  modeSettingWrite(&thread, "tps", 6);
  modeSettingWrite(&face, "tps", 3);
  expectL("threading keeps its spring passes", modeSettingRead(&thread, "spp"), 2);
  expectL("facing keeps none", modeSettingRead(&face, "spp"), 0);
  expectL("threading keeps its pass count", modeSettingRead(&thread, "tps"), 6);
  expectL("facing keeps its own", modeSettingRead(&face, "tps"), 3);

  // Every mode-scoped row in the table must be a key these functions actually handle, or the
  // menu would show an item that always reads zero and refuses every write.
  bool everyRowHandled = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (!settingIsModeScoped(i) || settingIsAction(i)) continue;
    ModeSettings probe;
    memset(&probe, 0, sizeof(probe));
    // 1 is inside the accepted range of every mode-scoped item.
    if (modeSettingWrite(&probe, SETTINGS[i].prefKey, 1) != NULL) everyRowHandled = false;
  }
  expectB("every mode row's key is one the storage handles", everyRowHandled, true);
}

int main() {
  printf("calibration_math.h\n==================\n");
  testGeometry();
  testPitchCalibration();
  testTravelAndSpeed();
  testEncoderPpr();
  testSpindleModulo();
  testRpmAccumulator();
  testBigDroLayout();
  testDerivedFigures();
  testSettingsTable();
  testSlotting();
  testPassDepth();
  testPassSequencing();
  testFlankInfeed();
  testPeck();
  testEncoderHealth();
  testBeeper();
  testAuxPins();
  testIndexing();
  testSurfaceSpeed();
  testModeSettings();
  testModePredicates();
  testLcdLine();
  testSetupLine();
  printf("\n%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
