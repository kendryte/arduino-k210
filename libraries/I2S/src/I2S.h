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

#ifndef _I2S_H_INCLUDED
#define _I2S_H_INCLUDED

#include <Arduino.h>

#include "I2SDoubleBuffer.h"

#include "k210-hal.h"

#define I2S_HAS_SET_BUFFER_SIZE 1

typedef enum {
  I2S_PHILIPS_MODE          = STANDARD_MODE,
  I2S_RIGHT_JUSTIFIED_MODE  = RIGHT_JUSTIFYING_MODE,
  I2S_LEFT_JUSTIFIED_MODE   = LEFT_JUSTIFYING_MODE
} i2s_mode_t;

class I2SClass : public Stream
{
public:
  typedef void (*callback_t)(void);
  typedef void (*callback_with_arg_t)(void *);

  // the device index and pins must map to the "COM" pads in Table 6-1 of the datasheet 
  I2SClass(uint8_t deviceIndex, uint8_t clockGenerator, uint8_t sdPin, uint8_t sckPin, uint8_t fsPin);

  // the SCK and FS pins are driven as outputs using the sample rate
  int begin(int mode, long sampleRate, int bitsPerSample);
  // the SCK and FS pins are inputs, other side controls sample rate
  int begin(int mode, int bitsPerSample);
  void end();

  // from Stream
  virtual int available();
  virtual int read();
  virtual int peek();
  virtual void flush();

  // from Print
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buffer, size_t size);

  virtual int availableForWrite();

  int read(void* buffer, size_t size);

  size_t write(int);
  size_t write(uint32_t);
  size_t write(const void *buffer, size_t size);

  void onTransmit(callback_with_arg_t cb, void *arg);
  void onReceive(callback_with_arg_t cb, void *arg);

  void onTransmit(callback_t cb) {
    onTransmit(reinterpret_cast<callback_with_arg_t>(cb), NULL);
  }
  void onReceive(callback_t cb) {
    onReceive(reinterpret_cast<callback_with_arg_t>(cb), NULL);
  }

  /// @brief Change Receive and Transmit Buffer Size, should call befor begin.
  /// @param bufferSize New Buffer Size.
  void setBufferSize(int bufferSize);
  int getBufferSize(void) {
    return _doubleBuffer.getSize();
  }

  /// @brief for dma irq use, use can not call this.
  /// @param None
  void _onTransferComplete(void);

private:
  int begin(int mode, long sampleRate, int bitsPerSample, bool driveClock);

  void enableTransmitter();
  void enableReceiver();

  void sendData(void);
  void recvData(void);

private:
  typedef enum {
    I2S_STATE_IDLE,
    I2S_STATE_TRANSMITTER,
    I2S_STATE_RECEIVER
  } i2s_state_t;

  uint8_t _deviceIndex;
  uint8_t _clockGenerator;
  uint8_t _sdPin;
  uint8_t _sckPin;
  uint8_t _fsPin;

  i2s_state_t _state;
  dmac_channel_number_t _dmaChannel;
  uint32_t _sampleRate;
  int _bitsPerSample;
  i2s_mode_t _mode;

  volatile bool _dmaTransferInProgress;
  I2SDoubleBuffer _doubleBuffer;

  void *_onTransmitArg;
  void *_onReceiveArg;

  callback_with_arg_t _onTransmit;
  callback_with_arg_t _onReceive;
};

#endif
