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
#include <vector>

class Application {
private:
	// TypeDef
	using Tokens	= std::vector<std::string>;
	using Command	= std::string;
	using Args		= std::vector<std::string>;

	// SPI Driver
	SPIDriver spi_driver_;

	// ADC record
	std::vector<uint32_t> record_;

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
