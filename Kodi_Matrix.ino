#include <LEDMatrixDriver.hpp>

// This sketch draw marquee text on your LED matrix using the hardware SPI driver Library by Bartosz Bielawski.
// Example written 16.06.2017 by Marko Oette, www.oette.info

// Define the ChipSelect pin for the led matrix (Dont use the SS or MISO pin of your Arduino!)
// Other pins are Arduino specific SPI pins (MOSI=DIN, SCK=CLK of the LEDMatrix) see https://www.arduino.cc/en/Reference/SPI
const uint8_t LEDMATRIX_CS_PIN = 9;

// Define LED Matrix dimensions: 32x8, 16x8, or 8x8
const int LEDMATRIX_WIDTH    = 32;
const int LEDMATRIX_HEIGHT   = 8;
const int LEDMATRIX_SEGMENTS = LEDMATRIX_WIDTH / LEDMATRIX_HEIGHT;

// The LEDMatrixDriver class instance
LEDMatrixDriver lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN);

void setup() {
	// init the display
	lmd.setEnabled(true);
	lmd.setIntensity(5);   // 0 = low, 10 = high

	Serial.begin(57600);
}

int maximum = 0;
int elapsed = 0;
void loop() {
	delay(10);
	recvWithEndMarker();

	lmd.clear();
	lmd.display();

	if (!elapsed) {
		return;
	}

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

	//char buf[40];
	//sprintf(buf,"Elapsed: %i Total: %i\r\n", elapsed, maximum);
	//Serial.print(buf);

	//sprintf(buf,"Hours: %i Mins: %i Seconds: %i\r\n", hours, minutes, seconds);
	//Serial.print(buf);

	if (elapsed <= 3600) {
		drawSprite( sprites[dig2],  0,  0, 8, 8 );
		drawSprite( sprites[dig3],  4,  0, 8, 8 );
		drawSprite( sprites[10],    8,  0, 8, 8 );
		drawSprite( sprites[dig4],  10, 0, 8, 8 );
		drawSprite( sprites[dig5],  14, 0, 8, 8 );
	} else {
		drawSprite( sprites[dig1],  0,  0, 8, 8 );
		drawSprite( sprites[10],    4,  0, 8, 8 );
		drawSprite( sprites[dig2],  6,  0, 8, 8 );
		drawSprite( sprites[dig3],  10, 0, 8, 8 );
		drawSprite( sprites[10],    14, 0, 8, 8 );
		drawSprite( sprites[dig4],  16, 0, 8, 8 );
		drawSprite( sprites[dig5],  20, 0, 8, 8 );
	}

	float percent = ((float)elapsed / (float)maximum) * 100;
	draw_percent_bar(percent);

	// Toggle display of the new framebuffer
	lmd.display();
}

void draw_percent_bar(float percent) {
	float dot_width = (1.0 / (float)(LEDMATRIX_WIDTH)) * 100;
	//Serial.print("DotWidth: ");
	//Serial.println(dot_width);

	for (int i = 0 ; i < 32 ; i++) {
		float top_dot_percent    = ((float)i / (float)(LEDMATRIX_WIDTH - 1)) * 100;
		float bottom_dot_percent = top_dot_percent - (dot_width / 2.0);

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

const byte numChars = 100;
char receivedChars[numChars]; // an array to store the received data
boolean newData = false;

void recvWithEndMarker() {
	static byte ndx = 0;
	char endMarker = '\n';
	char rc;

	// if (Serial.available() > 0) {
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
			ndx = 0;
			newData = true;
			//Serial.print("Got: ");
			//Serial.println(receivedChars);

			int id = 0;

			char *word;
			word = strtok(receivedChars,":|");
			while (word != NULL) {
				if (id == 0) {
					elapsed = atoi(word);
				} else if (id == 1) {
					maximum = atoi(word);
				} else {
					// Stop/Start/Pause
				}

				word = strtok (NULL, ":|");
				id++;
			}

			newData = false;
			char buf[40] = "";
			sprintf(buf, "Elapsed: %i Total: %i\r\n", elapsed, maximum);

			Serial.print(buf);
		}
	}
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
