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

// A typed number converted to deci-microns. Metric entry is in millimetres and imperial in
// inches - the units a drawing states - with an optional decimal point.
//
// Entry used to be in whole microns, so four millimetres cost four keystrokes: 4000. The smallest
// useful figure set the length of every figure. Typing 4 for 4mm and 4.25 for 4.25mm is both
// shorter and what you would write down.
//
// The number arrives with the point already taken out: "4.25" is digits 425 and fracDigits 2.
// Kept in integers because the scale is exact in both systems, where a float would turn 0.1mm
// into something that is not quite 1000du and leave it there.
//
// Clamped rather than allowed to wrap. Eight digits of inches overflows a signed long, and a
// wrapped value would come back as a plausible move in the wrong direction rather than an error.
#define CAL_NUMPAD_DU_MAX 100000000L // 10 metres, past any lathe and any axis limit

// The sign arrives separately because the panel has no minus key - plus and minus flip it while an
// entry is under way. It is applied last, after the magnitude has been clamped, so a negative is
// held to the same limit as a positive rather than escaping it.
inline long calNumpadToDu(long digits, int fracDigits, bool inchMode, bool negative = false) {
  if (digits <= 0 || fracDigits < 0) {
    return 0;
  }
  long long num = (long long)digits * (inchMode ? 254000LL : 10000LL);
  long long den = 1;
  for (int i = 0; i < fracDigits && den <= 100000000LL; i++) {
    den *= 10;
  }
  long long v = (num + den / 2) / den; // round half up, so 0.0001in is 25du and not 25.4 truncated
  long du = v > CAL_NUMPAD_DU_MAX ? CAL_NUMPAD_DU_MAX : (long)v;
  return negative ? -du : du;
}

// The same typed number as a plain value, with its point applied and no unit attached: digits 425
// and two fractional digits is 4.25.
//
// For the two figures on the machine that take a decimal but are not distances - a taper ratio and
// a thread count - so there is nothing to convert into millimetres or inches. Both are used as
// floats downstream anyway, which is why this one does not stay in integers.
inline double calNumpadValue(long digits, int fracDigits, bool negative = false) {
  double v = (double)digits;
  for (int i = 0; i < fracDigits && i < 9; i++) {
    v /= 10.0;
  }
  return negative ? -v : v;
}

// ---------------------------------------------------------------------------
// Figures derived from the settings
// ---------------------------------------------------------------------------
//
// None of these are stored anywhere - they fall out of values the user already set, and exist
// because the consequences of a setting are not obvious from the setting itself. A 3mm pitch and
// a Z axis that tops out at 1200mm/min mean a hard ceiling of 400 spindle rpm, and nothing on the
// machine says so until a thread is ruined.

// Distance one motor step moves the axis, in deci-microns. The floor under everything else: a
// backlash or pitch correction finer than this cannot be expressed at all.
inline float calStepResolutionDu(float screwPitch, float motorSteps) {
  if (motorSteps < 1) {
    return 0;
  }
  return screwPitch / motorSteps;
}

// Motor steps per millimetre, the way most other motion controllers state the same fact.
inline float calStepsPerMm(float screwPitch, float motorSteps) {
  if (screwPitch <= 0) {
    return 0;
  }
  return motorSteps * 10000.0f / screwPitch;
}

// Fastest the axis can travel, in deci-microns per minute.
inline long calMaxFeedDuPerMin(long speedStepsPerSec, float screwPitch, float motorSteps) {
  if (motorSteps < 1) {
    return 0;
  }
  return calRoundL((double)speedStepsPerSec * 60.0 * screwPitch / motorSteps);
}

// Highest spindle speed the axis can still keep up with at this pitch, in rpm. Above it the axis is
// asked to move faster than its maximum step rate and the thread loses sync - quietly, because
// nothing stops or complains; the axis simply stops keeping up and the thread walks out of pitch.
//
// duPerRev is the pitch in deci-microns per spindle revolution; its sign is irrelevant, so
// left-hand threads give the same answer. starts multiplies it: a multi-start thread advances one
// pitch per start per spindle revolution, so a two-start thread halves the ceiling.
//
// motorSteps is steps per revolution of the SCREW, so any motor-to-screw belt or gear reduction is
// already inside it - see calStepsPerScrewRev(). Passing motorStepsPerTurn here instead would drop
// the reduction and overstate the ceiling by exactly that ratio.
//
// speedStepsPerSec is the axis's maximum step rate. That is the machine's number rather than a
// theoretical one: it comes from the "Max speed" setting, which the max-speed calibration routine
// measures by ramping the axis until it stalls and backing off. Whatever is stored there is what
// this figure means, so a conservative stored value gives a conservative ceiling.
inline long calMaxRpmForPitch(long speedStepsPerSec, float screwPitch, float motorSteps,
                              long duPerRev, long starts) {
  long pitch = calAbsL(duPerRev);
  if (starts < 1) {
    starts = 1;
  }
  if (pitch == 0 || motorSteps < 1 || speedStepsPerSec < 1) {
    return 0;
  }
  return calRoundL((double)speedStepsPerSec * 60.0 * screwPitch /
      ((double)motorSteps * pitch * starts));
}

// Highest rpm the ENCODER ITSELF may turn at before the glitch filter starts discarding real
// pulses as noise. The filter accepts a transition only once the input has been stable for its
// number of 12.5ns cycles, so the shortest pulse it will pass sets the ceiling. Same formula as
// the comment on ENCODER_FILTER in machine_config.h.
inline long calMaxEncoderRpm(int ppr, int filter) {
  if (ppr < 1 || filter < 1) {
    return 0;
  }
  return calRoundL(2.4e9 / ((double)ppr * filter));
}

// The same ceiling expressed at the spindle, which is what the operator actually controls. A
// geared-up encoder spins faster than the spindle and so hits its limit sooner.
inline long calMaxSpindleRpm(int ppr, int filter, int spindleTeeth, int pulleyTeeth) {
  long encoderRpm = calMaxEncoderRpm(ppr, filter);
  if (spindleTeeth < 1 || pulleyTeeth < 1) {
    return encoderRpm;
  }
  return calRoundL((double)encoderRpm * pulleyTeeth / spindleTeeth);
}

// Spindle rotation represented by one count, in degrees. How precisely the controller can know
// where the spindle is, which is the limit on thread start repeatability.
inline float calAngularResolutionDeg(long countsPerSpindleRev) {
  if (countsPerSpindleRev < 1) {
    return 0;
  }
  return 360.0f / countsPerSpindleRev;
}

// How far the axis still travels while slowing from full speed to a stop, in deci-microns. Worth
// knowing before setting a soft limit close to the chuck.
inline long calStopDistanceDu(long speedManualMove, long speedStart, long acceleration,
                              float screwPitch, float motorSteps) {
  if (motorSteps < 1) {
    return 0;
  }
  long steps = calDecelerateSteps(speedManualMove, speedStart, acceleration);
  return calRoundL((double)steps * screwPitch / motorSteps);
}

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------

// The access point password is stored as a number, because that is all the settings table holds.
// WPA2 needs at least 8 characters, and softAP() quietly opens the network rather than failing if
// it gets fewer - so the PIN has to be exactly 8 digits, which means no leading zero to lose.
inline bool calWifiPinValid(long pin) {
  return pin >= 10000000L && pin <= 99999999L;
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
