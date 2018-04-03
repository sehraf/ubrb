#include "ubrb.h"
#include <string.h>

//#define UBRB_DEBUG
#ifdef UBRB_DEBUG
    #define debug_printf(args...) printf(args)
#else
    #define debug_printf(args...) do {} while(0)
#endif

uint16_t i;
uint8_t charReceived;
uint8_t *char_p;
uint16_t counter;

int readTimeout(struct ubrb *ubrb);
void setLED(struct ubrb *, uint8_t v);
char bin2hex(uint8_t b);
uint8_t hex2bin(char c);

void ubrb_tick(struct ubrb *ubrb) {	
	charReceived = '\0';

	switch(ubrb->state) {
	case idle:
		// anything to receive?
		if (!ubrb->ops.readByte(&charReceived))
			break;

		// valid char?
		if (charReceived != 'G' && charReceived != 'S' && charReceived != 'C')
			break;

		// decode operation
		switch (charReceived) {
		case 'C':
			ubrb->state = clear;
			break;
		case 'G':
			ubrb->state = get;
			break;
		case 'S':
			ubrb->state = set;
			break;
		}

		// receive and decode bank number
		if (readTimeout(ubrb)) {
			if (charReceived >= '0' && charReceived <= '9') {
				// decode index
				charReceived = hex2bin(charReceived);

				if (charReceived < ubrb->banks.num)
					ubrb->activeBank = ubrb->banks.bank[charReceived];
				else
					ubrb->state = idle;
			}
			else
				ubrb->state = idle;
		} 
		else
			ubrb->state = idle;

		// skip rest in case of timeout
		if (ubrb->state == idle)
			break;

		// wait for \n
		if (readTimeout(ubrb)) {
			if (charReceived != '\n')
				ubrb->state = idle;
		} else
			ubrb->state = idle;

		debug_printf("switching state: ");
		debug_printf(ubrb->state + '0');
		debug_printf('\n');

		break;
	case get:
		setLED(ubrb, 1);
		ubrb_get(ubrb);
		ubrb->state = idle;
		debug_printf("get\n");
		break;
	case set:
		setLED(ubrb, 1);
		// clear target buffer first
		ubrb_clear(ubrb);
		ubrb_set(ubrb);
		ubrb->state = idle;
		debug_printf("set\n");
		break;
	case clear:
		setLED(ubrb, 1);
		ubrb_clear(ubrb);
		ubrb->state = idle;
		debug_printf("clear\n");
		break;
	}
	setLED(ubrb, 0);
}

void ubrb_get(struct ubrb *ubrb) {
	char_p = &ubrb->activeBank[0];
	
	for (i = 0; i < ubrb->banks.size; ++i, ++char_p) {
		// send first nibble
		ubrb->ops.writeByte(bin2hex((*char_p >> 4) & 0xf));
		// send second nibble
		ubrb->ops.writeByte(bin2hex( *char_p       & 0xf));

		setLED(ubrb, i % 2);
	}

	ubrb->ops.writeByte('\n');
}

void ubrb_set(struct ubrb *ubrb) {
	char_p = &ubrb->activeBank[0];

	i = 0;
	// we receive twice the size
	while ((i >> 1) < ubrb->banks.size) {
		// read next character
		if (!readTimeout(ubrb))
			break;

		setLED(ubrb, i % 2);

		// check for end
		if (charReceived == '\n')
			break;

		// valid hex?
		if (!(	(charReceived >= '0' && charReceived <= '9') || 
				(charReceived >= 'a' && charReceived <= 'f')))
			continue; // just skip 

		// decode
		charReceived = hex2bin(charReceived);
		
		// write nibble
		*char_p |= charReceived;

		// processed i'th character
		++i;

		// shift character or go to next
		if (i % 2) {
			// wrote first nibble
			*char_p <<= 4;
			debug_printf("shift\n");
		} else {
			// wrote second nibble
			++char_p;
			debug_printf("next\n");
		}
	}
}

void ubrb_clear(struct ubrb *ubrb) {
	i = 0;
	// rng available?
	if (ubrb->ops.rng.getByte)
		for (; i < ubrb->banks.size; ++i) ubrb->activeBank[i] = ubrb->ops.rng.getByte();
	else
		//for (; i < ubrb->banks.size; ++i) ubrb->activeBank[i] = 0;
		memset(ubrb->activeBank, 0, ubrb->banks.size);
}

int readTimeout(struct ubrb *ubrb) {
	counter = 0;
	do {
		if (ubrb->ops.readByte(&charReceived)) 
			return 1;
		else
			if ((counter++) >> 2 >= timeout)
				return 0;
	} while(1);
}

void inline setLED(struct ubrb *ubrb, uint8_t val) {
	if (ubrb->ops.leds.setLED)
		ubrb->ops.leds.setLED(val);
}

char inline bin2hex(uint8_t b) {
	b &= 0xf;

	if (b >= 10)
		return 'a' + b - 10;
	else
		return '0' + b;
}

uint8_t inline hex2bin(char c) {
	if (c >= 'a')
		return c - 'a' + 10;
	else
		return c - '0';
}