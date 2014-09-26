/*************************************************************************
* AD7799
* by Christodoulos P. Lekkos <tolis81@gmail.com> , September 03, 2014.
*
* This file is free software; you can redistribute it and/or modify
* it under the terms of either the GNU General Public License version 3
* published by the Free Software Foundation.
*************************************************************************/

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <AD779X.h>

/* _adcFlags byte
* bit location * Description
*			 7 * 
*			 6 * 
*			 5 * 
*			 4 * 
*			 3 * CREAD
*			 2 * Calibrate
*			 1 * First measurement
*			 0 * ADC model (0/1 AD7798/AD7799)
*/	
	
/* General purpose functions (Private)
 ************************************************************************************************************************
 * adcCommRegByte(MODE_REG, READ_REG)					function to create the byte for a Read operation to the Mode register
 * adcRead(ID_REG)									return the corresponding register value
 * adcWrite(unsigned char registerSelection, unsigned char val)		write the First and Second byte of the corresponding register
 ************************************************************************************************************************
 */
 
 unsigned char AD779X::adcCommRegByte(unsigned char registerAddressBits, unsigned char operation) { // choose register according the Register Address Bits and set the R/~W bit for a reading operation 
	unsigned char commRegVal = 0;
	if (operation == READ_REG) {
		commRegVal = registerAddressBits | READ_REG;
	}
	else {
		commRegVal = registerAddressBits | WRITE_REG;
	}
	#if DEBUG_ADC
		if (operation == READ_REG) {
			Serial.print("Setting Communication Register for a READ Operation: "); 
		}
		else {
			Serial.print("Setting Communication Register for a WRITE Operation: "); 
		}
		Serial.println(commRegVal, BIN);
	#endif
	return commRegVal;
}

unsigned long AD779X::adcRead(unsigned char registerSelection) {
	unsigned long registerValue = 0;
	unsigned char incomingByte = 0;
	if (!(registerSelection == DATA_REG && adcFlag(CREAD))) {						// in CREAD there is no need to specify the Communication register for a read to Data register
		unsigned char commReg = adcCommRegByte(registerSelection, READ_REG);
		incomingByte = SPI.transfer(commReg);
	}
	else {
		#if DEBUG_ADC
			Serial.println("ADC in CREAD mode");
		#endif
	}
	incomingByte = SPI.transfer(STUFFIN);
	if (registerSelection == STATUS_REG || registerSelection == ID_REG || registerSelection == IO_REG) {		
		registerValue = incomingByte;
		#if DEBUG_ADC
			Serial.print("Reading an 8-bit Register: ");
			Serial.println(registerValue, BIN);
		#endif		
		return registerValue;
	}
	else {
		#if DEBUG_ADC
			Serial.print("First Byte: ");
			Serial.print(incomingByte, BIN);
		#endif
		registerValue = incomingByte << 8; 											// store selected register FByte
		incomingByte = SPI.transfer(STUFFIN);                      						// read selected register S(econd)Byte
		#if DEBUG_ADC
			Serial.print(" Second Byte: ");
			Serial.print(incomingByte, BIN);
		#endif
		registerValue |= incomingByte;
		if (registerSelection == MODE_REG || registerSelection == CONFIG_REG || (registerSelection == DATA_REG && !adcFlag(ADC_MODEL)) || (registerSelection == OFFSET_REG && !adcFlag(ADC_MODEL)) || (registerSelection == FULL_SCALE_REG && !adcFlag(ADC_MODEL))) {	
			#if DEBUG_ADC
				Serial.print(" Reading a 16-bit Register: ");
				Serial.println(registerValue, BIN);
			#endif			
			return registerValue;
		}
		else {
			registerValue <<= 8;
			incomingByte = SPI.transfer(STUFFIN);											// read selected register Third Byte
			registerValue |= incomingByte;												// store 24-bit register value
			registerValue &= 0xFFFFFF;
			#if DEBUG_ADC
				Serial.print(" Third Byte: ");
				Serial.print(incomingByte, HEX);
				Serial.print(" Read a 24-bit Register: ");
				Serial.println(registerValue, HEX);
			#endif				
			return registerValue;
		}
	}
}

void AD779X::adcWrite(unsigned char registerSelection, unsigned char val) {							// write Mode Register and select Operating Mode OR write Offset/Full-Scale register value
	unsigned char commReg = adcCommRegByte(registerSelection, WRITE_REG);
	unsigned char incomingByte = SPI.transfer(commReg);	// specify the communication register for a writing operation to the selected register	
	if (registerSelection == CONFIG_REG) {
		_configRegSByte = (_configRegSByte & CHANNEL_MASK) | val;
		#if DEBUG_ADC
			Serial.print("Writing Configuration Register FByte: ");
			Serial.println(_configRegFByte, BIN);
			Serial.print("Writing Configuration Register SByte: ");
			Serial.println(_configRegSByte, BIN);			
		#endif
		incomingByte = SPI.transfer(_configRegFByte);								// write CONFIGURATION REGISTER FByte
		incomingByte = SPI.transfer(_configRegSByte);							// write CONFIGURATION REGISTER SByte - val: Channel Select
	}
	else if (registerSelection == MODE_REG) {
		_modeRegFByte = (_modeRegFByte & OPERATING_MODE_MASK) | val;
		#if DEBUG_ADC
			Serial.print("Writing Mode Register FByte: ");
			Serial.println(_modeRegFByte, BIN);
			Serial.print("Writing Mode Register SByte: ");
			Serial.println(_modeRegSByte, BIN);			
		#endif
		incomingByte = SPI.transfer(_modeRegFByte);							// write MODE REGISTER FByte - val: Operating Mode
		incomingByte = SPI.transfer(_modeRegSByte);									// write MODE REGISTER SByte
	}
	else if (registerSelection == IO_REG) {											// write IO REGISTER
		incomingByte = SPI.transfer(val);
	}
	else if (registerSelection == OFFSET_REG || registerSelection == FULL_SCALE_REG) {	// write OFFSET or FULL-SCALE REGISTER (16-bits for AD7798 / 24-bits for AD7799)
		for (int i = 0; i < _nBytes; i++) {
			incomingByte = SPI.transfer(val >> 8*(_nBytes - i - 1));
		}
	}
	// delay(1);
}

void AD779X::adcFlag(unsigned char bit, unsigned char flag) {
	if (bit == SET) {
		_adcFlags |= 1 << flag;
	}
	else if (bit == CLEAR) {
		_adcFlags &= ~(1 << flag);
	}
}

bool AD779X::adcFlag(unsigned char flag) {
	return _adcFlags & (1 << flag);
}

/* END of General purpose functions */
 
 
 
/* Private Functions
 *******************************************************************
 * Reset()	Reset the ADC
 * Init()	Reset the ADC,
 *			store the ID,
 *			set variables according the ADC model
 *			configure each channel according the latest user inputs,
 * 			calibrate the channel and
 *			store gain
 *******************************************************************
 */
 
void AD779X::adcReset() {						// write 32 ones to reset ADC
	#if DEBUG_ADC
		Serial.println("Reseting the ADC...");
	#endif	
	byte incomingByte = 0;
	for (int i = 0; i < 4; i++) {				// send 0xFFFFFFFF
		incomingByte = SPI.transfer(RESET_ADC);
	}
	delayMicroseconds(500);						// (datasheet --> p.23 ~p.19) wait 500us
	#if DEBUG_ADC
		Serial.println("ADC reset");
	#endif	
}

void AD779X::adcResetVars() {
	#if DEBUG_ADC
		Serial.println("Initalizing variables");
	#endif
	_adcChannels = 3;			// ADC has 3 physical channels
	_adcFlags = 0;				// reset the flags
	_gain = 128;				// reinitialize gain
	_settleTime = 120;			// reset settle time to default value
	_numberOfChannels = 3;		// reset number of channels used to default value
	_channelIndex = 0;			// reset channel indexing
	_configRegFByte = 0x07;		// default value of Configuration Register First Byte (datasheet p.16)
	_configRegSByte = 0x10;		// default value of Configuration Register Second Byte (datasheet p.16)
	_modeRegFByte = 0x40;		// default value of Mode Register First Byte (datasheet p.14)
	_modeRegSByte = 0x0A;		// default value of Mode Register Second Byte (datasheet p.14)
}

 void AD779X::Init() {
	#if DEBUG_ADC
		Serial.println("Start of Init()");
	#endif
	adcResetVars();					// reset variables to default state
	adcReset();							// reset the device

	if (adcRead(STATUS_REG) & 0x08) {	// store adc model
		#if DEBUG_ADC
			Serial.println("ADC Model: AD7799");
		#endif
		adcFlag(SET, ADC_MODEL);
		_nBytes = 3;
	}
	else {
		#if DEBUG_ADC
			Serial.println("ADC Model: AD7798");
		#endif	
	_nBytes = 2;
	}
	#if DEBUG_ADC
		Serial.println("End of Init()");
	#endif
}

/* Public Functions
 *******************
 * AD7799 myADC(2.5)						initalize a new library object with the name myADC
											the applied vRef is 2.5V
 * myADC.Begin(2)							the device cs pin in number 2
 * myADC.Config(1, 2, 1, 1, 0, 0, 0, 0)		ADC and channel specific configuration
 * myADC.readRaw(1)							read channel 1 and return raw value
 * myADC.readmV(2)							read channel 2 and return value in mV
 **********************************************************************************************
 */
AD779X::AD779X(float vRef) {
	_vRef = vRef;
}

void AD779X::Begin(int csPin) {
	_csPin = csPin;							// store the CS pin
	pinMode(_csPin, OUTPUT);				// set cs pin as output
	#if DEBUG_ADC
		Serial.println("Start of Begin()");
		Serial.print("ADC CS PIN: ");
		Serial.println(_csPin);
	#endif	
	digitalWrite(_csPin, LOW);				// select the device
	Init();
	long statusByte = StatusReg();
	if (!statusByte) {
		#if DEBUG_ADC
			Serial.println("NO CHIP PRESENT");
		#endif
		while(1){
		}
	}
	digitalWrite(_csPin, HIGH);				// deselect the device
	#if DEBUG_ADC
		Serial.println("End of Begin()");
	#endif
}

unsigned char AD779X::StatusReg() {
	unsigned char statusReg = adcRead(STATUS_REG);
	return statusReg;
}


void AD779X::Setup(unsigned char numberOfChannels, unsigned char firstChannel, unsigned char secondChannel, unsigned char thirdChannel) {
	_numberOfChannels = numberOfChannels;
	unsigned char channelArray[3] = {firstChannel, secondChannel, thirdChannel};
	for (int i = 0; i < _numberOfChannels; i++) {
		_channelArray[i] = channelArray[i];
	}
	#if DEBUG_ADC
		Serial.println("****************************");
		Serial.println("ADC Setup");
		Serial.print("Number of channels: ");
		Serial.print(_numberOfChannels);
		Serial.print("\tSelected channels: ");
		for (int i = 0; i < _numberOfChannels; i++) {
			Serial.print(_channelArray[i]);
		}
		Serial.println("");
		Serial.println("****************************");
	#endif
}

void AD779X::Config(unsigned char gain, unsigned char coding, unsigned char updateRate, unsigned char buffer, unsigned char refDet, unsigned char burnoutCurrent, unsigned char powerSwitch) {
	unsigned char newConfigRegFByte = ((burnoutCurrent << 5) & 0x20) | ((coding << 4) & 0x10) | (gain) & 0x07;	// store values passed by user
	unsigned char newConfigRegSByte = ((refDet << 5) & 0x20) | (buffer << 4) & 0x10;							// store values passed by user
	unsigned char newModeRegFByte = (powerSwitch << 4) & 0x10;													// store values passed by user
	unsigned char newModeRegSByte = updateRate & 0x0F;															// store values passed by user
	if (_modeRegSByte & 0x0F != updateRate) { 																	// check if update rate has been changed
		if (updateRate == 0x01) {
		_settleTime = 4;
		}
		else if (updateRate == 0x02) {
		_settleTime = 8;
		}
		else if (updateRate == 0x03) {
		_settleTime = 16;
		}
		else if (updateRate == 0x04) {
		_settleTime = 32;
		}
		else if (updateRate == 0x05) {
		_settleTime = 40;
		}
		else if (updateRate == 0x06) {
		_settleTime = 48;
		}
		else if (updateRate == 0x07) {
		_settleTime = 60;
		}
		else if (updateRate == 0x08) {
		_settleTime = 101;
		}
		else if (updateRate == 0x09 || updateRate == 0x0A) {
		_settleTime = 120;
		}
		else if (updateRate == 0x0B) {
		_settleTime = 160;
		}
		else if (updateRate == 0x0C) {
		_settleTime = 200;
		}
		else if (updateRate == 0x0D) {
		_settleTime = 240;
		}
		else if (updateRate == 0x0E) {
		_settleTime = 320;
		}
		else if (updateRate == 0x0F) {
		_settleTime = 480;
		}
		#if DEBUG_ADC
			Serial.print("New Update Rate. Settling time: ");
			Serial.println(updateRate);
		#endif
	}
	if (_configRegFByte & 0x07 != gain & 0x07) {									// in case the gain has been changed 
		_gain = 2 << (gain - 1);													// store new gain
		#if DEBUG_ADC
			Serial.print("Setting Gain: ");
			Serial.println(_gain);
		#endif
		if  (gain <= 0x02 || (gain > 0x02 && updateRate <= 0x05) | gain != 0x07) {	// check if calibration is needed - datasheet p.15 - p24
			adcFlag(SET, CALIBRATE);												// raise the calibration flag if needed
		}
	}
	if (adcFlag(CALIBRATE)) {					// if calibration is needed
		_configRegFByte = newConfigRegFByte;	// store new Configuration Register FByte
		_configRegSByte = newConfigRegSByte;	// store new Configuration Register SByte
		_modeRegFByte = newModeRegFByte;		// store new Mode Register FByte
		_modeRegSByte = newModeRegSByte;		// store new Mode Register	SByte
		digitalWrite(_csPin, LOW);				// select the device
		adcCalibrate(INT_FULL_SCALE_CAL);		// select each channel and calibrate
		digitalWrite(_csPin, HIGH);				// deselect the device
		adcFlag(CLEAR, CALIBRATE);				// clear calibration flag
	}
	else {										// in case no calibration is needed check
		if (_configRegFByte != newConfigRegFByte || _configRegSByte != newConfigRegSByte || _modeRegFByte != newModeRegFByte || _modeRegSByte != newModeRegSByte) {
			#if DEBUG_ADC
				Serial.println("Setting new values");
			#endif
			_configRegFByte = newConfigRegFByte;
			_configRegSByte = newConfigRegSByte;
			_modeRegFByte = newModeRegFByte;
			_modeRegSByte = newModeRegSByte;
			digitalWrite(_csPin, LOW);				// select the device
			adcWrite(CONFIG_REG, 0x00);
			adcWrite(MODE_REG, IDLE_MODE);			
			digitalWrite(_csPin, HIGH);				// deselect the device
		}
	}		
}

// void AD779X::adcCheck() {
	// unsigned long readConfigReg = adcRead(CONFIG_REG);
	// unsigned long readModeReg = adcRead(MODE_REG);
	// Serial.print("Read Value: ");
	// Serial.println(readConfigReg, BIN);
	// Serial.print("Write Value:");
	// Serial.println(_configRegFByte << 8 | _configRegSByte, BIN);	
	// Serial.print("Read Value: ");
	// Serial.println(readModeReg, BIN);
	// Serial.print("Write Value:");
	// Serial.println(_modeRegFByte << 8 | _modeRegSByte, BIN);
// }

void AD779X::adcCalibrate(unsigned char calibrationMode) {
    #if DEBUG_ADC
    if (calibrationMode == INT_ZERO_SCALE_CAL) {
		Serial.println("Starting Internal Zero-Scale Calibration...");
	}
    else if (calibrationMode == INT_FULL_SCALE_CAL) {
		Serial.println("Starting Internal Full-Scale Calibration...");
	}
    else if (calibrationMode == SYS_ZERO_SCALE_CAL) {
		Serial.println("Starting System Zero-Scale Calibration...");
	}
    else if (calibrationMode == SYS_FULL_SCALE_CAL) {
		Serial.println("Starting System Full Scale Calibration...");
	}
	else {
		Serial.println("Invalid Calibration Mode");
	}
    #endif
	for (int i = 0; i < _numberOfChannels; i++) {
		adcWrite(CONFIG_REG, i);
		adcWrite(MODE_REG, calibrationMode);
		#if DEBUG_ADC
			Serial.print("Calibration of channel ");
			Serial.print(_channelArray[i]);
			Serial.println(" in progress");
		#endif	
		while((adcRead(STATUS_REG) >> 7)) {
			#if DEBUG_ADC	
				Serial.println(".");
			#endif
		}
		#if DEBUG_ADC
			Serial.print("Channel ");
			Serial.print(_channelArray[i]);
			Serial.println( " calibrated.");
		#endif
	}
	 #if DEBUG_ADC
		Serial.println("End of Calibration...");
    #endif
}

bool AD779X::Update() {
	if (!adcFlag(FIRST_MEASUREMENT)) {
		adcFlag(SET, FIRST_MEASUREMENT);
		#if DEBUG_ADC
			Serial.println("Starting first measurement");
		#endif
		digitalWrite(_csPin, LOW);
		startConversion(_channelIndex);
		digitalWrite(_csPin, HIGH);
		_previousMillis = millis();	// start the clock for first time
		return false;
	}
	else {
		unsigned long timePassed = millis() - _previousMillis;	// store time passed since last check
		#if DEBUG_ADC
			Serial.print("Settle Time: ");
			Serial.println(_settleTime);
		#endif
		if (timePassed >= _settleTime) {						// if settle time has passed				
			#if DEBUG_ADC
				Serial.print("Time Passed(ms): ");
				Serial.println(timePassed);
			#endif
			digitalWrite(_csPin, LOW);
			unsigned long statusByte = adcRead(STATUS_REG);
			if (statusByte >> 7) {						// and no data available yet
				#if DEBUG_ADC
					Serial.println("No data available yet.");
				#endif
				if (timePassed > 4*_settleTime) {				// then if it takes too long
					#if DEBUG_ADC
						Serial.print("Timeout (ms): ");
						Serial.println(timePassed);
					#endif
					adcReset();									// reset the device
					Config(_configRegFByte & 0x07, 
						   _configRegFByte & 0x10, 
						   _modeRegSByte & 0x0F, 
						   _configRegSByte & 0x10, 
						   _configRegSByte & 0x20, 
						   _configRegFByte & 0x0F, 
						   _modeRegFByte & 0x10);				// reconfigure the ADC according the last user settings
					adcFail++;									// and store a failed attempt
				}
				digitalWrite(_csPin, HIGH);						// deselect the device
				return false;
			}
			else {  											// else get data, start the measurement of the next channel and reset the clock
				if (statusByte & 0x40) {
					#if DEBUG_ADC
						Serial.print("Warning!! Channel ");
						Serial.print(_channelArray[_channelIndex]);
						Serial.println(" Overrange or Underrange");
					#endif
				}
				#if DEBUG_ADC
					Serial.println("DATA READY!!");
					Serial.print("Writing data for channel ");
					Serial.println(_channelArray[_channelIndex], DEC);
				#endif
				_dataRaw[_channelArray[_channelIndex]] = adcRead(DATA_REG);
				#if DEBUG_ADC
					Serial.print("Channel ");
					Serial.print(_channelArray[_channelIndex], DEC);
					Serial.print(" Raw Value: ");
					Serial.println(_dataRaw[_channelArray[_channelIndex]], HEX);
				#endif				
				_channelIndex = _channelIndex++ >= (_numberOfChannels - 1)  ? 0 : _channelIndex++;
				startConversion(_channelIndex);
				digitalWrite(_csPin, HIGH);
				_previousMillis = millis();
				return true;
			}
		}
		else {
			#if DEBUG_ADC
				Serial.println("Time passed < Settle time.");
			#endif
			return false;
		}
	}
}

void AD779X::startConversion(unsigned char channel) {
	#if DEBUG_ADC
		Serial.print("Starting Conversion of channel: ");
		Serial.println(_channelArray[channel], DEC);
	#endif
	channel = _channelArray[channel];
	adcWrite(MODE_REG, SNGL_CONV_MODE);		// select Conversion Mode
	adcWrite(CONFIG_REG, channel);			// select Channel
}

unsigned long AD779X::readRaw(unsigned char channel) {
	if (channel < _numberOfChannels) {	
		return _dataRaw[channel];
	}
	else {
		#if DEBUG_ADC
			Serial.println("Channel out of range.");
		#endif
		return 0xFFFFFF;
	}	
}

float AD779X::readmV(unsigned char channel) {
	if (_configRegFByte & 0x10) {																			// Unipolar Mode
		if (adcFlag(ADC_MODEL)) {																			// AD7799
			_datamV[channel] = (float)(_dataRaw[channel])*0.000000059604644775390625*_vRef/_gain*1000;		// datasheet p.23
		}
		else {																								// AD7798
			_datamV[channel] = (float)(_dataRaw[channel])*0.0000152587890625*_vRef/_gain*1000;				// datasheet p.23
		}
	}
	else { 																									// Bipolar
		if (adcFlag(ADC_MODEL)) {																			// AD7799
			_datamV[channel] = ((float)(_dataRaw[channel])*0.00000011920928955078125 - 1)*_vRef/_gain*1000;	// datasheet p.23
		}
		else {																								// AD7798
			_datamV[channel] = ((float)(_dataRaw[channel])*0.000030517578125 - 1)*_vRef/_gain*1000;			// datasheet p.23
		}
	}
	return _datamV[channel];
}
	


void AD779X::cRead(unsigned char channel, unsigned char enter) {
	unsigned char incomingByte = 0;
	if (enter && !adcFlag(CREAD)) {
		adcFlag(SET, CREAD);
		incomingByte = SPI.transfer(ENTER_CREAD);
	}
	else if (!enter && adcFlag(CREAD)) {
	adcFlag(CLEAR, CREAD);
	incomingByte = SPI.transfer(EXIT_CREAD);
	}
}

// END of public functions