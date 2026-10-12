// Parsing de tópico + validação de payload (Entrega 4)
// Tópico: ifmg/iot3/<turma>/<aluno>/<dispositivo>/<tipo>
const TYPES = ['telemetry', 'command', 'state', 'availability'];
const ID_RE = /^[A-Za-z0-9_-]{1,64}$/;

function parseTopic(topic) {
  const p = topic.split('/');
  if (p.length !== 6 || p[0] !== 'ifmg' || p[1] !== 'iot3') {
    return { ok: false, error: 'tópico fora do padrão ifmg/iot3/<turma>/<aluno>/<dispositivo>/<tipo>' };
  }
  const [, , turma, aluno, deviceId, type] = p;
  if (!TYPES.includes(type)) return { ok: false, error: `tipo de tópico desconhecido: ${type}` };
  if (!ID_RE.test(deviceId)) return { ok: false, error: `deviceId inválido no tópico: ${deviceId}` };
  return { ok: true, turma, aluno, deviceId, type };
}

const isNum = (v) => typeof v === 'number' && Number.isFinite(v);
const isInt = (v) => Number.isInteger(v) && v >= 0;

function validateTelemetry(m, topicDeviceId) {
  const e = [];
  if (m.deviceId !== topicDeviceId) e.push(`deviceId do payload (${m.deviceId}) difere do tópico (${topicDeviceId})`);
  if (m.sensor !== 'temperatura') e.push('sensor deve ser "temperatura"');
  // null é válido: é o sinal de falha do DS18B20 definido na Entrega 3
  if (m.value !== null && !isNum(m.value)) e.push('value deve ser número ou null');
  if (isNum(m.value) && (m.value < -55 || m.value > 125)) e.push('value fora da faixa física do DS18B20 (-55..125 °C)');
  if (m.unit !== 'C') e.push('unit deve ser "C"');
  if (!isInt(m.sequence)) e.push('sequence deve ser inteiro >= 0');
  if (!isInt(m.uptimeMs)) e.push('uptimeMs deve ser inteiro >= 0');
  if (m.wifiRssi !== undefined && !(isNum(m.wifiRssi) && m.wifiRssi <= 0)) e.push('wifiRssi deve ser número <= 0');
  if (typeof m.armed !== 'boolean') e.push('armed deve ser boolean');
  if (typeof m.ledOn !== 'boolean') e.push('ledOn deve ser boolean');
  return e;
}

function validateState(m, topicDeviceId) {
  const e = [];
  if (m.deviceId !== topicDeviceId) e.push('deviceId do payload difere do tópico');
  if (!['alarm', 'led'].includes(m.actuator)) e.push('actuator deve ser "alarm" ou "led"');
  if (typeof m.state !== 'boolean') e.push('state deve ser boolean');
  if (m.requestId !== undefined && typeof m.requestId !== 'string') e.push('requestId deve ser string');
  return e;
}

function validateCommand(m) {
  const e = [];
  if (m.action !== 'set') e.push('action deve ser "set"');
  if (!['alarm', 'led'].includes(m.target)) e.push('target deve ser "alarm" ou "led"');
  if (typeof m.value !== 'boolean') e.push('value deve ser boolean');
  return e;
}

// Retorna { ok, topic, data, errors }
function validate(topic, raw) {
  const t = parseTopic(topic);
  if (!t.ok) return { ok: false, errors: [t.error] };

  const text = raw.toString('utf8').trim();

  // availability é texto puro ("online"/"offline"), não JSON
  if (t.type === 'availability') {
    if (text !== 'online' && text !== 'offline') return { ok: false, topic: t, errors: [`availability inválido: "${text}"`] };
    return { ok: true, topic: t, data: { status: text } };
  }

  let data;
  try { data = JSON.parse(text); } catch (err) {
    return { ok: false, topic: t, errors: [`JSON malformado: ${err.message}`] };
  }
  if (data === null || typeof data !== 'object' || Array.isArray(data)) {
    return { ok: false, topic: t, errors: ['payload deve ser um objeto JSON'] };
  }

  const errors =
    t.type === 'telemetry' ? validateTelemetry(data, t.deviceId)
    : t.type === 'state' ? validateState(data, t.deviceId)
    : validateCommand(data);

  return errors.length ? { ok: false, topic: t, data, errors } : { ok: true, topic: t, data };
}

module.exports = { validate, parseTopic };
