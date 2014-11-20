Fuel guage description
====

Overview
----

In several projects I have had a need to measure current consumption on battery-powered devices, and there has never been any really good way, no apparatus or tools I found have proven sufficient. 
The typical use case is when you develop a low-power product with a battery that should last for a long time, but with potentially high peak currents, for instance during radio transmission bursts. 
You need to understand how much power your product uses during normal operation in order to optimize the hardware and firmware for lowest power consumption, but there is no way without having high 
resistors in series with your battery, which ruins operation if the device for instance needs to do a radio burst. 

I did this as a simple measurement shield for Arudino Uno, the schematics and layout is in this GitHub repository, together with the Arduino firmware. 

Current features
----
* 1 uA – 1 A measurement range
* No switching series resistors that could interfere with operation, the entire range measured using the same 50 mOhm resistor
* Data output on serial port to PC showing momentary voltage and current

Ideas to future improvements
----
* Display accumulated energy use (mAh or mWh) since measurement start
* Time stamped logging to SD-card shield
* Output on display shield for standalone operation
* Make calibration sticky (may not be neccessary to calibrate on every boot) - store in eeprom
* Increase max voltage range by having a high-impedance divider _before_ the unity-gain OP instead of after. (hits maximum input voltage on the OP; VCC-1,5V)

All of those are quite easy, I just didn’t find the time yet. 


Hardware design description
----
The design is quite straight forward, a low-side current measurement resistor (50 mOhm) being amplified using three separate amplifiers and A/D converters, giving three ranges: 
* 0 – 500 uA, 500 nA, theoretical resolution on the Arduino 10-bit A/D
* 500 uA – 20 mA, theoretical resolution 20 uA
* 20 mA – 1 A, theoretical resolution 2 mA

Also a fourth channel measures the voltage through a standard voltage divider. See the schematics for the measurement shield for details. 

All amplifiers have a first order low-pass filter (~10 Hz) to even out any spikes higher than the Arduino sample frequency (100 Hz).  

The rest should be self-explanatory by looking at the schematics. 

Hardware revisions
----
There are two revisions, where rev A was done in a hurry and had a few quirks, don't use that one. Rev B has the correct schematics but I never made a PCB from that one, it was easier to build rev B using the rev A board, with some patches. 

Firmware design description
----
The Arduino Uno samples these four channels every 10 ms (exactly!), chooses the range automatically and outputs the results on a serial port. 

Measured accuracy
----
Note that since three amplifiers are used, the inaccuracy varies across the range – measurements close to the range breaking points are less accurate than other points, which means there is no true general accuracy statement. 
This is what I measured on the first sample: 

(Reference multimeter | DUT | error)  
Range 0:  
 9 uA  |    8 uA  |  <12%  
13 uA  |   11 uA  |  <16%  
19 uA  |   17 uA  |  <11%  
30 uA  |   29 uA  |  <4%  
46 uA  |   46 uA  |  <1%  
89 uA  |   89 uA  |  <1%  
212 uA  |  211 uA  |  <1%  
252 uA  |  249 uA  |  <2%  
325 uA  |  320 uA  |  <2%  
424 uA  |  419 uA  |  <2%  

Range 1:  
505 uA  |  516 uA  |  <3%  
613 uA  |  626 uA  |  <3%  
917 uA  |  921 uA  |  <1%  
1197 uA |  1198 uA |  <1%  
1704 uA |  1714 uA |  <1%  
11,3 mA |  11,1 mA |  <2%  

Range 2:  
20,8 mA |  21 mA   |  <1%  
51,4 mA |  52 mA   |  <2%  
96,0 mA |  95 mA   |  <2%  
235 mA  |  233 mA  |  <1%  
0,53 A  |  514 mA  |  <3%  
0,71 A  |  697 mA  |  <2%  
0,88 A  |  873 mA  |  <1%  

Voltage:  
285 mV  |   275 mV | <4%  
1,726 v |  1755 mV | <2%  
2,327 V |  2375 mV | <1%  
3,34 V  |  3415 mV | <3%  
3,60 V  |  3680 mV | <3%  

So in general, better than 4% accuracy can be expected in average. Accuracy is worse in the lower area of each range (due to limited resolution), and at some points it might theoretically be worse than 4%.

Other tests carried out
----
* step response, made sure LP filters on all three ranges has time constant well above the 10 ms sampling rate. 
* tested that variations in supply voltage does not affect readings

