# Cadeia Fria IoT — ESP32 (IoT III, Etapa 1)

Dispositivo ESP32 que monitora a temperatura de uma cadeia fria (ex.:
transporte de vacinas) com um sensor DS18B20 à prova d'água e aciona um
buzzer como alarme local quando a temperatura sai da faixa segura. Suporta
controle remoto (armar/desarmar) via MQTT, seguindo a arquitetura e a
convenção de tópicos da apostila da disciplina Internet das Coisas III
(IFMG — Campus Ouro Branco).

Ver também, na pasta do projeto (`..`): `lista_de_materiais.md`,
`roteiro_demonstracao.md` e o diagrama de arquitetura.

## Hardware

| Componente | Pino no ESP32 | Observação |
|---|---|---|
| DS18B20 (dado, OneWire) | GPIO4 | Resistor de pull-up de 4,7 kΩ entre dado e 3,3 V |
| DS18B20 (alimentação) | 3,3 V / GND | Versão à prova d'água, 3 fios |
| Buzzer ativo | GPIO2 | Liga em nível alto (ajustável em `config.h`) |

## Bibliotecas necessárias

Instale pelo Gerenciador de Bibliotecas da Arduino IDE:
- **PubSubClient** (Nick O'Leary)
- **ArduinoJson** (Benoit Blanchon), versão 7.x
- **OneWire** (Paul Stoffregen / Jim Studt)
- **DallasTemperature** (Miles Burton)

Certifique-se de que o pacote de placas **ESP32** está instalado e que a
placa/porta corretas estão selecionadas.

## Configuração

1. Edite `secrets.h` (já presente como modelo, idêntico a
   `secrets.example.h`) com o SSID, a senha do Wi-Fi e os dados do broker
   MQTT fornecidos pelo professor.
2. Abra `config.h` e ajuste:
   - `TURMA` e `ALUNO` — usados para montar os tópicos MQTT exclusivos
     (`ifmg/iot3/<turma>/<aluno>/esp32/...`), conforme exigido pela apostila.
   - Pinos (`ONE_WIRE_BUS_PIN`, `BUZZER_PIN`) e polaridade do buzzer, se o
     seu módulo for diferente do padrão.
   - Faixa segura de temperatura (`TEMP_MIN_C` / `TEMP_MAX_C`), hoje
     configurada para 2–8 °C (padrão de vacinas).
3. **Nunca** publique `secrets.h` em repositórios públicos — ele já está
   listado em `.gitignore`.

## Compilação e upload

1. Abra `projeto_cadeia_fria_etapa1.ino` na Arduino IDE (os arquivos
   `config.h` e `secrets.h` devem estar na mesma pasta).
2. Selecione a placa ESP32 e a porta serial corretas.
3. Compile e faça upload.
4. Abra o Monitor Serial em 115200 bps.

## Comportamento esperado

- O ESP32 conecta ao Wi-Fi e tenta reconectar a cada 5 s em caso de queda,
  sem necessidade de reset manual.
- Ao conectar ao broker MQTT, publica `online` (retained) em
  `.../availability` e configura Last Will `offline` para desconexões
  inesperadas.
- A cada ~5 s, lê o DS18B20 (leitura não bloqueante) e publica telemetria
  JSON em `.../telemetry`.
- Se o alarme estiver armado (`alarmArmed = true`, padrão inicial) e a
  temperatura sair da faixa 2–8 °C, o buzzer liga automaticamente e o
  estado é confirmado (retained) em `.../state`.
- Comandos MQTT em `.../command` no formato
  `{"action":"set","target":"alarm","value":true|false,"requestId":"..."}`
  armam ou desarmam o alarme remotamente; o estado resultante do buzzer é
  sempre confirmado em `.../state`.
- JSON inválido ou comandos com `action`/`target` desconhecidos são
  rejeitados e registrados no monitor serial, sem alterar o estado.
- Após qualquer reconexão MQTT, o tópico de comando é reassinado
  automaticamente.

## Estrutura desta pasta

```
firmware/
├── projeto_cadeia_fria_etapa1.ino
├── config.h
├── secrets.h            (não versionar)
├── secrets.example.h
├── .gitignore
├── README.md
└── docs/
    ├── topicos.md
    └── testes.md
```

## Próximos passos (fora do escopo da Etapa 1)

- Testar com o broker MQTT definitivo fornecido pelo professor.
- Montar o circuito físico assim que os componentes chegarem.
- Registrar evidências (capturas do monitor serial e do cliente MQTT) na
  pasta `evidencias/` sugerida pela apostila.
- Integrar com o gateway em nuvem e o banco de dados nas etapas seguintes.
