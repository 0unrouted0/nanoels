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
  body: el(), // refreshBar() marks it while there are unsaved edits
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
  '\nmodule.exports = {fmtSpeed,speedToStored,parseCount,unitText,speedUnit,fmtDu,parseDu,status,makeRow,render};'
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
eq('metric parses back', page.speedToStored(100), 100);

group('only a distance takes a fraction');
// The panel refuses the decimal point on anything stored as a whole number. The page has to agree,
// and parseInt would not: it takes 4.5 as 4 and says nothing, so the setting silently becomes a
// different one from the one that was typed.
eq('a count reads back whole', page.parseCount('12'), 12);
ok('a fraction is refused rather than truncated', page.parseCount('4.5') === undefined);
ok('so is one written with a comma', page.parseCount('4,5') === undefined);
ok('rubbish is rejected rather than becoming zero', page.parseCount('abc') === null);
eq('a distance still takes its fraction', page.parseDu('4.25'), 42500);

group('rows, one per kind');
// A row is [label, control, unit]. Which control it gets is the whole of what `kind` decides,
// and a kind the page does not know silently falls through to a text box - which for a list
// would mean typing the index of a material by hand.
function rowFor(item) { return page.makeRow(item); }

const boolRow = rowFor({ kind: 'bool', label: 'Direction', key: 'einv', value: 1, on: 'inverted', off: 'normal' });
eq('a toggle shows its own wording', boolRow.children[1].textContent, 'inverted');
eq('and no unit', boolRow.children[2].textContent, '');

const listRow = rowFor({ kind: 'list', label: 'Material', key: 'cmat', value: 2, options: ['Manual', 'Aluminium', 'Brass'] });
eq('a list becomes a dropdown', listRow.children[1].children.length, 3);
eq('showing the names, not the index', listRow.children[1].children[1].textContent, 'Aluminium');
eq('selecting the stored one', listRow.children[1].value, '2');

const numRow = rowFor({ kind: 'num', label: 'Motor steps', key: 'Zmst', value: 800, unit: 'steps' });
eq('a number is shown plainly', numRow.children[1].value, '800');
eq('with the unit from the table', numRow.children[2].textContent, 'steps');

const duRow = rowFor({ kind: 'du', label: 'Backlash', key: 'Zbla', value: 6500 });
eq('a distance is converted', duRow.children[1].value, '0.65');
eq('and names its unit, which the table cannot', duRow.children[2].textContent, 'mm');

const speedRow = rowFor({ kind: 'speed', label: 'Manual speed', key: 'css', value: 120 });
eq('a speed in metric is itself', speedRow.children[1].value, '120');
eq('and names its unit too', speedRow.children[2].textContent, 'm/min');

// A phone keyboard with no point on it is a clearer refusal than an error after the fact.
eq('a distance gets the decimal keyboard', duRow.children[1].inputMode, 'decimal');
eq('a count gets the numeric one', numRow.children[1].inputMode, 'numeric');
eq('and so does a speed', speedRow.children[1].inputMode, 'numeric');

group('following a units change made on the panel');
// The measurement system can be changed on the machine while this page is open. Every distance
// and speed on it is rendered in whichever is current, so the values and the unit labels both
// have to be repainted - the label used to be written once and left.
const items = [
  { kind: 'du', label: 'Backlash', key: 'Zbla', value: 6500 },
  { kind: 'speed', label: 'Manual speed', key: 'css', value: 120 },
];
page.render({ version: 'H4 V16', metric: 1, sections: [{ name: 'Test', items: items }] });
eq('metric distance', items[0].row.children[1].value, '0.65');
eq('metric speed unit', items[1].row.children[2].textContent, 'm/min');

const inchPayload = {
  on: 0, busy: 0, mode: '', rpm: 0, z: 0, x: 0, a1: 0, surface: 0, targetRpm: 0,
  coherence: -1, worstCoherence: -1, dirtyWindows: 0, flips: 0, coherenceFloor: 95,
  a1active: 0, dia: 0, measure: 1, pitch: 0, uptime: 1,
};
group('status strip');
const payload = {
  on: 1, busy: 0, mode: 'TURN', rpm: 850, z: 123450, x: 254000, a1: 0,
  diameterDu: 508000, surface: 98, wanted: 120, material: 'Mild steel', targetRpm: 620,
  coherence: 100, worstCoherence: 96, dirtyWindows: 0, flips: 0, coherenceFloor: 95,
  a1active: 0, dia: 1, measure: 0, pitch: 20000, uptime: 42,
};
const chipOf = (text) => '<span' + strip.split('<span').find((c) => c.includes(text));

// status() fetches, so every assertion about what it drew has to wait a turn. Sequential awaits
// rather than nested callbacks: the order these run in is the point, and one payload mutating
// under another's pending promise is exactly the sort of test bug that reads as a code bug.
function poll(next) {
  global.fetch = () => Promise.resolve({ json: () => Promise.resolve(next) });
  strip = null;
  page.status();
  return new Promise((resolve) => setImmediate(resolve));
}

(async () => {
  // Switching the panel to inches has to repaint the values and the unit labels together.
  await poll(inchPayload);
  eq('the distance follows into inches', items[0].row.children[1].value, '0.02559');
  eq('and its unit label with it', items[0].row.children[2].textContent, 'in');
  eq('the speed follows too', items[1].row.children[1].value, '394');
  eq('and its unit label', items[1].row.children[2].textContent, 'ft/min');

  await poll(payload);
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

  payload.coherence = 62;
  payload.worstCoherence = 41;
  payload.dirtyWindows = 7;
  payload.flips = 3;
  await poll(payload);
  ok('a noisy signal is flagged', chipOf('signal').includes('bad'));
  ok('counts the bad windows', strip.includes('7 bad'));
  ok('counts the flips', strip.includes('3 flips'));

  // 200 against 620 is -67.7%, which rounds away from zero to -68.
  payload.rpm = 200;
  await poll(payload);
  ok('running slow reads negative', strip.includes('-68%'));

  // With constant speed off there is no target, and the chip must disappear rather than render a
  // division by zero.
  payload.targetRpm = 0;
  payload.surface = 0;
  await poll(payload);
  ok('no target, no chip', !strip.includes('Mild steel'));
  ok('no diameter, no cutting speed', !strip.includes('m/min'));
  ok('the rest of the strip survives', strip.includes('200'));

  console.log(`\n${checks} checks, ${failures} failures`);
  process.exit(failures === 0 ? 0 : 1);
})();
