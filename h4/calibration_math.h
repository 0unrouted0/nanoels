// Pure arithmetic shared by the firmware and the host test suite in test/.
//
// Nothing here may touch Arduino, FreeRTOS or any global state: the whole point is that the
// same code compiles both for the ESP32 and for a desktop compiler, so test/test_calibration_math.cpp
// exercises the functions the firmware actually calls rather than a reimplementation of them.
//
// Arduino.h defines round() and abs() as macros, which would resolve differently on the host
// (std::round returns double, std::abs(long) may narrow). Every function below therefore uses
// the calRoundL / calAbsL helpers instead, so both builds compute bit-identical results.

#ifndef CALIBRATION_MATH_H
#define CALIBRATION_MATH_H

// Matches Arduino's round() macro exactly: half away from zero, truncated to long.
inline long calRoundL(double v) {
  return v >= 0 ? (long)(v + 0.5) : (long)(v - 0.5);
}

inline long calAbsL(long v) {
  return v < 0 ? -v : v;
}

// ---------------------------------------------------------------------------
// Axis geometry. Deliberately computed in float, not double, to match the types
// the Axis struct stores and keep motion behaviour bit-identical to before.
// ---------------------------------------------------------------------------

inline long calStepsToDu(float screwPitch, float motorSteps, long steps) {
  return calRoundL(steps * screwPitch / motorSteps);
}

inline long calDuToSteps(float screwPitch, float motorSteps, long du) {
  return calRoundL(du / screwPitch * motorSteps);
}

// Steps the motor must turn for one revolution of the lead screw. A belt or gear drive between
// them means the motor turns screwTeeth/motorTeeth times per screw revolution, so the effective
// steps per screw revolution scale by the same ratio. Both 1 is a motor coupled straight to the
// screw. Everything else works in steps per SCREW revolution, so this is the only place the
// ratio appears.
inline float calStepsPerScrewRev(float motorStepsPerTurn, long motorTeeth, long screwTeeth) {
  if (motorTeeth < 1 || screwTeeth < 1) {
    return motorStepsPerTurn;
  }
  return motorStepsPerTurn * screwTeeth / motorTeeth;
}

inline long calBacklashSteps(long backlashDu, float motorSteps, float screwPitch) {
  return (long)(backlashDu * motorSteps / screwPitch);
}

inline long calEstopSteps(long maxTravelMm, float screwPitch, float motorSteps) {
  return (long)(maxTravelMm * 10000 / screwPitch * motorSteps);
}

// Number of steps before the target at which deceleration has to start.
inline long calDecelerateSteps(long speedManualMove, long speedStart, long acceleration) {
  long steps = 0;
  long s = speedManualMove;
  while (s > speedStart) {
    steps++;
    // Subtract in float and truncate the result, which is what the compound assignment this
    // replaced did. Truncating the divisor first instead would round differently and change
    // the ramp length, so the cast has to sit outside the subtraction.
    s = (long)(s - acceleration / (float)s);
  }
  return steps;
}

// ---------------------------------------------------------------------------
// Screw pitch calibration
// ---------------------------------------------------------------------------

// steps = du * motorSteps / screwPitch, so for a fixed step count the true pitch scales with
// actual/commanded travel. Computed in double because screwPitch * actualDu reaches ~1e10,
// past float's ~7 significant digits.
inline long calCorrectedPitch(float screwPitch, long actualDu, long nominalDu) {
  if (nominalDu == 0) {
    return 0;
  }
  return calRoundL((double)screwPitch * actualDu / nominalDu);
}

// A correction beyond +-20% is far more likely a mistyped measurement than a real error,
// and silently accepting one would wreck every subsequent cut. Exactly +-20% is allowed.
inline bool calPitchPlausible(float oldPitch, long newPitch) {
  return !(newPitch < oldPitch * 0.8 || newPitch > oldPitch * 1.2);
}

// ---------------------------------------------------------------------------
// Travel limits and speed ramp
// ---------------------------------------------------------------------------

// Truncates rather than rounds: a max travel that overstates the real span would let a move
// run into the hard stop, so erring short is the safe direction.
inline long calTravelMm(long spanDu) {
  return spanDu / 10000;
}

// Next trial speed in the max-speed ramp, +10%. The floor of 1 matters because speeds below
// 10 would divide to zero and leave the ramp stuck on the same value forever.
inline long calSpeedNext(long speed) {
  long step = speed / 10;
  return speed + (step > 0 ? step : 1);
}

// 80% of the last speed that completed a stroke, never below the axis start speed.
inline long calSpeedBackoff(long lastGood, long speedStart) {
  long safe = lastGood * 4 / 5;
  return safe < speedStart ? speedStart : safe;
}

// ---------------------------------------------------------------------------
// Spindle encoder
// ---------------------------------------------------------------------------

// Standard encoder resolutions a measurement snaps to when it lands within 2%.
const int CAL_COMMON_PPR[] = {100, 200, 240, 360, 400, 500, 600, 1000, 1024, 1200, 2000, 2500};
const int CAL_COMMON_PPR_COUNT = sizeof(CAL_COMMON_PPR) / sizeof(CAL_COMMON_PPR[0]);

// Counts the hardware pulse counter produces per encoder pulse. Full 4x quadrature: both edges
// of both channels. Everything that converts between counts and revolutions derives from this,
// so the decode mode is described in exactly one place.
#define ENCODER_COUNTS_PER_PULSE 4

// Counts per revolution of the SPINDLE, which is what the rest of the firmware means by a
// revolution. A belt-driven encoder turns spindleTeeth/pulleyTeeth times per spindle turn, and
// the divider folds that many raw counts into one step to steady a fluttering encoder.
inline long calCountsPerSpindleRev(int ppr, int spindleTeeth, int pulleyTeeth, int divider) {
  long steps = (long) ppr * ENCODER_COUNTS_PER_PULSE;
  if (pulleyTeeth > 0) {
    steps = steps * spindleTeeth / pulleyTeeth;
  }
  if (divider > 1) {
    steps /= divider;
  }
  return steps < 1 ? 1 : steps;
}

inline long calPprFromCounts(long counts, long revs) {
  if (revs <= 0) {
    return 0;
  }
  return calRoundL(calAbsL(counts) / (double(ENCODER_COUNTS_PER_PULSE) * revs));
}

inline bool calPprValid(long ppr) {
  return ppr >= 24 && ppr <= 10000;
}

// First match wins, so the table must stay in ascending order.
inline long calSnapPpr(long ppr) {
  for (int i = 0; i < CAL_COMMON_PPR_COUNT; i++) {
    if (calAbsL(ppr - CAL_COMMON_PPR[i]) * 50 <= CAL_COMMON_PPR[i]) {
      return CAL_COMMON_PPR[i];
    }
  }
  return ppr;
}

inline long calSpindleModulo(long value, int encoderSteps) {
  value = value % encoderSteps;
  if (value < 0) {
    value += encoderSteps;
  }
  return value;
}

// Accumulates encoder movement until a full RPM_BULK worth of counts has gone past, returning
// true on the step that completes one. Two properties matter and are covered by tests:
//   - the delta is accumulated signed, so back-and-forth jitter cancels instead of being
//     counted twice and reading as a spindle spinning faster than it is;
//   - the overshoot is carried over rather than zeroed, so no counts are lost at speed.
inline bool calRpmAccumulate(int* index, int delta, long rpmBulk) {
  *index += delta;
  if (calAbsL(*index) < rpmBulk) {
    return false;
  }
  *index -= *index > 0 ? rpmBulk : -rpmBulk;
  return true;
}

// Numpad entry is in whole microns in metric and whole thou in imperial, both converted to
// deci-microns. One definition, because the settings menu, the calibration routines and their
// preview lines all have to agree on what a typed number means.
inline long calNumpadRawToDu(bool inchMode, long raw) {
  return inchMode ? raw * 254 : raw * 10;
}

// ---------------------------------------------------------------------------
// Large position display layout
// ---------------------------------------------------------------------------

// Decimal places for the big digits. Held to 4 digits total so the info strip beside them never
// has to move: 0.01mm below 100mm, 0.1mm above, and 0.001" / 0.01" in imperial.
inline int calBigDroPoints(bool metric, float magnitude) {
  if (metric) {
    return magnitude < 100 ? 2 : 1;
  }
  return magnitude < 10 ? 3 : 2;
}

// Columns a formatted value occupies: 3 per digit, 1 for the decimal point.
inline int calBigDroWidth(const char* formatted) {
  int width = 0;
  for (int i = 0; formatted[i] != 0; i++) {
    width += formatted[i] == '.' ? 1 : 3;
  }
  return width;
}

// Right-aligns the value so the ones column stays put, clamped so it never starts left of the
// sign column. numCols is the first column belonging to the info strip.
inline int calBigDroStartCol(int numCols, int width) {
  int col = numCols - width;
  return col < 1 ? 1 : col;
}

inline int calRpmFromBulkMicros(unsigned long microsPerBulk) {
  if (microsPerBulk == 0) {
    return 0;
  }
  return (int)(60000000 / microsPerBulk);
}

#endif
