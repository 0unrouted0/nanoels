// G-code programs as files on LittleFS.
//
// They used to live in the Preferences "gc" namespace, which shares one 20kB NVS partition with
// every machine setting - so a couple of real programs filled it and the settings had nowhere
// left to go. The "spiffs" partition default.csv already reserves is 1.4MB and was unused.
//
// Nothing here may be called from the motion loop: every function touches flash.

#ifndef GCODE_STORE_H
#define GCODE_STORE_H

#include <LittleFS.h>

const int GCODE_MAX_PROGRAMS = 64;
const int GCODE_NAME_MAX = 24;
const char GCODE_SUFFIX[] = ".gcode";
const int GCODE_SUFFIX_LEN = 6;

int gcodeProgramIndex = 0; // Program the panel's GCODE mode has selected
int gcodeProgramCount = 0;
String gcodeIndexNames[GCODE_MAX_PROGRAMS];

bool gcodeCharValid(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
      c == '-' || c == '_' || c == ' ';
}

// Names arrive over HTTP as well as from the panel, so anything that could address a file outside
// the root - a slash, a dot, a control character - is refused rather than sanitised.
bool gcodeNameValid(const String& name) {
  if (name.length() < 2 || name.length() > GCODE_NAME_MAX) return false;
  for (int i = 0; i < (int) name.length(); i++) {
    if (!gcodeCharValid(name.charAt(i))) return false;
  }
  return true;
}

String gcodePath(const String& name) {
  return "/" + name + GCODE_SUFFIX;
}

// Anything gcodeNameValid() would refuse becomes an underscore. May still come back too short to
// be a valid name, which the callers handle - see gcodeUniqueName().
String gcodeSanitizeName(const String& raw) {
  String out = "";
  for (int i = 0; i < (int) raw.length() && (int) out.length() < GCODE_NAME_MAX; i++) {
    out += gcodeCharValid(raw.charAt(i)) ? raw.charAt(i) : '_';
  }
  out.trim();
  return out;
}

// A usable program name out of an uploaded file's name, so the browser does not have to ask for
// one.
String gcodeNameFromFilename(const String& filename) {
  int slash = filename.lastIndexOf('/');
  String name = slash >= 0 ? filename.substring(slash + 1) : filename;
  int dot = name.lastIndexOf('.');
  if (dot > 0) name = name.substring(0, dot);
  return gcodeSanitizeName(name);
}

// Alphabetical, because LittleFS returns directory entries in creation order - so the index the
// panel's up/down keys walk would rearrange itself every time a program was added or deleted.
void gcodeRefreshIndex() {
  gcodeProgramCount = 0;
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) return;
  for (File f = root.openNextFile(); f; f = root.openNextFile()) {
    String filename = f.name();
    f.close();
    if (filename.startsWith("/")) filename = filename.substring(1);
    if (!filename.endsWith(GCODE_SUFFIX)) continue;
    String name = filename.substring(0, filename.length() - GCODE_SUFFIX_LEN);
    if (name.length() == 0 || gcodeProgramCount >= GCODE_MAX_PROGRAMS) continue;
    int at = gcodeProgramCount;
    while (at > 0 && gcodeIndexNames[at - 1] > name) {
      gcodeIndexNames[at] = gcodeIndexNames[at - 1];
      at--;
    }
    gcodeIndexNames[at] = name;
    gcodeProgramCount++;
  }
  if (gcodeProgramIndex >= gcodeProgramCount) {
    gcodeProgramIndex = gcodeProgramCount > 0 ? gcodeProgramCount - 1 : 0;
  }
}

String gcodeNameAt(int index) {
  if (index < 0 || index >= gcodeProgramCount) return "";
  return gcodeIndexNames[index];
}

bool gcodeExists(const String& name) {
  return gcodeNameValid(name) && LittleFS.exists(gcodePath(name));
}

// A sanitised name that nothing is stored under yet. Two source names differing only in characters
// the filesystem will not take would otherwise collapse onto one file and lose a program.
String gcodeUniqueName(const String& raw, const String& fallback) {
  String base = gcodeSanitizeName(raw);
  if (base.length() < 2) base = fallback;
  if ((int) base.length() > GCODE_NAME_MAX - 3) base = base.substring(0, GCODE_NAME_MAX - 3);
  String name = base;
  for (int n = 2; n < 100 && gcodeExists(name); n++) {
    name = base + "_" + String(n);
  }
  return name;
}

long gcodeSizeOf(const String& name) {
  if (!gcodeNameValid(name)) return 0;
  File f = LittleFS.open(gcodePath(name), "r");
  if (!f) return 0;
  long size = f.size();
  f.close();
  return size;
}

size_t gcodeFreeBytes() {
  size_t total = LittleFS.totalBytes();
  size_t used = LittleFS.usedBytes();
  return total > used ? total - used : 0;
}

String gcodeReadProgram(const String& name) {
  if (!gcodeNameValid(name)) return "";
  File f = LittleFS.open(gcodePath(name), "r");
  if (!f) return "";
  String out;
  out.reserve(f.size() + 1);
  char buf[128];
  while (f.available()) {
    size_t n = f.readBytes(buf, sizeof(buf));
    out.concat(buf, n);
  }
  f.close();
  return out;
}

// Returns an error to report, or NULL. Overwrites an existing program of the same name.
const char* gcodeWriteProgram(const String& name, const String& text) {
  if (!gcodeNameValid(name)) return "name must be 2-24 chars, letters digits - _ space";
  if (text.length() < 2) return "program too short";
  // A new program has to fit; an existing one only has to fit its growth, but LittleFS frees the
  // old blocks only on close, so require the whole length either way.
  if (text.length() + 256 > gcodeFreeBytes() + gcodeSizeOf(name)) return "not enough space";
  if (!gcodeExists(name) && gcodeProgramCount >= GCODE_MAX_PROGRAMS) return "too many programs";

  File f = LittleFS.open(gcodePath(name), "w");
  if (!f) return "failed to open file";
  size_t written = f.print(text);
  f.close();
  if (written != text.length()) {
    LittleFS.remove(gcodePath(name));
    gcodeRefreshIndex();
    return "write failed, program removed";
  }
  gcodeRefreshIndex();
  return NULL;
}

bool gcodeDeleteByName(const String& name) {
  if (!gcodeExists(name)) return false;
  bool ok = LittleFS.remove(gcodePath(name));
  gcodeRefreshIndex();
  return ok;
}

bool gcodeDeleteByIndex(int index) {
  String name = gcodeNameAt(index);
  if (name.length() == 0) return false;
  return gcodeDeleteByName(name);
}

bool gcodeDeleteAll() {
  bool ok = true;
  // The index is a snapshot, so removing while walking the directory itself is avoided.
  for (int i = gcodeProgramCount - 1; i >= 0; i--) {
    if (!LittleFS.remove(gcodePath(gcodeIndexNames[i]))) ok = false;
  }
  gcodeProgramIndex = 0;
  gcodeRefreshIndex();
  return ok;
}

#endif // GCODE_STORE_H
