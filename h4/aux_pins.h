// Who owns the auxiliary terminals. Shared with the host test suite in test/ - pure arithmetic,
// no Arduino.
//
// A11-A13 and A21-A23 are the only spare pins on the board, and four different things want them:
//
//        A11    A12     A13    A21    A22     A23
//   A1   ENA    DIR     STEP   -      -       -
//   HW1  out    in/IRQ  in     -      -       -
//   HW2  -      -       -      out    in/IRQ  in
//   JOY  left   right   up     down   move    step
//
// All four are runtime settings, so nothing stopped two of them being switched on over the same
// three pins. That does not fail loudly - the pins simply get configured twice, and whichever
// device was set up last wins while the other reports nothing or moves an axis unbidden. This
// header is the single description of the overlap, so the settings layer can refuse the write
// instead.

#ifndef AUX_PINS_H
#define AUX_PINS_H

enum AuxDevice {
  AUX_A1_AXIS = 0,
  AUX_HANDWHEEL_1,
  AUX_HANDWHEEL_2,
  AUX_JOYSTICK,
  AUX_DEVICE_COUNT,
};

// Bit per terminal: 0 = A11, 1 = A12, 2 = A13, 3 = A21, 4 = A22, 5 = A23.
#define AUX_PIN_A11 (1 << 0)
#define AUX_PIN_A12 (1 << 1)
#define AUX_PIN_A13 (1 << 2)
#define AUX_PIN_A21 (1 << 3)
#define AUX_PIN_A22 (1 << 4)
#define AUX_PIN_A23 (1 << 5)

inline int auxClaimMask(int device) {
  switch (device) {
    case AUX_A1_AXIS:     return AUX_PIN_A11 | AUX_PIN_A12 | AUX_PIN_A13;
    case AUX_HANDWHEEL_1: return AUX_PIN_A11 | AUX_PIN_A12 | AUX_PIN_A13;
    case AUX_HANDWHEEL_2: return AUX_PIN_A21 | AUX_PIN_A22 | AUX_PIN_A23;
    case AUX_JOYSTICK:    return AUX_PIN_A11 | AUX_PIN_A12 | AUX_PIN_A13 |
                                 AUX_PIN_A21 | AUX_PIN_A22 | AUX_PIN_A23;
    default:              return 0;
  }
}

// Short enough to fit an LCD error line with "Used by " in front of it.
inline const char* auxDeviceName(int device) {
  switch (device) {
    case AUX_A1_AXIS:     return "A1 axis";
    case AUX_HANDWHEEL_1: return "handwheel 1";
    case AUX_HANDWHEEL_2: return "handwheel 2";
    case AUX_JOYSTICK:    return "joystick";
    default:              return "?";
  }
}

inline int auxEnabledMask(bool a1Active, bool handwheel1, bool handwheel2, bool joystick) {
  return (a1Active ? 1 << AUX_A1_AXIS : 0)
       | (handwheel1 ? 1 << AUX_HANDWHEEL_1 : 0)
       | (handwheel2 ? 1 << AUX_HANDWHEEL_2 : 0)
       | (joystick ? 1 << AUX_JOYSTICK : 0);
}

// The already-enabled device that would collide with switching `device` on, or -1 if the pins are
// free. A device never conflicts with itself, so re-enabling something already on is allowed.
//
// Returns the first collision rather than all of them: the operator has to resolve them one at a
// time anyway, and naming one thing to turn off is a clearer instruction than naming two.
inline int auxConflict(int device, int enabledMask) {
  int want = auxClaimMask(device);
  if (want == 0) {
    return -1;
  }
  for (int other = 0; other < AUX_DEVICE_COUNT; other++) {
    if (other == device || !(enabledMask & (1 << other))) {
      continue;
    }
    if (auxClaimMask(other) & want) {
      return other;
    }
  }
  return -1;
}

// Of everything that claims to be enabled, which devices actually get their pins.
//
// Needed because a setup stored before the conflict check existed can hold two devices on the same
// terminals, and because the conflicts are not pairwise-independent: dropping one device can free
// pins that make a later one viable, so the answer depends on the order they are granted in.
//
// The A1 axis goes first because it is part of the machine rather than an accessory - silently
// disabling a motion axis to keep a joystick alive would be the wrong way round - then the
// handwheels, then the stick, which needs all six terminals and so is the easiest to give up.
inline int auxResolveConflicts(int enabledMask) {
  static const int priority[AUX_DEVICE_COUNT] = {
    AUX_A1_AXIS, AUX_HANDWHEEL_1, AUX_HANDWHEEL_2, AUX_JOYSTICK,
  };
  int granted = 0;
  for (int i = 0; i < AUX_DEVICE_COUNT; i++) {
    int d = priority[i];
    if (!(enabledMask & (1 << d))) {
      continue;
    }
    if (auxConflict(d, granted) >= 0) {
      continue;
    }
    granted |= 1 << d;
  }
  return granted;
}

#endif // AUX_PINS_H
