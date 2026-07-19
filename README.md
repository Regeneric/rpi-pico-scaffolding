# c-iot-drivers

Small C++ library providing bus wrappers, non-volatile storage access, timing helpers, and environmental sensor drivers. The API favors explicit configuration, caller-owned contexts, fixed-width types, deterministic lifetime, and status codes instead of exceptions.

## Contents

| Component | Namespace | Public header | Status |
|---|---|---|---|
| Common types | global | `<hkk/defines.h>` | Ready |
| Logging bridge | global macros | `<hkk/logger/logger.h>` | Ready |
| Timing and utility functions | `hkk::utils` | `<hkk/utils/utils.hpp>` | Ready |
| I2C | `hkk::bus::i2c` | `<hkk/bus/i2c/i2c.hpp>` | Ready |
| SPI | `hkk::bus::spi` | `<hkk/bus/spi/spi.hpp>` | Experimental |
| UART | `hkk::bus::uart` | `<hkk/bus/uart/uart.hpp>` | Blocking I/O ready; timeout and DMA entry points experimental |
| Non-volatile memory | `hkk::storage::nvm` | `<hkk/storage/nvm.hpp>` | Ready |
| BME280 | `hkk::bme280` | `<hkk/drivers/bme280/bme280.hpp>` | Core flow ready; see notes |
| DHT20 | `hkk::dht20` | `<hkk/drivers/dht20/dht20.hpp>` | Ready |
| SGP30 | `hkk::sgp30` | `<hkk/drivers/sgp30/sgp30.hpp>` | Ready |
| PMS5003 | `hkk::pms5003` | `<hkk/drivers/pms5003/pms5003.hpp>` | Initial implementation |

## Quick start

The usual workflow is:

1. Obtain or bind a bus object.
2. Initialize the bus.
3. Create a sensor `Config`.
4. Construct the sensor with the bus and configuration.
5. Call `setup()` once.
6. Call `process()` whenever a new processed sample is required.
7. Read values through getters or a caller-owned `Context`.

```cpp
#include <hkk/bus/i2c/i2c.hpp>
#include <hkk/drivers/bme280/bme280.hpp>

auto &i2c = hkk::bus::i2c::I2C0;

int8 status = i2c.init();
if(status < hkk::bus::i2c::I2C_OK) {
    // Handle bus initialization failure.
}

hkk::bme280::Config config {
    .enable = true,
    .address = 0x76,
    .humidity_sampling = static_cast<uint8>(
        hkk::bme280::Command::OversamplingX1
    ),
    .iir_coefficient = static_cast<uint8>(
        (hkk::bme280::Command::StandbyTime1000 << 5) |
        (hkk::bme280::Command::FilterCoeff16 << 2)
    ),
    .temperature_pressure_mode = static_cast<uint8>(
        (hkk::bme280::Command::OversamplingX2 << 5) |
        (hkk::bme280::Command::OversamplingX16 << 2) |
        hkk::bme280::Command::NormalMode
    ),
    .name = "BME280_0",
    .location = "Living Room"
};

hkk::bme280::BME280 sensor(i2c, config);
hkk::bme280::Context sample;

status = sensor.setup(sample);
if(status < hkk::bme280::BME280_OK) {
    // Handle sensor initialization failure.
}

status = sensor.process(sample);
if(status == hkk::bme280::BME280_OK) {
    float64 temperature = sample.temperature;
    float64 humidity = sample.humidity;
    float64 pressure = sample.pressure;
}
```

## Build integration

Add the repository from the parent CMake project:

```cmake
add_subdirectory(lib/c-iot-drivers)
```

Link the sensor libraries required by the application:

```cmake
target_link_libraries(app
    PRIVATE
        hkk::drivers::bme280
        hkk::drivers::dht20
        hkk::drivers::sgp30
        hkk::drivers::pms5003
)
```

Also link the bus, storage, and timing components selected by the parent project. A driver target supplies the driver implementation and public include directory; it does not automatically select application resources.

## API conventions

### Return values

Most operations use the following convention:

| Value | Meaning |
|---|---|
| `0` | Success. |
| `< 0` | Error code from the relevant `Result` enumeration. |
| `> 0` | A count or value, when explicitly documented for that function. |

Every major module provides `rts(int8 status)` to translate a status code to a static string:

```cpp
int8 status = sensor.process();
if(status < hkk::dht20::DHT20_OK) {
    HERROR("DHT20 error: %s (%d)", hkk::dht20::rts(status), status);
}
```

### Internal and external contexts

Sensor operations commonly have two overloads:

```cpp
int8 process();
int8 process(Context &res);
```

The parameterless overload stores results in the sensor's internal context. Getters such as `temperature()` read that internal state. The overload accepting `Context &res` writes to the supplied context, allowing the application to own and inspect all raw and processed data.

Do not mix the two forms accidentally. Calling `process(external_context)` does not update values returned by parameterless getters unless the driver explicitly copies them.

### Lifetime rules

- A bus object passed to a sensor constructor must outlive the sensor.
- A `ConfigContext` passed to `bind()` must outlive the bound object.
- Objects referenced by opaque pointers in a configuration must remain valid while the related component is in use.
- Sensor constructors copy `Config`, but they do not copy the referenced bus or optional storage/timer objects.
- A buffer passed to an asynchronous operation must remain valid until that operation completes.

### Blocking calls

`read()` and `write()` are blocking unless their function description explicitly states otherwise. A blocking read requesting `len` bytes may wait until all requested bytes arrive. Use a completed timeout or asynchronous API when indefinite waiting is not acceptable.

## Logging

Header:

```cpp
#include <hkk/logger/logger.h>
```

The library uses printf-style macros:

```cpp
HFATAL(...)
HERROR(...)
HWARN(...)
HINFO(...)
HDEBUG(...)
HTRACE(...)
```

Define `HLOGGER_HEADER` to include an application logger:

```cmake
target_compile_definitions(app PRIVATE HLOGGER_HEADER="core/logger/logger.h")
```

If a macro is not supplied by the selected logger header, it becomes a no-op.

## Common types

Header:

```cpp
#include <hkk/defines.h>
```

### Fixed-width aliases

| Alias | Meaning |
|---|---|
| `uint8`, `uint16`, `uint32`, `uint64` | Unsigned fixed-width integers. |
| `int8`, `int16`, `int32`, `int64` | Signed fixed-width integers. |
| `float32`, `float64` | 32-bit and 64-bit floating-point types where supported. |
| `bool8`, `bool32` | Integer boolean storage types. |
| `cstring` | `const char *`. |

The header also defines volatile (`vuint*`, `vint*`), fast (`fuint*`, `fint*`), and least-width (`luint*`, `lint*`) aliases.

### Common macros

| Macro | Description |
|---|---|
| `FOREVER` | Boolean true, intended for permanent loops. |
| `ALWAYS` | Boolean true. |
| `OK` | Generic success value `0`. |
| `ERROR` | Generic error marker `1`. |
| `ARRAY_LEN(array)` | Number of elements in a compile-time array. |
| `STATIC_ASSERT` | `static_assert` in C++ and `_Static_assert` in C. |

## Time and utility API

Headers:

```cpp
#include <hkk/utils/time.hpp>
#include <hkk/utils/utils.hpp>
```

### Time constants

```cpp
SECOND // 1000 ms
MINUTE // 60 * SECOND
HOUR   // 60 * MINUTE
```

The constants are expressed in milliseconds:

```cpp
hkk::utils::sleep_ms(5 * SECOND);
```

### `msb()`

```cpp
constexpr uint8 msb(uint16 data);
```

Returns the most significant byte of `data`.

**Parameters**

- `data`: 16-bit input value.

**Returns**

The upper eight bits of `data`.

```cpp
uint8 high = hkk::utils::msb(0x1234); // 0x12
```

### `lsb()`

```cpp
constexpr uint8 lsb(uint16 data);
```

Returns the least significant byte of `data`.

**Parameters**

- `data`: 16-bit input value.

**Returns**

The lower eight bits of `data`.

```cpp
uint8 low = hkk::utils::lsb(0x1234); // 0x34
```

### `sleep_us()` and `sleep_ms()`

```cpp
void sleep_us(uint64 us);
void sleep_ms(uint64 ms);
```

Block for the requested interval.

**Parameters**

- `us`: delay in microseconds.
- `ms`: delay in milliseconds.

**Returns**

Nothing.

### Repeating timers

```cpp
using TimerCallback = bool8 (*)(void *);

struct TimerContext {
    TimerCallback callback;
    void *data;
    void *timer;
};

bool repeating_timer_us(int64 us, TimerContext *ctx);
bool repeating_timer_ms(int64 ms, TimerContext *ctx);
```

Starts a repeating callback.

**Parameters**

- `us` / `ms`: repetition interval in the indicated unit.
- `ctx`: timer configuration. `callback` is invoked with `data`; `timer` identifies caller-owned timer storage.

**Returns**

`true` if the timer was started; otherwise `false`.

The callback returns `true` to continue repeating and `false` to stop.

```cpp
bool8 tick(void *data) {
    auto *counter = static_cast<uint32 *>(data);
    ++(*counter);
    return true;
}

uint32 counter = 0;
hkk::utils::TimerContext timer {
    .callback = tick,
    .data = &counter,
    .timer = timer_storage
};

bool started = hkk::utils::repeating_timer_ms(SECOND, &timer);
```

`timer_storage` is an application-provided pointer compatible with the linked timer implementation.

### One-shot alarms

```cpp
using AlarmCallback = bool8 (*)(void *);

struct AlarmContext {
    AlarmCallback callback;
    void *data;
};

int32 alarm_us(uint32 us, AlarmContext *ctx, bool8 fire_if_past = true);
int32 alarm_ms(uint32 ms, AlarmContext *ctx, bool8 fire_if_past = true);
```

Schedules a one-shot callback.

**Parameters**

- `us` / `ms`: delay in the indicated unit.
- `ctx`: alarm callback and user data. Must remain valid until the callback runs or the alarm is cancelled externally.
- `fire_if_past`: when true, allows an already-expired deadline to run immediately.

**Returns**

A non-negative alarm identifier on success or a negative error value on failure.

```cpp
bool8 ready_callback(void *data) {
    *static_cast<bool8 *>(data) = true;
    return false;
}

bool8 ready = false;
hkk::utils::AlarmContext alarm {
    .callback = ready_callback,
    .data = &ready
};

int32 alarm_id = hkk::utils::alarm_ms(250, &alarm);
```

## I2C API

Header and namespace:

```cpp
#include <hkk/bus/i2c/i2c.hpp>

using namespace hkk::bus::i2c;
```

### Result codes

| Code | Meaning |
|---|---|
| `I2C_OK` | Success. |
| `I2C_ERROR_NULL_CONTEXT` | Missing configuration context. |
| `I2C_ERROR_NULL_INSTANCE` | Missing bus instance. |
| `I2C_ERROR_NULL_DATA` | Null data pointer. |
| `I2C_ERROR_ZERO_LENGTH` | Requested length is zero. |
| `I2C_ERROR_NO_ACK` | Address or data was not acknowledged. |
| `I2C_ERROR_PARTIAL_WRITE` | Only part of the requested data was written. |
| `I2C_ERROR_WRITE_FAILED` | Write failed. |
| `I2C_ERROR_PARTIAL_READ` | Only part of the requested data was read. |
| `I2C_ERROR_READ_FAILED` | Read failed. |
| `I2C_ERROR_NOT_SUPPORTED` | Operation is unavailable. |
| `I2C_ERROR_TIMEOUT` | Operation exceeded its timeout. |
| `I2C_ERROR_NULL_MUTEX` | Transaction lock is missing. |
| `I2C_ERROR_BUSY` | Bus is owned by another transaction. |
| `I2C_KEEP_ALIVE` | Transaction remains active; numerically equal to `I2C_OK`. |
| `I2C_ERROR_GENERIC` | Unclassified failure. |
| `I2C_FUNCTION_NOT_IMPLEMENTED` | Function pointer is not bound. |
| `I2C_ERROR_UNKNOWN` | Unknown status. |

### Predeclared objects

```cpp
extern I2C I2C0;
extern I2C I2C1;
// ... I2C2 through I2C7
```

Only use objects configured by the application build. An unconfigured object returns `I2C_FUNCTION_NOT_IMPLEMENTED` from operations requiring an unbound callback.

### `LockState`

```cpp
struct LockState {
    void *mutex = nullptr;
    void *owner = nullptr;
    bool8 active = false;
    bool8 nostop = false;
};
```

Stores transaction ownership.

| Field | Description |
|---|---|
| `mutex` | Opaque caller-owned lock object. |
| `owner` | Pointer identifying the current transaction owner. |
| `active` | True while a transaction owns the bus. |
| `nostop` | True while the current sequence requests that the transaction remain open. |

### `ConfigContext`

```cpp
struct ConfigContext {
    LockState *transaction;
    void *instance;
    uint32 baudrate = 100000;
    uint8 sda;
    uint8 scl;
    int8 index = -1;
    int8 status = I2C_OK;
};
```

| Field | Description |
|---|---|
| `transaction` | Transaction state owned by the application. |
| `instance` | Opaque bus instance. |
| `baudrate` | Requested bus frequency in hertz. Defaults to 100 kHz. |
| `sda`, `scl` | Data and clock pin identifiers. |
| `index` | Bus index after initialization. |
| `status` | Last recorded status. |

### `bind()`

```cpp
void bind(I2C &i2c, ConfigContext &cfg);
void bind(I2C &i2c, ConfigContext &cfg, BackendTable &callbacks);
```

Binds an `I2C` object to its caller-owned configuration and operation table.

**Parameters**

- `i2c`: object to configure.
- `cfg`: context that must remain valid for the lifetime of `i2c`.
- `callbacks`: explicit low-level operation table. Omit it to use the table selected by the build.

**Returns**

Nothing.

### `rts()`

```cpp
const char *rts(int8 status);
```

Returns a static string corresponding to an I2C status code.

### `I2C::init()` and `I2C::deinit()`

```cpp
int8 init();
int8 deinit();
```

Initializes or deinitializes the bound bus.

**Parameters**

None.

**Returns**

`I2C_OK` on success or a negative `Result` code.

```cpp
auto &i2c = hkk::bus::i2c::I2C0;
int8 status = i2c.init();
// ...
status = i2c.deinit();
```

### `I2C::baudrate()`

```cpp
int8 baudrate(uint32 value);
int32 baudrate();
```

Sets or gets the bus frequency.

**Parameters**

- `value`: requested frequency in hertz.

**Returns**

- Setter: `I2C_OK` or a negative `Result` code.
- Getter: current frequency in hertz or a negative `Result` code.

### `I2C::index()`

```cpp
int8 index(int8 value);
int32 index();
```

Sets or gets the bus index.

**Parameters**

- `value`: requested index.

**Returns**

- Setter: `I2C_OK`, `I2C_ERROR_NOT_SUPPORTED`, or another negative status.
- Getter: non-negative bus index or a negative status.

### `I2C::write()`

```cpp
int32 write(uint8 addr, const uint8 *src, size_t len, bool8 nostop = false);

template<size_t N>
int32 write(uint8 addr, const uint8 (&src)[N], bool8 nostop = false);

int32 write(uint8 addr, uint8 byte, bool8 nostop = false);
```

Writes one or more bytes to a device.

**Parameters**

- `addr`: 7-bit device address stored in a `uint8`.
- `src`: source buffer.
- `len`: number of bytes to write.
- `byte`: single byte to write.
- `nostop`: when true, requests that no STOP condition be generated after the operation.

**Returns**

A non-negative transfer result or a negative `Result` code. Check for `< I2C_OK` rather than assuming that success is always exactly zero.

```cpp
uint8 register_address = 0xD0;
int32 result = i2c.write(0x76, register_address, true);
```

### `I2C::read()`

```cpp
int32 read(uint8 addr, uint8 *dst, size_t len, bool8 nostop = false);

template<size_t N>
int32 read(uint8 addr, uint8 (&dst)[N], bool8 nostop = false);

int32 read(uint8 addr, uint8 &byte, bool8 nostop = false);
```

Reads one or more bytes from a device.

**Parameters**

- `addr`: device address.
- `dst`: destination buffer.
- `len`: requested number of bytes.
- `byte`: destination for a single byte.
- `nostop`: requests continuation after the read when true.

**Returns**

A non-negative transfer result or a negative `Result` code.

### `I2C::write_timeout()`

```cpp
int32 write_timeout(
    uint8 addr,
    const uint8 *src,
    size_t len,
    uint32 timeout_us,
    bool8 nostop = false
);
```

Writes bytes with a timeout expressed in microseconds. Array and single-byte overloads are also available.

**Parameters**

- `addr`: device address.
- `src`: source bytes.
- `len`: byte count.
- `timeout_us`: maximum operation time in microseconds.
- `nostop`: continuation request.

**Returns**

A non-negative transfer result or a negative `Result` code, including `I2C_ERROR_TIMEOUT`.

Pass `nostop` explicitly when using overloads: the current array overload defaults it to `true`, while pointer and single-byte overloads default it to `false`.

### `I2C::transaction()`

```cpp
TransactionGuard transaction(void *owner = nullptr);
```

Starts an owned transaction and returns a non-copyable RAII guard. The guard calls the commit function when it leaves scope if acquisition succeeded.

**Parameters**

- `owner`: stable pointer identifying the caller. Pass the same pointer to related operations and explicit `commit()` calls.

**Returns**

A `TransactionGuard`. Inspect its public `status` field before accessing the bus.

```cpp
auto transaction = i2c.transaction(&sensor);
if(transaction.status == hkk::bus::i2c::I2C_OK) {
    i2c.write(address, register_address, true);
    i2c.read(address, data, false);
}
```

### `I2C::commit()`

```cpp
int8 commit(void *owner = nullptr);
```

Explicitly requests transaction completion.

**Parameters**

- `owner`: transaction owner used during acquisition.

**Returns**

`I2C_OK` when the transaction is closed or kept alive, or a negative `Result` code.

## SPI API

Header and namespace:

```cpp
#include <hkk/bus/spi/spi.hpp>

using namespace hkk::bus::spi;
```

The SPI interface mirrors the high-level shape of `I2C`, including compatibility overloads used by drivers that can operate over either bus. SPI itself does not use device addresses or STOP conditions; address and `nostop` arguments in compatibility overloads are ignored.

### Result codes

| Code | Meaning |
|---|---|
| `SPI_OK` | Success. |
| `SPI_ERROR_NULL_CONTEXT` | Missing configuration context. |
| `SPI_ERROR_NULL_INSTANCE` | Missing bus instance. |
| `SPI_ERROR_NULL_DATA` | Null data pointer. |
| `SPI_ERROR_ZERO_LENGTH` | Requested length is zero. |
| `SPI_ERROR_NO_ACK` | Reserved compatibility error. |
| `SPI_ERROR_PARTIAL_WRITE` | Partial write. |
| `SPI_ERROR_WRITE_FAILED` | Write failed. |
| `SPI_ERROR_PARTIAL_READ` | Partial read. |
| `SPI_ERROR_READ_FAILED` | Read failed. |
| `SPI_ERROR_NOT_SUPPORTED` | Operation is unavailable. |
| `SPI_ERROR_TIMEOUT` | Operation exceeded its timeout. |
| `SPI_ERROR_NULL_MUTEX` | Transaction lock is missing. |
| `SPI_ERROR_BUSY` | Bus is owned by another transaction. |
| `SPI_ERROR_GENERIC` | Unclassified failure. |
| `SPI_FUNCTION_NOT_IMPLEMENTED` | Function pointer is not bound. |
| `SPI_ERROR_UNKNOWN` | Unknown status. |

### Predeclared objects

```cpp
extern SPI SPI0;
extern SPI SPI1;
// ... SPI2 through SPI7
```

Use only objects configured by the application build.

### Configuration types

```cpp
struct LockState {
    void *mutex = nullptr;
    void *owner = nullptr;
    bool8 active = false;
};

struct ConfigContext {
    LockState *transaction;
    void *instance;
    uint8 miso;
    uint8 mosi;
    uint8 sck;
    uint8 cs;
    uint8 reset;
    int8 index = -1;
    int8 status = SPI_OK;
};
```

| Field | Description |
|---|---|
| `transaction` | Caller-owned transaction state. |
| `instance` | Opaque bus instance. |
| `miso`, `mosi`, `sck` | Input, output, and clock pin identifiers. |
| `cs` | Chip-select pin identifier. |
| `reset` | Optional reset pin identifier. |
| `index` | Bus index after initialization. |
| `status` | Last recorded status. |

### `bind()` and `rts()`

```cpp
void bind(SPI &spi, ConfigContext &cfg);
void bind(SPI &spi, ConfigContext &cfg, BackendTable &callbacks);
const char *rts(int8 status);
```

`bind()` associates an SPI object with a caller-owned context and operation table. `rts()` converts a result code to a static string.

### Initialization and properties

```cpp
int8 init();
int8 deinit();

int8 baudrate(uint32 value);
int32 baudrate();

int8 index(int8 value);
int32 index();
```

| Function | Parameters | Returns |
|---|---|---|
| `init()` | None. | `SPI_OK` or a negative status. |
| `deinit()` | None. | `SPI_OK` or a negative status. |
| `baudrate(value)` | Frequency in hertz. | `SPI_OK` or a negative status. |
| `baudrate()` | None. | Current frequency or a negative status. |
| `index(value)` | Requested bus index. | `SPI_OK`, `SPI_ERROR_NOT_SUPPORTED`, or another error. |
| `index()` | None. | Non-negative bus index or a negative status. |

### `SPI::write()`

```cpp
int32 write(const uint8 *src, size_t len, bool8 nostop = false);
int32 write(uint8 byte, bool8 nostop = false);

template<size_t N>
int32 write(const uint8 (&src)[N], bool8 nostop = false);
```

Writes one or more bytes.

**Parameters**

- `src`: source buffer.
- `len`: number of bytes.
- `byte`: single byte.
- `nostop`: compatibility argument; ignored for SPI.

**Returns**

A non-negative transfer result or a negative `Result` code.

Compatibility overloads additionally accept an ignored `uint8 addr` as their first parameter.

```cpp
uint8 command[] = {0xF4, 0x27};
int32 result = spi.write(command);
```

### `SPI::read()`

```cpp
int32 read(uint8 *dst, size_t len, bool8 nostop = false);
int32 read(uint8 &byte, bool8 nostop = false);

template<size_t N>
int32 read(uint8 (&dst)[N], bool8 nostop = false);
```

Reads one or more bytes.

**Parameters**

- `dst`: destination buffer.
- `len`: number of requested bytes.
- `byte`: destination for a single byte.
- `nostop`: compatibility argument; ignored for SPI.

**Returns**

A non-negative transfer result or a negative `Result` code.

Compatibility overloads additionally accept an ignored `uint8 addr`.

### `SPI::write_timeout()`

```cpp
int32 write_timeout(
    const uint8 *src,
    size_t len,
    uint32 timeout_us,
    bool8 nostop = false
);
```

Writes bytes with a timeout. Array, single-byte, and address-compatibility overloads are available.

**Parameters**

- `src`: source bytes.
- `len`: byte count.
- `timeout_us`: maximum operation time in microseconds.
- `nostop`: compatibility argument; ignored.

**Returns**

A non-negative transfer result or a negative status, including `SPI_ERROR_TIMEOUT`.

### Transactions

```cpp
TransactionGuard transaction(void *owner = nullptr);
int8 commit(void *owner = nullptr);
```

`transaction()` returns a non-copyable RAII guard and `commit()` explicitly requests completion. The owner pointer identifies the caller and must remain stable for the transaction lifetime.

```cpp
auto transaction = spi.transaction(&device);
if(transaction.status == hkk::bus::spi::SPI_OK) {
    spi.write(command);
    spi.read(response);
}
```

### SPI readiness note

The SPI public surface is experimental. In this snapshot the no-address `write()`, `read()`, and `write_timeout()` base overloads call themselves recursively; use of those overloads is unsafe until their forwarding targets are corrected. Transaction callbacks are also incomplete. Do not use this API for production transfers yet.

## UART API

Header and namespace:

```cpp
#include <hkk/bus/uart/uart.hpp>

using namespace hkk::bus::uart;
```

UART provides blocking byte-stream reads and writes. It does not define packet boundaries, addresses, acknowledgement, or checksums; those belong to the protocol implemented above UART.

### Result codes

| Code | Meaning |
|---|---|
| `UART_OK` | Success. |
| `UART_ERROR_NULL_CONTEXT` | Missing configuration context. |
| `UART_ERROR_NULL_INSTANCE` | Missing UART instance. |
| `UART_ERROR_NULL_DATA` | Null data pointer. |
| `UART_ERROR_ZERO_LENGTH` | Requested length is zero. |
| `UART_ERROR_NO_ACK` | Reserved compatibility error. |
| `UART_ERROR_PARTIAL_WRITE` | Partial write. |
| `UART_ERROR_WRITE_FAILED` | Write failed. |
| `UART_ERROR_PARTIAL_READ` | Partial read. |
| `UART_ERROR_READ_FAILED` | Read failed. |
| `UART_ERROR_NOT_SUPPORTED` | Operation is unavailable. |
| `UART_ERROR_TIMEOUT` | Operation exceeded its timeout. |
| `UART_ERROR_NULL_MUTEX` | Reserved transaction error. |
| `UART_ERROR_BUSY` | Resource is busy. |
| `UART_DMA_ERROR_GENERIC` | Generic asynchronous-transfer failure. |
| `UART_ERROR_NULL_DMA_CONTEXT` | Missing asynchronous-transfer context. |
| `UART_ERROR_GENERIC` | Unclassified failure. |
| `UART_FUNCTION_NOT_IMPLEMENTED` | Function pointer is not bound. |
| `UART_ERROR_UNKNOWN` | Unknown status. |

### Predeclared objects

```cpp
extern UART UART0;
extern UART UART1;
// ... UART2 through UART7
```

Only use objects configured by the application build.

### `ConfigContext`

```cpp
struct ConfigContext {
    void *instance;
    void *dma;

    uint32 baudrate = 9600;
    uint8 data_bits = 8;
    uint8 parity_bits = 0;
    uint8 stop_bits = 1;

    uint8 tx;
    uint8 rx;

    bool8 rx_dma_enabled = false;
    bool8 tx_dma_enabled = false;

    int8 index = -1;
    int8 status = UART_OK;
};
```

| Field | Description |
|---|---|
| `instance` | Opaque UART instance. |
| `dma` | Optional opaque asynchronous-transfer context. |
| `baudrate` | Requested bits per second. Defaults to `9600`. |
| `data_bits` | Data bits per frame. Defaults to `8`. |
| `parity_bits` | Parity selection. Defaults to `0`. |
| `stop_bits` | Stop bits per frame. Defaults to `1`. |
| `tx`, `rx` | Transmit and receive pin identifiers. |
| `rx_dma_enabled`, `tx_dma_enabled` | Availability state for asynchronous transfers. |
| `index` | UART index after initialization. |
| `status` | Last recorded status. |

The default line format is 9600 baud, 8 data bits, no parity, and one stop bit (9600 8N1).

### `bind()` and `rts()`

```cpp
void bind(UART &uart, ConfigContext &cfg);
void bind(UART &uart, ConfigContext &cfg, BackendTable &callbacks);
const char *rts(int8 status);
```

`bind()` associates a UART object with a caller-owned context and operation table. `rts()` returns a static name for a status code.

### `UART::init()`

```cpp
int8 init(bool8 use_dma = false);
```

Initializes the UART using the bound configuration.

**Parameters**

- `use_dma`: requests initialization of experimental asynchronous-transfer support. Keep this `false` when using only blocking operations.

**Returns**

`UART_OK` on successful UART initialization or a negative `Result` code. Asynchronous initialization failure may be reported separately while leaving blocking UART available, depending on the bound operation table.

```cpp
auto &uart = hkk::bus::uart::UART0;
int8 status = uart.init();
```

### `UART::deinit()`

```cpp
int8 deinit();
```

Deinitializes the UART.

**Returns**

`UART_OK` on success or a negative `Result` code.

### `UART::baudrate()`

```cpp
int8 baudrate(uint32 value);
int32 baudrate();
```

Sets or gets the baud rate.

**Parameters**

- `value`: requested bits per second.

**Returns**

- Setter: `UART_OK` or a negative status.
- Getter: actual stored baud rate or a negative status.

### `UART::index()`

```cpp
int8 index(int8 value);
int32 index();
```

Sets or gets the UART index. The setter may return `UART_ERROR_NOT_SUPPORTED` when the index is determined by the bound instance.

### `UART::write()`

```cpp
int32 write(const uint8 *src, size_t len);

template<size_t N>
int32 write(const uint8 (&src)[N]);

int32 write(uint8 byte);
```

Blocks while writing one or more bytes.

**Parameters**

- `src`: source buffer.
- `len`: number of bytes.
- `byte`: single byte.

**Returns**

`UART_OK` or a negative `Result` code in the current interface.

```cpp
uint8 command[] = {0x42, 0x4D, 0xE2, 0x00, 0x00, 0x01, 0x71, 0x00};
int32 result = uart.write(command);
```

### `UART::read()`

```cpp
int32 read(uint8 *dst, size_t len);

template<size_t N>
int32 read(uint8 (&dst)[N]);

int32 read(uint8 &byte);
```

Blocks until the requested bytes have been read or an error is returned.

**Parameters**

- `dst`: destination buffer.
- `len`: number of requested bytes.
- `byte`: destination for a single byte.

**Returns**

`UART_OK` or a negative `Result` code in the current interface.

```cpp
uint8 frame[32];
int32 result = uart.read(frame);
```

The UART layer does not search for a frame header. Starting the read in the middle of a stream produces a correspondingly shifted buffer.

### `UART::write_timeout()`

```cpp
int32 write_timeout(const uint8 *src, size_t len, uint32 timeout_us);
```

Reserved timeout-capable write entry point. Array and single-byte overloads are available.

**Parameters**

- `src`: source bytes.
- `len`: byte count.
- `timeout_us`: requested timeout in microseconds.

**Returns**

A UART status.

This entry point is experimental. Do not assume that `timeout_us` is enforced until the selected operation table documents that guarantee.

### `UART::read_dma()`

```cpp
int32 read_dma(uint8 *dst, size_t len);
```

Reserved asynchronous-read entry point. Array and single-byte overloads are available.

**Parameters**

- `dst`: destination buffer that must remain alive until completion.
- `len`: requested byte count.

**Returns**

A UART status indicating whether the request was accepted.

This entry point is experimental and does not yet define a public completion, cancellation, or synchronization contract. Use `read()` for stable behavior.

### UART example

```cpp
#include <hkk/bus/uart/uart.hpp>

auto &uart = hkk::bus::uart::UART0;

int8 status = uart.init(false);
if(status < hkk::bus::uart::UART_OK) {
    HERROR("UART initialization failed: %s", hkk::bus::uart::rts(status));
}

uint8 frame[32];
int32 result = uart.read(frame);
if(result < hkk::bus::uart::UART_OK) {
    HERROR("UART read failed: %s", hkk::bus::uart::rts(result));
}
```

## NVM API

Header and namespace:

```cpp
#include <hkk/storage/nvm.hpp>

using namespace hkk::storage::nvm;
```

### Result codes

| Code | Meaning |
|---|---|
| `NVM_OK` | Success. |
| `NVM_ERROR_NULL_CONTEXT` | Missing configuration context. |
| `NVM_ERROR_NULL_MUTEX` | Transaction lock is missing. |
| `NVM_ERROR_BUSY` | Storage is owned by another transaction. |
| `NVM_ERROR_NULL_DATA` | Null data pointer. |
| `NVM_ERROR_ZERO_LENGTH` | Requested length is zero. |
| `NVM_ERROR_OOB` | Address or range is out of bounds. |
| `NVM_ERROR_GENERIC` | Unclassified failure. |
| `NVM_FUNCTION_NOT_IMPLEMENTED` | Function pointer is not bound. |
| `NVM_ERROR_UNKNOWN` | Unknown status. |
| `NVM_DATA_TRUNCATED` | Only part of the data was accepted. |
| `NVM_NULL_ADDRESS` | Sentinel requesting the configured implicit address. |

### Storage binding functions

```cpp
void hkk::storage::flash::bind(nvm::NVM &nvm, nvm::ConfigContext &cfg);
void hkk::storage::eeprom::bind(nvm::NVM &nvm, nvm::ConfigContext &cfg);
void hkk::storage::fram::bind(nvm::NVM &nvm, nvm::ConfigContext &cfg);
```

Bind an `NVM` object to the selected storage medium. Availability is determined by the application build.

### `LockState`

```cpp
struct LockState {
    void *mutex = nullptr;
    void *owner = nullptr;
    bool8 active = false;
};
```

Stores caller-owned synchronization and transaction ownership.

### `ConfigContext`

```cpp
struct ConfigContext {
    LockState *transaction;
    void *instance;

    const uint32 page_size = 0;
    uint32 sector_size = 0;
    uint32 pages_per_sector = 0;
    uint32 sectors_number = 0;
    uint32 storage_offset = 0;
    uint32 current_page = 0;

    int8 status = NVM_OK;
};
```

| Field | Description |
|---|---|
| `transaction` | Caller-owned transaction state. |
| `instance` | Optional opaque storage instance. |
| `page_size` | Number of bytes in one page. |
| `sector_size` | Number of bytes in one sector. |
| `pages_per_sector` | Derived page count per sector. |
| `sectors_number` | Number of sectors exposed through the object. |
| `storage_offset` | Base offset used by implicit-address operations. |
| `current_page` | Current page selection. |
| `status` | Last recorded status. |

### `bind()` and `rts()`

```cpp
void bind(NVM &nvm, ConfigContext &cfg);
void bind(NVM &nvm, ConfigContext &cfg, const BackendTable &callbacks);
const char *rts(int8 status);
```

Associates an NVM object with its caller-owned context and operation table. `rts()` returns the static name of a status code.

### `NVM::init()` and `NVM::deinit()`

```cpp
int8 init(bool8 clear_data = false);
int8 deinit();
```

Initializes or deinitializes storage.

**Parameters**

- `clear_data`: requests clearing the configured storage region during initialization.

**Returns**

`NVM_OK` or a negative `Result` code.

Do not enable `clear_data` unless erasing existing contents is intentional.

### `NVM::clear()`

```cpp
int8 clear(int32 offset, size_t sectors_number);
int8 clear();
```

Clears one or more sectors. The parameterless overload clears the currently configured range.

**Parameters**

- `offset`: first sector or medium-specific offset accepted by the bound operation.
- `sectors_number`: number of sectors to clear.

**Returns**

`NVM_OK` or a negative `Result` code.

### `NVM::offset()`

```cpp
int8 offset(int32 value);
uint32 offset();
```

Sets or gets the configured storage offset.

### `NVM::sectors()`

```cpp
int8 sectors(int32 value);
uint32 sectors();
```

Sets or gets the number of exposed sectors.

### `NVM::pages()`

```cpp
uint32 pages(bool8 current_page = false);
```

Returns page information.

**Parameters**

- `current_page`: when false, requests the total page count represented by the operation; when true, requests the current page value.

**Returns**

A page count/index. Because the return type is unsigned, compare carefully when an unavailable function may be represented by a converted negative status.

### `NVM::write()`

```cpp
int8 write(int32 addr, const uint8 *src, size_t len);
int8 write(const uint8 *src, size_t len);

template<size_t N>
int8 write(int32 addr, const uint8 (&src)[N]);

template<size_t N>
int8 write(const uint8 (&src)[N]);
```

Writes bytes to storage. Overloads without `addr` use `NVM_NULL_ADDRESS`, requesting the configured implicit location.

**Parameters**

- `addr`: explicit storage address or `NVM_NULL_ADDRESS`.
- `src`: source buffer.
- `len`: number of bytes.

**Returns**

`NVM_OK`, `NVM_DATA_TRUNCATED`, or a negative error code.

### `NVM::read()`

```cpp
int8 read(int32 addr, int32 page, uint8 *dst, size_t len);
int8 read(int32 addr, uint8 *dst, size_t len);
int8 read(uint8 *dst, size_t len);
```

Reads bytes from storage. Compile-time array overloads are available for all three forms.

**Parameters**

- `addr`: explicit address or `NVM_NULL_ADDRESS`.
- `page`: page offset, when applicable.
- `dst`: destination buffer.
- `len`: requested number of bytes.

**Returns**

`NVM_OK` or a negative `Result` code.

### NVM transactions

```cpp
TransactionGuard transaction(void *owner = nullptr);
int8 commit(void *owner = nullptr);
```

`transaction()` acquires owned access and returns a non-copyable RAII guard. `commit()` explicitly requests completion.

```cpp
uint8 data[] = {0x10, 0x20, 0x30};

auto transaction = storage.transaction(&storage_owner);
if(transaction.status == hkk::storage::nvm::NVM_OK) {
    int8 status = storage.write(0, data);
}
```

## BME280 API

Header:

```cpp
#include <hkk/drivers/bme280/bme280.hpp>
```

Namespace: `hkk::bme280`.

The driver reads temperature, pressure, and relative humidity. It also calculates absolute humidity, dew point, vapor pressure, saturation vapor pressure, and vapor pressure deficit.

### Data units

| `Context` field | Unit |
|---|---:|
| `temperature` | degC |
| `pressure` | hPa |
| `humidity` | %RH |
| `absolute_humidity` | g/m3 |
| `dew_point` | degC |
| `vapor_pressure` | Pa |
| `saturation_vapor_pressure` | Pa |
| `vapor_pressure_deficit` | kPa |

### Configuration values

`Command` contains the unshifted values used to compose the three configuration registers:

| Group | Values |
|---|---|
| Oversampling | `OversamplingSkip`, `OversamplingX1`, `OversamplingX2`, `OversamplingX4`, `OversamplingX8`, `OversamplingX16` |
| Operation mode | `SleepMode`, `ForceMode`, `NormalMode` |
| Standby time | `StandbyTime0_5`, `StandbyTime62_5`, `StandbyTime125`, `StandbyTime250`, `StandbyTime500`, `StandbyTime1000`, `StandbyTime10`, `StandbyTime20` |
| Filter | `FilterOff`, `FilterCoeff2`, `FilterCoeff4`, `FilterCoeff8`, `FilterCoeff16` |

The fields in `Config` are complete register bytes, not independent enum selections:

- `humidity_sampling`: `ctrl_hum`, with humidity oversampling in bits 2:0.
- `iir_coefficient`: `config`, with standby time in bits 7:5 and filter coefficient in bits 4:2.
- `temperature_pressure_mode`: `ctrl_meas`, with temperature oversampling in bits 7:5, pressure oversampling in bits 4:2, and mode in bits 1:0.

Example configuration for normal mode:

```cpp
hkk::bme280::Config cfg {
    .enable = true,
    .address = 0x76,
    .humidity_sampling = static_cast<uint8>(hkk::bme280::OversamplingX1),
    .iir_coefficient = static_cast<uint8>(
        (hkk::bme280::StandbyTime1000 << 5) |
        (hkk::bme280::FilterCoeff4 << 2)
    ),
    .temperature_pressure_mode = static_cast<uint8>(
        (hkk::bme280::OversamplingX2 << 5) |
        (hkk::bme280::OversamplingX16 << 2) |
        hkk::bme280::NormalMode
    ),
    .name = "BME280_0",
    .location = "Living Room",
};
```

For forced mode, replace `NormalMode` with `ForceMode`. Each call to `measure()` or `process()` then starts a new conversion by writing `ctrl_meas` again.

### `Config`

```cpp
struct Config {
    bool8 enable;
    uint8 address;
    uint8 humidity_sampling;
    uint8 iir_coefficient;
    uint8 temperature_pressure_mode;
    const char *name = nullptr;
    const char *location = nullptr;
};
```

`enable` gates every sensor operation. `address` is the configured bus address. `name` and `location` are optional application metadata. The object copies `Config`, but does not copy the strings to which its pointers refer.

### `Context`

```cpp
struct Context {
    CalibrationParams calibration;
    float64 pressure;
    float64 temperature;
    float64 humidity;
    float64 absolute_humidity;
    float64 dew_point;
    float64 vapor_pressure;
    float64 saturation_vapor_pressure;
    float64 vapor_pressure_deficit;
    uint8 pressure_raw_data[3];
    uint8 temperature_raw_data[3];
    uint8 humidity_raw_data[2];
    uint8 operation_mode;
    int8 status = BME280_OK;
};
```

`calibration` holds the device-specific `dig_T*`, `dig_P*`, and `dig_H*` coefficients. The three raw arrays contain the most recent uncompensated sample. `operation_mode` is determined during initialization from the low two bits of `temperature_pressure_mode`.

### Result codes

All BME280 operations return `BME280_OK` on success or a negative `Result` value. The main errors describe disabled operation, missing data, zero-length buffers, failed reads or writes, transaction failures, an unexpected device ID, timeout, CRC, storage, and generic or unimplemented operations. Use `hkk::bme280::rts(status)` to obtain a stable string for logging.

### `BME280::BME280()`

```cpp
BME280(hkk::bus::i2c::I2C &i2c, const Config &cfg);
BME280(hkk::bus::spi::SPI &spi, const Config &cfg);
```

Constructs a sensor object using the supplied bus and a copy of `cfg`. The bus object must remain alive for the complete lifetime of the sensor object.

### `BME280::setup()`

```cpp
int8 setup();
int8 setup(Context &res);
```

Performs the complete startup sequence by calling `init()`. The overload without a context updates the internal context.

**Returns**

`BME280_OK` on success; otherwise a negative BME280 result.

### `BME280::init()`

```cpp
int8 init();
int8 init(Context &res);
```

Reads and validates the device ID, performs a soft reset, waits for the internal memory copy to finish, reads calibration coefficients, writes all three configured register bytes, and reads each byte back for verification. It also records sleep, forced, or normal mode in the context.

**Parameters**

- `res`: context that receives calibration data, mode, and status.

**Returns**

`BME280_OK` on success, `BME280_ERROR_DEVICE_NOT_FOUND` for an unexpected ID, or another negative result on communication or verification failure.

### `BME280::get_sensor_id()`

```cpp
int8 get_sensor_id();
int8 get_sensor_id(Context &res);
```

Reads the `Id` register and verifies the expected BME280 value.

**Returns**

`BME280_OK`, `BME280_ERROR_DEVICE_NOT_FOUND`, or a communication error.

### `BME280::soft_reset()`

```cpp
int8 soft_reset();
int8 soft_reset(Context &res);
```

Writes `SoftReset` to the `Reset` register and polls the `ImUpdate` status bit until the internal memory update completes.

**Returns**

`BME280_OK` or a negative result.

### `BME280::calibrate()`

```cpp
int8 calibrate();
int8 calibrate(Context &res);
```

Reads both calibration regions and decodes all temperature, pressure, and humidity coefficients into `res.calibration`.

**Returns**

`BME280_OK` or a negative read error.

### `BME280::measure()`

```cpp
int8 measure();
int8 measure(Context &res);
```

In forced mode, starts a conversion by writing the configured `ctrl_meas` value. It then waits until the `Measuring` status bit clears and performs one contiguous eight-byte read beginning at `PressMsb`. The bytes are split into the pressure, temperature, and humidity raw arrays.

This function does not compensate the sample. Use `process()` when engineering values are required.

**Returns**

`BME280_OK` or a negative result. The current wait loop has no independent deadline, so a sensor that never clears `Measuring` can keep the call blocked.

### `BME280::process()`

```cpp
int8 process();
int8 process(Context &res);
```

Calls `measure()`, decodes the raw ADC values, applies the stored calibration coefficients in the required temperature-pressure-humidity order, and calculates all derived humidity and vapor values.

**Returns**

`BME280_OK` on success or the first negative error returned by the measurement path.

```cpp
hkk::bme280::Context sample;

if(sensor.setup(sample) == hkk::bme280::BME280_OK &&
   sensor.process(sample) == hkk::bme280::BME280_OK) {
    HINFO("T=%0.2f C RH=%0.2f %% P=%0.2f hPa",
          sample.temperature, sample.humidity, sample.pressure);
}
```

### `BME280::status()`

```cpp
int8 status();
int8 status(Context &res);
```

Returns the last status from the internal or supplied context. If the sensor is disabled, the disabled status is stored and returned.

### Register access

```cpp
int8 write_register(Register reg, uint8 value);
int8 write_register(Register reg, uint8 *data, size_t len);
int8 write_register(uint8 addr, Register reg, uint8 *data, size_t len);

int8 read_register(Register reg, uint8 &value);
int8 read_register(Register reg, uint8 *data, size_t len);
int8 read_register(uint8 addr, Register reg, uint8 *data, size_t len);
```

Compile-time array overloads are available for the buffer forms. Overloads without `addr` use `Config::address`.

**Parameters**

- `addr`: explicit bus address.
- `reg`: first register to access.
- `value`: one byte to write or a reference receiving one byte.
- `data`: source or destination buffer.
- `len`: number of data bytes, excluding the register selector.

**Returns**

`BME280_OK`, `BME280_ERROR_NULL_DATA`, `BME280_ERROR_ZERO_LENGTH`, or a mapped bus error.

### Measurement getters

```cpp
float64 humidity(bool8 absolute_humidity = false);
void humidity(Context &res);

float64 temperature();
void temperature(Context &res);

float64 pressure();
void pressure(Context &res);

float64 dew_point();
void dew_point(Context &res);

float64 vapor(Vapor type);
void vapor(Context &res);
```

Scalar overloads return values from the internal context. Context overloads copy the corresponding internal values into `res`.

- `humidity(false)` returns `%RH`; `humidity(true)` returns `g/m3`.
- `temperature()` and `dew_point()` return degC.
- `pressure()` returns hPa.
- `vapor(Vapor::Pressure)` and `vapor(Vapor::Saturation)` return Pa.
- `vapor(Vapor::Deficit)` returns kPa.

Call `process()` successfully before reading any getter.

The two `pressure()` overloads are declared by the public header but have no definition in this snapshot. Read `Context::pressure` after `process(Context&)` until those definitions are added.

### BME280 startup and sampling flow

1. Construct `Config` with complete register bytes.
2. Construct the sensor with its bus.
3. Call `setup()` once.
4. Call `process()` for each required sample.
5. Read fields from the same external context or use the internal-context getters.

In normal mode the sensor converts according to its configured standby cycle. In forced mode each `process()` call starts one conversion and waits for it to finish.

### BME280 current notes

- Use normal or forced mode for sampling. In sleep mode `measure()` does not start a conversion.
- Configuration bytes are read back during `init()`, but a byte mismatch currently returns the last read status; it can therefore log a mismatch while returning `BME280_OK`.
- The explicit-address `write_register()` overload currently routes the write through `Config::address`. The explicit-address `read_register()` overload uses its `addr` argument.
- The `pressure()` getter declarations currently lack definitions; use `Context::pressure` as described above.

## DHT20 API

Header:

```cpp
#include <hkk/drivers/dht20/dht20.hpp>
```

Namespace: `hkk::dht20`.

The driver reads temperature and relative humidity from the seven-byte measurement frame, validates CRC-8, and calculates absolute humidity, dew point, and vapor values.

### Data units

| `Context` field | Unit |
|---|---:|
| `temperature` | degC |
| `humidity` | %RH |
| `absolute_humidity` | g/m3 |
| `dew_point` | degC |
| `vapor_pressure` | Pa |
| `saturation_vapor_pressure` | Pa |
| `vapor_pressure_deficit` | kPa |

### `Config`

```cpp
struct Config {
    void *nvm = nullptr;
    bool8 enable;
    uint8 address = 0x38;
    const char *name = nullptr;
    const char *location = nullptr;
};
```

`enable` gates sensor operations. `address` defaults to `0x38`. `nvm` is reserved for optional storage use. `name` and `location` are optional application metadata.

### `Context`

```cpp
struct Context {
    float64 temperature;
    float64 humidity;
    float64 absolute_humidity;
    float64 dew_point;
    float64 vapor_pressure;
    float64 saturation_vapor_pressure;
    float64 vapor_pressure_deficit;
    uint8 *raw_data[DATA_FRAME_LENGTH];
    int8 status = DHT20_OK;
};
```

The numeric fields use the units listed above. `raw_data` is the raw-frame storage field as declared by the public API. `status` stores the last driver result.

### Result codes

All DHT20 operations return `DHT20_OK` on success or a negative `Result` value. Errors cover disabled operation, invalid pointers and lengths, failed reads or writes, transaction failure, missing device, timeout, CRC, storage, and generic or unimplemented operations. `hkk::dht20::rts(status)` converts a result to text.

### `DHT20::DHT20()`

```cpp
DHT20(hkk::bus::i2c::I2C &i2c, const Config &cfg);
```

Constructs a sensor object and copies `cfg`. The bus and all objects referenced through configuration pointers must outlive the sensor.

```cpp
hkk::dht20::Config cfg {
    .enable = true,
    .address = 0x38,
    .name = "DHT20_0",
    .location = "Living Room",
};

hkk::dht20::DHT20 sensor(bus, cfg);
```

### `DHT20::setup()`

```cpp
int8 setup();
int8 setup(Context &res);
```

Waits for sensor startup, calls `init()`, and observes the required settling delay before the first sample.

**Returns**

`DHT20_OK` or a negative DHT20 result.

### `DHT20::init()`

```cpp
int8 init();
int8 init(Context &res);
```

Reads the status byte using `Command::Init`. If the expected state bits are missing, it invokes the reset/recovery sequence.

**Parameters**

- `res`: context updated with the result.

**Returns**

`DHT20_OK` or a negative result.

### `DHT20::measure()`

```cpp
int8 measure();
int8 measure(Context &res);
```

Starts a conversion with `Command::Measure`, waits for the result, reads the complete frame, validates its CRC byte, and converts its packed 20-bit humidity and temperature fields.

**Parameters**

- `res`: destination for the measurement and status.

**Returns**

`DHT20_OK`, `DHT20_ERROR_CRC` on checksum mismatch, or another negative result.

### `DHT20::process()`

```cpp
int8 process();
int8 process(Context &res);
```

Runs `measure()` and then calculates absolute humidity, dew point, actual vapor pressure, saturation vapor pressure, and vapor pressure deficit.

**Returns**

`DHT20_OK` or the first negative result.

```cpp
hkk::dht20::Context sample;

if(sensor.setup(sample) == hkk::dht20::DHT20_OK &&
   sensor.process(sample) == hkk::dht20::DHT20_OK) {
    HINFO("T=%0.2f C RH=%0.2f %% AH=%0.2f g/m3",
          sample.temperature, sample.humidity, sample.absolute_humidity);
}
```

### `DHT20::status()`

```cpp
int8 status();
int8 status(Context &res);
```

Returns the last status from the internal or supplied context.

### `DHT20::soft_reset()`

```cpp
int8 soft_reset();
int8 soft_reset(Context &res);
```

Runs the recovery sequence used when initialization finds an invalid sensor state.

**Returns**

`DHT20_OK` or a negative result.

### Command and payload access

```cpp
int8 send_command(Command command);
int8 send_command(uint8 address, Command command);

int8 send_payload(uint8 *payload, size_t len);
int8 send_payload(uint8 addr, uint8 *payload, size_t len);
```

Compile-time array overloads are available for `send_payload()`. Forms without `address` or `addr` use `Config::address`.

**Parameters**

- `command`: one-byte `Init` or `Measure` command.
- `addr`, `address`: explicit bus address.
- `payload`: bytes to send.
- `len`: payload size in bytes.

**Returns**

`DHT20_OK`, `DHT20_ERROR_NULL_DATA`, `DHT20_ERROR_ZERO_LENGTH`, or a mapped bus error.

### Measurement getters

```cpp
float64 humidity(bool8 absolute_humidity = false);
void humidity(Context &res);

float64 temperature();
void temperature(Context &res);

float64 dew_point();
void dew_point(Context &res);

float64 vapor(Vapor type);
void vapor(Context &res);
```

Scalar overloads read the internal context. Context overloads copy the corresponding internal values into `res`.

- `humidity(false)` returns `%RH`; `humidity(true)` returns `g/m3`.
- `temperature()` and `dew_point()` return degC.
- `vapor(Vapor::Pressure)` and `vapor(Vapor::Saturation)` return Pa.
- `vapor(Vapor::Deficit)` returns kPa.

Call `process()` successfully before using these getters.

### CRC utilities

Header:

```cpp
#include <hkk/drivers/dht20/utils.hpp>
```

```cpp
int8 crc_calculate(uint8 &checksum, uint8 *data, size_t len);
int8 crc_validate(uint8 *data, size_t len);
const char *rts(int8 status);
```

Array overloads infer `len` at compile time. `crc_calculate()` writes the CRC-8 result to `checksum`. `crc_validate()` treats the final byte of `data` as the transmitted checksum.

**Returns**

`DHT20_OK` for a valid operation or `DHT20_ERROR_CRC` for an unsupported length or checksum mismatch.

## SGP30 API

Header:

```cpp
#include <hkk/drivers/sgp30/sgp30.hpp>
```

Namespace: `hkk::sgp30`.

The driver manages IAQ initialization, eCO2 and TVOC measurements, raw H2 and ethanol signals, humidity compensation, self-test, serial-number access, and persistent IAQ baselines.

### Data units

| `Context` field | Meaning | Unit |
|---|---|---:|
| `eco2` | equivalent carbon dioxide | ppm |
| `tvoc` | total volatile organic compounds | ppb |
| `h2` | raw hydrogen signal | raw ticks |
| `c2h6o` | raw ethanol signal | raw ticks |
| `absolute_humidity` | compensation input | g/m3 |

### Commands

`Command` defines `SoftReset`, `IaqInit`, `MeasureIaq`, `GetIaqBaseline`, `SetIaqBaseline`, `SetAbsoluteHumidity`, `MeasureTest`, `GetFeatureSet`, `MeasureRaw`, `GetTvocInceptiveBaseline`, `SetTvocBaseline`, and `GetSerialId`.

### `Config`

```cpp
struct Config {
    void *nvm = nullptr;
    void *timer = nullptr;
    bool8 enable;
    bool8 humid_compensation;
    uint8 address = 0x58;
    const char *name = nullptr;
    const char *location = nullptr;
};
```

**Fields**

- `nvm`: optional `hkk::storage::nvm::NVM *` used to retain IAQ baselines. The current `setup()` requires it.
- `timer`: optional timer-storage pointer used for automatic baseline-store requests.
- `enable`: enables public sensor operations.
- `humid_compensation`: permits `compensate_humidity()` to update the sensor.
- `address`: bus address, normally `0x58`.
- `name`, `location`: optional application metadata.

All referenced objects must remain valid for as long as the sensor uses them.

### `Context`

```cpp
struct Context {
    uint32 tvoc;
    uint32 eco2;
    uint32 h2;
    uint32 c2h6o;
    float32 absolute_humidity;
    bool8 calibrated = false;
    uint8 raw_absolute_humidity[3];
    uint8 tvoc_baseline[3];
    uint8 measure_test[3];
    uint8 feature_set[3];
    uint8 raw_data[6];
    uint8 baseline[6];
    uint8 serial_number[9];
    int8 status = SGP30_OK;
};
```

Three-byte sensor words contain two data bytes and one CRC byte. The six-byte IAQ baseline is stored publicly as the eCO2 word followed by the TVOC word. `calibrated` records whether a baseline was provided or restored.

### Result codes

All SGP30 operations return `SGP30_OK` or a negative `Result`. Errors distinguish disabled operation, invalid buffers, failed reads or writes, transaction failure, missing device, timeout, CRC failure, missing storage, storage transaction failure, and generic or unimplemented operations. Use `hkk::sgp30::rts(status)` for text.

### `SGP30::SGP30()`

```cpp
SGP30(hkk::bus::i2c::I2C &i2c, const Config &cfg);
```

Constructs a sensor using a referenced bus and a copied configuration.

```cpp
hkk::sgp30::Config cfg {
    .nvm = &storage,
    .timer = &timer_storage,
    .enable = true,
    .humid_compensation = true,
    .address = 0x58,
    .name = "SGP30_0",
    .location = "Living Room",
};

hkk::sgp30::SGP30 sensor(bus, cfg);
```

### `SGP30::setup()`

```cpp
int8 setup();
int8 setup(Context &res);
```

Initializes the IAQ algorithm and reads the serial number. If `res.baseline` already contains two valid CRC-protected words, it restores and stores that baseline. Otherwise it searches configured storage, restores a valid entry when found, and starts periodic store requests. A missing stored entry is treated as a cold start.

**Parameters**

- `res`: runtime context and optional input baseline.

**Returns**

`SGP30_OK`, `SGP30_ERROR_NVM` when storage is absent, or another negative result.

### `SGP30::process()`

```cpp
int8 process();
int8 process(Context &res);
```

Performs one IAQ measurement, one raw-signal measurement, refreshes the current baseline, and handles a pending baseline-store request.

The IAQ algorithm expects regular sampling. Schedule this call approximately once per second instead of invoking it in a tight loop.

**Returns**

`SGP30_OK` or the first negative result.

```cpp
hkk::sgp30::Context air;

if(sensor.setup(air) == hkk::sgp30::SGP30_OK) {
    if(sensor.process(air) == hkk::sgp30::SGP30_OK) {
        HINFO("eCO2=%u ppm TVOC=%u ppb", air.eco2, air.tvoc);
    }
}
```

### IAQ commands

```cpp
int8 iaq_init();
int8 iaq_init(Context &res);

int8 measure_iaq();
int8 measure_iaq(Context &res);

int8 get_iaq_baseline();
int8 get_iaq_baseline(Context &res);

int8 set_iaq_baseline(uint8 *baseline);
int8 set_iaq_baseline(Context &res);
```

- `iaq_init()` starts the IAQ algorithm.
- `measure_iaq()` reads eCO2 and TVOC and updates `raw_data`.
- `get_iaq_baseline()` reads two CRC-protected baseline words into `baseline`.
- `set_iaq_baseline()` restores the six-byte public baseline. The pointer form copies its input into the internal context first.

**Returns**

`SGP30_OK`, `SGP30_ERROR_CRC` for an invalid frame, a buffer error where applicable, or another negative result.

### Humidity compensation

```cpp
int8 set_absolute_humidity(uint8 *humidity);
int8 set_absolute_humidity(Context &res);

int8 compensate_humidity(float32 absolute_humidity);
int8 compensate_humidity(Context &res);
```

`set_absolute_humidity()` accepts the two data bytes of the sensor's unsigned 8.8 fixed-point humidity value and sends a CRC-protected command word. `compensate_humidity()` accepts absolute humidity in `g/m3`, converts it to that representation, and sends it.

`compensate_humidity()` returns `SGP30_ERROR_GENERIC` when `Config::humid_compensation` is false.

```cpp
if(dht.process(dht_sample) == hkk::dht20::DHT20_OK) {
    sgp.compensate_humidity(
        static_cast<float32>(dht_sample.absolute_humidity)
    );
}
```

Update compensation when absolute humidity changes materially; a fixed interval such as tens of seconds is normally sufficient for slowly changing room conditions.

### Diagnostic and raw commands

```cpp
int8 measure_test();
int8 measure_test(Context &res);

int8 feature_set();
int8 feature_set(Context &res);

int8 measure_raw();
int8 measure_raw(Context &res);

int8 get_tvoc_inceptive_baseline();
int8 get_tvoc_inceptive_baseline(Context &res);

int8 set_tvoc_baseline(uint8 *baseline);
int8 set_tvoc_baseline(Context &res);

int8 get_serial_number();
int8 get_serial_number(Context &res);

int8 soft_reset();
```

- `measure_test()` runs the built-in self-test and verifies its expected result.
- `feature_set()` reads and verifies the supported feature-set word.
- `measure_raw()` updates raw H2 and ethanol signals.
- `get_tvoc_inceptive_baseline()` and `set_tvoc_baseline()` access the TVOC-only baseline word.
- `get_serial_number()` reads three CRC-protected serial-number words.
- `soft_reset()` sends the reset command to `Config::address` and waits for restart.

Each function returns `SGP30_OK` or a negative result; read commands may return `SGP30_ERROR_CRC`.

### Low-level command access

```cpp
int8 send_command(Command command);
int8 send_payload(uint8 *payload, size_t len);

template<size_t N>
int8 send_payload(uint8 (&payload)[N]);
```

`send_command()` transmits the big-endian two-byte command. `send_payload()` transmits arbitrary bytes to `Config::address`.

**Returns**

`SGP30_OK`, `SGP30_ERROR_NULL_DATA`, `SGP30_ERROR_ZERO_LENGTH`, or a mapped bus error.

### Baseline lifecycle

```cpp
int8 calibrate();
int8 calibrate(Context &res);

int8 store_baseline();
int8 store_baseline(Context &res);

int8 load_baseline();
int8 load_baseline(Context &res);
```

- `calibrate()` clears configured storage, samples once per second until the internal 12-hour alarm marks calibration complete, and then stores the resulting baseline. This is intentionally a long blocking operation.
- `store_baseline()` reads the current sensor baseline and places it in configured storage.
- `load_baseline()` reads a stored baseline and applies it to the sensor.

All three require `Config::nvm`. They return `SGP30_ERROR_NVM` when it is absent and `SGP30_ERROR_NVM_TRANSACTION` when owned storage access fails.

### Status and getters

```cpp
int8 status();
int8 status(Context &res);

uint32 eco2();
void eco2(Context &res);
uint32 tvoc();
void tvoc(Context &res);
uint32 h2();
void h2(Context &res);
uint32 c2h6o();
void c2h6o(Context &res);

uint8 *baseline(bool8 tvoc_baseline = false);
void baseline(Context &res, bool8 tvoc_baseline = false);
uint8 *test();
void test(Context &res);
uint8 *serial_number();
void serial_number(Context &res);
float32 humidity_debug();
void humidity_debug(Context &res);
```

`status()` returns the context status. Numeric getters return or copy the most recent measurement. `baseline(false)` selects the six-byte IAQ baseline; `baseline(true)` selects the three-byte TVOC baseline. Pointer-returning getters expose internal storage and must not be freed or retained beyond the sensor object's lifetime. `humidity_debug()` returns the last absolute-humidity input.

### CRC utilities

Header:

```cpp
#include <hkk/drivers/sgp30/utils.hpp>
```

```cpp
int8 crc_calculate(uint8 &checksum, uint8 *data, size_t len);
int8 crc_validate(uint8 *data, size_t len);
const char *rts(int8 status);
```

Array overloads infer `len`. A sensor word contains two data bytes followed by one CRC byte. The functions return `SGP30_OK` or `SGP30_ERROR_CRC`.

## PMS5003 API

Header:

```cpp
#include <hkk/drivers/pms5003/pms5003.hpp>
```

Namespace: `hkk::pms5003`.

The driver models the 32-byte PMS5003 measurement frame, active and passive reporting modes, sleep and wake commands, atmospheric and CF=1 mass concentrations, and particle-count bins.

This driver is an initial implementation. Read the [current limitations](#pms5003-current-limitations) before using it in an application.

### Measurement units

- `pm1`, `pm2_5`, and `pm10`: atmospheric mass concentration in `ug/m3`.
- `pm1_ref`, `pm2_5_ref`, and `pm10_ref`: CF=1/reference-particle mass concentration in `ug/m3`.
- Particle-count bins: number of particles above the stated diameter in `0.1 L` of air.

For ambient-air reporting, use the fields without `_ref`. CF=1 values are primarily intended for controlled particle/reference interpretation.

### Commands and modes

```cpp
enum Command : uint8 {
    ChangeMode = 0xE1,
    Read       = 0xE2,
    SetSleep   = 0xE4,
};

enum Mode : uint8 {
    Passive = 0x00,
    Active  = 0x01,
};

enum Power : uint8 {
    Sleep  = 0x00,
    Wakeup = 0x01,
};
```

In active mode the sensor emits frames continuously. In passive mode the host sends `Read` for each requested frame. `Mode` and `Power` are intentionally separate types even though their current numeric values coincide.

### `Config`

```cpp
struct Config {
    void *nvm = nullptr;
    bool8 enable;
    bool8 compensate_humidity;
    Mode operation_mode = Mode::Passive;
    Power sleep_mode = Power::Wakeup;
    const char *name = nullptr;
    const char *location = nullptr;
};
```

`operation_mode` and `sleep_mode` are applied by `setup()`. `compensate_humidity` reserves the application policy switch for correction; the current `process()` path does not apply it. `nvm` is reserved. `name` and `location` are optional metadata.

### `Context`

```cpp
struct Context {
    uint16 pm1;
    uint16 pm2_5;
    uint16 pm10;
    uint16 pm1_ref;
    uint16 pm2_5_ref;
    uint16 pm10_ref;
    Mode operation_mode;
    Power sleep_mode;
    uint8 raw_data[DATA_FRAME_LENGTH];
    uint8 raw_pc_0_3[2];
    uint8 raw_pc_0_5[2];
    uint8 raw_pc_1_0[2];
    uint8 raw_pc_2_5[2];
    uint8 raw_pc_5_0[2];
    uint8 raw_pc_10_0[2];
    int8 status = PMS5003_OK;
};
```

The six `raw_pc_*` arrays hold big-endian particle-count words. Convert one with:

```cpp
uint16 pc_0_3 = static_cast<uint16>(
    (static_cast<uint16>(sample.raw_pc_0_3[0]) << 8) |
     sample.raw_pc_0_3[1]
);
```

### Data frame layout

| Bytes | Length | Field | Representation |
|---:|---:|---|---|
| 0-1 | 2 | Start marker | `0x42`, `0x4D` |
| 2-3 | 2 | Frame length | Big-endian; normally `28` bytes after this field |
| 4-5 | 2 | PM1.0 CF=1 | `ug/m3` |
| 6-7 | 2 | PM2.5 CF=1 | `ug/m3` |
| 8-9 | 2 | PM10 CF=1 | `ug/m3` |
| 10-11 | 2 | PM1.0 atmospheric | `ug/m3` |
| 12-13 | 2 | PM2.5 atmospheric | `ug/m3` |
| 14-15 | 2 | PM10 atmospheric | `ug/m3` |
| 16-17 | 2 | Particles >0.3 um | count/0.1 L |
| 18-19 | 2 | Particles >0.5 um | count/0.1 L |
| 20-21 | 2 | Particles >1.0 um | count/0.1 L |
| 22-23 | 2 | Particles >2.5 um | count/0.1 L |
| 24-25 | 2 | Particles >5.0 um | count/0.1 L |
| 26-27 | 2 | Particles >10 um | count/0.1 L |
| 28 | 1 | Version | Sensor version byte |
| 29 | 1 | Error code | Sensor error byte |
| 30-31 | 2 | Checksum | Big-endian sum of bytes 0-29 |

All two-byte values are big-endian. The current context exposes bytes 0-27 plus the decoded PM values; it does not provide separate named version or error fields.

### Result codes

PMS5003 functions return `PMS5003_OK` or a negative `Result`. The enum currently includes several legacy `PMS5003_ERROR_I2C*` names even though this driver communicates through UART; treat them as compatibility identifiers, not as a description of the transport. `hkk::pms5003::rts(status)` converts results to text.

### `PMS5003::PMS5003()`

```cpp
PMS5003(hkk::bus::uart::UART &uart, const Config &cfg);
```

Constructs a sensor using a referenced UART and a copied configuration. The UART must remain valid for the sensor's lifetime. Configure it for `9600` baud, eight data bits, no parity, and one stop bit.

```cpp
hkk::pms5003::Config cfg {
    .enable = true,
    .compensate_humidity = false,
    .operation_mode = hkk::pms5003::Mode::Passive,
    .sleep_mode = hkk::pms5003::Power::Wakeup,
    .name = "PMS5003_0",
    .location = "Living Room",
};

hkk::pms5003::PMS5003 sensor(uart, cfg);
```

### `PMS5003::setup()`

```cpp
int8 setup();
int8 setup(Context &res);
```

Copies configured operation and power modes into the context, then requests both states through `mode()` and `sleep()`.

**Returns**

`PMS5003_OK` or the first negative command result.

### `PMS5003::measure()`

```cpp
int8 measure();
int8 measure(Context &res);
```

The intended passive-mode flow sends `Command::Read`, receives exactly 32 bytes, validates the additive checksum, decodes the six mass concentrations, and copies six particle-count words. The overload without `res` uses the internal context.

**Returns**

`PMS5003_OK`, `PMS5003_ERROR_CRC`, a buffer error, or a read error.

### `PMS5003::process()`

```cpp
int8 process();
int8 process(Context &res);
```

Calls `measure()` and propagates its status. Automatic humidity correction is currently disabled in this path.

### Mode and power control

```cpp
int8 mode(Mode mode);
int8 mode(Context &res);

int8 sleep(Power mode);
int8 sleep(Context &res);
```

The value overloads update the internal context. Context overloads use `operation_mode` or `sleep_mode` already stored in `res`. Mode control sends `ChangeMode`; power control sends `SetSleep`.

**Returns**

`PMS5003_OK` or a negative command result.

### `PMS5003::send_command()`

```cpp
int8 send_command(Command command, uint8 value);
```

Builds the eight-byte command frame from the start marker, command, value, and additive checksum.

**Parameters**

- `command`: `ChangeMode`, `Read`, or `SetSleep`.
- `value`: command-specific mode or power byte; pass `0x00` for `Read`.

**Returns**

`PMS5003_OK` or `PMS5003_ERROR_CRC` if checksum construction fails.

### PM getters

```cpp
uint16 pm1(bool8 ref = false);
void pm1(Context &res, bool8 ref = false);

uint16 pm2_5(bool8 ref = false);
void pm2_5(Context &res, bool8 ref = false);

uint16 pm10(bool8 ref = false);
void pm10(Context &res, bool8 ref = false);
```

For scalar getters, `ref == false` selects the atmospheric value and `ref == true` selects CF=1. Values are in `ug/m3`. Call `process()` or `measure()` successfully first.

### Status text

Header:

```cpp
#include <hkk/drivers/pms5003/utils.hpp>
```

```cpp
const char *rts(int8 status);
```

Returns the symbolic name of a PMS5003 result.

### PMS5003 current limitations

The public types and frame decoder are present, but this snapshot still has the following functional gaps:

- `send_command()` calculates the command frame but does not yet write it to UART. Consequently `setup()`, mode changes, wake/sleep commands, and passive read requests do not reach the sensor.
- `measure()` returns immediately in active mode instead of consuming the next streamed frame.
- Frame decoding validates the checksum but does not yet reject an invalid start marker, frame-length field, version byte, or sensor error byte.
- `sleep(Power)` dispatches through the mode path instead of the power path.
- Context-copy PM getters currently select the opposite destination field; scalar getters have the expected `ref` behavior.
- UART error mapping currently returns `PMS5003_OK` for every input.
- Humidity/temperature correction is not part of public processing; only an internal experimental PM2.5 CF=1 expression remains.

Until these items are completed, use the UART API directly for production frame acquisition, then apply the frame table and checksum rule documented above.

## Recommended application flow

1. Initialize each required bus or storage object once.
2. Construct sensor configurations and sensor objects after their dependencies exist.
3. Call each sensor's `setup()` once and check the returned status.
4. Call `process(Context&)` at the cadence required by that sensor.
5. Read values from the same context passed to `process()`.
6. Log failures with the `rts()` function from the same namespace as the failing component.

Keep one `Context` per sensor instance when external contexts are used. Do not mix internal-context getters with an unrelated external context unless the copy semantics are explicitly desired.

## Error-handling pattern

```cpp
hkk::bme280::Context sample;

int8 status = sensor.setup(sample);
if(status < hkk::bme280::BME280_OK) {
    HERROR("BME280 setup failed: %s (%d)",
           hkk::bme280::rts(status), status);
    return;
}

status = sensor.process(sample);
if(status < hkk::bme280::BME280_OK) {
    HERROR("BME280 sample failed: %s (%d)",
           hkk::bme280::rts(status), status);
    return;
}
```

Status values are signed. Compare them with the component's zero-valued success constant; do not store them in an unsigned type.

## API maturity summary

| Area | Recommended use |
|---|---|
| Logger, bit helpers, sleep helpers | Normal use |
| I2C blocking transfer and ownership guard | Normal use |
| NVM operations and ownership guard | Normal use |
| BME280, DHT20, SGP30 core flows | Normal use, subject to their documented notes and blocking waits |
| UART blocking read/write | Normal use |
| UART timeout and DMA entry points | Experimental |
| SPI transfers and transactions | Experimental |
| PMS5003 | Initial implementation; see its limitations section |
