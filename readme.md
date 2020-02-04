# Hass Sidecar
*Arduino code turns a Nano into a 4 channel "serial sensor" for use with Home Assistant*

## General
This code for a nano board, is intended to add 4 analog sensors to hass via the "serial sensor" platform. This allows the user to monitor voltage of a backup battery, mains power state, doorbell state, and primary power supply state. The nano interfaces with the signals using a voltage divider (for backup battery voltage), a pair of mains powered 5V supplies (for mains and primary supply), and an ACS7xx 5AAC hall sensor (for the 12VAC doorbell).

Raw ADC values are sent through the serial port to the Hass in JSON format. The code makes use of adjustable minimum change thresholds and timeouts to prevent spamming hass with excess data from noisy signals. Note all sensors must share a common ground with the nano for the ADC to read them correctly; with the usb connection this means they will also be common ground with your pi. Keep this in mind if using an isolated step down converter for your pi's 5V supply.

## Install
Flash your nano with the default hardcoded values, wire up the sensors, plug in to your pi USB port, and configure your hass serial sensor and sensor templates. Calibration mode outputs raw ADC values from all 4 sensors every 2 seconds. After getting the serial sensor working in hass, write down the raw on/off values shown in hass for the mains, bell, and primary psu sensors. Adjust values in the code accordingly, and reflash without calibration mode. Your Hassio Sidecar is now ready for action. To calibrate battery voltage, measure actual voltage with a DMM, and add a sensor template to hass to convert the raw adc value to a meaningful voltage or percent, whichever you prefer.

## Sensors

### Doorbell
A 5A AC Allegro type hall sensor can be installed to read the AC current signal from a common doorbell chime circuit. The code blocks for 0.02sec by default (~1 period) to read current peaks and valleys on the 60hz doorbell signal. If the difference between previously sent and current peak to peak value crosses "doorbellThreshold", a new peak to peak value is uploaded to the "bell" topic. If you are not seeing changes in hass, try reducing the threshold. If too much data is being sent (aka spamming hass), try reducing the threshold.

After a value is sent and "doorbellTimeout" has elapsed, the code looks for the hall sensor to fall below the threshold. When the sensor crosses the threshold to 'off', an "off" message is sent to "doorbell", and the timer is reset. The timeout prevents spamming the home assistant sensor in case the doorbell is cycled too fast. Reduce the timeout as needed if your doorbell sensor needs a faster response (... like doorbell Morse code automations, lol!).

### Mains Power
We can install a 5V wallwart (or usb charger) on the mains power, and use this signal to determine when the mains power is on or off. Similar to the doorbell, the mains analog signal is constantly monitored. If the signal goes 'low', an "off" message is sent to the "mains" topic, then it is rechecked after "mainsTimeout" to see if power is restored. When power is restored, an "on" message is sent to "mains". The timeout also prevents excessive messaging during 'brownouts' or other situations where rapid power cycling can occur. The default hardcoded values should work for most folks, since this is pretty much a digital signal (on/off... so very large thresholds are appropriate).

### Primary Supply
Useful if the hass device's primary supply is powered off of a UPS (so may stay on when mains is off). Similar in setup and function to the mains power, a wallwart is connected to the UPS output (you can also use your rpi brick, etc...), which sends 0V or 5V depending on the output of the UPS. Like the mains state, primary supply messages obey brickTimeout & brickThreshold, and sends ADC values to the "brick" topic if the threshold for change is passed. Again, like mains power the default hardcoded values for primary supply should work fine for most ppl.

### Backup Battery Voltage
Useful if there is a battery backup for the hass device's primary supply. A voltage divider can be installed on a 12V lead acid backup battery to output 0-5V based on battery voltage. Every "battUploadPeriod" milliseconds the voltage is read and the ADC value is sent to "battery". Use appropriate resistor values such that the divider output will not exceed 5V when the battery voltage is at it's maximum (I used 10k and 4k7).

To properly display battery voltage or battery % remaining in hass, you can setup a template sensor in hass using the proper equations and calibration values. Info on setting up hass template sensors, and calibrating analog sensors are both beyond the scope of this readme. Fortunately the internet is full of excellent info on both of these subjects.

### Notes
The author of this code built a "DIY UPS" system to power his RPi 3b+, which runs an instance of hassio. The Pi is powered by a 5VDC-10A buck converter (overkill yes). The converter gets power from a DIY UPS: either a 14VDC-2.5A mains powered supply (a Samsung monitor brick), or a 12VDC lead acid backup battery. These 2 supplies are connected to the 5VDC buck converter through a pair of diodes (1n5822). This way the battery remains inactive (just maintaining charge with a battery tender) until the mains brick goes down. When the mains brick is off (measured by a divider on the brick output), the battery will take over supplying current. When mains power returns, the primary supply takes over (fed through the big UPS), and the battery tender recharges the battery.

This DIY UPS system is in turn plugged in to a bigger name brand UPS, which also powers some other servers in the home. Hence the primary supply sensor is required to monitor the entire power system status. (in a nutshell: * mains -> big UPS -> [14VDC supply or 12V batt] -> 5V buck -> Pi *) This way your hass can react to UPS, mains, and backup battery status without needing complicated UPS management software.*
