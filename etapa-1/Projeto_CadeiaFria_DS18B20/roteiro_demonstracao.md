# Roteiro de Demonstração — Monitor de Cadeia Fria (DS18B20 + Buzzer)

**Disciplina:** Internet das Coisas III — Etapa 1
**Instituição:** IFMG — Campus Ouro Branco
**Projeto:** Monitor de temperatura de cadeia fria com alarme remoto (DS18B20 à prova d'água + buzzer)

Este roteiro adapta o checklist de demonstração da apostila (Anexo C) ao projeto específico. Está organizado em três blocos: preparação, execução do fluxo principal e testes de robustez/falha.

> **Sensor:** DS18B20 à prova d'água, protocolo digital OneWire, conectado ao pino GPIO 4 (com resistor de pull-up de 4,7kΩ entre dados e 3,3V). Leitura já calibrada de fábrica, sem necessidade de equação de conversão no firmware.

---

## 1. Preparação (antes de ligar)

- [ ] Definir previamente duas faixas de teste: **normal** (2°C–8°C, faixa típica de conservação de vacina) e **fora da faixa** (abaixo de 2°C ou acima de 8°C).
- [ ] Conferir rapidamente o sensor antes da demonstração: com o dispositivo ligado e em repouso à temperatura ambiente, comparar a leitura exibida no monitor serial com um termômetro de referência (valor esperado ~20–25°C). O DS18B20 já vem calibrado de fábrica, então divergências grandes geralmente indicam problema de fiação (pull-up ausente ou fios trocados).
- [ ] Separar os elementos de simulação de temperatura:
  - **Aquecimento:** segurar a ponta da sonda entre os dedos por ~30s (eleva ~5–10°C rapidamente).
  - **Resfriamento:** encostar a sonda em gelo ou água gelada — a sonda utilizada é à prova d'água, então esse teste é seguro sem restrições adicionais.
- [ ] Abrir o cliente MQTT de desktop (MQTT Explorer ou equivalente) e assinar o wildcard `ifmg/iot3/<turma>/<aluno>/esp32/#`, para visualizar os 4 tópicos simultaneamente.
- [ ] Conferir que o `secrets.h` está preenchido com as credenciais corretas e que o firmware mais recente foi carregado no ESP32.

## 2. Execução — fluxo principal

| Passo | Ação | O que observar |
|---|---|---|
| 1 | Abrir o monitor serial e identificar o dispositivo | `DEVICE_ID` e mensagens de boot impressas |
| 2 | Ligar o ESP32 e aguardar conexão Wi-Fi | IP obtido, gateway e RSSI impressos no serial |
| 3 | Aguardar conexão MQTT | Publicação retida em `.../availability` = `"online"` (Last Will já configurado, ainda não disparado) |
| 4 | Observar telemetria no cliente desktop | Mensagens periódicas em `.../telemetry`; apontar os campos do JSON (`deviceId`, `value` em °C, `alarmArmed`, `alarmActive`, `sequence`, `uptimeMs`) e explicar por que `sequence`/`uptime` substituem timestamp real nesta etapa. A leitura do DS18B20 é digital e estável, sem ruído de ADC como em sensores analógicos |
| 5 | Aquecer o sensor com a mão até ultrapassar o limiar configurado | Buzzer dispara fisicamente; telemetria reflete o novo valor de temperatura |
| 6 | Enviar comando válido pelo cliente desktop no tópico `.../command` (ex.: `{"action":"set","target":"alarm","value":false,"requestId":"demo-01"}`) | Buzzer é silenciado/desarmado remotamente |
| 7 | Observar a confirmação publicada em `.../state` | Mesmo `requestId` do comando enviado — prova a comunicação bidirecional |

## 3. Testes de robustez e tratamento de falhas

- [ ] **JSON malformado:** enviar um payload com erro de sintaxe (ex.: vírgula sobrando) no tópico de comando e mostrar a mensagem de erro no monitor serial, sem travar o dispositivo.
- [ ] **Comando inválido:** enviar um comando com `target` desconhecido (ex.: `"target":"led"`) e mostrar que é rejeitado sem acionar nada.
- [ ] **Queda de Wi-Fi:** desligar o roteador (ou desativar a rede) por ao menos 30s e mostrar que o dispositivo fica indisponível — `.../availability` deve refletir `"offline"` via Last Will.
- [ ] **Recuperação:** restaurar a rede e mostrar reconexão automática sem reset manual, reassinatura do tópico de comando e retorno de `.../availability` para `"online"`.
- [ ] **Fechamento:** apresentar o diagrama de arquitetura, justificar os 4 tópicos utilizados e explicar por que o buzzer é um atuador seguro (baixa potência, sem carga externa conectada diretamente ao ESP32).

## Priorização em caso de tempo curto

Este roteiro cobre os itens do Anexo C em cerca de 6 a 8 minutos. Se o tempo de demonstração for reduzido, priorizar, nesta ordem:

1. Passos 4 a 7 do fluxo principal (comunicação bidirecional completa).
2. Um teste de falha — preferencialmente JSON inválido ou queda de rede.

Essa priorização segue o peso dos critérios da rubrica da apostila (seção 17): MQTT-publicação (20%), MQTT-comando (20%) e tratamento de falhas (10%) somam metade da nota prática.

## Tópicos MQTT utilizados

| Tópico | Direção | Conteúdo |
|---|---|---|
| `ifmg/iot3/<turma>/<aluno>/esp32/telemetry` | ESP32 → broker | Leitura periódica de temperatura (°C) |
| `ifmg/iot3/<turma>/<aluno>/esp32/command` | Gateway/cliente → ESP32 | Comando para armar/desarmar o buzzer |
| `ifmg/iot3/<turma>/<aluno>/esp32/state` | ESP32 → broker | Estado confirmado do buzzer após o comando |
| `ifmg/iot3/<turma>/<aluno>/esp32/availability` | ESP32 → broker | Indicação online/offline (retained + Last Will) |
