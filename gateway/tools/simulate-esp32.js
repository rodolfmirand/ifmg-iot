// Simula o ESP32 (mesmos tópicos/payloads do firmware) + mensagens inválidas propositais.
// Uso: MQTT_URL=mqtt://localhost:1883 node tools/simulate-esp32.js
const mqtt = require('mqtt');
const url = process.env.MQTT_URL || 'mqtt://broker.hivemq.com:1883';
const base = 'ifmg/iot3/2026-1/rodolfo/esp32cadeiafria';
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

const c = mqtt.connect(url, {
  clientId: 'sim-esp32-' + Math.random().toString(16).slice(2, 6),
  will: { topic: `${base}/availability`, payload: 'offline', qos: 1, retain: true },
});

c.on('connect', async () => {
  const pub = (t, p, o = {}) => new Promise((r) => c.publish(`${base}/${t}`, typeof p === 'string' ? p : JSON.stringify(p), { qos: 1, ...o }, r));
  let armed = true;
  await pub('availability', 'online', { retain: true });
  const temps = [null, 20.88, 20.94, 7.5, 5.2, 9.1];
  for (let i = 0; i < temps.length; i++) {
    const v = temps[i];
    await pub('telemetry', { deviceId: 'esp32cadeiafria', sensor: 'temperatura', value: v, unit: 'C', sequence: i,
      uptimeMs: 15029 * (i + 1), wifiRssi: -55, armed, ledOn: armed && v !== null && (v < 2 || v > 8) });
    if (i === 3) {
      await pub('command', { action: 'set', target: 'alarm', value: false, requestId: 'sim-01' });
      armed = false;
      await pub('state', { deviceId: 'esp32cadeiafria', actuator: 'alarm', state: false, requestId: 'sim-01' }, { retain: true });
    }
    await sleep(300);
  }
  // --- Mensagens inválidas propositais (validação da Entrega 4) ---
  await pub('telemetry', '{"deviceId":"esp32cadeiafria", value: 5');                       // JSON malformado
  await pub('telemetry', { deviceId: 'esp32cadeiafria', sensor: 'temperatura', value: 'quente', unit: 'C',
    sequence: 99, uptimeMs: 1, armed: true, ledOn: false });                                 // value não numérico
  await pub('telemetry', { deviceId: 'outro-device', sensor: 'temperatura', value: 4, unit: 'C',
    sequence: 1, uptimeMs: 1, armed: true, ledOn: false });                                  // deviceId diverge do tópico
  await pub('availability', 'talvez');                                                       // availability inválido
  await sleep(300);
  console.log('simulação concluída');
  c.end();
});
