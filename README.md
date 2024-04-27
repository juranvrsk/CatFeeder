**The cat feed box. **
The project for simple cad feed box.
It is just a box, could be openned by schedule or manually.
The box are quite simple: i made it from planks. The cap of box made from the tinner planks
The cap autiomation working like a retro boom barrier: the cap is normally openned by counterweight and gravitation.
In closed state the cap held by moving hook. The hook is driven by solenoid actuator.
The control system is based on NodeMCU(ESP8266). Due to the high current demand of an actuator i installed the transistor and diode snubber.
The Indication based on the LED. Overall voltage is 12V.

The software contains:
 - The self realization of pseudo-RTC. It sync with NTP service every hour.
 - Yes, i avoid to use espressif time libraries: it is too difficult, where you need three simple periods: the 2 minutes long for LED, the one hour long for NTC sync, and one day long for open cap;
 - The web page, which parse open time, indicates current time, acutator state and receives manual openning command.
 - The soft powering and de-powering the actuator coil: it basen on PWM, and slowly,during one second, rising up voltage on the coil, wait for half second, then slowly lowering down the woltage for one second.

