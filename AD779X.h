/*************************************************************************
* AD7799 libary
* by Christodoulos P. Lekkos <tolis81@gmail.com> , September 03, 2014.
*
* This file is free software; you can redistribute it and/or modify
* it under the terms of either the GNU General Public License version 3
* published by the Free Software Foundation.
*************************************************************************/


/*************************************************************************
* Mode register MSB Byte
**************************************************************************
* 7:5 * MD2-MD0  * Operation Mode
* 4   * PSW      * Power Switch Control
* 3:0 * 0        * Always 0
*************************************************************************/ 
 
/*************************************************************************
* Mode register LSB Byte
**************************************************************************
* 7:4 * 0       * Always 0
* 3:0 * FS3-FS0 * Filter Update Rate
*************************************************************************/ 
 
/*************************************************************************
* Configuration register MSB Byte
**************************************************************************
* 7:6 * 0     * Always 0
* 5   * BO    * Burnout Current (only when buffer or amp is active)
* 4   * U/~B  * Unipolar Bipolar
* 3   * 0     * Always 0
* 2:0 * G2-G0 * Gain Select 1,2,4,..,128
*************************************************************************/ 
 
/*************************************************************************
* Configuration register LSB Byte
**************************************************************************
* 7:6 * 0       * Always 0
* 5   * REF_DET * Reference detect function on/off
* 4   * BUF     * Enable buffer, can disable only when gain equals 1 or 2
* 3   * 0       * Always 0
* 2:0 * CH2-CH0 * Channel Select 0,1,2
*************************************************************************/

/*************************************************************************
* Communication register Byte
**************************************************************************
* 7	  * ~WEN    * Always 0 to clock the data to Communication register
* 6   * R/~W    * Specifies if the next operation is Read or Write
* 5:3 * RS2-RS0 * Register Selection Bits 0,1,...,7
* 2   * CREAD   * Set for Continuous Read of the Data register
* 1:0 * CR1-CR0 * Always 0
*************************************************************************/

/*************************************************************************
* Status register Byte
**************************************************************************
* 7	  * ~RDY    * Monitor for end of conversion and calibration
* 6   * ERR     * In case of overrange or underrange
* 5   * NOREF   * Cleared to indicate that a valid reference is applied
* 4   * 0       * Always 0
* 3   * 0/1     * 0 indicates an AD7798, 1 indicates an AD7799
* 2:0 * CH2-CH0 * Indicates which channel is being converted by the ADC
*************************************************************************/

/*************************************************************************
* IO register Byte
**************************************************************************
* 7	  * 0       * Always 0
* 6   * IOEN    * 0 AiN3(+)/P1 and AIN3(-)/P2 pins as DO, 1 as AI
* 5   * IO2DAT  * When IOEN is set, 0 set P2 LOW, 1 HIGH
* 4   * IO1DAT  * When IOEN is set, 0 set P1 LOW, 1 HIGH
* 3:0 * 0       * Always 0
*************************************************************************/ 


#ifndef AD779X_H
#define AD779X_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "SPI.h"

// Communication Register
#define READ_REG				0x40
#define WRITE_REG				0x00
#define ENTER_CREAD				0x5C
#define EXIT_CREAD				0x58

// Register Selection		   (0x00 : 0x07) << 3
#define COMM_REG				0x00
#define STATUS_REG				0x00
#define MODE_REG				0x08
#define CONFIG_REG 				0x10
#define DATA_REG				0x18
#define ID_REG					0x20
#define IO_REG					0x28
#define OFFSET_REG				0x30
#define FULL_SCALE_REG			0x38

// Operating Modes 			   (0x00 : 0x07) << 5
#define CONT_CONV_MODE			0x00
#define SNGL_CONV_MODE			0x20
#define IDLE_MODE				0x40
#define PWR_DWN_MODE			0x60
#define INT_ZERO_SCALE_CAL      0x80
#define INT_FULL_SCALE_CAL      0xA0
#define SYS_ZERO_SCALE_CAL		0xC0
#define SYS_FULL_SCALE_CAL		0xE0

// Register Masks
#define OPERATING_MODE_MASK		0x1F
#define CHANNEL_MASK			0xF8

// Useful declarations
#define RESET_ADC               0xFF 
#define STUFFIN                 0x00

// adcFlags
#define CLEAR					0x00
#define SET						0x01

#define ADC_MODEL				0X00
#define FIRST_MEASUREMENT		0X01
#define CALIBRATE				0x02
#define CREAD					0x03

#define DEBUG_ADC 				0	// set to 1 for debugging


class AD779X
{
	public:
	 unsigned char adcFail;
		AD779X(float vRef);
		void Begin(int csPin);
		void Setup(unsigned char numberOfChannels = 3, unsigned char firstChannel = 0, unsigned char secondChannel = 1, unsigned char thirdChannel = 2);
		void Config(unsigned char gain = 0x07, unsigned char coding = 0x01, unsigned char updateRate = 0x09, unsigned char buffer = 0x01, unsigned char refDet = 0x00, unsigned char burnoutCurrent = 0x00, unsigned char powerSwitch = 0x00);
		void cRead(unsigned char channel, unsigned char enter);		
		void readID();
		bool Update();
		unsigned char StatusReg();
		unsigned long readRaw(unsigned char channel);
		float readmV(unsigned char channel);

	private:
	 unsigned long _previousMillis, _settleTime, _offsetReg[3], _fullScaleReg[3], _dataRaw[3];
	 unsigned char _csPin, _nBytes, _adcChannels, _numberOfChannels, _channelIndex, _modeRegFByte, _modeRegSByte, _configRegFByte,_configRegSByte, _adcFlags, _channelArray[3];
	 float _vRef, _gain, _datamV[3];
	 long _interval;
		void Init();
		void adcReset();
		void adcResetVars();
		void adcWrite(unsigned char registerSelection, unsigned char val);
		void adcCalibrate(unsigned char mode);
		void adcFlag(unsigned char bit, unsigned char flag);
		// void adcCheck();
		void startConversion(unsigned char channel);
		bool adcFlag(unsigned char flag);
		unsigned char adcCommRegByte(unsigned char registerAddressBits, unsigned char operation);
		unsigned long adcRead(unsigned char registerSelection);
};
#endif 