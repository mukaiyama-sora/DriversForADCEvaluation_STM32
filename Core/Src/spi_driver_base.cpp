/*
 * spi_driver_base.cpp
 *
 *  Created on: Dec 16, 2025
 */
#include <spi_driver_base.hpp>

/*----- Variables -----*/
SPI_HandleTypeDef* SPIDriverBase::hspi_ = nullptr;
std::vector<uint8_t> SPIDriverBase::rx_buffer_ = std::vector<uint8_t>();
std::vector<uint8_t> SPIDriverBase::tx_buffer_ = std::vector<uint8_t>();
size_t SPIDriverBase::callback_pin_index_ = 0;

HAL_StatusTypeDef SPIDriverBase::rxit_state_ = HAL_OK;
HAL_StatusTypeDef SPIDriverBase::txrxit_state_ = HAL_OK;
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
	active_ = this;
	hspi_ = hspi;
	this->cs_pin_ = std::move(cs_pin);
	for(auto& pin: this->cs_pin_) {
		pin.High();
	}
}
void SPIDriverBase::InitBuffer() {
	if(hspi_ == nullptr) return;
	if(HAL_SPI_GetState(hspi_) != HAL_SPI_STATE_READY) return;
	tx_buffer_.clear();
	rx_buffer_.clear();
}


/*----- Setter -----*/
void SPIDriverBase::SetCallbackPinIndex(size_t i) { callback_pin_index_ = i; }
HAL_StatusTypeDef SPIDriverBase::SetBufferSize(size_t n)
{
	if(hspi_ == nullptr) return HAL_ERROR;
	if(HAL_SPI_GetState(hspi_) != HAL_SPI_STATE_READY) return HAL_ERROR;
	tx_buffer_.resize(n);
	rx_buffer_.resize(n);

	return HAL_OK;
}
HAL_StatusTypeDef SPIDriverBase::SetTxBuffer(std::vector<uint8_t> buffer) {
	if(hspi_ == nullptr) return HAL_ERROR;
	if(HAL_SPI_GetState(hspi_) != HAL_SPI_STATE_READY) return HAL_ERROR;
	tx_buffer_ = std::move(buffer);
	rx_buffer_.resize(tx_buffer_.size());

	return HAL_OK;
}


/*----- Getter -----*/
const std::vector<uint8_t>& SPIDriverBase::GetBuffer() { return rx_buffer_; }
size_t SPIDriverBase::GetCallbackPinIndex() { return callback_pin_index_; }
HAL_StatusTypeDef SPIDriverBase::GetReadITState() { return rxit_state_; }
HAL_StatusTypeDef SPIDriverBase::GetReadWriteITState() { return txrxit_state_; }


/*----- I/O -----*/
HAL_StatusTypeDef SPIDriverBase::Write(size_t pin_index)
{
	if(hspi_ == nullptr) return HAL_ERROR;
	if(Assert(pin_index) != HAL_OK) return HAL_ERROR;

	const auto state = HAL_SPI_Transmit(hspi_, tx_buffer_.data(), static_cast<uint16_t>(tx_buffer_.size()), spi_constants::kTimeOut);
	Deassert(pin_index);
	return state;
}
HAL_StatusTypeDef SPIDriverBase::ReadWrite(size_t pin_index)
{
	if(hspi_ == nullptr) return HAL_ERROR;
	if(Assert(pin_index) != HAL_OK) return HAL_ERROR;

	const auto state = HAL_SPI_TransmitReceive(hspi_, tx_buffer_.data(), rx_buffer_.data(), static_cast<uint16_t>(tx_buffer_.size()), spi_constants::kTimeOut);
	Deassert(pin_index);
	return state;
}
void SPIDriverBase::ReadIT()
{
	if(hspi_ == nullptr)
	{
		rxit_state_ = HAL_ERROR;
		return;
	}
	if(Assert(callback_pin_index_) != HAL_OK)
	{
		rxit_state_ = HAL_ERROR;
		return;
	}

	rxit_state_ = HAL_SPI_Receive_IT(hspi_, rx_buffer_.data(), static_cast<uint16_t>(rx_buffer_.size()));

	if(rxit_state_ != HAL_OK)
	{
		Deassert(callback_pin_index_);
	}
	return;
}
void SPIDriverBase::ReadWriteIT()
{
	if(hspi_ == nullptr)
	{
		txrxit_state_ = HAL_ERROR;
		return;
	}
	if(Assert(callback_pin_index_) != HAL_OK)
	{
		txrxit_state_ = HAL_ERROR;
		return;
	}

	txrxit_state_ = HAL_SPI_TransmitReceive_IT(hspi_, tx_buffer_.data(), rx_buffer_.data(), static_cast<uint16_t>(tx_buffer_.size()));

	if(txrxit_state_ != HAL_OK)
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
}
extern "C" void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef* hspi)
{
	if(SPIDriverBase::active_ == nullptr) return;
	if(SPIDriverBase::hspi_ != hspi) return;
	SPIDriverBase::active_->TxRxInterruptCallback(hspi);
}
