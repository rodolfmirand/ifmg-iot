// Gateway MQTT → Redis — Cadeia Fria IoT (Entregas 4 e 5)
const mqtt = require('mqtt');
const cfg = require('./config');
const store = require('./store');
const { validate } = require('./validator');

const ts = () => new Date().toISOString();
const stats = { valid: 0, invalid: 0, persistErrors: 0 };

async function handle(topic, payload) {
  const raw = payload.toString('utf8');
  const r = validate(topic, payload);

  if (!r.ok) {
    stats.invalid++;
    console.warn(`${ts()} [INVÁLIDA] ${topic} → ${r.errors.join('; ')} | payload=${raw.slice(0, 200)}`);
    await store.logMessage(false, { at: Date.now(), topic, payload: raw.slice(0, 500), errors: r.errors });
    return;
  }

  const { deviceId, type } = r.topic;
  try {
    switch (type) {
      case 'telemetry':
        await store.saveTelemetry(deviceId, r.data, topic);
        console.log(`${ts()} [OK] ${deviceId} telemetry seq=${r.data.sequence} T=${r.data.value}°C armed=${r.data.armed} led=${r.data.ledOn}`);
        break;
      case 'state':
        await store.saveActuatorState(deviceId, r.data);
        console.log(`${ts()} [OK] ${deviceId} state ${r.data.actuator}=${r.data.state} req=${r.data.requestId ?? '-'}`);
        break;
      case 'command':
        await store.saveCommand(deviceId, r.data);
        console.log(`${ts()} [OK] ${deviceId} command ${r.data.target}=${r.data.value} req=${r.data.requestId ?? '-'}`);
        break;
      case 'availability':
        await store.saveAvailability(deviceId, r.data.status);
        console.log(`${ts()} [OK] ${deviceId} availability=${r.data.status}`);
        break;
    }
    stats.valid++;
    await store.logMessage(true, { at: Date.now(), topic, deviceId, type });
  } catch (err) {
    stats.persistErrors++;
    console.error(`${ts()} [ERRO REDIS] ${topic}: ${err.message}`);
  }
}

async function main() {
  await store.connect();
  console.log(`${ts()} [redis] conectado em ${cfg.redis.url}`);

  const client = mqtt.connect(cfg.mqtt.url, {
    clientId: cfg.mqtt.clientId,
    keepalive: 30,
    reconnectPeriod: 5000,
    clean: true,
  });

  client.on('connect', () => {
    console.log(`${ts()} [mqtt] conectado em ${cfg.mqtt.url} como ${cfg.mqtt.clientId}`);
    client.subscribe(cfg.mqtt.subscribe, { qos: 1 }, (err) => {
      if (err) console.error(`${ts()} [mqtt] falha ao assinar: ${err.message}`);
      else console.log(`${ts()} [mqtt] assinado: ${cfg.mqtt.subscribe}`);
    });
  });
  client.on('reconnect', () => console.log(`${ts()} [mqtt] reconectando...`));
  client.on('close', () => console.log(`${ts()} [mqtt] conexão fechada`));
  client.on('error', (e) => console.error(`${ts()} [mqtt] erro: ${e.message}`));

  // Processamento serializado: preserva a ordem de chegada no Redis
  let chain = Promise.resolve();
  client.on('message', (topic, payload) => {
    chain = chain.then(() => handle(topic, payload)).catch((e) => console.error(e));
  });

  const shutdown = async () => {
    console.log(`\n${ts()} encerrando — válidas=${stats.valid} inválidas=${stats.invalid} errosRedis=${stats.persistErrors}`);
    client.end(true);
    await chain;
    await store.close();
    process.exit(0);
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}

main().catch((e) => { console.error('Falha fatal:', e); process.exit(1); });
