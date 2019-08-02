/**
 * UM3750.h - An arduino library to emulate the UM3750 encoder and transmit 
 *            fixed 12 bit codes over the 315/433MHz bandwidth
 * Created by Francesco Bianco Morghet in July 2019
 * Distributed under the MIT License
 */

#ifndef UM3750_h
#define UM3750_h

#include "Arduino.h"

#define TRANSITIONS_PER_SYMBOL 3
#define SYMBOLS 12
#define TOTAL_TRANSITIONS TRANSITIONS_PER_SYMBOL * SYMBOLS * 2

// According to the datasheet, 0.96ms is the symbol duration time when the IC
// works at a frequency of 100 Khz
#define DEFAULT_SYMBOL_DURATION_TIME 960 //us
// Should be fine for most receivers
#define DEFAULT_TRANSMIT_TIMES 48

class UM3750 {
public:

	struct Code {
		uint16_t value;
		float symbolDurationTime = DEFAULT_SYMBOL_DURATION_TIME;
		
		Code(int value) {
			this->value = value;
		}
		
		Code(int value, float symbolDurationTime) {
			this->value = value;
			this->symbolDurationTime = symbolDurationTime;
		}
	};

	UM3750(void);
		
	void enableTransmit(uint8_t pin);	
	void disableTransmit(void);
	
	//void enableReceive(uint8_t pin);
	//void disableReceive(void);
	
	void transmitCode(Code code, uint32_t times);
	void transmitCode(Code code);
	static bool isTransmitting();
	
	//UM3750Code receiveCode(uint8_t minValids);
	//UM3750Code receiveCode();
			
private:
	
	int16_t transmitPin, receivePin;
	
	uint8_t receiveIndex;
	
	////////////////////////////////
	// STATIC TRANSMIT VARIABLES //
	//////////////////////////////
	struct transmit_s {
		uint8_t timer1_init_done;
		
		/**
		 * Each symbol can be decomposed into three steps of same duration.
		 * The first is always low, the second is either low or high whether it  
		 * is 0 or 1 being transmitted, and the third is always high.
		 *      ________                ____
		 *     |                       |
		 * ____|               ________|
		 *     One                 Zero
		 *
		 * Before transmitting the 12 symbols of data, there is a sync symbol,
		 * which is equal to a zero symbol.
		 * So in total a single transmission consists of
		 * - 1 sync symbol
		 * - 12 data symbols
		 * - 11 symbols time equivalent of silence
		 *
		 * For simplicity an array is used to store these values in order to keep 
		 * the ISR short and simple.
		 */	
		uint8_t vals[TOTAL_TRANSITIONS];
		uint8_t i;
		
		/**
		 * Store how many times the code has been played
		 */
		uint32_t current_times;
		uint32_t total_times;
		
		/**
		 * Digital pin address accessed in the ISRs
		 */
		uint8_t pin;
	};
	
	static transmit_s transmit;
		
	static void __digitalWrite(uint8_t pin, uint8_t val);
	
	static void UM3750ISRtransmit(void);
	//static void UM3750ISRreceive(void);
		
};

#endif