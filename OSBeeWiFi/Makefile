SKETCH = ./osbeeMain.ino

LIBS = . \
$(ESP_LIBS)/Wire \
$(ESP_LIBS)/SPI \
$(ESP_LIBS)/ESP8266WiFi \
$(ESP_LIBS)/ESP8266mDNS \
$(ESP_LIBS)/DNSServer \
$(ESP_LIBS)/ESP8266WebServer \
$(ESP_LIBS)/ESP8266HTTPClient \
$(ESP_LIBS)/Ticker \
$(ESP_LIBS)/Hash \
~/Arduino/libraries/SSD1306 \
~/Arduino/libraries/Blynk \
~/Arduino/libraries/OTF-Controller-Library \
~/Arduino/libraries/WebSockets \

ESP_ROOT = $(HOME)/esp8266_2.7.4/
ESPCORE_VERSION = 274
BUILD_ROOT = /tmp/$(MAIN_NAME)

UPLOAD_SPEED = 460800
UPLOAD_VERB = -v

FLASH_DEF = 4M2M
FLASH_MODE = dio
FLASH_SPEED = 40
F_CPU = 80000000L

BOARD = generic

include ./makeEspArduino.mk

