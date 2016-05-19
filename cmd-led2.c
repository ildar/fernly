#include <string.h>
#include "bionic.h"

#include "printf.h"
#include "serial.h"
#include "led.h"

int cmd_led(int argc, char **argv) {
	uint32_t state;
	uint32_t led;

	if (argc < 1) {
		printf("Usage: led  [0 = green 1 = red 2 = blue] [1 = on, 0 = off]\n");
		return -1;
	}

	led = strtoul(argv[0], NULL, 0);
	state = strtoul(argv[1], NULL, 0);

	switch (led) {

	case 0:
		LedGreen(state);
		break;
	case 1:
		LedRed(state);
		break;
	case 2:
		LedBlue(state);
		break;

	default:
		printf("Wrong led number\n");

	}

	return 0;
}

