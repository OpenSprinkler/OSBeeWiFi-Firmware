This folder contains modifications to files in other libraries. Copy these files to the corresponding folders to overwrite the existing files there.

* BlynkSimpleEsp8266.h: (under Blynk library folder)

Added an additional begin() function that does not require ssid and password. This allows the OpenGarage main loop to manage WiFi connection.


* Updater.h: (under esp8266/x.x.x/cores/esp8266 folder)

Added a public reset function to allow OTA update to abort and reset upon error.

