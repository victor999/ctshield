# CtShield

Arduino energy monitor shield sketch for measuring energy consumption.
Data logged to openenergymonitor.org - you need to open account there. Alternatively you can clone emoncms to your hosting and send data to your emoncms.

Project page: https://hackaday.io/project/8505-energy-monitor-with-arduino

Arduino used: MEGA, because of it's 2 serial ports.

Libraries used in sketch (you should install them):

https://github.com/adafruit/Adafruit_SSD1306

(modify H file: enable #define SSD1306_128_64 and disable #define SSD1306_128_32)

https://github.com/adafruit/Adafruit-GFX-Library

https://github.com/openenergymonitor/EmonLib

https://github.com/itead/ITEADLIB_Arduino_WeeESP8266


After download - replace

char g_SSID[] = "YOUR_WIFI_SSID";
char g_PASSWORD[] = "YOUR_WIFI_PASSWORD";

to your wifi SSID and password.