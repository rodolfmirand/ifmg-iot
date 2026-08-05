# Códigos — Internet das Coisas III — Etapa 1

Este pacote reúne os códigos e exemplos apresentados na apostila da Etapa 1: Dispositivo, conectividade Wi-Fi e comunicação MQTT.

## Ordem sugerida de estudo

1. `01_secrets.h` — modelo para credenciais de Wi-Fi e broker MQTT.
2. `02_teste_sensor_atuador.ino` — validação local do sensor e do atuador.
3. `03_wifi_reconexao.ino` — conexão Wi-Fi, diagnóstico e tentativas periódicas de reconexão.
4. `04_mqtt_conexao_last_will.ino` — conexão ao broker MQTT e configuração de Last Will.
5. `05_publicacao_telemetria.ino` — função de publicação periódica de telemetria em JSON.
6. `06_callback_comando.ino` — recebimento de comando MQTT, validação JSON e confirmação do estado do atuador.
7. `07_programa_integrado.ino` — exemplo completo integrando Wi-Fi, MQTT, telemetria, comando e reconexão.
8. `08_codigo_para_analise.ino` — trecho propositalmente limitado usado em exercício de análise.

## Arquivos de apoio

- `exemplo_telemetria.json` — estrutura de mensagem de telemetria.
- `exemplo_comando.json` — estrutura de comando para o atuador.
- `exemplo_estado_atuador.json` — confirmação do estado do atuador.
- `topicos_mqtt.txt` — convenção de tópicos usada na apostila.

## Observações importantes

- Substitua os identificadores de turma, aluno e dispositivo pelos valores definidos para o projeto individual.
- Nunca publique credenciais reais em repositórios públicos. Use `secrets.h` local e adicione-o ao `.gitignore`.
- Alguns arquivos (`04`, `05` e `06`) representam blocos didáticos que dependem de variáveis e funções apresentadas no contexto da apostila. Para uma referência executável consolidada, use `07_programa_integrado.ino`.
- Os exemplos consideram ESP32, Arduino IDE, PubSubClient e ArduinoJson conforme descrito na apostila.
