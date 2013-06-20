/* Copyright (C) 2013 Kristian Lauszus, TKJ Electronics. All rights reserved.

 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").

 Contact information
 -------------------

 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#ifndef _gsmsim300_h_
#define _gsmsim300_h_

#if defined(ARDUINO) && ARDUINO >=100
#include "Arduino.h"
#else
#include <WProgram.h>
#endif

/**
 * Communication with the GSM module is done via software implementation of the UART protocol.
 * You might have to increase the RX buffer in order to be able to the listSMS function.
 */
#include <SoftwareSerial.h>

#define DEBUG // Print serial debugging
//#define EXTRADEBUG // Print every character received from the GSM module

/** States used for the GSM state machine */
#define GSM_POWER_ON              0
#define GSM_POWER_ON_WAIT         1
#define GSM_SET_PIN               2
#define GSM_CHECK_CONNECTION      3
#define GSM_CHECK_CONNECTION_WAIT 5
#define GSM_CONNECTION_RESPONSE   6
#define GSM_RUNNING               7
#define GSM_POWER_OFF             8
#define GSM_POWER_OFF_WAIT        9

/** States used for the SMS state machine */
#define SMS_IDLE                  0
#define SMS_MODE                  1
#define SMS_ALPHABET              2
#define SMS_NUMBER                3
#define SMS_CONTENT               4
#define SMS_WAIT                  5

/** States used for the call state machine */
#define CALL_IDLE                 0
#define CALL_NUMBER               1
#define CALL_SETUP                2
#define CALL_SETUP_WAIT           3
#define CALL_RESPONSE             4
#define CALL_ACTIVE               5

/** The GSMSIM300 class is able to call and answer calls, send messages and receive messages and some other useful features. */
class GSMSIM300 {
public:
	/**
	 * Constructor for the library.
	 * @param pinCode  Pin code for the SMS card.
	 * @param rx       Receive pin to use with SoftwareSerial library. If argument is omitted then it will be set to 2.
	 * @param tx       Transmit pin to use with SoftwareSerial library. If argument is omitted then it will be set to 3.
	 * @param powerPin Power pin used to turn the GSM module on and off. This should be connected to the status pin on the module.
	 *                 If argument is omitted then it will be set to 4.
	 * @param running  Set this to true to if the GSM module is already powered on and configured. Useful when developing.
	 *                 If argument is omitted then it will be set to false.
	 */
	GSMSIM300(const char *pinCode, uint8_t rx = 2, uint8_t tx = 3, uint8_t powerPin = 4, bool running = false);

	/** Destructor for the library. This will take care of freeing the SoftwareSerial instance. */
	~GSMSIM300();

	/**
	 * Call this to start the serial communication with the GSM module.
	 * @param baud Desired baud rate to use.
	 */
	void begin(uint32_t baud = 9600);

	/** Used to update the state machine in the library. */
	void update();

	/**
	 * Use this to call a number.
	 * @param num Number to call.
	 */
	void call(const char *num);

	/** Use this to hangup a conversation. */
	void hangup();

	/**
	 * Used to check if a new SMS has been received.
	 * @return Returns true if a new message has been received.
	 */
	bool newSMS() {
		return newSms;
	};

	/**
	 * Used to send a SMS.
	 * @param num Number to send message to.
	 * @param mes Message to send. Maximum is 160 characters.
	 */
	void sendSMS(const char *num, const char *mes);

	/**
	 * Read SMS at a specific index. If no index is set the last received SMS will be read.
	 * @param  index SMS index to read. If argument is omitted then the last received SMS will be read.
	 * @return       Returns true if both number and message is successfully extracted.
	 */
	bool readSMS(char *index = NULL);

	/**
	 * Used to delete a SMS at a specific index. If no index is set the last received SMS will be read.
	 * @param index SMS index to delete. If argument is omitted then the last received SMS will be read.
	 */
	
	void deleteSMS(char *index = NULL);

	/**
	 * Deletes all messages of the corresponding type on the SIM card. You have to call this at some point or the SIM card will get full.
	 * @param type Available ones are: "DEL READ", "DEL UNREAD", "DEL SENT", "DEL UNSENT", "DEL INBOX”, and "DEL ALL". Default to "DEL ALL".
	 */
	void deleteSMSAll(const char *type = "DEL ALL");

	/**
	 * List all messages stored on the SIM card.
	 * @param type  Available ones are: "REC UNREAD", "REC READ", "STO UNSENT", "STO SENT", and "ALL". Default to "ALL".
	 */
	void listSMS(const char *type = "ALL");

	/**
	 * Used to get the state of the GSM module.
	 * @return Returns the state of the GSM state machine.
	 */
	uint8_t getState() {
		return gsmState;
	}

	/**
	 * Used to set the state of the GSM module.
	 * @param newState The new state for the GSM state machine.
	 */
	void setState(uint8_t newState) {
		gsmState = newState;
	}

	/** Buffer for the last ingoing and outgoing number. */
	char numberIn[20], numberOut[20];

	/** Buffer for the last ingoing and outgoing message. */
	char messageIn[161], messageOut[161];
private:
	/** Pointer to the SoftwareSerial instance allocated dynamically. */
	SoftwareSerial *gsm;

	/** Used by the library to check for ingoing messages. */
	void checkSMS();

	/** Used to update the SMS state machine. */
	void updateSMS();

	/** Used to update the call state machine. */
	void updateCall();

	/** Used to set the SMS mode to normal text. */
	void setSMSTextMode();

	/**
	 * Used to set the next string the GSM state machine should wait for.
	 * @param str String to wait for.
	 */
	void setGsmWaitingString(const char *str);

	/**
	 * Used to set the next string the SMS or call state machine should wait for.
	 * @param str String to wait for.
	 */
	void setOutWaitingString(const char *str);

	/** Used to check if the desired string has been received. */
	bool checkGsmWaitingString();
	bool checkOutWaitingString();

	/**
	 * Used to check if a specific sentence has been received.
	 * @param  input     The input from the GSM module.
	 * @param  cmpString Sentence to compare too.
	 * @param  pString   Pointer to the sentence.
	 * @return           Returns true if the sentence has been received.
	 */
	bool checkString(char input, const char *cmpString, char **pString);

	/**
	 * Used to extract content two characters. Not including those.
	 * @param  buffer    Buffer to read into.
	 * @param  size      Size of buffer.
	 * @param  beginChar First character to look for.
	 * @param  endChar   Last character to look for.
	 * @param  offset    Offset after first character to the actual string.
	 * @return           Returns true if the content is successfully extracted.
	 */
	bool extractContent(char *buffer, uint8_t size, char beginChar, char endChar, uint8_t offset);
	
	/** Used by the library to automatically pick up incoming calls. */
	void answer();

	/** Used to power the on or off respectively using the power pin set in the constructor. */
	void powerOn();
	void powerOff();

	/** Pin code for the SIM card. */
	const char *pinCode;

	/** Power pin connected to the module's status pin. */
	const uint8_t powerPin;

	/** Sentences to look for in the incoming characters sent from the GSM module. */
	static const char *receiveSmsString, *incomingCallString, *hangupCallString, *powerDownString, *errorString;

	/** Pointers to the sentence to look for. */
	char *pReceiveSmsString, *pIncomingCallString, *pHangupCallString, *pPowerDownString, *pErrorString;

	/** Last incoming character received from the GSM module. */
	char incomingChar;

	/** Buffers used set sentences to look for. */
	char gsmString[20], outString[20];

	/** Pointers to those buffers. */
	char *pGsmString, *pOutString;

	/** Timer used to reset the GSM module if it does not respond. */
	uint32_t gsmTimer;

	/** State variables for the states machines. */
	uint8_t gsmState, smsState, callState;	

	/** Buffer for last index received. */
	char lastIndex[5];

	/** Counter used to extract index from a received SMS. */
	uint8_t indexCounter;
	
	/** Bool used to check when it should start reading the index. */
	bool readIndex;

	/** True if a new SMS has been received, but not yet read. */
	bool newSms;	
};

#endif