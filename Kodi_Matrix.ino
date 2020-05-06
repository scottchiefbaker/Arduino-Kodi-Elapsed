#include <LEDMatrixDriver.hpp>
#include <EEPROM.h>

#include <PrintEx.h>
PrintEx s = Serial;

// This sketch draw marquee text on your LED matrix using the hardware SPI driver Library by Bartosz Bielawski.
// Example written 16.06.2017 by Marko Oette, www.oette.info

// Define the ChipSelect pin for the led matrix (Dont use the SS or MISO pin of your Arduino!)
//* DIN => Pin #13
//* CLK => Pin #11
//* CS  => Pin #9
const uint8_t LEDMATRIX_CS_PIN = 5;

// Define LED Matrix dimensions: 32x8, 16x8, or 8x8
const int LEDMATRIX_WIDTH    = 32;
const int LEDMATRIX_HEIGHT   = 8;
const int LEDMATRIX_SEGMENTS = LEDMATRIX_WIDTH / LEDMATRIX_HEIGHT;

// If the matrixes are diplaying in the wrong direction (right to left) change
// this to true
bool display_reverse = true;
// The LEDMatrixDriver class instance
LEDMatrixDriver lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN, display_reverse);

// Variable to store the last time we saw a serial update
unsigned long last_update = 0;

int maximum   = 0; // Total number of seconds in the media
int elapsed   = 0; // Elapsed seconds in the media
int play_mode = 0; // 1 = Play, 2 = Pause, 3 = Stop
int invert    = 0; // 1 = Show remaining time, 0 = Show elapsed time
int debug     = 0; // Enable debug output on the serial port

// Font designed at: https://gurgleapps.com/tools/matrix
byte sprites[15][8] = {
	{ 0x20,0x50,0x50,0x50,0x20,0x00,0x00,0x00 }, // 0
	{ 0x20,0x60,0x20,0x20,0x70,0x00,0x00,0x00 }, // 1
	{ 0x60,0x10,0x20,0x40,0x70,0x00,0x00,0x00 }, // 2
	{ 0x60,0x10,0x60,0x10,0x60,0x00,0x00,0x00 }, // 3
	{ 0x50,0x50,0x70,0x10,0x10,0x00,0x00,0x00 }, // 4
	{ 0x70,0x40,0x60,0x10,0x60,0x00,0x00,0x00 }, // 5
	{ 0x20,0x40,0x60,0x50,0x20,0x00,0x00,0x00 }, // 6
	{ 0x70,0x10,0x20,0x20,0x20,0x00,0x00,0x00 }, // 7
	{ 0x20,0x50,0x20,0x50,0x20,0x00,0x00,0x00 }, // 8
	{ 0x20,0x50,0x30,0x10,0x20,0x00,0x00,0x00 }, // 9
	{ 0x00,0x40,0x00,0x40,0x00,0x00,0x00,0x00 }, // :
	{ 0x40,0x60,0x70,0x60,0x40,0x00,0x00,0x00 }, // Play >
	{ 0x50,0x50,0x50,0x50,0x50,0x00,0x00,0x00 }, // Pause ||
	{ 0x00,0x70,0x70,0x70,0x00,0x00,0x00,0x00 }, // Stop
};

void loop() {
	delay(10);
	int ok = process_serial_commands();

	if (!maximum) {
		Serial.print("No input data\r\n");
		delay(250);
	}

	// If we don't have any new serial data in X seconds clear the display
	unsigned long now = millis();
	if (now - last_update > 4000) {
		clear_display();

		return;
	}

	if (!ok) {
		return;
	}

	lmd.clear();

	draw_elapsed();
	draw_percent_bar();

	// Toggle display of the new framebuffer
	lmd.display();
}

void draw_elapsed() {
	int time = elapsed;
	if (invert) {
		time = maximum - elapsed;
	}

	int hours   = time / 3600;
	int minutes = time / 60;
	int seconds = time % 60;

	if (hours) {
		minutes = minutes - (hours * 60);
	}

	int dig1 = hours;
	int dig2 = minutes / 10;
	int dig3 = minutes % 10;
	int dig4 = seconds / 10;
	int dig5 = seconds % 10;

	//s.printf("Elapsed: %i Total: %i\r\n", time, maximum);
	//s.printf("Hours: %i Mins: %i Seconds: %i\r\n", hours, minutes, seconds);

	// Play
	if (play_mode == 1) {
		drawSprite( sprites[11], 0, 0, 8, 8 );
	// Pause
	} else if (play_mode == 2) {
		drawSprite( sprites[12], 0, 0, 8, 8 );
	// Stop
	} else if (play_mode == 3) {
		drawSprite( sprites[13], 0, 0, 8, 8 );
	}

	// Number of pixels to start the numbers (from the left)
	// This leaves room for the Play/Pause symbol

	if (time < 3600) {
		int offset = 12;

		// Don't draw the first leading zero
		if (dig2 != 0) {
			drawSprite( sprites[dig2], offset + 1,  0, 8, 8 ); // X_:__
		}

		drawSprite( sprites[dig3], offset + 5,  0, 8, 8 ); // _X:__
		drawSprite( sprites[10],   offset + 9,  0, 8, 8 ); // __:__ (the colon)
		drawSprite( sprites[dig4], offset + 11, 0, 8, 8 ); // __:X_
		drawSprite( sprites[dig5], offset + 15, 0, 8, 8 ); // __:_X
	} else {
		int offset = 6;

		drawSprite( sprites[dig1], offset + 1,  0, 8, 8 ); // X:__:__
		drawSprite( sprites[10],   offset + 5,  0, 8, 8 ); // _:__:__ (the first colon)
		drawSprite( sprites[dig2], offset + 7,  0, 8, 8 ); // _:X_:__
		drawSprite( sprites[dig3], offset + 11, 0, 8, 8 ); // _:_X:__
		drawSprite( sprites[10],   offset + 15, 0, 8, 8 ); // _:__:__ (the second colon)
		drawSprite( sprites[dig4], offset + 17, 0, 8, 8 ); // _:__:X_
		drawSprite( sprites[dig5], offset + 21, 0, 8, 8 ); // _:__:_X
	}
}

void draw_percent_bar() {
	float percent = ((float)elapsed / (float)maximum) * 100;

	float dot_width = (1.0 / (float)(LEDMATRIX_WIDTH * 2 - 1)) * 100;
	//s.printf("DotWidth: %0.3f\r\n", dot_width);

	for (int i = 0 ; i < 32 ; i++) {
		float bottom_dot_percent = ((float)(i * 2)     * dot_width);
		float top_dot_percent    = ((float)(i * 2 + 1) * dot_width);

		//s.printf("%02d = %0.3f/%0.3f\r\n", i, top_dot_percent, bottom_dot_percent);

		if (bottom_dot_percent <= percent) {
			lmd.setPixel(i,7,true);
		} else {
			lmd.setPixel(i,7,false);
		}

		if (top_dot_percent <= percent) {
			lmd.setPixel(i,6,true);
		} else {
			lmd.setPixel(i,6,false);
		}
	}

	// Add 0%, 50%, 100% indicators so you can see it at a distance
	//lmd.setPixel(0,7,true);
	//lmd.setPixel((LEDMATRIX_WIDTH / 2),7,true);
	//lmd.setPixel((LEDMATRIX_WIDTH -1),7,true);
}

// Buffer to store incoming serial commands
const byte numChars = 30;
char receivedChars[numChars]; // an array to store the received data

int process_serial_commands() {
    static boolean recvInProgress = false;
    static byte ndx               = 0;
    char startMarker              = '<';
    char endMarker                = '>';
    char rc;

    while (Serial.available() > 0) {
        rc = Serial.read();
		//Serial.print(rc);

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }

				//s.printf(buf,"Adding '%c' ... string is now %d chars long\r\n", rc, ndx);
            } else {
                receivedChars[ndx] = '\0'; // terminate the string
				String input = receivedChars;

				//s.printf(buf,"Input: '%s'\r\n", receivedChars);

				// Reset all the variables so we start fresh
                memset(receivedChars, 0, numChars);
                recvInProgress = false;
                ndx            = 0;

				int first  = input.indexOf(":");
				int second = input.indexOf(":", first + 1);

				// If there is no colon at all
				if (first == -1) {
					elapsed   = 0;
					maximum   = 0;
					play_mode = 0;

					return 0;
				}

				String parts[3];

				parts[0] = input.substring(0, first);
				parts[1] = input.substring(first + 1, second);
				parts[2] = input.substring(second + 1);

				if (debug) {
					s.printf("Words: %s / %s / %s\r\n", parts[0].c_str(), parts[1].c_str(), parts[2].c_str());
				}

				if (parts[0].startsWith("!")) {
					int ok = process_cmd(parts[0], parts[1]);

					if (debug) {
						s.printf("Command: %s = %s\r\n", parts[0].c_str(), parts[1].c_str());
					}
				} else {
					if (parts[2] == "Play") {
						play_mode = 1;
						last_update = millis();
					} else if (parts[2] == "Pause") {
						play_mode = 2;
						last_update = millis();
					} else if (parts[2] == "Stop") {
						play_mode = 3;
					} else {
						play_mode = 0;
					}

					if (play_mode > 0) {
						elapsed = parts[0].toInt();
						maximum = parts[1].toInt();
					}

					if (debug) {
						s.printf("Serial: \"%s\" / %i / %i\r\n", parts[2].c_str(), elapsed, maximum);
					}
				}

				return 1;
			}
		}

		else if (rc == startMarker) {
			recvInProgress = true;
		}
	}

    return 0;
}

/**
 * This draws a sprite to the given position using the width and height supplied (usually 8x8)
 */
void drawSprite( byte* sprite, int x, int y, int width, int height ) {
	// The mask is used to get the column bit from the sprite row
	byte mask = B10000000;

	for( int iy = 0; iy < height; iy++ ) {
		for( int ix = 0; ix < width; ix++ ) {
			lmd.setPixel(x + ix, y + iy, (bool)(sprite[iy] & mask ));

			// shift the mask by one pixel to the right
			mask = mask >> 1;
		}

		// reset column mask
		mask = B10000000;
	}
}

void init_matrix() {
	clear_display();

	int pixel_delay = 30;

	// Horizontal lines
	for (int i = 0; i < 32; i++) {
		lmd.setPixel(i,7,true);
		lmd.setPixel(31 - i,0,true);

		lmd.display();
		delay(pixel_delay);
	}

	// Vertical lines
	for (int i = 0; i < 8; i++) {
		lmd.setPixel(0,i,true);
		lmd.setPixel(31,8 - i,true);

		lmd.display();
		delay(pixel_delay);
	}

	pixel_delay = 10;

	for (int x = 1; x < LEDMATRIX_WIDTH - 1; x++) {
		for (int y = 1; y < 7 ; y++) {
			int val = ((x + y) % 2) == 0;
			lmd.setPixel(x,y,val);
		}

		lmd.display();

		delay(pixel_delay);
	}
}

void clear_display() {
	lmd.clear();
	lmd.display();
}

int process_cmd(String cmd, String value) {
	//s.printf("Command: %s => %s", cmd.c_str(), value.c_str());
	int ret = 0;

	if (cmd == "!intensity") {
		int num = value.toInt();
		save_intensity(num);

		ret = 1;
	} else if (cmd == "!invert") {
		int num = value.toInt();
		set_invert(num);

		ret = 1;
	} else if (cmd == "!debug") {
		int num = value.toInt();
		debug   = num; // Enable/disable global debug

		ret = 1;
	}

	return ret;
}

int set_invert(int val) {
	// -1 is a toggle (set it to the opposite)
	if (val == -1) {
		val = !invert;
	}
	invert = val;

	EEPROM.write(11,val);
#if defined(ESP8266) || defined(ESP32)
	EEPROM.commit();
#endif
}

int get_invert() {
	int ret = EEPROM.read(11);

	return ret;
}

int save_intensity(int val) {
	set_brightness(val);

	EEPROM.write(17,val);
#if defined(ESP8266) || defined(ESP32)
	EEPROM.commit();
#endif
}

int fetch_intensity() {
	int ret = EEPROM.read(17);

	if (ret > 8) {
		ret = 2; // Default value
	}

	return ret;
}

void set_brightness(int num) {
#ifdef TM1637
	display.setBrightness(num);
#else
	lmd.setIntensity(num);
#endif
}

void setup() {
	Serial.begin(57600);

#if defined(ESP8266) || defined(ESP32)
	EEPROM.begin(64);
#endif

	// init the display
	lmd.setEnabled(true);
	set_brightness(fetch_intensity()); // 0 = low, 10 = high
	invert = get_invert();

	// Show the splash screen so you know it's on
	init_matrix();
	delay(500);

	clear_display();
}

#ifdef TM1637
int show_time_tm1637(int seconds) {
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
#endif
