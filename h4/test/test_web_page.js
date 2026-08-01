// Host tests for the JavaScript in ../web_page.h, which is otherwise only exercised by opening
// the page on the machine - where a mistyped field name fails silently and leaves a blank chip.
//
// Extracts the script block from the header and runs it against a stubbed DOM, so these test the
// shipped code rather than a copy of it, the same way the C++ tests do.
//
// Run with test/run_web_tests.ps1 (needs node; the C++ suite does not depend on this).

const fs = require('fs');
const path = require('path');

const header = fs.readFileSync(path.join(__dirname, '..', 'web_page.h'), 'utf8');
const block = /<script>\s*([\s\S]*?)\s*<\/script>/.exec(header);
if (!block) {
  console.error('No <script> block found in web_page.h');
  process.exit(1);
}

// Enough DOM for the script to load and render. Everything returns a fresh node rather than a
// shared one, so appendChild chains do not alias.
function el() {
  return {
    style: {}, dataset: {}, children: [], textContent: '', innerHTML: '', value: '', type: '',
    className: '', inputMode: '',
    classList: { add() {}, remove() {}, toggle() {}, contains() { return false; } },
    appendChild(c) { this.children.push(c); return c; },
    insertBefore(c) { this.children.push(c); return c; },
    addEventListener() {},
    get nextElementSibling() { return null; },
    get parentNode() { return el(); },
  };
}

let strip = null;
global.document = {
  hidden: false,
  createElement: () => el(),
  addEventListener: () => {},
  getElementById(id) {
    const e = el();
    if (id === 'strip') {
      Object.defineProperty(e, 'innerHTML', {
        get() { return strip; }, set(v) { strip = v; }, configurable: true,
      });
    }
    return e;
  },
};
global.window = global;
global.fetch = () => new Promise(() => {});
global.setInterval = () => 0;
global.setTimeout = () => 0;
global.FormData = class { append() {} };
global.XMLHttpRequest = class {
  open() {} send() {} setRequestHeader() {} addEventListener() {}
  get upload() { return { addEventListener() {} }; }
};

const mod = { exports: {} };
new Function('module', 'exports', block[1] +
  '\nmodule.exports = {fmtSpeed,parseSpeed,unitText,speedUnit,fmtDu,parseDu,status};'
)(mod, mod.exports);
const page = mod.exports;

let checks = 0;
let failures = 0;
function ok(what, cond) {
  checks++;
  if (cond) { console.log('  ok   ' + what); }
  else { failures++; console.log('  FAIL ' + what); }
}
function eq(what, got, want) {
  checks++;
  if (got === want) { console.log('  ok   ' + what); }
  else { failures++; console.log(`  FAIL ${what}: got ${got}, want ${want}`); }
}
function group(name) { console.log('\n' + name); }

group('units');
// Distances and speeds carry no unit in the settings table - theirs follows the metric/inch
// setting - so the page has to supply one. It used to leave distances blank.
eq('distance names its unit', page.unitText({ kind: 'du' }), 'mm');
eq('speed names its unit', page.unitText({ kind: 'speed' }), 'm/min');
eq('a plain number uses the table unit', page.unitText({ kind: 'num', unit: 'teeth' }), 'teeth');
eq('no unit is blank, not undefined', page.unitText({ kind: 'num' }), '');
eq('a list has no unit', page.unitText({ kind: 'list' }), '');

group('surface speed conversion');
eq('metric passes through', page.fmtSpeed(100), '100');
eq('metric parses back', page.parseSpeed('100'), 100);
ok('rubbish is rejected rather than becoming zero', page.parseSpeed('abc') === null);

group('status strip');
const payload = {
  on: 1, busy: 0, mode: 'TURN', rpm: 850, z: 123450, x: 254000, a1: 0,
  diameterDu: 508000, surface: 98, wanted: 120, material: 'Mild steel', targetRpm: 620,
  coherence: 100, worstCoherence: 96, dirtyWindows: 0, flips: 0, coherenceFloor: 95,
  a1active: 0, dia: 1, measure: 0, pitch: 20000, uptime: 42,
};
global.fetch = () => Promise.resolve({ json: () => Promise.resolve(payload) });
const chipOf = (text) => '<span' + strip.split('<span').find((c) => c.includes(text));

page.status();
setImmediate(() => {
  ok('the strip renders', typeof strip === 'string' && strip.length > 0);
  ok('shows rpm', strip.includes('850'));
  ok('shows the cutting speed', strip.includes('98'));
  ok('names the material', strip.includes('Mild steel'));
  ok('shows the target rpm', strip.includes('620'));
  ok('shows the signal figure', strip.includes('signal'));
  ok('shows the low-water mark', strip.includes('low 96%'));
  ok('a clean signal is not flagged', !chipOf('signal').includes('bad'));
  // 850 against a 620 target is 37% fast, which has to look wrong as well as read wrong.
  ok('an off-target speed is flagged', chipOf('Mild steel').includes('bad'));
  ok('and says which way', chipOf('Mild steel').includes('+37%'));

  strip = null;
  payload.coherence = 62;
  payload.worstCoherence = 41;
  payload.dirtyWindows = 7;
  payload.flips = 3;
  page.status();
  setImmediate(() => {
    ok('a noisy signal is flagged', chipOf('signal').includes('bad'));
    ok('counts the bad windows', strip.includes('7 bad'));
    ok('counts the flips', strip.includes('3 flips'));

    // 200 against 620 is -67.7%, which rounds away from zero to -68.
    strip = null;
    payload.rpm = 200;
    page.status();
    setImmediate(() => {
      ok('running slow reads negative', strip.includes('-68%'));

      // With constant speed off there is no target, and the chip must disappear rather than
      // render a division by zero.
      strip = null;
      payload.targetRpm = 0;
      payload.surface = 0;
      page.status();
      setImmediate(() => {
        ok('no target, no chip', !strip.includes('Mild steel'));
        ok('no diameter, no cutting speed', !strip.includes('m/min'));
        ok('the rest of the strip survives', strip.includes('850') || strip.includes('200'));

        console.log(`\n${checks} checks, ${failures} failures`);
        process.exit(failures === 0 ? 0 : 1);
      });
    });
  });
});
