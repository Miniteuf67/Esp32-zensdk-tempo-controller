#include "web_pages.h"
#include "wifi_manager.h"

String dashboardHtml() {
  return R"rawliteral(
<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Zendure Tempo Controller</title>
<style>
:root{
  --bg:#08111d;--panel:#101b29;--panel2:#142336;--line:#26394f;
  --txt:#eef5fb;--muted:#90a4b9;--yellow:#f3bd4f;--white:#fff;
  --violet:#a768ff;--cyan:#68d4e8;--red:#ff5d67;--green:#55d887;--blue:#4da3ff;
}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--txt);font-family:Inter,Segoe UI,Arial,sans-serif}
a{color:inherit;text-decoration:none}
button{font:inherit}
.wrap{max-width:1450px;margin:auto;padding:18px}
.top{display:grid;grid-template-columns:1fr 1fr 1.25fr;gap:12px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:13px;padding:15px}
.k{font-size:11px;text-transform:uppercase;letter-spacing:.07em;color:var(--muted);font-weight:800}
.clock{font-size:31px;font-weight:900;margin-top:7px}
.date{color:var(--muted);margin-top:3px}
.tempo{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:10px}
.tbox{background:var(--panel2);padding:10px;border-radius:9px;text-align:center}
.tbox b{font-size:20px;display:block;margin-top:5px}
.weather{display:grid;grid-template-columns:auto 1fr auto;align-items:center;gap:12px}
.wicon{font-size:44px}.wtemp{font-size:30px;font-weight:900}
.section{margin-top:24px}.section h2{font-size:18px;margin:0 0 11px}
.gauges{display:grid;grid-template-columns:repeat(5,minmax(0,1fr));gap:12px}
.gauge{position:relative;height:178px;display:flex;align-items:center;justify-content:center;overflow:hidden}
.gauge svg{position:absolute;inset:10px 15px auto 15px;width:calc(100% - 30px);height:130px}
.gauge .track{fill:none;stroke:#26394f;stroke-width:18;stroke-linecap:round}
.gauge .fill{fill:none;stroke-width:18;stroke-linecap:round;transition:stroke-dasharray .35s ease}
.gval{position:relative;text-align:center;margin-top:32px}
.gval strong{font-size:28px;display:block}.gval span{color:var(--muted)}
.battery{display:grid;grid-template-columns:auto 1fr 1fr 1fr;gap:18px;align-items:center}
.bicon{font-size:76px;line-height:1}.big{font-size:30px;font-weight:900;margin-top:5px}
.state{font-size:25px;font-weight:900;color:var(--green);margin-top:5px}
.power{font-size:28px;font-weight:900;color:var(--violet);margin-top:5px}
.graph{height:410px;position:relative}
canvas{width:100%;height:100%}
.legend{display:flex;flex-wrap:wrap;gap:10px 18px;margin-bottom:8px;color:var(--muted);font-size:12px}
.legend i{width:12px;height:4px;display:inline-block;border-radius:2px;margin-right:5px;vertical-align:middle}
.tip{position:absolute;display:none;background:#07101aee;border:1px solid #41546a;border-radius:9px;padding:10px 12px;font-size:12px;line-height:1.55;pointer-events:none;z-index:3;min-width:190px}
.energy{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:10px}
.ecard{background:var(--panel);border:1px solid var(--line);border-radius:11px;padding:13px}
.ecard strong{font-size:27px;display:block;margin-top:7px}.ecolor{font-weight:800}
.sub{color:var(--muted);font-size:12px;margin-top:3px}
.twocol{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.sysgrid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:10px}
.tablewrap{overflow-x:auto}
table{width:100%;border-collapse:collapse;font-size:13px;min-width:760px}
th,td{padding:11px 10px;border-bottom:1px solid var(--line);text-align:left;white-space:nowrap}
th{color:var(--muted);text-transform:uppercase;font-size:11px}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}.on{background:var(--green)}.off{background:var(--red)}
.f1btns{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
.btn{border:1px solid var(--line);color:#fff;border-radius:9px;padding:10px 15px;font-weight:900;background:#244e75}
.btn.green{background:#1f7a49;border-color:#2f8f5a}.btn.red{background:#87343b;border-color:#a3474d}.btn:disabled{opacity:.4;cursor:not-allowed}
.settingsLink{display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap}
.settingsBtn{display:inline-flex;align-items:center;gap:8px;padding:11px 15px;border-radius:9px;border:1px solid var(--line);background:var(--panel2);font-weight:800}
@media(max-width:1100px){
  .top{grid-template-columns:1fr 1fr}.top .weathercard{grid-column:1/-1}
  .gauges{grid-template-columns:repeat(3,1fr)}.energy{grid-template-columns:1fr 1fr}.sysgrid{grid-template-columns:1fr 1fr}
}
@media(max-width:650px){
  .wrap{padding:10px}.top{grid-template-columns:1fr}.top .weathercard{grid-column:auto}
  .gauges,.energy,.sysgrid{grid-template-columns:1fr 1fr}.battery{grid-template-columns:auto 1fr}
  .graph{height:340px}.twocol{grid-template-columns:1fr}
}
@media(max-width:420px){.gauges,.energy,.sysgrid{grid-template-columns:1fr}}
</style>
</head>
<body>
<div class="wrap">

<div class="top">
  <div class="card">
    <div class="k">Date & heure</div>
    <div id="clock" class="clock">--:--</div>
    <div id="date" class="date">--</div>
    <div id="net" class="sub">Chargement…</div>
  </div>
  <div class="card">
    <div class="k">Tempo EDF</div>
    <div class="tempo">
      <div class="tbox"><span class="sub">Aujourd’hui</span><b id="tempoToday">--</b></div>
      <div class="tbox"><span class="sub">Demain</span><b id="tempoTomorrow">--</b></div>
    </div>
    <div id="tempoDetail" class="sub">--</div>
  </div>
  <div class="card weathercard">
    <div class="k">Météo</div>
    <div class="weather">
      <div id="weatherIcon" class="wicon">☁️</div>
      <div><div id="weatherTemp" class="wtemp">-- °C</div><div id="weatherDetail" class="sub">--</div></div>
      <div style="text-align:right"><div class="sub">Demain <span id="weatherTomorrowIcon">☁️</span></div><strong id="weatherTomorrow">--</strong></div>
    </div>
  </div>
</div>

<div class="section">
  <h2>Puissances — 24 h</h2>
  <div class="card">
    <div class="legend">
      <span><i style="background:#f3bd4f"></i>Production PV</span>
      <span><i style="background:#fff"></i>Conso maison</span>
      <span><i style="background:#a768ff"></i>Batterie (+ décharge / − charge)</span>
      <span><i style="background:#68d4e8"></i>Routage</span>
      <span><i style="background:#ff5d67"></i>EDF (Shelly)</span>
    </div>
    <div class="graph"><canvas id="chart"></canvas><div id="tip" class="tip"></div></div>
  </div>
</div>

<div class="section">
  <h2>Puissances instantanées</h2>
  <div id="gauges" class="gauges">
    <div class="card gauge" data-gauge="pv">
      <svg viewBox="0 0 200 110"><path class="track" d="M20 95 A80 80 0 0 1 180 95"/><path id="gPvArc" class="fill" pathLength="100" stroke="#f3bd4f" d="M20 95 A80 80 0 0 1 180 95"/></svg>
      <div class="gval"><strong id="gPv">--</strong><span>Production PV</span></div>
    </div>
    <div class="card gauge" data-gauge="house">
      <svg viewBox="0 0 200 110"><path class="track" d="M20 95 A80 80 0 0 1 180 95"/><path id="gHouseArc" class="fill" pathLength="100" stroke="#ffffff" d="M20 95 A80 80 0 0 1 180 95"/></svg>
      <div class="gval"><strong id="gHouse">--</strong><span>Conso maison</span></div>
    </div>
    <div id="gRoutedCard" class="card gauge" data-gauge="routed">
      <svg viewBox="0 0 200 110"><path class="track" d="M20 95 A80 80 0 0 1 180 95"/><path id="gRoutedArc" class="fill" pathLength="100" stroke="#68d4e8" d="M20 95 A80 80 0 0 1 180 95"/></svg>
      <div class="gval"><strong id="gRouted">--</strong><span>Routage</span></div>
    </div>
    <div class="card gauge" data-gauge="edf">
      <svg viewBox="0 0 200 110"><path class="track" d="M20 95 A80 80 0 0 1 180 95"/><path id="gEdfArc" class="fill" pathLength="100" stroke="#ff5d67" d="M20 95 A80 80 0 0 1 180 95"/></svg>
      <div class="gval"><strong id="gEdf">--</strong><span>EDF · Shelly</span></div>
    </div>
    <div class="card gauge" data-gauge="battery">
      <svg viewBox="0 0 200 110"><path class="track" d="M20 95 A80 80 0 0 1 180 95"/><path id="gBatteryArc" class="fill" pathLength="100" stroke="#a768ff" d="M20 95 A80 80 0 0 1 180 95"/></svg>
      <div class="gval"><strong id="gBattery">--</strong><span id="gBatteryLabel">Batterie</span></div>
    </div>
    <div id="gAuxCard" class="card gauge" style="display:none" data-gauge="aux">
      <svg viewBox="0 0 200 110"><path class="track" d="M20 95 A80 80 0 0 1 180 95"/><path id="gAuxArc" class="fill" pathLength="100" stroke="#55d887" d="M20 95 A80 80 0 0 1 180 95"/></svg>
      <div class="gval"><strong id="gAux">--</strong><span id="gAuxLabel">Canal personnalisé</span></div>
    </div>
  </div>
</div>

<div class="section">
  <h2>Batterie</h2>
  <div class="card battery">
    <div class="bicon">🔋</div>
    <div><div class="k">SOC moyen</div><div id="batterySoc" class="big">-- %</div><div id="batteryOnline" class="sub">--</div></div>
    <div><div class="k">État</div><div id="batteryState" class="state">--</div><div id="batteryStateSub" class="sub">--</div></div>
    <div><div class="k">Puissance</div><div id="batteryPower" class="power">-- W</div><div class="sub">Valeur absolue</div></div>
  </div>
</div>

<div class="section">
  <h2>Énergie</h2>
  <div class="energy">
    <div class="ecard"><div class="ecolor" style="color:#f3bd4f">☀ PV jour</div><strong id="ePvDay">--</strong><div class="sub">Production depuis minuit</div></div>
    <div class="ecard"><div class="ecolor" style="color:#f3bd4f">☀ PV total</div><strong id="ePvTotal">--</strong><div class="sub">Compteur logiciel ESP</div></div>
    <div class="ecard"><div class="ecolor" style="color:#ff5d67">⚡ Import EDF jour</div><strong id="eImpDay">--</strong><div class="sub">Canal réseau Shelly</div></div>
    <div class="ecard"><div class="ecolor" style="color:#ff5d67">⚡ Import EDF total</div><strong id="eImpTotal">--</strong><div class="sub">Compteur Shelly</div></div>
    <div class="ecard"><div class="ecolor" style="color:#55d887">↗ Export EDF jour</div><strong id="eExpDay">--</strong><div class="sub">Injection réseau</div></div>
    <div class="ecard"><div class="ecolor" style="color:#55d887">↗ Export EDF total</div><strong id="eExpTotal">--</strong><div class="sub">Compteur Shelly</div></div>
    <div id="eRoutedDayCard" class="ecard"><div class="ecolor" style="color:#68d4e8">↪ Routage jour</div><strong id="eRoutedDay">--</strong><div class="sub">Énergie routée</div></div>
    <div id="eRoutedTotalCard" class="ecard"><div class="ecolor" style="color:#68d4e8">↪ Routage total</div><strong id="eRoutedTotal">--</strong><div class="sub">Compteur Shelly</div></div>
    <div id="eAuxDayCard" class="ecard" style="display:none"><div id="eAuxDayTitle" class="ecolor" style="color:#55d887">Canal personnalisé · jour</div><strong id="eAuxDay">--</strong><div class="sub">Énergie du canal</div></div>
    <div id="eAuxTotalCard" class="ecard" style="display:none"><div id="eAuxTotalTitle" class="ecolor" style="color:#55d887">Canal personnalisé · total</div><strong id="eAuxTotal">--</strong><div class="sub">Compteur Shelly</div></div>
  </div>
</div>

<div id="f1Section" class="section">
  <h2>F1ATB</h2>
  <div class="card">
    <div class="twocol">
      <div class="ecard">
        <div class="k">Liaison</div>
        <strong id="f1Link" style="font-size:24px">--</strong>
        <div id="f1LastSync" class="sub">Dernière synchro : --</div>
      </div>
      <div class="ecard">
        <div class="k">Action détectée</div>
        <strong id="f1Name" style="font-size:24px">--</strong>
        <div id="f1ActionNo" class="sub">Action #--</div>
      </div>
      <div class="ecard"><div class="k">État</div><strong id="f1State" style="font-size:24px">--</strong></div>
      <div class="ecard">
        <div class="k">Forçage F1ATB</div>
        <div class="f1btns" style="margin-top:8px">
          <button id="f1On" type="button" class="btn green">Allumer 30 min</button>
          <button id="f1Off" type="button" class="btn red">Éteindre 30 min</button>
          <button id="f1Cancel" type="button" class="btn">Annuler</button>
        </div>
        <div id="f1ForceInfo" class="sub" style="margin-top:8px">Prêt</div>
      </div>
    </div>
  </div>
</div>

<div class="section">
  <h2>Appareils SolarFlow</h2>
  <div class="card tablewrap">
    <table>
      <thead><tr><th>Nom</th><th>Statut</th><th>SOC</th><th>PV</th><th>Maison</th><th>EDF Zendure</th><th>Mode</th></tr></thead>
      <tbody id="zbody"></tbody>
    </table>
  </div>
</div>

<div class="section">
  <div class="card sysgrid">
    <div><div class="k">Système</div><strong id="sysOnline" style="color:var(--green);font-size:19px">En ligne</strong></div>
    <div><div class="k">Mode</div><strong id="sysMode">--</strong></div>
    <div><div class="k">Uptime</div><strong id="sysUptime">--</strong></div>
    <div><div class="k">Zendure</div><strong id="sysZendure">--</strong></div>
  </div>
</div>

<div class="section">
  <div class="card settingsLink">
    <div>
      <div class="k">Configuration</div>
      <div style="font-size:18px;font-weight:800;margin-top:4px">Paramètres du contrôleur</div>
      <div class="sub">Réseau, IP fixe, Shelly, F1ATB, météo, Tempo et pilotage Zendure</div>
    </div>
    <a class="settingsBtn" href="/admin">⚙ Paramètres</a>
  </div>
</div>

</div>

<script>
const q=s=>document.querySelector(s);
let points=[];

function fmtPower(v){
  const n=Math.abs(Number(v)||0);
  return n>=1000?(n/1000).toFixed(n>=10000?1:2)+' kW':Math.round(n)+' W';
}
function fmtEnergy(wh){
  const n=Math.max(0,Number(wh)||0);
  if(n>=1000000)return (n/1000000).toFixed(2)+' MWh';
  if(n>=1000)return (n/1000).toFixed(2)+' kWh';
  return Math.round(n)+' Wh';
}
function fmtUptime(v){
  let s=Math.max(0,Number(v)||0);const d=Math.floor(s/86400);s%=86400;
  const h=Math.floor(s/3600);s%=3600;const m=Math.floor(s/60);
  return (d?d+' j ':'')+h+' h '+m+' min';
}
function wicon(code){
  code=Number(code);
  if(code===0)return '☀️';
  if(code===1||code===2)return '🌤️';
  if(code===3)return '☁️';
  if(code===45||code===48)return '🌫️';
  if([51,53,55,56,57].includes(code))return '🌦️';
  if([61,63,65,66,67,80,81,82].includes(code))return '🌧️';
  if([71,73,75,77,85,86].includes(code))return '🌨️';
  if([95,96,99].includes(code))return '⛈️';
  return '☁️';
}
function tempoColor(el,val){
  el.style.color=val==='Rouge'?'#ff5d67':val==='Blanc'?'#eef5fb':'#4da3ff';
}
function clock(){
  const d=new Date();
  q('#clock').textContent=d.toLocaleTimeString('fr-FR',{hour:'2-digit',minute:'2-digit',second:'2-digit'});
  q('#date').textContent=d.toLocaleDateString('fr-FR',{weekday:'long',day:'numeric',month:'long',year:'numeric'});
}
setInterval(clock,1000);clock();

function gauge(value,max,arc,valueEl,labelEl,label){
  const v=Math.abs(Number(value)||0);
  const pct=Math.max(0,Math.min(100,v/Math.max(1,max)*100));
  arc.style.strokeDasharray=pct+' '+(100-pct);
  valueEl.textContent=fmtPower(v);
  if(labelEl&&label!==undefined)labelEl.textContent=label;
}

const c=q('#chart'),tip=q('#tip');
function draw(active=null){
  const r=c.getBoundingClientRect(),dpr=devicePixelRatio||1;
  c.width=Math.max(1,r.width*dpr);c.height=Math.max(1,r.height*dpr);
  const x=c.getContext('2d');x.setTransform(dpr,0,0,dpr,0,0);
  const W=r.width,H=r.height,p={l:60,r:12,t:10,b:30};
  x.fillStyle='#0b1623';x.fillRect(0,0,W,H);
  if(!points.length){x.fillStyle='#90a4b9';x.font='13px sans-serif';x.fillText('Historique en attente…',70,60);return}

  let max=500,min=0;
  for(const pt of points){
    max=Math.max(max,Number(pt.pv)||0,Number(pt.house)||0,Number(pt.routed)||0,Number(pt.gridstore)||0,Number(pt.battery)||0);
    min=Math.min(min,Number(pt.battery)||0);
  }
  max=Math.ceil(max/500)*500;
  if(min<0)min=Math.floor(min/500)*500; else min=-500;
  const range=max-min;
  const y=v=>p.t+(H-p.t-p.b)*(max-v)/range;
  const xx=i=>p.l+(W-p.l-p.r)*(points.length<=1?0:i/(points.length-1));

  x.font='11px sans-serif';
  const steps=Math.max(4,Math.min(8,Math.round(range/500)));
  for(let i=0;i<=steps;i++){
    const v=max-range*i/steps,yy=y(v);
    x.strokeStyle=Math.abs(v)<range/(steps*2)?'#52677d':'#24364a';
    x.beginPath();x.moveTo(p.l,yy);x.lineTo(W-p.r,yy);x.stroke();
    x.fillStyle='#90a4b9';x.fillText(Math.round(v)+' W',5,yy+4);
  }

  const t0=points[0].t,t1=points[points.length-1].t||t0+1;
  for(let i=0;i<=6;i++){
    const X=p.l+(W-p.l-p.r)*i/6;
    const ts=t0+(t1-t0)*i/6;
    x.fillStyle='#90a4b9';
    x.fillText(new Date(ts*1000).toLocaleTimeString('fr-FR',{hour:'2-digit',minute:'2-digit'}),Math.max(p.l,X-18),H-8);
  }

  const zero=y(0),bw=Math.max(1,(W-p.l-p.r)/Math.max(1,points.length)*.82);
  x.fillStyle='#f3bd4f99';
  points.forEach((pt,i)=>{const yy=y(Number(pt.pv)||0);x.fillRect(xx(i)-bw/2,yy,bw,Math.max(1,zero-yy))});

  const line=(k,col,w=1.8)=>{
    x.strokeStyle=col;x.lineWidth=w;x.beginPath();
    points.forEach((pt,i)=>{const X=xx(i),Y=y(Number(pt[k])||0);i?x.lineTo(X,Y):x.moveTo(X,Y)});
    x.stroke();
  };
  line('house','#fff',2);line('battery','#a768ff',2);line('routed','#68d4e8');line('gridstore','#ff5d67');

  if(active!==null&&points[active]){
    const X=xx(active),pt=points[active];
    x.strokeStyle='#cbd6e0aa';x.lineWidth=1;x.beginPath();x.moveTo(X,p.t);x.lineTo(X,H-p.b);x.stroke();
    [['pv','#f3bd4f'],['house','#fff'],['battery','#a768ff'],['routed','#68d4e8'],['gridstore','#ff5d67']].forEach(([k,col])=>{
      x.fillStyle=col;x.beginPath();x.arc(X,y(Number(pt[k])||0),3.5,0,Math.PI*2);x.fill();
    });
  }
}
function chartIndex(ev){
  if(!points.length)return null;
  const r=c.getBoundingClientRect();
  const clientX=ev.touches?.[0]?.clientX??ev.clientX;
  const px=Math.max(60,Math.min(r.width-12,clientX-r.left));
  return Math.max(0,Math.min(points.length-1,Math.round((px-60)/Math.max(1,r.width-72)*(points.length-1))));
}
function showTip(ev){
  const i=chartIndex(ev);if(i===null)return;
  const pt=points[i],bat=Number(pt.battery)||0;draw(i);
  tip.style.display='block';
  tip.innerHTML=`<b>${new Date(pt.t*1000).toLocaleString('fr-FR',{day:'2-digit',month:'2-digit',hour:'2-digit',minute:'2-digit'})}</b><br>
    <span style="color:#f3bd4f">Production PV : ${Math.round(Number(pt.pv)||0)} W</span><br>
    <span>Conso maison : ${Math.round(Number(pt.house)||0)} W</span><br>
    <span style="color:#a768ff">Batterie : ${bat>=0?'+':''}${Math.round(bat)} W</span><br>
    <span style="color:#68d4e8">Routage : ${Math.round(Number(pt.routed)||0)} W</span><br>
    <span style="color:#ff5d67">EDF : ${Math.round(Number(pt.gridstore)||0)} W</span>`;
  const rr=q('.graph').getBoundingClientRect(),cx=(ev.touches?.[0]?.clientX??ev.clientX)-rr.left,cy=(ev.touches?.[0]?.clientY??ev.clientY)-rr.top;
  tip.style.left=Math.min(rr.width-205,Math.max(8,cx+12))+'px';
  tip.style.top=Math.max(8,Math.min(rr.height-135,cy-55))+'px';
}
c.addEventListener('mousemove',showTip);
c.addEventListener('touchstart',showTip,{passive:true});
c.addEventListener('touchmove',showTip,{passive:true});
c.addEventListener('mouseleave',()=>{tip.style.display='none';draw()});
c.addEventListener('touchend',()=>setTimeout(()=>{tip.style.display='none';draw()},900));
addEventListener('resize',()=>draw());

async function loadHistory(){
  try{
    const h=await (await fetch('/api/history',{cache:'no-store'})).json();
    points=h.points||[];
    draw();
  }catch(e){}
}

async function loadStatus(){
  try{
    const s=await (await fetch('/api/status',{cache:'no-store'})).json();
    const f=s.flows||{},sh=s.shelly||{},en=s.energy||{};
    const zs=(s.zendure||[]).filter(z=>z.configured!==false&&z.enabled!==false);
    const online=zs.filter(z=>z.online);
    const socs=online.map(z=>Number(z.soc)).filter(v=>v>=0&&v<=100);
    const soc=socs.length?Math.round(socs.reduce((a,b)=>a+b,0)/socs.length):null;

    q('#net').textContent=`${s.wifi?.ssid||'Wi‑Fi'} · ${s.wifi?.rssi??'--'} dBm · ${s.wifi?.ip||'--'}`;
    q('#tempoToday').textContent=s.tempo?.today||'--';tempoColor(q('#tempoToday'),s.tempo?.today);
    q('#tempoTomorrow').textContent=s.tempo?.tomorrow_defined===false?'Non défini':(s.tempo?.tomorrow||'--');tempoColor(q('#tempoTomorrow'),s.tempo?.tomorrow);
    q('#tempoDetail').textContent=`${s.tempo?.now_tariff||'--'} · ${s.tempo?.window||'--'}`;

    if(s.weather?.enabled&&s.weather?.online){
      q('#weatherIcon').textContent=wicon(s.weather.code);
      q('#weatherTemp').textContent=Number(s.weather.temp_c).toFixed(1)+' °C';
      q('#weatherDetail').textContent=`${s.weather.city||''} · ressenti ${Number(s.weather.feels_c).toFixed(1)} °C · ${Math.round(Number(s.weather.humidity)||0)} % · vent ${Math.round(Number(s.weather.wind_kmh)||0)} km/h`;
      q('#weatherTomorrowIcon').textContent=wicon(s.weather.tomorrow_code);
      q('#weatherTomorrow').textContent=`${Math.round(Number(s.weather.tomorrow_min_c)||0)} / ${Math.round(Number(s.weather.tomorrow_max_c)||0)} °C`;
    }else{
      q('#weatherIcon').textContent='☁️';q('#weatherTemp').textContent='-- °C';
      q('#weatherDetail').textContent=s.weather?.enabled?'Indisponible':'Météo désactivée';q('#weatherTomorrow').textContent='--';
    }

    const batteryW=Number(f.battery_w)||0;
    const batteryAbs=Math.abs(batteryW);
    const batteryState=batteryW>20?'En décharge':batteryW<-20?'En charge':'Au repos';
    const batterySub=batteryW>20?'Les batteries alimentent actuellement la maison':batteryW<-20?'Les batteries absorbent actuellement de l’énergie':'Pas de flux batterie significatif';

    gauge(f.pv_w,3000,q('#gPvArc'),q('#gPv'));
    gauge(f.house_w,4000,q('#gHouseArc'),q('#gHouse'));
    gauge(f.routed_w,3500,q('#gRoutedArc'),q('#gRouted'));
    gauge(f.from_grid_w,4000,q('#gEdfArc'),q('#gEdf'));
    gauge(batteryAbs,2400,q('#gBatteryArc'),q('#gBattery'),q('#gBatteryLabel'),'Batterie · '+batteryState);

    q('#gRoutedCard').style.display=sh.routed_enabled?'':'none';
    const aux=!!sh.aux_enabled;
    q('#gAuxCard').style.display=aux?'':'none';
    if(aux){
      gauge(sh.aux_power_w,4000,q('#gAuxArc'),q('#gAux'),q('#gAuxLabel'),sh.aux_label||'Canal personnalisé');
    }

    q('#batterySoc').textContent=soc===null?'-- %':soc+' %';
    q('#batteryOnline').textContent=`${online.length} / ${zs.length} SolarFlow en ligne`;
    q('#batteryState').textContent=batteryState;
    q('#batteryState').style.color=batteryState==='En charge'?'#4da3ff':batteryState==='En décharge'?'#55d887':'#90a4b9';
    q('#batteryStateSub').textContent=batterySub;
    q('#batteryPower').textContent=fmtPower(batteryAbs);

    q('#ePvDay').textContent=fmtEnergy(en.pv_today_wh);
    q('#ePvTotal').textContent=fmtEnergy(en.pv_total_wh);
    q('#eImpDay').textContent=fmtEnergy(sh.grid_import_today_wh);
    q('#eImpTotal').textContent=fmtEnergy(sh.grid_import_total_wh);
    q('#eExpDay').textContent=fmtEnergy(sh.grid_export_today_wh);
    q('#eExpTotal').textContent=fmtEnergy(sh.grid_export_total_wh);

    q('#eRoutedDayCard').style.display=sh.routed_enabled?'':'none';
    q('#eRoutedTotalCard').style.display=sh.routed_enabled?'':'none';
    q('#eRoutedDay').textContent=fmtEnergy(sh.routed_today_wh);
    q('#eRoutedTotal').textContent=fmtEnergy(sh.routed_total_wh);

    q('#eAuxDayCard').style.display=aux?'':'none';
    q('#eAuxTotalCard').style.display=aux?'':'none';
    if(aux){
      const lbl=sh.aux_label||'Canal personnalisé';
      q('#eAuxDayTitle').textContent=lbl+' · jour';
      q('#eAuxTotalTitle').textContent=lbl+' · total';
      q('#eAuxDay').textContent=fmtEnergy(sh.aux_today_wh);
      q('#eAuxTotal').textContent=fmtEnergy(sh.aux_total_wh);
    }

    q('#f1Section').style.display=s.f1atb?.enabled?'':'none';
    const f1=s.f1atb||{};
    q('#f1Link').textContent=f1.synced?'● Synchronisé':(f1.online?'● En ligne · action non synchronisée':'● Hors ligne');
    q('#f1Link').style.color=f1.synced?'var(--green)':(f1.online?'var(--yellow)':'var(--red)');
    q('#f1LastSync').textContent=f1.online?`Dernière synchro : il y a ${f1.last_ok_age_s??0} s`:'Dernière synchro : --';
    q('#f1Name').textContent=f1.detected_name||f1.action_label||'--';
    q('#f1ActionNo').textContent=`Action #${f1.action_number??'--'}`;
    q('#f1State').textContent=f1.action_state||'--';
    q('#f1On').disabled=!f1.online;
    q('#f1Off').disabled=!f1.online;
    q('#f1Cancel').disabled=!f1.online;
    const fm=Number(f1.force_minutes||0);
    let forceTxt='Aucun forçage en cours';
    if(fm>0) forceTxt=`Forçage ON · ${fm} min`;
    else if(fm<0) forceTxt=`Forçage OFF · ${Math.abs(fm)} min`;
    q('#f1ForceInfo').textContent=!f1.online?'F1ATB hors ligne':(!f1.synced?'F1ATB joignable · état action en attente':forceTxt);

    q('#zbody').innerHTML=zs.length?zs.map(z=>`<tr>
      <td><b>${z.label||z.name||'SolarFlow'}</b></td>
      <td><span class="dot ${z.online?'on':'off'}"></span>${z.online?'En ligne':'Hors ligne'}</td>
      <td>${z.soc>=0?z.soc+' %':'--'}</td>
      <td>${z.solar_w>=0?z.solar_w+' W':'--'}</td>
      <td>${z.home_w>=0?z.home_w+' W':'--'}</td>
      <td>${z.grid_w>=0?z.grid_w+' W':'--'}</td>
      <td>${s.control?.effective||'--'}</td>
    </tr>`).join(''):'<tr><td colspan="7" class="sub">Aucun SolarFlow configuré</td></tr>';

    q('#sysMode').textContent=s.control?.requested||'--';
    q('#sysUptime').textContent=fmtUptime(s.system?.uptime_s);
    q('#sysZendure').textContent=`${online.length} / ${zs.length} en ligne`;

    try{
      const a=await (await fetch('/api/auth/status',{cache:'no-store'})).json();
      document.body.classList.toggle('is-admin',!!a.admin);
    }catch(e){}
  }catch(e){
    q('#sysOnline').textContent='Déconnecté';q('#sysOnline').style.color='#ff5d67';
  }
}

async function f1Force(path,label){
  const info=q('#f1ForceInfo');
  info.textContent=label+'…';
  try{
    const r=await fetch(path,{method:'POST'});
    if(!r.ok){
      info.textContent='Commande F1ATB échouée';
      return;
    }
    info.textContent=label+' envoyé';
    setTimeout(loadStatus,300);
  }catch(e){info.textContent='F1ATB injoignable';}
}
q('#f1On').addEventListener('click',()=>f1Force('/api/f1atb/on','Allumer 30 min'));
q('#f1Off').addEventListener('click',()=>f1Force('/api/f1atb/off','Éteindre 30 min'));
q('#f1Cancel').addEventListener('click',()=>f1Force('/api/f1atb/cancel','Annuler le forçage'));

loadStatus();loadHistory();
setInterval(loadStatus,5000);
setInterval(loadHistory,120000);
</script>
</body>
</html>
)rawliteral";
}

String adminHtml(bool authenticated) {
  if (!authenticated) {
    return R"rawliteral(
<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Connexion</title>
<style>
body{font-family:Arial;background:#f3f4f6;margin:0}
.box{max-width:380px;margin:50px auto;background:#fff;padding:20px;border:1px solid #ddd;border-radius:8px}
input,button{width:100%;padding:12px;margin:7px 0;font-size:16px}
button{background:#263746;color:#fff;border:0;border-radius:5px}
.e{color:#b22}
</style>
</head>
<body>
<div class="box">
  <h2>Administration</h2>
  <p>La vue publique reste accessible sans mot de passe.</p>
  <form id="f">
    <input id="p" type="password" placeholder="Mot de passe" autocomplete="current-password">
    <button>Connexion</button>
  </form>
  <div id="e" class="e"></div>
  <p><a href="/">← Retour</a></p>
</div>

<script>
document.querySelector('#f').addEventListener('submit',async e=>{
  e.preventDefault();
  const r=await fetch('/api/login',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({password:document.querySelector('#p').value})
  });
  if(r.ok) location.reload();
  else document.querySelector('#e').textContent='Mot de passe incorrect';
});
</script>
</body>
</html>
)rawliteral";
  }

  return R"rawliteral(
<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Admin Zendure Tempo</title>
<style>
body{font-family:Arial;background:#f3f4f6;margin:0;color:#17202a}
header{background:#262f38;color:white;padding:12px 16px}
.wrap{max-width:850px;margin:auto;padding:12px}
.card{background:#fff;border:1px solid #d7dce2;border-radius:8px;padding:14px;margin:10px 0}
button,input,select{font-size:16px;padding:10px;margin:5px;border:1px solid #bbc2c9;border-radius:5px}
button{cursor:pointer}.danger{background:#b33;color:#fff}.primary{background:#263746;color:#fff}
.row{display:flex;flex-wrap:wrap;gap:8px;align-items:center}
.muted{color:#667085;font-size:13px}.warn{color:#9a6200;font-weight:bold}
.summary{display:grid;gap:8px}.metric{border:1px solid #e1e5ea;border-radius:6px;padding:8px}.metric .sub,.sub{display:block;color:#667085;font-size:12px}
</style>
</head>
<body>
<header><b>Administration Zendure Tempo</b> — <a href="/" style="color:white">Vue publique</a></header>

<div class="wrap">
  <div class="card">
    <h3>Commande</h3>
    <div id="safety" class="warn"></div>

    <div class="row">
      <button class="primary" data-mode="auto">AUTO</button>
      <button data-mode="self">Autoconsommation</button>
      <button data-mode="charge">Charge forcée</button>
    </div>

    <div class="row">
      <label>
        Puissance charge forcée
        <select id="power"></select>
      </label>

      <label>
        Forçage manuel
        <input id="mins" type="number" min="1" max="720" value="60"> min
      </label>
    </div>

    <div class="muted">
      AUTO : charge uniquement pendant les HC qui préparent un jour rouge.
      Le reste du temps, l'ESP relâche la commande pour laisser le HEMS reprendre la main.
    </div>
  </div>

  <div class="card">
    <h3>Paramètres</h3>

    <div class="row">
      <label>Hostname <input id="host"></label>
      <label><input id="wifiStaticEnabled" type="checkbox"> Utiliser une IP fixe</label>
      <label>IP fixe <input id="wifiStaticIp" placeholder="192.168.1.50"></label>
      <label>Passerelle <input id="wifiGateway" placeholder="192.168.1.254"></label>
      <label>Masque <input id="wifiSubnet" placeholder="255.255.255.0"></label>
      <label>DNS 1 <input id="wifiDns1" placeholder="192.168.1.254"></label>
      <label>DNS 2 <input id="wifiDns2" placeholder="1.1.1.1 (optionnel)"></label>
      <label>Shelly <input id="shelly"></label>
      <label>Profil Shelly
        <select id="shellyProfile">
          <option value="mono">Monophasé (EM1)</option>
          <option value="tri">Triphasé (EM)</option>
        </select>
      </label>
      <label>Canal réseau
        <select id="shellyGridChannel">
          <option value="0">0</option><option value="1">1</option><option value="2">2</option>
        </select>
      </label>
      <label>Canal routage
        <select id="shellyRoutedChannel">
          <option value="-1">Désactivé</option><option value="0">0</option><option value="1">1</option><option value="2">2</option>
        </select>
      </label>
      <label>Canal personnalisé
        <select id="shellyAuxChannel">
          <option value="-1">Désactivé</option><option value="0">0</option><option value="1">1</option><option value="2">2</option>
        </select>
      </label>
      <label>Nom du canal personnalisé
        <input id="shellyAuxLabel" placeholder="Ex. Climatisation">
      </label>
      <label>F1ATB <input id="f1"></label>
         <label><input id="f1en" type="checkbox"> Présence F1ATB</label>
    </div>

    <div class="muted">
      IP fixe : les nouveaux paramètres réseau prennent effet au prochain redémarrage de l’ESP32.
      Si l’option est désactivée, le contrôleur utilise DHCP.
    </div>

    <div class="muted">
      Shelly Pro 3EM : en profil monophasé, chaque pince est un compteur EM1 indépendant.
      Le canal réseau alimente la courbe et les compteurs import/export ; le canal routage et le canal personnalisé sont optionnels.
      En monophasé, les canaux optionnels fournissent puissance, énergie du jour et énergie totale.
      En profil triphasé, le réseau utilise EM.GetStatus/EMData.GetStatus et les canaux EM1 optionnels sont désactivés.
    </div>

    <div class="row">
      <label>Nom de l'action <input id="f1ActionLabel" placeholder="Chauffe-eau"></label>
      <label>Numéro d'action <input id="f1ActionNumber" type="number" min="0" max="15" step="1"></label>
    </div>

    <div class="muted">
      F1ATB identifie les commandes par <b>NumAction</b>, pas par le nom.
      Le triac est toujours l'action <b>0</b>; le premier relais/SSR est 1, puis 2, etc.
      Le nom sert uniquement à rendre l'interface lisible.
    </div>

    <div class="row">
      <label>
        <input id="zwrite" type="checkbox">
        Autoriser réellement les écritures Zendure
      </label>
    </div>

    <div class="muted">
      Sécurité : désactivé au premier démarrage.
      Active-le seulement après vérification de la découverte des 3 appareils.
    </div>

    <div class="row">
      <button id="save" class="primary">Enregistrer</button>
      <button id="saveRestart" type="button">Enregistrer et redémarrer</button>
    </div>
  </div>

  <div class="card">
    <h3>SolarFlow Zendure</h3>
    <div class="muted">
      Ajoute uniquement les appareils que ce contrôleur doit gérer. Le numéro de série sert
      d'identité stable ; l'IP est optionnelle et peut rester vide pour utiliser la découverte mDNS.
    </div>

    <div id="zendureRows"></div>

    <div class="row">
      <button id="addZendure" type="button">+ Ajouter un périphérique</button>
      <button id="removeZendure" type="button">− Retirer le dernier</button>
    </div>
  </div>

  <div class="card">
    <h3>Météo extérieure</h3>

    <div class="row">
      <label><input id="weatherEnabled" type="checkbox"> Activer la météo</label>
    </div>

    <div class="row">
      <input id="weatherQuery" placeholder="Ville, ex. Phalsbourg">
      <button id="weatherSearch" type="button">Rechercher</button>
    </div>

    <div id="weatherResults"></div>

    <div class="muted" id="weatherSelected">
      Aucune ville sélectionnée.
    </div>

    <input id="weatherCity" type="hidden">
    <input id="weatherDisplayName" type="hidden">
    <input id="weatherLat" type="hidden">
    <input id="weatherLon" type="hidden">
  </div>

  <div class="card">
    <h3>État mémoire / flash</h3>
    <div class="summary" style="grid-template-columns:repeat(2,minmax(0,1fr))">
      <div class="metric"><span class="sub">Heap libre</span><b id="sysFreeHeap">--</b></div>
      <div class="metric"><span class="sub">Heap min.</span><b id="sysMinHeap">--</b></div>
      <div class="metric"><span class="sub">Plus gros bloc</span><b id="sysMaxAlloc">--</b></div>
      <div class="metric"><span class="sub">Heap total</span><b id="sysHeapSize">--</b></div>
      <div class="metric"><span class="sub">Firmware</span><b id="sysSketch">--</b></div>
      <div class="metric"><span class="sub">Espace OTA libre</span><b id="sysSketchFree">--</b></div>
      <div class="metric"><span class="sub">Flash totale</span><b id="sysFlash">--</b></div>
      <div class="metric"><span class="sub">Uptime</span><b id="sysUptime">--</b></div>
    </div>
    <div class="muted" style="margin-top:8px">
      Le minimum de heap permet de repérer les chutes de mémoire pendant les requêtes Web/OTA.
    </div>
  </div>

  <div class="card">
    <h3>Mise à jour OTA</h3>
    <div class="muted">
      Sélectionne le fichier firmware <b>.bin</b> compilé pour ce contrôleur.
      L'ESP redémarrera automatiquement si la mise à jour réussit.
    </div>
    <div class="row" style="margin-top:10px">
      <input id="otaFile" type="file" accept=".bin,application/octet-stream">
      <button id="otaUpload" type="button">Installer le firmware</button>
    </div>
    <div id="otaProgressWrap" style="display:none;margin-top:10px">
      <progress id="otaProgress" value="0" max="100" style="width:100%"></progress>
      <div id="otaMsg" class="muted">0 %</div>
    </div>
  </div>

  <div class="card">
    <h3>Système ESP32</h3>
    <div class="row">
      <button id="espReboot" type="button">Redémarrer l'ESP</button>
      <button id="factoryReset" type="button" style="background:#8a3038;color:#fff">Factory reset</button>
    </div>
    <div class="muted">
      Factory reset efface le Wi-Fi, le mot de passe Admin et toute la configuration locale,
      puis redémarre en mode configuration initiale.
    </div>
    <div id="systemMsg" class="muted"></div>
  </div>

  <div class="card">
    <h3>Wi-Fi</h3>
    <div id="current"></div>
    <button id="scan">Scanner</button>
    <div id="nets"></div>

    <div class="row">
      <input id="ssid" placeholder="SSID">
      <input id="wpass" type="password" placeholder="Nouveau mot de passe Wi-Fi">
      <button id="wifiSave">Changer Wi-Fi</button>
    </div>
  </div>

  <div class="card">
    <h3>Système</h3>
    <button id="discover">Redécouvrir Zendure</button>
    <button id="logout">Déconnexion</button>
    <pre id="msg"></pre>
  </div>
</div>

<script>
const q=s=>document.querySelector(s);
const fmtBytes=v=>{
  const n=Number(v)||0;
  if(n>=1024*1024) return (n/(1024*1024)).toFixed(2)+' Mo';
  if(n>=1024) return (n/1024).toFixed(1)+' Ko';
  return n+' o';
};
const fmtUptime=v=>{
  let s=Math.max(0,Number(v)||0);
  const d=Math.floor(s/86400); s%=86400;
  const h=Math.floor(s/3600); s%=3600;
  const m=Math.floor(s/60);
  return (d?d+'j ':'')+h+'h '+m+'min';
};
const msg=t=>q('#msg').textContent=t;

let zendureDevices=[];

function esc(v){
  return String(v??'').replace(/[&<>"']/g,c=>({
    '&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'
  }[c]));
}

function renderZendures(){
  const box=q('#zendureRows');

  if(!zendureDevices.length){
    box.innerHTML='<div class="muted">Aucun SolarFlow configuré.</div>';
    return;
  }

  box.innerHTML=zendureDevices.map((z,i)=>`
    <div class="card" style="background:#f8f9fa">
      <div class="row">
        <b>Zendure ${i+1}</b>
        <label><input class="zen-enabled" data-i="${i}" type="checkbox" ${z.enabled!==false?'checked':''}> Actif</label>
      </div>
      <div class="row">
        <label>Nom <input class="zen-label" data-i="${i}" value="${esc(z.label||('SolarFlow '+(i+1)))}"></label>
        <label>SN <input class="zen-sn" data-i="${i}" value="${esc(z.sn||'')}" placeholder="Numéro de série"></label>
        <label>IP (optionnelle) <input class="zen-ip" data-i="${i}" value="${esc(z.manual_ip||'')}" placeholder="mDNS si vide"></label>
      </div>
      <div class="muted">
        Détecté : ${esc(z.ip||'non')} ${z.online?'• en ligne':''}
      </div>
    </div>
  `).join('');
}

function collectZendures(){
  return zendureDevices.map((z,i)=>({
    enabled:q(`.zen-enabled[data-i="${i}"]`)?.checked ?? true,
    label:q(`.zen-label[data-i="${i}"]`)?.value.trim() ?? '',
    sn:q(`.zen-sn[data-i="${i}"]`)?.value.trim() ?? '',
    manual_ip:q(`.zen-ip[data-i="${i}"]`)?.value.trim() ?? ''
  }));
}

async function status(){
  const [statusResponse,configResponse]=await Promise.all([
    fetch('/api/status',{cache:'no-store'}),
    fetch('/api/config',{cache:'no-store'})
  ]);

  if(!statusResponse.ok) throw new Error('status_http_'+statusResponse.status);
  if(!configResponse.ok) throw new Error('config_http_'+configResponse.status);

  const s=await statusResponse.json();
  const saved=await configResponse.json();
  s.config=saved.config||{};
  s.zendure=saved.zendure||s.zendure||[];

  const activeDevices=(s.zendure||[]).filter(z=>z.configured!==false && z.enabled!==false);
  const chargeMax=Math.min(2400,Math.max(100,activeDevices.length*800));

  q('#power').innerHTML=[];
  q('#power').innerHTML=Array.from({length:Math.floor(chargeMax/100)},(_,i)=>(i+1)*100)
    .map(w=>`<option value="${w}">${w} W</option>`).join('');
  q('#power').value=String(Math.min(Number(s.config.charge_w||800),chargeMax));
  q('#host').value=s.config.hostname;
  q('#wifiStaticEnabled').checked=!!s.config.wifi_static_enabled;
  q('#wifiStaticIp').value=s.config.wifi_static_ip||'';
  q('#wifiGateway').value=s.config.wifi_gateway||'';
  q('#wifiSubnet').value=s.config.wifi_subnet||'255.255.255.0';
  q('#wifiDns1').value=s.config.wifi_dns1||'';
  q('#wifiDns2').value=s.config.wifi_dns2||'';
  syncStaticIpUi();
  q('#shelly').value=s.config.shelly_ip;
  q('#shellyProfile').value=s.config.shelly_profile||'mono';
  q('#shellyGridChannel').value=String(s.config.shelly_grid_channel??0);
  q('#shellyRoutedChannel').value=String(s.config.shelly_routed_channel??1);
  q('#shellyAuxChannel').value=String(s.config.shelly_aux_channel??-1);
  q('#shellyAuxLabel').value=s.config.shelly_aux_label||'Canal personnalisé';
  q('#shellyRoutedChannel').disabled=q('#shellyProfile').value==='tri';
  q('#shellyAuxChannel').disabled=q('#shellyProfile').value==='tri';
  q('#shellyAuxLabel').disabled=q('#shellyProfile').value==='tri';
  q('#f1').value=s.config.f1atb_ip;
  q('#f1en').checked=s.config.f1atb_enabled;
  q('#f1ActionLabel').value=s.config.f1atb_action_label||'Chauffe-eau';
  q('#f1ActionNumber').value=s.config.f1atb_action_number??0;
  q('#zwrite').checked=s.config.zendure_writes;

  q('#weatherEnabled').checked=s.config.weather_enabled;
  q('#weatherQuery').value=s.config.weather_city||'';
  q('#weatherCity').value=s.config.weather_city||'';
  q('#weatherDisplayName').value=s.config.weather_display_name||'';
  q('#weatherLat').value=s.config.weather_lat??'';
  q('#weatherLon').value=s.config.weather_lon??'';

  q('#weatherSelected').textContent=s.config.weather_display_name
    ? `Ville sélectionnée : ${s.config.weather_display_name}`
    : 'Aucune ville sélectionnée.';

  zendureDevices=(s.zendure||[]).map(z=>({
    enabled:z.enabled,
    label:z.label,
    sn:z.sn,
    manual_ip:z.manual_ip,
    ip:z.ip,
    online:z.online
  }));
  renderZendures();

  q('#current').textContent=
    `${s.wifi.ssid} • ${s.wifi.rssi} dBm • ${s.wifi.ip}`;

  q('#safety').textContent=
    s.config.zendure_writes
      ? 'Écritures Zendure ACTIVES'
      : 'Écritures Zendure BLOQUÉES (mode sûr)';

  if(s.system){
    q('#sysFreeHeap').textContent=fmtBytes(s.system.free_heap);
    q('#sysMinHeap').textContent=fmtBytes(s.system.min_free_heap);
    q('#sysMaxAlloc').textContent=fmtBytes(s.system.max_alloc_heap);
    q('#sysHeapSize').textContent=fmtBytes(s.system.heap_size);
    q('#sysSketch').textContent=fmtBytes(s.system.sketch_size);
    q('#sysSketchFree').textContent=fmtBytes(s.system.free_sketch_space);
    q('#sysFlash').textContent=fmtBytes(s.system.flash_size);
    q('#sysUptime').textContent=fmtUptime(s.system.uptime_s);
  }
}

document.querySelectorAll('[data-mode]').forEach(b=>{
  b.onclick=async()=>{
    const mode=b.dataset.mode;
    const mins=Number(q('#mins').value);
    const power=Number(q('#power').value);

    const r=await fetch('/api/control',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify({mode,minutes:mins,power_w:power})
    });

    msg(await r.text());
  };
});

function syncStaticIpUi(){
  const on=q('#wifiStaticEnabled').checked;
  ['#wifiStaticIp','#wifiGateway','#wifiSubnet','#wifiDns1','#wifiDns2'].forEach(id=>q(id).disabled=!on);
}
q('#wifiStaticEnabled').onchange=syncStaticIpUi;

q('#shellyProfile').onchange=()=>{ const tri=q('#shellyProfile').value==='tri'; q('#shellyRoutedChannel').disabled=tri; q('#shellyAuxChannel').disabled=tri; q('#shellyAuxLabel').disabled=tri; };

async function saveSettings(reboot=false){
  const body={
    hostname:q('#host').value,
    wifi_static_enabled:q('#wifiStaticEnabled').checked,
    wifi_static_ip:q('#wifiStaticIp').value.trim(),
    wifi_gateway:q('#wifiGateway').value.trim(),
    wifi_subnet:q('#wifiSubnet').value.trim(),
    wifi_dns1:q('#wifiDns1').value.trim(),
    wifi_dns2:q('#wifiDns2').value.trim(),
    shelly_ip:q('#shelly').value,
    shelly_profile:q('#shellyProfile').value,
    shelly_grid_channel:Number(q('#shellyGridChannel').value),
    shelly_routed_channel:q('#shellyProfile').value==='mono'?Number(q('#shellyRoutedChannel').value):-1,
    shelly_aux_channel:q('#shellyProfile').value==='mono'?Number(q('#shellyAuxChannel').value):-1,
    shelly_aux_label:q('#shellyAuxLabel').value.trim()||'Canal personnalisé',
    f1atb_ip:q('#f1').value,
    f1atb_enabled:q('#f1en').checked,
    f1atb_action_label:q('#f1ActionLabel').value,
    f1atb_action_number:Number(q('#f1ActionNumber').value||0),
    weather_enabled:q('#weatherEnabled').checked,
    weather_city:q('#weatherCity').value,
    weather_display_name:q('#weatherDisplayName').value,
    weather_lat:Number(q('#weatherLat').value||0),
    weather_lon:Number(q('#weatherLon').value||0),
    charge_w:Number(q('#power').value),
    zendure_writes:q('#zwrite').checked,
    zendure:collectZendures(),
    reboot
  };

  const r=await fetch('/api/config',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify(body)
  });

  const text=await r.text();
  msg(text);
  if(!r.ok) return;

  if(reboot){
    msg('Configuration enregistrée. Redémarrage de l’ESP...');
    return;
  }

  await status();
}

q('#save').onclick=()=>saveSettings(false).catch(e=>msg('Erreur d’enregistrement : '+e.message));
q('#saveRestart').onclick=()=>saveSettings(true).catch(e=>msg('Erreur d’enregistrement : '+e.message));

q('#addZendure').onclick=()=>{
  if(zendureDevices.length>=8){
    msg('Maximum 8 périphériques.');
    return;
  }
  zendureDevices.push({
    enabled:true,
    label:'SolarFlow '+(zendureDevices.length+1),
    sn:'',
    manual_ip:'',
    ip:'',
    online:false
  });
  renderZendures();
};

q('#removeZendure').onclick=()=>{
  if(!zendureDevices.length) return;
  zendureDevices.pop();
  renderZendures();
};

q('#weatherSearch').onclick=async()=>{
  const query=q('#weatherQuery').value.trim();

  if(query.length<2){
    msg('Saisis au moins 2 caractères pour la ville.');
    return;
  }

  q('#weatherResults').textContent='Recherche...';

  try{
    const r=await fetch('/api/weather/search?q='+encodeURIComponent(query),{cache:'no-store'});
    const d=await r.json();

    if(!r.ok) throw new Error(d.error||'Erreur');

    const results=d.results||[];

    if(!results.length){
      q('#weatherResults').textContent='Aucun résultat.';
      return;
    }

    q('#weatherResults').innerHTML=results.map((x,i)=>{
      const label=[x.name,x.admin1,x.country].filter(Boolean).join(', ');
      return `<button type="button" class="weather-result" data-i="${i}">${esc(label)}</button>`;
    }).join(' ');

    q('#weatherResults').querySelectorAll('.weather-result').forEach(btn=>{
      btn.onclick=()=>{
        const x=results[Number(btn.dataset.i)];
        const label=[x.name,x.admin1,x.country].filter(Boolean).join(', ');

        q('#weatherCity').value=x.name||query;
        q('#weatherDisplayName').value=label;
        q('#weatherLat').value=x.latitude;
        q('#weatherLon').value=x.longitude;
        q('#weatherSelected').textContent='Ville sélectionnée : '+label;
      };
    });
  }catch(e){
    q('#weatherResults').textContent='Recherche météo impossible.';
  }
};

q('#otaUpload').onclick=()=>{
  const file=q('#otaFile').files[0];

  if(!file){
    q('#otaMsg').textContent='Choisis d’abord un fichier .bin.';
    q('#otaProgressWrap').style.display='block';
    return;
  }

  if(!file.name.toLowerCase().endsWith('.bin')){
    q('#otaMsg').textContent='Le fichier doit être un firmware .bin.';
    q('#otaProgressWrap').style.display='block';
    return;
  }

  if(!confirm('Installer le firmware '+file.name+' ? L’ESP redémarrera automatiquement.')){
    return;
  }

  q('#otaProgressWrap').style.display='block';
  q('#otaProgress').value=0;
  q('#otaMsg').textContent='Envoi du firmware...';

  const xhr=new XMLHttpRequest();
  xhr.open('POST','/api/system/ota');

  xhr.upload.onprogress=(e)=>{
    if(!e.lengthComputable) return;
    const pct=Math.round((e.loaded/e.total)*100);
    q('#otaProgress').value=pct;
    q('#otaMsg').textContent=pct+' %';
  };

  xhr.onload=()=>{
    if(xhr.status>=200&&xhr.status<300){
      q('#otaProgress').value=100;
      q('#otaMsg').textContent='Mise à jour terminée. Redémarrage de l’ESP...';
    }else{
      q('#otaMsg').textContent='Échec de la mise à jour OTA.';
    }
  };

  xhr.onerror=()=>{
    q('#otaMsg').textContent='Erreur réseau pendant la mise à jour.';
  };

  const fd=new FormData();
  fd.append('firmware',file,file.name);
  xhr.send(fd);
};

q('#espReboot').onclick=async()=>{
  if(!confirm('Redémarrer maintenant le contrôleur ESP32 ?')) return;

  q('#systemMsg').textContent='Redémarrage...';

  try{
    const r=await fetch('/api/system/reboot',{method:'POST'});
    if(!r.ok) throw new Error();
    q('#systemMsg').textContent='ESP en cours de redémarrage...';
  }catch(e){
    q('#systemMsg').textContent='Commande de redémarrage impossible.';
  }
};

q('#factoryReset').onclick=async()=>{
  const ok=confirm(
    'FACTORY RESET\\n\\n' +
    'Toutes les configurations locales seront effacées : Wi-Fi, mot de passe Admin, Zendure, Shelly, F1ATB, météo et cache Tempo.\\n\\n' +
    'Continuer ?'
  );

  if(!ok) return;

  const second=confirm(
    'Dernière confirmation : remettre réellement le contrôleur aux paramètres usine ?'
  );

  if(!second) return;

  q('#systemMsg').textContent='Effacement et redémarrage...';

  try{
    const r=await fetch('/api/system/factory-reset',{method:'POST'});
    if(!r.ok) throw new Error();
    q('#systemMsg').textContent='Configuration effacée. Redémarrage...';
  }catch(e){
    q('#systemMsg').textContent='Factory reset impossible.';
  }
};

q('#scan').onclick=async()=>{
  q('#nets').textContent='Scan...';

  const a=await (await fetch('/api/wifi/scan')).json();

  q('#nets').innerHTML=a.networks.map(n=>
    `<button type="button" data-ssid="${n.ssid.replace(/"/g,'&quot;')}">${n.ssid} (${n.rssi} dBm)</button>`
  ).join(' ');

  q('#nets').querySelectorAll('button').forEach(b=>{
    b.onclick=()=>q('#ssid').value=b.dataset.ssid;
  });
};

q('#wifiSave').onclick=async()=>{
  const r=await fetch('/api/wifi/save',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({
      ssid:q('#ssid').value,
      password:q('#wpass').value
    })
  });

  msg(await r.text());
};

q('#discover').onclick=async()=>{
  msg(await (await fetch('/api/zendure/discover',{method:'POST'})).text());
};


q('#logout').onclick=async()=>{
  await fetch('/api/logout',{method:'POST'});
  location.reload();
};

status().catch(e=>msg('Impossible de charger les paramètres enregistrés : '+e.message));
</script>
</body>
</html>
)rawliteral";
}


String setupHtml() {
  String html = R"rawliteral(
<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Configuration Wi-Fi</title>
<style>
body{font-family:Arial;background:#f3f4f6;color:#17202a}
.box{max-width:520px;margin:25px auto;background:#fff;border:1px solid #ddd;border-radius:8px;padding:16px}
input,button{box-sizing:border-box;width:100%;padding:11px;margin:6px 0;font-size:16px;border-radius:6px}
input{border:1px solid #cbd5e1;background:#fff}
button{background:#263746;color:#fff;border:0;font-weight:600}
button:disabled{opacity:.55}
.net{background:#eef1f4;color:#222;text-align:left;border:1px solid #d8dee4}
.note{font-size:14px;color:#64748b;line-height:1.4}
.status{padding:10px;margin:8px 0;background:#f8fafc;border:1px solid #e2e8f0;border-radius:6px;white-space:pre-wrap}
</style>
</head>
<body>
<div class="box">
  <h2 id="title">Configuration Wi-Fi</h2>
  <p id="intro" class="note">Recherche des réseaux disponibles…</p>

  <button id="scan" type="button">Scanner le Wi-Fi</button>
  <div id="nets">__INITIAL_NETWORKS__</div>

  <form id="f">
    <input id="ssid" placeholder="SSID" required>
    <input id="wp" type="password" placeholder="Mot de passe Wi-Fi">
    <input id="ap" type="password" minlength="8" placeholder="Mot de passe administrateur (8 caractères min.)">
    <div id="adminHint" class="note"></div>
    <button id="save">Enregistrer et redémarrer</button>
  </form>

  <div id="m" class="status"></div>
</div>

<script>
const q=s=>document.querySelector(s);
let hasAdmin=false;

async function loadStatus(){
  try{
    const r=await fetch('/api/setup/status',{cache:'no-store'});
    if(!r.ok) throw new Error('HTTP '+r.status);
    const s=await r.json();
    hasAdmin=!!s.has_admin;
    if(s.saved_ssid) q('#ssid').value=s.saved_ssid;

    if(s.has_saved_wifi){
      q('#title').textContent='Wi-Fi indisponible — mode secours';
      q('#intro').textContent='Le réseau mémorisé « '+s.saved_ssid+' » est actuellement inaccessible. Sélectionnez un réseau ci-dessous ou corrigez les identifiants.';
    }else{
      q('#title').textContent='Zendure Tempo — première installation';
      q('#intro').textContent='Sélectionnez le réseau Wi-Fi auquel connecter le contrôleur.';
    }

    if(hasAdmin){
      q('#ap').required=false;
      q('#ap').placeholder='Nouveau mot de passe admin (optionnel)';
      q('#adminHint').textContent='Mot de passe administrateur déjà enregistré : laissez ce champ vide pour le conserver.';
    }else{
      q('#ap').required=true;
      q('#adminHint').textContent='Créez un mot de passe administrateur d’au moins 8 caractères.';
    }
  }catch(e){
    q('#m').textContent='Impossible de lire l’état du portail : '+e.message;
  }
}

function bindNetButtons(){
  q('#nets').querySelectorAll('button.net').forEach(b=>{
    b.onclick=()=>{ q('#ssid').value=b.dataset.s; q('#wp').focus(); };
  });
}

async function scan(){
  const btn=q('#scan');
  btn.disabled=true;
  btn.textContent='Scan en cours…';
  q('#m').textContent='Recherche des réseaux Wi-Fi…';
  q('#nets').innerHTML='';
  try{
    const r=await fetch('/api/setup/scan',{cache:'no-store'});
    if(!r.ok) throw new Error('HTTP '+r.status);
    const a=await r.json();
    if(!a.scan_ok) throw new Error('scan ESP32 '+(a.error??'inconnu'));
    if(!a.networks || !a.networks.length){
      q('#m').textContent='Aucun réseau détecté. Appuyez sur « Scanner le Wi-Fi » pour réessayer.';
      return;
    }
    q('#nets').innerHTML=a.networks.map(n=>
      `<button type="button" class="net" data-s="${String(n.ssid).replace(/&/g,'&amp;').replace(/"/g,'&quot;').replace(/</g,'&lt;')}">${n.ssid} • ${n.rssi} dBm${n.open?' • ouvert':''}</button>`
    ).join('');
    bindNetButtons();
    q('#m').textContent=a.networks.length+' réseau(x) détecté(s).';
  }catch(e){
    q('#m').textContent='Échec du scan : '+e.message+' — réessayez dans quelques secondes.';
  }finally{
    btn.disabled=false;
    btn.textContent='Scanner le Wi-Fi';
  }
}

q('#scan').onclick=scan;

q('#f').onsubmit=async e=>{
  e.preventDefault();
  q('#save').disabled=true;
  q('#m').textContent='Enregistrement…';
  try{
    const r=await fetch('/api/setup/save',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify({
        ssid:q('#ssid').value.trim(),
        password:q('#wp').value,
        admin:q('#ap').value
      })
    });
    const txt=await r.text();
    q('#m').textContent=txt;
    if(!r.ok) q('#save').disabled=false;
  }catch(e){
    q('#m').textContent='Échec de l’enregistrement : '+e.message;
    q('#save').disabled=false;
  }
};

(async()=>{
  // Les réseaux du premier scan sont déjà injectés par l'ESP32: ils restent
  // utilisables même si le navigateur captif bloque les fetch automatiques.
  bindNetButtons();
  await loadStatus();

  // Un second scan est tenté ensuite pour rafraîchir la liste, sans rendre la
  // page dépendante de JavaScript pour son premier affichage.
  setTimeout(scan,2500);
})();
</script>
</body>
</html>
)rawliteral";
  html.replace("__INITIAL_NETWORKS__", setupWifiScanHtml());
  return html;
}
