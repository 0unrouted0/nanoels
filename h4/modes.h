// What each operating mode is, and everything that can be answered about one without touching
// hardware. Shared with the host test suite in test/ - no Arduino here.
//
// These predicates are read all over the sketch and a wrong answer is quiet rather than loud: a
// mode missing from needZStops() lets an operation start with no limits set, and the wrong start
// corner sends the tool to the far end of the work before the first cut. They were spread through
// h4.ino as one-line functions over a global, which put them out of reach of any test.
//
// Every function here takes the mode rather than reading one, which is what makes them testable
// and also what lets the settings layer ask about a mode it is not currently in.

#ifndef MODES_H
#define MODES_H

#include "settings_table.h"

#define MODE_NORMAL 0
#define MODE_XGEAR 1
#define MODE_ASYNC 2
#define MODE_CONE 3
#define MODE_TURN 4
#define MODE_FACE 5
#define MODE_CUT 6
#define MODE_THREAD 7
#define MODE_ELLIPSE 8
#define MODE_GCODE 9
#define MODE_A1 10
// 11 is left free: it is MODE_JOYSTICK on H5, which needs an analog stick this board doesn't have.
#define MODE_SLOT 12
#define MODE_TPR 13

// Threading, straight or tapered. TPR is identical to THREAD everywhere except for one extra
// setup step and the X drift applied during the cut, so almost everything asks this instead.
inline bool modeIsThread(int m) {
  return m == MODE_THREAD || m == MODE_TPR;
}

// Spindle-synchronised continuous feed. Which axis it drives is the only difference between the
// two: MODE_NORMAL feeds Z along the bed, MODE_XGEAR feeds X across the face.
inline bool modeIsGearbox(int m) {
  return m == MODE_NORMAL || m == MODE_XGEAR;
}

inline bool modeNeedsZStops(int m) {
  return m == MODE_TURN || m == MODE_FACE || modeIsThread(m) || m == MODE_ELLIPSE || m == MODE_SLOT;
}

// Runs a series of passes between the stops rather than feeding continuously, which is what makes
// it need the setup wizard and a pass count.
inline bool modeIsPass(int m) {
  return m == MODE_TURN || m == MODE_FACE || m == MODE_CUT || modeIsThread(m) ||
      m == MODE_ELLIPSE || m == MODE_SLOT;
}

inline bool modeAllowsManualMovesWhenOn(int m) {
  return modeIsGearbox(m) || m == MODE_ASYNC || m == MODE_CONE || m == MODE_A1;
}

// Whether pressing the same button again lands on a different mode. Gear hides XGEAR and thread
// hides TPR, and nothing on the panel says so - hence the marker beside the name.
inline bool modeHasSiblingOf(int m) {
  return m == MODE_NORMAL || m == MODE_THREAD;
}

// ---------------------------------------------------------------------------
// Buttons that open more than one screen
// ---------------------------------------------------------------------------
//
// Three keys carry several modes each: gear covers the two gearboxes, thread covers straight and
// tapered, and the mode key covers everything without a key of its own. Pressing one of those used
// to step to the next mode in its list from wherever you were, which meant leaving a screen and
// coming back put you on a different one - press gear from turning and you landed on the Z gearbox
// even though you had been using the cross-feed one, and the only way back was to press it again
// and watch the machine change what it drives in between.
//
// So the first press returns to whichever of that button's screens you were last on, and only a
// second press moves along its list. Getting back to where you were costs one press and changes
// nothing about how the machine behaves on the way.

#define MODE_GROUP_NONE 0
#define MODE_GROUP_GEARBOX 1
#define MODE_GROUP_THREAD 2
#define MODE_GROUP_OTHER 3
#define MODE_GROUP_COUNT 4

// The modes reached by the mode key - the ones with no dedicated key of their own.
inline bool modeIsOther(int m) {
  return m == MODE_A1 || m == MODE_ELLIPSE || m == MODE_GCODE || m == MODE_ASYNC || m == MODE_SLOT;
}

inline int modeGroupOf(int m) {
  if (modeIsGearbox(m)) return MODE_GROUP_GEARBOX;
  if (modeIsThread(m)) return MODE_GROUP_THREAD;
  if (modeIsOther(m)) return MODE_GROUP_OTHER;
  return MODE_GROUP_NONE;
}

// The next screen on the same button, wrapping round. hasA1 says whether the third axis is fitted:
// its screen is only reachable on a machine that has one, so it drops out of the ring otherwise.
inline int modeNextInGroup(int m, bool hasA1) {
  switch (m) {
    case MODE_NORMAL: return MODE_XGEAR;
    case MODE_XGEAR: return MODE_NORMAL;
    case MODE_THREAD: return MODE_TPR;
    case MODE_TPR: return MODE_THREAD;
    case MODE_A1: return MODE_ELLIPSE;
    case MODE_ELLIPSE: return MODE_GCODE;
    case MODE_GCODE: return MODE_ASYNC;
    case MODE_ASYNC: return MODE_SLOT;
    case MODE_SLOT: return hasA1 ? MODE_A1 : MODE_ELLIPSE;
    default: return m; // single-screen buttons have nowhere else to go
  }
}

// Where a button lands the first time it is pressed, before it has anything to remember.
inline int modeGroupDefault(int group, bool hasA1) {
  switch (group) {
    case MODE_GROUP_GEARBOX: return MODE_NORMAL;
    case MODE_GROUP_THREAD: return MODE_THREAD;
    case MODE_GROUP_OTHER: return hasA1 ? MODE_A1 : MODE_ELLIPSE;
    default: return MODE_NORMAL;
  }
}

// Shown on the panel and in the web status, so the two never disagree about what the machine is
// doing. MODE_NORMAL is the plain gearbox and shows nothing, which is why this can return "".
inline const char* modeNameOf(int m) {
  switch (m) {
    case MODE_XGEAR: return "XGEAR";
    case MODE_SLOT: return "SLOT";
    case MODE_TPR: return "TPR";
    case MODE_ASYNC: return "ASY";
    case MODE_CONE: return "CONE";
    case MODE_TURN: return "TURN";
    case MODE_FACE: return "FACE";
    case MODE_CUT: return "CUT";
    case MODE_THREAD: return "THRD";
    case MODE_ELLIPSE: return "ELLI";
    case MODE_GCODE: return "GCODE";
    case MODE_A1: return "A1";
    default: return "";
  }
}

// How many questions the setup wizard asks before the next play press starts the machine. The
// last step is always the "Go?" confirmation, so every mode ends the same way - cone used to
// return 2 here while its display drew "Go?" at step 3, which meant the step never arrived and
// cone was the one mode that began cutting straight off a question.
inline int modeLastSetupIndex(int m) {
  if (m == MODE_GCODE) return 2; // program, spindle check
  if (m == MODE_CONE) return 3; // ratio, external/internal, go
  if (m == MODE_TPR) return 4; // passes, external/internal, taper ratio, go
  if (modeIsPass(m)) return 3; // passes, external/internal, go
  return 0;
}

// Which set of per-mode settings a running mode uses. Modes with nothing of their own - the
// gearbox, async, gcode, A1 - map to SMODE_NONE.
inline int modeSettingsOf(int m) {
  switch (m) {
    case MODE_TURN: return SMODE_TURN;
    case MODE_FACE: return SMODE_FACE;
    case MODE_CUT: return SMODE_CUT;
    case MODE_THREAD: return SMODE_THREAD;
    case MODE_TPR: return SMODE_TPR;
    case MODE_ELLIPSE: return SMODE_ELLIPSE;
    case MODE_SLOT: return SMODE_SLOT;
    case MODE_CONE: return SMODE_CONE;
    default: return SMODE_NONE;
  }
}

// Which axis the spindle drives in this mode: true for X, false for Z. Facing and the cross-feed
// gearbox drive the cross slide, everything else drives the carriage.
inline bool modePitchOnX(int m) {
  return m == MODE_FACE || m == MODE_XGEAR;
}

// The corner an operation starts from, given the mode's stops. Getting these wrong sends the tool
// to the far end of the work before the first cut, so they are worth stating once and testing
// rather than repeating at each call site.
//
// leftStop is the larger coordinate on both axes - that is the axis convention throughout - so
// "start at the right" means start at rightStop.
inline long modeZStart(int m, long zLeftStop, long zRightStop, long zPos, long dupr,
                       bool auxForward) {
  if (m == MODE_TURN || modeIsThread(m)) return dupr > 0 ? zRightStop : zLeftStop;
  // Facing takes its direction from external/internal rather than from the sign of the pitch,
  // because the pitch is the depth of cut per revolution and says nothing about which way along
  // the face it travels.
  if (m == MODE_FACE) return auxForward ? zRightStop : zLeftStop;
  if (m == MODE_ELLIPSE) return dupr > 0 ? zLeftStop : zRightStop;
  // Slotting always starts at the right and cuts to the left, like a shaper stroke.
  if (m == MODE_SLOT) return zRightStop;
  return zPos;
}

inline long modeXStart(int m, long xLeftStop, long xRightStop, long xPos, long dupr,
                       bool auxForward) {
  if (m == MODE_TURN || modeIsThread(m)) return auxForward ? xRightStop : xLeftStop;
  if (m == MODE_FACE || m == MODE_CUT) return dupr > 0 ? xRightStop : xLeftStop;
  if (m == MODE_ELLIPSE) return xRightStop;
  if (m == MODE_SLOT) return auxForward ? xRightStop : xLeftStop;
  return xPos;
}

#endif // MODES_H
