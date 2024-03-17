
/*
Creator: Nhat Nguyen
Date: March 10, 2023
  Pin connection
  LCD Display   PORTC     A5-PORTC5:  SCL 
                          A4-PORTC4:  SDA               
                Address   0x3F      
  DS1302    PORTB:  -PORTB2:   CLK
                    -PORTB3:   (I/O) DAT
                    -PORTB4:   (CE)RST
  Button    PORTD:  -PORTD3:  MODE button
                    -PORTD5:  Digit select.
                    -PORTD7:  Value Select

*/
/*
  Acknowledgement:  Fucnction and code written to interface the device DS1302 is base on an DS1302 library
                  found on Arduino Playground at https://playground.arduino.cc/Main/DS1302/
                  Annything changes related to using arduino function and repalce with Bit manipulation method,
                  but otherwise, the program is unchange functionally
*/
#include <avr/io.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// LiquidCrystal_I2C lcd(0x3F, 16, 2);  // set the LCD address to 0x3F for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x3F for a 16 chars and 2 line display


// #define DS1302_S;E_PIN     17   // Arduino pin for the Chip Enable

// #define SCLK  PORTC2   //CLK
// #define DAT   PORTC1   //DAT
// #define CE    PORTC0   //RST

#define DS1302_SCLK_PIN 13  // Arduino pin for the Serial Clock
#define DS1302_IO_PIN 14   // Arduino pin for the Data I/O
#define DS1302_CE_PIN 15    // Arduino pin for the Chip Enable

// #define SCLK  15   //CLK
// #define DAT   14   //DAT
// #define CE    13   //RST


#define MODE_BUTTON   PORTD3
#define DIGIT_BUTTON  PORTD5
#define VALUE_BUTTON  PORTD7

// Macro for conversion between Binary Coded Decimals and binary
#define bcd2bin(h, l) (((h)*10) + (l))
#define bin2bcd_h(x) ((x) / 10)
#define bin2bcd_l(x) ((x) % 10)

//Defining Write command bytes as declared in the DS1302 datasheet
//To convert write command to read command, set LSB to '1'. i.e seconds write is 0x80 and seconds read is 0x81
#define DS1302_SECONDS              0x80
#define DS1302_MINUTES              0x82
#define DS1302_HOURS                0x84
#define DS1302_DATE                 0x86
#define DS1302_MONTH                0x88
#define DS1302_DAY                  0x8A
#define DS1302_YEAR                 0x8C
#define DS1302_ENABLE               0x8E
#define DS1302_TRICKLE              0x90
#define DS1302_CLOCK_BURST          0xBE
#define DS1302_CLOCK_BURST_WRITE    0xBE
#define DS1302_CLOCK_BURST_READ     0xBF
#define DS1302_RAMSTART             0xC0
#define DS1302_RAMEND               0xFC
#define DS1302_RAM_BURST            0xFE
#define DS1302_RAM_BURST_WRITE      0xFE
#define DS1302_RAM_BURST_READ       0xFF



//Defining the bits
#define DS1302_D0 0
#define DS1302_D1 1
#define DS1302_D2 2
#define DS1302_D3 3
#define DS1302_D4 4
#define DS1302_D5 5
#define DS1302_D6 6
#define DS1302_D7 7


// Bit for reading (bit in address)
#define DS1302_READBIT DS1302_D0  // READBIT=1: read instruction

// Bit for clock (0) or ram (1) area,
// called R/C-bit (bit in address)
#define DS1302_RC DS1302_D6

// Seconds Register
#define DS1302_CH DS1302_D7  // 1 = Clock Halt, 0 = start

// Hour Register
#define DS1302_AM_PM DS1302_D5  // 0 = AM, 1 = PM
#define DS1302_12_24 DS1302_D7  // 0 = 24 hour, 1 = 12 hour

// Enable Register
#define DS1302_WP DS1302_D7  // 1 = Write Protect, 0 = enabled

// Trickle Register
#define DS1302_ROUT0  DS1302_D0
#define DS1302_ROUT1  DS1302_D1
#define DS1302_DS0    DS1302_D2
#define DS1302_DS1    DS1302_D2
#define DS1302_TCS0   DS1302_D4
#define DS1302_TCS1   DS1302_D5
#define DS1302_TCS2   DS1302_D6
#define DS1302_TCS3   DS1302_D7


// Structure for the first 8 registers for use with clock burst
struct ds1302_struct {
  uint8_t Seconds : 4;    // low decimal digit 0-9
  uint8_t Seconds10 : 3;  // high decimal digit 0-5
  uint8_t CH : 1;         // CH = Clock Halt
  uint8_t Minutes : 4;
  uint8_t Minutes10 : 3;
  uint8_t reserved1 : 1;
  union {
    struct {
      uint8_t Hour : 4;
      uint8_t Hour10 : 2;
      uint8_t reserved2 : 1;
      uint8_t hour_12_24 : 1;  // 0 for 24 hour format
    } h24;
    struct {
      uint8_t Hour : 4;
      uint8_t Hour10 : 1;
      uint8_t AM_PM : 1;  // 0 for AM, 1 for PM
      uint8_t reserved2 : 1;
      uint8_t hour_12_24 : 1;  // 1 for 12 hour format
    } h12;
  };
  uint8_t Date : 4;  // Day of month, 1 = first day
  uint8_t Date10 : 2;
  uint8_t reserved3 : 2;
  uint8_t Month : 4;  // Month, 1 = January
  uint8_t Month10 : 1;
  uint8_t reserved4 : 3;
  uint8_t Day : 3;  // Day of week, 1 = first day (any day)
  uint8_t reserved5 : 5;
  uint8_t Year : 4;  // Year, 0 = year 2000
  uint8_t Year10 : 4;
  uint8_t reserved6 : 7;
  uint8_t WP : 1;  // WP = Write Protect
};
//74HC595
unsigned char LED_0F[] = {  // 0	 1	  2	   3	4	 5	  6	   7	8	 9	  A	   b	C    d	  E    F    -
  0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90, 0x8C, 0xBF, 0xC6, 0xA1, 0x86, 0xFF, 0xbf
};
char DAYOFWEEK[7][6] = { "MON ", "TUE ", "WED ", "THU ", "FRI ", "SAT ", "SUN " };
//                         0       1       2       3       4       5       6
char Am_Pm[2][3] = { "AM", "PM" };
char MONTHS[12][5] = { "JAN ", "FEB ", "MAR ", "APR ", "MAY ", "JUN ", "JUL ", "AUG ", "SEP ", "OCT ", "NOV ", "DEC " };

//Defining states for clock and display
enum time_state { IDLE_STATE,
                  SET_TIME_STATE,
                  SET_DAY_STATE,
                  READ_STATE };
enum time_state State;

enum num_state { MIN_0,
                 MIN_10,
                 HOUR_0,
                 HOUR_10,
                 AM_PM,
                 DAY,      //day of the week 1-6
                 DATE_0,   //date of the month
                 DATE_10,  //date of the month
                 MONTH,    //1-12
                 YEAR      //2020+
};
enum num_state Nstate;
//Digits for incrementing in set state
int set0 = 0;
int set1 = 0;
int set2 = 0;
int set3 = 0;
int am_pm_ip = 0;  //0 = AM, 1 = PM

int day_ip = 0;
int date_ip_0 = 1;
int date_ip_10 = 0;
int month_ip = 1;
int year_ip = 20;

byte _2dot[8] = {
  0b00000,
  0b00000,
  0b00100,
  0b00000,
  0b00000,
  0b00100,
  0b00000,
  0b00000
};
byte Diamond_empty[8] = {
  0b00000,
  0b00100,
  0b01010,
  0b10001,
  0b01010,
  0b00100,
  0b00000,
  0b00000
};
byte Diamond_full[8] = {
  0b00000,
  0b00100,
  0b01110,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};
byte Hourglass_empty[8] = {
  0b00000,
  0b11111,
  0b01010,
  0b00100,
  0b00100,
  0b01010,
  0b11111,
  0b00000
};
byte Heart_full[8] = {
  0b00000,
  0b01010,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000
};
byte Hourglass_turn[8] = {
  0b00000,
  0b10001,
  0b11011,
  0b10101,
  0b11011,
  0b10001,
  0b00000,
  0b00000
};
byte Spade[8] = {
  0b00100,
  0b01110,
  0b11111,
  0b11111,
  0b01110,
  0b00100,
  0b01110,
  0b00000
};

int main(void) {
  Serial.begin(9600);
  Serial.println("program start.");
  //  Serial.print(hr);
          Serial.print(":");
          // Serial.print(mi);
          Serial.print(":");
          // Serial.println(sc);

  init();
  lcd.init();
  lcd.init();

  Nstate = MIN_0;
  State = IDLE_STATE; //idle state

  DDRD &= ~((1 << VALUE_BUTTON) | (1 << MODE_BUTTON) | (1 << DIGIT_BUTTON));  //3 buttons
  ds1302_struct rtc;                                         // Create a ds1302_struct variable for use with writing and reading data from DS1302
  
  lcd.init();
  lcd.clear();
  lcd.backlight();           // Make sure backlight is on
  lcd.createChar(1, _2dot);  //:
  
  lcd.createChar(2, Heart_full);
  
  lcd.createChar(7, Spade);

  lcd.createChar(4, Diamond_empty);  //
  lcd.createChar(5, Diamond_full);   //

  lcd.createChar(11, Hourglass_empty);         //-11
  lcd.createChar(14, Hourglass_turn);    //14

  int seconds = 0, minutes = 0, hours = 0, dayofweek = 0, dayofmonth = 0, month = 0, year = 0;

  while (1) {
    if ((PIND & (1 << MODE_BUTTON))) {  // Change mode
      _delay_ms(300);              // debounce
      if (State == IDLE_STATE) {   //ilde->read
        State = READ_STATE;
      } else if (State == READ_STATE) {  //read ->set time
        State = SET_DAY_STATE;
        Nstate = DAY;
      } else if (State == SET_DAY_STATE) {  //set time -> set day
        State = SET_TIME_STATE;
        Nstate = HOUR_10;
      } else if (State == SET_TIME_STATE) {  //set day -> read
        State = READ_STATE;
      }
    }
    if ((PIND & (1 << DIGIT_BUTTON))) {  // Select value
      _delay_ms(300);
      if (State == SET_TIME_STATE) {  // debounce
        if (Nstate == HOUR_10) {
          Nstate = HOUR_0;
        } else if (Nstate == HOUR_0) {
          Nstate = MIN_10;
        } else if (Nstate == MIN_10) {
          Nstate = MIN_0;
        } else if (Nstate == MIN_0) {
          Nstate = AM_PM;
        } else if (Nstate == AM_PM) {
          Nstate = HOUR_10;
        }
      }
      if (State == SET_DAY_STATE) {
        if (Nstate == DAY) {
          Nstate = DATE_10;
        } else if (Nstate == DATE_10) {
          Nstate = DATE_0;
        } else if (Nstate == DATE_0) {
          Nstate = MONTH;
        } else if (Nstate == MONTH) {
          Nstate = YEAR;
        } else if (Nstate == YEAR) {
          Nstate = DAY;
        }
      }
    }
    if ((PIND & (1 << VALUE_BUTTON))) {  // Increase number
      _delay_ms(300);              // debounce
      if (Nstate == MIN_0) {
        set0++;
        if (set0 > 9) {
          set0 = 0;
        }
      }
      if (Nstate == MIN_10) {
        set1++;
        if (set1 > 5) {
          set1 = 0;
        }
      }
      if (Nstate == HOUR_0) {
        set2++;
        if (set3 == 1 && set2 > 2) {  //above 12
          set3 = 1;
          set2 = 0;
        }
        if (set3 < 1 && set2 > 9) {  //under 12
          set2 = 0;
        }
      }
      if (Nstate == HOUR_10) {
        set3++;
        if (set3 > 1) {
          set3 = 0;
        }
        if (set3 == 1 && set2 > 2) {
          set2 = 2;
        }
      }
      if (Nstate == AM_PM) {
        am_pm_ip++;
        if (am_pm_ip > 1) {
          am_pm_ip = 0;
        }
      }

      if (Nstate == DAY) {  //day of the week: 0 = Monday
        day_ip++;
        if (day_ip > 6) {
          day_ip = 0;
        }
      }
      if (Nstate == DATE_0) {
        date_ip_0++;
        if (date_ip_10 == 3 && date_ip_0 > 1) {
          date_ip_10 = 3;
          date_ip_0 = 0;
        }
        if (date_ip_0 > 9) {
          date_ip_0 = 0;
        }
      }
      if (Nstate == DATE_10) {
        date_ip_10++;
        if (date_ip_10 == 3 && date_ip_0 > 1) {
          date_ip_10 = 3;
          date_ip_0 = 0;
        }
        if (date_ip_10 > 3) {
          date_ip_10 = 0;
        }
      }
      if (Nstate == MONTH) {
        month_ip++;
        if (month_ip > 12) {
          month_ip = 1;
        }
      }
      if (Nstate == YEAR) {
        year_ip++;
        if (year_ip > 30) {
          year_ip = 20;
        }
      }
    }
    switch (State) {
      case IDLE_STATE:
        {
          lcd.setCursor(2, 0);
          lcd.print("DS1302 CLOCK");
          lcd.setCursor(4, 1);
          lcd.print("Kevin N.");
          break;
        }
      case SET_TIME_STATE:
        {
          lcd.setCursor(10, 1);
          lcd.print("  ");  //clear charater
          int m;
          int h;
          lcd.setCursor(0, 1);
          lcd.write(4);  //setting logo
          switch (Nstate) {
            case MIN_0:
              //hours didplay
              lcd.setCursor(2, 1);
              lcd.print(set3);    //hour10
              lcd.print(set2);    //hour0
              lcd.write(1);  //lcd.print(":");
              lcd.print(set1);    //min10
              lcd.print(set0);    //min0
              lcd.write(1);  //lcd.print(":");
              if (rtc.Seconds10 == 0) {
                lcd.print("0");
              }
              lcd.print((rtc.Seconds10 * 10) + rtc.Seconds);
              lcd.setCursor(12, 1);
              lcd.print(Am_Pm[am_pm_ip]);

              //calendar display
              lcd.setCursor(1, 0);
              lcd.print(DAYOFWEEK[day_ip]);
              lcd.print(date_ip_10);
              lcd.print(date_ip_0);
              lcd.print("/");
              if (month_ip < 10) {
                lcd.print("0");
              }
              lcd.print(month_ip);
              lcd.print("/");
              lcd.print(2000 + year_ip);

              //display hourglass
              lcd.setCursor(15, 1);
              lcd.write(14);
              break;
            case MIN_10:
              lcd.setCursor(2, 1);
              lcd.print(set3);
              lcd.print(set2);
              lcd.write(1);  //lcd.print(":");
              lcd.print(set1);
              lcd.print(set0);
              lcd.write(1);  //lcd.print(":");
              if (rtc.Seconds10 == 0) {
                lcd.print("0");
              }
              lcd.print((rtc.Seconds10 * 10) + rtc.Seconds);
              lcd.setCursor(12, 1);
              lcd.print(Am_Pm[am_pm_ip]);

              lcd.setCursor(1, 0);
              lcd.print(DAYOFWEEK[day_ip]);
              lcd.print(date_ip_10);
              lcd.print(date_ip_0);
              lcd.print("/");
              if (month_ip < 10) {
                lcd.print("0");
              }
              lcd.print(month_ip);
              lcd.print("/");
              lcd.print(2000 + year_ip);

              //display hourglass
              lcd.setCursor(15, 1);
              lcd.write(14);
              break;
            case HOUR_0:
              lcd.setCursor(2, 1);
              lcd.print(set3);
              lcd.print(set2);
              lcd.write(1);  //lcd.print(":");
              lcd.print(set1);
              lcd.print(set0);
              lcd.write(1);  //lcd.print(":");
              if (rtc.Seconds10 == 0) {
                lcd.print("0");
              }
              lcd.print((rtc.Seconds10 * 10) + rtc.Seconds);
              lcd.setCursor(12, 1);
              lcd.print(Am_Pm[am_pm_ip]);

              lcd.setCursor(1, 0);
              lcd.print(DAYOFWEEK[day_ip]);
              lcd.print(date_ip_10);
              lcd.print(date_ip_0);
              lcd.print("/");
              if (month_ip < 10) {
                lcd.print("0");
              }
              lcd.print(month_ip);
              lcd.print("/");
              lcd.print(2000 + year_ip);

              //display hourglass
              lcd.setCursor(15, 1);
              lcd.write(11);
              break;
            case HOUR_10:
              lcd.setCursor(2, 1);
              lcd.print(set3);
              lcd.print(set2);
              lcd.write(1);  //lcd.print(":");
              lcd.print(set1);
              lcd.print(set0);
              lcd.write(1);  //lcd.print(":");
              if (rtc.Seconds10 == 0) {
                lcd.print("0");
              }
              lcd.print((rtc.Seconds10 * 10) + rtc.Seconds);
              lcd.setCursor(12, 1);
              lcd.print(Am_Pm[am_pm_ip]);

              lcd.setCursor(1, 0);
              lcd.print(DAYOFWEEK[day_ip]);
              lcd.print(date_ip_10);
              lcd.print(date_ip_0);
              lcd.print("/");
              if (month_ip < 10) {
                lcd.print("0");
              }
              lcd.print(month_ip);
              lcd.print("/");
              lcd.print(2000 + year_ip);

              //display hourglass
              lcd.setCursor(15, 1);
              lcd.write(11);
              break;
            case AM_PM:
              lcd.setCursor(2, 1);
              lcd.print(set3);
              lcd.print(set2);
              lcd.write(1);  //lcd.print(":");
              lcd.print(set1);
              lcd.print(set0);
              lcd.write(1);  //lcd.print(":");
              if (rtc.Seconds10 == 0) {
                lcd.print("0");
              }
              lcd.print((rtc.Seconds10 * 10) + rtc.Seconds);
              lcd.setCursor(12, 1);
              lcd.print(Am_Pm[am_pm_ip]);

              lcd.setCursor(1, 0);
              lcd.print(DAYOFWEEK[day_ip]);
              lcd.print(date_ip_10);
              lcd.print(date_ip_0);
              lcd.print("/");
              if (month_ip < 10) {
                lcd.print("0");
              }
              lcd.print(month_ip);
              lcd.print("/");
              lcd.print(2000 + year_ip);

              //display hourglass
              lcd.setCursor(15, 1);
              lcd.write(11);
              break;
          }
          h = (set3 * 10) + set2;
          m = (set1 * 10) + set0;

          DS1302_write(DS1302_ENABLE, 0);

          // Disable Trickle Charger.
          DS1302_write(DS1302_TRICKLE, 0x00);

          // int seconds, minutes, hours, dayofweek, dayofmonth, month, year;

          seconds = 0;
          minutes = m;  // Take minute from user
          hours = h;    // Take hour from user
          dayofweek = day_ip;
          // dayofmonth = DoM;
          // month = month_ip;
          // year = 2000 + year_ip;

          // Fill the structure with zeros to make
          // any unused bits zero
          memset((char *)&rtc, 0, sizeof(rtc));

          rtc.Seconds = bin2bcd_l(seconds);
          rtc.Seconds10 = bin2bcd_h(seconds);
          rtc.CH = 0;  // Start the clock by clearing the clcok halt bit
          rtc.Minutes = bin2bcd_l(minutes);
          rtc.Minutes10 = bin2bcd_h(minutes);
          //12hour format
          rtc.h12.Hour = bin2bcd_l(hours);
          rtc.h12.Hour10 = bin2bcd_h(hours);
          rtc.h12.AM_PM = am_pm_ip;  // AM = 0
          rtc.h12.hour_12_24 = 1;    // 1 for 24 hour format
                                     /*24h format
          // rtc.h24.Hour = bin2bcd_l(hours);
          // rtc.h24.Hour10 = bin2bcd_h(hours);
          // rtc.h24.hour_12_24 = 0;  // 0 for 24 hour format
    */
          rtc.Date = bin2bcd_l(dayofmonth);
          rtc.Date10 = bin2bcd_h(dayofmonth);
          rtc.Month = bin2bcd_l(month);
          rtc.Month10 = bin2bcd_h(month);
          rtc.Day = dayofweek;
          rtc.Year = bin2bcd_l(year - 2000);
          rtc.Year10 = bin2bcd_h(year - 2000);
          rtc.WP = 0;

          //Write clock data using burst writeDA
          DS1302_clock_burst_write((uint8_t *)&rtc);
          break;
        }
      case SET_DAY_STATE:
        {
          lcd.setCursor(0, 1);
          lcd.write(7);

          int DoM;  //date of month
          switch (Nstate) {
            case DAY:
              lcd.setCursor(2, 1);
              lcd.print(set3);
              lcd.print(set2);
              lcd.write(1);  //lcd.print(":");
              lcd.print(set1);
              lcd.print(set0);
              lcd.write(1);  //lcd.print(":");
              if (rtc.Seconds10 == 0) {
                lcd.print("0");
              }
              lcd.print((rtc.Seconds10 * 10) + rtc.Seconds);
              lcd.setCursor(12, 1);
              lcd.print(Am_Pm[am_pm_ip]);


              lcd.setCursor(1, 0);
              lcd.print(DAYOFWEEK[day_ip]);
              lcd.print(date_ip_10);
              lcd.print(date_ip_0);
              lcd.print("/");
              if (month_ip < 10) {
                lcd.print("0");
              }
              lcd.print(month_ip);
              lcd.print("/");
              lcd.print(2000 + year_ip);

              //display calendar
              lcd.setCursor(15, 1);
              lcd.write(2);

              break;

            case DATE_0:
              lcd.setCursor(2, 1);
              lcd.print(set3);
              lcd.print(set2);
              lcd.write(1);  //lcd.print(":");
              lcd.print(set1);
              lcd.print(set0);
              lcd.write(1);  //lcd.print(":");
              if (rtc.Seconds10 == 0) {
                lcd.print("0");
              }
              lcd.print((rtc.Seconds10 * 10) + rtc.Seconds);
              lcd.setCursor(12, 1);
              lcd.print(Am_Pm[am_pm_ip]);


              lcd.setCursor(1, 0);
              lcd.print(DAYOFWEEK[day_ip]);
              lcd.print(date_ip_10);
              lcd.print(date_ip_0);
              lcd.print("/");
              if (month_ip < 10) {
                lcd.print("0");
              }
              lcd.print(month_ip);
              lcd.print("/");
              lcd.print(2000 + year_ip);

              //display calendar
              lcd.setCursor(15, 1);
              lcd.write(2);

              break;
            case DATE_10:
              lcd.setCursor(2, 1);
              lcd.print(set3);
              lcd.print(set2);
              lcd.write(1);  //lcd.print(":");
              lcd.print(set1);
              lcd.print(set0);
              lcd.write(1);  //lcd.print(":");
              if (rtc.Seconds10 == 0) {
                lcd.print("0");
              }
              lcd.print((rtc.Seconds10 * 10) + rtc.Seconds);
              lcd.setCursor(12, 1);
              lcd.print(Am_Pm[am_pm_ip]);


              lcd.setCursor(1, 0);
              lcd.print(DAYOFWEEK[day_ip]);
              lcd.print(date_ip_10);
              lcd.print(date_ip_0);
              lcd.print("/");
              if (month_ip < 10) {
                lcd.print("0");
              }
              lcd.print(month_ip);
              lcd.print("/");

              lcd.print(2000 + year_ip);

              //display calendar
              lcd.setCursor(15, 1);
              lcd.write(2);
              break;
            case MONTH:
              lcd.setCursor(2, 1);
              lcd.print(set3);
              lcd.print(set2);
              lcd.write(1);  //lcd.print(":");
              lcd.print(set1);
              lcd.print(set0);
              lcd.write(1);  //lcd.print(":");
              if (rtc.Seconds10 == 0) {
                lcd.print("0");
              }
              lcd.print((rtc.Seconds10 * 10) + rtc.Seconds);
              lcd.setCursor(12, 1);
              lcd.print(Am_Pm[am_pm_ip]);


              lcd.setCursor(1, 0);
              lcd.print(DAYOFWEEK[day_ip]);
              lcd.print(date_ip_10);
              lcd.print(date_ip_0);
              lcd.print("/");
              if (month_ip < 10) {
                lcd.print("0");
              }
              lcd.print(month_ip);
              lcd.print("/");

              lcd.print(2000 + year_ip);

              //display calendar
              lcd.setCursor(15, 1);
              lcd.write(2);
              break;
            case YEAR:
              lcd.setCursor(2, 1);
              lcd.print(set3);
              lcd.print(set2);
              lcd.write(1);  //lcd.print(":");
              lcd.print(set1);
              lcd.print(set0);
              lcd.write(1);  //lcd.print(":");
              if (rtc.Seconds10 == 0) {
                lcd.print("0");
              }
              lcd.print((rtc.Seconds10 * 10) + rtc.Seconds);
              lcd.setCursor(12, 1);
              lcd.print(Am_Pm[am_pm_ip]);

              lcd.setCursor(1, 0);
              lcd.print(DAYOFWEEK[day_ip]);
              lcd.print(date_ip_10);
              lcd.print(date_ip_0);
              lcd.print("/");
              if (month_ip < 10) {
                lcd.print("0");
              }
              lcd.print(month_ip);
              lcd.print("/");
              lcd.print(2000 + year_ip);

              //display calendar
              lcd.setCursor(15, 1);
              lcd.write(2);
              break;
          }
          DoM = (date_ip_10 * 10) + date_ip_0;

          DS1302_write(DS1302_ENABLE, 0);

          // Disable Trickle Charger.
          DS1302_write(DS1302_TRICKLE, 0x00);

          // int seconds, minutes, hours, dayofweek, dayofmonth, month, year;

          // seconds = 0;
          // minutes = m;  // Take minute from user
          // hours = h;    // Take hour from user
          dayofweek = day_ip;
          dayofmonth = DoM;
          month = month_ip;
          year = 2000 + year_ip;

          // Fill the structure with zeros to make
          // any unused bits zero
          memset((char *)&rtc, 0, sizeof(rtc));

          rtc.Seconds = bin2bcd_l(seconds);
          rtc.Seconds10 = bin2bcd_h(seconds);
          rtc.CH = 0;  // Start the clock by clearing the clcok halt bit
          rtc.Minutes = bin2bcd_l(minutes);
          rtc.Minutes10 = bin2bcd_h(minutes);
          //12hour format
          rtc.h12.Hour = bin2bcd_l(hours);
          rtc.h12.Hour10 = bin2bcd_h(hours);
          rtc.h12.AM_PM = am_pm_ip;  // AM = 0
          rtc.h12.hour_12_24 = 1;    // 1 for 24 hour format
                                     /*24h format
          // rtc.h24.Hour = bin2bcd_l(hours);
          // rtc.h24.Hour10 = bin2bcd_h(hours);
          // rtc.h24.hour_12_24 = 0;  // 0 for 24 hour format
    */
          rtc.Date = bin2bcd_l(dayofmonth);
          rtc.Date10 = bin2bcd_h(dayofmonth);
          rtc.Month = bin2bcd_l(month);
          rtc.Month10 = bin2bcd_h(month);
          rtc.Day = dayofweek;
          rtc.Year = bin2bcd_l(year - 2000);
          rtc.Year10 = bin2bcd_h(year - 2000);
          rtc.WP = 0;

          //Write clock data using burst writeDA
          DS1302_clock_burst_write((uint8_t *)&rtc);
          break;
        }
      case READ_STATE:
        {
          lcd.setCursor(10, 1);
          lcd.print("  ");

          DS1302_clock_burst_read((uint8_t *)&rtc);

          int dy = rtc.Day;  //day of the week
          int dt = (rtc.Date10 * 10) + rtc.Date;
          int mon = (rtc.Month10 * 10) + rtc.Month;
          int ye = 2000 + (rtc.Year10 * 10) + rtc.Year;

          lcd.setCursor(1, 0);
          lcd.print(DAYOFWEEK[dy]);
          if (dt < 10) { lcd.print("0"); }
          lcd.print(dt);
          lcd.print("/");
          if (mon < 10) { lcd.print("0"); }
          lcd.print(mon);
          lcd.print("/");
          lcd.print(ye);

          int hr = (rtc.h12.Hour10 * 10) + rtc.h12.Hour;
          int mi = (rtc.Minutes10 * 10) + rtc.Minutes;
          int sc = (rtc.Seconds10 * 10) + rtc.Seconds;
          int a_p = rtc.h12.AM_PM;
          lcd.setCursor(2, 1);
          if (hr < 10) { lcd.print("0"); }
          lcd.print(hr);
          lcd.write(1);  //lcd.print(":");
          if (mi < 10) { lcd.print("0"); }
          lcd.print(mi);
          lcd.write(1);  //lcd.print(":");
          if (sc < 10) { lcd.print("0"); }
          lcd.print(sc);

          lcd.setCursor(12, 1);
          if (a_p == 0) {
            lcd.print(Am_Pm[a_p]);
          }
          if (a_p == 1) {
            lcd.print(Am_Pm[a_p]);
          }

          lcd.setCursor(0, 1);
          lcd.write(5);  //diamond full

          // lcd.setCursor(15, 1);
          // lcd.write(14);  //hourglass_straight
          // _delay_ms(500);

          lcd.setCursor(15, 1);
          lcd.write(11);  //hourglass_turn
          _delay_ms(500);
          Serial.print(hr);
          Serial.print(":");
          Serial.print(mi);
          Serial.print(":");
          Serial.println(sc);
          
          break;
        }
    }
  }
}

//Function that delay for n microseconds when called
//Works on the principle that since Timer 1 at a prescaler of 8
//Ticks for 0.5 microsecond each, generating a microsecond delay is
//as simple as counting the amount of tick that has passed.
// I.e to delay for 1 microsecond, wait for 2 counts(0.5 microsecond * 2 = 1 microsecond)
// Idea from user "cattledog" from this arduino forum thread https://forum.arduino.cc/t/help-swapping-micros-to-timer1-on-nano-mega328p/699003

//clock functions
// void DS1302_clock_burst_read(uint8_t *p) {  // Function that reads 8 bytes clock data in burst mode from the DS1302.
//   int i;
//   _DS1302_start();  // Setup start condition
//   _DS1302_togglewrite(DS1302_CLOCK_BURST_READ, true);
//   for (i = 0; i < 8; i++) {
//     *p++ = _DS1302_toggleread();
//   }
//   _DS1302_stop();
// }
// void DS1302_clock_burst_write(uint8_t *p) {  // Function that reads 8 bytes clock data in burst mode from the DS1302.
//   int i;
//   _DS1302_start();
//   _DS1302_togglewrite(DS1302_CLOCK_BURST_WRITE, false);
//   for (i = 0; i < 8; i++) {
//     _DS1302_togglewrite(*p++, false);
//   }
//   _DS1302_stop();
//   Serial.print("read completd");

// }
// uint8_t DS1302_read(int address) {  //Main Function for reading a single byte from the inputted address
//   uint8_t data;
//   // set lowest bit (read bit) in address
//   address |= (1 << DS1302_READBIT);
//   _DS1302_start();
//   _DS1302_togglewrite(address, true);  // Write the command of the address which we want to read from
//   data = _DS1302_toggleread();         // Read the data
//   _DS1302_stop();

//   return (data);
// }
// void DS1302_write(int address, uint8_t data) {  // Main Function for writing a single byte into the inputted address
//   // clear lowest bit (read bit) in address
//   address &= ~(1 << DS1302_READBIT);
//   _DS1302_start();
//   _DS1302_togglewrite(address, false);  // First write the command
//   _DS1302_togglewrite(data, false);     // Then write the data
//   _DS1302_stop();
// }
// void _DS1302_start(void) {  // Clock initializing function
//   //defining directions for the pins that is connected to the clock
//   PORTC &= ~(1 << CE);  //0
//   DDRC |= (1 << CE);  //output

//   PORTC &= ~(1 << SCLK);  //0
//   DDRC |= (1 << SCLK);  //output
  
//   DDRC |= (1 << DAT); // output
  
//   PINC |= (1 << CE);  // Start CE to high to begin running opreations
//   delayMicroseconds(4);  // delayMicro(4);
// }
// void _DS1302_stop(void) {  // Stop function that is called when we're done communicating with the clock
//   PORTC &= ~(1 << CE);     // Set CE low to stop transmitting
//   delayMicroseconds(4);  // delayMicro(4);
// }
// uint8_t _DS1302_toggleread(void) {  // Byte reading function
//   int i, data;
//   data = 0;
//   for (i = 0; i <= 7; i++) {
//     PORTC |= (1 << SCLK);
//     delayMicroseconds(1); //delayMicro(1);
//     PORTC &= ~(1 << SCLK);
//     delayMicroseconds(1);  //  delayMicro(1);
//     writeBit(&data, i, readDAT());
//   }
//   return (data);
// }
// void writeBit(int *x, int n, int value) {  //Bit manipulation function writes value to position n of x
//   if (value) {
//     *x |= (1 << n);
//   } else {
//     *x &= ~(1 << n);
//   }
// }
// int readBit(int *x, int n) {  //Function for reading a specific bit x in position n
//   return (*x & (1 << n)) ? 1 : 0;
// }
// int readDAT() {  // Port reading function for reading DAT
//   if (!(PINC & (1 << DAT))) {
//     return 0;
//   } else {
//     return 1;
//   }
// }
// void _DS1302_togglewrite(int data, int release) {  //Byte Writing function. Release is used for when we are still writing afterwards and still don't want to release the I/O line just yet
//   int i;
//   for (i = 0; i <= 7; i++) {
//     if (readBit(&data, i)) {
//       PORTC |= (1 << DAT);
//     } else {
//       PORTC &= ~(1 << DAT);
//     }
//    delayMicroseconds(1);//delayMicro(1);  // 1 microseconds      // tDC = 200ns

//     PORTC |= (1 << SCLK);
//     //digitalWrite( DS1302_SCLK_PIN, HIGH);
//     delayMicroseconds(1);//delayMicro(1);  // 1 microseconds       // tCH = 1000ns, tCDH = 800ns

//     if (release && i == 7) {  //Release I/O line if write is followed by read
//       DDRC &= ~(1 << DAT);
//       //pinMode( DS1302_IO_PIN, INPUT);
//     } else {
//       PORTC &= ~(1 << SCLK);
//       delayMicroseconds(1);//delayMicro(1);  // 1 microseconds
//     }
//   }
// }
//-----------------------------------------------------------------------------------------------
// /*
void DS1302_clock_burst_read(uint8_t *p) {
  int i;

  _DS1302_start();

  // Instead of the address,
  // the CLOCK_BURST_READ command is issued
  // the I/O-line is released for the data
  _DS1302_togglewrite(DS1302_CLOCK_BURST_READ, true);

  for (i = 0; i < 8; i++) {
    *p++ = _DS1302_toggleread();
  }
  _DS1302_stop();
    // Serial.println("read completed");

}

void DS1302_clock_burst_write(uint8_t *p) {
  int i;

  _DS1302_start();

  // Instead of the address,
  // the CLOCK_BURST_WRITE command is issued.
  // the I/O-line is not released
  _DS1302_togglewrite(DS1302_CLOCK_BURST_WRITE, false);

  for (i = 0; i < 8; i++) {
    // the I/O-line is not released
    _DS1302_togglewrite(*p++, false);
  }
  _DS1302_stop();
  // Serial.println("write completed");
}

uint8_t DS1302_read(int address) {
  uint8_t data;

  // set lowest bit (read bit) in address
  bitSet(address, DS1302_READBIT);

  _DS1302_start();
  // the I/O-line is released for the data
  _DS1302_togglewrite(address, true);
  data = _DS1302_toggleread();
  _DS1302_stop();

  return (data);
}

void DS1302_write(int address, uint8_t data) {
  // clear lowest bit (read bit) in address
  bitClear(address, DS1302_READBIT);

  _DS1302_start();
  // don't release the I/O-line
  _DS1302_togglewrite(address, false);
  // don't release the I/O-line
  _DS1302_togglewrite(data, false);
  _DS1302_stop();
}

void _DS1302_start(void) {
  digitalWrite(DS1302_CE_PIN, LOW);  // default, not enabled
  pinMode(DS1302_CE_PIN, OUTPUT);

  digitalWrite(DS1302_SCLK_PIN, LOW);  // default, clock low
  pinMode(DS1302_SCLK_PIN, OUTPUT);

  pinMode(DS1302_IO_PIN, OUTPUT);

  digitalWrite(DS1302_CE_PIN, HIGH);  // start the session
  delayMicroseconds(4);               // tCC = 4us
}

void _DS1302_stop(void) {
  // Set CE low
  digitalWrite(DS1302_CE_PIN, LOW);

  delayMicroseconds(4);  // tCWH = 4us
}

uint8_t _DS1302_toggleread(void) {
  uint8_t i, data;

  data = 0;
  for (i = 0; i <= 7; i++) {
    // Issue a clock pulse for the next databit.
    // If the 'togglewrite' function was used before
    // this function, the SCLK is already high.
    digitalWrite(DS1302_SCLK_PIN, HIGH);
    delayMicroseconds(1);

    // Clock down, data is ready after some time.
    digitalWrite(DS1302_SCLK_PIN, LOW);
    delayMicroseconds(1);  // tCL=1000ns, tCDD=800ns

    // read bit, and set it in place in 'data' variable
    bitWrite(data, i, digitalRead(DS1302_IO_PIN));
  }
  return (data);
}

void _DS1302_togglewrite(uint8_t data, uint8_t release) {
  int i;

  for (i = 0; i <= 7; i++) {
    // set a bit of the data on the I/O-line
    digitalWrite(DS1302_IO_PIN, bitRead(data, i));
    delayMicroseconds(1);  // tDC = 200ns

    // clock up, data is read by DS1302
    digitalWrite(DS1302_SCLK_PIN, HIGH);
    delayMicroseconds(1);  // tCH = 1000ns, tCDH = 800ns

    if (release && i == 7) {
      // If this write is followed by a read,
      // the I/O-line should be released after
      // the last bit, before the clock line is made low.
      // This is according the datasheet.
      // I have seen other programs that don't release
      // the I/O-line at this moment,
      // and that could cause a shortcut spike
      // on the I/O-line.
      pinMode(DS1302_IO_PIN, INPUT);

      // For Arduino 1.0.3, removing the pull-up is no longer needed.
      // Setting the pin as 'INPUT' will already remove the pull-up.
      // digitalWrite (DS1302_IO, LOW); // remove any pull-up
    } else {
      digitalWrite(DS1302_SCLK_PIN, LOW);
      delayMicroseconds(1);  // tCL=1000ns, tCDD=800ns
    }
  }
}
// */
