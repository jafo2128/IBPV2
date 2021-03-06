// DIT = 1200 / WPM
// DIT: DIT duration in milliseconds
// WPM: speed in WPM
// WPM=22, DIT=54.55ms, DAH=163.64ms

#include "beacon.h"
#include "morse.h"

LEDMorseSender cwBeacon(CWLINE);

void CWSetup() {
  cwBeacon.setup();
  cwBeacon.setWPM(22.0);
}  

void send_id(char *id) {
  debug_print(F("send CW id:"));
  debug_println(id);
  digitalWrite(CWLINE,LOW);
  digitalWrite(PTTLINE,PTTON);
//   digitalWrite(LED, HIGH);
  FPBLRED
  cwBeacon.setMessage(id);
  cwBeacon.sendBlocking();
  id_sent = true;
  digitalWrite(CWLINE,LOW);
//   digitalWrite(LED, LOW);
}

void longDAH() {
  digitalWrite(CWLINE,HIGH);
}
