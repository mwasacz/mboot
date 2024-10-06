// This file is part of MBoot
// Copyright (C) 2021 Mikolaj Wasacz
// SPDX-License-Identifier: MIT

// This file includes code from rBoot by Richard A Burton, covered by the following copyright notice:

// rBoot open source boot loader for ESP8266.
// Copyright 2015 Richard A Burton
// richardaburton@gmail.com
// See license.txt for license terms.

#include "rboot-private.h"
#include <rboot-hex2a.h>

#ifndef UART_CLK_FREQ
// reset apb freq = 2x crystal freq: http://esp8266-re.foogod.com/wiki/Serial_UART
#define UART_CLK_FREQ	(26000000 * 2)
#endif

static uint32_t check_image(uint32_t readpos) {

	uint8_t buffer[BUFFER_SIZE];
	uint8_t sectcount;
	uint8_t sectcurrent;
	uint8_t chksum = CHKSUM_INIT;
	uint32_t loop;
	uint32_t remaining;
	uint32_t romaddr;

	rom_header_new *header = (rom_header_new*)buffer;
	section_header *section = (section_header*)buffer;

	if (readpos == 0 || readpos == 0xffffffff) {
		return 0;
	}

	// read rom header
	if (SPIRead(readpos, header, sizeof(rom_header_new)) != 0) {
		return 0;
	}

	// check header type
	if (header->magic == ROM_MAGIC) {
		// old type, no extra header or irom section to skip over
		romaddr = readpos;
		readpos += sizeof(rom_header);
		sectcount = header->count;
	} else if (header->magic == ROM_MAGIC_NEW1 && header->count == ROM_MAGIC_NEW2) {
		// new type, has extra header and irom section first
		romaddr = readpos + header->len + sizeof(rom_header_new);
		// skip the extra header and irom section
		readpos = romaddr;
		// read the normal header that follows
		if (SPIRead(readpos, header, sizeof(rom_header)) != 0) {
			return 0;
		}
		sectcount = header->count;
		readpos += sizeof(rom_header);
	} else {
		return 0;
	}

	// test each section
	for (sectcurrent = 0; sectcurrent < sectcount; sectcurrent++) {

		// read section header
		if (SPIRead(readpos, section, sizeof(section_header)) != 0) {
			return 0;
		}
		readpos += sizeof(section_header);

		// get section address and length
		remaining = section->length;

		while (remaining > 0) {
			// work out how much to read, up to BUFFER_SIZE
			uint32_t readlen = (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE;
			// read the block
			if (SPIRead(readpos, buffer, readlen) != 0) {
				return 0;
			}
			// increment next read position
			readpos += readlen;
			// decrement remaining count
			remaining -= readlen;
			// add to chksum
			for (loop = 0; loop < readlen; loop++) {
				chksum ^= buffer[loop];
			}
		}
	}

	// round up to next 16 and get checksum
	readpos = readpos | 0x0f;
	if (SPIRead(readpos, buffer, 1) != 0) {
		return 0;
	}

	// compare calculated and stored checksums
	if (buffer[0] != chksum) {
		return 0;
	}

	return romaddr;
}

#ifdef BOOT_BAUDRATE
static enum rst_reason get_reset_reason(void) {

	// reset reason is stored @ offset 0 in system rtc memory
	volatile uint32_t *rtc = (uint32_t*)0x60001100;

	return *rtc;
}
#endif

// prevent this function being placed inline with main
// to keep main's stack size as small as possible
// don't mark as static or it'll be optimised out when
// using the assembler stub
uint32_t NOINLINE find_image(void) {

	sector_config sectorconf;
	uint8_t buffer[SECTOR_SIZE];
	rom_header *header = (rom_header*)buffer;
	rom_config *romconf = (rom_config*)buffer;

#ifdef BOOT_BAUDRATE
	// soft reset doesn't reset PLL/divider, so leave as configured
	if (get_reset_reason() != REASON_SOFT_RESTART) {
		uart_div_modify( 0, UART_CLK_FREQ / BOOT_BAUDRATE);
	}
#endif

#if defined BOOT_DELAY_MICROS && BOOT_DELAY_MICROS > 0
	// delay to slow boot (help see messages when debugging)
	ets_delay_us(BOOT_DELAY_MICROS);
#endif

	ets_printf("\r\nmBoot v1.0.0 by mwasacz\r\nBased on rBoot by richardaburton@gmail.com\r\n");

	// read rom header
	SPIRead(0, header, sizeof(rom_header));

	// get flash size and rom 1 addr, print flash size
	ets_printf("Flash Size:   ");
	uint8_t flag = header->flags2 >> 4;
	if (flag > 9) {
		flag = 7;
	}
	const flash_info infos[10] = {
		{ "512KB",   0x80000,   0x41000  }, // 0
		{ "256KB",   0x40000,   0x21000  }, // 1
		{ "2MB",     0x200000,  0x81000  }, // 2
		{ "1MB",     0x100000,  0x81000  }, // 3
		{ "4MB",     0x400000,  0x81000  }, // 4
		{ "2MB-c1",  0x200000,  0x101000 }, // 5
		{ "4MB-c1",  0x400000,  0x101000 }, // 6
		{ "unknown", 0x80000,   0x41000  }, // other
		{ "8MB",     0x800000,  0x101000 }, // 8
		{ "16MB",    0x1000000, 0x101000 }  // 9
	};
	flash_info info = infos[flag];
	ets_printf(info.label);

	// print spi mode
	ets_printf("\r\nFlash Mode:   ");
	if (header->flags1 == 0) {
		ets_printf("QIO");
	} else if (header->flags1 == 1) {
		ets_printf("QOUT");
	} else if (header->flags1 == 2) {
		ets_printf("DIO");
	} else if (header->flags1 == 3) {
		ets_printf("DOUT");
	} else {
		ets_printf("unknown");
	}

	// print spi speed
	ets_printf("\r\nFlash Speed:  ");
	flag = header->flags2 & 0x0f;
	if (flag == 0) {
		ets_printf("40MHz");
	} else if (flag == 1) {
		ets_printf("26.7MHz");
	} else if (flag == 2) {
		ets_printf("20MHz");
	} else if (flag == 0x0f) {
		ets_printf("80MHz");
	} else {
		ets_printf("unknown");
	}
	ets_printf("\r\n\r\n");

	// update chip_size to make SPIRead work properly with big flash sizes
	flashchip->chip_size = info.size;

	// read sector_config
	uint32_t addr = info.size - SECTOR_SIZE;
	SPIRead(addr, &sectorconf, sizeof(sector_config));

	// determine which sector holds recent rom_config and read it
	addr = sectorconf.sector ? info.size - 2 * SECTOR_SIZE : info.size - 3 * SECTOR_SIZE;
	SPIRead(addr, buffer, SECTOR_SIZE);

	// check if rom_config is valid
	uint16_t config = romconf->config;
	if ((config & 0x1f00) != 0x0700
		|| (config & 0x6000) == 0x2000
		|| (config & 0x8000) == 0
		|| (config & 0x0002) != 0)
	{
		ets_printf("Invalid configuration, using default.\r\n");
		romconf->raw[0] = 0xffffffff;
		romconf->raw[1] = 0xffffffff;
		config = 0xe7fc;
	}

	// determine default and alternative rom addresses
	uint32_t romAddr = config & DEFAULT_ROM ? info.rom1_addr : 0x1000;
	uint32_t altAddr = config & DEFAULT_ROM ? 0x1000 : info.rom1_addr;

	// check images
	uint32_t loadAddr = check_image(romAddr);
	if (loadAddr == 0) {
		loadAddr = check_image(altAddr);
		if (loadAddr == 0) {
			ets_printf("Both roms are bad.\r\n");
			return 0;
		}
		config &= ~BOOTING_DEFAULT;
	} else {
		config |= BOOTING_DEFAULT;
	}
	
	// update config if necessary
	if (config != romconf->config) {
		ets_printf("Updating configuration.\r\n");
		romconf->config = config;
		sectorconf.sector = !sectorconf.sector;

		addr = sectorconf.sector ? info.size - 2 * SECTOR_SIZE : info.size - 3 * SECTOR_SIZE;
		SPIEraseSector(addr / SECTOR_SIZE);
		SPIWrite(addr, buffer, SECTOR_SIZE);

		addr = info.size - SECTOR_SIZE;
		SPIEraseSector(addr / SECTOR_SIZE);
		SPIWrite(addr, &sectorconf, sizeof(sector_config));
	}

	// print rom to boot
	if (config & BOOTING_DEFAULT) {
		ets_printf("Booting rom %d", 1 + (config & DEFAULT_ROM));
	} else {
		ets_printf("Booting backup rom %d", 2 - (config & DEFAULT_ROM));
	}
	ets_printf(", load addr %x.\r\n\r\n", loadAddr);

	// copy the loader to top of iram
	ets_memcpy((void*)_text_addr, _text_data, _text_len);

	// return address to load from
	return loadAddr;
}

#ifdef BOOT_NO_ASM

// small stub method to ensure minimum stack space used
void call_user_start(void) {
	uint32_t addr;
	stage2a *loader;

	addr = find_image();
	if (addr != 0) {
		loader = (stage2a*)entry_addr;
		loader(addr);
	}
}

#else

// assembler stub uses no stack space
// works with gcc
void call_user_start(void) {
	__asm volatile (
		"mov a15, a0\n"          // store return addr, hope nobody wanted a15!
		"call0 find_image\n"     // find a good rom to boot
		"mov a0, a15\n"          // restore return addr
		"bnez a2, 1f\n"          // ?success
		"ret\n"                  // no, return
		"1:\n"                   // yes...
		"movi a3, entry_addr\n"  // get pointer to entry_addr
		"l32i a3, a3, 0\n"       // get value of entry_addr
		"jx a3\n"                // now jump to it
	);
}

#endif
