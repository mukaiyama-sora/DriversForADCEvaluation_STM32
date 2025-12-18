/*
 * uart_driver.hpp
 *
 *  Created on: Dec 16, 2025
 */

#ifndef INC_UART_DRIVER_HPP_
#define INC_UART_DRIVER_HPP_

extern "C" {
#include "main.h"
}
#include <atomic>
#include <string>

namespace uart_constants {

	constexpr uint32_t kTimeOut = 1000;
	constexpr uint8_t kCR = '\r';
	constexpr uint8_t kLF = '\n';

}

class UARTDriver {
private:
	static UART_HandleTypeDef* huart_;
	static std::atomic<bool> is_char_received_;
	static uint8_t buffer_;

	static void ReadChar();

public:
	UARTDriver() = delete;

	// Initializer
	static void Init(UART_HandleTypeDef*);

	// I/O
	static std::string ReadLine();
	static void WriteLine(const std::string&);

	// Interrupt Callback
	friend void HAL_UART_RxCpltCallback(UART_HandleTypeDef*);
};


#endif /* INC_UART_DRIVER_HPP_ */
