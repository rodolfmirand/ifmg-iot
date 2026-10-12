// Camada de persistência Redis (Entrega 5)
// Ver KEYS.md para a documentação completa do modelo de chaves.
const { createClient } = require('redis');
const cfg = require('./config');

const NS = 'cadeiafria';
const K = {
  devices: () => `${NS}:devices`,                         // SET   — índice de dispositivos conhecidos
  state: (id) => `${NS}:device:${id}:state`,              // HASH  — estado atual (último snapshot)
  telemetry: (id) => `${NS}:device:${id}:telemetry`,      // ZSET  — histórico (score = epoch ms)
  presence: (id) => `${NS}:device:${id}:presence`,        // HASH  — status online/offline + lastSeen
  alive: (id) => `${NS}:device:${id}:alive`,              // STRING c/ TTL — heartbeat
  commands: (id) => `${NS}:device:${id}:commands`,        // LIST  — auditoria de comandos/confirmações
  logValid: () => `${NS}:log:valid`,                      // LIST  — diagnóstico (últimas 500)
  logInvalid: () => `${NS}:log:invalid`,                  // LIST  — diagnóstico (últimas 500)
};

const LOG_MAX = 500;
const CMD_MAX = 200;

let client;
async function connect() {
  client = createClient({ url: cfg.redis.url });
  client.on('error', (e) => console.error('[redis] erro:', e.message));
  await client.connect();
  return client;
}

// Qualquer mensagem válida do dispositivo conta como "contato"
function touchPresence(multi, id, now) {
  multi.sAdd(K.devices(), id);
  multi.hSet(K.presence(id), { lastSeen: String(now), lastSeenIso: new Date(now).toISOString() });
  multi.set(K.alive(id), String(now), { EX: cfg.presence.ttlSeconds });
}

async function saveTelemetry(id, msg, topic) {
  const now = Date.now();
  const record = { ...msg, receivedAt: now };
  const m = client.multi();
  touchPresence(m, id, now);
  m.hSet(K.state(id), {
    temperature: msg.value === null ? 'null' : String(msg.value),
    sensorOk: String(msg.value !== null),
    armed: String(msg.armed),
    ledOn: String(msg.ledOn),
    sequence: String(msg.sequence),
    uptimeMs: String(msg.uptimeMs),
    wifiRssi: String(msg.wifiRssi ?? ''),
    lastTelemetryAt: String(now),
    lastTelemetryIso: new Date(now).toISOString(),
    lastTopic: topic,
  });
  // Score = instante de recepção → consulta por janela de tempo com ZRANGEBYSCORE
  m.zAdd(K.telemetry(id), { score: now, value: JSON.stringify(record) });
  // Mantém só as N mais recentes (remove as mais antigas)
  m.zRemRangeByRank(K.telemetry(id), 0, -(cfg.history.maxEntries + 1));
  await m.exec();
}

async function saveActuatorState(id, msg) {
  const now = Date.now();
  const m = client.multi();
  touchPresence(m, id, now);
  // Estado confirmado pelo ESP32 é a fonte de verdade do atuador
  const field = msg.actuator === 'alarm' ? 'armed' : 'ledOn';
  m.hSet(K.state(id), {
    [field]: String(msg.state),
    [`${msg.actuator}ConfirmedAt`]: String(now),
    lastRequestId: msg.requestId ?? '',
  });
  m.lPush(K.commands(id), JSON.stringify({ kind: 'state', ...msg, receivedAt: now }));
  m.lTrim(K.commands(id), 0, CMD_MAX - 1);
  await m.exec();
}

// Comando é publicado por terceiros: NÃO conta como contato do dispositivo
async function saveCommand(id, msg) {
  const now = Date.now();
  await client.multi()
    .sAdd(K.devices(), id)
    .lPush(K.commands(id), JSON.stringify({ kind: 'command', ...msg, receivedAt: now }))
    .lTrim(K.commands(id), 0, CMD_MAX - 1)
    .exec();
}

async function saveAvailability(id, status) {
  const now = Date.now();
  const m = client.multi();
  m.sAdd(K.devices(), id);
  m.hSet(K.presence(id), { status, statusChangedAt: String(now), statusChangedIso: new Date(now).toISOString() });
  if (status === 'online') {
    m.hSet(K.presence(id), { lastSeen: String(now), lastSeenIso: new Date(now).toISOString() });
    m.set(K.alive(id), String(now), { EX: cfg.presence.ttlSeconds });
  } else {
    m.del(K.alive(id)); // LWT/offline derruba o heartbeat imediatamente
  }
  await m.exec();
}

async function logMessage(valid, entry) {
  const key = valid ? K.logValid() : K.logInvalid();
  await client.multi().lPush(key, JSON.stringify(entry)).lTrim(key, 0, LOG_MAX - 1).exec();
}

// ---------- Consultas ----------
async function listDevices() {
  return (await client.sMembers(K.devices())).sort();
}

async function getState(id) {
  const [state, presence, aliveTtl] = await Promise.all([
    client.hGetAll(K.state(id)),
    client.hGetAll(K.presence(id)),
    client.ttl(K.alive(id)),
  ]);
  // "online" efetivo = LWT diz online E heartbeat ainda não expirou
  const reachable = presence.status !== 'offline' && aliveTtl > 0;
  return { deviceId: id, state, presence: { ...presence, heartbeatTtl: aliveTtl, reachable } };
}

async function getHistory(id, { limit = 20, fromMs, toMs } = {}) {
  const raw = fromMs || toMs
    ? await client.zRangeByScore(K.telemetry(id), fromMs ?? '-inf', toMs ?? '+inf')
    : await client.zRange(K.telemetry(id), -limit, -1);
  return raw.map((r) => JSON.parse(r));
}

async function getHistoryCount(id) { return client.zCard(K.telemetry(id)); }
async function getCommands(id, limit = 10) {
  return (await client.lRange(K.commands(id), 0, limit - 1)).map((r) => JSON.parse(r));
}
async function getLogs(valid, limit = 10) {
  return (await client.lRange(valid ? K.logValid() : K.logInvalid(), 0, limit - 1)).map((r) => JSON.parse(r));
}

async function close() { if (client) await client.quit(); }

module.exports = {
  K, connect, close,
  saveTelemetry, saveActuatorState, saveCommand, saveAvailability, logMessage,
  listDevices, getState, getHistory, getHistoryCount, getCommands, getLogs,
};
