/**
 * UM3750.cpp - An arduino library to emulate the UM3750 encoder and transmit 
 *              fixed 12 bit codes over the 315/433MHz bandwidth
 * Created by Francesco Bianco Morghet in July 2019
 * Distributed under the MIT License
 */

#include "UM3750.h"

uint8_t UM3750::timer1_init_done = 0;

uint8_t UM3750::transmit_vals[TRANSITIONS_PER_SYMBOL * SYMBOLS * 2];
uint8_t UM3750::transmit_i;

uint32_t UM3750::transmit_current_times;
uint32_t UM3750::transmit_total_times;

uint8_t UM3750::transmit_pin;
	

UM3750::UM3750(void) {
	this->transmitPin = -1;
}

void UM3750::enableTransmit(uint8_t pin) {
	this->transmitPin = pin;
		
	pinMode(this->transmitPin, OUTPUT);
	digitalWrite(this->transmitPin, LOW);
	if(!UM3750::timer1_init_done) {
		timer1_isr_init();
		timer1_attachInterrupt(UM3750ISRtransmit);
		UM3750::timer1_init_done++;
	}
}

void UM3750::disableTransmit(void) {
	if(UM3750::transmit_pin >= 0) {
		UM3750::timer1_init_done--;
		// before detaching the interrupt, be sure that every other UM3750
		// object is enabled to transmit (ie has called enableTransmit but
		// not disableTransmit yet)
		if(!UM3750::timer1_init_done) {
			timer1_detachInterrupt();
			UM3750::transmit_total_times = 0;
		}
	}
}

void UM3750::transmitCode(Code code, uint32_t times) {	
	if(this->transmitPin < 0 || times == 0)
		return;
		
	uint8_t i = 0;
	
	while(UM3750::isTransmitting())
		yield();
	
	UM3750::transmit_pin = this->transmitPin;
	
	UM3750::transmit_i = 0;
	UM3750::transmit_current_times = 0;
	UM3750::transmit_total_times = times;
		
	for(i = 0; i < TOTAL_TRANSITIONS; i++) {
		UM3750:transmit_vals[i] = 0;  
	}
	
	UM3750::transmit_vals[2] = 1; // sync pulse
	
	for(i = 0; i < SYMBOLS; i++){
		UM3750::transmit_vals[TRANSITIONS_PER_SYMBOL + (i * TRANSITIONS_PER_SYMBOL)] = 0;
		UM3750::transmit_vals[TRANSITIONS_PER_SYMBOL + (i * TRANSITIONS_PER_SYMBOL) + 1] = ((code.value >> (SYMBOLS - (i + 1))) & 0x01);
		UM3750::transmit_vals[TRANSITIONS_PER_SYMBOL + (i * TRANSITIONS_PER_SYMBOL) + 2] = 1;   
	}
	
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
	// As the ESP8266 run at 8MHz and we set a division by 16 on the counter,
	// it means that the counter ticks every 0.2 microseconds.
	// So we first multiply symbolDurationTime by five to get the number of
	// ticks after which a single symbol duration time has passed.
	// Then we divide that number by 3 as we need to update the logical value
	// 3 times during each symbol duration time.
	timer1_write((uint32_t) ((code.symbolDurationTime * 5.0) / 3.0));
}

void UM3750::transmitCode(Code code) {
	this->transmitCode(code, DEFAULT_TRANSMIT_TIMES);
}

bool UM3750::isTransmitting() {
	return 
		UM3750::transmit_total_times && 
		UM3750::transmit_current_times < UM3750::transmit_total_times;
}

// From Arduino/cores/esp8266/core_esp8266_wiring_digital.cpp
void ICACHE_RAM_ATTR UM3750::__digitalWrite(uint8_t pin, uint8_t val) {
	if(pin < 16){
		if(val) GPOS = (1 << pin);
		else GPOC = (1 << pin);
	} else if(pin == 16){
		if(val) GP16O |= 1;
		else GP16O &= ~1;
	}
}

void ICACHE_RAM_ATTR UM3750::UM3750ISRtransmit(void) {
	UM3750::__digitalWrite(UM3750::transmit_pin, UM3750::transmit_vals[UM3750::transmit_i]);
	
	if(++UM3750::transmit_i == TOTAL_TRANSITIONS){
		UM3750::transmit_i = 0;
		UM3750::transmit_current_times++;  
	}
	
	if(UM3750::transmit_current_times == UM3750::transmit_total_times)
		timer1_disable();
}
