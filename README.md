
This folder contains the OpenSprinkler Bee WiFi (OSBeeWiFi) firmware code for Arduino with ESP8266 core. Below are some screen shots of the user interface. For details, visit [http://bee.opensprinkler.com](http://bee.opensprinkler.com)


<img src="Screenshots/osbee1.png" height=180> <img src="Screenshots/osbee6.png" height=180> <img src="Screenshots/osbee5.png" height=180> <img src="Screenshots/osbee4.png" height=180> <img src="Screenshots/osbee3.png" height=180> <img src="Screenshots/osbee7.png" height=180> <img src="Screenshots/osbee8.png" height=180>

Updates:
=======
* (Dec 5) Release OSBeeWiFi firmware 1.0.0


Preparation:
===========

* Install makeEspArduino (Method 1 below), or Arduino 1.6.x with ESP8266 core (Method 2 below)
* Blynk library for Arduino (https://github.com/blynkkk/blynk-library)
* SSD1306 library (https://github.com/squix78/esp8266-oled-ssd1306)
* This (OSBeeWiFi) library


Compilation:
===========

#### Method 1: use makeEspArduino

If you are familiar with Makefile, I highly recommend you to use makeEspArduino:

https://github.com/plerup/makeEspArduino

Follow the instructions to install,and run get.py to get ESP8266 core files. Then copy the Blynk, SSD1306, and OSBeeWiFi libraries to the 'libraries' subfolder in your local esp8266 directory. The OSBeeWiFi library has an example Makefile which you can modify to match your specific path.

You also need to modify two files in the ESP8266 core and Blynk library. See README.txt in the Modifications folder for details. Then simply run 'make' in the OSBeeWiFi folder to compile the programs. The compiled firmware (named mainArduino.cpp.bin) by default exists in a temporary folder.

#### Method 2: use Arduino IDE

Go to http://arduino.cc to download and install the Arduino software. Then follow the instructions at:

https://github.com/esp8266/Arduino

to install the ESP8266 core for Arduino. Next, copy the Blynk, SSD1306, and OSBeeWiFi libraries to your Arduino's 'libraries' subfolder. You also need to modify two files in the ESP8266 core and Blynk library. See README.txt in the Modifications folder for details.

To compile, launch Arduino, select:

* File -> Examples -> OSBeeWiFi -> mainArduino.
* Tools -> Board -> Generic ESP8266 Module (if this is not available, check if you've installed the ESP8266 core).
* Tools -> Flash Size -> 4M (1M SPIFFS).

Press Ctrl + R to compile. The compiled firmware (named mainArduino.cpp.bin) by default exists in a temporary folder.


Upload Firmware:
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

The firmware supports a built-in web interface (which you can access using the device's local IP) as well as remote access using the Blynk app. 

* Install Blynk App:

It's recommended that you install the Blynk app, and scan the QR code (in the BlynkApp folder) before proceeding, in order to enable remote access right away. Once you have created the OSBeeWiFi app, you can go to the project settings to copy the authorization token.


* Initial Connection to WiFi:

On first boot-up, the firmware starts in AP mode, creating a WiFi network named OSB_XXXXXX where XXXXXX is the last six digits of the MAC address. This is an open AP with no password. The LCD displays the AP name and a dot blinks quickly at about twice per second. Using your phone or computer to connect to this AP, then open a browser and type in

192.168.4.1

to access the AP homepage. If you use Android phone, it may warn you about 'No Internet Connection'. Simply click the checkbox to confirm.

Select (or manually type in) the desired SSID, password, and (optionally) Blynk's authorization token (refer to the instructions above). If you don't want to input the Blynk token, you can leave it empty for now. Then click 'Submit'. Wait for 15 to 30 seconds for the device to connect to your router. If successful, it will display the local IP address with further instructions.


* Reset to Factory Default:

At any time, you can press and hold the pushbutton for at least 8 seconds, until the LCD displays the 'Resetting' message. At this point, the device will perform factory reset and restart in AP mode.


* Using Blynk App:

Assuming you've input the Blynk authorization token during the set up, you can now open the Blynk app and it should show the device status (e.g zone status, history etc.) in the Status tab, and control options in the Control tab. If any zone is running, you can click on 'Reset All' button in the Status tab to turn of all running zones.


* Using the Built-in Web Interface:

Open a browser and type in the device's local IP address. The build-in web interface allows you to:

  - Check the device status
  - Change settings
  - Trigger zone actions
  - Set programs and preview programs
  - Manually run a program
  - View log
  - Reboot the device
  - Update firmware
  
Certain operations require a device key, which by default is

opendoor

In Settings, you can change the time zone, device name, zone names, Blynk token, HTTP port, and device key etc. Changing certain options (such as token, HTTP port) requires a reboot.



Update Firmware:
===============

The current firmware version is displayed at the homepage. When a new firmware becomes available, you can click on the 'Update' button next to the firmware version to upload the new firmware.

If the update ever fails, power cycle (unplug power and plug back power) the device and try again. If it still doesn't work, you can update firmware through the USB port (see instructions above).


Notes about Modifying Firmware:
===============
You can change the firmware code to add new features and follow the instructions above to compile and upload. Note that the built-in user interface embeds HTML files to the firmware source code. To modify the HTML files (so that you can change how the webpages are rendered), go to the html/ subfolder, which contains the original HTML files. Make any changes necessary, and use the html2raw tool to convert these HTML files into program memory strings (which are saved in htmls.h). Every time you make changes to the HTML files, you must re-run html2raw in the html folder to re-generate the program memory strings.
