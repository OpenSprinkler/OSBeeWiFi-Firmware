# OpenSprinkler Bee WiFi firmware 1.0.3

Download [osbee_1.0.3.bin](osbee_1.0.3.bin) and upload it through the device's firmware update page. For USB recovery instructions, see the [repository README](../../README.md#update-firmware).

## Changes from 1.0.2

- Change the default valve boost voltage on hardware v3 to 9V, suitable for common 9V latching valves. Explicit boost voltage settings of at least 5V still take precedence.
- Update the ESP8266 build platform and OpenThings Framework (OTC) library.
- Minify and gzip-compress the embedded web pages to reduce flash storage and transfer size, and enable browser caching.
- Generate the embedded HTML automatically during PlatformIO builds using Node.js.

## User manual and API reference

Firmware 1.0.3 retains the firmware 1.0.2 API and user workflows. Continue to use the existing documentation:

- [User manual (1.0.2)](../firmware1.0.2/OSBeeManual1.0.2.pdf)
- [API reference (1.0.2)](../firmware1.0.2/OSBeeAPI1.0.2.pdf)

The default boost voltage change above supersedes any earlier default described in the manual.

## Binary checksum

SHA-256 of `osbee_1.0.3.bin`:

```text
c3e8478c7b1364651f9858c98ee425a70329206cfaef10b2e2a2e0440d2ecfae
```
