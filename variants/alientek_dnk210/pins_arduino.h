#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

// UART
#define UART1_TX            (5)
#define UART1_RX            (4)

#define TFT_CSX_PIN         (36)
#define TFT_RST_PIN         (37)
#define TFT_DCX_PIN         (38)
#define TFT_CLK_PIN         (39)

#define SDCARD_CLK_PIN      (27)
#define SDCARD_MIS_PIN      (26)
#define SDCARD_MOS_PIN      (28)
#define SDCARD_CSX_PIN      (29)

#define CAMERA_PCLK_PIN     (47)
#define CAMERA_XCLK_PIN     (46)
#define CAMERA_HSYNC_PIN    (45)
#define CAMERA_VSYNC_PIN    (43)
#define CAMERA_PWDN_PIN     (44)
#define CAMERA_RST_PIN      (42)
#define CAMERA_SDA_PIN      (40)
#define CAMERA_SCL_PIN      (41)

// flash filesystem config
#define FLASH_FS_ADDR       (0xD00000)
#define FLASH_FS_SIZE       (0x300000)

#endif /* Pins_Arduino_h */
