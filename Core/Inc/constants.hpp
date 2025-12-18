/*
 * constants.hpp
 *
 *  Created on: Dec 16, 2025
 */

#ifndef INC_CONSTANTS_HPP_
#define INC_CONSTANTS_HPP_

#include <array>

namespace reg_constants {

	constexpr uint64_t kReadFlag 	= 0b1000'0000;
	constexpr uint64_t kWriteFlag	= 0b0000'0000;
	constexpr std::array<uint64_t, 5> kRegAddr = {
		0b0000'0000,
		0b0000'0001,
		0b0000'0010,
		0b0000'0011,
		0b0000'0100
	};

}
namespace cs_constants {

	constexpr size_t kADC = 0;
	constexpr size_t kReg = 1;

}

#endif /* INC_CONSTANTS_HPP_ */
