# Lista de Materiais — Monitor de Cadeia Fria (NTC 10k B3950 + Buzzer)

**Disciplina:** Internet das Coisas III — Etapa 1
**Instituição:** IFMG — Campus Ouro Branco
**Projeto:** Monitor de temperatura de cadeia fria com alarme remoto (termistor NTC 10k B3950 à prova d'água + buzzer)

## Hardware

- [ ] 1x ESP32 DevKit (placa de desenvolvimento com Wi-Fi integrado).
- [ ] 1x termistor NTC 10k, coeficiente Beta 3950, sonda à prova d'água.
- [ ] 1x resistor de 10kΩ (par do divisor de tensão com o NTC — mesma faixa nominal do sensor a 25°C).
- [ ] 1x buzzer ativo 5V (emite som sozinho ao receber tensão, sem precisar de sinal PWM).
- [ ] 1x resistor de 220Ω–330Ω (proteção em série com o buzzer — opcional, mas recomendado).
- [ ] 1x protoboard (breadboard) pequena/média.
- [ ] Jumpers macho-macho (ligações na protoboard) e macho-fêmea (ligação direta nos pinos do ESP32).
- [ ] 1x cabo USB (micro-USB ou USB-C, conforme o modelo da placa) para alimentação e upload do firmware.
- [ ] Computador com porta USB disponível.

## Software

- [ ] Arduino IDE (ou ambiente equivalente aprovado pelo professor).
- [ ] Pacote de placas ESP32 instalado na Arduino IDE.
- [ ] Biblioteca **PubSubClient** (cliente MQTT).
- [ ] Biblioteca **ArduinoJson** (montagem/leitura dos payloads JSON).
- [ ] Cliente MQTT de desktop para testes manuais (MQTT Explorer ou equivalente).

## Infraestrutura fornecida pela disciplina

- [ ] Rede Wi-Fi de teste (SSID/senha).
- [ ] Broker MQTT acessível (host, porta, usuário e senha fornecidos pelo professor).

## Observações

- Não é necessário resistor pull-up externo para o NTC se a sonda já vier com resistência bem definida — mas vale medir com multímetro antes de montar, para confirmar o valor real do resistor série e calibrar a equação de Beta no firmware.
- O buzzer ativo dispensa cálculo de frequência/PWM (basta `digitalWrite` HIGH/LOW), o que simplifica o código em relação a um buzzer passivo.
- Custo total estimado: baixo — todos os itens de hardware são componentes de prototipagem comuns, sem necessidade de peças importadas ou de alta potência.
