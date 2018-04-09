#ifndef UBRB_h
#define UBRB_h

// #include <stdio.h>
#include <stdint.h>

#define sleepTime 50 // ms
#define timeout 20 // --> sleepTime * timeout = 1s

enum state {
	idle = 0,
	get,
	set,
	clear
};

/*
	rng and led are optional
	the rest must be set
*/
struct ubrb_rng {
	uint8_t  (*getByte)();
	/* uint16_t (*getWord)(); */
};

struct ubrb_leds {
	void (*setLED)(uint8_t);
	/* void (*setLED)(uint8_t led, uint8_t val); */
};

struct ubrb_ops {
	int (*readByte)(uint8_t *);
	void (*writeByte)(uint8_t);
	struct ubrb_rng rng;
	struct ubrb_leds leds;
	void (*delay)(uint16_t);
};

struct ubrb_banks {
	uint8_t num;
	uint16_t size;
	uint8_t **bank;
};

struct ubrb {
	struct ubrb_ops ops;
	uint8_t *activeBank;
	struct ubrb_banks banks;
	enum state state;
};

void ubrb_tick(struct ubrb *ubrb);
void ubrb_get(struct ubrb *ubrb);
void ubrb_set(struct ubrb *ubrb);
void ubrb_clear(struct ubrb *ubrb);
void ubrb_checkStruct(struct ubrb *ubrb);

#endif