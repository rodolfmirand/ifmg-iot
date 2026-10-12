# Modelo de chaves Redis — Cadeia Fria IoT

Padrão: `cadeiafria:device:<deviceId>:<aspecto>`. O prefixo `cadeiafria:` isola o projeto (namespace) e o `<deviceId>` — extraído do **tópico MQTT** e conferido contra o campo `deviceId` do payload — distingue cada dispositivo.

| Chave | Tipo | Conteúdo | Escrita | Retenção |
|---|---|---|---|---|
| `cadeiafria:devices` | SET | IDs de todos os dispositivos já vistos | qualquer mensagem válida | permanente |
| `cadeiafria:device:<id>:state` | HASH | **Estado atual**: `temperature`, `sensorOk`, `armed`, `ledOn`, `sequence`, `uptimeMs`, `wifiRssi`, `lastTelemetryAt`, `alarmConfirmedAt`, `lastRequestId` | `telemetry` e `state` | sobrescrito (só o último) |
| `cadeiafria:device:<id>:telemetry` | ZSET | **Histórico**: cada membro é o JSON da leitura + `receivedAt`; score = epoch ms da recepção | `telemetry` | últimas 10.000 (`HISTORY_MAX`) |
| `cadeiafria:device:<id>:presence` | HASH | **Presença**: `status` (online/offline, vindo do LWT), `statusChangedAt`, `lastSeen` (último contato) | `availability`, `telemetry`, `state` | sobrescrito |
| `cadeiafria:device:<id>:alive` | STRING + TTL | heartbeat; expira 60 s após o último contato (`PRESENCE_TTL`) | idem `lastSeen` | TTL |
| `cadeiafria:device:<id>:commands` | LIST | auditoria de comandos e confirmações (`kind: command\|state`) | `command`, `state` | últimas 200 |
| `cadeiafria:log:valid` / `cadeiafria:log:invalid` | LIST | diagnóstico do gateway (inválidas guardam payload + erros) | toda mensagem | últimas 500 |

## Decisões de modelagem

- **HASH para estado atual:** leitura O(1) de um campo (`HGET ... armed`) ou do snapshot todo (`HGETALL`), sem desserializar JSON.
- **ZSET para histórico (em vez de LIST):** o score temporal permite consulta por **janela de tempo** (`ZRANGEBYSCORE key <de> <até>`), essencial para responder "a temperatura saiu da faixa entre 2h e 4h?". LIST só permite acesso por posição. Trade-off: ZSET usa mais memória por elemento.
  - Alternativa considerada: **Redis Streams** (`XADD`), também temporal e mais idiomático para log de eventos. Não adotado para manter compatibilidade com Redis antigo e simplicidade de consulta; é a evolução natural se surgirem múltiplos consumidores.
- **Presença com duas fontes:** `status` vem do LWT (o broker avisa queda anormal), mas o LWT só dispara após ~1,5× o keepalive e não cobre gateway que perdeu a conexão. O `alive` com TTL é uma segunda verificação independente. A consulta considera o dispositivo **alcançável** só se `status != offline` **e** o TTL não expirou.
- **Comando não conta como contato:** `command` é publicado por terceiros; registrá-lo como `lastSeen` mascararia um ESP32 morto.
- **`state` é a fonte de verdade do atuador:** sobrescreve `armed`/`ledOn` no HASH, porque é a confirmação de execução (Entrega 3, Seção 5).
- **Escritas atômicas (`MULTI/EXEC`):** estado, histórico e presença de uma mesma mensagem são gravados juntos — nunca há estado novo com histórico velho.
- **Retenção limitada (`ZREMRANGEBYRANK`):** evita crescimento ilimitado de memória.

## Consultas equivalentes em `redis-cli`

```bash
SMEMBERS cadeiafria:devices
HGETALL  cadeiafria:device:esp32cadeiafria:state
HGETALL  cadeiafria:device:esp32cadeiafria:presence
TTL      cadeiafria:device:esp32cadeiafria:alive
ZRANGE   cadeiafria:device:esp32cadeiafria:telemetry -5 -1          # últimas 5
ZCARD    cadeiafria:device:esp32cadeiafria:telemetry                # total
ZRANGEBYSCORE cadeiafria:device:esp32cadeiafria:telemetry <ms_inicio> +inf
LRANGE   cadeiafria:log:invalid 0 4
```
