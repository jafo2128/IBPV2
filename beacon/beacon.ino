//
//   BEACON main
//
//   NCDXF/IARU International Beacon Program - V2
//
#include <limits.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <inttypes.h>
#include <LCDi2cNHD.h>
#include <Bounce2.h>

#include "beacon.h"
#include "config.h"
#include "debug.h"
#include "stations.h"
#include "softreset.h"


//  Global vars

// slotindex - 0-17 index into stations array for callsign, club name, and time slot in seconds
//   255 indicates don't transmit
byte slotindex = 255;
uint32_t uptime = 0;   // count of times we've been thru the beacon 3 minute loop;

// RxD, TxD UART is done in the software - to the GPS Radio. 
SoftwareSerial gps_serial(GPSRxD, GPSTxD);
//  the GPS NMEA sentence processing class
TinyGPS gps;
// front panel LCD display class
// rows, columns, I2C address, display (not use, set to zero)
LCDi2cNHD fp_lcd = LCDi2cNHD(2,16,0x50>>1,0);
// front panel MENU button - Bounce (2) object
Bounce menuBtn = Bounce();

#define GPSECHO false
// turn on only the 'seconds' sentence (GPRMC)
#define PMTK_SET_NMEA_OUTPUT_RMCONLY F("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29")
#define PMTK_SET_NMEA_UPDATE_1HZ  F("$PMTK220,1000*1F")
#define PMTK_API_SET_FIX_CTL_1HZ  F("$PMTK300,1000,0,0,0,0*1C")

volatile boolean ticked = false;

// wall_ticks counts seconds and goes from 0-180 because there is a 3-minute beacon schedule.
volatile byte wall_ticks = 0;

// Set this to the click second at which you want the tick() interrupt handler to key the transmitter.
volatile byte next_tx_click = 255;

// Set this to true if the tick() interrupt handler to measure update millis_per_second to the most recent value.
// It should never be on if there is a chance that other interrupts will interfere or if interrupts are disabled, such as during SoftwareSerial output (input is OK).
volatile boolean disciplining_milliclock = false;

// The disciplining value for the milliclock, which really means measuring it and recording millis_per_second.
volatile int millis_per_second = 0;

// Outside the interrupt, when disciplining the milliclock, update this with the value needed to achieve 750ms.  It starts at 750 and disciplining the milliclock sets this number.
int seven_fifty_millis = 750;

volatile int last_millis = 0;

// set on id send, reset on band change.  prevents starting up in the middle of the schedule.
volatile boolean id_sent = false;


// tick interrupt
// keep track of wall_ticks; use GPS PPS to discipline millis_per_second when it's safe to do so (no interrupts masked).
void tick() {
  wall_ticks = (wall_ticks+1) % (3*60);
//  if ((wall_ticks - stations[slotindex].start_time) == next_tx_click) {
//    txon();
//  }

  if (disciplining_milliclock) {
    int t = last_millis;
    last_millis = millis();
    millis_per_second = millis() - t;
  }

  ticked = true;
}

// run once, at boot
void setup()  {

	// find our slot to beacon into from the EEPROM on the Leonardo baord.
	slotindex = eeprom_slotid();
	// Kill radio TX as soon as we wake up
	pinMode(LED, OUTPUT);	// TX on LED
	setALCPwr(LOW);
	pinMode(PTTLINE, OUTPUT);
	txoff();

	// FP LCD set up
	pinMode(BLBLUE , OUTPUT);
	pinMode(BLGREEN, OUTPUT);
	pinMode(BLRED,   OUTPUT);   
	
	FPBLBLUE
	fp_lcd.init();
    fp_lcd.cursor_off();
    FPPRINTRC(0,0,F("          "));
	FPPRINTRC(0,0,F(VERSION));
// if slotindex isn't valid - say so on the FP LCD
	if (slotindex == 255) { // slot ID not valid in EEPROM
		FPBLRED
    	FPPRINTRC(0,7,F("ERROR"));
		FPPRINTRC(0,12,F("-1"));
	}
	else {
    	FPPRINTRC(0,6,stations[slotindex].call);
		FPPRINTRC(0,13,stations[slotindex].start_time);
	}

  // Serial debug output to desktop computer.  
#if DEBUG
    FPPRINTRC(1,0,F("QRX Serial CNSOL"));
	setup_debug_print();
// dump EEPORM to serial console
	dump_eeprom();
#endif
  debug_println(F(VERSION));
	if (slotindex == 255) {
		debug_println(F("SLOTID Not set"));
	}
	else {
		debug_println(slotindex);
		debug_println(stations[slotindex].call);
	}

  FPPRINTRC(1,0,F("QRX INIT      "))
  
  // PPS interrupt from GPS on pin 3 (Int.0) on Arduino Leonardo
  pinMode(3, INPUT_PULLUP);         // PPS is 2.8V so give it pullup help
  attachInterrupt(digitalPinToInterrupt(PPS), tick, RISING); // tick happens 0.5s after GPS serial sends the time.


  // GPS Setup 
    gps_serial.begin(9600);
    gps_serial.println(PMTK_SET_NMEA_OUTPUT_RMCONLY);
    gps_serial.println(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz NMEA sentence rate
    gps_serial.println(PMTK_API_SET_FIX_CTL_1HZ);   // 1 Hz fix rate

    delay(1000);
    // Stop interrupts from GPS serial input.
    // All GPS readers have/ to call listen() 
    // and end with stopListening, or else
    // interrupts disturb time-critical code sections.
    gps_serial.stopListening();

  FPPRINTRC(1,0,F("QRX INIT GPS DO "))
  do {
    debug_println(F("*** GPS Discipline clock"));
  } while (! gps_discipline_clock(LONG_MAX));
  debug_println(F("*** GPS Locked "));

	debug_println(F("*** GPS Discipline Milliclock"));
    gps_begin_milliclock_discipline(); 
    delay(5000);
    gps_end_milliclock_discipline();
	debug_println(F("*** Milliclock disciplined "));
  
  FPPRINTRC(1,0,F("QRX INIT RADIO  "))
  debug_println(F("Radio init"));

	int radioStatus = radioSetup();
	if (radioStatus < 0) { // radio not talking
  		FPPRINTRC(1,0,F("               "));
  		FPPRINTRC(1,0,F("RADIO error"));
  		FPBLRED
		while(1)
			delay(1000);
	}
  CWSetup();
	pinMode(MENUBTN,INPUT_PULLUP);
	menuBtn.attach(MENUBTN);	// menu button on front panel
	menuBtn.interval(10);   	// debounce time
  
	if (slotindex == 255) {
  		FPPRINTRC(1,0,F("               "));
  		FPPRINTRC(1,0,F("BAD SLOT"));
  		FPBLRED
		while(1)
			delay(1000);
	}
  FPPRINTRC(1,0,F("               "));
  FPPRINTRC(1,0,F("OPER"));
  FPBLGREEN
	debug_println(F("Operational"));

//  prepare so we can reboot from within the loop code
//	wdt_init();
//	debug_println(F("WDT init"));

}

//  main cycle loop.  Starts right after setup() exits.  
//  if we saw an interrupt tick, then call the scheduler
void loop()
{
  boolean tocked = false;

// debounce the front panel menu button
//    TODO - is the time spent inside update variable, and how will that affect TX loop start offset?
	menuBtn.update();

  if (ticked) {
    ticked = false;
    tocked = true;
  }

  if (tocked) {
    handle_tick();
  }

}



