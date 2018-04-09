#include <DigiCDC.h>
#include <ubrb.h>

#define LED 1 //LED on Model A

#define bankNum 1
#define bankSize 100

int read(uint8_t *c) {
  if (SerialUSB.available())
     *c = SerialUSB.read();
  else
    return 0;

  return 1;  
}

void write(uint8_t c) {
  SerialUSB.write(c);
}

void led(uint8_t v) {
  digitalWrite(LED, v ? HIGH : LOW); 
}

void doDelay(uint16_t ms) {
  SerialUSB.delay(ms);
}

struct ubrb ubrb = {
  .ops = {
    .readByte = read,
    .writeByte = write,
    .rng = {
      .getByte = NULL
    },
    .leds = {
      .setLED = led
    },
    .delay = doDelay
  },
  .activeBank = NULL,
  .banks = {
    .num = bankNum,
    .size = bankSize,
    .bank = NULL
  },
  .state = idle
};

void setup() {
  // LEDs configuration
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  SerialUSB.begin();

  // allocalte banks
  ubrb.banks.bank = (uint8_t **)malloc(sizeof(uint8_t *) * ubrb.banks.num);
  for (int i = 0; i < bankNum; ++i) {
    ubrb.banks.bank[i] = (uint8_t *)malloc(ubrb.banks.size);
    memset(ubrb.banks.bank[i], 0, bankSize);
  }

  // write "Hello world!" in bank zero
  static const char str[] = "Hello world!\0";
  if (bankSize >= sizeof(str))
    memcpy(ubrb.banks.bank[0], str, sizeof(str));

  digitalWrite(LED, LOW);
}

void loop() {
  ubrb_tick(&ubrb);
  SerialUSB.delay(100);
}
