#include <RFM69registers.h>
#include <SPI.h>


// Energenie specific radio config values
#define RADIO_VAL_SYNCVALUE1FSK          0x2D  // 1st byte of Sync word
#define RADIO_VAL_SYNCVALUE2FSK          0xD4 // 2nd byte of Sync word
#define RADIO_VAL_PACKETCONFIG1FSKNO     0xA0  // Variable length, Manchester coding

#define HRF_MASK_WRITE_DATA            0x80

uint8_t CONFIG[][2] =
{
  {REG_DATAMODUL,       RF_DATAMODUL_MODULATIONTYPE_FSK},         // modulation scheme FSK
  {REG_FDEVMSB,            RF_FDEVMSB_30000},                // frequency deviation 5kHz 0x0052 -> 30kHz 0x01EC
  {REG_FDEVLSB,            RF_FDEVLSB_30000},                // frequency deviation 5kHz 0x0052 -> 30kHz 0x01EC
  {REG_FRFMSB,              RF_FRFMSB_434},                 // carrier freq -> 434.3MHz 0x6C9333
  {REG_FRFMID,              RF_FRFMID_434|0x13},                 // carrier freq -> 434.3MHz 0x6C9333
  {REG_FRFLSB,              RF_FRFLSB_434|0x33},                 // carrier freq -> 434.3MHz 0x6C9333
  {REG_AFCCTRL,            RF_AFCCTRL_LOWBETA_OFF},                 // standard AFC routine
  {REG_LNA,                RF_LNA_ZIN_200},                    // 200ohms, gain by AGC loop -> 50ohms
  {REG_RXBW,               RF_RXBW_DCCFREQ_010|RF_RXBW_EXP_3},                   // channel filter bandwidth 10kHz -> 60kHz  page:26
  {REG_BITRATEMSB,         RF_BITRATEMSB_4800},                             // 4800b/s
  {REG_BITRATELSB,         RF_BITRATELSB_4800},                             // 4800b/s
  {REG_SYNCCONFIG,         RF_SYNC_ON|RF_SYNC_SIZE_2},              // Size of the Synch word = 2 (SyncSize + 1)
  {REG_SYNCVALUE1,         RADIO_VAL_SYNCVALUE1FSK},          // 1st byte of Sync word
  {REG_SYNCVALUE2,         RADIO_VAL_SYNCVALUE2FSK},          // 2nd byte of Sync word
  {REG_PACKETCONFIG1,      RADIO_VAL_PACKETCONFIG1FSKNO},     // Variable length, Manchester coding
  {REG_PAYLOADLENGTH,         66},             // max Length in RX, not used in Tx
  {REG_NODEADRS,        0x06}, 
  { REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOOVERRUN }, // writing to this bit ensures that the FIFO & status flags are reset
  {255, 0}
};

byte _mode;
byte cs_pin;

void radio_init_spi(byte CS_PIN) {
  cs_pin = CS_PIN;
  unselect();
  pinMode(cs_pin, OUTPUT);
  SPI.begin();
}

void radio_reset(byte RESET_PIN) {
  // Hard Reset the RFM module
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  delay(100);
  digitalWrite(RESET_PIN, LOW);
  delay(100);
}

uint8_t readReg(uint8_t addr) {
  select();
  SPI.transfer(addr & 0x7F);
  uint8_t regval = SPI.transfer(0);
  unselect();
  return regval;
}

uint8_t radio_version() {
  return readReg(REG_VERSION);
}

void writeReg(uint8_t addr, uint8_t value)
{
  select();
  SPI.transfer(addr | 0x80);
  SPI.transfer(value);
  unselect();
}

void select() {
  noInterrupts();
  // set RFM69 SPI settings
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV4); // decided to slow down from DIV2 after SPI stalling in some instances, especially visible on mega1284p when RFM69 and FLASH chip both present
  digitalWrite(cs_pin, LOW);
}

// unselect the RFM69 transceiver (set CS high, restore SPI settings)
void unselect() {
  digitalWrite(cs_pin, HIGH);
  // restore SPI settings to what they were before talking to RFM69
  interrupts();
}

void radio_init_energenie_fsk() {
  for (uint8_t i = 0; CONFIG[i][0] != 255; i++) {
    writeReg(CONFIG[i][0], CONFIG[i][1]);
  }
  radio_receiver();
}

void radio_receiver() {
  Serial.println("Set receiver mode");
  _mode = RF_OPMODE_RECEIVER;
  writeReg(REG_OPMODE, RF_OPMODE_RECEIVER);
  writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_01);
}

void radio_transmitter() {
  Serial.println("Set transmitter mode");
  _mode = RF_OPMODE_TRANSMITTER;
  writeReg(REG_OPMODE, RF_OPMODE_TRANSMITTER);
  writeReg(REG_DIOMAPPING1, RF_DIOMAPPING1_DIO0_00);
}

void radio_init_interrupt(byte interruptPin) {
  pinMode(interruptPin, INPUT); 
  attachInterrupt(digitalPinToInterrupt(interruptPin), radio_interrupt, RISING);
}

void radio_interrupt() {
  byte irq2 = readReg(REG_IRQFLAGS2);
  Serial.print("Interrrupt ");
  Serial.print(_mode, HEX);
  Serial.print(" ");
  Serial.println(irq2, HEX);
  
  if (_mode == RF_OPMODE_RECEIVER && irq2 & RF_IRQFLAGS2_PAYLOADREADY) {
    uint8_t buf[66];
    memset(buf, 0, 66);
    byte len = read_fifo(buf);
    Serial.print("Received(");
    Serial.print(len);
    Serial.print(")");
    for (byte i=0; i<len; i++) {
      Serial.print(" 0x");
      Serial.print(buf[i], HEX);
    }
    Serial.println("");
  }
}

byte read_fifo(uint8_t* buf) {
    select();
    SPI.transfer(REG_FIFO & 0x7F);
    byte len = SPI.transfer(0);
    SPI.transfer(buf, len);
    unselect();
    return len;
}

bool radio_wait_ready() {
  return pollreg(REG_IRQFLAGS1, RF_IRQFLAGS1_MODEREADY, RF_IRQFLAGS1_MODEREADY, 50);
}

uint8_t radio_transmit(uint8_t *payload, uint8_t len, uint8_t times) {
  radio_transmitter();

  writeReg(REG_FIFOTHRESH, len-1);
  if (!pollreg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFONOTEMPTY, 0x00, 100)) {
    Serial.println("??");
    radio_receiver();
    return 0;
  }
  if (!pollreg(REG_IRQFLAGS1, RF_IRQFLAGS1_MODEREADY|RF_IRQFLAGS1_TXREADY, RF_IRQFLAGS1_MODEREADY|RF_IRQFLAGS1_TXREADY, 100)) {
    Serial.println("not ready");
    radio_receiver();
    return 0;
  }
  for (byte i=0; i<times; i++)
  {
    Serial.print("before");
    Serial.println(readReg(REG_IRQFLAGS2), HEX);
    writefifoburst(payload, len);
    // Tx will auto start when fifolevel is exceeded by loading the payload
    // so the level register must be correct for the size of the payload
    // otherwise transmit will never start.
    /* wait for FIFO to not exceed threshold level */
    Serial.print("after");
    Serial.println(readReg(REG_IRQFLAGS2), HEX);

    if (!pollreg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFOLEVEL, 0x00, 100)) {
      Serial.print("Only tranmitted x times: ");
      Serial.println(i);
      radio_receiver();
      return i;
    }

    Serial.print("wait");
    Serial.println(readReg(REG_IRQFLAGS2), HEX);
  }
  if (!pollreg(REG_IRQFLAGS2, RF_IRQFLAGS2_FIFONOTEMPTY, 0x00, 100*times)) {
    Serial.println("??");
    radio_receiver();
    return 0;
  }
  Serial.println(readReg(REG_IRQFLAGS2), HEX);
  radio_receiver();
  return times;
}

bool pollreg(byte addr, byte mask, byte expected, unsigned long timeout) {
  unsigned long start = millis();
  while (((readReg(addr) & mask) != expected) && millis()-start < timeout);
  if (millis()-start >= timeout) {
    return false;
  }
  return true;  
}

void writefifoburst(uint8_t *buf, uint8_t len) {
    select();
    SPI.transfer(REG_FIFO | HRF_MASK_WRITE_DATA);
    for (byte i=0; i < len; i++) {
      Serial.print(buf[i]);
      Serial.print(" ");
      SPI.transfer(buf[i]);
    }
//    SPI.transfer(buf, len); // This might write over the buf
    unselect();
}

