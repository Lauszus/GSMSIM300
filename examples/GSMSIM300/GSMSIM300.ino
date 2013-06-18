#include <GSMSIM300.h>
#include <SoftwareSerial.h> // Please increase rx buffer to 256

const char* pinCode = "1234"; // Set this to your pin code
const char* number = "0123456789"; // Set this to the desired phone number

// The constructor below will start a SoftwareSerial connection on the chosen rx and tx pins
// The power pin is connected to the status pin on the module
// Letting the microcontroller turn the module on and off
GSMSIM300 GSM(pinCode,2,3,4); // Pin code, rx, tx, power pin

void setup() {
  Serial.begin(115200);
  GSM.begin(28800); // I have found this baud rate to work pretty well with my module
  
  Serial.println(F("GSMSIM300 library is running!"));
}

void loop() {
  GSM.update(); // This will update the state machine in the library
  if (GSM.getState() == GSM_RUNNING) { // Make sure the GSM module is up and running
    if (Serial.available()) {
      char c = Serial.read();
      if (c == 'C')
        GSM.call(number); // Call number
      else if (c == 'H')
        GSM.hangup(); // Hangup conversation
      else if (c == 'S')
        GSM.sendSMS(number, "You just received a SMS from a SIM300 GSM module :)"); // Send SMS
      else if (c == 'R')
        GSM.readSMS(); // Read the last returned SMS
      else if (c == 'L')
        GSM.listSMS("ALL"); // List all messages
    }
    if(GSM.newSMS()) { // Check if a new SMS is received
      if(GSM.readSMS()) // Returns true if the number and message of the sender is successfully extracted from the SMS
        GSM.sendSMS(GSM.numberIn, "Automatic response from SIM300 GSM module"); // Sends a response to that number
    } 
  }
}
