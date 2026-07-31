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
  expectL("item count", SETTINGS_COUNT, 64);
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
  expectL("sections cover every item exactly once", covered, SETTINGS_COUNT);

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
    if ((settingIsToggle(i) || settingIsAction(i) || settingUsesDu(i)) && u != 0) unitsPlaced = false;
    if (u != 0 && (strlen(u) > 8 || u[0] == 0 || u[0] == ' ')) unitsFit = false;
  }
  expectB("only measurable items carry a unit", unitsPlaced, true);
  expectB("units are short enough for the LCD", unitsFit, true);

  // Every plain number should say what it is counting. The WiFi PIN is the one real exception:
  // it is a code, not a quantity.
  int unitless = 0;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (settingIsToggle(i) || settingIsAction(i) || settingUsesDu(i)) continue;
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

  int actionCount = 0;
  for (int i = 0; i < SETTINGS_COUNT; i++) if (settingIsAction(i)) actionCount++;
  expectL("there is exactly one action item", actionCount, 1);

  bool kindsExclusive = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    if (settingUsesDu(i) && (settingIsToggle(i) || settingIsAction(i))) kindsExclusive = false;
  }
  expectB("kinds are mutually exclusive", kindsExclusive, true);

  bool keysPresent = true;
  for (int i = 0; i < SETTINGS_COUNT; i++) {
    bool hasKey = SETTINGS[i].prefKey != 0 && SETTINGS[i].prefKey[0] != 0;
    if (hasKey == settingIsAction(i)) keysPresent = false;
  }
  expectB("storable items have a key, the action has none", keysPresent, true);

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
  char qualified[80][20];
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
  expectL("preferences holds the operation settings", settingSectionCount(SEC_PREFS), 9);
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
  printf("\n%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
