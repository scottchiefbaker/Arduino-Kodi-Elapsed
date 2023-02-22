#include <TM1637Display.h>

#define CLK 16
#define DIO 17

TM1637Display display(CLK, DIO);

// Header definition
void show_clock(uint8_t hours, uint8_t minutes);

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

void clear_display() {
	display.clear();
}

void init_matrix() {
	const short my_delay = 200;

	show_clock(11,11);
	delay(my_delay);
	show_clock(22,22);
	delay(my_delay);
	show_clock(33,33);
	delay(my_delay);
	show_clock(44,44);
	delay(my_delay);
	show_clock(55,55);
	delay(my_delay);
	show_clock(66,66);
	delay(my_delay);
	show_clock(77,77);
	delay(my_delay);
	show_clock(88,88);
	delay(my_delay);
	show_clock(99,99);
	delay(my_delay);
}

void set_brightness(int num) {
	display.setBrightness(num);
}

void show_clock(uint8_t hours, uint8_t minutes) {
	uint8_t d1 = hours   / 10; // Tens digit of hours
	uint8_t d2 = hours   % 10; // Ones digit of hours
	uint8_t d3 = minutes / 10; // Tens digit of minutes
	uint8_t d4 = minutes % 10; // Ones digit of minutes

	/*
	char msg[100] = "";
	snprintf(msg, 100, "Displaying: %d hours %d minutes (%d%d:%d%d)\n", hours, minutes, d1, d2, d3, d4);
	Serial.print(msg);
	*/

	uint8_t digits[4];
	digits[0] = display.encodeDigit(d1);
	digits[1] = display.encodeDigit(d2);
	digits[2] = display.encodeDigit(d3);
	digits[3] = display.encodeDigit(d4);

	// Turn on the colon
	digits[1] |= 128;

	// No leading zero on times like 1:34
	if (!d1) {
		digits[0] = 0; // Off
	}

	display.setSegments(digits, 4, 0);
}

int show_elapsed(unsigned int seconds, uint8_t play_mode) {
	uint8_t d1, d2, d3, d4;
	uint8_t hours, minutes;

	if (seconds == 0) {
		clear_display();

		return 0;
	}

	// This is minutes mode
	// Kick in at 99:59
	if (seconds <= 99 * 60 + 59) {
		hours    = 0;
		minutes  = seconds / 60;
		seconds -= (minutes * 60);

		show_clock(minutes, seconds);
	// This is seconds mode
	} else {
		hours    = seconds / 3600;
		seconds -= (hours * 3600);

		minutes = seconds / 60;
		seconds -= (minutes * 60);

		show_clock(hours, minutes);
	}

	//char msg[100] = "";
	//snprintf(msg, 100, "Displaying: %d hours %d minutes %d seconds (%d%d:%d%d)\n", hours, minutes, seconds, d1, d2, d3, d4);
	//Serial.print(msg);
	return 1;
}
