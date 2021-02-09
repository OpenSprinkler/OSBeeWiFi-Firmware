SKETCH ?= ./examples/osbeeMain/osbeeMain.ino
LIBS = . \
  $(ESP_LIBS)/Wire \
  $(ESP_LIBS)/SPI \
	$(ESP_LIBS)/ESP8266WiFi \
	$(ESP_LIBS)/ESP8266mDNS \
	$(ESP_LIBS)/ESP8266WebServer \
	~/Arduino/libraries/Blynk \
	~/Arduino/libraries/SSD1306 \

ESP_ROOT = $(HOME)/esp8266_2.7.4/
ESPCORE_VERSION = 274
BUILD_ROOT = /tmp/$(MAIN_NAME)

UPLOAD_SPEED = 460800
UPLOAD_VERB = -v
# for OSbee 3.0: reset mode is nodemcu
UPLOAD_RESET = nodemcu
# Uncomment the line below for OSbee 2.0: reset mode is ck
# UPLOAD_RESET = ck

FLASH_DEF = 4M2M
FLASH_MODE = dio
FLASH_SPEED = 40

BOARD = generic

include ./makeEspArduino.mk

