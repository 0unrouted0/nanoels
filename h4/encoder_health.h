// Pure arithmetic for judging the spindle encoder's signal, and for the dead-band that decides
// how faithfully the axes follow it. Shared by the firmware and the host test suite in test/.
//
// Nothing here may touch Arduino, FreeRTOS or any global state - see calibration_math.h for why.

#ifndef ENCODER_HEALTH_H
#define ENCODER_HEALTH_H

#include "calibration_math.h"

// How closely the follower tracks the raw count.
//
// ENC_DEADBAND_ONE_WAY keeps the follower in [pos, pos + band]: it snaps forward the instant the
// count rises and lags only on the way back. That models mechanical backlash in a lead screw,
// which is what the setting is named after, and it is the behaviour the firmware has always had.
// It is not a noise filter and does not act as one - a spurious count in the rising direction
// passes straight through, and the offset it leaves is permanent, because the follower simply
// waits at the higher value for real movement to catch up. Blips in one direction therefore
// accumulate across a pass.
//
// ENC_DEADBAND_SYMMETRIC keeps the follower in [pos - band, pos + band] instead, moving only once
// the count pushes past the band on either side. Both directions are then filtered equally. The
// price is that the rising direction now lags by up to band counts where it previously did not,
// so on a machine whose encoder is clean this is a cost with no benefit.
inline long encFollowDeadband(long pos, long avg, long band, bool symmetric) {
  if (symmetric) {
    if (pos > avg + band) return pos - band;
    if (pos < avg - band) return pos + band;
    return avg;
  }
  if (pos > avg) return pos;
  if (pos < avg - band) return pos + band;
  return avg;
}

// A sliding window of encoder activity, so the signal can be measured rather than guessed at.
//
// Two sums are kept over the same window and they answer different questions:
//
//   net  = sum of delta      - the movement that actually happened
//   path = sum of |delta|    - the distance the counter travelled to get there
//
// A cleanly rotating spindle has path == |net|. A stopped one has both at zero. An encoder
// dithering on a quadrature transition has path high and net near zero: loud without going
// anywhere. Separating those three states is the whole reason both sums exist, and it matters
// because the RPM figure the rest of the firmware uses is accumulated signed (see
// calRpmAccumulate) and so reads a dithering encoder as a stopped spindle.
//
// Coherence is |net| / path as a percentage. It reads 100 on a healthy signal at any speed and
// falls as noise adds travel that never becomes movement. Both sums cover the same window, so
// the elapsed time cancels and coherence needs no timing at all.
struct EncHealth {
  long pathCounts;       // running sums for the window in progress
  long netCounts;
  unsigned long windowStartUs;
  long pathRate;         // counts/sec over the last completed window
  long netRate;          // counts/sec, signed
  int coherence;         // 0-100 over the last completed window, -1 before the first one
  int worstCoherence;    // lowest coherence seen while the spindle was actually turning, -1 if none
  long dirtyWindows;     // completed windows that came in under the floor
};

inline void encHealthReset(EncHealth* h, unsigned long nowUs) {
  h->pathCounts = 0;
  h->netCounts = 0;
  h->windowStartUs = nowUs;
  h->pathRate = 0;
  h->netRate = 0;
  h->coherence = -1;
  h->worstCoherence = -1;
  h->dirtyWindows = 0;
}

// Drops the window in progress without touching the figures already recorded. For the caller to
// use when the spindle has been still long enough that the partial window is stale: scoring it
// against the elapsed time would report a nearly stationary spindle for movement that happened
// before the pause.
inline void encHealthRestartWindow(EncHealth* h, unsigned long nowUs) {
  h->pathCounts = 0;
  h->netCounts = 0;
  h->windowStartUs = nowUs;
}

// Feeds one counter delta in. Returns true on the count that completes a window, which is the
// only time anything is divided - every other call is two adds and a compare, because this runs
// in the motion loop alongside the step pulse train.
//
// busyRate is the count rate below which a window is measured but not judged. Spin-up, run-down
// and a spindle nudged by hand all produce low coherence honestly, and recording those would
// bury the ones that are actually noise.
inline bool encHealthAdd(EncHealth* h, long delta, unsigned long nowUs, unsigned long windowUs,
                         long busyRate, int coherenceFloor) {
  h->netCounts += delta;
  h->pathCounts += calAbsL(delta);
  // Unsigned throughout, so this stays correct across the micros() rollover every 71.6 minutes.
  unsigned long elapsed = nowUs - h->windowStartUs;
  if (elapsed < windowUs || elapsed == 0) {
    return false;
  }
  // 64-bit intermediates: 3000 rpm on a 4000 count encoder is 200k counts/sec, and counts times
  // a million overflows a 32-bit long well before that.
  h->pathRate = (long)((long long)h->pathCounts * 1000000LL / (long long)elapsed);
  h->netRate = (long)((long long)h->netCounts * 1000000LL / (long long)elapsed);
  h->coherence = h->pathCounts > 0 ? (int)(calAbsL(h->netCounts) * 100 / h->pathCounts) : 100;
  if (h->pathRate >= busyRate) {
    if (h->worstCoherence < 0 || h->coherence < h->worstCoherence) {
      h->worstCoherence = h->coherence;
    }
    if (h->coherence < coherenceFloor) {
      h->dirtyWindows++;
    }
  }
  h->pathCounts = 0;
  h->netCounts = 0;
  h->windowStartUs = nowUs;
  return true;
}

// Count rate a spindle at this many rpm produces, for turning the busy threshold into the units
// the window measures in.
inline long encRateFromRpm(long countsPerRev, long rpm) {
  return countsPerRev * rpm / 60;
}

#endif // ENCODER_HEALTH_H
