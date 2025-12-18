
/*
 * application.cpp
 *
 *  Created on: Dec 16, 2025
 */
extern "C" {
#include "main.h"
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart3;
}
#include <constants.hpp>
#include <application.hpp>
#include <sstream>
#include <string>
#include <array>
#include <algorithm>
#include <bitset>

Application app;

extern "C" void application_init()
{
	app.Init();
	UARTDriver::WriteLine("RESET");
}
extern "C" void application_run()
{
	app.Run();
}

void Application::Init()
{
	GPIOWrapper cs0(GPIOD, GPIO_PIN_14),  cs1(GPIOF, GPIO_PIN_12);

	spi_driver_.Init(&hspi1, {cs0, cs1});
	UARTDriver::Init(&huart3);
}

Application::Tokens Application::InputTokenizer(std::string s)
{
	std::stringstream	ss{s};
	std::string			tmp;
	Tokens				tokens;

	while(ss >> tmp)
	{
		tokens.push_back(tmp);
	}
	return tokens;
}
void Application::CommandDispatcher(const Tokens& tokens)
{
	if(tokens.empty()) return;

	Command command = tokens.front();
	Args args		= Args(tokens.begin() + 1, tokens.end());

	if(command == "WREG") {
		Wreg(args);
	}
	else if(command == "RREG") {
		Rreg(args);
	}
	else if(command == "RADC") {
		Radc(args);
	}
}
void Application::Run()
{
	CommandDispatcher(InputTokenizer(UARTDriver::ReadLine()));
}

void Application::Wreg(const Args& args)
{
	if(args.size() != 3) return;

	std::array<uint64_t, 3> reg;
	for(size_t i = 0; i < 3; ++i)
	{
		reg[i] = ((reg_constants::kWriteFlag | reg_constants::kRegAddr[i]) << 32) | static_cast<uint64_t>(std::stoll(args[i]));
	}

	std::array<std::array<uint8_t, 8>, 3> cast;
	for(size_t i = 0; i < 3; ++i)
	{
		cast.at(i) = std::bit_cast<std::array<uint8_t, 8>>(reg.at(i));
		std::reverse(cast.at(i).begin(), cast.at(i).end());
	}

	std::array<uint8_t, 5> tmp;
	for(size_t i = 0; i < 3; ++i)
	{
		std::copy_n(cast.at(i).begin() + 3, 5, tmp.begin());
		//spi_driver_.Write<5>(tmp, cs_constants::kReg);

		SPIDriver::SetTxBuffer(std::vector<uint8_t>(tmp.begin(), tmp.end()));
		spi_driver_.Write(cs_constants::kReg);
	}
	UARTDriver::WriteLine("WREG OK");
}

void Application::Rreg(const Args& args)
{
	if(args.size() != 1) return;

	std::array<uint8_t, 5> write;
	write.fill(0);

	write.at(0) = static_cast<uint8_t>(reg_constants::kReadFlag) | static_cast<uint8_t>(std::stol(args.front()));

	// spi_driver_.ReadWrite<5>(write, cs_constants::kReg);

	SPIDriver::SetTxBuffer(std::vector<uint8_t>(write.begin(), write.end()));
	spi_driver_.ReadWrite(cs_constants::kReg);

	uint64_t out = 0;
	uint64_t shift = 32;
	for(const auto& reg : SPIDriver::GetBuffer()) {
		out 	|= (static_cast<uint64_t>(reg) << shift);
		shift 	-= 8;
	}

	UARTDriver::WriteLine(std::bitset<40>(out).to_string());
}

void Application::Radc(const Args& args)
{
	if(args.size() != 1) return;

	SPIDriver::InitReadCount();
	SPIDriver::SetBufferSize(3);
	SPIDriver::SetCallbackPinIndex(cs_constants::kADC);

	record_length_ = static_cast<size_t>(std::stol(args.front()));
	if(record_length_ > app_constants::kMax) return;

	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	while(SPIDriver::GetReadCount() < record_length_);

	UARTDriver::WriteLine(std::to_string(SPIDriver::GetReadCount()));

	for(size_t i = 0; i < record_length_; ++i) {
		UARTDriver::WriteLine(std::bitset<32>(record_[i]).to_string());
	}
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if(SPIDriver::GetSPIState() != HAL_SPI_STATE_READY) return;
	if(const size_t count = SPIDriver::GetReadCount(); count == 0)
	{
		app.spi_driver_.ReadIT();
	}
	else if(count != 0 && (count - 1) < app.record_length_) {
		const auto& buffer = std::vector<uint8_t>(SPIDriver::GetBuffer().begin(), SPIDriver::GetBuffer().end());
		const auto data =
			(static_cast<uint32_t>(buffer.at(0)) << 16) |
			(static_cast<uint32_t>(buffer.at(1)) << 8)	| static_cast<uint32_t>(buffer.at(2));
		app.record_.at(count - 1) = data;
		app.spi_driver_.ReadIT();
	}
	else {
		HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
	}
}
