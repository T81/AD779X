/* AD779X library
 Reading all channels of an AD7799 ADC
 Author: T81
 http://www.analog.com/en/analog-to-digital-converters/ad-converters/ad7799/products/product.html
*/

#include <SPI.h>    // include the SPI library:
#include <AD779X.h> // include the AD779X library 

AD779X myADC(1.8);  // create new object, the voltage reference is 1.8V

void setup() {

  Serial.begin(9600);          // initialize serial port
  SPI.begin();                 // wake up the SPI
  SPI.setDataMode(SPI_MODE3);  // datasheet p6-7
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV32);  // datasheet p6
  myADC.Begin(10);             // ADC attached to CS pin 10
  myADC.Setup();               // default values: 3 channels, 0...2
  myADC.Config();              // default values: gain 124, unipolar, 80dB (50 Hz only) rejection, reference detection disabled, buffer enabled, burnout current disabled, power switch disabled

}

void loop() {
  if (myADC.Update()) {                      // if new values available
    for (int i = 0; i < 3; i++) {            // print RAW and mV values
      Serial.print("CH");
      Serial.print(i);
      Serial.print(" - RAW: ");
      Serial.print(myADC.readRaw(i), HEX);
      Serial.print("\tmV: ");
      Serial.println(myADC.readmV(i), HEX); 
    }
  }
}
