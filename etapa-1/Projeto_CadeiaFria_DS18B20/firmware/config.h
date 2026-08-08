#ifndef CONFIG_H
#define CONFIG_H

// =====================================================================
// config.h — Parâmetros do projeto individual (Cadeia Fria IoT)
// IoT III — Etapa 1 (IFMG — Campus Ouro Branco)
//
// Ajuste os valores marcados com "SUA_" / "SEU_" antes da demonstração
// e da entrega. Os tópicos MQTT seguem a convenção da apostila:
//   ifmg/iot3/<turma>/<aluno>/<dispositivo>/telemetry
//   ifmg/iot3/<turma>/<aluno>/<dispositivo>/command
//   ifmg/iot3/<turma>/<aluno>/<dispositivo>/state
//   ifmg/iot3/<turma>/<aluno>/<dispositivo>/availability
// =====================================================================

// ---------------------------------------------------------------------
// Identificação do projeto (obrigatório pela apostila, seção 3.3 e 15):
// tópicos exclusivos por turma, aluno e dispositivo.
// TODO: preencha com seus dados reais antes da entrega.
// ---------------------------------------------------------------------
#define TURMA      "SUA_TURMA"
#define ALUNO      "SEU_USUARIO"
#define DEVICE_ID  "esp32"

// Prefixo base e tópicos completos (concatenação de literais em tempo
// de compilação — não altere esta parte, apenas os campos acima).
#define TOPIC_BASE         "ifmg/iot3/" TURMA "/" ALUNO "/" DEVICE_ID "/"
#define TOPIC_TELEMETRY    TOPIC_BASE "telemetry"
#define TOPIC_COMMAND      TOPIC_BASE "command"
#define TOPIC_STATE        TOPIC_BASE "state"
#define TOPIC_AVAILABILITY TOPIC_BASE "availability"

// ---------------------------------------------------------------------
// Hardware — Cadeia Fria (sensor DS18B20 à prova d'água + buzzer ativo)
// ---------------------------------------------------------------------
#define ONE_WIRE_BUS_PIN  4   // GPIO4: barramento OneWire do DS18B20
#define BUZZER_PIN        2   // GPIO2: buzzer ativo (digitalWrite HIGH/LOW)
#define BUZZER_ACTIVE_HIGH true  // buzzer liga em HIGH; inverta se o seu módulo for ativo em LOW

// Resolução do DS18B20 (9 a 12 bits). 12 bits = mais precisão, mais tempo
// de conversão (até ~750 ms). A leitura é feita de forma não bloqueante.
#define DS18B20_RESOLUTION_BITS 12

// ---------------------------------------------------------------------
// Limiares de temperatura da cadeia fria (ex.: transporte de vacinas)
// Fora da faixa [TEMP_MIN_C, TEMP_MAX_C] o alarme soa automaticamente,
// desde que esteja armado (ver lógica de "armar/desarmar" via MQTT).
// ---------------------------------------------------------------------
#define TEMP_MIN_C 2.0f
#define TEMP_MAX_C 8.0f

// Valor de erro retornado pela DallasTemperature quando a leitura falha
// (sensor desconectado, curto, etc.). Usado para não disparar falso alarme.
#define TEMP_ERROR_C -127.0f

// ---------------------------------------------------------------------
// Temporizações (todas não bloqueantes, controladas por millis())
// ---------------------------------------------------------------------
#define WIFI_RETRY_MS            5000UL
#define MQTT_RETRY_MS             5000UL
#define TELEMETRY_INTERVAL_MS     5000UL   // leitura + publicação periódica
#define DS18B20_CONVERSION_MS      750UL   // tempo máximo de conversão em 12 bits

#endif // CONFIG_H
