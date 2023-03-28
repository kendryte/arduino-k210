#include "ts_lib.h"

void push_event_begin(struct ts_event_t *ts_event, int x, int y)
{
    ts_event->type = TOUCH_BEGIN;
    ts_event->x = x;
    ts_event->y = y;
}

void push_event_move(struct ts_event_t *ts_event, int x, int y)
{
    ts_event->type = TOUCH_MOVE;
    ts_event->x = x;
    ts_event->y = y;
}

void push_event_end(struct ts_event_t *ts_event, int x, int y)
{
    ts_event->type = TOUCH_END;
    ts_event->x = x;
    ts_event->y = y;
}

void push_event_none(struct ts_event_t *ts_event)
{
    ts_event->type = TOUCH_NONE;
    ts_event->x = 0;
    ts_event->y = 0;
}
