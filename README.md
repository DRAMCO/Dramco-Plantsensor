# Dramco Plantsensor
![Blinking plant sensor](https://github.com/DRAMCO/Dramco-Plantsensor/blob/main/Hardware/Pictures/Blinking.gif?raw=true)

## How it works
The Dramco Plantsensor measures the moisture level of the soil every 30 seconds. When the soil is dry, the green LED will start blinking (first rapidly to get your attention, after a while it will slow down to every 30 seconds). The battery is running low when the red LED also starts blinking. If you have the rechargeable version, you can recharge using the micro USB connection on top. If not, replace the battery (you can also power the sensor of a USB cable). 

## How to assemble
When you get your Dramco Plantsensor, some components still need to be soldered. Solder all through hole components to complete the sensor. More information in this pdf. 

## How to program
To program the Dramco Plantsensor, you'll need a [Dramco Uno as ATTINY85 programmer](https://create.arduino.cc/projecthub/arjun/programming-attiny85-with-arduino-uno-afb829), some [POGO pins](https://benl.rs-online.com/web/p/test-pins/1613648/?cm_mmc=BE-PLA-DS3A-_-google-_-CSS_BE_NL_Test_%26_Measurement_Whoop-_-(BE:Whoop!)+Test+Probes-_-1613648&matchtype=&pla-339422850668&gclid=Cj0KCQjwwNWKBhDAARIsAJ8HkhclvIvYQJyAc5_Ah08zPnTl_AxyPAWuMrbqTGvm6uMTI3wQCO1g0-QaAm2mEALw_wcB&gclsrc=aw.ds) and the [Arduino IDE](https://www.arduino.cc/en/software) installed. The pinout of the programming pads, is shown in the schematics. Adapt and use the following command to program: 
 ```
 "C:\Program Files (x86)\Arduino\hardware\tools\avr/bin/avrdude" -C"C:\Program Files (x86)\Arduino\hardware\tools\avr/etc/avrdude.conf" -v -pattiny85 -cstk500v1 -PCOM4 -b19200 -Uflash:w:"Soil_sensor.hex":i -U lfuse:w:0xF1:m -U hfuse:w:0x57:m -U efuse:w:0xFE:m
 ```
(!) This command disables the RESET pin of the ATTINY85, use only if you have a [high voltage flash programmer](https://td0g.ca/2020/04/13/high-voltage-flash-programming-on-attiny85/).