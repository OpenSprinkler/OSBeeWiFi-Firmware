
This folder contains the OpenSprinkler Bee WiFi (OSBeeWiFi) firmware code for Arduino with ESP8266 core. Below are some screen shots of the user interface. For details, visit [http://bee.opensprinkler.com](http://bee.opensprinkler.com)


<img src="Screenshots/osbee1.png" height=240> <img src="Screenshots/osbee5.png" height=240> <img src="Screenshots/osbee4.png" height=180> <img src="Screenshots/osbee3.png" height=180> <img src="Screenshots/osbee6.png" height=180> <img src="Screenshots/osbee7.png" height=180> <img src="Screenshots/osbee2.png" height=180> <img src="Screenshots/osbee8.png" height=180> <img src="Screenshots/osbee9.png" height=180>

Updates:
=======
* (10/17/2022) Firmware 1.0.2 for OSBeeWiFi (support for both Blynk and OTC)
* (02/09/2021) Update firmware for OSBeeWiFi version 3.0
* (12/05/2016) Release first version of OSBeeWiFi firmware


Firmware Compilation Instructions
=======
Check README.md in the `OSBeeWiFi` folder.

Update Firmware:
=========

As OSBeeWiFi firmware supports OTA (over-the-air) update, it's highly recommended that you upload the firmware through the web interface.

If OTA updates fails, you can still upload the firmware through USB instead. To do so:

* First, install CH340 driver:
  - Mac OSX: install the driver from http://raysfiles.com/drivers/ch341ser_mac.zip
  - Linux: (dirver is not needed, make sure you run Arduino as root, or add 1a86:7523 to udev rules).
  - Windows: driver is only needed for Win 7 64-bit or Win XP (http://raysfiles.com./drivers/ch341ser.exe)

* Let OSBeeWiFi enter bootloading mode. Specifically:
  - Unplug the USB cable
  - Press and hold the pushbutton on OSBeeWiFi while plugging in the USB cable
  - Release the pushbutton (the LED should stay on)
  The LCD screen should remain off. If the LCD screen lights up, it has failed to enter bootloading mode. Just repeated the procedure. Every time you upload a new firmware through USB, you need to let it enter bootloading mode first.

* If you have compiled the firmware using the makeEspArduino method, you can use the Makefile to upload firmware. First, check the USB serial port name:
  - Mac OSX: the port name is typically /dev/tty.wchusbserial####. You can run 'ls /dev/tty.wch*' to find out.
  - Linux: the port name is typically /dev/ttyUSB#. You can run 'ls /dev/ttyUSB*' to find out.
Next, modify the Makefile to change the serial port name to match yours. Then run 'make upload' in the OSBeeWiFi folder. 

* If you have compiled In Arduino, press Ctrl + U to start uploading.


Firmware Features:
=================

The firmware supports a built-in web interface (which you can access using the device's local IP) as well as remote access using either Blynk legacy app, or OpenThingsCloud (OTC) connection.

* Initial Connection to WiFi:

On first boot-up, the firmware starts in AP mode, creating a WiFi network named OSB_XXXXXX where XXXXXX is the last six digits of the MAC address. This is an open AP with no password. The LCD displays the AP name and a dot blinks quickly at about twice per second. Using your phone or computer to connect to this AP, then open a browser and type in

192.168.4.1

to access the AP homepage. If you use Android phone, it may warn you about 'No Internet Connection'. Simply click the checkbox to confirm.

Select (or manually type in) the desired SSID, password, and (optionally) Blynk's authorization token (refer to the instructions above). If you don't want to input the Blynk token, you can leave it empty for now. Then click 'Submit'. Wait for 15 to 30 seconds for the device to connect to your router. If successful, it will display the local IP address with further instructions.

* For details of the other firmware features, please check the User Manual in the docs folder.


Update Firmware:
===============

The current firmware version is displayed at the homepage. When a new firmware becomes available, you can click on the 'Update' button on the homepage to upload a new firmware.

If the update ever fails, power cycle (unplug power and plug back power) the device and try again. If it still doesn't work, you can update firmware through the USB port (see instructions above).



