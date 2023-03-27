#include "NoePixelWrap.h"

#define PIN 18
#define NUMPIXELS 1
#define DELAYVAL 10

Adafruit_NeoPixel_Warp pixels(NUMPIXELS, PIN, NEO_GRB);

void setup()
{
    Serial.begin(115200);
    if (!Serial)
    {
        exit(-1);

        while (1)
        {
        }
    }

    pixels.begin();

    pixels.clear();
    pixels.setPixelColor(0, pixels.Color(0, 0, 128));
    pixels.show();
}

void loop()
{
}
