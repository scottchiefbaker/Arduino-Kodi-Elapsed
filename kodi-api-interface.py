#!/usr/bin/python
import urllib2
import json
import time
import serial
import sys

arduino_serial_port  = "/dev/ttyUSB0"
arduino_serial_speed = 57600

ser = serial.Serial(arduino_serial_port, arduino_serial_speed, timeout=1)

def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--test":
        run_test()
        sys.exit(1)
    else:
        loop()

def loop():
    # Where to connect for the Kodi API
    kodi_ip = "192.168.5.21"
    url     = 'http://' + kodi_ip + '/jsonrpc?request={"jsonrpc":"2.0","method":"Player.GetProperties","params":{"playerid":1,"properties":["time","totaltime","percentage","speed"]},"id":"1"}'

    while 1:
        # This is raw bytes
        resp = urllib2.urlopen(url).read()
        # Convert bytes to a string
        resp = resp.decode("utf-8")

        # String to hash
        x = json.loads(resp)

        #print(x);

        hours     = x["result"]["time"]["hours"];
        hours_str = str(hours).zfill(2)
        mins      = x["result"]["time"]["minutes"];
        mins_str  = str(mins).zfill(2)
        secs      = x["result"]["time"]["seconds"]
        secs_str  = str(secs).zfill(2)

        cur_time = (hours * 3600) + (mins * 60) + secs
        total    = (x["result"]["totaltime"]["hours"] * 3600) + (x["result"]["totaltime"]["minutes"] * 60) + x["result"]["totaltime"]["seconds"]

        speed = x['result']['speed'];
        if (speed == 1):
            speed_str = "Play"
        elif (speed == 0 and total > 0):
            speed_str = "Pause"
        elif (speed == 0):
            speed_str = "Stop"

        # Build the string to send to the Arduino via serial
        # Format: ElapsedSeconds:TotalSeconds:PlayMode
        # Example: 1042:2820:Play
        line = str(cur_time) + ":" + str(total) + ":" + speed_str
        print(line)

        # Write the line to the serial port
        ser.write(line + "\n")

        # If it's more than 99 minutes
        #if cur_time > 5940:
        #    print(hours_str + ":" + mins_str)
        #else:
        #    mins = (hours * 60) + mins
        #    mins_str = str(mins).zfill(2)
        #    print(mins_str + ":" + secs_str)

        # Sleep X seconds
        time.sleep(0.5)

def run_test():
    i     = 0
    total = 6000;

    while i < total:
        line = str(i) + ":" + str(total) + ":Play"

        # Write the line to the serial port
        ser.write(line + "\n")
        #print(line)

        time.sleep(0.05)
        i += 1

    return i

main()
