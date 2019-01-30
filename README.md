# Kodi Elapsed Time via Arduino LED Matrix

Use an Arduino and an LED Matrix to show the elasped time on a Kodi installation.

## Requirements

* Kodi installation with the HTTP API enabled
* 32x8 LED Matrix
* Arduino to drive the LED Matrix
* Python and Pyserial installed on Kodi to send data to Arduino
* Arduino connected to the RPi via USB (for serial connectivity)

## Installation

* Install sketch on Arduino
* Wire LED Matrix to Arduino
  * DIN => Pin #13
  * CLK => Pin #11
  * CS  => Pin #9
* Run Python script to read Kodi API and determine time elapsed
* Python script feeds data via serial to Arduino to display on LED Matrix

## Testing

* `echo "1236:2860:Play" > /dev/ttyUSB0`
* run `kodi-api-interface.py --test`
