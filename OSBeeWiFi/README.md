Firmware Compilation Instructions
===========

There are three methods to compile the firmware. You can choose any of them.

Method 1: Use PlatformIO
=======

Install Visual Studio Code (VS Code) (https://code.visualstudio.com/). Then launch VS Code and install the **platformio** extension. Once the extension is installed, open this `OSBeeWiFi` folder. A `platformio.ini` file is provided in the folder, which defines all libraries and settings required to compile the firmware. Click `PlatformIO:Build` at the bottom of the screen to build the firwmare.


Method 2: Use makeEspArduino and Makefile
=======

#### Step A: Install ESP8266 core and required libraries
Follow https://github.com/plerup/makeEspArduino to install **ESP8266 core 2.7.4**. Then download or `git clone` the following libraries to a folder such as `$(HOME)/Arduino/libraries` (note the version of each library):

* Blynk library for Arduino v1.0.1 (https://github.com/blynkkk/blynk-library/releases/tag/v1.0.1) 
* SSD1306 library v4.2.1 (https://github.com/ThingPulse/esp8266-oled-ssd1306/releases/tag/4.2.1) 
* OpenThingsFramework library ()https://github.com/OpenThingsIO/OpenThings-Framework-Firmware-Library)
* WebSocket library v2.3.5 (https://github.com/Links2004/arduinoWebSockets/releases/tag/2.3.5)
* This (OSBeeWiFi) source code (download the repository and extract the `OpenGarage` folder to your Arduino's `libraries` folder.)

#### Step B: Use the Makefile
To compile the firmware code using makeESPArduino, simply run `make` in command line. Before doing so, you probaby need to open `Makefile` and modify some path variables therein to match where you installed the `esp8266` folder.

Method 3: Use the Arduino IDE
=======

#### Step A: Install Arduino IDE, ESP8266 core, and required libraries
* Install Arduino IDE (https://arduino.cc).
* Launch Arduino IDE and install ESP8266 core 2.7.4 for Arduino (https://github.com/esp8266/Arduino/releases/tag/2.7.4)
* Install the libraries listed above.
* Download this repository and extract the `OpenGarage` folder to your Arduino's `libraries` folder.

#### Step B: Compile
To compile, launch Arduino, select:

* File -> Examples -> OSBeeWiFi -> osbeeMain.
* Tools -> Board -> Generic ESP8266 Module (if this is not available, check if you've installed the ESP8266 core).
* Tools -> Flash Mode -> DIO
* Tools -> Flash Size -> **4M (2M SPIFFS)**.

Press Ctrl + R to compile. The compiled firmware (named osbeeMain.cpp.bin) by default exists in a temporary folder.

Notes about Modifying Firmware:
=======

You can change the firmware code to add new features and follow the instructions above to compile and upload. Note that the built-in user interface embeds HTML files to the firmware source code. To modify the HTML files (so that you can change how the webpages are rendered), go to the html/ subfolder, which contains the original HTML files. Make any changes necessary, and use the html2raw tool to convert these HTML files into program memory strings (which are saved in htmls.h). Every time you make changes to the HTML files, you must re-run html2raw in the html folder to re-generate the program memory strings.
