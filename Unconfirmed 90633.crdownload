const $ = id => document.getElementById(id);

const state = {
  selected: null,
  sensor: null,
  regions: [],
  history: [],
  time: 0,
  maxTime: 120,
  scenario: 'monsoon',
  map: null,
  gridLayer: null,
  sensorMarker: null,
  routeLayer: null,
  simulationReady: false,
  playing: false,
  playTimer: null,
  selectedRegionId: 'R-41'
};

const scenarios = {
  light:   { rain: 35,  drain: 65, water: 8,  terrain: 620 },
  monsoon: { rain: 80,  drain: 45, water: 20, terrain: 620 },
  extreme: { rain: 180, drain: 28, water: 32, terrain: 600 }
};


// =====================================================
// ESP32 / VELXIO MQTT
// =====================================================

const FLOW_MQTT_BROKER = 'wss://broker.hivemq.com:8884/mqtt';
const FLOW_MQTT_TELEMETRY = 'flowshield/node1/telemetry';
const FLOW_MQTT_LOCATION = 'flowshield/node1/command/location';
let flowMQTT = null;

function connectFlowShieldMQTT() {
  if (typeof mqtt === 'undefined') {
    console.warn('MQTT.js was not loaded.');
    return;
  }

  if (flowMQTT && flowMQTT.connected) return;

  const clientId =
    'FlowShieldDashboard-' +
    Math.random().toString(16).slice(2);

  flowMQTT = mqtt.connect(
    FLOW_MQTT_BROKER,
    {
      clientId,
      clean: true,
      reconnectPeriod: 2000,
      connectTimeout: 8000
    }
  );

  flowMQTT.on('connect', () => {
    $('connectionText').textContent = 'MQTT • ESP32 WAITING';
    if ($('mqttState')) $('mqttState').textContent = 'CONNECTED';
    $('connectionDot').className = 'dot online';
    $('sensorPill').textContent = 'MQTT';
    flowMQTT.subscribe(FLOW_MQTT_TELEMETRY);
  });

  flowMQTT.on('close', () => {
    if (!state.sensor) {
      $('connectionText').textContent = 'MQTT • DISCONNECTED';
      if ($('mqttState')) $('mqttState').textContent = 'DISCONNECTED';
      $('connectionDot').className = 'dot';
      $('sensorPill').textContent = 'DEMO';
    }
  });

  flowMQTT.on('error', err => {
    console.warn('FlowShield MQTT:', err);
  });

  flowMQTT.on('message', (topic, payload) => {
    if (topic !== FLOW_MQTT_TELEMETRY) return;

    try {
      const d = JSON.parse(payload.toString());
      const lat = Number(d.latitude);
      const lng = Number(d.longitude);
      const rain = Number(d.rainfall_mm_h);
      const waterCm = Number(d.water_level_cm);
      const terrain = Number(d.gps_altitude_m);
      const inflow = Number(d.inflow_adc);
      const drain = Number(d.drainage_adc);

      // The ESP32 always sends finite coordinates once a map point is selected.
      if (!Number.isFinite(lat) || !Number.isFinite(lng)) return;

      const elevation = Number.isFinite(terrain) && terrain >= 0
        ? terrain
        : Number($('terrain').value);

      applySensor({
        lat, lng,
        rainfall: Number.isFinite(rain) ? rain : 0,
        elevation,
        ts: Date.now(),
        upstream_cm: Number(d.upstream_cm),
        downstream_cm: Number(d.downstream_cm),
        inflow_adc: inflow,
        drainage_adc: drain,
        water_level_cm: waterCm,
        water_level_pct: Number(d.water_level_pct)
      });

      $('sensorPill').textContent = 'ESP32 LIVE';
      $('connectionText').textContent = 'ESP32 • VELXIO • LIVE';
      $('connectionDot').className = 'dot online';
      if ($('mqttState')) $('mqttState').textContent = 'LIVE • ' + new Date().toLocaleTimeString();

      if ($('sensorInflow')) $('sensorInflow').textContent = Number.isFinite(inflow) ? Math.round(inflow) : '—';
      if ($('sensorDrain')) $('sensorDrain').textContent = Number.isFinite(drain) ? Math.round(drain) : '—';

      // Live ESP32 readings become the simulation inputs.
      if (Number.isFinite(rain)) {
        $('rainfall').value = clamp(rain, 0, 250);
        $('rainOut').textContent = rain.toFixed(1);
      }
      if (Number.isFinite(waterCm)) {
        $('water').value = clamp(waterCm, 0, 100);
        $('waterOut').textContent = waterCm.toFixed(1);
      }
      if (Number.isFinite(drain)) {
        // Map ADC 0..4095 to the existing drainage-control range 5..120 mm/h.
        const drainCapacity = 5 + (clamp(drain, 0, 4095) / 4095) * 115;
        $('drainage').value = drainCapacity;
        $('drainOut').textContent = drainCapacity.toFixed(1);
      }

      // Rebuild immediately so a pot movement changes the flood state.
      runSimulation();
    } catch (err) {
      console.warn('Invalid ESP32 telemetry:', err, payload?.toString?.());
    }
  });
}

function sendMapLocationToESP32(lat, lng) {
  if (!flowMQTT || !flowMQTT.connected) {
    $('connectionText').textContent = 'MAP SELECTED • ESP32 OFFLINE';
    if ($('mqttState')) $('mqttState').textContent = 'ESP32 OFFLINE';
    return false;
  }

  const payload =
    Number(lat).toFixed(6) +
    ',' +
    Number(lng).toFixed(6);

  flowMQTT.publish(
    FLOW_MQTT_LOCATION,
    payload,
    { qos: 1, retain: true },
    err => {
      if (err) {
        console.warn('FlowShield location publish failed:', err);
        $('connectionText').textContent = 'MAP → ESP32 • SEND FAILED';
        if ($('mqttState')) $('mqttState').textContent = 'LOCATION SEND FAILED';
        return;
      }

      $('connectionText').textContent = 'MAP → ESP32 • SENT';
      if ($('mqttState')) $('mqttState').textContent = 'LOCATION SENT';
      console.log('FlowShield location sent:', payload);
    }
  );

  return true;
}


const riskColor = {
  Safe: '#39d98a',
  Warning: '#f4c95d',
  Critical: '#ff5f6d'
};

function fmt(n) { return Math.round(n).toLocaleString('en-IN'); }
function clamp(v, a, b) { return Math.max(a, Math.min(b, v)); }

function setSlider(id, out, transform = v => v) {
  $(id).addEventListener('input', () => $(out).textContent = transform($(id).value));
}
setSlider('rainfall', 'rainOut');
setSlider('drainage', 'drainOut');
setSlider('water', 'waterOut');
setSlider('terrain', 'terrainOut');
setSlider('pop', 'popOut', v => fmt(v));

$('clock').textContent = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
setInterval(() => {
  $('clock').textContent = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}, 1000);

// Standalone map: map-core.js does not request external map tiles.
state.map = L.map('map', { zoomControl: false }).setView([12.9716, 77.5946], 11);
L.control.zoom({ position: 'bottomright' }).addTo(state.map);
state.map.on('click', e => {
  selectPoint(e.latlng.lat, e.latlng.lng);
  sendMapLocationToESP32(e.latlng.lat, e.latlng.lng);
});

function selectPoint(lat, lng) {
  state.selected = { lat, lng };
  $('selectedLocation').textContent = `${lat.toFixed(5)}, ${lng.toFixed(5)}`;
  $('sensorLat').textContent = lat.toFixed(5);
  $('sensorLng').textContent = lng.toFixed(5);

  if (state.sensorMarker) state.map.removeLayer(state.sensorMarker);
  state.sensorMarker = L.marker([lat, lng]).addTo(state.map)
    .bindPopup(`<b>Selected sensor point</b><br>${lat.toFixed(5)}, ${lng.toFixed(5)}<br><small>Run ESP32 simulation or use the controls.</small>`)
    .openPopup();

  // Re-running after a new location guarantees that every panel uses this point.
  state.simulationReady = false;
  $('chartLocation').textContent = 'Select point • simulation pending';
  $('routeEmpty').textContent = 'Run simulation to calculate the nearest safe region and route.';
  $('routeInfo').classList.add('hidden');
  $('routeEmpty').classList.remove('hidden');
  $('alertPreview').textContent = 'Location selected. Run the flood simulation to generate updated warnings.';
}

function applySensor(s) {
  state.sensor = s;
  if (!state.selected) selectPoint(Number(s.lat), Number(s.lng));
  else state.selected = { lat: Number(s.lat), lng: Number(s.lng) };

  $('sensorLat').textContent = Number(s.lat).toFixed(5);
  $('sensorLng').textContent = Number(s.lng).toFixed(5);
  $('sensorRain').textContent = Number(s.rainfall).toFixed(1);
  $('sensorTerrain').textContent = Number(s.elevation).toFixed(1);
  if ($('sensorInflow')) $('sensorInflow').textContent = Number.isFinite(Number(s.inflow_adc)) ? Math.round(Number(s.inflow_adc)) : '—';
  if ($('sensorDrain')) $('sensorDrain').textContent = Number.isFinite(Number(s.drainage_adc)) ? Math.round(Number(s.drainage_adc)) : '—';
  $('rainfall').value = clamp(Number(s.rainfall), 0, 250);
  $('terrain').value = clamp(Number(s.elevation), 0, 1500);
  $('rainOut').textContent = Number(s.rainfall).toFixed(1);
  $('terrainOut').textContent = Number(s.elevation).toFixed(1);
}

$('demoEsp').onclick = () => {
  if (!state.selected) selectPoint(12.9716, 77.5946);
  applySensor({
    lat: state.selected.lat,
    lng: state.selected.lng,
    rainfall: Number($('rainfall').value),
    elevation: Number($('terrain').value),
    ts: Date.now()
  });
  $('sensorPill').textContent = 'DEMO ESP32';
  $('connectionText').textContent = 'ESP32 DEMO • READY';
  $('connectionDot').className = 'dot online';
};

document.querySelectorAll('[data-scenario]').forEach(btn => {
  btn.onclick = () => {
    document.querySelectorAll('[data-scenario]').forEach(x => x.classList.remove('active'));
    btn.classList.add('active');
    state.scenario = btn.dataset.scenario;
    const s = scenarios[state.scenario];
    $('rainfall').value = s.rain;
    $('drainage').value = s.drain;
    $('water').value = s.water;
    $('terrain').value = s.terrain;
    $('rainOut').textContent = s.rain;
    $('drainOut').textContent = s.drain;
    $('waterOut').textContent = s.water;
    $('terrainOut').textContent = s.terrain;
    // Scenario change immediately recalculates the complete dashboard.
    runSimulation();
  };
});

$('simulate').onclick = runSimulation;
$('reset').onclick = reset;
$('timeSlider').oninput = () => {
  if (!state.simulationReady) return;
  state.time = Number($('timeSlider').value);
  renderTime(state.time);
};

function seeded(i, j = 0) {
  const x = Math.sin(i * 91.7 + j * 37.2) * 43758.5453;
  return x - Math.floor(x);
}

function buildRegions() {
  const base = state.selected || { lat: 12.9716, lng: 77.5946 };
  const rain = Number($('rainfall').value);
  const drain = Number($('drainage').value);
  const water = Number($('water').value);
  const terrain = Number($('terrain').value);
  const popDensity = Number($('pop').value);
  const n = 9;
  const out = [];

  for (let y = 0; y < n; y++) {
    for (let x = 0; x < n; x++) {
      const dx = x - 4, dy = y - 4;
      const dist = Math.hypot(dx, dy);
      const noise = seeded(x, y);
      const elevation = Math.max(0, terrain + dx * 8 - dy * 6 + (noise - .5) * 55 - dist * 2.5);
      const localRain = rain * (0.82 + seeded(y + 11, x + 3) * 0.42);
      const localDrain = Math.max(5, drain * (0.70 + seeded(x + 7, y + 4) * 0.55));
      const initial = Math.max(0, water * (0.72 + seeded(x + 4, y + 9) * 0.52));
      const population = Math.round((popDensity / 1e6) * 0.15 * (0.72 + noise * 0.58) * 100000);
      out.push({
        id: `R-${String(y * n + x + 1).padStart(2, '0')}`,
        x, y, n,
        lat: base.lat + (y - 4) * 0.0095,
        lng: base.lng + (x - 4) * 0.0115,
        elevation,
        rain: localRain,
        drain: localDrain,
        initial,
        pop: population,
        water: [],
        risk: [],
        criticalAt: null
      });
    }
  }
  return out;
}

function simulateScenario(name, override = {}, sourceRegions = null) {
  const s = scenarios[name];
  const rain = override.rain ?? s.rain;
  const drain = override.drain ?? s.drain;
  const init = override.water ?? s.water;
  const terrain = override.terrain ?? s.terrain;
  const regs = (sourceRegions || buildRegions()).map(r => ({ ...r, water: [], risk: [], criticalAt: null }));
  const steps = state.maxTime + 1;
  const matrix = [];
  const index = (x, y) => y * 9 + x;

  regs.forEach(r => {
    r.rain = r.rain * (rain / Math.max(1, Number($('rainfall').value)));
    r.drain = r.drain * (drain / Math.max(1, Number($('drainage').value)));
    r.initial = r.initial * (init / Math.max(1, Number($('water').value)));
  });

  for (let t = 0; t < steps; t++) {
    const levels = new Array(regs.length).fill(0);

    regs.forEach((r, idx) => {
      const prev = t === 0 ? r.initial : r.water[t - 1];
      const neighborIds = [];
      if (r.x > 0) neighborIds.push(index(r.x - 1, r.y));
      if (r.x < 8) neighborIds.push(index(r.x + 1, r.y));
      if (r.y > 0) neighborIds.push(index(r.x, r.y - 1));
      if (r.y < 8) neighborIds.push(index(r.x, r.y + 1));

      // Rainfall adds water; drainage removes it; higher water moves downhill.
      let inflow = 0;
      let outflow = 0;
      neighborIds.forEach(nid => {
        const nb = regs[nid];
        const nbPrev = t === 0 ? nb.initial : nb.water[t - 1];
        const elevationDrop = r.elevation - nb.elevation;
        const pressure = Math.max(0, prev - nbPrev);
        if (elevationDrop > 0) inflow += pressure * 0.028 * (1 + elevationDrop / 120);
        else outflow += pressure * 0.012;
      });

      const runoff = r.rain * 0.0105;
      const terrainFactor = clamp((700 - r.elevation) / 700, -0.15, 1);
      const drainageLoss = r.drain * 0.0065 * (1 + terrainFactor * 0.45);
      const level = Math.max(0, prev + runoff + inflow - outflow - drainageLoss);
      levels[idx] = level;
      r.water[t] = level;

      // Thresholds are intentionally simple demo-model thresholds.
      const risk = level >= 50 ? 'Critical' : level >= 25 ? 'Warning' : 'Safe';
      r.risk[t] = risk;
      if (risk === 'Critical' && r.criticalAt === null) r.criticalAt = t;
    });

    matrix.push(levels);
  }
  return { regs, matrix };
}

function selectedRegion() {
  if (!state.regions.length) return null;
  const p = state.selected || { lat: 12.9716, lng: 77.5946 };
  return state.regions.reduce((best, r) => {
    const d = Math.hypot(r.lat - p.lat, r.lng - p.lng);
    return !best || d < best.d ? { r, d } : best;
  }, null)?.r || null;
}

function runSimulation() {
  if (!state.selected) selectPoint(12.9716, 77.5946);

  stopPlayback();
  state.maxTime = 120;
  state.time = 0;
  $('timeSlider').max = state.maxTime;
  $('timeSlider').value = 0;

  state.regions = buildRegions();
  const result = simulateScenario(state.scenario, {
    rain: Number($('rainfall').value),
    drain: Number($('drainage').value),
    water: Number($('water').value),
    terrain: Number($('terrain').value)
  }, state.regions);
  state.regions = result.regs;
  state.history = result.matrix;
  state.selectedRegionId = selectedRegion()?.id || 'R-41';
  state.simulationReady = true;

  $('sensorRain').textContent = Number($('rainfall').value).toFixed(1);
  $('sensorTerrain').textContent = Number($('terrain').value).toFixed(1);
  $('sensorPill').textContent = state.sensor ? 'ESP32' : 'SIMULATED';
  $('connectionText').textContent = state.sensor ? 'ESP32 DATA • SIMULATED' : 'MODEL • SIMULATED';
  $('connectionDot').className = 'dot online';

  renderGrid();
  renderTime(0);
  renderChart();
  renderRegions();
  renderScenarioComparison();
  renderRoute();
  generateAlert();
}

function renderGrid() {
  if (state.gridLayer) state.map.removeLayer(state.gridLayer);
  const group = L.layerGroup();

  state.regions.forEach(r => {
    const stepLat = 0.0095, stepLng = 0.0115;
    const b = [
      [r.lat - stepLat / 2, r.lng - stepLng / 2],
      [r.lat + stepLat / 2, r.lng + stepLng / 2]
    ];
    const rect = L.rectangle(b, {
      color: '#d5e8de',
      weight: 1,
      fillColor: riskColor.Safe,
      fillOpacity: 0.32
    });
    rect.bindTooltip(`<b>${r.id}</b><br>Elevation: ${r.elevation.toFixed(0)} m`);
    rect.on('click', e => {
      L.DomEvent.stopPropagation(e);
      state.selectedRegionId = r.id;
      state.selected = { lat: r.lat, lng: r.lng };
      sendMapLocationToESP32(r.lat, r.lng);
      $('selectedLocation').textContent = `${r.lat.toFixed(5)}, ${r.lng.toFixed(5)}`;
      $('sensorLat').textContent = r.lat.toFixed(5);
      $('sensorLng').textContent = r.lng.toFixed(5);
      if (state.sensorMarker) state.map.removeLayer(state.sensorMarker);
      state.sensorMarker = L.marker([r.lat, r.lng]).addTo(state.map).bindPopup(`<b>${r.id}</b><br>Region selected`).openPopup();
      renderTime(state.time);
      renderChart();
      renderRoute();
      generateAlert();
    });
    rect.addTo(group);
  });

  state.gridLayer = group.addTo(state.map);
}

function renderTime(t) {
  if (!state.simulationReady) return;
  state.time = t;
  $('timeLabel').textContent = `${t} min`;

  state.regions.forEach((r, i) => {
    const risk = r.risk[t] || 'Safe';
    const layer = state.gridLayer?.layers?.[i];
    if (layer) {
      layer.setStyle({
        fillColor: riskColor[risk],
        fillOpacity: risk === 'Critical' ? 0.72 : risk === 'Warning' ? 0.56 : 0.30,
        color: risk === 'Critical' ? '#ffb1b8' : risk === 'Warning' ? '#ffe6a3' : '#c9eadb',
        weight: 1.4
      });
      layer.setTooltipContent(`<b>${r.id}</b><br>Water: ${(r.water[t] || 0).toFixed(1)} cm<br>Risk: ${risk}<br>Elevation: ${r.elevation.toFixed(0)} m<br>Population: ${fmt(r.pop)}`);
    }
  });

  const levels = state.regions.map(r => r.water[t] || 0);
  const critical = state.regions.filter(r => r.risk[t] === 'Critical');
  const affected = state.regions.filter(r => r.risk[t] !== 'Safe').reduce((a, r) => a + r.pop, 0);
  const warning = state.regions.filter(r => r.risk[t] === 'Warning').length;
  const safe = state.regions.length - critical.length - warning;
  const criticalTimes = state.regions.map(r => r.criticalAt).filter(x => x !== null);

  $('maxWater').textContent = `${Math.max(...levels).toFixed(1)} cm`;
  $('criticalCount').textContent = critical.length;
  $('affectedPop').textContent = fmt(affected);
  $('criticalTime').textContent = criticalTimes.length ? `${Math.min(...criticalTimes)} min` : '—';
  $('criticalSub').textContent = criticalTimes.length ? 'Earliest modeled critical region' : 'No critical threshold reached';
  $('regionCount').textContent = `${safe} safe • ${warning} warning • ${critical.length} critical`;

  const sr = selectedRegion();
  $('chartLocation').textContent = sr ? `${sr.id} • ${sr.risk[t]}` : 'Selected region';
  renderChart();
  renderRegions();
  renderRoute();
  generateAlert();
}

function renderChart() {
  if (!state.simulationReady) return;
  const c = $('waterChart');
  const ctx = c.getContext('2d');
  const w = c.clientWidth || 600, h = 190, dpr = devicePixelRatio || 1;
  c.width = w * dpr; c.height = h * dpr; ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, w, h);

  const r = selectedRegion() || state.regions[40];
  const max = Math.max(60, ...r.water);
  const x0 = 38, x1 = w - 12, y0 = h - 28, y1 = 18;

  ctx.strokeStyle = '#29493e'; ctx.lineWidth = 1;
  for (let i = 0; i <= 4; i++) {
    const y = y0 - (i / 4) * (y0 - y1);
    ctx.beginPath(); ctx.moveTo(x0, y); ctx.lineTo(x1, y); ctx.stroke();
  }

  // Warning / critical thresholds.
  [25, 50].forEach((threshold, idx) => {
    const y = y0 - (threshold / max) * (y0 - y1);
    ctx.setLineDash([5, 4]);
    ctx.strokeStyle = idx === 0 ? '#b08d32' : '#a9444d';
    ctx.beginPath(); ctx.moveTo(x0, y); ctx.lineTo(x1, y); ctx.stroke();
    ctx.setLineDash([]);
    ctx.font = '9px system-ui'; ctx.fillStyle = idx === 0 ? '#d5bd6a' : '#ff8d96';
    ctx.fillText(idx === 0 ? 'Warning' : 'Critical', x1 - 50, y - 4);
  });

  ctx.strokeStyle = '#59e1a0'; ctx.lineWidth = 2.5; ctx.beginPath();
  r.water.forEach((v, i) => {
    const x = x0 + (i / state.maxTime) * (x1 - x0);
    const y = y0 - (v / max) * (y0 - y1);
    i ? ctx.lineTo(x, y) : ctx.moveTo(x, y);
  });
  ctx.stroke();

  const t = state.time;
  const tv = r.water[t] || 0;
  const tx = x0 + (t / state.maxTime) * (x1 - x0);
  const ty = y0 - (tv / max) * (y0 - y1);
  ctx.fillStyle = '#ffffff'; ctx.beginPath(); ctx.arc(tx, ty, 4, 0, Math.PI * 2); ctx.fill();

  ctx.fillStyle = '#7d9c90'; ctx.font = '9px system-ui';
  ctx.fillText('0', 10, y0 + 3); ctx.fillText(`${Math.round(max)} cm`, 4, y1 + 3);
  ctx.fillText('0 min', x0, h - 7); ctx.fillText(`${state.maxTime} min`, x1 - 44, h - 7);
}

function renderRegions() {
  if (!state.regions.length) return;
  const sorted = [...state.regions].sort((a, b) => (b.water[state.time] || 0) - (a.water[state.time] || 0)).slice(0, 12);
  $('regionTable').innerHTML = '<div class="region-row header"><span>Region</span><span>Water</span><span>Risk</span><span>Critical at</span></div>' +
    sorted.map(r => {
      const risk = r.risk[state.time] || 'Safe';
      return `<div class="region-row"><span>${r.id}</span><span>${(r.water[state.time] || 0).toFixed(1)} cm</span><span class="risk-${risk.toLowerCase()}">${risk}</span><span>${r.criticalAt !== null ? r.criticalAt + ' min' : '—'}</span></div>`;
    }).join('');
}

function renderScenarioComparison() {
  if (!state.simulationReady) return;

  // Keep the comparison independent of the currently selected scenario.
  // buildRegions() supplies the same spatial layout/population/elevation template,
  // while simulateScenario() uses each scenario's own rainfall, drainage and
  // initial-water values. The previous code passed `current` as the override,
  // which made all three scenarios use the same inputs and therefore appear identical.
  const templateRegions = buildRegions();
  const vals = ['light', 'monsoon', 'extreme'].map(name => {
    const x = simulateScenario(name, {}, templateRegions);
    const finalCritical = x.regs.filter(r => r.risk[state.maxTime] === 'Critical').length;
    const finalWarning = x.regs.filter(r => r.risk[state.maxTime] === 'Warning').length;
    const max = Math.max(...x.regs.map(r => Math.max(...r.water)));
    const affected = x.regs.filter(r => r.risk[state.maxTime] !== 'Safe').reduce((a, r) => a + r.pop, 0);
    return { name, finalCritical, finalWarning, max, affected };
  });
  const max = Math.max(1, ...vals.map(x => x.max));
  $('scenarioBars').innerHTML = vals.map(x => `
    <div class="bar-row scenario-card">
      <div class="scenario-name"><b>${x.name[0].toUpperCase() + x.name.slice(1)}</b><small>${x.finalCritical} critical • ${x.finalWarning} warning</small></div>
      <div class="bar-track"><div class="bar-fill" style="width:${Math.min(100, x.max / max * 100)}%"></div></div>
      <div class="scenario-value"><b>${x.max.toFixed(0)} cm</b><small>${fmt(x.affected)} affected</small></div>
    </div>`).join('');
}

function nearestSafe() {
  if (!state.selected || !state.regions.length) return null;
  const safe = state.regions.filter(r => r.risk[state.time] === 'Safe');
  if (!safe.length) return null;
  const base = state.selected;
  let best = null;
  safe.forEach(r => {
    const d = Math.hypot((r.lat - base.lat) * 111, (r.lng - base.lng) * 111 * Math.cos(base.lat * Math.PI / 180));
    if (!best || d < best.d) best = { r, d };
  });
  return best;
}

function renderRoute() {
  if (!state.simulationReady) return;
  const b = nearestSafe();
  if (state.routeLayer) { state.map.removeLayer(state.routeLayer); state.routeLayer = null; }

  if (!b) {
    $('routeEmpty').textContent = 'No safe region is currently available in the modeled grid.';
    $('routeInfo').classList.add('hidden');
    $('routeEmpty').classList.remove('hidden');
    return;
  }

  $('routeEmpty').classList.add('hidden');
  $('routeInfo').classList.remove('hidden');
  $('safeRegion').textContent = `${b.r.id} • Safe`;
  $('routeDistance').textContent = `${b.d.toFixed(2)} km`;
  $('routeTime').textContent = `${Math.max(2, Math.round(b.d * 12))} min`;

  const start = state.selected;
  const mid = { lat: (start.lat + b.r.lat) / 2, lng: (start.lng + b.r.lng) / 2 };
  state.routeLayer = L.layerGroup();
  L.polyline([[start.lat, start.lng], [mid.lat, mid.lng], [b.r.lat, b.r.lng]], { color: '#73e5b1', weight: 4, dashArray: '7 6' }).addTo(state.routeLayer);
  state.routeLayer.addTo(state.map);

  $('routeSteps').innerHTML = `<span>Current point</span><b>→</b><span>Safe corridor</span><b>→</b><span>${b.r.id}</span>`;
}

function generateAlert() {
  if (!state.simulationReady) return;
  const sr = selectedRegion();
  const risk = sr?.risk[state.time] || 'Safe';
  const affected = state.regions.filter(r => r.risk[state.time] !== 'Safe').reduce((a, r) => a + r.pop, 0);
  const b = nearestSafe();
  const criticalAt = sr?.criticalAt;
  const next = criticalAt !== null && criticalAt > state.time ? ` Estimated time to critical for this region: ${criticalAt - state.time} min.` : '';
  $('alertPreview').textContent = `FLOWSHIELD ${risk.toUpperCase()} ALERT: At ${state.time} min, ${sr?.id || 'the selected region'} is ${risk}. Estimated affected population: ${fmt(affected)}.${next} ${b ? `Nearest modeled safe region: ${b.r.id}, ${b.d.toFixed(1)} km away.` : 'No modeled safe region is available.'} This is a demonstration model; follow official emergency instructions.`;
}

$('generateMsg').onclick = generateAlert;
$('copyMsg').onclick = async () => {
  try { await navigator.clipboard.writeText($('alertPreview').textContent); } catch (_) {}
};

// Add a simple play/pause control dynamically so the map and all panels can be watched together.
const timeBox = $('timeSlider').parentElement;
const playBtn = document.createElement('button');
playBtn.className = 'ghost-btn';
playBtn.textContent = '▶ Play flood progression';
playBtn.style.marginTop = '8px';
timeBox.appendChild(playBtn);
playBtn.onclick = () => {
  if (!state.simulationReady) return;
  if (state.playing) stopPlayback();
  else {
    state.playing = true;
    playBtn.textContent = '⏸ Pause flood progression';
    state.playTimer = setInterval(() => {
      const next = Number($('timeSlider').value) + 1;
      if (next > state.maxTime) return stopPlayback();
      $('timeSlider').value = next;
      renderTime(next);
    }, 180);
  }
};
function stopPlayback() {
  state.playing = false;
  clearInterval(state.playTimer);
  state.playTimer = null;
  playBtn.textContent = '▶ Play flood progression';
}

function reset() {
  stopPlayback();
  state.regions = []; state.history = []; state.simulationReady = false;
  $('regionTable').innerHTML = '<div class="empty">Run a simulation to populate vulnerable regions.</div>';
  $('scenarioBars').innerHTML = '<div class="empty">Run a simulation to compare scenarios.</div>';
  $('maxWater').textContent = '0 cm';
  $('criticalCount').textContent = '0';
  $('affectedPop').textContent = '0';
  $('criticalTime').textContent = '—';
  $('routeInfo').classList.add('hidden');
  $('routeEmpty').classList.remove('hidden');
  $('routeEmpty').textContent = 'Run a simulation to calculate the nearest safe region and route.';
  $('alertPreview').textContent = 'Select a point and run a simulation to generate a citizen warning.';
  if (state.gridLayer) { state.map.removeLayer(state.gridLayer); state.gridLayer = null; }
  if (state.routeLayer) { state.map.removeLayer(state.routeLayer); state.routeLayer = null; }
}

window.addEventListener('resize', () => {
  if (state.simulationReady) { renderChart(); renderTime(state.time); }
});

window.addEventListener('load', connectFlowShieldMQTT);
