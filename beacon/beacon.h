
/*

	beacon.h - defines shared across all files

*/

#ifndef BEACON_H
#define BEACON_H


// bunch of "beacon" defines
#define VERSION "2.12"
#define GPSECHO false
#define CWLINE 5 		// pin connected ot CW keyer line - for morse code generation
#define PTTLINE 12		// go to TX
#define PTTON LOW		// logic value to enable TX
#define PTTOFF HIGH
#define DLEAYTIMER 60	// number of seconds to wait for USB serial monitor to connect
#define PPS 7			// pin connected to GPS PPS interrupt input
#define LED 13			// LED on PCA
#define ALC_PWR 13		// shutdown signal into ALC Power invertor - shared with USER LED
#define MENUBTN 10		// menu button on front panel

#define MAX_UPTIME  4800   // 10 days of 3 minute beacon loop cycles
//   60 minutes an hour; 60/3 = 20 cycles/hour * 24 hours / day * 10 days = 4800
#define RADIO_RETRY_LIMIT 10  // try radio command limit

// orignal
//#define GPSRxD 11                // GPS software serial RxD pin
//#define GPSTxD 8               // GPS software serial TxD pin - NOTE - non-standard usage, and incompat with Adafruit shield wiring

// time slot max index value
#define MAX_SLOTID 17

// SWAPPed
#define GPSRxD 8                // GPS software serial RxD pin
#define GPSTxD 11  

// radio (CI-V) defines
#define radio_baudrate 4800
#define CIV_CA 0xE0
#define CIV_RA 0x76

// Front Panel LCD - I2C comm access, RGB, 16 x 2, NHD P/N .....
#define BLBLUE	6
#define BLGREEN	4
#define BLRED	9
#define FPBLBLUE  { digitalWrite(BLGREEN, LOW);  digitalWrite(BLRED, LOW);   digitalWrite(BLBLUE, HIGH); }
#define FPBLGREEN { digitalWrite(BLBLUE, LOW);  digitalWrite(BLRED, LOW);  digitalWrite(BLGREEN, HIGH); }
#define FPBLRED   { digitalWrite(BLBLUE, LOW);  digitalWrite(BLGREEN, LOW);    digitalWrite(BLRED, HIGH); }
#define FPPRINTRC(row,col,msg) { fp_lcd.setCursor(row,col); fp_lcd.print(msg); }

// uses the built in I2C hardware and the Wire interface.

//   RIG control macros
#define KEYDOWN digitalWrite(CWLINE,HIGH);
#define KEYUP   digitalWrite(CWLINE,LOW);

enum rMode {beaconMode = 0, PSK31Mode = 1};

// EEPROM layout
// rather than a struct, #defines for offsets to give to EEPROM.read().
#define EEProm_layout_version	0
#define EEProm_used				1
#define EEProm_crc 				2
#define EEProm_debug			3
#define EEProm_slotID			4
#define EEProm_sernum			5
#define EEProm_HWver			6
#define EEProm_last 			7

// function prototypes 
void dump_eeprom();
int set_eeprom_sloitd(byte);

#endif


