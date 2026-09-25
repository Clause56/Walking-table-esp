#include "web.h"

#include <WebServer.h>
#include <WiFi.h>

#include "config.h"

namespace {
WebServer server(80);
ControllerState *st = nullptr;
SbusReceiver *rx = nullptr;
Motor *mL = nullptr;
Motor *mR = nullptr;

const char PAGE[] PROGMEM = R"HTML(<!DOCTYPE html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Walking Table</title>
<style>
body{font-family:system-ui,sans-serif;margin:0;padding:16px;background:#111;color:#eee;max-width:480px;margin:auto}
h1{font-size:20px;margin:0 0 12px}
.card{background:#1d1d1d;border-radius:10px;padding:12px;margin-bottom:12px}
.row{display:flex;justify-content:space-between;padding:3px 0}
.ok{color:#4caf50}.bad{color:#f44336}.warn{color:#ffb300}
button{font-size:18px;padding:14px;border:0;border-radius:8px;width:100%;color:#fff;cursor:pointer}
#en.on{background:#2e7d32}#en.off{background:#c62828}
#stop{background:#c62828;margin-top:10px}
.bar{height:10px;background:#333;border-radius:5px;position:relative;margin:4px 0 8px}
.bar i{position:absolute;top:0;bottom:0;background:#42a5f5;border-radius:5px}
#pad{width:240px;height:240px;border-radius:50%;background:#2a2a2a;margin:8px auto;position:relative;touch-action:none}
#knob{width:70px;height:70px;border-radius:50%;background:#42a5f5;position:absolute;left:85px;top:85px}
.dis{opacity:.35;pointer-events:none}
small{color:#999}
</style></head><body>
<h1>Walking Table</h1>
<div class="card">
<div class="row"><span>RC receiver</span><b id="rc">-</b></div>
<div class="row"><span>Control source</span><b id="src">-</b></div>
<div class="row"><span>SBUS frames</span><span id="fr">-</span></div>
<div class="row"><span>Uptime</span><span id="up">-</span></div>
<div>Left motor <span id="lv"></span></div><div class="bar"><i id="lb"></i></div>
<div>Right motor <span id="rv"></span></div><div class="bar"><i id="rb"></i></div>
<small id="ch"></small>
</div>
<div class="card"><button id="en" class="on">ENABLED</button></div>
<div class="card" id="man">
<b>Manual drive</b> <small id="mnote"></small>
<div id="pad"><div id="knob"></div></div>
<div>Max speed <span id="spv">50</span>%</div>
<input id="sp" type="range" min="10" max="100" value="50" style="width:100%">
<button id="stop">STOP</button>
</div>
<script>
const $=id=>document.getElementById(id);
let s={},jx=0,jy=0,held=false;
function bar(el,v){const w=Math.abs(v)/20;el.style.width=w+'%';el.style.left=(v<0?50-w:50)+'%';}
async function poll(){
 try{s=await (await fetch('/status')).json();}catch(e){$('rc').textContent='no connection';$('rc').className='bad';return;}
 $('rc').textContent=s.rc?'connected':(s.frames?'lost / failsafe':'not detected');
 $('rc').className=s.rc?'ok':'bad';
 $('src').textContent=s.enabled?s.source:'DISABLED';$('src').className=s.enabled?'':'warn';
 $('fr').textContent=s.frames+(s.frames?' ('+s.since+' ms ago)':'');
 $('up').textContent=Math.floor(s.uptime/1000)+' s';
 $('lv').textContent=s.left/10+'%';$('rv').textContent=s.right/10+'%';bar($('lb'),s.left);bar($('rb'),s.right);
 $('ch').textContent=s.rc?'CH1-6: '+s.ch.join(' '):'';
 $('en').textContent=s.enabled?'ENABLED (tap to disable)':'DISABLED (tap to enable)';$('en').className=s.enabled?'on':'off';
 const can=s.enabled&&!s.rc;
 $('pad').classList.toggle('dis',!can);
 $('mnote').textContent=!s.enabled?'(controller disabled)':(s.rc?'(RC in control)':'');
}
setInterval(poll,300);poll();
$('en').onclick=()=>fetch('/enable?on='+(s.enabled?0:1),{method:'POST'}).then(poll);
$('stop').onclick=()=>{held=false;jx=jy=0;knob();fetch('/stop',{method:'POST'});};
$('sp').oninput=()=>$('spv').textContent=$('sp').value;
const pad=$('pad');
function knob(){$('knob').style.left=(85+jx*85)+'px';$('knob').style.top=(85-jy*85)+'px';}
function move(e){const r=pad.getBoundingClientRect();let x=(e.clientX-r.left-120)/85,y=-(e.clientY-r.top-120)/85;
 const m=Math.hypot(x,y);if(m>1){x/=m;y/=m;}jx=x;jy=y;knob();}
pad.onpointerdown=e=>{held=true;pad.setPointerCapture(e.pointerId);move(e);};
pad.onpointermove=e=>{if(held)move(e);};
pad.onpointerup=pad.onpointercancel=()=>{held=false;jx=jy=0;knob();send();};
function send(){const k=$('sp').value*10;let l=jy+jx,r=jy-jx;const m=Math.max(Math.abs(l),Math.abs(r),1);
 if(m>1){l/=m;r/=m;}
 fetch('/drive?l='+Math.round(l*k)+'&r='+Math.round(r*k),{method:'POST'}).catch(()=>{});}
setInterval(()=>{if(held)send();},100);
</script></body></html>)HTML";

void handleStatus() {
  String j;
  j.reserve(256);
  j += "{\"enabled\":";
  j += st->enabled ? "true" : "false";
  j += ",\"rc\":";
  j += rx->connected(SBUS_TIMEOUT_MS) ? "true" : "false";
  j += ",\"failsafe\":";
  j += rx->failsafe() ? "true" : "false";
  j += ",\"frames\":";
  j += rx->frameCount();
  j += ",\"since\":";
  j += rx->frameCount() ? rx->msSinceFrame() : 0;
  j += ",\"source\":\"";
  j += sourceName(st->source);
  j += "\",\"left\":";
  j += mL->current();
  j += ",\"right\":";
  j += mR->current();
  j += ",\"uptime\":";
  j += millis();
  j += ",\"ch\":[";
  for (int c = 1; c <= 6; c++) {
    if (c > 1) j += ',';
    j += rx->us(c);
  }
  j += "]}";
  server.send(200, "application/json", j);
}

void handleEnable() {
  st->enabled = server.arg("on") != "0";
  st->webCmdValid = false;
  server.send(200, "text/plain", st->enabled ? "enabled" : "disabled");
}

void handleDrive() {
  if (!st->enabled || rx->connected(SBUS_TIMEOUT_MS)) {
    st->webCmdValid = false;
    server.send(409, "text/plain", "manual drive unavailable");
    return;
  }
  st->webLeft = constrain(server.arg("l").toInt(), -1000, 1000);
  st->webRight = constrain(server.arg("r").toInt(), -1000, 1000);
  st->webCmdMs = millis();
  st->webCmdValid = true;
  server.send(200, "text/plain", "ok");
}

void handleStop() {
  st->webLeft = st->webRight = 0;
  st->webCmdValid = false;
  mL->stopNow();
  mR->stopNow();
  server.send(200, "text/plain", "stopped");
}
}  // namespace

void webBegin(ControllerState &state, SbusReceiver &sbus, Motor &left, Motor &right) {
  st = &state;
  rx = &sbus;
  mL = &left;
  mR = &right;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
  Serial.printf("WiFi AP \"%s\" up, open http://%s/\n", WIFI_AP_SSID,
                WiFi.softAPIP().toString().c_str());

  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", PAGE); });
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/enable", HTTP_POST, handleEnable);
  server.on("/drive", HTTP_POST, handleDrive);
  server.on("/stop", HTTP_POST, handleStop);
  server.onNotFound([] { server.send(404, "text/plain", "not found"); });
  server.begin();
}

void webLoop() { server.handleClient(); }
