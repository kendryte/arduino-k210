#define DBG_TAG "Wire"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "Arduino.h"
#include "Wire.h"

static int i2c_slave_irq(void *TwoWireInst);

TwoWire Wire = TwoWire(0);

TwoWire::TwoWire(int busNum)
    :_clockFreq(100000)
    ,_sclPin(-1)
    ,_sdaPin(-1)
    ,_txAddress(0)
    ,_txBuff(NULL)
    ,_rxBuff(NULL)
    ,_nonStop(false)
    ,_nonStopTask(NULL)
    ,_lock(NULL)
    ,_initialized(false)
    ,_isSlave(false)
    ,_slaveTransmitting(false)
    ,user_onRequest(NULL)
    ,user_onReceive(NULL)
{
    if(busNum >= I2C_DEVICE_MAX) {
        LOG_E("TwoWire Invaild busNum %d\n", busNum);
        return;
    }
    _i2cNum = (i2c_device_number_t)busNum;
}

TwoWire::~TwoWire()
{
    end();
}

// Master
bool TwoWire::begin(int8_t sclPin, int8_t sdaPin, uint32_t frequency)
{
    bool started = false;
    char _lock_name[32]; // real max len 8

    if(_i2cNum >= I2C_DEVICE_MAX) {
        LOG_E("TwoWire Invaild _i2cNum %d\n", _i2cNum);
        started = false;
        goto _end;
    }

    if(_initialized) {
        LOG_W("Bus already started.");
        started = true;
        goto _end;
    }

    if(_isSlave) {
        LOG_W("Bus already started in Slave Mode.\n");
        goto _end;
    }

    _sclPin = sclPin;
    _sdaPin = sdaPin;
    _clockFreq = frequency;

    if((sclPin < 0) || (sdaPin < 0)) {
        LOG_E("Invaild sclPin(%d) or sdaPin(%d)\n", sclPin, sdaPin);
        return false;
    }

    snprintf(_lock_name, sizeof(_lock_name), "%2dMi2c", _i2cNum);
    if(NULL == _lock) {
        _lock = rt_mutex_create(_lock_name, RT_NULL);
        if(NULL == _lock) {
            LOG_E("rt_mutex_create failed.\n");
            return false;
        }
    }

    if(RT_EOK != rt_mutex_take(_lock, RT_WAITING_FOREVER)) {
        LOG_E("could not acquire lock.");
        return false;
    }

    _txBuff = new RingBuffer();
    _rxBuff = new RingBuffer();

    if(!_txBuff || !_rxBuff) {
        LOG_E("Alloc ringbuffer failed.\n");
        rt_mutex_release(_lock);
        goto _end;
    }

    hal_fpioa_set_pin_func(_sclPin, (fpioa_function_t)(_i2cNum * 2 + FUNC_I2C0_SCLK));
    hal_fpioa_set_pin_func(_sdaPin, (fpioa_function_t)(_i2cNum * 2 + FUNC_I2C0_SDA));

    started = true;

    rt_mutex_release(_lock);

_end:
    if(!started) {
        delete _txBuff;
        delete _rxBuff;
    }

    if(started) {
        _initialized = true;
    }

    return started;
}

// Slave
bool TwoWire::begin(uint16_t address, int8_t sclPin, int8_t sdaPin, uint32_t frequency)
{
#if 0
    bool started = false;
    int address_width = 7;

    volatile i2c_t *i2c_adapter = i2c[_i2cNum];

    _sclPin = sclPin;
    _sdaPin = sdaPin;
    _clockFreq = frequency;

    _isSlave = true;
    _slaveTransmitting = false;

    if((sclPin < 0) || (sdaPin < 0)) {
        LOG_E("Invaild sclPin(%d) or sdaPin(%d)\n", sclPin, sdaPin);
        return false;
    }

    char _lock_name[RT_NAME_MAX];
    snprintf(_lock_name, sizeof(_lock_name), "%dSi2c", _i2cNum);
    if(NULL == _lock) {
        _lock = rt_mutex_create(_lock_name, RT_NULL);
        if(NULL == _lock) {
            LOG_E("rt_mutex_create failed.\n");
            return false;
        }
    }

    if(RT_EOK != rt_mutex_take(_lock, RT_WAITING_FOREVER)) {
        LOG_E("could not acquire lock.");
        return false;
    }

    _txBuff = new RingBuffer();
    _rxBuff = new RingBuffer();

    if(!_txBuff || !_rxBuff) {
        LOG_E("Alloc ringbuffer failed.\n");
        goto _end;
    }

    hal_fpioa_set_pin_func(_sclPin, (fpioa_function_t)(_i2cNum * 2 + FUNC_I2C0_SCLK));
    hal_fpioa_set_pin_func(_sdaPin, (fpioa_function_t)(_i2cNum * 2 + FUNC_I2C0_SDA));

    address_width = (address & 0x380) ? 10 : 7;

//{
    // i2c_init_as_slave
    sysctl_clock_enable((sysctl_clock_t)(SYSCTL_CLOCK_I2C0 + _i2cNum));
    sysctl_clock_set_threshold((sysctl_threshold_t)(SYSCTL_THRESHOLD_I2C0 + _i2cNum), 3);

    i2c_adapter->enable = 0;
    i2c_adapter->con = (address_width == 10 ? I2C_CON_10BITADDR_SLAVE : 0) | I2C_CON_SPEED(1) | I2C_CON_STOP_DET_IFADDRESSED;
    i2c_adapter->ss_scl_hcnt = I2C_SS_SCL_HCNT_COUNT(37);
    i2c_adapter->ss_scl_lcnt = I2C_SS_SCL_LCNT_COUNT(40);
    i2c_adapter->sar = I2C_SAR_ADDRESS(address);
    i2c_adapter->rx_tl = I2C_RX_TL_VALUE(0);
    i2c_adapter->tx_tl = I2C_TX_TL_VALUE(0);
    i2c_adapter->intr_mask = I2C_INTR_MASK_RX_FULL | I2C_INTR_MASK_START_DET | I2C_INTR_MASK_STOP_DET | I2C_INTR_MASK_RD_REQ;

    plic_set_priority((plic_irq_t)(IRQN_I2C0_INTERRUPT + _i2cNum), 1);
    plic_irq_register((plic_irq_t)(IRQN_I2C0_INTERRUPT + _i2cNum), i2c_slave_irq, this);
    plic_irq_enable((plic_irq_t)(IRQN_I2C0_INTERRUPT + _i2cNum));

    i2c_adapter->enable = I2C_ENABLE_ENABLE;
//}

    started = true;

_end:
    if(!started) {
        delete _txBuff;
        delete _rxBuff;
    }
    rt_mutex_release(_lock);

    return started;
#else
    if(!begin(sclPin, sdaPin, frequency)) {
        return false;
    }

    if(NULL != _lock) {
        if(RT_EOK != rt_mutex_take(_lock, RT_WAITING_FOREVER)) {
            LOG_E("could not acquire lock.");
            return false;
        }
    }

    _isSlave = true;
    _slaveTransmitting = false;

    int address_width = (address & 0x380) ? 10 : 7;

//{
    // i2c_init_as_slave
    sysctl_clock_enable((sysctl_clock_t)(SYSCTL_CLOCK_I2C0 + _i2cNum));
    sysctl_clock_set_threshold((sysctl_threshold_t)(SYSCTL_THRESHOLD_I2C0 + _i2cNum), 3);

    volatile i2c_t *i2c_adapter = i2c[_i2cNum];

    i2c_adapter->enable = 0;
    i2c_adapter->con = (address_width == 10 ? I2C_CON_10BITADDR_SLAVE : 0) | I2C_CON_SPEED(1) | I2C_CON_STOP_DET_IFADDRESSED;
    i2c_adapter->ss_scl_hcnt = I2C_SS_SCL_HCNT_COUNT(37);
    i2c_adapter->ss_scl_lcnt = I2C_SS_SCL_LCNT_COUNT(40);
    i2c_adapter->sar = I2C_SAR_ADDRESS(address);
    i2c_adapter->rx_tl = I2C_RX_TL_VALUE(0);
    i2c_adapter->tx_tl = I2C_TX_TL_VALUE(0);
    i2c_adapter->intr_mask = I2C_INTR_MASK_RX_FULL | I2C_INTR_MASK_START_DET | I2C_INTR_MASK_STOP_DET | I2C_INTR_MASK_RD_REQ;

    plic_set_priority((plic_irq_t)(IRQN_I2C0_INTERRUPT + _i2cNum), 1);
    plic_irq_register((plic_irq_t)(IRQN_I2C0_INTERRUPT + _i2cNum), i2c_slave_irq, this);
    plic_irq_enable((plic_irq_t)(IRQN_I2C0_INTERRUPT + _i2cNum));

    i2c_adapter->enable = I2C_ENABLE_ENABLE;
//}

    rt_mutex_release(_lock);

    return true;
#endif
}

bool TwoWire::end()
{
    if(NULL != _lock) {
        if(RT_EOK != rt_mutex_take(_lock, RT_WAITING_FOREVER)) {
            LOG_E("could not acquire lock.");
            return false;
        }
    }

    if(_isSlave) {
        // release irq
        _isSlave = false;

        plic_irq_disable((plic_irq_t)(IRQN_I2C0_INTERRUPT + _i2cNum));
        plic_irq_unregister((plic_irq_t)(IRQN_I2C0_INTERRUPT + _i2cNum));
    }

    sysctl_clock_disable((sysctl_clock_t)(SYSCTL_CLOCK_I2C0 + _i2cNum));

    // free pins
    hal_fpioa_clr_pin_func(_sclPin);
    hal_fpioa_clr_pin_func(_sdaPin);

    delete _txBuff;
    delete _rxBuff;

    _initialized = false;
    _txAddress = 0;

    rt_mutex_release(_lock);

    if(NULL != _lock) {
        rt_mutex_delete(_lock);
        _lock = NULL;
    }

    return true;
}

void TwoWire::setClock(uint32_t freq)
{
    _clockFreq = freq;
}

void TwoWire::beginTransmission(uint16_t address)
{
    if(_isSlave) {
        LOG_E("Bus is Slave Mode\n");
        return;
    }

    if(_nonStop && _nonStopTask == rt_thread_self()) {
        LOG_W("Unfinished Repeated Start transaction! Expected requestFrom, not beginTransmission! Clearing...\n");
        //release lock
        rt_mutex_release(_lock);
    }

    if((NULL == _lock) || (RT_EOK != rt_mutex_take(_lock, RT_WAITING_FOREVER))) {
        LOG_E("could not acquire lock\n");
        return;
    }

    flush();
    _nonStop = false;

    if(_txAddress != address) {
        _txAddress = address;

        uint32_t addr_width = (_txAddress & 0x380) ? 10 : 7;
        i2c_init(_i2cNum, _txAddress, addr_width, _clockFreq);
    }
}

/*
https://www.arduino.cc/reference/en/language/functions/communication/wire/endtransmission/
endTransmission() returns:
0: success.
1: data too long to fit in transmit buffer.
2: received NACK on transmit of address.
3: received NACK on transmit of data.
4: other error.
5: timeout
6: bus error.
*/
uint8_t TwoWire::endTransmission(bool sendStop)
{
    int ret = 0;

    if(_isSlave) {
        LOG_W("Bus is Slave Mode\n");
        return 4;
    }
    if(!_txBuff) {
        LOG_D("NULL TX buffer pointer\n");
        return 4;
    }

    if(sendStop) {
        uint8_t buf[HARDWARE_I2C_BUFF_LEN];
        size_t size = _txBuff->available();

        if(size > sizeof(buf)) {
            LOG_E("data too long to transfer\n");
            return 1;
        }
        for(size_t i = 0; i < size; i++) {
            buf[i] = _txBuff->read_char();
        }

        rt_enter_critical();
        if(0x00 != i2c_send_data(_i2cNum, buf, size)) {
            ret = 6;
        }
        rt_exit_critical();

        rt_mutex_release(_lock);
    } else {
        _nonStop = true;
        _nonStopTask = rt_thread_self();
    }

    return ret;
}

size_t TwoWire::requestFrom(uint16_t address, size_t len, bool sendStop)
{
    if(_isSlave) {
        LOG_W("Bus is Slave Mode\n");
        return 0;
    }
    if(!_txBuff || !_rxBuff) {
        LOG_W("NULL buffer pointer\n");
        return 0;
    }

    uint8_t rxBuf[HARDWARE_I2C_BUFF_LEN];
    size_t rxSize = len > HARDWARE_I2C_BUFF_LEN ? HARDWARE_I2C_BUFF_LEN : len;

    if(_nonStop && _nonStopTask == rt_thread_self()) {
        if(address != _txAddress){
            LOG_D("Unfinished Repeated Start transaction! Expected address do not match! %u != %u", address, _txAddress);
            return 0;
        }
        _nonStop = false;

        uint8_t txBuf[HARDWARE_I2C_BUFF_LEN];
        size_t txSize= _txBuff->available();
        if(sizeof(txBuf) < txSize) {
            LOG_E("data too long to transfer\n");
            return 0;
        }

        for(size_t i = 0; i < txSize; i++) {
            txBuf[i] = _txBuff->read_char();
        }

        rt_enter_critical();
        if(0x00 != i2c_recv_data(_i2cNum, txBuf, txSize, rxBuf, rxSize)) {
            rt_exit_critical();

            rxSize = 0;
            LOG_D("i2c nonStop read error\n");
        } else {
            rt_exit_critical();

            _rxBuff->clear();
            for(size_t i = 0; i < rxSize; i++) {
                if(!_rxBuff->availableForStore()) {
                    break;
                }
                _rxBuff->store_char(rxBuf[i]);
            }
        }
    } else {
        if((NULL == _lock) || (RT_EOK != rt_mutex_take(_lock, RT_WAITING_FOREVER))) {
            LOG_E("could not acquire lock\n");
            return 0;
        }

        if(0x00 != i2c_recv_data(_i2cNum, NULL, 0, rxBuf, rxSize)) {
            rxSize = 0;
            LOG_D("i2c nonStop read error\n");
        } else {
            _rxBuff->clear();
            for(size_t i = 0; i < rxSize; i++) {
                if(!_rxBuff->availableForStore()) {
                    break;
                }
                _rxBuff->store_char(rxBuf[i]);
            }
        }
    }

    rt_mutex_release(_lock);

    return rxSize;
}


size_t TwoWire::requestFrom(uint8_t address, size_t len, bool sendStop)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), static_cast<bool>(sendStop));
}
  
uint8_t TwoWire::requestFrom(uint8_t address, uint8_t len, uint8_t sendStop)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), static_cast<bool>(sendStop));
}

uint8_t TwoWire::requestFrom(uint16_t address, uint8_t len, uint8_t sendStop)
{
    return requestFrom(address, static_cast<size_t>(len), static_cast<bool>(sendStop));
}

/* Added to match the Arduino function definition: https://github.com/arduino/ArduinoCore-API/blob/173e8eadced2ad32eeb93bcbd5c49f8d6a055ea6/api/TwoWire.h#L39
 * See: https://github.com/arduino-libraries/ArduinoECCX08/issues/25
*/
uint8_t TwoWire::requestFrom(uint16_t address, uint8_t len, bool sendStop)
{
    return requestFrom((uint16_t)address, (size_t)len, sendStop);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t len)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), true);
}

uint8_t TwoWire::requestFrom(uint16_t address, uint8_t len)
{
    return requestFrom(address, static_cast<size_t>(len), true);
}

uint8_t TwoWire::requestFrom(int address, int len)
{
    return requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), true);
}

uint8_t TwoWire::requestFrom(int address, int len, int sendStop)
{
    return static_cast<uint8_t>(requestFrom(static_cast<uint16_t>(address), static_cast<size_t>(len), static_cast<bool>(sendStop)));
}

void TwoWire::beginTransmission(int address)
{
    beginTransmission(static_cast<uint16_t>(address));
}

void TwoWire::beginTransmission(uint8_t address)
{
    beginTransmission(static_cast<uint16_t>(address));
}

uint8_t TwoWire::endTransmission(void)
{
    return endTransmission(true);
}

size_t TwoWire::write(uint8_t c)
{
    if((!_txBuff) || (_txBuff->isFull())) {
        return 0;
    }

    _txBuff->store_char(c);

    return 1;
}

size_t TwoWire::write(const uint8_t *buffer, size_t size)
{
    for(size_t i = 0; i < size; i++) {
        if(!write(buffer[i])) {
            return i;
        }
    }
    return size;
}

int TwoWire::available()
{
    return _rxBuff->available();
}

int TwoWire::read()
{
    return _rxBuff->read_char();
}

int TwoWire::peek()
{
    return _rxBuff->peek();
}

void TwoWire::flush()
{
    _txBuff->clear();
    _rxBuff->clear();
}

// Slave Mode, on Master read
void TwoWire::onRequest(void(*function)(void))
{
    user_onRequest = function;
}

// Slave Mode, on Master write
void TwoWire::onReceive(void(*function)(int))
{
    user_onReceive = function;
}

void TwoWire::_on_event(i2c_event_t event)
{
    if(I2C_EV_START == event) {
        _slaveTransmitting = true;
    } else /* if(I2C_EV_STOP == event) */ {
        _slaveTransmitting = false;
    }
}

void TwoWire::_on_receive(uint32_t data)
{
    if(_slaveTransmitting) {
        _rxBuff->store_char((uint8_t)data);

        if(user_onReceive) {
            user_onReceive(1);
        }
    }
}

uint32_t TwoWire::_on_transmit(void)
{
    if(_slaveTransmitting) {
        if(user_onRequest) {
            user_onRequest();
        }

        return _txBuff->read_char();
    }

    return -1;
}

static int i2c_slave_irq(void *TwoWireInst)
{
    TwoWire *_i2cInst = reinterpret_cast<TwoWire *>(TwoWireInst); 

    volatile i2c_t *i2c_adapter = i2c[_i2cInst->_i2cNum];

    uint32_t status = i2c_adapter->intr_stat;

    if (status & I2C_INTR_STAT_START_DET) {
        _i2cInst->_on_event(I2C_EV_START);
        readl(&i2c_adapter->clr_start_det);
    }

    if (status & I2C_INTR_STAT_STOP_DET) {
        _i2cInst->_on_event(I2C_EV_STOP);
        readl(&i2c_adapter->clr_stop_det);
    }

    if (status & I2C_INTR_STAT_RX_FULL) {
        _i2cInst->_on_receive(i2c_adapter->data_cmd);
    }

    if (status & I2C_INTR_STAT_RD_REQ) {
        i2c_adapter->data_cmd = _i2cInst->_on_transmit();
        readl(&i2c_adapter->clr_rd_req);
    }

    return 0;
}
