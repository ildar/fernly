/*
 *
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "memio.h"

#include <stdint.h>
#include "led.h"

#include "fernvale-gpio.h"

void LedInit() {

	uint32_t value;

	// set mode GPIO 12 15 15
	value = 0;
	value |= GPIO_CTRL_MODE2_IO17_MASK;
	writel(0, GPIO_CTRL_MODE2_CLEAR);

	value = 0;
	value |= GPIO_CTRL_MODE1_IO12_MASK;
	value |= GPIO_CTRL_MODE1_IO15_MASK;
	writel(0, GPIO_CTRL_MODE1_CLEAR);


	// set GPIO as output
	value = 0;
	value |= GPIO12; // blue
	value |= GPIO17; // red
	value |= GPIO15; // green

	writel(value, GPIO_CTRL_DIR0_SET);

	// switch all led off
	value = 0;
	value |= GPIO12;
	value |= GPIO17;
	value |= GPIO15;
	writel(value, GPIO_CTRL_DOUT0_SET);

}

void LedGreen(uint32_t on) {

	uint32_t value = 0;

	value |= GPIO15; // green

	if (on == 1) {
		/* LED on */
		writel(value, GPIO_CTRL_DOUT0_CLEAR);
	} else {
		writel(value, GPIO_CTRL_DOUT0_SET);
	}

}

void LedBlue(uint32_t on) {

	uint32_t value = 0;
	value |= GPIO12; // blue

	if (on == 1) {
		/* LED on */
		writel(value, GPIO_CTRL_DOUT0_CLEAR);
	} else {
		writel(value, GPIO_CTRL_DOUT0_SET);
	}

}

void LedRed(uint32_t on) {

	uint32_t value = 0;

	value |= GPIO17; // red

	if (on == 1) {
		/* LED on */
		writel(value, GPIO_CTRL_DOUT0_CLEAR);
	} else {
		writel(value, GPIO_CTRL_DOUT0_SET);
	}

}

