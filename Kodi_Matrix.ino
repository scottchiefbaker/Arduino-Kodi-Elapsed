#include <LEDMatrixDriver.hpp>

#include <PrintEx.h>
PrintEx s = Serial;

// This sketch draw marquee text on your LED matrix using the hardware SPI driver Library by Bartosz Bielawski.
// Example written 16.06.2017 by Marko Oette, www.oette.info

// Define the ChipSelect pin for the led matrix (Dont use the SS or MISO pin of your Arduino!)
//* DIN => Pin #13
//* CLK => Pin #11
//* CS  => Pin #9
const uint8_t LEDMATRIX_CS_PIN = D2;

// Define LED Matrix dimensions: 32x8, 16x8, or 8x8
const int LEDMATRIX_WIDTH    = 32;
const int LEDMATRIX_HEIGHT   = 8;
const int LEDMATRIX_SEGMENTS = LEDMATRIX_WIDTH / LEDMATRIX_HEIGHT;

// The LEDMatrixDriver class instance
LEDMatrixDriver lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN);

void setup() {
	// init the display
	lmd.setEnabled(true);
	lmd.setIntensity(2); // 0 = low, 10 = high

	Serial.begin(57600);

	// Show the splash screen so you know it's on
	init_matrix();
	delay(2000);

	clear_display();
}

// Variable to store the last time we saw a serial update
unsigned long last_update = 0;

int maximum   = 0; // Total number of seconds in the media
int elapsed   = 0; // Elapsed seconds in the media
int play_mode = 0; // 1 = Play, 2 = Pause, 3 = Stop

byte sprites[15][8] = {
	{ 0x20,0x50,0x50,0x50,0x20,0x00,0x00,0x00 }, // 0
	{ 0x20,0x60,0x20,0x20,0x70,0x00,0x00,0x00 }, // 1
	{ 0x20,0x50,0x10,0x20,0x70,0x00,0x00,0x00 }, // 2
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
byte clear[8] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

void loop() {
	delay(10);
	int ok = process_serial_commands();

	if (!maximum) {
		Serial.print("No input data\r\n");
		clear_display();

		delay(250);

		return;
	}

	// If we don't have any new serial data in X seconds clear the display
	unsigned long now = millis();
	if (now - last_update > 5000) {
		clear_display();

		return;
	}

	draw_elapsed();
	draw_percent_bar();

	// Toggle display of the new framebuffer
	lmd.display();
}

void draw_elapsed() {
	int hours   = elapsed / 3600;
	int minutes = elapsed / 60;
	int seconds = elapsed % 60;

	if (hours) {
		minutes = minutes - (hours * 60);
	}

	int dig1 = hours;
	int dig2 = minutes / 10;
	int dig3 = minutes % 10;
	int dig4 = seconds / 10;
	int dig5 = seconds % 10;

	//s.printf("Elapsed: %i Total: %i\r\n", elapsed, maximum);
	//s.printf("Hours: %i Mins: %i Seconds: %i\r\n", hours, minutes, seconds);

	// Play
	if (play_mode == 1) {
		drawSprite( sprites[11], -1, 0, 8, 8 );
	// Pause
	} else if (play_mode == 2) {
		drawSprite( sprites[12], -1, 0, 8, 8 );
	// Stop
	} else if (play_mode == 3) {
		drawSprite( sprites[13], -1, 0, 8, 8 );
	}

	// Number of pixels to start the numbers (from the left)
	// This leaves room for the Play/Pause symbol
	int offset = 3;

	if (elapsed < 3600) {
		drawSprite( sprites[dig2], offset + 1,  0, 8, 8 );
		drawSprite( sprites[dig3], offset + 5,  0, 8, 8 );
		drawSprite( sprites[10],   offset + 9,  0, 8, 8 );
		drawSprite( sprites[dig4], offset + 11, 0, 8, 8 );
		drawSprite( sprites[dig5], offset + 15, 0, 8, 8 );
	} else {
		drawSprite( sprites[dig1], offset + 1,  0, 8, 8 );
		drawSprite( sprites[10],   offset + 5,  0, 8, 8 );
		drawSprite( sprites[dig2], offset + 7,  0, 8, 8 );
		drawSprite( sprites[dig3], offset + 11, 0, 8, 8 );
		drawSprite( sprites[10],   offset + 15, 0, 8, 8 );
		drawSprite( sprites[dig4], offset + 17, 0, 8, 8 );
		drawSprite( sprites[dig5], offset + 21, 0, 8, 8 );
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
}

// Buffer to store incoming serial commands
const byte numChars = 50;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false;

int process_serial_commands() {
	static byte ndx = 0;
	char endMarker = '\n';
	char rc;

	while (Serial.available() > 0 && newData == false) {
		rc = Serial.read();

		if (rc != endMarker) {
			receivedChars[ndx] = rc;
			ndx++;
			if (ndx >= numChars) {
				ndx = numChars - 1;
			}
		} else {
			receivedChars[ndx] = '\0'; // terminate the string
			ndx                = 0;
			newData            = true;
			//s.printf("Got: %s\r\n", receivedChars);

			// Input format is: "3190:4940:Play"
			// Input format is: "!command:value"

			String input = receivedChars;

			int first  = input.indexOf(":");
			int second = input.indexOf(":", first + 1);

			if (first == -1) {
				elapsed   = 0;
				maximum   = 0;
				play_mode = 0;

				newData = false;
				return 0;
			}

			String parts[3];

			parts[0] = input.substring(0, first);
			parts[1] = input.substring(first + 1, second);
			parts[2] = input.substring(second + 1);

			//s.printf("Words: %s / %s / %s\r\n", parts[0].c_str(), parts[1].c_str(), parts[2].c_str());

			if (parts[0].startsWith("!")) {
				int ok = process_cmd(parts[0],parts[1]);
			} else {
				elapsed = parts[0].toInt();
				maximum = parts[1].toInt();

				if (parts[2] == "Play") {
					play_mode = 1;
				} else if (parts[2] == "Pause") {
					play_mode = 2;
				} else if (parts[2] == "Stop") {
					play_mode = 3;
				}

				last_update = millis();
			}

			newData = false;
			//s.printf("Elapsed: %i Total: %i\r\n", elapsed, maximum);
		}
	}

	return 1;
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
		lmd.setIntensity(num);

		//s.printf("Setting intensity to %d\r\n", num);

		ret = 1;
	}

	return ret;
}
