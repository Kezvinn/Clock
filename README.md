# Clock DS1302

Product hardwares:
 - LCD 16x2
   - I2C commincation to display time and other coorespond message
 - TTP223 - 3 units
   - Touch sensor act as a button for setting time and switching mode in the system
 - DS1302
   - Main module to keep time
   - Use UART to communicate to set and read time.

How to use:
 -  By default, the LCD display a welcome message when first start up.
 -  The state machine of the system go back and forth between clock mode and setting mode (date and time)
   - B1 is to switch between the 3 main modes
   - B2 is to switch between  day-> date-> month-> year or hour -> min depend on the current mode of the program
   - B3 is to increase the value of each variable in setting
 - After setting done, press B1 to to back to clock mode and view the time.  

Notes:
- The project can be implemented with buzzer to create arlarm clock
- Other implementation can be done using this framework of DS1302 clock
