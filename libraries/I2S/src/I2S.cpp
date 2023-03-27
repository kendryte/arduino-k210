/*
  Copyright (c) 2016 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#define DBG_TAG "I2S"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "I2S.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I2SClass Public Fcuntions //////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// the device index and pins must map to the "COM" pads in Table 6-1 of the datasheet
I2SClass::I2SClass(uint8_t deviceIndex, uint8_t clockGenerator, uint8_t sdPin, uint8_t sckPin, uint8_t fsPin)
:_deviceIndex(deviceIndex)
,_sdPin(sdPin)
,_sckPin(sckPin)
,_fsPin(fsPin)

,_state(I2S_STATE_IDLE)
,_dmaChannel(DMAC_CHANNEL_MAX)
,_sampleRate(0)
,_bitsPerSample(0)
,_mode(I2S_PHILIPS_MODE)
,_dmaTransferInProgress(false)

,_onTransmitArg(NULL)
,_onReceiveArg(NULL)
,_onTransmit(NULL)
,_onReceive(NULL)
{

}

// the SCK and FS pins are driven as outputs using the sample rate
int I2SClass::begin(int mode, long sampleRate, int bitsPerSample)
{
  // master mode (driving clock and frame select pins - output)
  return begin(mode, sampleRate, bitsPerSample, true);
}

// the SCK and FS pins are inputs, other side controls sample rate
__attribute__ ((unavailable ("K210 not support I2S Slave mode"))) int I2SClass::begin(int mode, int bitsPerSample)
{
  LOG_E("K210 not support I2S Slave Mode.");
  return begin(mode, 16 * 1000, bitsPerSample, true); // default 16KHz samplerate.
}

void I2SClass::end()
{
  hal_dma_releas_chn(_dmaChannel);
  _dmaChannel = DMAC_CHANNEL_MAX;

  _doubleBuffer.end();

  hal_fpioa_clr_pin_func(_sdPin);
  hal_fpioa_clr_pin_func(_sckPin);
  hal_fpioa_clr_pin_func(_fsPin);
}

// from Stream
int I2SClass::available()
{
  return int(_doubleBuffer.available());
}

union i2s_sample_t {
  int16_t b16;
  int32_t b32;
};

int I2SClass::read()
{
  i2s_sample_t sample;

  sample.b32 = 0;

  read(&sample, _bitsPerSample / 8);

  if (_bitsPerSample == 32) {
    return sample.b32;
  } else if (_bitsPerSample == 16) {
    return sample.b16;
  } else {
    return 0;
  }
  return 0;
}

int I2SClass::peek()
{
  i2s_sample_t sample;

  sample.b32 = 0;

  _doubleBuffer.peek(&sample, _bitsPerSample / 8);

  if (_bitsPerSample == 32) {
    return sample.b32;
  } else if (_bitsPerSample == 16) {
    return sample.b16;
  } else {
    return 0;
  }
  return 0;
}

void I2SClass::flush()
{
  _doubleBuffer.flush();
}

// from Print
size_t I2SClass::write(uint8_t data)
{
  return write((int32_t)data);
}

size_t I2SClass::write(const uint8_t *buffer, size_t size)
{
  return write((const void*)buffer, size);
}

int I2SClass::availableForWrite()
{
  return _doubleBuffer.availableForWrite();
}

int I2SClass::read(void *buffer, size_t size)
{
  if (_state != I2S_STATE_RECEIVER) {
    enableReceiver();
  }

  int read = _doubleBuffer.read(buffer, size);

  if (_dmaTransferInProgress == false && _doubleBuffer.available() == 0) {
    // no DMA transfer in progress, start a receive process
    _dmaTransferInProgress = true;

    // DMA.transfer(_dmaChannel, i2sd.data(_deviceIndex), _doubleBuffer.data(), _doubleBuffer.availableForWrite());
    recvData();

    // switch to the next buffer for user output (will be empty)
    _doubleBuffer.swap();
  }

  return read;
}

size_t I2SClass::write(int data)
{
  return write((uint32_t)data);
}

size_t I2SClass::write(uint32_t data)
{
  return write((const void *)&data, 4);
}

size_t I2SClass::write(const void *buffer, size_t size)
{
  if (_state != I2S_STATE_TRANSMITTER) {
    enableTransmitter();
  }

  size_t written = _doubleBuffer.write(buffer, size);

  if ((false == _dmaTransferInProgress)&& _doubleBuffer.available()) {
    _dmaTransferInProgress = true;

    sendData();
    // switch to the next buffer for input
    _doubleBuffer.swap();
  }

  return written;
}

void I2SClass::onTransmit(callback_with_arg_t cb, void *arg)
{
  _onTransmit = cb;
  _onTransmitArg = arg;
}

void I2SClass::onReceive(callback_with_arg_t cb, void *arg)
{
  _onReceive = cb;
  _onReceiveArg = arg;
}

void I2SClass::setBufferSize(int bufferSize)
{
  _doubleBuffer.setSize(bufferSize);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I2SClass Private Fcuntions /////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int I2SClass::begin(int mode, long sampleRate, int bitsPerSample, bool driveClock)
{
  if (_state != I2S_STATE_IDLE) {
    LOG_E("I2s not in idle state");
    return -1;
  }

  switch (mode) {
    case I2S_PHILIPS_MODE:
    case I2S_RIGHT_JUSTIFIED_MODE:
    case I2S_LEFT_JUSTIFIED_MODE:

      _mode = i2s_mode_t(mode);
      break;

    default:
      // invalid mode
      LOG_E("Unsupport mode %d", mode);
      return -1;
  }

  switch (bitsPerSample) {
    case 16:
    case 32:
      _bitsPerSample = bitsPerSample;
      break;

    default:
      // invalid bits per sample
      LOG_E("Unsupport bitsPerSample %d", bitsPerSample);
      return -1;
  }
  _sampleRate = (uint32_t)sampleRate;

  _dmaChannel = hal_dma_get_free_chn();

  if(DMAC_CHANNEL_MAX == _dmaChannel) {
    LOG_E("failed get free dma chn");
    return -1;
  }

  _doubleBuffer.reset();

  hal_fpioa_set_pin_func(_sckPin, fpioa_function_t(FUNC_I2S0_SCLK + _deviceIndex * 11));
  hal_fpioa_set_pin_func(_fsPin, fpioa_function_t(FUNC_I2S0_WS + _deviceIndex * 11));

  return 0;
}

void I2SClass::enableTransmitter()
{
  hal_fpioa_clr_pin_func(_sdPin);
  hal_fpioa_set_pin_func(_sdPin, fpioa_function_t(FUNC_I2S0_OUT_D0 + _deviceIndex * 11));

  i2s_init(i2s_device_number_t(_deviceIndex), I2S_TRANSMITTER, 0x03);

  i2s_tx_channel_config(i2s_device_number_t(_deviceIndex), I2S_CHANNEL_0,
                        (16 == _bitsPerSample) ? RESOLUTION_16_BIT : RESOLUTION_32_BIT,
                        // (16 == _bitsPerSample) ? SCLK_CYCLES_16 : SCLK_CYCLES_32,
                        SCLK_CYCLES_32,
                        TRIGGER_LEVEL_4, i2s_work_mode_t(_mode));

  i2s_set_sample_rate(i2s_device_number_t(_deviceIndex), _sampleRate);

  if(16 == _bitsPerSample) {
    i2s_set_dma_divide_16(i2s_device_number_t(_deviceIndex), 1);
  } else {
    i2s_set_dma_divide_16(i2s_device_number_t(_deviceIndex), 0);
  }

  flush();
  _state = I2S_STATE_TRANSMITTER;
}

void I2SClass::enableReceiver()
{
  hal_fpioa_clr_pin_func(_sdPin);
  hal_fpioa_set_pin_func(_sdPin, fpioa_function_t(FUNC_I2S0_IN_D0 + _deviceIndex * 11));

  i2s_init(i2s_device_number_t(_deviceIndex), I2S_RECEIVER, 0x03);

  i2s_rx_channel_config(i2s_device_number_t(_deviceIndex), I2S_CHANNEL_0,
                        (16 == _bitsPerSample) ? RESOLUTION_16_BIT : RESOLUTION_32_BIT,
                        // (16 == _bitsPerSample) ? SCLK_CYCLES_16 : SCLK_CYCLES_32,
                        SCLK_CYCLES_32,
                        TRIGGER_LEVEL_4, i2s_work_mode_t(_mode));

  i2s_set_sample_rate(i2s_device_number_t(_deviceIndex), _sampleRate);

  if(16 == _bitsPerSample) {
    i2s_set_dma_divide_16(i2s_device_number_t(_deviceIndex), 1);
  } else {
    i2s_set_dma_divide_16(i2s_device_number_t(_deviceIndex), 0);
  }

  flush();
  _state = I2S_STATE_RECEIVER;
}

static int i2s_dma_irq(void *ctx)
{
  I2SClass *i2s = reinterpret_cast<I2SClass *>(ctx);

  i2s->_onTransferComplete();

  return 0;
}

void I2SClass::recvData(void)
{
#if 0
  uint32_t *buff = (uint32_t *)_doubleBuffer.data();
  size_t size = _doubleBuffer.availableForWrite() / (_bitsPerSample / 8) / 2;

  i2s_receive_data_dma(i2s_device_number_t(_deviceIndex), buff, size, _dmaChannel);
  
  _dmaTransferInProgress = false;

  if (_onReceive) {
    _onReceive(_onReceiveArg);
  }
#else
  i2s_data_t data;
  plic_interrupt_t irq;

  data.wait_dma_idle = true;
  data.wait_dma_done = false;

  data.mode = I2S_RECEIVER;

  data.recv.rx_channel = _dmaChannel;
  data.recv.rx_buf = (uint32_t *)_doubleBuffer.data();
  data.recv.rx_len = _doubleBuffer.availableForWrite() / (_bitsPerSample / 8) / 2;

  irq.callback = i2s_dma_irq;
  irq.ctx = this;
  irq.priority = 1;

  if(data.recv.rx_len)
    i2s_handle_data_dma(i2s_device_number_t(_deviceIndex), data, &irq);
#endif
}

void I2SClass::sendData(void)
{
#if 0
  uint32_t *buff = (uint32_t *)_doubleBuffer.data();
  size_t size = _doubleBuffer.available() / (_bitsPerSample / 8) / 2;

  i2s_send_data_dma(i2s_device_number_t(_deviceIndex), buff, size, _dmaChannel);

  _dmaTransferInProgress = false;

  if (_onTransmit) {
    _onTransmit(_onTransmitArg);
  }
#else
  i2s_data_t data;
  plic_interrupt_t irq;

  data.wait_dma_idle = true;
  data.wait_dma_done = false;

  data.mode = I2S_TRANSMITTER;

  data.send.tx_channel = _dmaChannel;
  data.send.tx_buf = (uint32_t *)_doubleBuffer.data();
  data.send.tx_len = _doubleBuffer.available() / (_bitsPerSample / 8) / 2;

  irq.callback = i2s_dma_irq;
  irq.ctx = this;
  irq.priority = 1;

  if(data.send.tx_len)
    i2s_handle_data_dma(i2s_device_number_t(_deviceIndex), data, &irq);
#endif
}

void I2SClass::_onTransferComplete(void)
{
  if (_state == I2S_STATE_TRANSMITTER) {
    // transmit complete

    if (_doubleBuffer.available()) {
      // output is available to transfer, start the DMA process for the current buffer

      // DMA.transfer(_dmaChannel, _doubleBuffer.data(), i2sd.data(_deviceIndex), _doubleBuffer.available());
      sendData();

      // swap to the next user buffer for input
      _doubleBuffer.swap();
    } else {
      // no user data buffered to send
      _dmaTransferInProgress = false;
    }

    // call the users transmit callback if provided
    if (_onTransmit) {
      _onTransmit(_onTransmitArg);
    }
  } else {
    // receive complete

    if (_doubleBuffer.available() == 0) {
      // the user has read all the current input, start the DMA process to fill it again
      // DMA.transfer(_dmaChannel, i2sd.data(_deviceIndex), _doubleBuffer.data(), _doubleBuffer.availableForWrite());
      recvData();

      // swap to the next buffer that has previously been filled, so that the user can read it
      _doubleBuffer.swap(_doubleBuffer.availableForWrite());
    } else {
      // user has not read current data, no free buffer to transfer into
      _dmaTransferInProgress = false;
    }

    // call the users receveive callback if provided
    if (_onReceive) {
      _onReceive(_onReceiveArg);
    }
  }
}
