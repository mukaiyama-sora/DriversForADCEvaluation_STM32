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
#include <span>
#include <algorithm>
#include <gpio_wrapper.hpp>

namespace spi_constants {
	constexpr size_t kMax = 256;
	constexpr uint32_t kTimeOut = 1000;
}

class SPIDriverBase {
private:
	bool is_initialized_{false};
	std::vector<GPIOWrapper> cs_pin_;

	static SPI_HandleTypeDef* hspi_;

	static std::array<uint8_t, spi_constants::kMax> rx_buffer_;
	static std::array<uint8_t, spi_constants::kMax> tx_buffer_;
	static uint16_t buffer_size_;
	static size_t callback_pin_index_;

	struct AtomicInterruptStatusTypeDef {
			std::atomic<HAL_StatusTypeDef> state;
			std::atomic<bool> done;
	};
	static AtomicInterruptStatusTypeDef rx_;
	static AtomicInterruptStatusTypeDef txrx_;
	static SPIDriverBase* active_;

public:
	SPIDriverBase() = default;
	struct InterruptStatusTypeDef {
		HAL_StatusTypeDef state;
		bool done;
	};

	// Pin assertion
	HAL_StatusTypeDef Assert(size_t);
	HAL_StatusTypeDef Deassert(size_t);

	// Initializer
	void Init(SPI_HandleTypeDef*, std::vector<GPIOWrapper>);
	static void InitBuffer();

	// Getter
	static const HAL_SPI_StateTypeDef GetSPIState();
	static std::span<const uint8_t> GetBuffer();
	static size_t GetCallbackPinIndex();
	static InterruptStatusTypeDef GetReadITState();
	static InterruptStatusTypeDef GetReadWriteITState();

	// Setter
	static void SetCallbackPinIndex(size_t);
	static HAL_StatusTypeDef SetBufferSize(size_t);
	static HAL_StatusTypeDef SetTxBuffer(std::span<const uint8_t>);

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
	if(N == 0) return HAL_ERROR;
	if(Assert(pin_index) != HAL_OK) return HAL_ERROR;

	const auto state = HAL_SPI_Transmit(hspi_, write.data(), static_cast<uint16_t>(write.size()), spi_constants::kTimeOut);
	Deassert(pin_index);
	return state;
}

template <uint16_t N> HAL_StatusTypeDef SPIDriverBase::ReadWrite(const std::array<uint8_t, N>& write, size_t pin_index)
{
	if(hspi_ == nullptr) return HAL_ERROR;
	if(N == 0) return HAL_ERROR;
	if(SetBufferSize(write.size()) != HAL_OK) return HAL_ERROR;
	if(Assert(pin_index) != HAL_OK) return HAL_ERROR;

	const auto state = HAL_SPI_TransmitReceive(hspi_, write.data(), rx_buffer_.data(), static_cast<uint16_t>(write.size()), spi_constants::kTimeOut);
	Deassert(pin_index);
	return state;
}

#endif /* INC_SPI_DRIVER_BASE_HPP_ */
