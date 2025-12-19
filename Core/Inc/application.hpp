/*
 * application.hpp
 *
 *  Created on: Dec 16, 2025
 */

#ifndef INC_APPLICATION_HPP_
#define INC_APPLICATION_HPP_

#include <spi_driver.hpp>
#include <uart_driver.hpp>
#include <string>
#include <array>

namespace app_constants {
	constexpr size_t kMax = 4096;
}

class Application {
private:
	// TypeDef
	using Tokens	= std::vector<std::string>;
	using Command	= std::string;
	using Args		= std::vector<std::string>;

	// SPI Driver
	SPIDriver spi_driver_;

	// ADC record
	std::array<uint8_t, 3> buffer_;
	std::array<uint32_t, app_constants::kMax> record_;
	size_t record_length_;

	// Command Analysis
	Tokens InputTokenizer(std::string);
	void CommandDispatcher(const Tokens&);

	// Command Declaration
	void Wreg(const Args&);
	void Rreg(const Args&);
	void Radc(const Args&);

	friend void HAL_GPIO_EXTI_Callback(uint16_t);

public:
	Application() = default;

	// Initializer
	void Init();
	void Run();
};

#endif /* INC_APPLICATION_HPP_ */
