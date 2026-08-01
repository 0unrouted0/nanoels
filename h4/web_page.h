// The configuration page served at http://192.168.4.1/ when WiFi is enabled.
//
// One self-contained document: no external stylesheets, fonts or scripts, because the controller
// makes its own access point and a phone joined to it has no route to the internet. Anything
// fetched from a CDN would simply never load.
//
// The page knows nothing about what the settings are. It renders whatever /api/settings returns,
// which is generated from SETTINGS[] in settings_table.h, so adding an item to the LCD menu makes
// it appear here with no change to this file.

#ifndef WEB_PAGE_H
#define WEB_PAGE_H

const char WEB_PAGE[] PROGMEM = R"HTMLPAGE(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>NanoEls</title>
<style>
:root{
  --bg:#f4f5f7; --card:#fff; --ink:#14161a; --dim:#5d6470; --line:#dde1e7;
  --accent:#0b6ec9; --ok:#1c7c40; --bad:#b5301f; --warn:#8a5a00; --warnbg:#fdf3d8;
}
@media (prefers-color-scheme:dark){
  :root{
    --bg:#14161a; --card:#1d2027; --ink:#e9ecf1; --dim:#99a1b0; --line:#2e333d;
    --accent:#54a8f0; --ok:#4cc47c; --bad:#ff7c68; --warn:#e0b45a; --warnbg:#2e2717;
  }
}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);
  font:15px/1.45 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif}
header{position:sticky;top:0;z-index:5;background:var(--card);border-bottom:1px solid var(--line);
  padding:.7rem 1rem}
h1{margin:0;font-size:1rem;font-weight:650;letter-spacing:.01em}
h1 span{color:var(--dim);font-weight:400}
#strip{display:flex;flex-wrap:wrap;gap:.45rem;margin-top:.5rem}
.chip{background:var(--bg);border:1px solid var(--line);border-radius:.4rem;
  padding:.2rem .5rem;font-size:.8rem;font-variant-numeric:tabular-nums;white-space:nowrap}
.chip b{font-weight:600}
.chip.live{color:var(--ok)}
.chip.bad{color:var(--warn);border-color:var(--warn)}
main{max-width:44rem;margin:0 auto;padding:1rem}
section{background:var(--card);border:1px solid var(--line);border-radius:.6rem;
  margin-bottom:.9rem;overflow:hidden}
section>h2{margin:0;padding:.65rem .9rem;font-size:.82rem;text-transform:uppercase;
  letter-spacing:.07em;color:var(--dim);border-bottom:1px solid var(--line);font-weight:650}
.row{display:flex;align-items:center;gap:.7rem;padding:.5rem .9rem;border-bottom:1px solid var(--line)}
.row:last-child{border-bottom:0}
.row label{flex:1;min-width:0}
.row .unit{color:var(--dim);font-size:.75rem;width:3.8rem;white-space:nowrap}
input[type=text]{width:7.5rem;padding:.4rem .5rem;text-align:right;
  font:inherit;font-variant-numeric:tabular-nums;
  background:var(--bg);color:var(--ink);border:1px solid var(--line);border-radius:.35rem}
input[type=text]:focus{outline:2px solid var(--accent);outline-offset:-1px;border-color:transparent}
/* Same footprint as the text box and the toggle, so every row's control lines up in one column. */
select{width:7.5rem;padding:.4rem .5rem;font:inherit;
  background:var(--bg);color:var(--ink);border:1px solid var(--line);border-radius:.35rem}
select:focus{outline:2px solid var(--accent);outline-offset:-1px;border-color:transparent}
.row.dirty select{border-color:var(--accent)}
button{font:inherit;cursor:pointer;border-radius:.35rem;border:1px solid var(--line);
  background:var(--bg);color:var(--ink);padding:.4rem .8rem}
button:hover{border-color:var(--accent)}
.toggle{width:7.5rem;font-weight:600}
.toggle[data-on="1"]{background:var(--accent);border-color:var(--accent);color:#fff}
.row.ok{animation:flash .9s}
@keyframes flash{from{background:rgba(28,124,64,.22)}to{background:transparent}}
.row.dirty{background:rgba(11,110,201,.07)}
.row.dirty label::after{content:" •";color:var(--accent);font-weight:700}
.row.dirty input[type=text]{border-color:var(--accent)}

/* Sticky action bar. Sits above the safe-area inset so it clears an iPhone's home indicator. */
#bar{position:fixed;left:0;right:0;bottom:0;z-index:10;display:none;
  align-items:center;gap:.7rem;padding:.7rem 1rem;
  padding-bottom:calc(.7rem + env(safe-area-inset-bottom));
  background:var(--card);border-top:1px solid var(--line);
  box-shadow:0 -4px 16px rgba(0,0,0,.13)}
#bar.show{display:flex}
#count{flex:1;font-size:.85rem;color:var(--dim)}
#save{background:var(--accent);border-color:var(--accent);color:#fff;font-weight:600;
  padding:.5rem 1.3rem}
#save:disabled{opacity:.6;cursor:default}
body.pending{padding-bottom:4.5rem}
.err{color:var(--bad);font-size:.78rem;padding:0 .9rem .5rem;margin-top:-.3rem}
#banner{display:none;background:var(--warnbg);color:var(--warn);border:1px solid var(--warn);
  border-radius:.5rem;padding:.6rem .8rem;margin-bottom:.9rem;font-size:.88rem}
#banner.show{display:block}
.tools{display:flex;flex-wrap:wrap;gap:.6rem;padding:.9rem}
.note{color:var(--dim);font-size:.8rem;padding:0 .9rem .9rem}
.tblwrap{overflow-x:auto}
table{border-collapse:collapse;width:100%;font-size:.85rem}
th,td{padding:.4rem .6rem;text-align:right;white-space:nowrap;border-bottom:1px solid var(--line)}
th:first-child,td:first-child{text-align:left}
thead th{color:var(--dim);font-weight:600;font-size:.75rem;text-transform:uppercase;letter-spacing:.05em}
tbody tr:last-child td{border-bottom:0}
td{font-variant-numeric:tabular-nums}
td.warn{color:var(--warn);font-weight:600}
progress{width:100%;height:.5rem;margin-top:.6rem}
a.btn{text-decoration:none}
</style>
</head>
<body>
<header>
  <h1>NanoEls <span id="ver"></span></h1>
  <div id="strip"></div>
</header>
<main>
  <div id="banner"></div>
  <div id="derived"></div>
  <div id="settings"></div>

  <section>
    <h2>Backup</h2>
    <div class="tools">
      <a class="btn" href="/api/dump" download><button type="button">Download settings</button></a>
    </div>
    <p class="note">Saves every value as text. Restore by pasting the lines back over USB serial.</p>
  </section>

  <section>
    <h2>Firmware update</h2>
    <div class="tools">
      <input type="file" id="fw" accept=".bin">
      <button type="button" id="flash">Install</button>
    </div>
    <progress id="prog" value="0" max="100" hidden></progress>
    <p class="note" id="fwnote">Upload a compiled .bin. The machine must be stopped. It restarts by
      itself when the update finishes &mdash; do not cut the power while it is writing.</p>
  </section>
</main>

<div id="bar">
  <span id="count"></span>
  <button type="button" id="discard">Discard</button>
  <button type="button" id="save">Save</button>
</div>
<script>
"use strict";
var DU_MM = 10000, DU_IN = 254000;
var metric = true, busy = false;

function fmtDu(v){
  if(metric) return (v/DU_MM).toFixed(4).replace(/0+$/,"").replace(/\.$/,"") || "0";
  return (v/DU_IN).toFixed(5).replace(/0+$/,"").replace(/\.$/,"") || "0";
}
function parseDu(t){
  var n = parseFloat(t);
  if(isNaN(n)) return null;
  return Math.round(n * (metric ? DU_MM : DU_IN));
}

// Surface speed travels as metres per minute the way distances travel as deci-microns, and is
// converted only here. A shop working in inches thinks in surface feet per minute.
var FT_PER_M = 3.280839895;
function fmtSpeed(v){ return String(metric ? v : Math.round(v * FT_PER_M)); }
function parseSpeed(t){
  var n = parseFloat(t);
  if(isNaN(n)) return null;
  return Math.round(metric ? n : n / FT_PER_M);
}
function speedUnit(){ return metric ? "m/min" : "ft/min"; }

function post(key, value){
  var body = "key=" + encodeURIComponent(key) + "&value=" + encodeURIComponent(value);
  return fetch("/api/setting", {
    method:"POST",
    headers:{"Content-Type":"application/x-www-form-urlencoded"},
    body:body
  }).then(function(r){ return r.json().catch(function(){ return {ok:false,error:"bad reply"}; }); });
}

function showError(row, msg){
  var e = row.nextElementSibling;
  if(!e || !e.classList.contains("err")){
    e = document.createElement("div");
    e.className = "err";
    row.parentNode.insertBefore(e, row.nextSibling);
  }
  e.textContent = msg;
}
function clearError(row){
  var e = row.nextElementSibling;
  if(e && e.classList.contains("err")) e.remove();
}
function flashOk(row){
  row.classList.remove("ok");
  void row.offsetWidth;
  row.classList.add("ok");
}

// Edits are staged, not sent as you type. Nothing reaches the controller until Save, so a
// half-typed number never lands on the machine and several related values (a pitch and the motor
// steps that go with it) commit together instead of one at a time.
var ALL = [];

function isDirty(item){ return item.pending !== undefined && item.pending !== item.value; }

function dirtyItems(){ return ALL.filter(isDirty); }

function refreshBar(){
  var n = dirtyItems().length;
  var bar = document.getElementById("bar");
  document.getElementById("count").textContent =
    n === 1 ? "1 unsaved change" : n + " unsaved changes";
  bar.classList.toggle("show", n > 0);
  // Reserve the bar's height so it can never cover the last row of settings.
  document.body.classList.toggle("pending", n > 0);
  ALL.forEach(function(it){
    if(it.row) it.row.classList.toggle("dirty", isDirty(it));
  });
}

function shown(item, v){
  if(item.kind === "du") return fmtDu(v);
  if(item.kind === "speed") return fmtSpeed(v);
  return String(v);
}

// Distances and speeds carry no unit in the settings table because theirs is not fixed - it
// follows the metric/inch setting - so it is supplied here. The panel has always named the unit
// beside a distance; this page used to leave it blank.
function unitText(item){
  if(item.kind === "du") return metric ? "mm" : "in";
  if(item.kind === "speed") return speedUnit();
  return item.unit || "";
}

function makeRow(item){
  var row = document.createElement("div");
  row.className = "row";
  item.row = row;
  var lab = document.createElement("label");
  lab.textContent = item.label;
  row.appendChild(lab);

  if(item.kind === "bool"){
    var b = document.createElement("button");
    b.type = "button";
    b.className = "toggle";
    var on = item.on || "on", off = item.off || "off";
    item.paint = function(){
      var v = item.pending !== undefined ? item.pending : item.value;
      b.dataset.on = v ? "1" : "0";
      b.textContent = v ? on : off;
    };
    item.paint();
    b.onclick = function(){
      var v = item.pending !== undefined ? item.pending : item.value;
      item.pending = v ? 0 : 1;
      item.paint();
      clearError(row);
      refreshBar();
    };
    row.appendChild(b);
    var pad = document.createElement("span");
    pad.className = "unit";
    row.appendChild(pad);
    return row;
  }

  if(item.kind === "list"){
    var sel = document.createElement("select");
    (item.options || []).forEach(function(name, i){
      var o = document.createElement("option");
      o.value = String(i);
      o.textContent = name;
      sel.appendChild(o);
    });
    item.paint = function(){
      sel.value = String(item.pending !== undefined ? item.pending : item.value);
    };
    item.paint();
    sel.onchange = function(){
      item.pending = parseInt(sel.value, 10);
      clearError(row);
      refreshBar();
    };
    row.appendChild(sel);
    var listPad = document.createElement("span");
    listPad.className = "unit";
    row.appendChild(listPad);
    return row;
  }

  var inp = document.createElement("input");
  inp.type = "text";
  inp.inputMode = "decimal";
  inp.value = shown(item, item.value);
  // Declared before paint() so the unit can be repainted with the value. Distances and speeds
  // have no fixed unit - it follows the metric/inch setting, which can be changed on the panel
  // while this page is open - so the label has to be rewritten, not just the number.
  var u = document.createElement("span");
  u.className = "unit";
  item.paint = function(){
    inp.value = shown(item, item.pending !== undefined ? item.pending : item.value);
    u.textContent = unitText(item);
  };
  inp.oninput = function(){
    var v = item.kind === "du" ? parseDu(inp.value)
          : item.kind === "speed" ? parseSpeed(inp.value)
          : parseInt(inp.value, 10);
    if(v === null || isNaN(v)){
      showError(row, "not a number");
      item.pending = undefined;
    } else {
      clearError(row);
      item.pending = v;
    }
    refreshBar();
  };
  // Enter saves everything rather than just this field, which is what you want after typing the
  // last of a group of related values.
  inp.onkeydown = function(e){ if(e.key === "Enter"){ e.preventDefault(); inp.blur(); saveAll(); } };
  row.appendChild(inp);
  u.textContent = unitText(item);
  row.appendChild(u);
  return row;
}

// Sequentially, never in parallel: each write takes the controller's motion lock and commits to
// flash, and firing sixty at once would queue them all behind each other anyway while making the
// failure reporting much harder to follow.
function saveAll(){
  var pending = dirtyItems();
  if(pending.length === 0) return;
  var btn = document.getElementById("save");
  btn.disabled = true;
  btn.textContent = "Saving...";
  var failed = 0;

  function next(i){
    if(i >= pending.length){
      btn.disabled = false;
      btn.textContent = "Save";
      refreshBar();
      loadDerived();
      if(failed === 0) document.getElementById("bar").classList.remove("show");
      return;
    }
    var item = pending[i];
    post(item.key, item.pending).then(function(r){
      if(r.ok){
        item.value = item.pending;
        item.pending = undefined;
        clearError(item.row);
        flashOk(item.row);
      } else {
        failed++;
        showError(item.row, r.error);
      }
      next(i + 1);
    });
  }
  next(0);
}

function discardAll(){
  ALL.forEach(function(it){
    if(isDirty(it)){
      it.pending = undefined;
      it.paint();
      clearError(it.row);
    }
  });
  refreshBar();
}

// ---- derived figures ----------------------------------------------------
// Nothing here is editable. It exists because the consequence of a setting is often invisible in
// the setting itself: a 3mm pitch on an axis that tops out at 300mm/min caps the spindle at
// 100rpm, and only this table says so.

function mmPerMin(du){ return metric ? (du/DU_MM).toFixed(0) : (du/DU_IN).toFixed(1); }
function len(du){ return fmtDu(du) + (metric ? " mm" : " in"); }
function trim1(n){ return n.toFixed(1).replace(/\.0$/,""); }
// Resolution in mm would read 0.0008 and lose the interesting digits, so use the units a
// machinist actually thinks in at this scale.
function res(du){ return metric ? (du/10).toFixed(2) + " µm" : (du/254).toFixed(4) + " thou"; }

function cell(text, warn){
  return "<td" + (warn ? ' class="warn"' : "") + ">" + text + "</td>";
}

function renderDerived(d){
  var speedUnit = metric ? "mm/min" : "in/min";
  var h = '<section><h2>Derived from your settings</h2><div class="tblwrap"><table>' +
    "<thead><tr><th>Axis</th><th>Resolution</th><th>Steps/" + (metric ? "mm" : "in") +
    "</th><th>Max feed</th><th>Stops in</th><th>Backlash</th><th>Max rpm</th></tr></thead><tbody>";

  d.axes.forEach(function(a){
    // Backlash compensation below one full step cannot do anything, which is worth flagging: the
    // setting looks active but has no effect.
    var blWarn = a.backlashSteps === 0;
    var bl = a.backlashSteps + (blWarn ? " (none)" : "");
    var rpm = d.pitch ? a.maxRpm : "—";
    h += "<tr><td>" + a.name + "</td>" +
      cell(res(a.resDu)) +
      cell(trim1(metric ? a.stepsPerMm : a.stepsPerMm * 25.4)) +
      cell(mmPerMin(a.feedDuMin) + " " + speedUnit) +
      cell(len(a.stopDu)) +
      cell(bl + " st", blWarn) +
      cell(rpm) +
      "</tr>";
  });
  h += "</tbody></table></div>";

  var e = d.encoder;
  h += '<div class="tblwrap"><table><thead><tr><th>Spindle</th><th>Counts/rev</th>' +
    "<th>Angle</th><th>Encoder limit</th><th>Spindle limit</th><th>Dead-band</th>" +
    "</tr></thead><tbody><tr>" +
    "<td>encoder</td>" +
    cell(e.counts + (e.divider > 1 ? " (÷" + e.divider + ")" : "")) +
    cell(e.deg.toFixed(3) + "°") +
    cell(e.maxEncoderRpm + " rpm") +
    cell(e.maxSpindleRpm + " rpm") +
    cell(e.symmetric ? "symmetric" : "one-way") +
    "</tr></tbody></table></div>";

  var notes = [];
  if(d.pitch){
    var slowest = Math.min.apply(null, d.axes.filter(function(a){ return a.fitted; })
                                             .map(function(a){ return a.maxRpm; }));
    notes.push("At the pitch currently set, keep the spindle under <b>" + slowest +
               " rpm</b> or the axis cannot keep up and the thread will lose sync.");
  } else {
    notes.push("Set a pitch to see the spindle speed the axes can keep up with.");
  }
  notes.push("The encoder stops being read reliably above <b>" + e.maxSpindleRpm +
             " rpm</b> at the spindle. Raise the glitch filter only if you understand that trade.");
  notes.push("The <b>signal</b> chip at the top is live: it reads 100% on a clean encoder and " +
             "keeps the lowest figure seen, so leave this page open through a job and check it " +
             "afterwards. If it falls, fix the cable before masking it with the glitch filter or " +
             "a symmetric dead-band.");
  h += '<p class="note">' + notes.join("<br>") + "</p></section>";

  document.getElementById("derived").innerHTML = h;
}

function loadDerived(){
  fetch("/api/derived").then(function(r){ return r.json(); })
    .then(renderDerived).catch(function(){});
}

function render(data){
  document.getElementById("ver").textContent = data.version;
  metric = !!data.metric;
  ALL = [];
  var host = document.getElementById("settings");
  host.textContent = "";
  data.sections.forEach(function(sec){
    var s = document.createElement("section");
    var h = document.createElement("h2");
    h.textContent = sec.name;
    s.appendChild(h);
    sec.items.forEach(function(it){ ALL.push(it); s.appendChild(makeRow(it)); });
    host.appendChild(s);
  });
  refreshBar();
}

function chip(label, value, live, bad){
  return '<span class="chip' + (live ? " live" : "") + (bad ? " bad" : "") + '">' + label +
         ' <b>' + value + '</b></span>';
}

function status(){
  if(document.hidden) return;
  fetch("/api/status").then(function(r){ return r.json(); }).then(function(s){
    // The measurement system can be changed on the panel while this page is open. Every distance
    // and speed on it is rendered in whichever is current, so follow the change rather than
    // showing millimetres until someone reloads.
    if(s.measure !== undefined){
      var wasMetric = metric;
      metric = s.measure === 0;
      if(metric !== wasMetric){
        ALL.forEach(function(it){ if(it.paint) it.paint(); });
        loadDerived();
      }
    }
    var html = "";
    html += chip("", s.on ? "RUNNING" : "stopped", s.on);
    if(s.mode) html += chip("mode", s.mode);
    html += chip("rpm", s.rpm);
    html += chip("Z", fmtDu(s.z) + (metric ? " mm" : " in"));
    html += chip(s.dia ? "X&oslash;" : "X", fmtDu(s.x) + (metric ? " mm" : " in"));
    if(s.a1active) html += chip("C", fmtDu(s.a1));

    // Cutting speed at the tool. Only meaningful once there is a diameter to cut, which means X
    // zeroed on the centerline - before that it reads zero and saying so would just be noise.
    if(s.surface > 0) html += chip("cut", fmtSpeed(s.surface) + " " + speedUnit());

    // Constant speed: what the spindle should be doing, and which way to turn the dial. Green
    // once you are within a tenth of the target, which is as close as a belt-step lathe gets.
    if(s.targetRpm > 0){
      var off = Math.round((s.rpm - s.targetRpm) / s.targetRpm * 100);
      var near = Math.abs(off) <= 10;
      html += chip(s.material, s.targetRpm + " rpm " + (off >= 0 ? "+" : "") + off + "%", near, !near);
    }

    // Encoder signal. The low-water mark is the number that matters, so it is shown beside the
    // current one rather than buried - noise arrives in bursts nobody is watching for.
    if(s.coherence >= 0){
      var lo = s.worstCoherence;
      var dirty = s.dirtyWindows > 0 || s.flips > 0;
      var sig = s.coherence + "%";
      if(lo >= 0 && lo < s.coherence) sig += " low " + lo + "%";
      if(s.dirtyWindows > 0) sig += " · " + s.dirtyWindows + " bad";
      if(s.flips > 0) sig += " · " + s.flips + " flips";
      html += chip("signal", sig, !dirty && lo >= s.coherenceFloor, dirty || (lo >= 0 && lo < s.coherenceFloor));
    }
    document.getElementById("strip").innerHTML = html;

    busy = !!s.busy;
    var b = document.getElementById("banner");
    if(busy){
      b.textContent = "The machine is running. Settings and firmware updates are refused until it is stopped.";
      b.classList.add("show");
    } else {
      b.classList.remove("show");
    }
  }).catch(function(){});
}

document.getElementById("save").onclick = saveAll;
document.getElementById("discard").onclick = discardAll;

// Staged edits live only in the page, so leaving with unsaved ones loses them silently.
window.onbeforeunload = function(e){
  if(dirtyItems().length > 0){ e.preventDefault(); return ""; }
};

document.getElementById("flash").onclick = function(){
  var f = document.getElementById("fw").files[0];
  var note = document.getElementById("fwnote");
  if(!f){ note.textContent = "Choose a .bin file first."; return; }
  if(busy){ note.textContent = "Stop the machine before updating."; return; }
  var prog = document.getElementById("prog");
  prog.hidden = false;
  var fd = new FormData();
  fd.append("firmware", f, f.name);
  var xhr = new XMLHttpRequest();
  xhr.open("POST", "/update");
  xhr.upload.onprogress = function(e){
    if(e.lengthComputable) prog.value = (e.loaded / e.total) * 100;
  };
  xhr.onload = function(){
    var r = {};
    try { r = JSON.parse(xhr.responseText); } catch(err){}
    if(r.ok){
      note.textContent = "Installed. The controller is restarting - reconnect to its WiFi in a few seconds.";
    } else {
      note.textContent = "Update failed: " + (r.error || xhr.status);
      prog.hidden = true;
    }
  };
  xhr.onerror = function(){
    // The controller reboots the moment it finishes, so a dropped connection here is expected and
    // usually means it worked.
    note.textContent = "Connection closed. If the update completed the controller is restarting.";
  };
  xhr.send(fd);
};

// Settings first: it sets `metric`, which decides the units the derived table is written in.
fetch("/api/settings").then(function(r){ return r.json(); }).then(function(d){
  render(d);
  loadDerived();
}).catch(function(){
  document.getElementById("settings").textContent = "Could not load settings.";
});
status();
setInterval(status, 1000);
</script>
</body>
</html>
)HTMLPAGE";

#endif
