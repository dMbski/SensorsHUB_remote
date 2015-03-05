Sensors station by dMb (2015)
Supported:
-DHT22 sensor (Digital2)
-Maxim OneWire (Digital3) DS18b20 in 12bit resolution.
It is part of collection of environmental data system
but it usable as standalone sensor hub for PC.

All values in Remote_mode and returned by R command (voltage, temps, humidity) are mult by 10. To get proper value divide them by 10. (ex: 48= 4.8 235= 23.5)

To switch into terminal mode connect D8 and D7.
Configuration can only be done under teminal mode.

Remote base commands:
***Terminal mode
?	Prints commands list.
p	Prints Sensors table
l	Load Sensors table from flash
s	Save Sensors table to flash
c	Clear Sensors table.
w	Write Sensor's addresHEX to table
a	Add next Sensor to table
r	Read Sensor by Index (r2)
R	Read Senor only value (R2)
o	Scan OneWire bus.
v	Voltage
h	Humidity %
t	Temps in deg Celc

HowTo:
1.Clear ST
2.Sensors 0-2 are added by default
3.Scan DS bus (type o)
4.Add sensor to ST (type aHEXaddres)
5.Check ST (print- type p)
6.Save table to flash (type s)
Reset then and check ST after reboot. 

***Remote mode	RM_REMOTE
Command returns # at the end of trasmission (there isn't CR nor LF).
##	Echoes #
#Rx Return x Sensor value (#R0 = 49# #R3= 102#)
#Vx Return x sensor value info:
		C - temperature value in deg. C
		H - percentage of humidity
		V - voltage V
	(#V0 = C# #V1 = H# #V2 = C#)
#Mx Return sensors addres data (#M1 = _2345678#)
	For above commands 0 <= x < Count sensors
#c	Count sensors (#c = 2#)

You can use #Mx command to get family code of sensor. Sensors 0-2 family 5F.
To check if sensor hub can accept commands, just send '#' and stop when You get response #.

Sensor type by IDX (0-2 set as default after clear):
0	Voltage info
1	Sensor DHT22 humidity
2	Sensor DHT22 temp
3...9	DS18b20 temp sensors 


	