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

#include "GSMSIM300.h"

const char *GSMSIM300::receiveSmsString = "+CMTI: \"SM\","; // +CMTI: "SM",index\r\n
const char *GSMSIM300::incomingCallString = "RING";
const char *GSMSIM300::hangupCallString = "NO CARRIER";
const char *GSMSIM300::powerDownString = "NORMAL POWER DOWN";
const char *GSMSIM300::errorString = "+CME ERROR:"; // +CME ERROR: <err>

// TODO: Remove all delays

GSMSIM300::GSMSIM300(const char *pinCode, uint8_t rx /*= 2*/, uint8_t tx /*= 3*/, uint8_t powerPin /*= 4*/, bool running /*= false*/) :
pinCode(pinCode),
powerPin(powerPin),
pReceiveSmsString((char*)receiveSmsString),
pIncomingCallString((char*)incomingCallString),
pHangupCallString((char*)hangupCallString),
pPowerDownString((char*)powerDownString),
pErrorString((char*)errorString),
pGsmString(gsmString),
pOutString(outString),
readIndex(false),
newSms(false)
{
    pinMode(powerPin,OUTPUT);
    digitalWrite(powerPin,HIGH);

    gsm = new SoftwareSerial(rx, tx);
    if (running)
        gsmState = GSM_RUNNING;
    else
        gsmState = GSM_POWER_ON;

    smsState = SMS_IDLE;
    callState = CALL_IDLE;
}

GSMSIM300::~GSMSIM300() {
    delete gsm;
}

void GSMSIM300::begin(uint32_t baud /*= 9600*/) {
    gsm->begin(baud); // You can try to set this higher, but 9600 should work
}

void GSMSIM300::update() {
    incomingChar = gsm->read();
#ifdef EXTRADEBUG
    if(incomingChar != -1)
        Serial.write(incomingChar);
#endif
    if (checkString(incomingChar,powerDownString,&pPowerDownString)) {
#ifdef DEBUG
        Serial.println(F("GSM module turned off"));
#endif
        gsmState = GSM_POWER_ON;
    }
    if (checkString(incomingChar,errorString,&pErrorString)) {
#ifdef DEBUG
        char error[5], i = 0;
        while (i < sizeof(error)-1) { // TODO: Check time
            error[i] = gsm->read();
            if (error[i] == '\r' || error[i] == '\n' || error[i] == '\0')
                break;
            else if (error[i] != -1 && error[i] != ' ')
                i++;
        }
        error[i] = '\0';
        Serial.print(F("The GSM module returned the following error: "));
        Serial.println(error);
#endif
        gsmState = GSM_POWER_ON;
    }

    switch(gsmState) {
        case GSM_POWER_ON:
            delay(1000);
            gsm->print(F("AT+CPOWD=0\r")); // Turn off the module if it's already on
#ifdef DEBUG
            Serial.println(F("GSM PowerOn"));
#endif
            delay(1000);
            powerOn();
#ifdef DEBUG
            Serial.println(F("GSM check state"));
#endif
            gsm->print(F("AT\r"));
            delay(500);
            gsm->print(F("AT\r"));
            setGsmWaitingString("OK");
            gsmState = GSM_POWER_ON_WAIT;
            break;

        case GSM_POWER_ON_WAIT:
            if(checkWaitingString(incomingChar,gsmString,&pGsmString)) {
#ifdef DEBUG
                Serial.println(F("GSM Module is powered on\r\nChecking SIM Card"));
#endif
                gsm->print(F("AT+CPIN="));
                gsm->print(pinCode); // Set pin
                gsm->print(F("\r"));
                setGsmWaitingString("Call Ready");
                gsmState = GSM_SET_PIN;
            }
            break;
        
      	case GSM_SET_PIN:
            if(checkWaitingString(incomingChar,gsmString,&pGsmString)) {
#ifdef DEBUG
                Serial.println(F("SIM Card ready"));
                Serial.print(F("Waiting for GSM to get ready"));
#endif
                gsmState = GSM_CHECK_CONNECTION;
            }
            break;
            
        case GSM_CHECK_CONNECTION:
#if defined(DEBUG) && !defined(EXTRADEBUG)
            Serial.print(F("."));
#elif defined(EXTRADEBUG)
            Serial.print(F("\r\nChecking Connection"));
#endif
            gsm->print(F("AT+CREG?\r"));
            setGsmWaitingString("+CREG: 0,");
            gsmState = GSM_CHECK_CONNECTION_WAIT;
            break;

        case GSM_CHECK_CONNECTION_WAIT:
            if(checkWaitingString(incomingChar,gsmString,&pGsmString))
                gsmState = GSM_CONNECTION_RESPONSE;
            break;

        case GSM_CONNECTION_RESPONSE:
            if(incomingChar != -1) {
#ifdef EXTRADEBUG
                Serial.print(F("\r\nConnection response: "));
                Serial.print(incomingChar);
#endif
                if(incomingChar != '1') {
                    delay(1000);
                    gsmState = GSM_CHECK_CONNECTION;
                } else {
#ifdef DEBUG
                    Serial.println(F("\r\nGSM module is up and running!\r\n"));
#endif
                    gsmState = GSM_RUNNING;
                }
            }
            break;

        case GSM_RUNNING:
            updateSMS();
            updateCall();
            checkSMS(); // Check if a new SMS is received
            if (checkString(incomingChar,incomingCallString,&pIncomingCallString)) {
#ifdef DEBUG
                Serial.println(F("Incoming Call"));
#endif
                answer();
            }
            break;

        case GSM_POWER_OFF:
#ifdef DEBUG
            Serial.println(F("Shutting down GSM module"));
#endif
            gsm->print(F("AT+CPOWD=1\r")); // Turn off the module if it's already on
            setGsmWaitingString(powerDownString);
            gsmState = GSM_POWER_OFF_WAIT;
            break;

        case GSM_POWER_OFF_WAIT:
            if(checkWaitingString(incomingChar,gsmString,&pGsmString)) {
                powerOff();
#ifdef DEBUG
                Serial.println(F("GSM PowerOff"));
#endif
                gsmState = GSM_POWER_ON;
            }
            break;

        default:
            break;
    }
}

void GSMSIM300::checkSMS() {
    if (incomingChar == -1)
        return;
    if (readIndex) {
        if (incomingChar == '\r') { // End of index
            lastIndex[indexCounter] = '\0';
            readIndex = false;
            newSms = true;
#ifdef DEBUG
            Serial.print(F("Received SMS at index: "));
            Serial.println(lastIndex);
#endif
        } else if (indexCounter < sizeof(lastIndex)-1)
            lastIndex[indexCounter++] = incomingChar;
        else {
#ifdef DEBUG
            Serial.println(F("Index is too long"));
#endif
            readIndex = false;
        }
    } else if (checkString(incomingChar,receiveSmsString,&pReceiveSmsString)) {
        readIndex = true;
        indexCounter = 0;
    }
}

// I know this might seem confusing, but in order to change the pointer I have to create a pointer to a pointer
// **pString will get the value of the original pointer, while
// *pString will get the address of the original pointer
bool GSMSIM300::checkString(char input, const char *cmpString, char **pString) {
    if (input == -1)
        return false;
    if (input == **pString) {
        (*pString)++;
        if (**pString == '\0') {
            *pString = (char*)cmpString; // Reset pointer to start of string
            return true;
        }
    } else
        *pString = (char*)cmpString; // Reset pointer to start of string
    return false;
}

// TODO: Check if updateSMS or updateCall is caught in a loop
void GSMSIM300::updateSMS() {
    switch(smsState) {
        case SMS_IDLE:
            break;

        case SMS_MODE:
#ifdef DEBUG
            Serial.println(F("SMS setting text mode"));
#endif
            setSMSTextMode();
            setOutWaitingString("OK");
            smsState = SMS_ALPHABET;
            break;

        case SMS_ALPHABET:
            if(checkWaitingString(incomingChar,outString,&pOutString)) {
#ifdef DEBUG
                Serial.println(F("SMS setting alphabet"));
#endif
                gsm->print(F("AT+CSCS=\"GSM\"\r")); // GSM default alphabet
                setOutWaitingString("OK");
                smsState = SMS_NUMBER;
            }
            break;

        case SMS_NUMBER:
            if(checkWaitingString(incomingChar,outString,&pOutString)) {
#ifdef DEBUG
                Serial.print(F("Number: "));
                Serial.println(numberOut);
#endif
                gsm->print(F("AT+CMGS=\""));
                gsm->print(numberOut);
                gsm->print(F("\"\r"));
                setOutWaitingString(">");
                smsState = SMS_CONTENT;
            }
            break;

        case SMS_CONTENT:
            if(checkWaitingString(incomingChar,outString,&pOutString)) {
#ifdef DEBUG
                Serial.print(F("Message: \""));
                Serial.print(messageOut);
                Serial.println("\"");
#endif
                gsm->print(messageOut);
                gsm->write(26); // CTRL-Z
                setOutWaitingString("OK");
                smsState = SMS_WAIT;
            }
            break;

        case SMS_WAIT:
            if(checkWaitingString(incomingChar,outString,&pOutString)) {
#ifdef DEBUG
                Serial.println(F("SMS is sent"));
#endif
                smsState = SMS_IDLE;
            }
            break;

        default:
            break;
    }
}

void GSMSIM300::updateCall() {
    switch(callState) {
        case CALL_IDLE:
            break;

        case CALL_NUMBER:
#ifdef DEBUG
            Serial.print(F("Calling: "));
            Serial.println(numberOut);
#endif
            gsm->print(F("ATD")); // Dial
            gsm->print(numberOut);
            gsm->print(F(";\r"));
            callState = CALL_SETUP;
#ifdef DEBUG
            Serial.print(F("Waiting for connection"));
#endif
            break;

        case CALL_SETUP:
#if defined(DEBUG) && !defined(EXTRADEBUG)
            Serial.print(F("."));
#elif defined(EXTRADEBUG)
            Serial.print(F("\r\nChecking response"));
#endif
            gsm->print(F("AT+CLCC\r"));
            setOutWaitingString("+CLCC: 1,0,");
            callState = CALL_SETUP_WAIT;
            break;

        case CALL_SETUP_WAIT:
            if(checkWaitingString(incomingChar,outString,&pOutString)) {
#ifdef EXTRADEBUG
                Serial.print(F("\r\nGot first response"));
#endif
                callState = CALL_RESPONSE;
            }
            break;

        case CALL_RESPONSE:
            if(incomingChar != -1) {
#ifdef EXTRADEBUG
                Serial.print(F("\r\nConnection response: "));
                Serial.print(incomingChar);
#endif
                if(incomingChar != '0') {
                    delay(1000);
                    callState = CALL_SETUP;
                } else {
#ifdef DEBUG
                    Serial.println(F("\r\nCall active"));
#endif
                    callState = CALL_ACTIVE;
                }
            }
            break;

        case CALL_ACTIVE:
            if (checkString(incomingChar,hangupCallString,&pHangupCallString)) {
#ifdef DEBUG
                Serial.println(F("Call hangup!"));
#endif
                callState = CALL_IDLE;
            }
            break;

        default:
            break;
    }
}

void GSMSIM300::setGsmWaitingString(const char *str) {
    strcpy(gsmString,str);
    pGsmString = gsmString;
    gsmTimer = millis();
}

void GSMSIM300::setOutWaitingString(const char *str) {
    strcpy(outString,str);
    pOutString = outString;
    gsmTimer = millis();
}

bool GSMSIM300::checkWaitingString(char input, const char *str, char **pStr) {
    if (checkString(input,str,&(*pStr))) {
#ifdef EXTRADEBUG
        Serial.print(F("\r\nResponse success: "));
        Serial.write(str);
        Serial.println();
#endif
        return true;
    }
    if(millis() - gsmTimer > 10000) { // Only wait 10s for response
#ifdef DEBUG
        Serial.println("\r\nNo response from GSM module\r\nResetting...");
#endif
        gsmState = GSM_POWER_ON;
        smsState = SMS_IDLE;
        callState = CALL_IDLE;
    }
    return false;
}

void GSMSIM300::call(const char *num) {
    strcpy(numberOut,num);
    callState = CALL_NUMBER;
}

void GSMSIM300::hangup() {
    gsm->print(F("ATH\r")); // Response: 'OK'
#ifdef DEBUG
    Serial.println(F("Call hangup!"));
#endif
    callState = CALL_IDLE;
}

void GSMSIM300::answer() {
    gsm->print(F("ATA\r"));
    callState = CALL_ACTIVE;
}

void GSMSIM300::listSMS(const char *type) {
    setSMSTextMode();
    gsm->print(F("AT+CMGL=\""));
    gsm->print(type);
    gsm->print(F("\"\r"));

#ifdef DEBUG
    // Returned as:
    // +CMGL: 1,"REC READ","number",,"13/06/16,15:01:58+08"
    // Content
    
    uint32_t startTime;
    const char *header = "+CMGL:";
    char *pHeader = (char*)header;

    while (1) {
        startTime = millis();
        while (!checkString(gsm->read(),header,&pHeader)) {
            if (millis() - startTime > 1000)
                return;
        }
        while (gsm->read() != ',') {
            if (millis() - startTime > 1000)
                return;
        }

        if (extractContent(numberIn, sizeof(numberIn), ',', '"', 1) && extractContent(messageIn, sizeof(messageIn), '\n', '\r', 0)) { // TODO: Take care of new line in a message
            Serial.print(F("Received: \""));
            Serial.print(messageIn);
            Serial.print(F("\" From: "));
            Serial.println(numberIn);
        } else
            return;
    }
#endif
}

void GSMSIM300::deleteSMSAll(const char *type) {
    setSMSTextMode();
    gsm->print(F("AT+CMGDA=\""));
    gsm->print(type);
    gsm->print(F("\"\r"));

#ifdef DEBUG
    Serial.print(F("Deleted all messages of the following type: "));
    Serial.println(type);
#endif
}

void GSMSIM300::deleteSMS(char *index) {
    if (index == NULL) {
        if (strlen(lastIndex) == 0) {
#ifdef DEBUG
            Serial.println(F("No index was set"));
#endif
            return;
        }
        index = lastIndex;
    }
    setSMSTextMode();
    gsm->print(F("AT+CMGD="));
    gsm->print(index);
    gsm->print(F("\r"));

#ifdef DEBUG
    Serial.print(F("Deleted SMS at index: "));
    Serial.println(index);
#endif
}

//gsm->print(F("ATS0=001\r")); // Activate auto answer - 'RING' will be received on an incoming call
//gsm->print(F("AT+CSQ\r")); // Check signal strength - response: 'OK' and then the information

void GSMSIM300::sendSMS(const char *num, const char *mes) {
    strcpy(numberOut,num);
    strcpy(messageOut,mes);
    smsState = SMS_MODE;
}

bool GSMSIM300::readSMS(char *index) {
    if (index == NULL) {
        if (strlen(lastIndex) == 0) {
#ifdef DEBUG
            Serial.println(F("No index was set"));
#endif
            return false;
        }
        index = lastIndex;
    }
    newSms = false;
    setSMSTextMode();
    gsm->print(F("AT+CMGR="));
    gsm->print(index);
    gsm->print(F("\r"));

    // Read the sender's number and the message. The SMS is returned as:
    // +CMGR: "REC UNREAD","number",,"date"
    // message

    bool numberFound = extractContent(numberIn, sizeof(numberIn), ',', '"', 1);
    bool messageFound = extractContent(messageIn, sizeof(messageIn), '\n', '\r', 0); // TODO: Take care of new line in a message
#ifdef DEBUG
    if (numberFound) {
        Serial.print(F("Extracted the following number: "));
        Serial.println(numberIn);
    }
    if (messageFound) {
        Serial.print(F("Received message: "));
        Serial.println(messageIn);
    }
#endif
    return numberFound && messageFound;
}

// TODO: Replace with Stream implementation
bool GSMSIM300::extractContent(char *buffer, uint8_t size, char beginChar, char endChar, uint8_t offset) {
    uint32_t startTime = millis();
    int8_t i = 0;  

    while (gsm->read() != beginChar) {
        if (millis() - startTime > 1000)
            return false;
    }

    while(offset) { // Offset from beginChar to the actual string
        if (gsm->read() != -1)
            offset--;
        if (millis() - startTime > 1000)
            return false;
    }

    while (millis() - startTime < 1000) { // Only do this for 1s
        char c;
        do {
            c = gsm->read();
            if (millis() - startTime > 1000)
                return false;
        } while (c == -1);

#ifdef EXTRADEBUG
        Serial.write(c);
#endif
        if (c == endChar) {
            buffer[i] = '\0';
            return true;
        }
        buffer[i++] = c;
        if (i >= size) {
#ifdef DEBUG
            Serial.println(F("String is too large for the buffer"));
            return false;
#endif
        }
    }
    return false;
}

void GSMSIM300::setSMSTextMode() {
    gsm->print(F("AT+CMGF=1\r")); // Set SMS type to text mode
}

void GSMSIM300::powerOn() {
    digitalWrite(powerPin,LOW);
    delay(4000);
    digitalWrite(powerPin,HIGH);
    delay(2000);
}

void GSMSIM300::powerOff() {  
    digitalWrite(powerPin,LOW);
    delay(800);
    digitalWrite(powerPin,HIGH);
    delay(6000);
}