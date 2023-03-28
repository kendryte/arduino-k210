#pragma once

#include "TouchScreen.h"

#define NS2009_SLV_ADDR     (0x48)

enum
{
    NS2009_LOW_POWER_READ_X = 0xc0,
    NS2009_LOW_POWER_READ_Y = 0xd0,
    NS2009_LOW_POWER_READ_Z1 = 0xe0,
    NS2009_LOW_POWER_READ_Z2 = 0xf0,
};

class NS2009 : public TouchScreen {
    public:
        NS2009(TwoWire *wire);
        ~NS2009();

        bool begin(int cal[7] = NULL);

        int _poll();
    private:
        int _cal[7];

        void end(void);
};
