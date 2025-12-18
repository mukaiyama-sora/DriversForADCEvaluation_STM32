/*
 * spi_driver_base.hpp
 *
 *  Created on: Dec 16, 2025
 */

#ifndef INC_SPI_DRIVER_BASE_HPP_
#define INC_SPI_DRIVER_BASE_HPP_

extern "C" {
#include "main.h"
}
#include <atomic>
#include <array>
#include <vector>
#include <gpio_wrapper.hpp>

namespace spi_constants {
	constexpr uint32_t kTimeOut = 1000;
}

class SPIDriverBase {
private:
	std::vector<GPIOWrapper> cs_pin_;

	static SPI_HandleTypeDef* hspi_;
	static std::vector<uint8_t> rx_buffer_;
	static std::vector<uint8_t> tx_buffer_;
	static size_t callback_pin_index_;

	static HAL_StatusTypeDef rxit_state_;
	static HAL_StatusTypeDef txrxit_state_;
	static SPIDriverBase* active_;

public:
	SPIDriverBase() = default;

	// Pin assertion
	HAL_StatusTypeDef Assert(size_t);
	HAL_StatusTypeDef Deassert(size_t);

	// Initializer
	void Init(SPI_HandleTypeDef*, std::vector<GPIOWrapper>);
	static void InitBuffer();

	// Getter
	static const std::vector<uint8_t>& GetBuffer();
	static size_t GetCallbackPinIndex();
	static HAL_StatusTypeDef GetReadITState();
	static HAL_StatusTypeDef GetReadWriteITState();

	// Setter
	static void SetCallbackPinIndex(size_t);
	static HAL_StatusTypeDef SetBufferSize(size_t);
	static HAL_StatusTypeDef SetTxBuffer(std::vector<uint8_t>);

	// I/O
	template <uint16_t N> 	HAL_StatusTypeDef 	Write(const std::array<uint8_t, N>&, size_t);
							HAL_StatusTypeDef 	Write(size_t);
	template <uint16_t N> 	HAL_StatusTypeDef 	ReadWrite(const std::array<uint8_t, N>&, size_t);
							HAL_StatusTypeDef 	ReadWrite(size_t);
							void 				ReadIT();
							void				ReadWriteIT();

private:
	// Interrupt Callback
	friend void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef*);
	friend void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef*);
	virtual void RxInterruptCallback(SPI_HandleTypeDef*) = 0;
	virtual void TxRxInterruptCallback(SPI_HandleTypeDef*) = 0;
};

template <uint16_t N> HAL_StatusTypeDef SPIDriverBase::Write(const std::array<uint8_t, N>& write, size_t pin_index)
{
	if(hspi_ == nullptr) return HAL_ERROR;
	if(Assert(pin_index) != HAL_OK) return HAL_ERROR;

	const auto state = HAL_SPI_Transmit(hspi_, write.data(), static_cast<uint16_t>(write.size()), spi_constants::kTimeOut);
	Deassert(pin_index);
	return state;
}

template <uint16_t N> HAL_StatusTypeDef SPIDriverBase::ReadWrite(const std::array<uint8_t, N>& write, size_t pin_index)
{
	if(hspi_ == nullptr) return HAL_ERROR;
	if(SetBufferSize(write.size()) != HAL_OK) return HAL_ERROR;
	if(Assert(pin_index) != HAL_OK) return HAL_ERROR;

	const auto state = HAL_SPI_TransmitReceive(hspi_, write.data(), rx_buffer_.data(), static_cast<uint16_t>(write.size()), spi_constants::kTimeOut);
	Deassert(pin_index);
	return state;
}

#endif /* INC_SPI_DRIVER_BASE_HPP_ */
