# UM3750 Library
An arduino library to emulate the UM3750 encoder and transmit fixed 12 bit codes
over the 315/433MHz bandwidth.
Currently, using this library you cannot demodulate and decode the code,
so this has to be achieved by other means, such as a USB RTL-SDR (which I used).

This library currently support the ESP8266 only.

## How to use
The functionalities offered by this library can be accessed using the object
`UM3750` declared in the global scope.
Before transmitting your code, you have to specify the output pin and the symbol
duration time in microseconds.
	
    bool code[] = {1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1};
    int outputPin = D7;
    float symbolDurationTime = 612.4;
    
    UM3750.enableTransmit(outputPin, symbolDurationTime);
    UM3750.playCode(code);
    
Please note that the symbol duration time should be a multiple of 0.2
microseconds.
Also, you cannot use the PWM functionality after calling `enableTransmit`.
To restore it, you have to use `UM3750.disableTransmit();`.

You can choose how many times to play the same code by passing the value as a
second argument to the `playCode` function, such as `UM3750.playCode(code, 48);`.
By default it's 48 times, which should be enough for most appliances and should
not trigger them more than once.

## Output Waveform
<img align="right" width="50%" src="https://raw.githubusercontent.com/francibm97/UM3750/master/extras/symbols.png">
The UM3750 IC and clones output digital symbols as displayed in the figure on
the side.

Each symbol can be decomposed into three parts of same duration:

 - The first part is always low;
 - The second part is either low or high depending on the bit transmitted;
 - The third part is always high.
The symbol duration time Ts can be adjusted on the real IC by changing the value
of the RC circuit connected to the input.

The UM3750 IC outputs the same waveform periodically.
After some silence and before transmitting the 12 symbols of data, there is a 
sync symbol, which is equal to a zero symbol.
So in total a single output waveform period consists of:
 - Silence, whose duration is equivalent to 11 symbols;
 - 1 sync symbol;
 - 12 data symbols.

<img width="100%" src="https://raw.githubusercontent.com/francibm97/UM3750/master/extras/timings.png">

Most appliances, which make use of this IC, trasmit this output waveform via RF
using amplitude modulation on either 315MHz or 433MHz.
