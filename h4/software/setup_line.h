// The bottom line of an automated mode: the setup wizard's questions, and the pass counter once
// it is running. Shared with the host test suite in test/ - no Arduino here.
//
// This is the line that tells you how to drive the machine, and it was the hardest part of the
// firmware to be sure of: composed of twenty-odd lcd.print() calls across a branch per step, with
// no way to see what any of them produced without flashing it.

#ifndef SETUP_LINE_H
#define SETUP_LINE_H

#include "lcd_line.h"
#include "modes.h"

struct SetupLineInput {
  int mode;
  long setupIndex;      // 0 before the wizard starts
  long passes;          // the stored count
  long numpadPasses;    // what is being typed at step 1, or 0
  double taper;         // the stored taper, for the tapered thread's extra step
  double numpadTaper;   // what is being typed at that step, or 0
  long zOffsetDu;       // how far the tool moves to reach the start corner
  long xOffsetDu;
  long opIndex;         // which pass is running
  long opTotal;         // passes plus spring passes
  long springStart;     // opIndex above this is a spring pass
  bool auxForward;
  long dupr;
  bool metric;
  bool missingStops;
  bool isOn;
  bool inNumpad;
};

// "2/3 " in front of a question. Without it there is nothing to say the questions are a sequence
// with an end rather than an unbounded interrogation, and no way to tell how far in you are.
inline void setupLineStep(LcdLine* l, const SetupLineInput* in) {
  lcdLineLong(l, in->setupIndex);
  lcdLineChar(l, '/');
  lcdLineLong(l, modeLastSetupIndex(in->mode));
  lcdLineChar(l, ' ');
}

inline void buildSetupLine(LcdLine* l, const SetupLineInput* in) {
  lcdLineClear(l);
  const int m = in->mode;

  if (!in->inNumpad && in->missingStops) {
    lcdLineStr(l, modeNeedsZStops(m) ? "Set all stops" : "Set X stops");
    return;
  }

  if (!in->isOn && in->setupIndex == 1) {
    setupLineStep(l, in);
    long passes = in->numpadPasses != 0 ? in->numpadPasses : in->passes;
    if (passes > MODE_PASSES_MAX) passes = MODE_PASSES_MAX;
    lcdLineLong(l, passes);
    lcdLineStr(l, passes == 1 ? " pass?" : " passes?");
    return;
  }

  if (!in->isOn && in->setupIndex == 2) {
    setupLineStep(l, in);
    if (m == MODE_FACE) {
      lcdLineStr(l, in->auxForward ? "Right-left" : "Left-right");
    } else if (m == MODE_CUT) {
      // Cut-off takes its direction from the sign of the pitch rather than from this question,
      // so there is nothing here for the arrows to change.
      lcdLineStr(l, in->dupr >= 0 ? "Ext, pitch>0" : "Int, pitch<0");
      return;
    } else {
      lcdLineStr(l, in->auxForward ? "External" : "Internal");
    }
    // Nothing else on the panel says the arrows change the answer, and a question mark on its own
    // reads as a yes/no that play would answer.
    lcdLineStr(l, " <>");
    return;
  }

  if (!in->isOn && m == MODE_TPR && in->setupIndex == 3) {
    setupLineStep(l, in);
    lcdLineStr(l, "Taper ");
    lcdLineFixed(l, in->numpadTaper != 0 ? in->numpadTaper : in->taper, 5);
    lcdLineChar(l, '?');
    return;
  }

  if (!in->isOn && in->setupIndex == modeLastSetupIndex(m) && in->setupIndex > 0) {
    setupLineStep(l, in);
    lcdLineStr(l, "Go");
    // The offsets say the tool is about to move before it does, which is the point of the
    // confirmation. Two of them plus the step counter can overrun the line, so each goes in only
    // if it fits alongside the question mark.
    //
    // Each is rendered on its own first and measured, rather than its width being estimated. The
    // estimate is what let this line overrun in the first place, and a conservative one drops an
    // offset that would have fitted - which is how "Go Z-5mm X-2mm?" lost its X.
    LcdLine frag;
    if (in->zOffsetDu != 0) {
      lcdLineClear(&frag);
      lcdLineStr(&frag, " Z");
      lcdLineDeciMicrons(&frag, in->zOffsetDu, 2, in->metric);
      if (l->len + frag.len + 1 <= LCD_LINE_MAX) lcdLineStr(l, frag.buf);
    }
    if (in->xOffsetDu != 0) {
      lcdLineClear(&frag);
      lcdLineStr(&frag, " X");
      lcdLineDeciMicrons(&frag, in->xOffsetDu, 2, in->metric);
      if (l->len + frag.len + 1 <= LCD_LINE_MAX) lcdLineStr(l, frag.buf);
    }
    lcdLineChar(l, '?');
    return;
  }

  if (in->isOn && !in->inNumpad) {
    lcdLineStr(l, in->opIndex > in->springStart ? "Spring " : "Pass ");
    lcdLineLong(l, in->opIndex);
    lcdLineStr(l, " of ");
    lcdLineLong(l, in->opTotal);
    return; // the progress bar is drawn by the caller, which owns the block glyph
  }

  if (!in->isOn && !in->inNumpad && in->setupIndex == 0) {
    // The way in. With the stops set and nothing running there was nothing here at all, so the
    // wizard behind the play button was invisible unless you already knew it existed.
    lcdLineStr(l, "ON to set up ");
    lcdLineLong(l, modeLastSetupIndex(m));
    lcdLineStr(l, " steps");
  }
}

#endif // SETUP_LINE_H
