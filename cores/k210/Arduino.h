// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
// Public entry point for the K210 compatibility layer.
#include "Binary.h"
#include "Common.h"
#include "k210-hal.h"
#include "pins_arduino.h"
#include "rtthread.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define ARDUINO_API_VERSION 10300
#ifdef __cplusplus
#include "Client.h"
#include "Compat.h"
#include "HardwareSerial.h"
#include "IPAddress.h"
#include "Print.h"
#include "Printable.h"
#include "Server.h"
#include "Stream.h"
#include "Udp.h"
#include "WCharacter.h"
#include "WString.h"
#endif
