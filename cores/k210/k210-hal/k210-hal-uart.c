#define DBG_TAG "HAL_UART"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "k210-hal.h"

#include <stdio.h>

#include "rthw.h"
#include "rtthread.h"
#include "rtdevice.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Uart                                                                                                              //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct device_uart
{
    rt_uint32_t hw_base;
    rt_uint32_t irqno;
};

/* START ported from kendryte standalone sdk uart.c */
#define __UART_BRATE_CONST  16

volatile uart_t* const  _uart[3] =
{
    (volatile uart_t*)UART1_BASE_ADDR,
    (volatile uart_t*)UART2_BASE_ADDR,
    (volatile uart_t*)UART3_BASE_ADDR
};

static void _uart_set_clock(uart_device_number_t channel, bool enable)
{
    if(enable) {
        sysctl_clock_enable(SYSCTL_CLOCK_UART1 + channel);
        sysctl_reset(SYSCTL_RESET_UART1 + channel);
    } else {
        sysctl_clock_disable(SYSCTL_CLOCK_UART1 + channel);
    }
}

/* END ported from kendryte standalone sdk uart.c */
static inline uart_device_number_t _get_uart_channel(rt_uint32_t addr)
{
    switch (addr)
    {
        case UART1_BASE_ADDR:
            return UART_DEVICE_1;
        case UART2_BASE_ADDR:
            return UART_DEVICE_2;
        case UART3_BASE_ADDR:
            return UART_DEVICE_3;
        default:
            return UART_DEVICE_MAX;
    }
}

/* UART ISR */
static void uart_irq_handler(int irqno, void *param)
{
    struct rt_serial_device *serial = (struct rt_serial_device *)param;
    struct device_uart *uart = serial->parent.user_data;
    uart_device_number_t channel = _get_uart_channel(uart->hw_base);
    RT_ASSERT(channel != UART_DEVICE_MAX);

    /* read interrupt status and clear it */
    if (_uart[channel]->LSR)
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
}

/*
 * UART interface
 */
static rt_err_t rt_uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct device_uart *uart;
    uart_bitwidth_t data_width = (uart_bitwidth_t)cfg->data_bits ;
    uart_stopbit_t stopbit = (uart_stopbit_t)cfg->stop_bits;
    uart_parity_t parity = (uart_parity_t)cfg->parity;

    uint32_t freq = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
    uint32_t divisor = freq / (uint32_t)cfg->baud_rate;
    uint8_t dlh = divisor >> 12;
    uint8_t dll = (divisor - (dlh << 12)) / __UART_BRATE_CONST;
    uint8_t dlf = divisor - (dlh << 12) - dll * __UART_BRATE_CONST;

    RT_ASSERT(serial != RT_NULL);
    serial->config = *cfg;

    uart = serial->parent.user_data;
    RT_ASSERT(uart != RT_NULL);

    uart_device_number_t channel = _get_uart_channel(uart->hw_base);
    RT_ASSERT(channel != UART_DEVICE_MAX);

    RT_ASSERT(data_width >= 5 && data_width <= 8);
    if (data_width == 5)
    {
        RT_ASSERT(stopbit != UART_STOP_2);
    }
    else
    {
        RT_ASSERT(stopbit != UART_STOP_1_5);
    }

    uint32_t stopbit_val = stopbit == UART_STOP_1 ? 0 : 1;
    uint32_t parity_val = 0;
    switch (parity)
    {
        case UART_PARITY_NONE:
            parity_val = 0;
            break;
        case UART_PARITY_ODD:
            parity_val = 1;
            break;
        case UART_PARITY_EVEN:
            parity_val = 3;
            break;
        default:
            RT_ASSERT(!"Invalid parity");
            break;
    }

    _uart[channel]->LCR |= 1u << 7;
    _uart[channel]->DLH = dlh;
    _uart[channel]->DLL = dll;
    _uart[channel]->DLF = dlf;
    _uart[channel]->LCR = 0;
    _uart[channel]->LCR = (data_width - 5) |
                          (stopbit_val << 2) |
                          (parity_val << 3);
    _uart[channel]->LCR &= ~(1u << 7);
    _uart[channel]->IER |= 0x80; /* THRE */
    _uart[channel]->FCR = UART_RECEIVE_FIFO_1 << 6 |
                          UART_SEND_FIFO_8 << 4 |
                          0x1 << 3 |
                          0x1;

    return (RT_EOK);
}

static rt_err_t uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct device_uart *uart;

    uart = serial->parent.user_data;
    uart_device_number_t channel = _get_uart_channel(uart->hw_base);

    RT_ASSERT(uart != RT_NULL);
    RT_ASSERT(channel != UART_DEVICE_MAX);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        /* Disable the UART Interrupt */
        rt_hw_interrupt_mask(uart->irqno);
        _uart[channel]->IER &= ~0x1;
        break;

    case RT_DEVICE_CTRL_SET_INT:
        /* install interrupt */
        rt_hw_interrupt_install(uart->irqno, uart_irq_handler, serial, serial->parent.parent.name);
        rt_hw_interrupt_umask(uart->irqno);
        _uart[channel]->IER |= 0x1;
        break;
    }

    return (RT_EOK);
}

static int drv_uart_putc(struct rt_serial_device *serial, char c)
{
    struct device_uart *uart = serial->parent.user_data;
    uart_device_number_t channel = _get_uart_channel(uart->hw_base);
    RT_ASSERT(channel != UART_DEVICE_MAX);

    while (_uart[channel]->LSR & (1u << 5));
    _uart[channel]->THR = c;

    return (1);
}

static int drv_uart_getc(struct rt_serial_device *serial)
{
    struct device_uart *uart = serial->parent.user_data;
    uart_device_number_t channel = _get_uart_channel(uart->hw_base);
    RT_ASSERT(channel != UART_DEVICE_MAX);

    if (_uart[channel]->LSR & 1)
        return (char)(_uart[channel]->RBR & 0xff);
    else
        return EOF;
    /* Receive Data Available */

    return (-1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Uart HAL                                                                                                          //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const struct rt_uart_ops _uart_ops =
{
    rt_uart_configure,
    uart_control,
    drv_uart_putc,
    drv_uart_getc,
    //TODO: add DMA support
    RT_NULL
};

#define K210_UART_DEV_NAME_MAX_LEN  (7)

struct uart_struct_t {
    const uint8_t               num;
    const char                  name[K210_UART_DEV_NAME_MAX_LEN];
    const struct device_uart    uart;

    struct rt_serial_device     serial;
    rt_device_t                 dev;
};

static hal_uart_t _uart_bus_array[UART_DEVICE_MAX] = {
    {
        .num    = 0,
        .name   = "uart1",
        .dev    = NULL,
        .uart = {UART1_BASE_ADDR, IRQN_UART1_INTERRUPT},
        .serial = {.ops = &_uart_ops, .config = RT_SERIAL_CONFIG_DEFAULT},
    },
    {
        .num    = 1,
        .name   = "uart2",
        .dev    = NULL,
        .uart = {UART2_BASE_ADDR, IRQN_UART2_INTERRUPT},
        .serial = {.ops = &_uart_ops, .config = RT_SERIAL_CONFIG_DEFAULT},
    },
    {
        .num    = 2,
        .name   = "uart3",
        .dev    = NULL,
        .uart = {UART3_BASE_ADDR, IRQN_UART3_INTERRUPT},
        .serial = {.ops = &_uart_ops, .config = RT_SERIAL_CONFIG_DEFAULT},
    },
};

hal_uart_t* hal_uart_begin(uint8_t uartNum, uint32_t baudrate, uint32_t config, int8_t rxPin, int8_t txPin, uint16_t buffer_size)
{
    rt_err_t err = 0;

    if(uartNum >= UART_DEVICE_MAX) {
        return NULL;
    }

    hal_uart_t* hal_uart = &_uart_bus_array[uartNum];
    if(hal_uart->dev) {
        hal_uart_end(hal_uart);
    }

    _uart_set_clock(UART_DEVICE_1 + hal_uart->num, true);
    hal_fpioa_set_pin_func(txPin, FUNC_UART1_TX + hal_uart->num * 2);
    hal_fpioa_set_pin_func(rxPin, FUNC_UART1_RX + hal_uart->num * 2);

    hal_uart->dev = rt_device_find(hal_uart->name);

    if(!hal_uart->dev) { // if no device, register it.
        const struct device_uart *uart  = &hal_uart->uart;
        struct rt_serial_device *serial = &hal_uart->serial;

        serial->config.bufsz = buffer_size;

        rt_hw_serial_register(serial, (const char *)hal_uart->name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX , (void *)uart);

        hal_uart->dev = rt_device_find(hal_uart->name);
    }

    if(hal_uart->dev) {
        struct serial_configure _config = RT_SERIAL_CONFIG_DEFAULT;

        switch (config & SERIAL_DATA_MASK)
        {
        case SERIAL_DATA_5:
            _config.data_bits = UART_BITWIDTH_5BIT;
            break;
        case SERIAL_DATA_6:
            _config.data_bits = UART_BITWIDTH_6BIT;
            break;
        case SERIAL_DATA_7:
            _config.data_bits = UART_BITWIDTH_7BIT;
            break;
        default:
        case SERIAL_DATA_8:
            _config.data_bits = UART_BITWIDTH_8BIT;
            break;
        }

        switch (config & SERIAL_STOP_BIT_MASK)
        {
        case SERIAL_STOP_BIT_1_5:
            _config.stop_bits = UART_STOP_1_5;
            break;
        case SERIAL_STOP_BIT_2:
            _config.stop_bits = UART_STOP_2;
            break;
        default:
        case SERIAL_STOP_BIT_1:
            _config.stop_bits = UART_STOP_1;
            break;
        }

        switch (config & SERIAL_PARITY_MASK)
        {
        case SERIAL_PARITY_EVEN:
            _config.parity = UART_PARITY_EVEN;
            break;
        case SERIAL_PARITY_ODD:
            _config.parity = UART_PARITY_ODD;
            break;

        // not supported.
        /*
        case SERIAL_PARITY_MARK:
            _config.parity = UART_PARITY_NONE;
            break;
        case SERIAL_PARITY_SPACE:
            _config.parity = UART_PARITY_NONE;
            break;
        */

        default:
        case SERIAL_PARITY_NONE:
            _config.parity = UART_PARITY_NONE;
            break;
        }

        _config.bufsz = ((struct rt_serial_device *)hal_uart->dev)->config.bufsz; // rtt can not modify this size.
        _config.baud_rate = baudrate;

        err = rt_device_open(hal_uart->dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
        err += rt_device_control(hal_uart->dev, RT_DEVICE_CTRL_CONFIG, &_config);

        RT_ASSERT(err == RT_EOK);
    }

    return hal_uart;
}

void hal_uart_end(hal_uart_t* uart)
{
    if(uart == NULL) {
        return;
    }

    if(uart->dev) {
        rt_err_t err = 0;

        err = rt_device_close(uart->dev);
        RT_ASSERT(err == RT_EOK);

        _uart_set_clock(UART_DEVICE_1 + uart->num, false);

        uart->dev = NULL;
    }
}

int hal_uart_available(hal_uart_t *uart)
{
    if (uart->dev == NULL)
    {
        return 0;
    }

    struct rt_serial_device *_dev = (struct rt_serial_device *)uart->dev;
    struct rt_serial_rx_fifo *rx_fifo = (struct rt_serial_rx_fifo *)_dev->serial_rx;

    if (NULL == rx_fifo)
    {
        return 0;
    }

    rt_base_t level = rt_hw_interrupt_disable();

    int rx_length = (rx_fifo->put_index >= rx_fifo->get_index) ? \
                        (rx_fifo->put_index - rx_fifo->get_index) : \
                        (_dev->config.bufsz - (rx_fifo->get_index - rx_fifo->put_index));

    rt_hw_interrupt_enable(level);

    return rx_length;
}

int hal_uart_read_one(hal_uart_t* uart)
{
    if(uart->dev == NULL) {
        return -1;
    }

    uint8_t c = 0;

    rt_size_t l = rt_device_read(uart->dev, 0, &c, 1);

    return (l == 1) ? c : -1;
}

size_t hal_uart_read_to_buffer(hal_uart_t* uart, uint8_t *buffer, size_t size)
{
    if((uart->dev == NULL) || (NULL == buffer) || (0x0 == size)) {
        return 0;
    }

    return rt_device_read(uart->dev, 0,buffer, size);
}

size_t hal_uart_write_one(hal_uart_t* uart, uint8_t c)
{
    if(uart->dev == NULL) {
        return 0;
    }

    uint8_t send = c;
    return rt_device_write(uart->dev, 0, &send, 1);
}

size_t hal_uart_write_from_buffer(hal_uart_t* uart, const uint8_t *buffer, size_t size)
{
    if((NULL == uart->dev) || (NULL == buffer) || (0x0 == size)) {
        return 0;
    }

    return rt_device_write(uart->dev, 0, buffer, size);
}

bool hal_uart_is_opened(hal_uart_t* uart)
{
    if(NULL == uart->dev) {
        return false;
    }

    return (uart->dev->open_flag & RT_DEVICE_OFLAG_OPEN) ? true : false;
}
