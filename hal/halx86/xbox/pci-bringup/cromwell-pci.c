#include "xbox-pci-compat.h"
#include <stdarg.h>

/* Whole file runs once via HalInitSystem / NxConfigurePciDevicesLate; the
 * build-time INIT pass (tools/lto-init-relink.py) places this init-only code in
 * the discardable INIT section automatically -- no source-side CODE_SEG tags. */
// by ozpaulb@hotmail.com 2002-07-14

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Copyright (C) 2018 Matt Borgerson                                     *
 *   Copyright (C) 2019 Stanislav Motylkov                                 *
 *                                                                         *
 ***************************************************************************/

/* xbox_ram lives in xbox/video/xbox-video-support.c (defined there as
 * `unsigned int xbox_ram = 64`).  Cromwell's original `unsigned int
 * xbox_ram;` definition is removed from this copy to avoid duplicate-symbol
 * link errors; this TU sees it as an extern through xbox-pci-compat.h ->
 * compiled-in elsewhere. */
extern unsigned int xbox_ram;

u8 PciReadByte(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off)
{
	u32 base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

		IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfc)));
		return IoInputByte(0xcfc + (reg_off & 3));
}

void PciWriteByte (unsigned int bus, unsigned int dev, unsigned int func,
		unsigned int reg_off, unsigned char byteval)
{
	u32 base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

	IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfc)));
	IoOutputByte(0xcfc + (reg_off & 3), byteval);
}


u16 PciReadWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off)
{
	u32 base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

		IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfe)));
		return IoInputWord(0xcfc + (reg_off & 1));
}


void PciWriteWord(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, u16 w)
{
	u32 base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #

	IoOutputDword(0xcf8, (base_addr + (reg_off & 0xfc)));
	IoOutputWord(0xcfc + (reg_off & 1), w);
}


u32 PciReadDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off)
{
	u32 base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #
        base_addr |= ((func & 0x07) << 8);
        base_addr |= ((reg_off & 0xff));

	IoOutputDword(0xcf8, base_addr);
	return IoInputDword(0xcfc);
}


u32 PciWriteDword(unsigned int bus, unsigned int dev, unsigned int func, unsigned int reg_off, unsigned int dw)
{

	u32 base_addr = 0x80000000;
	base_addr |= ((bus & 0xFF) << 16);	// bus #
	base_addr |= ((dev & 0x1F) << 11);	// device #
	base_addr |= ((func & 0x07) << 8);	// func #
	base_addr |= ((reg_off & 0xff));

	IoOutputDword(0xcf8, base_addr );
	IoOutputDword(0xcfc ,dw);

	return 0;
}
#define RTC_REG_A		10
#define RTC_REG_B		11
#define RTC_REG_C		12
#define RTC_REG_D		13
#define RTC_FREQ_SELECT		RTC_REG_A
#define RTC_CONTROL		RTC_REG_B
#define RTC_INTR_FLAGS		RTC_REG_C

/* On PCs, the checksum is built only over bytes 16..45 */
#define PC_CKS_RANGE_START	16
#define PC_CKS_RANGE_END	45
#define PC_CKS_LOC		46

#define RTC_RATE_1024HZ		0x06
#define RTC_REF_CLCK_32KHZ	0x20
#define RTC_FREQ_SELECT_DEFAULT (RTC_REF_CLCK_32KHZ | RTC_RATE_1024HZ)
#define RTC_24H 		0x02
#define RTC_CONTROL_DEFAULT (RTC_24H)

// access to RTC CMOS memory
u8 CMOS_READ(u8 addr) {
	IoOutputByte(0x70,addr);
	return IoInputByte(0x71);
}

void CMOS_WRITE(u8 val, u8 addr) {
	IoOutputByte(0x70,addr);
	IoOutputByte(0x71,val);
}

void BiosCmosWrite(u8 bAds, u8 bData) {
	IoOutputByte(0x70, bAds);
	IoOutputByte(0x71, bData);

	IoOutputByte(0x72, bAds);
	IoOutputByte(0x73, bData);
}

u8 BiosCmosRead(u8 bAds)
{
	IoOutputByte(0x72, bAds);
	return IoInputByte(0x73);
}

int rtc_checksum_valid(int range_start, int range_end, int cks_loc)
{
	int i;
	unsigned sum, old_sum;
	sum = 0;
	for(i = range_start; i <= range_end; i++) {
		sum += CMOS_READ(i);
	}
	sum = (~sum)&0x0ffff;
	old_sum = ((CMOS_READ(cks_loc)<<8) | CMOS_READ(cks_loc+1))&0x0ffff;
	return sum == old_sum;
}

void rtc_set_checksum(int range_start, int range_end, int cks_loc)
{
	int i;
	unsigned sum;
	sum = 0;
	for(i = range_start; i <= range_end; i++) {
		sum += CMOS_READ(i);
	}
	sum = ~(sum & 0x0ffff);
	CMOS_WRITE(((sum >> 8) & 0x0ff), cks_loc);
	CMOS_WRITE(((sum >> 0) & 0x0ff), cks_loc+1);
}

/* SMSC LPC47M157 Super I/O functions */
/* pin and register compatible with LPC47M192 */
void LpcSelectRegister(u8 index)
{
	IoOutputByte(0x2E, index);
}

void LpcEnterConfiguration(void)
{
	LpcSelectRegister(0x55);
}

void LpcExitConfiguration(void)
{
	LpcSelectRegister(0xAA);
}

u8 LpcReadRegister(u8 index)
{
	LpcSelectRegister(index);
	return IoInputByte(0x2F);
}

void LpcWriteRegister(u8 index, u8 value)
{
	LpcSelectRegister(index);
	IoOutputByte(0x2F, value);
}

int LpcGetSerialState(void)
{
	// Select serial device
	LpcWriteRegister(0x07, 0x04);

	// Check whether device is enabled
	return LpcReadRegister(0x30);
}

void LpcSetSerialState(int enable)
{
	// Select serial device
	LpcWriteRegister(0x07, 0x04);

	if (enable) {
		// Set serial base
		LpcWriteRegister(0x61, SERIAL_PORT & 0xFF);
		LpcWriteRegister(0x60, SERIAL_PORT >> 8);
	}

	// Enable device
	LpcWriteRegister(0x30, enable ? 0x01 : 0x00);
}

int LpcGetSerialIRQState(void)
{
	// Select serial device
	LpcWriteRegister(0x07, 0x04);

	// Check whether device has interrupt enabled
	return !!(LpcReadRegister(0x70) & 0x0F);
}

void LpcSetSerialIRQState(int enable)
{
	// Select serial device
	LpcWriteRegister(0x07, 0x04);

	// Enable device interrupt
	LpcWriteRegister(0x70, enable ? SERIAL_IRQ : 0x00);
}

int serial_putchar(int ch)
{
	/* Wait for THRE (bit 5) to be high */
	while ((IoInputByte(SERIAL_PORT + SERIAL_LSR) & (1 << 5)) == 0);
	IoOutputByte(SERIAL_PORT + SERIAL_THR, ch);

	if (ch == '\n')
	{
		/* Also send carriage return to the terminal */
		serial_putchar('\r');
	}

	return ch;
}

void bprintf(const char *fmt, ...)
{
	va_list args;
	char buf[256] = {0};
	int i;

	va_start(args, fmt);
	/* FIXME: vsprintf should know the size of buffer */
	vsprintf(buf, fmt, args);
	va_end(args);

	for (i = 0; i < (int)strlen(buf); i++)
	{
		serial_putchar(buf[i]);
	}
}

void BootAGPBUSInitialization(void)
{
	u32 temp;
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x54,   PciReadDword(BUS_0, DEV_1, FUNC_0, 0x54) | 0x88000000 );

	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x64,   (PciReadDword(BUS_0, DEV_0, FUNC_0, 0x64))| 0x88000000 );

	temp =  PciReadDword(BUS_0, DEV_0, FUNC_0, 0x6C);
	IoOutputDword(0xcfc , temp & 0xFFFFFFFE);
	IoOutputDword(0xcfc , temp );

	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x80, 0x00000100);

}

void BootDetectMemorySize(void)
{
	int result;
	unsigned char *fillstring;
	void *membasetop = (void*)((64*1024*1024));
	void *membaselow = (void*)((0));
	(void)result;

	(*(unsigned int*)(0xFD000000 + 0x100200)) = 0x03070103 ;
	(*(unsigned int*)(0xFD000000 + 0x100204)) = 0x11448000 ;

        PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x84, 0x7FFFFFF);  // 128 MB

	xbox_ram = 64;
	fillstring = malloc(0x200);
	memset(fillstring,0xAA,0x200);
	memset(membasetop,0xAA,0x200);
	asm volatile ("wbinvd\n");

	if (!memcmp(membasetop,fillstring,0x200)) {
		// Looks like there is memory .. maybe a 128MB box
		memset(fillstring,0x55,0x200);
		memset(membasetop,0x55,0x200);
		asm volatile ("wbinvd\n");
		if (!memcmp(membasetop,fillstring,0x200)) {
			// Looks like there is memory
			// now we are sure, we set memory
                        if (memcmp(membaselow,fillstring,0x200) == 0) {
                             	// Hell, we find the Test-string at 0x0 too !
                             	xbox_ram = 64;
                        } else {
                        	xbox_ram = 128;
                        }
		}

	}
	if (xbox_ram == 64) {
		PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x84, 0x3FFFFFF);  // 64 MB
	}
	else if (xbox_ram == 128) {
		PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x84, 0x7FFFFFF);  // 128 MB
	}
        free(fillstring);
}

void BootPciPeripheralInitialization(void)
{
	u32 val;

	__asm__ __volatile__ ( "cli" );

	/* LPC 0x80: disable ROM shadow mapping */
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x80, 0x00000002);
	if (PciReadByte(BUS_0, DEV_1, FUNC_0, 0x08) >= 0xd1) {
		PciWriteDword(BUS_0, DEV_1, FUNC_0, 0xc8, 0x8f00);
	}

	/* Port 0x61 (NMI mask), 0x92 (Fast A20), 0xCF9 (Reset control) */
	IoOutputByte(0x61, 0xff);
	IoOutputByte(0x92, 0x01);
	IoOutputByte(0xcf9, 0x00);

	/* LPC bridge chipset configuration */
	PciWriteByte(BUS_0, DEV_1, FUNC_0, 0x6a, 0x03);
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x64, 0x00000b0c);
	val = PciReadByte(BUS_0, DEV_1, FUNC_0, 0x81);
	PciWriteByte(BUS_0, DEV_1, FUNC_0, 0x81, (u8)(val | 0x08));

	/* LPC 0x8C: enable audio and IDE routing bits */
	val = PciReadDword(BUS_0, DEV_1, FUNC_0, 0x8c);
	val &= 0xf3ffffff;
	val |= 0x08000000;
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x8c, val);

	/* LPC 0x4C: enable RTC clock and IRQ 8 routing */
	PciWriteDword(BUS_0, DEV_1, FUNC_0, 0x4c, 0x000f0000);

	/* Setup Real Time Clock: 24-hr mode, 32kHz base / 1024Hz periodic rate */
	CMOS_WRITE(RTC_CONTROL_DEFAULT, RTC_CONTROL);
	CMOS_WRITE(RTC_FREQ_SELECT_DEFAULT, RTC_FREQ_SELECT);
	(void) CMOS_READ(RTC_INTR_FLAGS);

	/* IDE timing registers */
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x58, 0x20202020);
	PciWriteDword(BUS_0, DEV_9, FUNC_0, 0x60, 0xc0c0c0c0);

	/* USB0 and USB1 port enables */
	PciWriteDword(BUS_0, DEV_2, FUNC_0, 0x50, 0x0000000f);
	PciWriteDword(BUS_0, DEV_3, FUNC_0, 0x50, 0x00000030);

	/* AC97 / ACI audio registers */
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x44, 0x00020001);
	val = PciReadDword(BUS_0, DEV_6, FUNC_0, 0x4c);
	PciWriteDword(BUS_0, DEV_6, FUNC_0, 0x4c, val | 0x01010000);

	/* Host bridge 0:0.0 reg 0x48, 0x44, and memory limit */
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x48, 0x00000114);
	PciWriteDword(BUS_0, DEV_0, FUNC_0, 0x44, 0x80000000);
	PciWriteByte(BUS_0, DEV_0, FUNC_0, 0x87, 0x03);

	/* NV2A GPU reg 0x4C */
	PciWriteDword(BUS_1, DEV_0, FUNC_0, 0x4c, 0x00000114);

	/* ACPI hardware and interrupt routing (base 0x8000) */
	IoOutputByte(0x80ce, 0x08);                                /* RI# pin */
	IoOutputByte(0x80c0, 0x08);                                /* SMBUSC pin */
	IoOutputByte(0x8004, (u8)(IoInputByte(0x8004) | 0x01));    /* SCI enable */
	IoOutputWord(0x8022, (u16)(IoInputByte(0x8022) | 0x02));   /* Interrupt enable */
	IoOutputWord(0x8023, (u16)(IoInputByte(0x8023) | 0x02));
	IoOutputByte(0x8002, (u8)(IoInputByte(0x8002) | 0x01));    /* Timer status interrupt */
	IoOutputWord(0x8028, (u16)(IoInputByte(0x8028) | 0x01));
	IoOutputDword(0x80b4, 0x0000ffff);                          /* Reset ACPI inactivity timer */
	IoOutputByte(0x80cc, 0x08);                                /* EXTSMI# pin */
	IoOutputByte(0x80cd, 0x08);                                /* PRDY pin */
	IoOutputByte(0x80cf, 0x08);                                /* C32KHZ pin */
	IoOutputWord(0x8020, (u16)(IoInputWord(0x8020) | 0x0200)); /* Ack preceding ACPI interrupt */

	/* SMC initialization: allow audio (unmute) only; no SMC interrupt enable */
	HalpXboxSmBusWriteByte(0x10, 0x0b, 0x00);

	/* Super I/O COM1 initialization */
	LpcEnterConfiguration();
	if (!LpcGetSerialState()) {
		LpcSetSerialState(1);
		LpcSetSerialIRQState(1);
	}
	LpcExitConfiguration();
}
