#define SERIAL_BAUD   115200

#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif

String inputString = "";         // a string to hold incoming data

void serial_init() {
  while (!Serial);
  Serial.begin(SERIAL_BAUD);
}

void serialEventRun(void) {
  if (Serial.available()) serialEvent();
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\r') {
      blink(40, 3);
      Serial.print("Received: ");
      Serial.println(inputString);
      inputString = "";

      
    uint8_t payload[] = {13, 4, 2, 1, 0, 0, 26, 55, 243, 1, 0, 0, 196, 29}; // off
//    uint8_t payload[] = {13, 4, 2, 1, 0, 0, 26, 55, 243, 1, 1, 0, 247, 44}; // on
    radio_transmit(payload, 14, 10);
    Serial.println("Transmitted something");
    }
  }
}


