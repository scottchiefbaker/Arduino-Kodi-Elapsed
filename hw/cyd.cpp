#include <SPI.h>
#include <TFT_eSPI.h>

////////////////////////////////////////////////

// The font names are arrays references, thus must NOT be in quotes ""
//#include "NotoSansBold36.h"
//#define AA_FONT_LARGE NotoSansBold36

#include "Helvetica-80.h";
#define AA_FONT_LARGE Helvetica_80

//#include "DejaVuSansMono-Bold-80.h"
//#define AA_FONT_LARGE DejaVuSansMono_Bold_80

////////////////////////////////////////////////

TFT_eSPI tft = TFT_eSPI();
int offset = 10;

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

void clear_display() {
	tft.fillScreen(TFT_BLACK);
}

void init_matrix() {
	tft.begin();
	tft.setRotation(1);
	clear_display();

	/////////////////////////////////////////

	tft.loadFont(AA_FONT_LARGE);
	tft.setTextPadding(100);
	tft.setTextColor(TFT_WHITE, TFT_BLACK, true);
	tft.drawString("Init", 10, 10);

	/////////////////////////////////////////

	delay(2000);
	clear_display();
}

void set_brightness(int num) {
	// Nothing yet
}

void draw_action_glyph(uint8_t action) {
	static uint8_t last = 0;
	bool has_changed    = (action != last);

	// Only redraw if something has changed
	if (last && !has_changed) {
		//return;
	}

	int xpos = offset;
	int ypos = offset;
	int size = 36;

	// Clear the previous glyph
	tft.fillRect(xpos, ypos, size, size, TFT_BLACK);

	if (action == KODI_PLAY) {
		tft.fillTriangle(xpos, ypos, xpos, size, xpos + (size / 2.5), ypos + (size / 3), TFT_WHITE);
	} else if (action == KODI_PAUSE) {
		tft.fillRect(xpos     , ypos, 8, size, TFT_WHITE);
		tft.fillRect(xpos + 17, ypos, 8, size, TFT_WHITE);
	} else if (action == KODI_STOP) {
		tft.fillRect(xpos, ypos, size, size, TFT_WHITE);
	}

	last = action;
}

void draw_elapsed_text(uint16_t elapsed) {
	uint8_t hours   = elapsed / 3600;
	uint8_t minutes = elapsed / 60;
	uint8_t seconds = elapsed % 60;

	if (hours) {
		minutes = minutes - (hours * 60);
	}

	uint16_t xpos = (tft.width() - offset);
	uint16_t ypos = (tft.height() - (offset * 3));

	uint16_t font_offset = 8;
	ypos += font_offset;

	char buf[16] = "";

	int font_size;

	// XX:YY:ZZ
	if (elapsed >= 3600) {
		font_size = 120;
		snprintf(buf, sizeof(buf), " %01d:%02d:%02d", hours, minutes, seconds);
	// XX:YY
	} else {
		snprintf(buf, sizeof(buf), " %2d:%02d", minutes, seconds);
		font_size = 200;
	}

	tft.setTextDatum(BR_DATUM); // Bottom right

	////////////////////////////////////////////////////////

	tft.loadFont(AA_FONT_LARGE);
	tft.setTextPadding(100);
	tft.setTextColor(TFT_WHITE, TFT_BLACK, true);
	tft.drawString(buf, xpos, ypos);

	return;
}

void draw_progress_bar(float percent) {
	int width = tft.width() - (offset * 2); // Left + right
    int ypos  = tft.height() - (offset * 2);
    int total = (percent / 100.0) * width;

	uint8_t red   = 30;
	uint8_t green = 63;
	uint8_t blue  = 7;

	// This changes from the original color, more towards red the
	// higher the percent
	red = red - (6 * (percent / 100.0));

	uint32_t color = (red << 11) | (green << 5) | blue;

	// Draw the outline rectangle
	tft.fillRect(offset, ypos, width, offset, TFT_BLACK);

	// Draw the outline rectangle
	tft.drawRect(offset, ypos, width, offset, color);

	// Draw the filled progress bar
    tft.fillRect(offset, ypos, total, offset, color);

    //Serial.printf("Drawing: %f %d\n", percent, total);
}

int show_elapsed(unsigned int seconds, uint8_t play_mode) {
	if (seconds == 0) {
		clear_display();

		return 0;
	}

	draw_action_glyph(play_mode);

	draw_elapsed_text(seconds);

	float percent = ((float)seconds / maximum) * 100;
	draw_progress_bar(percent);

	return 1;
}
