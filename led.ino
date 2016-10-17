byte default_led;

void led_init(byte PIN) {
  default_led = PIN;
  pinMode(PIN, OUTPUT);
  blink(40, 1);
}

void blink(byte DELAY_MS, byte loops)
{
  for (byte i=0; i<loops; i++)
  {
    digitalWrite(default_led, HIGH);
    delay(DELAY_MS);
    digitalWrite(default_led, LOW);
    delay(DELAY_MS);
  }
}
