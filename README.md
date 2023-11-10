# Kodi Elapsed/Remaining Time display

Use an Arduino and an LED Matrix to show the elapsed or remaining time for Kodi. Now your Kodi installation can look just like and old DVD player.

## Requirements

* Kodi installation with the HTTP API enabled
* 32x8 LED Matrix or TM1637 seven segment "clock" display
* Arduino to drive the LED Matrix
  * Tested on: Arduino Nano, ESP8266, and ESP32
* Perl or Python installed to read data from Kodi and send data to Arduino
* Arduino connected to the RPi via USB (for serial connectivity)

## Installation

* Install sketch on Arduino Nano
* Wire LED Matrix to Arduino Nano
  * DIN => Pin #13
  * CLK => Pin #11
  * CS  => Pin #9
* Run Perl script to read Kodi API and send time to display
  * `systemd` service script provided to automate startup

## Screenshots
<img style="width: 500px;" src="https://user-images.githubusercontent.com/3429760/282158971-17c1fdd1-5235-428a-82a5-4e4f861c419c.jpg">
<img style="width: 500px;" src="https://user-images.githubusercontent.com/3429760/282158968-cb874d84-f467-4484-a4bb-fd6a66e53b10.jpg">

## Testing

Once your Arduino and display are connected you can test output with the following commands:

* `echo "<1236:2860:Play>" > /dev/ttyUSB0`
* run `kodi-led-service.pl --test`
