#!/usr/bin/python
import urllib2
import json
import time
import serial
import sys

kodi_ip              = "192.168.5.21"
player_id            = "1" # Video = 1, Audio = 0
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
    global player_id
    global kodi_ip

    while 1:
        start = time.time()

        # Where to connect for the Kodi API
        url = 'http://' + kodi_ip + '/jsonrpc?request={"jsonrpc":"2.0","method":"Player.GetProperties","params":{"playerid":' + player_id + ',"properties":["time","totaltime","percentage","speed"]},"id":"1"}'

        # This is raw bytes
        resp = urllib2.urlopen(url).read()
        # Convert bytes to a string
        resp = resp.decode("utf-8")

        # String to hash
        x = json.loads(resp)

        error_code = x.get("error", {}).get("code", 0)

        if error_code != 0:
            new_player_id = get_active_player();
            print("Error playerid " + player_id + " is not valid anymore. Switching to " + new_player_id)
            player_id = new_player_id

            time.sleep(2)
            continue

        timez = x.get('result', {}).get('time', {})
        if (len(timez) == 0):
            continue

        hours = timez.get('hours', 0)
        mins  = timez.get('minutes', 0)
        secs  = timez.get('seconds', 0)

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
        # Format : <ElapsedSeconds:TotalSeconds:PlayMode>
        # Example: <1042:2820:Play>
        line = "<" + str(cur_time) + ":" + str(total) + ":" + speed_str + ">"
        print(line)

        # Write the line to the serial port
        ser.write(line + "\n")

        end        = time.time()
        total      = end - start
        sleep_time = 0.4995 - total

        # Sleep X seconds
        time.sleep(sleep_time)

def run_test():
    i     = 0
    total = 6000;

    while i < total:
        line = "<" + str(i) + ":" + str(total) + ":Play" + ">"

        # Write the line to the serial port
        ser.write(line + "\n")
        #print(line)

        time.sleep(0.05)
        i += 1

    return i

def get_active_player():
    global kodi_ip

    url = 'http://' + kodi_ip + '/jsonrpc?request={"jsonrpc":"2.0","id":1,"method":"Player.GetActivePlayers"}'

    # This is raw bytes
    resp = urllib2.urlopen(url).read()
    # Convert bytes to a string
    resp = resp.decode("utf-8")

    # String to hash
    x = json.loads(resp)

    try:
        active_id = x["result"][0]["playerid"]
    except:
        active_id = -1

    active_id = str(active_id)

    print("Active playerid: " + active_id)

    return active_id

main()
