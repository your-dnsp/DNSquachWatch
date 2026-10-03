# Investigation notes — v0.10.4

The v0.10.3 application did not run an LDR brightness-control loop. The user's observed correlation between covering the sensor, dimming and resetting is real hardware feedback, but source inspection alone cannot establish its cause.

The previous backlight setup configured channels 0, 1 and 2 at eight bits and attached GPIO21, GPIO27 and GPIO32 on the ordinary CYD path. GPIO32 is resistive touch MOSI (or capacitive touch SCL). Status LED channel 3 uses twelve bits. The pinned Arduino ESP32 HAL selects LEDC timers as `(channel / 2) % 4`, making channels 2 and 3 share a timer. v0.10.4 selects only the actual backlight pin and channel 0, leaving the RGB timers separate.

The new ambient-light control uses GPIO34 (ADC1) for the ordinary CYD, defaults OFF, and substitutes raw 200 when OFF. ON applies a 1/4 filter every 100 ms; PWM moves by at most four duty steps per update. The uncalibrated mapping treats 200 as a medium-low reference, with a legibility floor and the user's brightness ceiling. No sensor value changes the SPI clock or reallocates the framebuffer.

Board pin reference: https://nuttx.apache.org/docs/latest/platforms/xtensa/esp32/boards/esp32-2432S028/index.html
Divider direction reference: https://github.com/kthxbyte/platformio-esp32-2432s028
PWM evidence: pinned `cores/esp32/esp32-hal-ledc.c` in Arduino-ESP32 2.0.17, timer selection in ledcSetup/ledcAttachPin.

Hardware confirmation of the reset fix remains necessary. The new report path and retained core dumps can supply better evidence if the reset repeats. No automatic reboot or destructive diagnostic is triggered by the light sensor.
