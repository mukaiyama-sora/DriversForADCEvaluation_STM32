/*
 * spi_driver_base.cpp
 *
 *  Created on: Dec 16, 2025
 */
#include <spi_driver_base.hpp>

/*----- Variables -----*/
SPI_HandleTypeDef* SPIDriverBase::hspi_ = nullptr;
std::array<uint8_t, spi_constants::kMax> SPIDriverBase::rx_buffer_ = {};
std::array<uint8_t, spi_constants::kMax> SPIDriverBase::tx_buffer_ = {};
uint16_t SPIDriverBase::buffer_size_ = 0;
size_t SPIDriverBase::callback_pin_index_ = 0;

SPIDriverBase::AtomicInterruptStatusTypeDef SPIDriverBase::rx_{HAL_OK, true};
SPIDriverBase::AtomicInterruptStatusTypeDef SPIDriverBase::txrx_{HAL_OK, true};
SPIDriverBase* SPIDriverBase::active_ = nullptr;

/*----- Private Functions -----*/
HAL_StatusTypeDef SPIDriverBase::Assert(size_t i)
{
	if(i >= cs_pin_.size()) return HAL_ERROR;
	cs_pin_[i].Low();

	return HAL_OK;
}
HAL_StatusTypeDef SPIDriverBase::Deassert(size_t i)
{
	if(i >= cs_pin_.size()) return HAL_ERROR;
	cs_pin_[i].High();
	return HAL_OK;
}


/*----- Initializer -----*/
void SPIDriverBase::Init(SPI_HandleTypeDef* hspi, std::vector<GPIOWrapper> cs_pin)
{
	if(is_initialized_) return;
	active_ = this;
	hspi_ = hspi;
	this->cs_pin_ = std::move(cs_pin);
	for(auto& pin: this->cs_pin_) {
		pin.High();
	}
	is_initialized_ = true;
}
void SPIDriverBase::InitBuffer() {
	if(hspi_ == nullptr) return;
	if(HAL_SPI_GetState(hspi_) != HAL_SPI_STATE_READY) return;
	tx_buffer_.fill(0);
	rx_buffer_.fill(0);
}


/*----- Setter -----*/
void SPIDriverBase::SetCallbackPinIndex(size_t i)
{
	if(hspi_ == nullptr) return;
	if(HAL_SPI_GetState(hspi_) != HAL_SPI_STATE_READY) return;
	if(i >= SPIDriverBase::active_->cs_pin_.size()) return;
	callback_pin_index_ = i;
}
HAL_StatusTypeDef SPIDriverBase::SetBufferSize(size_t n)
{
	if(hspi_ == nullptr) return HAL_ERROR;
	if(HAL_SPI_GetState(hspi_) != HAL_SPI_STATE_READY) return HAL_ERROR;
	if(n > spi_constants::kMax) return HAL_ERROR;

	buffer_size_ = n;

	return HAL_OK;
}
HAL_StatusTypeDef SPIDriverBase::SetTxBuffer(std::span<const uint8_t> buffer) {
	if(hspi_ == nullptr) return HAL_ERROR;
	if(HAL_SPI_GetState(hspi_) != HAL_SPI_STATE_READY) return HAL_ERROR;
	if(buffer.size() > spi_constants::kMax) return HAL_ERROR;

	std::copy(buffer.begin(), buffer.end(), tx_buffer_.data());
	buffer_size_ = buffer.size();

	return HAL_OK;
}


/*----- Getter -----*/
const HAL_SPI_StateTypeDef SPIDriverBase::GetSPIState()
{
	if(SPIDriverBase::hspi_ == nullptr) return HAL_SPI_STATE_ERROR;
	return HAL_SPI_GetState(SPIDriverBase::hspi_);
}
std::span<const uint8_t> SPIDriverBase::GetBuffer() { return std::span<const uint8_t>(rx_buffer_).first(buffer_size_); }
size_t SPIDriverBase::GetCallbackPinIndex() { return callback_pin_index_; }
SPIDriverBase::InterruptStatusTypeDef SPIDriverBase::GetReadITState() { return {rx_.state.load(), rx_.done.load()}; }
SPIDriverBase::InterruptStatusTypeDef SPIDriverBase::GetReadWriteITState() { return {txrx_.state.load(), txrx_.done.load()}; }


/*----- I/O -----*/
HAL_StatusTypeDef SPIDriverBase::Write(size_t pin_index)
{
	if(hspi_ == nullptr) return HAL_ERROR;
	if(buffer_size_ == 0) return HAL_ERROR;
	if(Assert(pin_index) != HAL_OK) return HAL_ERROR;

	const auto state = HAL_SPI_Transmit(hspi_, tx_buffer_.data(), buffer_size_, spi_constants::kTimeOut);
	Deassert(pin_index);
	return state;
}
HAL_StatusTypeDef SPIDriverBase::ReadWrite(size_t pin_index)
{
	if(hspi_ == nullptr) return HAL_ERROR;
	if(buffer_size_ == 0) return HAL_ERROR;
	if(Assert(pin_index) != HAL_OK) return HAL_ERROR;

	const auto state = HAL_SPI_TransmitReceive(hspi_, tx_buffer_.data(), rx_buffer_.data(), buffer_size_, spi_constants::kTimeOut);
	Deassert(pin_index);
	return state;
}
void SPIDriverBase::ReadIT()
{
	rx_.state.store(HAL_OK);
	rx_.done.store(false);
	if(hspi_ == nullptr)
	{
		rx_.state.store(HAL_ERROR);
		return;
	}
	if(buffer_size_ == 0)
	{
		rx_.state.store(HAL_ERROR);
		return;
	}
	if(Assert(callback_pin_index_) != HAL_OK)
	{
		rx_.state.store(HAL_ERROR);
		return;
	}

	rx_.state.store(HAL_SPI_Receive_IT(hspi_, rx_buffer_.data(), buffer_size_));

	if(rx_.state.load() != HAL_OK)
	{
		Deassert(callback_pin_index_);
	}
	return;
}
void SPIDriverBase::ReadWriteIT()
{
	txrx_.state.store(HAL_OK);
	txrx_.done.store(false);
	if(hspi_ == nullptr)
	{
		txrx_.state.store(HAL_ERROR);
		return;
	}
	if(buffer_size_ == 0)
	{
		txrx_.state.store(HAL_ERROR);
		return;
	}
	if(Assert(callback_pin_index_) != HAL_OK)
	{
		txrx_.state.store(HAL_ERROR);
		return;
	}

	txrx_.state.store(HAL_SPI_TransmitReceive_IT(hspi_, tx_buffer_.data(), rx_buffer_.data(), buffer_size_));

	if(txrx_.state.load() != HAL_OK)
	{
		Deassert(callback_pin_index_);
	}
	return;
}


/*----- Interrupt Callback -----*/
extern "C" void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef* hspi)
{
	if(SPIDriverBase::active_ == nullptr) return;
	if(SPIDriverBase::hspi_ != hspi) return;
	SPIDriverBase::active_->RxInterruptCallback(hspi);
	SPIDriverBase::active_->rx_.done.store(true);
}
extern "C" void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi)
{
	if(SPIDriverBase::active_ == nullptr) return;
	if(SPIDriverBase::hspi_ != hspi) return;
	SPIDriverBase::active_->TxRxInterruptCallback(hspi);
	SPIDriverBase::active_->txrx_.done.store(true);
}
