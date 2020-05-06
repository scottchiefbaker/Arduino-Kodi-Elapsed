#ifdef TM1637

#include <TM1637Display.h>

#define CLK 3
#define DIO 2

TM1637Display display(CLK, DIO);

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

void clear_display() {
	display.clear();
}

int show_time(int seconds, int play_mode) {
	int orig = seconds;
	int d1, d2, d3, d4, hours, minutes;

	if (orig == 0) {
		clear_display();

		return 0;
	}

	// Less than one hour
	if (orig < 3599) {
		hours    = 0;
		minutes  = seconds / 60;
		seconds -= (minutes * 60);

		d1 = minutes / 10;
		d2 = minutes % 10;
		d3 = seconds / 10;
		d4 = seconds % 10;
	} else {
		hours    = seconds / 3600;
		seconds -= (hours * 3600);

		minutes = seconds / 60;
		seconds -= (minutes * 60);

		d1 = hours   / 10;
		d2 = hours   % 10;
		d3 = minutes / 10;
		d4 = minutes % 10;
	}

	//char msg[100] = "";
	//snprintf(msg, 100, "Displaying: %d hours %d minutes %d seconds (%d%d:%d%d)\n", hours, minutes, seconds, d1, d2, d3, d4);
	//Serial.print(msg);

	// Convert the digits to a decimal number we can display
	int display_num = (d1 * 1000) + (d2 * 100) + (d3 * 10) + d4;

	// Number of digits to display
	uint8_t digit_count;
	uint8_t digit_pos;
	if (d1) {
		digit_count = 4;
		digit_pos   = 0;
	} else {
		digit_count = 3;
		digit_pos   = 1;
	}

	// https://github.com/avishorp/TM1637/blob/master/TM1637Display.h#L89
	display.showNumberDecEx(display_num, 0b11100000, true, digit_count, digit_pos);
}

void draw_percent_bar() {
}

void init_matrix() {
}

void set_brightness(int num) {
	display.setBrightness(num);
}

#endif
