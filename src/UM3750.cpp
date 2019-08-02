/**
 * UM3750.cpp - An arduino library to emulate the UM3750 encoder and transmit 
 *              fixed 12 bit codes over the 315/433MHz bandwidth
 * Created by Francesco Bianco Morghet in July 2019
 * Distributed under the MIT License
 */

#include "UM3750.h"

// Set timer1_init_done to zero
UM3750::transmit_s UM3750::transmit = {0};

uint8_t UM3750::current_receive_index;
// Initialize all elements to 0
uint8_t UM3750::receive_activated[MAX_RECEIVE_DEVICES] = {0};

UM3750::receive_s UM3750::receive[MAX_RECEIVE_DEVICES];

UM3750::UM3750(void) {
	this->transmitPin = -1;
	this->receivePin = -1;
	this->receiveIndex = -1;
}

void UM3750::enableTransmit(uint8_t pin) {
	this->transmitPin = pin;
		
	pinMode(this->transmitPin, OUTPUT);
	digitalWrite(this->transmitPin, LOW);
	if(!UM3750::transmit.timer1_init_done) {
		timer1_isr_init();
		timer1_attachInterrupt(UM3750ISRtransmit);
		UM3750::transmit.timer1_init_done++;
	}
}

void UM3750::disableTransmit(void) {
	if(UM3750::transmit.pin >= 0) {
		UM3750::transmit.timer1_init_done--;
		// before detaching the interrupt, be sure that every other UM3750
		// object is enabled to transmit (ie has called enableTransmit but
		// not disableTransmit yet)
		if(!UM3750::transmit.timer1_init_done) {
			timer1_detachInterrupt();
			UM3750::transmit.total_times = 0;
		}
	}
}

void UM3750::enableReceive(uint8_t pin, uint8_t minFoundTimes) {
	uint8_t i;
	
	for(i = 0; i < MAX_RECEIVE_DEVICES && receive_activated[i]; i++);

	if(i == MAX_RECEIVE_DEVICES)
		return; // No ISR available
	
	receive_activated[i] = 1;
	this->receivePin = pin;
	this->receiveIndex = i;
	
	UM3750::receive[i].min_found_times = minFoundTimes;
	for(uint8_t j = 0; j < 32; j++)
		UM3750::receive[i].durations[j] = 0;
	UM3750::receive[i].durations_i = 0;
	UM3750::receive[i].last_time = 0;
	UM3750::receive[i].found_code = 0;
	UM3750::receive[i].avg_ts = 0;
	UM3750::receive[i].found_code_times = 0;
	UM3750::receive[i].pin = this->receivePin;
	
	attachInterrupt(digitalPinToInterrupt(this->receivePin), ISR_RECEIVE_ASSIGN(i), CHANGE);
		
}

void UM3750::enableReceive(uint8_t pin) {
	this->enableReceive(pin, DEFAULT_MIN_RECEIVE_FOUND_TIMES);
}

void UM3750::disableReceive() {
	if(this->receivePin > 0) {
		detachInterrupt(digitalPinToInterrupt(this->receivePin));
		this->receivePin = -1;
		this->receiveIndex = -1;
	}

}

void UM3750::transmitCode(Code code, uint32_t times) {	
	if(this->transmitPin < 0 || times == 0)
		return;
		
	uint8_t i = 0;
	
	while(UM3750::isTransmitting())
		yield();
	
	UM3750::transmit.pin = this->transmitPin;
	
	UM3750::transmit.i = 0;
	UM3750::transmit.current_times = 0;
	UM3750::transmit.total_times = times;
		
	for(i = 0; i < TOTAL_TRANSITIONS; i++) {
		UM3750:transmit.vals[i] = 0;  
	}
	
	UM3750::transmit.vals[2] = 1; // sync pulse
	
	for(i = 0; i < SYMBOLS; i++){
		UM3750::transmit.vals[TRANSITIONS_PER_SYMBOL + (i * TRANSITIONS_PER_SYMBOL)] = 0;
		UM3750::transmit.vals[TRANSITIONS_PER_SYMBOL + (i * TRANSITIONS_PER_SYMBOL) + 1] = ((code.value >> (SYMBOLS - (i + 1))) & 0x01);
		UM3750::transmit.vals[TRANSITIONS_PER_SYMBOL + (i * TRANSITIONS_PER_SYMBOL) + 2] = 1;   
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
		UM3750::transmit.total_times && 
		UM3750::transmit.current_times < UM3750::transmit.total_times;
}

bool UM3750::isReceivedCodeAvailable() {
	return this->receivePin > 0 ?
		receive[this->receiveIndex].found_code_times >= receive[this->receiveIndex].min_found_times : 0;
}
UM3750::Code UM3750::getReceivedCode() {
	if(this->isReceivedCodeAvailable())
		return UM3750::Code(receive[this->receiveIndex].found_code,receive[this->receiveIndex].avg_ts);
	return UM3750::Code(0,0);
}

void UM3750::resetReceivedCode() {
	if(this->receivePin > 0)
		receive[this->receiveIndex].found_code_times = 0;
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
	UM3750::__digitalWrite(UM3750::transmit.pin, UM3750::transmit.vals[UM3750::transmit.i]);
	
	if(++UM3750::transmit.i == TOTAL_TRANSITIONS){
		UM3750::transmit.i = 0;
		UM3750::transmit.current_times++;  
	}
	
	if(UM3750::transmit.current_times == UM3750::transmit.total_times)
		timer1_disable();
}

void ICACHE_RAM_ATTR UM3750::UM3750ISRreceive(void) {
	uint32_t current_time;
    uint8_t isHigh;
	
	if(ISRGVAR(found_code_times) >= ISRGVAR(min_found_times)) {
		return;
	}
	
    isHigh = digitalRead(ISRGVAR(pin));
    current_time = micros();
    ISRGVAR(durations[ISRGVAR(durations_i)]) = abs(current_time - ISRGVAR(last_time));

    if(isHigh && ISRGVAR(durations[ISRGVAR(durations_i)]) > (ISRGVAR(durations[(ISRGVAR(durations_i) - 1) & 0x1F]) << 3)) {
      // We have had a pulse after a longer silence: check the previous values
      // to see if there is an actual transmission   
      uint8_t durations_i2 = (ISRGVAR(durations_i) - 24) & 0x1F; // beginning of the first symbol
      uint32_t ts = ISRGVAR(durations[durations_i2]) + ISRGVAR(durations[(durations_i2 + 1) & 0x1F]);
      uint32_t tolerance = ts >> 5; // 1/32 of ts
      uint8_t ok = ts > 200;
      uint32_t cur_ts;

      uint16_t current_found_code = 0; 
      uint32_t current_avg_ts = 0;
      
      // We do a redundant check for the value of the first symbol duration time
      // but we have to do so because we also determine the value of the transmitted bit.
      // If we find out that the transmission is not valid, we do not update the
      // found_code_times counter
      for(uint8_t c = 0; c < 12 && ok; c++){
          current_found_code <<= 1;
          // Determine the current bit (assuming valid transmission)
          current_found_code |= ISRGVAR(durations[durations_i2]) < ISRGVAR(durations[(durations_i2 + 1) & 0x1F]);
          // Check that the sum of the two durations is the same of the first Ts
          // detected (within some tolerance) to detect if it is a valid transmission
          // or just noise
          ok &= abs((cur_ts = (ISRGVAR(durations[durations_i2]) + ISRGVAR(durations[(durations_i2 + 1) & 0x1F]))) - ts) < tolerance;
          // Also check if the proportions check out. If you do the math, here
          // we are asking the short part to be less than 7/9 and greater than 
		  // 1/3 of the long part.
          // Ideally, the short part should be 1/2 of the long part.
          ok &= abs(ISRGVAR(durations[durations_i2]) - ISRGVAR(durations[(durations_i2 + 1) & 0x1F])) > (ts >> 3);
          ok &= abs(ISRGVAR(durations[durations_i2]) - ISRGVAR(durations[(durations_i2 + 1) & 0x1F])) < (ts >> 1);
          current_avg_ts += cur_ts;
          durations_i2 = (durations_i2 + 2) & 0x1F;
      }
      if(ok) {
        // Ignore a new code if the previous one has been transmitted more than MIN_FOUND_TIMES times
        // but hasn't been read by the client yet
        if(current_found_code != ISRGVAR(found_code)) {
          ISRGVAR(found_code) = current_found_code;
          ISRGVAR(avg_ts) = current_avg_ts / 12;
          ISRGVAR(found_code_times) = 1;
        }
        else {
          ISRGVAR(found_code_times)++;
          ISRGVAR(avg_ts) += current_avg_ts / 12;
          ISRGVAR(avg_ts) >>= 1; // Not really an average but close enough
        }
      }
    }

    ISRGVAR(durations_i) = (ISRGVAR(durations_i) + 1) & 0x1F;
    ISRGVAR(last_time) = current_time;
}

ISR_RECEIVE_DEFS_CPP
