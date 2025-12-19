/*
 * spi_driver.hpp
 *
 *  Created on: Dec 16, 2025
 */

#ifndef INC_SPI_DRIVER_HPP_
#define INC_SPI_DRIVER_HPP_

#include <atomic>
#include <spi_driver_base.hpp>

class SPIDriver : public SPIDriverBase {
private:
	static std::atomic<size_t> read_count_;

public:
	SPIDriver() = default;

	static void InitReadCount();
	static const size_t GetReadCount();

private:
	void RxInterruptCallback(SPI_HandleTypeDef*) override;
	void TxRxInterruptCallback(SPI_HandleTypeDef*) override;
};


#endif /* INC_SPI_DRIVER_HPP_ */
