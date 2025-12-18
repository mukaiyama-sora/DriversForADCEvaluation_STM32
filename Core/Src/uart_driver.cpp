/*
 * uart_driver.cpp
 *
 *  Created on: Dec 16, 2025
 */
#include <uart_driver.hpp>

/*----- Variables -----*/
UART_HandleTypeDef* UARTDriver::huart_ = nullptr;
std::atomic<bool> UARTDriver::is_char_received_{false};
uint8_t UARTDriver::buffer_ = 0;


/*----- Private Functions -----*/
void UARTDriver::ReadChar() { HAL_UART_Receive_IT(huart_, &buffer_, 1); }


/*----- Initializer -----*/
void UARTDriver::Init(UART_HandleTypeDef* huart) { huart_ = huart; }


/*----- I/O -----*/
std::string UARTDriver::ReadLine()
{
	if(huart_ == nullptr) return std::string();
	std::string res;
	while(true) {
		is_char_received_.store(false);
		ReadChar();
		while(!is_char_received_.load());
		res.push_back(buffer_);

		if(buffer_ == uart_constants::kLF) {
			break;
		}
	}

	return res;
}
void UARTDriver::WriteLine(const std::string& out)
{
	if(huart_ == nullptr) return;
	HAL_UART_Transmit(huart_, reinterpret_cast<const uint8_t*>(out.c_str()), out.size(), uart_constants::kTimeOut);
	HAL_UART_Transmit(huart_, &uart_constants::kCR, 1, uart_constants::kTimeOut);
	HAL_UART_Transmit(huart_, &uart_constants::kLF, 1, uart_constants::kTimeOut);
}


/*----- Interrupt Callback -----*/
extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) { UARTDriver::is_char_received_.store(true); }
