#include <GSMSIM300.h>
#include <SoftwareSerial.h> // Please increase RX buffer to 256

const char *pinCode = "1234"; // Set this to your pin code - set to NULL if no pin is used
const char *number = "0123456789"; // Set this to the desired phone number

SoftwareSerial gsmSerial(2, 3); // RX, TX

// The power pin is connected to the status pin on the module
// Letting the microcontroller turn the module on and off
GSMSIM300 GSM(&gsmSerial, pinCode, 4); // Pointer to serial instance, pin code, power pin

// You can also use a Hardware UART if you like:
//GSMSIM300 GSM(&Serial1, pinCode, 4); // Pointer to serial instance, pin code, power pin

void setup() {
  Serial.begin(115200);
  gsmSerial.begin(9600); // Start the communication with the GSM module
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
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
        GSM.readSMS(); // Read the last received SMS
      else if (c == 'L')
        GSM.listSMS("ALL"); // List all messages
      else if (c == 'D')
        GSM.deleteSMSAll(); // Deletes all messages on the SIM card - this is useful as the SIM card has very limited storage capability
    }
    if (GSM.newSMS()) { // Check if a new SMS is received
      if (GSM.readSMS()) { // Returns true if the number and message of the sender is successfully extracted from the SMS
        GSM.sendSMS(GSM.numberIn, "Automatic response from SIM300 GSM module"); // Sends a response to that number
        Serial.println(GSM.messageIn);
      }
      GSM.deleteSMS(); // Delete the SMS again, as the SIM card has very limited storage capability
    }
  }
}
