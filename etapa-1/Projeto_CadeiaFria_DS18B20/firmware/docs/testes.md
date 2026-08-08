# Registro de testes — Cadeia Fria IoT (Etapa 1)

Baseado no Anexo B da apostila. Preencha "Resultado obtido" e "Situação"
(OK / Falhou / Parcial) durante a execução real dos testes.

| ID | Cenário | Procedimento | Resultado esperado | Resultado obtido | Situação |
|---|---|---|---|---|---|
| T01 | Conexão Wi-Fi | Ligar o dispositivo e observar o monitor serial | ESP32 associa à rede, IP e RSSI exibidos | | |
| T02 | Perda do Wi-Fi | Desligar o roteador/AP por 30s e religar | Reconexão automática, sem reset manual | | |
| T03 | Conexão MQTT | Observar monitor serial após Wi-Fi conectado | Broker conectado, `availability = online` retained | | |
| T04 | Publicação de telemetria | Assinar `.../telemetry` no MQTT Explorer | JSON com temperatura a cada ~5s | | |
| T05 | Comando válido (armar) | Publicar `{"action":"set","target":"alarm","value":true,"requestId":"t05"}` | `armed=true` confirmado em `.../state` | | |
| T05b | Comando válido (desarmar) | Publicar `value:false` | `armed=false`, buzzer desliga se estava tocando | | |
| T06 | JSON inválido | Publicar payload malformado em `.../command` | Mensagem de erro no serial, nenhum estado alterado | | |
| T07 | Comando com target desconhecido | Publicar `target:"led"` | Comando rejeitado, log "não suportado" | | |
| T08 | Alarme automático — abaixo do limite | Resfriar o sensor abaixo de 2°C (armado) | Buzzer liga, `.../state` publicado com `requestId:"auto-limiar"` | | |
| T09 | Alarme automático — acima do limite | Aquecer o sensor acima de 8°C (armado) | Buzzer liga, estado publicado | | |
| T10 | Retorno à faixa segura | Temperatura volta para 2–8°C | Buzzer desliga, estado publicado | | |
| T11 | Recuperação do broker | Derrubar o broker e restaurar | ESP32 reconecta e reassina `.../command` | | |
| T12 | Last Will | Desligar o ESP32 abruptamente (sem MQTT disconnect) | Broker publica `availability = offline` | | |
