#include <LEDMatrixDriver.hpp> // LEDMatrixDriver.hpp v0.2.2

// Define the ChipSelect pin for the led matrix (Dont use the SS or MISO pin of your Arduino!)
//* DIN => Pin #13
//* CLK => Pin #11
//* CS  => Pin #9
const uint8_t LEDMATRIX_CS_PIN = 5;

// Define LED Matrix dimensions: 32x8, 16x8, or 8x8
const int LEDMATRIX_WIDTH    = 32;
const int LEDMATRIX_HEIGHT   = 8;
const int LEDMATRIX_SEGMENTS = LEDMATRIX_WIDTH / LEDMATRIX_HEIGHT;

// https://github.com/bartoszbielawski/LEDMatrixDriver/blob/master/src/LEDMatrixDriver.hpp#L44
// If the matrixes are diplaying in the wrong direction (right to left) change this:
// 1 = Invert segments
// 2 = Flip x-axis
// 4 = Flip y-axis
const int LEDMATRIX_FLAGS = 2;

// The LEDMatrixDriver class instance
LEDMatrixDriver lmd(LEDMATRIX_SEGMENTS, LEDMATRIX_CS_PIN, LEDMATRIX_FLAGS);

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

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

void clear_display() {
	lmd.clear();
	lmd.display();
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

void show_elapsed(int time, int play_mode) {
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

	lmd.clear();

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

	draw_percent_bar();

	// Toggle display of the new framebuffer
	lmd.display();
}

void init_matrix() {
	// init the display
	lmd.setEnabled(true);

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

void set_brightness(int num) {
	lmd.setIntensity(num);
}
