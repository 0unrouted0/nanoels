// Building a line of the display as text, so what the screen says can be tested rather than read.
// Shared with the host test suite in test/ - no Arduino here.
//
// Two problems this solves, both of which have already happened:
//
// A line longer than the display wraps onto the next one and overwrites it. The "Go Z-5.00 X-2.00?"
// confirmation could reach 23 characters with two large offsets, and the fix was to count them by
// hand at the call site. Here the builder simply stops at the width - an overrun becomes a
// truncated line rather than a corrupted screen, and the tests can assert the width directly.
//
// And nothing could check what a line said. Every screen was composed of lcd.print() calls, so
// the only way to know was to flash it and look.
//
// The number formatting is a transcription of Arduino's Print::printFloat rather than a call to
// snprintf, because the two round differently at the halfway point and a test that formats
// differently from the device is a test that lies.

#ifndef LCD_LINE_H
#define LCD_LINE_H

#include <string.h>

#define LCD_LINE_MAX 20

// Custom glyphs cannot travel through a C string as their real codes: the millimetre glyph is
// character 0, which would terminate the buffer. They travel as placeholders and whatever prints
// the line maps them back.
#define LCD_GLYPH_MM '\x01'

struct LcdLine {
  char buf[LCD_LINE_MAX + 1];
  int len;
  bool truncated; // something did not fit, which is worth asserting against in tests
};

inline void lcdLineClear(LcdLine* l) {
  l->buf[0] = 0;
  l->len = 0;
  l->truncated = false;
}

inline void lcdLineChar(LcdLine* l, char c) {
  if (l->len >= LCD_LINE_MAX) {
    l->truncated = true;
    return;
  }
  l->buf[l->len++] = c;
  l->buf[l->len] = 0;
}

inline void lcdLineStr(LcdLine* l, const char* s) {
  for (int i = 0; s[i] != 0; i++) lcdLineChar(l, s[i]);
}

inline void lcdLineULong(LcdLine* l, unsigned long v) {
  char tmp[12];
  int n = 0;
  if (v == 0) {
    lcdLineChar(l, '0');
    return;
  }
  while (v > 0 && n < 11) {
    tmp[n++] = (char)('0' + (v % 10));
    v /= 10;
  }
  while (n > 0) lcdLineChar(l, tmp[--n]);
}

inline void lcdLineLong(LcdLine* l, long v) {
  if (v < 0) {
    lcdLineChar(l, '-');
    lcdLineULong(l, (unsigned long)(-(v + 1)) + 1UL); // survives LONG_MIN
    return;
  }
  lcdLineULong(l, (unsigned long)v);
}

// Arduino's Print::printFloat, transcribed: add half of the last place, truncate the integer
// part, then take the fraction a digit at a time. Rounds half away from zero, where the C library
// rounds half to even - print(1.005, 2) is "1.01" here and "1.00" from snprintf.
inline void lcdLineFixed(LcdLine* l, double v, int digits) {
  if (v < 0.0) {
    lcdLineChar(l, '-');
    v = -v;
  }
  double rounding = 0.5;
  for (int i = 0; i < digits; i++) rounding /= 10.0;
  v += rounding;
  unsigned long ipart = (unsigned long)v;
  lcdLineULong(l, ipart);
  if (digits > 0) lcdLineChar(l, '.');
  double frac = v - (double)ipart;
  while (digits-- > 0) {
    frac *= 10.0;
    int d = (int)frac;
    lcdLineChar(l, (char)('0' + d));
    frac -= d;
  }
}

// Spaces up to a column, for the two-column layouts. Never removes anything already written.
inline void lcdLinePad(LcdLine* l, int toColumn) {
  while (l->len < toColumn) lcdLineChar(l, ' ');
}

// A distance in the units currently selected, with the trailing zeros dropped and the unit after
// it. Mirrors printDeciMicrons(), which chooses how many decimals to show from how many the value
// actually needs.
inline void lcdLineDeciMicrons(LcdLine* l, long deciMicrons, int precisionPointsMax, bool metric) {
  if (deciMicrons == 0) {
    lcdLineChar(l, '0');
    return;
  }
  long v = metric ? deciMicrons : (long)(deciMicrons / 25.4 + (deciMicrons > 0 ? 0.5 : -0.5));
  int points = 0;
  if (v == 0 && precisionPointsMax >= 5) points = 5;
  else if ((v % 10) != 0 && precisionPointsMax >= 4) points = 4;
  else if ((v % 100) != 0 && precisionPointsMax >= 3) points = 3;
  else if ((v % 1000) != 0 && precisionPointsMax >= 2) points = 2;
  else if ((v % 10000) != 0 && precisionPointsMax >= 1) points = 1;
  lcdLineFixed(l, deciMicrons / (metric ? 10000.0 : 254000.0), points);
  if (metric) {
    lcdLineChar(l, LCD_GLYPH_MM);
  } else {
    lcdLineChar(l, '"');
  }
}

// Room left before the line is full.
inline int lcdLineRoom(const LcdLine* l) {
  return LCD_LINE_MAX - l->len;
}

#endif // LCD_LINE_H
