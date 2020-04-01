# Corona Tracker

Got my M5 Core while in quarantine.

The corona crisis made for an excellent first project with it



## Getting started

Follow the instructions here https://docs.m5stack.com/#/en/arduino/arduino_development

to get Arduino installed with support for the M5 (ESP32, USB Driver)

 * Open the sketch in Arduino and install the following libraries
   * M5 Stack
   * ArduinoJson

make sure M5-Stack-Core-ESP32 is selected as the board and the correct port selected

Copy secrets.h.template to secrets.h and update with your Wifi credentials

Register an account at rapidapi.com and make sure the API Key is added in secrets.h

now you should be able to compile the sketch and upload it to your M5


