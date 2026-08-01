// Dividing the spindle into equal positions, and the constant-surface-speed arithmetic. Both are
// things the machine can work out from what it already measures - spindle angle and X position -
// and both are shared with the host test suite in test/, so no Arduino here.

#ifndef INDEXING_H
#define INDEXING_H

#include "calibration_math.h"

// Where the spindle sits relative to the nearest of `divisions` equally spaced marks.
//
// The encoder already resolves a revolution far finer than any chuck can be positioned by hand -
// 4000 counts is 0.09 degrees - so the machine can act as the dividing head it does not have. Turn
// the chuck until the error reads zero, cut, repeat. With SLOT mode that gives hex flats, keyway
// sets and splines without an indexer.
struct IndexPosition {
  long index;      // 0 to divisions-1, the nearest mark
  long errCounts;  // signed: how far past that mark the spindle is
};

// posInRev must already be reduced to 0..countsPerRev-1, which is what spindleModulo gives.
//
// Divisions rarely divide the count evenly - 4000 counts into 6 is 666.67 - so the target for each
// mark is rounded individually rather than derived by repeatedly adding a truncated step. Adding a
// step would accumulate the truncation and leave the last mark short by a count per division.
inline IndexPosition indexNearest(long posInRev, long countsPerRev, long divisions) {
  IndexPosition r;
  r.index = 0;
  r.errCounts = 0;
  if (divisions < 1 || countsPerRev < 1) {
    return r;
  }
  // Normalised here rather than trusted. spindleModulo already returns 0..countsPerRev-1 so the
  // firmware never passes anything else, but a negative slipping through would truncate towards
  // zero in the division below and hand back a negative mark number - which reads on the display
  // as mark 0 or -1 of 6, and looks like an encoder fault rather than an arithmetic one.
  posInRev = calSpindleModulo(posInRev, (int)countsPerRev);
  // Nearest mark, rounding half up. The result can land on `divisions` itself, meaning the spindle
  // is just short of coming back round to mark 0 - which is the same mark a whole turn later.
  long nearest = (posInRev * divisions + countsPerRev / 2) / countsPerRev;
  long target = (nearest * countsPerRev + divisions / 2) / divisions;
  r.errCounts = posInRev - target;
  r.index = nearest >= divisions ? 0 : nearest;
  return r;
}

inline float indexErrorDeg(long errCounts, long countsPerRev) {
  if (countsPerRev < 1) {
    return 0;
  }
  return errCounts * 360.0f / countsPerRev;
}

// How close counts as on the mark, from a tolerance given in tenths of a degree. Never returns
// zero: a tolerance finer than the encoder can resolve would be unreachable, so the machine would
// never say you had arrived.
inline long indexToleranceCounts(long countsPerRev, long tolTenthsDeg) {
  long counts = countsPerRev * tolTenthsDeg / 3600;
  return counts < 1 ? 1 : counts;
}

inline bool indexOnMark(long errCounts, long tolCounts) {
  return calAbsL(errCounts) <= tolCounts;
}

// ---------------------------------------------------------------------------
// Constant surface speed
// ---------------------------------------------------------------------------
//
// Cutting speed is what the tool actually feels, and it falls with diameter: the same rpm that is
// right on 50mm stock is a third of the speed on 16mm, and facing towards centre takes it to zero.
// The controller has both numbers already - X gives the diameter, the encoder gives the rpm - so
// it can say what the spindle should be doing.
//
// It cannot do it for you. There is no spindle output on this board; every pin is spoken for by
// the steppers, the encoder, the display and the keypad. So this advises, and the operator turns
// the dial. Keeping the target in one function means a VFD output could drive it later without any
// of the arithmetic moving.

// Cutting speeds by material and tool, in metres per minute.
//
// These are conservative starting points for turning, not gospel: the right speed also depends on
// depth of cut, feed, rigidity, whether there is coolant, and how the particular alloy behaves.
// They are here so the machine can suggest something sane rather than leave a number nobody knows
// how to pick, and every one of them is a figure a machinist would recognise as a safe first cut.
//
// Index 0 is Manual, which means "use the number I typed" rather than a material at all. Keeping
// it in the same list is what lets one setting cover both, so nothing has to explain which of two
// speed sources is in charge.
struct MaterialPreset {
  const char* name;   // at most 12 characters, to fit "Now " and the name on a 20-column line
  int hss;            // m/min with a high speed steel tool
  int carbide;        // m/min with carbide
};

inline int materialCount() {
  return 12;
}

inline const MaterialPreset* materialAt(int index) {
  static const MaterialPreset materials[12] = {
    {"Manual",       0,   0},
    {"Aluminium",    70,  200},
    {"Brass",        60,  180},
    {"Bronze",       40,  120},
    {"Copper",       50,  150},
    {"Cast iron",    25,  90},
    {"Mild steel",   30,  120},
    {"Alloy steel",  20,  90},
    {"Tool steel",   15,  60},
    {"Stainless",    15,  60},
    {"Titanium",     10,  40},
    {"Plastic",      100, 250},
  };
  if (index < 0 || index >= 12) {
    return &materials[0];
  }
  return &materials[index];
}

inline const char* materialName(int index) {
  return materialAt(index)->name;
}

// The surface speed actually in force: the material's figure for the fitted tool, or the typed
// number when the material is Manual. One function so the display, the web status and anything
// added later cannot disagree about which source wins.
inline long cssSpeedFor(int material, bool carbide, long manualMPerMin) {
  if (material <= 0 || material >= materialCount()) {
    return manualMPerMin < 0 ? 0 : manualMPerMin;
  }
  const MaterialPreset* m = materialAt(material);
  return carbide ? m->carbide : m->hss;
}

// Spindle rpm that gives this surface speed at this diameter.
//
//   rpm = 1000 * v / (pi * d)      v in m/min, d in mm
//
// Diameter comes in as deci-microns to match the axis positions, and the surface speed in whole
// m/min, which is how tooling is specified.
inline long cssTargetRpm(long diameterDu, long surfaceMPerMin) {
  if (diameterDu <= 0 || surfaceMPerMin <= 0) {
    return 0;
  }
  double diameterMm = diameterDu / 10000.0;
  return calRoundL(1000.0 * surfaceMPerMin / (3.14159265358979 * diameterMm));
}

// The surface speed a given diameter and rpm actually produce, in m/min. The other direction, for
// showing what is happening rather than what should be.
inline long cssActualMPerMin(long diameterDu, long rpm) {
  if (diameterDu <= 0 || rpm <= 0) {
    return 0;
  }
  double diameterMm = diameterDu / 10000.0;
  return calRoundL(3.14159265358979 * diameterMm * rpm / 1000.0);
}

// Percentage the actual rpm is off the target, signed, for a readout that says which way to turn
// the dial.
//
// Only the high side needs clamping. A stopped spindle is 100% below target and nothing can be
// further below, so the low end floors itself at -100; the high end has no such limit, and near
// the centre of a facing cut the target collapses towards zero and the figure would otherwise run
// off the line it shares with the rpm.
inline int cssDeviationPercent(long targetRpm, long actualRpm) {
  if (targetRpm <= 0) {
    return 0;
  }
  long pct = (actualRpm - targetRpm) * 100 / targetRpm;
  return pct > 999 ? 999 : (int)pct;
}

// A target the lathe cannot reach is worse than no target: it reads as a fault when it is really
// the physics of small diameters. Anything above the machine's own ceiling is reported as capped
// so the display can say so rather than showing an rpm nobody can dial in.
inline long cssCappedRpm(long targetRpm, long maxRpm) {
  if (maxRpm > 0 && targetRpm > maxRpm) {
    return maxRpm;
  }
  return targetRpm;
}

#endif // INDEXING_H
