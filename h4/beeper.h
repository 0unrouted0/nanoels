// Buzzer patterns, and the non-blocking state machine that plays them. Shared with the host test
// suite in test/ - nothing here may touch Arduino, so the caller does the tone() calls.
//
// One tone for everything means the shop hears "something happened" and has to walk over and read
// the LCD to find out what. These are meant to be told apart across a running lathe with the
// operator's eyes on the work, so the differences are in count and pitch direction rather than in
// subtle timing.

#ifndef BEEPER_H
#define BEEPER_H

enum BeepPattern {
  BEEP_NONE = 0,
  BEEP_ACK,        // one mid tone - the general acknowledgement, what beep() has always been
  BEEP_REFUSED,    // two short low - an input was rejected, pairs with the error splash
  BEEP_DONE,       // rising pair - an automated operation finished
  BEEP_SYNC_LOST,  // three falling - the thread lost its phase, the one you must not miss
  BEEP_INDEX,      // one short high blip - an index position was reached
  BEEP_PATTERN_COUNT,
};

// Frequency 0 is a silent gap. Patterns are short on purpose: a long one still playing when the
// next event happens would either mask it or delay it.
struct BeepStep {
  int freq;
  int ms;
};

#define BEEP_MAX_STEPS 5

struct BeepPatternDef {
  BeepStep steps[BEEP_MAX_STEPS];
  int count;
};

inline const BeepPatternDef* beepPatternDef(int pattern) {
  static const BeepPatternDef defs[BEEP_PATTERN_COUNT] = {
    {{{0, 0}}, 0},                                                      // BEEP_NONE
    {{{1000, 250}}, 1},                                                 // BEEP_ACK
    {{{400, 80}, {0, 60}, {400, 80}}, 3},                               // BEEP_REFUSED
    {{{800, 120}, {1200, 160}}, 2},                                     // BEEP_DONE
    {{{1200, 100}, {0, 50}, {900, 100}, {0, 50}, {600, 160}}, 5},       // BEEP_SYNC_LOST
    {{{1600, 60}}, 1},                                                  // BEEP_INDEX
  };
  if (pattern <= BEEP_NONE || pattern >= BEEP_PATTERN_COUNT) {
    return &defs[BEEP_NONE];
  }
  return &defs[pattern];
}

struct BeeperState {
  int pattern;
  int step;
  unsigned long stepStartMs;
};

inline void beeperReset(BeeperState* s) {
  s->pattern = BEEP_NONE;
  s->step = -1;
  s->stepStartMs = 0;
}

// Arms a pattern. Starting one while another is playing replaces it rather than queueing: the
// newest event is the one worth hearing, and a queue would report things in the wrong order.
inline void beeperStart(BeeperState* s, int pattern, unsigned long nowMs) {
  s->pattern = pattern;
  s->step = -1;
  s->stepStartMs = nowMs;
}

// Advances the machine. Returns true only when the output should change, so the caller touches
// the buzzer on step boundaries rather than every pass of its loop. *freq is then what to play,
// 0 meaning silence.
inline bool beeperTick(BeeperState* s, unsigned long nowMs, int* freq) {
  if (s->pattern <= BEEP_NONE || s->pattern >= BEEP_PATTERN_COUNT) {
    return false;
  }
  const BeepPatternDef* d = beepPatternDef(s->pattern);
  if (d->count < 1) {
    s->pattern = BEEP_NONE;
    return false;
  }
  if (s->step < 0) {
    s->step = 0;
    s->stepStartMs = nowMs;
    *freq = d->steps[0].freq;
    return true;
  }
  // Unsigned, so a pattern straddling the millis() rollover finishes rather than hanging on its
  // current step for the next 49 days.
  if (nowMs - s->stepStartMs < (unsigned long)d->steps[s->step].ms) {
    return false;
  }
  s->step++;
  s->stepStartMs = nowMs;
  if (s->step >= d->count) {
    beeperReset(s);
    *freq = 0;
    return true;
  }
  *freq = d->steps[s->step].freq;
  return true;
}

#endif // BEEPER_H
