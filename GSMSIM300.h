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
#include <SoftwareSerial.h>

#define DEBUG // Print serial debugging
//#define EXTRADEBUG // Print every character received from the GSM module

#define GSM_POWER_ON              0
#define GSM_POWER_ON_WAIT         1
#define GSM_SET_PIN               2
#define GSM_CHECK_CONNECTION      3
#define GSM_CHECK_CONNECTION_WAIT 5
#define GSM_CONNECTION_RESPONSE   6
#define GSM_RUNNING               7
#define GSM_POWER_OFF             8
#define GSM_POWER_OFF_WAIT        9

#define SMS_IDLE                  0
#define SMS_MODE                  1
#define SMS_ALPHABET              2
#define SMS_NUMBER                3
#define SMS_CONTENT               4
#define SMS_WAIT                  5

#define CALL_IDLE                 0
#define CALL_NUMBER               1
#define CALL_SETUP                2
#define CALL_SETUP_WAIT           3
#define CALL_RESPONSE             4
#define CALL_ACTIVE               5

// TODO: Comment each function

class GSMSIM300 {
public:
	GSMSIM300(const char *pinCode, uint8_t rx = 2, uint8_t tx = 3, uint8_t powerPin = 4, bool running = false);
	~GSMSIM300();

	void begin(uint32_t baud = 9600);
	void update();
	bool newSMS() {
		return newSms;
	};
	void call(const char *num);
	void hangup();
	void sendSMS(const char *num, const char *mes);
	bool readSMS(char *index = NULL);
	void deleteSMS(char *index = NULL);
	void deleteSMSAll();
	void listSMS(const char *type = "ALL", bool print = true); // Available ones are: "REC UNREAD", "REC READ", "STO UNSENT", "STO SENT", and "ALL"
	uint8_t getState() {
		return gsmState;
	}
	void setState(uint8_t newState) {
		gsmState = newState;
	}
	char numberIn[20], numberOut[20]; // You might have to adjust this - remember to include country code
	char messageIn[161], messageOut[161];
private:
	SoftwareSerial *gsm;

	void checkSMS();
	void updateSMS();
	void updateCall();
	void setGsmWaitingString(const char *str);
	void setOutWaitingString(const char *str);
	void setSMSTextMode();

	bool checkGsmWaitingString();
	bool checkOutWaitingString();
	bool checkString(char input, const char *cmpString, char **pString);
	bool extractContent(char *buffer, uint8_t size, char beginChar, char endChar, uint8_t offset); // Helper function to get content inside string
	
	void answer();
	void powerOn();
	void powerOff();

	const char *pinCode;
	const uint8_t powerPin;

	static const char *receiveSmsString, *incomingCallString, *hangupCallString, *powerDownString;
	char *pReceiveSmsString, *pIncomingCallString, *pHangupCallString, *pPowerDownString;
	
	char incomingChar;
	char gsmString[20], outString[20];
	char *pGsmString, *pOutString;
	char lastIndex[5];

	uint32_t gsmTimer;
	uint8_t gsmState, smsState, callState;	
	uint8_t indexCounter;
	
	bool readIndex;
	bool newSms;	
};

#endif