#include <EEPROM.h>
#include <esp-rix.h>

// Variable to store the last time we saw a serial update
unsigned long last_update = 0;

int maximum   = 0; // Total number of seconds in the media
int elapsed   = 0; // Elapsed seconds in the media
int play_mode = 0; // 1 = Play, 2 = Pause, 3 = Stop
int invert    = 0; // 1 = Show remaining time, 0 = Show elapsed time

// Store the previous values
int maximum_p   = 0;
int elapsed_p   = 0;
int play_mode_p = 0;

// Choose which display version you want to use
//#include "hw/tm1637.cpp"
#include "hw/matrix.cpp"

void loop() {
	delay(10);
	int ok = process_serial_commands();

	if (!maximum) {
		Serial.print("Waiting for input...\r\n");
		rix_5("Waiting for input...");

		delay(240);
	}

	// If we don't have any new serial data in X seconds clear the display
	unsigned long now = millis();
	if (now - last_update > 4000) {
		// We don't have any data so we reset all the time vars
		elapsed   = play_mode = maximum = 0;
		maximum_p = elapsed_p = play_mode_p = 0;

		clear_display();

		return;
	}

	// No data came in via serial
	if (!ok) {
		return;
	}

	bool is_new = (maximum != maximum_p || elapsed != elapsed_p || play_mode != play_mode_p);

	// Don't draw anything unless the numbers have changed
	if (!is_new) {
		return;
	}

	// Show the numbers on the display
	draw_elapsed();

	// Store the current values to we can check it next loop
	maximum_p   = maximum;
	elapsed_p   = elapsed;
	play_mode_p = play_mode;

  handle_rix();
}

void draw_elapsed() {
	int time = elapsed;
	if (invert) {
		time = maximum - elapsed;
	}

	show_elapsed(time, play_mode);
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

				//s.printf("Adding '%c' ... string is now %d chars long\r\n", rc, ndx);
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


				rix_6("Words: %s / %s / %s\r\n", parts[0].c_str(), parts[1].c_str(), parts[2].c_str());


				if (parts[0].startsWith("!")) {
					int ok = process_cmd(parts[0], parts[1]);

					rix_4("Command: %s = %s\r\n", parts[0].c_str(), parts[1].c_str());

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

					rix_6("Serial: \"%s\" / %i / %i\r\n", parts[2].c_str(), elapsed, maximum);
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

int process_cmd(String cmd, String value) {
	int ret = 0;

	if (cmd == "!intensity") {
		int num = value.toInt();
		set_intensity(num);

		ret = 1;
	} else if (cmd == "!invert") {
		int num = value.toInt();
		set_invert(num);

		ret = 1;
	}

	return ret;
}

void set_invert(int val) {
	// -1 is a toggle (set it to the opposite)
	if (val == -1) {
		val = !invert;
	}
	invert = val;

	EEPROM.write(11,val);
#if defined(ESP8266) || defined(ESP32)
	EEPROM.commit();
#endif

	clear_display();
}

int get_invert() {
	int ret = EEPROM.read(11);

	return ret;
}

void set_intensity(int val) {
	set_brightness(val);
	lmd.setIntensity(val);

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

void setup() {
	Serial.begin(115200);

#if defined(ESP8266) || defined(ESP32)
	EEPROM.begin(64);
#endif

	set_brightness(fetch_intensity()); // 0 = low, 10 = high
	invert = get_invert();

	// Show the splash screen so you know it's on
	init_matrix();
	delay(500);

	int ok = rix_init_wifi("YourSSID", "SekritPassword");

	clear_display();
}

// vim: tabstop=4 shiftwidth=4 noexpandtab autoindent softtabstop=4
