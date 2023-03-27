#pragma once

#include "Arduino.h"

namespace K210 {

typedef struct _lcd_pins
{
    int8_t clk;
    int8_t cs;
    int8_t dc;
    int8_t rst;
} lcd_pins_t;

class Bus8080
{
public:
    /// @brief Begin TFT Bus
    /// @param freq Output clock frequency
    static void begin(uint32_t freq = 15000000);

    /// @brief Begin TFT Bus
    /// @param pins LCD configure
    /// @param pins LCD configure
    /// @param freq Output clock frequency
    static void begin(lcd_pins_t &pins, uint32_t freq = 15000000);

    /// @brief Close lcd output
    /// @param  None
    static void end(void);

    /// @brief Control RESET Pin
    /// @param valid Rst Vaild state
    /// @param rst_ms Set Rst Invaild time
    static void reset(PinStatus valid, int rst_ms);

    /// @brief TFT Bus write command
    /// @param command Command, 8bit
    static void writeCommand(uint8_t command);

    /// @brief TFT Bus write command
    /// @param command Command, 16bit
    static void writeCommand(uint16_t command);

    /// @brief TFT Bus write Data
    /// @param data Data, 8bit
    static void writeData(uint8_t data);

    /// @brief TFT Bus write Data
    /// @param data Data, 8bit
    /// @param len Data length
    static void writeData(uint8_t *data, uint32_t len);

    /// @brief TFT Bus write Data
    /// @param data Data, 16bit
    /// @param len Data length
    static void writeData(uint16_t *data, uint32_t len);

    /// @brief TFT Bus write Data
    /// @param data Data, 32bit
    /// @param len Data length
    static void writeData(uint32_t *data, uint32_t len);

    /// @brief TFT Write data fill with color
    /// @param color Color value
    /// @param w Rect width
    /// @param h Rect height
    /// @note This is a optimized fillScreen method, use dma to burst.
    static void fillScreen(uint16_t color, int16_t w, int16_t h);

private:
    static lcd_pins_t _pins;
};

} // namespace K210
