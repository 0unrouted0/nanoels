// Pass sequencing arithmetic for the automated operations: how deep each pass cuts, how many
// there are, where multi-start threads are phased, and where a parting cut breaks the chip.
//
// This is the code that decides the dimensions of the finished part, so it is worth having under
// test rather than only observable by scrapping stock. No Arduino dependencies, so test/ can
// exercise exactly what modeTurn() and modeCut() call.

#ifndef PASS_MATH_H
#define PASS_MATH_H

#include <math.h>
#include "calibration_math.h"

// Arduino's PI, restated so the host build computes the identical flank shift.
#define PASS_PI 3.1415926535897932384626433832795
// Standard 29.5 degree thread flank, half of a 59 degree included angle.
#define PASS_FLANK_DEGREES 29.5

// Total operation steps, spring passes included. Multi-start threads repeat every pass once per
// start, so the count scales with starts.
inline long passTotalSteps(long turnPasses, long springPasses, long starts) {
  return (turnPasses + springPasses) * starts;
}

// 1-based pass number for an opIndex, clamped at turnPasses. The clamp is what makes spring
// passes work: they run past the last real pass but keep reporting it, so they re-cut at the
// same final depth instead of going deeper.
inline long passNumberForIndex(long opIndex, long starts, long turnPasses) {
  if (starts < 1) {
    starts = 1;
  }
  long n = (long)ceil(opIndex / (float)starts);
  return n > turnPasses ? turnPasses : n;
}

inline bool passIsSpring(long opIndex, long starts, long turnPasses) {
  if (starts < 1) {
    starts = 1;
  }
  return (long)ceil(opIndex / (float)starts) > turnPasses;
}

// Depth target for a slotting pass, stepping X from the start stop to the end stop. Pass 1 is
// already one increment in - the tool has to be cutting on the first stroke - and the last pass
// lands exactly on the end stop so the slot finishes at the depth you set.
inline long slotDepthPos(long startPos, long endPos, long passes, long passIndex) {
  if (passes < 1) {
    return startPos;
  }
  return endPos - calRoundL((double)(endPos - startPos) * (passes - passIndex) / passes);
}

// How far short of the far stop a slotting stroke ends. Each successive stroke stops earlier,
// leaving room for chips at the closed end of a blind slot; 0 gives full-length strokes.
//
// farPos is above nearPos: the stroke runs from the right stop to the left stop, and left is the
// larger coordinate here. Clamped at nearPos, because enough reduction would otherwise carry the
// end below the start and the stroke would run backwards.
inline long slotStrokeEnd(long nearPos, long farPos, long reductionSteps, long passIndex) {
  long endPos = farPos - reductionSteps * (passIndex - 1);
  return endPos < nearPos ? nearPos : endPos;
}

// Depth-axis target for a given pass, interpolating from startStop to endStop. Used by both the
// turning/threading passes and the cut-off passes, which compute it identically.
//
// Integer division deliberately truncates the per-pass increment, which is why the expression is
// arranged to subtract remaining passes from endStop rather than add completed ones to
// startStop: the final pass then lands exactly on endStop no matter how the division rounded.
inline long passDepthPos(long startStop, long endStop, long totalPasses, long passNumber) {
  if (totalPasses < 1) {
    return endStop;
  }
  return endStop - (endStop - startStop) / totalPasses * (totalPasses - passNumber);
}

// Encoder steps between the starts of a multi-start thread. Single start needs no offset.
inline long passStartOffset(long starts, float encoderSteps) {
  return starts == 1 ? 0 : calRoundL(encoderSteps / starts);
}

// Phase shift along the thread that makes the tool cut mostly on its leading flank instead of
// plunging on both. Proportional to the depth still to go, so it falls to zero on the final pass
// and on every spring pass - which is what leaves the last cut clean on both flanks.
inline long passFlankShift(long totalDepthDu, long turnPasses, long passNumber, float encoderSteps, long dupr) {
  if (dupr == 0 || turnPasses < 1) {
    return 0;
  }
  float depthToGo = totalDepthDu * (turnPasses - passNumber) / (float)turnPasses;
  return calRoundL(tan(PASS_FLANK_DEGREES / 180.0 * PASS_PI) * depthToGo * encoderSteps / dupr);
}

// Depth at which the next chip-breaking retract is due. A peck depth of 0 disables pecking, in
// which case the trigger is placed unreachably far so the cut runs straight through.
inline long peckNextDepth(long currentPos, long peckSteps, bool forward) {
  return currentPos + (forward ? 1 : -1) * peckSteps;
}

// Where to retract to when breaking the chip, never backing out past the start of the cut.
inline long peckRetractPos(long returnPos, long safeSteps, long startStop, bool forward) {
  long target = returnPos - (forward ? 1 : -1) * safeSteps;
  if (forward ? target < startStop : target > startStop) {
    return startStop;
  }
  return target;
}

#endif
