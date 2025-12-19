/*
 * spi_driver.cpp
 *
 *  Created on: Dec 16, 2025
 */

#include <spi_driver.hpp>

std::atomic<size_t> SPIDriver::read_count_{0};

void SPIDriver::InitReadCount() { read_count_.store(0); }
const size_t SPIDriver::GetReadCount() { return read_count_.load(); }

void SPIDriver::RxInterruptCallback(SPI_HandleTypeDef* hspi)
{
	++read_count_;
	Deassert(GetCallbackPinIndex());
}
void SPIDriver::TxRxInterruptCallback(SPI_HandleTypeDef* hspi)
{
	++read_count_;
	Deassert(GetCallbackPinIndex());
}
