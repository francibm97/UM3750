/**
 * UM3750.h - An arduino library to emulate the UM3750 encoder and trasmit fixed
 *            12 bit codes over the 315/433MHz bandwidth
 * Created by Francesco Bianco Morghet in July 2019
 * Distributed under the MIT License
 */

#include "UM3750.h"

uint8_t UM3750Class::tick_vals[TRANSITIONS_PER_SYMBOL * SYMBOLS * 2];
uint8_t UM3750Class::tick_i;

int UM3750Class::played_current_times;
int UM3750Class::played_total_times;

int UM3750Class::play_pin;
	

UM3750Class::UM3750Class(void) {
	this->pin = -1;
}

void UM3750Class::enableTransmit(int pin, float symbolDurationTime) {
	this->pin = pin;
	this->symbolDurationTime = symbolDurationTime;
	
	pinMode(this->pin, OUTPUT);
	digitalWrite(this->pin, 0);
	
	timer1_isr_init();
	timer1_attachInterrupt(UM3750ISR);
}

void UM3750Class::enableTransmit(int pin) {
	this->enableTransmit(pin, DEFAULT_SYMBOL_DURATION_TIME);
}

void UM3750Class::disableTransmit(void) {
	timer1_detachInterrupt();
	this->pin = -1;
}

void UM3750Class::playCode(bool code[], int playTimes) {	
	if(this->pin == -1)
		return;
	
	uint8_t i = 0;
	
	UM3750Class::tick_i = 0;
	UM3750Class::played_current_times = 0;
	UM3750Class::played_total_times = playTimes;
		
	for(i = 0; i < TOTAL_TRANSITIONS; i++) {
		UM3750Class:tick_vals[i] = 0;  
	}
	
	UM3750Class::tick_vals[2] = 1; // sync
	
	for(i = 0; i < SYMBOLS; i++){
		UM3750Class::tick_vals[TRANSITIONS_PER_SYMBOL + (i * TRANSITIONS_PER_SYMBOL)] = 0;
		UM3750Class::tick_vals[TRANSITIONS_PER_SYMBOL + (i * TRANSITIONS_PER_SYMBOL) + 1] = code[i];
		UM3750Class::tick_vals[TRANSITIONS_PER_SYMBOL + (i * TRANSITIONS_PER_SYMBOL) + 2] = 1;   
	}
	
	UM3750Class::play_pin = this->pin;
	
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
	timer1_write((uint32_t) ((this->symbolDurationTime * 5.0) / 3.0));
}

void UM3750Class::playCode(bool code[]) {
	this->playCode(code, DEFAULT_PLAY_TIMES);
}

// From Arduino/cores/esp8266/core_esp8266_wiring_digital.cpp
void ICACHE_RAM_ATTR UM3750Class::__digitalWrite(uint8_t pin, uint8_t val) {
	if(pin < 16){
		if(val) GPOS = (1 << pin);
		else GPOC = (1 << pin);
	} else if(pin == 16){
		if(val) GP16O |= 1;
		else GP16O &= ~1;
	}
}

void ICACHE_RAM_ATTR UM3750Class::UM3750ISR(void) {
	UM3750Class::__digitalWrite(UM3750Class::play_pin, UM3750Class::tick_vals[UM3750Class::tick_i]);
	
	if(++UM3750Class::tick_i == TOTAL_TRANSITIONS){
		UM3750Class::tick_i = 0;
		UM3750Class::played_current_times++;  
	}
	
	if(UM3750Class::played_current_times == UM3750Class::played_total_times)
		timer1_disable();
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_UM3750)
	UM3750Class UM3750;
#endif