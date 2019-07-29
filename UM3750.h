/**
 * UM3750.h - An arduino library to emulate the UM3750 encoder and trasmit fixed
 *            12 bit codes over the 315/433MHz bandwidth
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
#define DEFAULT_PLAY_TIMES 48

class UM3750Class {
public:

	UM3750Class(void);
	
	void enableTransmit(int pin, float symbolDurationTime);
	void enableTransmit(int pin);
	
	void disableTransmit(void);
	
	void playCode(bool code[], int playTimes);
	void playCode(bool code[]);
	
private:
	
	int pin;
	float symbolDurationTime;
	
	/**
	 * Each symbol can be decomposed into three steps of same duration.
	 * The first is always low, the second is either low or high whether it is 
	 * 0 or 1 being transmitted, and the third is always high.
	 *      ________                ____
	 *     |                       |
	 * ____|               ________|
	 *     One                 Zero
	 *
	 * Before transmitting the 12 symbols of data, there is a sync symbol, which
	 * is equal to a zero symbol.
	 * So in total a single transmission consists of
	 * - 1 sync symbol
	 * - 12 data symbols
	 * - 11 symbols time equivalent of silence
	 *
	 * For simplicity an array is used to store these values in order to keep 
	 * the ISR short and simple.
	 */	
	static uint8_t tick_vals[TOTAL_TRANSITIONS];
	static uint8_t tick_i;
	
	/**
	 * Store how many times the code has been played
	 */
	static int played_current_times;
	static int played_total_times;
	
	/**
	 * Digital pin address accessed in the ISR
	 */
	static int play_pin;
	
	static void UM3750ISR(void);
	
	static void __digitalWrite(uint8_t pin, uint8_t val);
	
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_UM3750)
	extern UM3750Class UM3750;
#endif

#endif