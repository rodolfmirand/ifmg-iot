# Gateway MQTT → Redis — Cadeia Fria IoT

Gateway Node.js (Entregas 4 e 5): assina os tópicos do projeto, identifica o dispositivo pelo tópico, valida JSON, registra mensagens válidas/inválidas e persiste estado, histórico e presença no Redis.

## Estrutura

| Arquivo | Função |
|---|---|
| `gateway.js` | Cliente MQTT, roteamento por tipo de tópico, logs |
| `validator.js` | Parsing do tópico + validação de schema por tipo |
| `store.js` | Persistência e consultas Redis |
| `query.js` | CLI de consulta |
| `config.js` | Configuração via variáveis de ambiente |
| `KEYS.md` | Documentação do modelo de chaves |
| `tools/simulate-esp32.js` | Simula o ESP32 (+ mensagens inválidas) |
| `tools/local-broker.js` | Broker MQTT local para teste offline |

## Pré-requisitos

- Node.js 18+
- Redis 6+ — Windows: `docker run -d --name redis -p 6379:6379 redis:7` (ou Memurai / WSL). Linux: `sudo apt install redis-server`.

## Execução

```bash
npm install
npm start                                   # gateway no broker.hivemq.com
npm run query -- state esp32cadeiafria      # consultas (outro terminal)
npm run query -- history esp32cadeiafria 20
npm run query -- window esp32cadeiafria 10
npm run query -- commands esp32cadeiafria
npm run query -- invalid 5
```

Variáveis: `MQTT_URL`, `MQTT_TOPIC`, `REDIS_URL`, `HISTORY_MAX`, `PRESENCE_TTL`.

## Teste sem o ESP32

```bash
npm run broker                                              # terminal 1
MQTT_URL=mqtt://localhost:1883 npm start                    # terminal 2
MQTT_URL=mqtt://localhost:1883 npm run simulate             # terminal 3
```

## Validações aplicadas

Tópico fora do padrão `ifmg/iot3/<turma>/<aluno>/<dispositivo>/<tipo>`; tipo desconhecido; JSON malformado ou não-objeto; `deviceId` do payload diferente do tópico; campos ausentes ou com tipo errado; temperatura fora da faixa física do DS18B20 (−55..125 °C); `availability` diferente de `online`/`offline`. `value: null` é **aceito** (sinal de falha de leitura definido na Entrega 3) e gravado como `sensorOk=false`.

## Limitações

- Broker público sem TLS/autenticação: qualquer um pode publicar nos tópicos; a validação mitiga dados malformados, mas não falsificação.
- O timestamp do histórico é o de **recepção no gateway** (o ESP32 não tem RTC/NTP; só `uptimeMs`). Atrasos de rede deslocam levemente o horário.
- QoS 1 pode entregar duplicatas; não há deduplicação por `sequence` (aceitável para telemetria).
