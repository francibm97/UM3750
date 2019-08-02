/**
 * UM3750.h - An arduino library to emulate the UM3750 encoder/decoder and 
 *            transmit or receive fixed 12 bit codes over the 315/433MHz 
 *            bandwidth.
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

#define DEFAULT_MIN_RECEIVE_FOUND_TIMES 4

// Set maximum interrupt handlers. If changed, you need to change
// ISR_RECEIVE_DEFS_H, ISR_RECEIVE_DEFS_CPP and ISR_RECEIVE_ASSIGN to.
#define MAX_RECEIVE_DEVICES 3

// Receive macro definitions
#define _UM3750ISRreceiveh(n) static void UM3750ISRreceive_##n(void)
#define _UM3750ISRreceivec(n) void ICACHE_RAM_ATTR UM3750::UM3750ISRreceive_##n(void) { current_receive_index = n; UM3750::UM3750ISRreceive(); }

#define ISR_RECEIVE_DEFS_H _UM3750ISRreceiveh(0); _UM3750ISRreceiveh(1); _UM3750ISRreceiveh(2);
#define ISR_RECEIVE_DEFS_CPP _UM3750ISRreceivec(0); _UM3750ISRreceivec(1); _UM3750ISRreceivec(2);
#define ISR_RECEIVE_ASSIGN(n) (n) == 0 ? UM3750::UM3750ISRreceive_0 : (n) == 1 ? UM3750::UM3750ISRreceive_1 : (n) == 2 ? UM3750::UM3750ISRreceive_2 : 0

// shorthan
#define ISRGVAR(v) UM3750::receive[current_receive_index].v

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
	
	void enableReceive(uint8_t pin, uint8_t minFoundTimes);
	void enableReceive(uint8_t pin);
	void disableReceive(void);
	
	void transmitCode(Code code, uint32_t times);
	void transmitCode(Code code);
	static bool isTransmitting();
	
	bool isReceivedCodeAvailable();
	Code getReceivedCode();
	void resetReceivedCode();
			
private:
	
	int16_t transmitPin, receivePin;
	
	// ISR receive index
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
	
	///////////////////////////////
	// STATIC RECEIVE VARIABLES //
	/////////////////////////////
	static uint8_t current_receive_index;
	static uint8_t receive_activated[MAX_RECEIVE_DEVICES];
	
	struct receive_s {
		uint8_t min_found_times;

		uint32_t durations[32];
		uint8_t durations_i;

		unsigned long last_time;

		uint16_t found_code;
		float avg_ts;
		volatile uint8_t found_code_times;
		
		uint8_t pin;
	};
	
	static receive_s receive[MAX_RECEIVE_DEVICES];
	
	static void __digitalWrite(uint8_t pin, uint8_t val);
	
	static void UM3750ISRtransmit(void);
	
	static void UM3750ISRreceive(void);
	ISR_RECEIVE_DEFS_H
		
};

#endif