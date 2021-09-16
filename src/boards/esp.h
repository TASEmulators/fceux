#ifndef _ESP_H_
#define _ESP_H_

#include "../types.h"

//////////////////////////////////////
// Abstract ESP firmware interface

class EspFirmware {
public:
	virtual ~EspFirmware() = default;

	// Communication pins (dont care about UART details, directly transmit bytes)
	virtual void rx(uint8 v) = 0;
	virtual uint8 tx() = 0;

	// General purpose I/O pins
	virtual void setGpio4(bool v) = 0;
	virtual bool getGpio4() = 0;
};

#endif
