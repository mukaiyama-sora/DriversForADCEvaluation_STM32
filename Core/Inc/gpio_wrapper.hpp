/*
 * gpio_wrapper.hpp
 *
 *  Created on: Dec 4, 2025
 */

#ifndef INC_GPIO_WRAPPER_HPP_
#define INC_GPIO_WRAPPER_HPP_

extern "C" {
#include "main.h"
}

struct GPIOWrapper {
	GPIO_TypeDef* 	port;
	uint16_t 		number;

	GPIOWrapper() : port(nullptr), number(0){}
	GPIOWrapper(GPIO_TypeDef* port, uint16_t number) : port(port), number(number){}

	void High()
	{
		if(port == nullptr) return;
		HAL_GPIO_WritePin(port, number, GPIO_PIN_SET);
	}
	void Low()
	{
		if(port == nullptr) return;
		HAL_GPIO_WritePin(port, number, GPIO_PIN_RESET);
	}
};


#endif /* INC_GPIO_WRAPPER_HPP_ */
