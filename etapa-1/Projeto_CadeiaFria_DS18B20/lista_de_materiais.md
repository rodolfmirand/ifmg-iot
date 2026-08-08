# Lista de Materiais — Monitor de Cadeia Fria (DS18B20 + Buzzer)

**Disciplina:** Internet das Coisas III — Etapa 1
**Instituição:** IFMG — Campus Ouro Branco
**Projeto:** Monitor de temperatura de cadeia fria com alarme remoto (DS18B20 à prova d'água + buzzer)

## Hardware

- [ ] 1x ESP32 DevKit (placa de desenvolvimento com Wi-Fi integrado).
- [ ] 1x sensor DS18B20 à prova d'água (sonda com 3 fios: dados, VCC, GND).
- [ ] 1x resistor de pull-up de 4,7kΩ entre o pino de dados e 3,3V (obrigatório no barramento OneWire).
- [X] 1x buzzer ativo 5V (emite som sozinho ao receber tensão, sem precisar de sinal PWM).
- [ ] 1x resistor de 220Ω–330Ω (proteção em série com o buzzer — opcional, mas recomendado).
- [ ] 1x protoboard (breadboard) pequena/média.
- [X] Jumpers macho-macho (ligações na protoboard) e macho-fêmea (ligação direta nos pinos do ESP32).
- [X] 1x cabo USB (micro-USB ou USB-C, conforme o modelo da placa) para alimentação e upload do firmware.
- [X] Computador com porta USB disponível.

## Software

- [X] Arduino IDE (ou ambiente equivalente aprovado pelo professor).
- [X] Pacote de placas ESP32 instalado na Arduino IDE.
- [X] Biblioteca **PubSubClient** (cliente MQTT).
- [X] Biblioteca **ArduinoJson** (montagem/leitura dos payloads JSON), versão 7.x.
- [X] Biblioteca **OneWire** (comunicação com o barramento do DS18B20).
- [X] Biblioteca **DallasTemperature** (leitura de temperatura do DS18B20).
- [ ] Cliente MQTT de desktop para testes manuais (MQTT Explorer ou equivalente).

## Infraestrutura fornecida pela disciplina

- [ ] Rede Wi-Fi de teste (SSID/senha).
- [ ] Broker MQTT acessível (host, porta, usuário e senha fornecidos pelo professor).

## Observações

- O DS18B20 é digital (protocolo OneWire) e já vem calibrado de fábrica, dispensando equações de calibração como as usadas em termistores NTC — simplifica o firmware e reduz erro de leitura.
- O resistor de pull-up de 4,7kΩ entre o pino de dados e 3,3V é obrigatório para o barramento OneWire funcionar; sem ele a leitura falha ou retorna valores inválidos.
- O buzzer ativo dispensa cálculo de frequência/PWM (basta `digitalWrite` HIGH/LOW), o que simplifica o código em relação a um buzzer passivo.
- Custo total estimado: baixo — todos os itens de hardware são componentes de prototipagem comuns, sem necessidade de peças importadas ou de alta potência.
