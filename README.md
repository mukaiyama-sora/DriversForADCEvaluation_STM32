# STM32 UART/SPI Driver
# ***UARTDriver***

必要なファイル: `uart_driver.hpp`, `uart_driver.cpp`

基本的なUART通信を行うためのC++ラッパ。
インスタンス作成は禁止されているので`UARTDriver::xxx`の形で使用する。
利用可能なのは以下の3つ。
- `static void Init(UART_HandleTypeDef*)`
- `static void WriteLine(const std::string&)`
- `static std::string ReadLine()`

## `static void Init(UART_HandleTypeDef*)`
UARTを指定したハンドルで初期化する。
これが実行されていない場合、`WriteLine`と`ReadLine`は何もしなくなる。

```cpp
UARTDriver::Init(&huart3);
```

## `static void WriteLine(const std::string&)`
引数で指定した文字列を終端文字(CR+LF)を付けて送信する。`std::string`に変換できるならOK。
次のような整数の二進数表記なども送ることができる。

```cpp
UARTDriver::WriteLine("Hello, World.");
UARTDriver::WriteLine(std::bitset<16>(123).to_string())  //0000000001111011
```

## `static std::string ReadLine()`
改行文字(LF)までを読み、`std::string`にして返す。

# ***SPIDriverBase***

必要なファイル: `gpio_wrapper.hpp`, `spi_driver_base.hpp`, `spi_driver_base.cpp`

- **単一インスタンスで運用する必要がある(2つ以上のSPIハンドラは持てない)。**
- **`SPIDriverBase`は抽象クラスなので、派生クラスで仮想関数(コールバック関数)をオーバーライドする必要がある。**
- **`std::span`を使用しているのでC++20以上である必要がある**
    

基本的なSPI通信を行うためのC++ラッパ。
通信で得た**データは内部のstaticな固定長バッファに格納される。**

割り込み通信で使用する**チップは内部のstaticな** *`callback_pin_index_`* **で指定される。**

非常に多くの関数があるが、役割ごとに分けると以下の6つになる。

- 初期化子
- Getter
- Setter
- I/O関連
- コールバック

以下各関数の説明である。

## 初期化子

### **`void Init(SPI_HandleTypeDef*, std::vector<GPIOWrapper>)`**

SPIドライバを引数で指定した**SPIのハンドラ**と**CSピンのベクタ**初期化する。
`GPIOWrapper`は第一引数にPort, 第二引数にNumberを指定することで初期化できる。
    
```cpp
SPIDriver spi_driver;
GPIOWrapper cs0(GPIOD, GPIO_PIN_14), cs1(GPIOF, GPIO_PIN_12);
spi_driver.Init(&hspi1, {cs0, cs1});
```

これが実行されていない場合、I/O関連の関数は`HAL_ERROR`を返す。
また、この関数は1度しか実行できない。

### **`void InitBuffer()`**

内部の固定長バッファの要素をすべて`0`で初期化する。

## Setter
これらの関数群は
**SPIハンドラが`nullptr`であるとき**、**SPIハンドラのステータスが`HAL_SPI_STATE_READY`でないとき**は何もしないかもしくは`HAL_ERROR`を返す。

### **`void SetCallbackPinIndex(size_t)`**

割り込みを行う関数(`ReadIT()`, `ReadWriteIT()`)内で使用するチップのインデックスを設定する。
指定されたインデックスが`Init(SPI_HandleTypeDef*, std::vector<GPIOWrapper>)`で指定したCSのリストのサイズより大きい場合は何もしない。

### **`HAL_StatusTypeDef SetBufferSize(size_t)`**

使用するバッファのサイズを指定する。
指定されたサイズが内部の固定長バッファより大きい場合は`HAL_ERROR`を返す。

### **`HAL_StatusTypeDef SetTxBuffer(std::span<const uint8_t>)`**
送信用のバッファに引数で指定したデータをセットする。
バッファのサイズは自動で引数で指定されたデータのサイズになる。
引数で指定されたデータのサイズが内部の固定長バッファより大きい場合は`HAL_ERROR`を返す。
引数の型は`std::span<const uint8_t>`であるが、これは`vector`や`array`から暗黙変換が可能なので次のように書ける。

```cpp
std::array<uint8_t, 5> arr{1, 2, 3, 4, 5};
SPIDriver::SetTxBuffer(arr);
SPIDriver::SetTxBuffer(std::vector<uint8_t>(arr.begin(), arr.end()));
```

## Getter
### **`HAL_SPI_StateTypeDef GetSPIState()`**

現在使用しているSPIハンドラのステータス(`HAL_SPI_STATE_READY`など)を取得する。
SPIハンドラがnullptrであるときは`HAL_SPI_STATE_ERROR`を返す。

### **`std::span<const uint8_t> GetBuffer()`**

内部バッファへの参照を現在指定されているサイズ分取得する。
戻り値の型は`std::span`である。`std::span`は`begin()`と`end()`が実装されているので`range-based for`を使用することができる。

```cpp
for(const auto b : SPIDriver::GetBuffer()){
	UARTDriver::WriteLine(std::to_string(static_cast<uint16_t>(b)));
}
```

一方で、`vector`や`array`に直接代入はできないので`std::copy`(`#include <algorithm>`が必要)を使う必要がある。
```cpp
std::vector<uint8_t> vec(256);
std::array<uint8_t, 256> arr;

std::copy(SPIDriver::GetBuffer.begin(), SPIDriver::GetBuffer.end(), vec.begin());
std::copy(SPIDriver::GetBuffer.begin(), SPIDriver::GetBuffer.end(), arr.begin());
```

### **`size_t GetCallbackPinIndex()`**

コールバック関数を実装する際に使用する。

現在設定されている割り込みを行う関数(`ReadIT()`, `ReadWriteIT()`)内で使用するチップのインデックスを取得する。

### **`InterruptStatusTypeDef GetReadITState()`, `InterruptStatusTypeDef GetReadWriteITState()`**

デバッグ用

`InterruptStatusTypeDef`は`HAL_StatusTypeDef`型の`state`と`bool`型の`done`をメンバに持つ構造体である。何らかの理由により`ReadIT()`, `ReadWriteIT()`が正常に終了しなかった場合`state`は`HAL_OK`以外の値を取る。
割り込み関数が正常に終了した場合は終了フラグ`done`が`true`になる。

### 

## I/O関連

### **`template <uint16_t N> HAL_StatusTypeDef Write(const std::array<uint8_t, N>&, size_t)`, `template <uint16_t N> HAL_StatusTypeDef ReadWrite(const std::array<uint8_t, N>&, size_t)`**
    
**第一引数で指定されたデータを第二引数で指定されたチップに送信する。**
`ReadWrite`の方は内部バッファにチップから来たデータを受信する。
    
```cpp
std::array<uint8_t, 5> write = {1, 2, 3, 4, 5};
spi_driver.Write<5>(write, 1);
spi_driver.ReadWrite<5>(write, 1);

std::array<uint8_t, 5> received;
std::copy(SPIDriver::GetBuffer().begin(), SPIDriver::GetBuffer().end(), received.begin());
```

### **`HAL_StatusTypeDef Write(size_t)`, `HAL_StatusTypeDef ReadWrite(size_t)`**

引数で指定したチップに`SetTxBuffer(std::span<const uint8_t>)`でセットした値を送信する。
`ReadWrite`の方は内部バッファにチップから来たデータを受信する。

```cpp
std::vector<uint8_t> write = {1, 2, 3, 4, 5};
SPIDriver::SetTxBuffer(write);
spi_driver.Write(1);
spi_driver.ReadWrite(1);

std::vector<uint8_t> received(5);
std::copy(SPIDriver::GetBuffer().begin(), SPIDriver::GetBuffer().end(), received.begin());
```

### **`void ReadIT()`, `void ReadWriteIT()`**
割り込みでデータを内部バッファに受信する。
通信は`SetCallbackPinIndex(size_t)`で指定されたチップと行う。

**なお、コールバック関数は純粋仮想関数であるため、`SPIDriverBase`を継承したクラスで`RxInterruptCallback`を実装する必要がある。**

```cpp
SPIDriver::SetBufferSize(3);
SPIDriver::SetCallbackPinIndex(0);

spi_driver.ReadIT();
const auto data = SPIDriver::GetBuffer();
```

## コールバック

### **`virtual void RxInterruptCallback(SPI_HandleTypeDef*)`, `virtual void TxRxInterruptCallback(SPI_HandleTypeDef*)`**

純粋仮想関数。以下のように別のクラスに継承して実装をする必要がある。
使わない関数は`void TxRxInterruptCallback(SPI_HandleTypeDef*) override {}`として空実装すればよい。

```cpp
#include <spi_driver_base.hpp>

class SPIDriver : public SPIDriverBase {
private:
	void RxInterruptCallback(SPI_HandleTypeDef*) override;
	void TxRxInterruptCallback(SPI_HandleTypeDef*) override {}
};
```
# 使用例

`SPIDriverBase`を継承して`SPIDriver`を作り、コールバック関数を実装する。
<details><summary><code>spi_driver.hpp</code></summary>
	
```cpp
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
```
</details>
<details><summary><code>spi_driver.cpp</code></summary>

```cpp
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
```

</details>

アプリケーション用のクラスに変数として実装する。

<details><summary><code>application.hpp</code>(一部)</summary>

```cpp

// ...
// ...

class Application {
private:
	// ...
	// ...

	// SPI Driver
	SPIDriver spi_driver_;

	// ...
	// ...

	// Command Declaration
	void Wreg(const Args&);
	void Rreg(const Args&);
	void Radc(const Args&);

	friend void HAL_GPIO_EXTI_Callback(uint16_t);

public:
	void Init();

	// ...
	// ...
};

// ...
// ...

```
</details>

<details><summary><code>application.cpp</code>(一部)</summary>

```cpp

// ...
// ...

Application app;

void Application::Init()
{
	GPIOWrapper cs0(GPIOD, GPIO_PIN_14),  cs1(GPIOF, GPIO_PIN_12);

	spi_driver_.Init(&hspi1, {cs0, cs1});
	UARTDriver::Init(&huart3);

	buffer_.fill(0);
}

void Application::Wreg(const Args& args)
{
	// ...
 	// ...

	std::array<std::array<uint8_t, 8>, 3> cast;

	// ...
	// ...

	std::array<uint8_t, 5> tmp;
	for(size_t i = 0; i < 3; ++i)
	{
		std::copy_n(cast.at(i).begin() + 3, 5, tmp.begin());
		spi_driver_.Write<5>(tmp, cs_constants::kReg);
	}
	UARTDriver::WriteLine("WREG OK");
}

void Application::Rreg(const Args& args)
{
	// ...
	// ...

	std::array<uint8_t, 5> write;

	// ...
	// ...

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

	std::array<uint8_t, 3> write;
	write.fill(0);
	SPIDriver::SetTxBuffer(write);

	record_length_ = static_cast<size_t>(std::stol(args.front()));
	if(record_length_ > app_constants::kMax) return;

	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	while(SPIDriver::GetReadCount() < record_length_);

	for(size_t i = 0; i < record_length_; ++i) {
		UARTDriver::WriteLine(std::bitset<32>(record_[i]).to_string());
	}
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if(SPIDriver::GetSPIState() != HAL_SPI_STATE_READY) return;
	if(const size_t count = SPIDriver::GetReadCount(); count == 0)
	{
		app.spi_driver_.ReadWriteIT();
	}
	else if(count != 0 && (count - 1) < app.record_length_) {
		std::copy(SPIDriver::GetBuffer().begin(), SPIDriver::GetBuffer().end(), app.buffer_.begin());
		const auto data =
			(static_cast<uint32_t>(app.buffer_[0]) << 16) |
			(static_cast<uint32_t>(app.buffer_[1]) << 8)	| static_cast<uint32_t>(app.buffer_[2]);
		app.record_.at(count - 1) = data;
		app.spi_driver_.ReadWriteIT();
	}
	else {
		HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
	}
}

```
</details>
