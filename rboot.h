// This file is part of MBoot
// SPDX-License-Identifier: MIT

// This file includes code from rBoot by Richard A Burton, covered by the following copyright notice:

// rBoot open source boot loader for ESP8266.
// Copyright 2015 Richard A Burton
// richardaburton@gmail.com
// See license.txt for license terms.

#ifndef __RBOOT_H__
#define __RBOOT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef RBOOT_INTEGRATION
#include <rboot-integration.h>
#endif

#define BOOT_BAUDRATE 115200

// uncomment to use only c code
// if you aren't using gcc you may need to do this
//#define BOOT_NO_ASM

// uncomment to add a boot delay, allows you time to connect
// a terminal before rBoot starts to run and output messages
// value is in microseconds
#define BOOT_DELAY_MICROS 250


// you should not need to modify anything below this line


#define CHKSUM_INIT 0xef

#define SECTOR_SIZE 0x1000

#ifdef __cplusplus
}
#endif

#endif
