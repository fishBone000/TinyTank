# TinyTank
A Arduino PDA worn on arm. It has RTC, OLED, 4X4 keypad and a MicroSD reader.
![Main Menu View](https://github.com/fishBone000/TinyTank/blob/master/Pictures/MainMenu.JPG)
More in Pictures!
# Features
- Hardware Shell:  Makes TinyTank wearable and provides some protection from water, also provides space for Li-battery and an external power switch. It's also 3D printed, making it fully costomizable.
- TinyTankShield: A PCB circuit for carrying other modules. It also carrys RTC clock circuit directly on itself (The original RTC module costs too much space, so I moved it directly to the shield.
- RTC: Using DS3231M, TinyTank displays current time at the right up corner of the OLED. 
- Temperature: With builtin temperature sensor of DS3231M, TinyTank updates & displays temperature at the left up corner of the OLED, with accuracy within 3 Celsius. 
- SDExplorer: (Queued) Allows users to explore a MicroSD card and modify files in it.  
- Reminder: (Queued) Based on SDExplorer, it saves editable reminders in the MicroSD card. It's not Alarm though.  
- Timer: (Halfly done) A pausable timer for counting seconds and minutes. Resetting still not added yet.
- Powersaving: 2 power saving modes allow TinyTank to display time only or turn off the OLED after certain seconds.  
- Critical Powersaving: (Queued) Displays and updates clock only after enabling.   
# Note
This project is under development. Due to the burden of my reality life recently, its development may be slow or even paused in 3 months.  
The code structure and other things are not optimized much. I even don't have much time to pick a license. But I will do it once I have enough time.  
# License
Anyone can use, modify, distribute it for any purpose except commercial purposes.  
Anyone who reproduce, distribute it should credit this original project page.  
