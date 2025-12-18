# STM32 UART/SPI Driver
# ***UARTDriver***

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
**`SPIDriverBase`は抽象クラスなので、派生クラスで仮想関数(コールバック関数)をオーバーライドする必要がある。**
    

基本的なSPI通信を行うためのC++ラッパ。
通信で得た**データは内部のstaticなバッファに格納される。**
割り込み通信で使用する**チップは内部のstaticな** *`callback_pin_index_`* **で指定される。**

非常に多くの関数があるが、役割ごとに分けると以下の6つになる。

- 初期化子
- Getter
- Setter
- I/O関連
- コールバック

以下各関数の説明であるが、簡単なものは省いている。

## 初期化子

### **`void Init(SPI_HandleTypeDef*, std::vector<GPIOWrapper>)`**

SPIドライバを引数で指定した**SPIのハンドラ**と**CSピンのベクタ**初期化する。
`GPIOWrapper`は第一引数にPort, 第二引数にNumberを指定することで初期化できる。
    
```cpp:
SPIDriver spi_driver;
GPIOWrapper cs0(GPIOD, GPIO_PIN_14), cs1(GPIOF, GPIO_PIN_12);
spi_driver.Init(&hspi1, {cs0, cs1});
```

これが実行されていない場合、I/O関連の関数は`HAL_ERROR`を返す。


## Setter

### **`HAL_StatusTypeDef SetTxBuffer(std::vector<uint8_t>)`**
送信用のバッファに引数で指定したデータをセットする。
この関数が使用された場合、内部で受信用のバッファのサイズが送信用のバッファのサイズと同じになるようにresizeされる。

## I/O関連

### `template <uint16_t N> HAL_StatusTypeDef Write(const std::array<uint8_t, N>&, size_t)`, `template <uint16_t N> HAL_StatusTypeDef ReadWrite(const std::array<uint8_t, N>&, size_t)`
    
**第一引数で指定されたデータを第二引数で指定されたチップに送信する。**
`ReadWrite`の方は内部バッファにチップから来たデータを受信する。
データ型は`std::array<int8_t, N>`である。
    
似た関数に`HAL_StatusTypeDef Write(size_t)`, `HAL_StatusTypeDef ReadWrite(size_t)`があるが、これは`SetTxBuffer(std::vector<uint8_t>)`でセットした値を送信する。
    
```cpp
std::array<uint8_t, 5> write1 = {1, 2, 3, 4, 5};
spi_driver.Write<5>(write1, 0);

std::vector<uint8_t> write2 = {1, 2, 3, 4, 5};
SPIDriver::SetTxBuffer(write2);
spi_driver.Write(0);
```

### **`void ReadIT()`, `void ReadWriteIT()`**
割り込みでデータを内部バッファに受信する。
通信は`SetCallbackPinIndex(size_t)`で指定されたチップと行う。

なお、コールバック関数は純粋仮想関数であるため、**`SPIDriverBase`を継承したクラスで`RxInterruptCallback`を実装する必要がある。**

```cpp
SPIDriver::SetBufferSize(3);
SPIDriver::SetCallbackPinIndex(0);

spi_driver.ReadIT();
const auto data = SPIDriver::GetBuffer();
```

## コールバック

### `virtual void RxInterruptCallback(SPI_HandleTypeDef*)`, `virtual void TxRxInterruptCallback(SPI_HandleTypeDef*)`

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

