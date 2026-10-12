// Configuração centralizada — tudo sobrescrevível por variável de ambiente
module.exports = {
  mqtt: {
    url: process.env.MQTT_URL || 'mqtt://broker.hivemq.com:1883',
    // Assina somente a árvore do aluno; '+' no lugar do dispositivo permite N ESP32
    subscribe: process.env.MQTT_TOPIC || 'ifmg/iot3/2026-1/rodolfo/+/#',
    clientId: process.env.MQTT_CLIENT_ID || `gw-cadeiafria-${Math.random().toString(16).slice(2, 8)}`,
  },
  redis: {
    url: process.env.REDIS_URL || 'redis://127.0.0.1:6379',
  },
  history: {
    maxEntries: Number(process.env.HISTORY_MAX || 10000), // limite por dispositivo
  },
  presence: {
    ttlSeconds: Number(process.env.PRESENCE_TTL || 60), // 4x o intervalo de telemetria (15 s)
  },
};
