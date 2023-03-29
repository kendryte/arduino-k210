#include "NS2009.h"

NS2009::NS2009(TwoWire *wire)
    : TouchScreen(wire, NS2009_SLV_ADDR)
{
    _cal[0] = -19;
    _cal[1] = -5904;
    _cal[2] = 21883430;
    _cal[3] = 4211;
    _cal[4] = 5;
    _cal[5] = -887123;
    _cal[6] = 65536;
}

NS2009::~NS2009()
{
    end();
}

bool NS2009::begin(int cal[7])
{
    if (0x00 != ts_read(NS2009_LOW_POWER_READ_Z1, NULL))
    {
        return false;
    }

    if(cal)
    {
        memcpy(_cal, cal, sizeof(int) * 7);
    }

    ts_pdata.filter = tsfilter_alloc(5, 5);
    tsfilter_setcal(ts_pdata.filter, _cal);

    ts_pdata.x = 0;
    ts_pdata.y = 0;
    ts_pdata.press = 0;

    ts_pdata.event = (struct ts_event_t *)touchscreen_malloc(sizeof(struct ts_event_t));
    ts_pdata.event->x = 0;
    ts_pdata.event->y = 0;
    ts_pdata.event->type = TOUCH_NONE;

    return true;
}

void NS2009::end(void)
{
    if (ts_pdata.filter)
    {
        tsfilter_free(ts_pdata.filter);
        ts_pdata.filter = NULL;
    }

    if (ts_pdata.event)
    {
        touchscreen_free(ts_pdata.event);
        ts_pdata.event = NULL;
    }
}

int NS2009::_poll()
{
    int x = 0, y = 0, z1 = 0;

    if (0x00 == ts_read(NS2009_LOW_POWER_READ_Z1, &z1))
    {
        if ((z1 > 70) && (z1 < 2000))
        {
            ts_read(NS2009_LOW_POWER_READ_X, &x);
            ts_read(NS2009_LOW_POWER_READ_Y, &y);
            tsfilter_update(ts_pdata.filter, &x, &y);

            if (!ts_pdata.press)
            {
                push_event_begin(ts_pdata.event, x, y);
                ts_pdata.press = 1;
            }
            else
            {
                if ((ts_pdata.x != x) || (ts_pdata.y != y))
                {
                    push_event_move(ts_pdata.event, x, y);
                }
            }
            ts_pdata.x = x;
            ts_pdata.y = y;
        }
        else
        {
            if (ts_pdata.press)
            {
                tsfilter_clear(ts_pdata.filter);
                push_event_end(ts_pdata.event, ts_pdata.x, ts_pdata.y);
                ts_pdata.press = 0;
            }
        }
    }
    else
    {
        push_event_none(ts_pdata.event);
    }

    return 0;
}
