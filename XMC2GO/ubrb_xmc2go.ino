#include <ubrb.h>

#define LED1 0x0e
#define LED2 0x0f
#define LED LED2

#define bankSize 64
#define bankNum 10

void send(uint8_t c) {
  Serial.write(c);
}

int receive(uint8_t *c) {
  if (Serial.available())
    *c = Serial.read();
  else
    return 0;

  return 1;
}

void led(uint8_t v) {
  digitalWrite(LED, v ? HIGH : LOW);
}

struct ubrb_leds leds = { .setLED = led };
struct ubrb_rng rng = {0};
struct ubrb_ops ops = {
      .readByte = receive,
      .writeByte = send,
      .rng = rng,
      .leds = leds
};
struct ubrb_banks banks =  {
      .num = bankNum,
      .size = bankSize,
      .bank = NULL
};
static struct ubrb ubrb = {
  .ops = ops,
  .activeBank = NULL,
  .banks = banks,
  .state = idle
};

void setup() {
  // LEDs configuration
  pinMode(LED1, OUTPUT); // unsued
  pinMode(LED2, OUTPUT);
  digitalWrite(LED, HIGH);

  Serial.begin(115200);

  // allocalte banks
  ubrb.banks.bank = (uint8_t **)malloc(sizeof(uint8_t *) * ubrb.banks.num);
  for (int i = 0; i < bankNum; ++i) {
    ubrb.banks.bank[i] = (uint8_t *)malloc(ubrb.banks.size);
    memset(ubrb.banks.bank[i], 0, bankSize);
  }

  digitalWrite(LED, LOW);
}

void loop() {
  ubrb_tick(&ubrb);
  delay(100);
}
