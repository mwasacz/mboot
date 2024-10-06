// This file is part of MBoot
// Copyright (C) 2021 Mikolaj Wasacz
// SPDX-License-Identifier: MIT

// This file includes code from rBoot by Richard A Burton, covered by the following copyright notice:

// rBoot open source boot loader for ESP8266.
// Copyright 2015 Richard A Burton
// richardaburton@gmail.com
// See license.txt for license terms.

#ifndef __RBOOT_PRIVATE_H__
#define __RBOOT_PRIVATE_H__

#include <rboot.h>

#define NOINLINE __attribute__ ((noinline))

#define ROM_MAGIC	   0xe9
#define ROM_MAGIC_NEW1 0xea
#define ROM_MAGIC_NEW2 0x04

// buffer size, must be at least 0x10 (size of rom_header_new structure)
#define BUFFER_SIZE 0x100

// stage2 read chunk maximum size (limit for SPIRead)
#define READ_SIZE 0x1000

typedef struct {
	uint32_t deviceId;
	uint32_t chip_size;
	uint32_t block_size;
	uint32_t sector_size;
	uint32_t page_size;
	uint32_t status_mask;
} SpiFlashChip;

// esp8266 built in rom functions
extern uint32_t SPIRead(uint32_t addr, void *outptr, uint32_t len);
extern uint32_t SPIEraseSector(int);
extern uint32_t SPIWrite(uint32_t addr, void *inptr, uint32_t len);
extern void ets_printf(char*, ...);
extern void ets_delay_us(int);
extern void ets_memset(void*, uint8_t, uint32_t);
extern void ets_memcpy(void*, const void*, uint32_t);
extern SpiFlashChip *flashchip;

typedef struct {
	char *label;
	uint32_t size;
	uint32_t rom1_addr;
} flash_info;

typedef union {
	uint8_t sector;
	uint32_t raw[1];
} sector_config;

typedef union {
	uint16_t config;
	uint32_t raw[2];
} rom_config;

#define DEFAULT_ROM		0x0001
#define BOOTING_DEFAULT	0x0004

// functions we'll call by address
typedef void stage2a(uint32_t);
typedef void usercode(void);

// standard rom header
typedef struct {
	// general rom header
	uint8_t magic;
	uint8_t count;
	uint8_t flags1;
	uint8_t flags2;
	usercode* entry;
} rom_header;

typedef struct {
	uint8_t* address;
	uint32_t length;
} section_header;

// new rom header (irom section first) there is
// another 8 byte header straight afterward the
// standard header
typedef struct {
	// general rom header
	uint8_t magic;
	uint8_t count; // second magic for new header
	uint8_t flags1;
	uint8_t flags2;
	uint32_t entry;
	// new type rom, lib header
	uint32_t add; // zero
	uint32_t len; // length of irom section
} rom_header_new;

// RTC reset reason values
enum rst_reason {
	REASON_DEFAULT_RST		= 0,
	REASON_WDT_RST			= 1,
	REASON_EXCEPTION_RST	= 2,
	REASON_SOFT_WDT_RST   	= 3,
	REASON_SOFT_RESTART 	= 4,
	REASON_DEEP_SLEEP_AWAKE	= 5,
	REASON_EXT_SYS_RST      = 6
};

#endif
