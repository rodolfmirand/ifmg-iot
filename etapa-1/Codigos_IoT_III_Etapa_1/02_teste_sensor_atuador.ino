const int SENSOR_PIN = 34;
const int ACTUATOR_PIN = 2;

void setup() {
  Serial.begin(115200);
  pinMode(ACTUATOR_PIN, OUTPUT);
  digitalWrite(ACTUATOR_PIN, LOW);
}

void loop() {
  int sensorValue = analogRead(SENSOR_PIN);
  Serial.print("Valor do sensor: ");
  Serial.println(sensorValue);

  bool activate = sensorValue > 2000;
  digitalWrite(ACTUATOR_PIN, activate ? HIGH : LOW);

  delay(1000);
}
