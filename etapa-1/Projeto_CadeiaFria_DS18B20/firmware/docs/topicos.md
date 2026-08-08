# Tabela de tópicos MQTT — Cadeia Fria IoT (Etapa 1)

Convenção da apostila (seção 7.3): `ifmg/iot3/<turma>/<aluno>/<dispositivo>/<sufixo>`

Os valores de `<turma>`, `<aluno>` e `<dispositivo>` são definidos em `config.h`
(`TURMA`, `ALUNO`, `DEVICE_ID`). Preencha `TURMA` e `ALUNO` com seus dados reais
antes da demonstração — os tópicos abaixo assumem os placeholders padrão.

| Tópico | Direção | Finalidade | QoS | Retained |
|---|---|---|---|---|
| `ifmg/iot3/<turma>/<aluno>/esp32/telemetry` | ESP32 → broker | Temperatura periódica (DS18B20) em JSON | 0 | não |
| `ifmg/iot3/<turma>/<aluno>/esp32/command` | Gateway/cliente → ESP32 | Armar/desarmar o alarme (buzzer) | 1 | não |
| `ifmg/iot3/<turma>/<aluno>/esp32/state` | ESP32 → broker | Estado confirmado do buzzer (manual ou automático) | 1 | sim |
| `ifmg/iot3/<turma>/<aluno>/esp32/availability` | ESP32 → broker | `online` / `offline` (Last Will) | 1 | sim |

## Mensagens

### Telemetria (`.../telemetry`)
```json
{
  "deviceId": "esp32",
  "sensor": "temperatura",
  "value": 5.4,
  "unit": "C",
  "minSafeC": 2.0,
  "maxSafeC": 8.0,
  "alarmArmed": true,
  "alarmActive": false,
  "sequence": 42,
  "uptimeMs": 128500,
  "wifiRssi": -61
}
```

### Comando (`.../command`)
```json
{
  "action": "set",
  "target": "alarm",
  "value": true,
  "requestId": "cmd-2026-001"
}
```
`value: true` arma o alarme (buzzer soa automaticamente fora da faixa segura).
`value: false` desarma (buzzer nunca soa até novo comando de armar).

### Estado confirmado (`.../state`)
```json
{
  "deviceId": "esp32",
  "actuator": "buzzer",
  "state": false,
  "armed": true,
  "requestId": "cmd-2026-001"
}
```
Publicado sempre que o buzzer muda de estado — por comando (`requestId` do
comando recebido), por limiar automático (`requestId: "auto-limiar"`) ou na
inicialização (`requestId: "startup"`).

### Disponibilidade (`.../availability`)
Payload simples: `"online"` (retained, publicado após conectar) ou `"offline"`
(Last Will, publicado pelo broker se a conexão cair inesperadamente).
