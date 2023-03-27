#pragma once


#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint8_t g_spi0_in_use;

/// @brief Tft bus write command as byte
/// @param cmd Command buffer
/// @param length Command length
void hal_tft_write_command(uint8_t *cmd, uint32_t length);

/// @brief Tft bus write data as byte
/// @param data_buf Write data buffer pointer
/// @param length Wirte data buffer length
void hal_tft_write_byte(uint8_t *data_buf, uint32_t length);

/// @brief Tft bus write data as half word
/// @param data_buf Write data buffer pointer
/// @param length Wirte data buffer length
void hal_tft_write_half(uint16_t *data_buf, uint32_t length);

/// @brief Tft bus write data as word
/// @param data_buf Write data buffer pointer
/// @param length Wirte data buffer length
void hal_tft_write_word(uint32_t *data_buf, uint32_t length);

/// @brief Tft bus fill data
/// @param data_buf Fill data in pixel
/// @param length Fill data length
void hal_tft_fill_data(uint32_t *data_buf, uint32_t length);


/// @brief Init tft bus, use hardware SPI0 as 8080 bus
/// @param clk_pin Clock output pin
/// @param cs_pin Chip select pin
/// @param dc_pin Data command pin
/// @param freq Bus clock frequency
void hal_tft_begin(int8_t clk_pin, int8_t cs_pin, int8_t dc_pin, uint32_t freq);

/// @brief Deinit tft bus
/// @param  None
void hal_tft_end(void);

/// @brief Set tft bus cs pin and chip_select func
/// @param pin  Cs Pin
/// @param chip_select Chip_select func, `-1` use default
void hal_tft_set_cs(int8_t pin, spi_chip_select_t chip_select);

/// @brief Set tft bus dc pin and dc gpio func
/// @param pin Dc Pin
/// @param gpio_func Gpio func, `-1` use  default
void hal_tft_set_dc(int8_t pin, int gpio_func);

/// @brief Set tft bus clock frequency
/// @param freq Frequency in hz
void hal_tft_set_clock_freq(uint32_t freq);

#ifdef __cplusplus
}
#endif
