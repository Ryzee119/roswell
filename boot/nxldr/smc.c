/*
 * boot/nxldr/smc.c -- Xbox System Management Controller (SMC / PIC) driver.
 *
 * Implements SMBus communication with the SMC PIC (0x10) for:
 *   - PIC challenge-response handshake to prevent hardware watchdog reset
 *   - Front-panel LED color / sequence control
 *
 * Reference: Ryzee119 XboxDoomBIOS lib/xbox/2bboot.c
 */

#include "smc.h"

#define SMBUS_IO_BASE 0xC000
#define SMBUS_STATUS (SMBUS_IO_BASE + 0x00)
#define SMBUS_CONTROL (SMBUS_IO_BASE + 0x02)
#define SMBUS_ADDRESS (SMBUS_IO_BASE + 0x04)
#define SMBUS_DATA (SMBUS_IO_BASE + 0x06)
#define SMBUS_COMMAND (SMBUS_IO_BASE + 0x08)

#define SMBUS_STATUS_BUSY 0x08
#define SMBUS_STATUS_COLLISION 0x02
#define SMBUS_CONTROL_START_BYTE 0x0A /* BYTE (0x02) | START (0x08) */

#define XBOX_SMBUS_ADDR_SMC 0x20

static __inline UCHAR
smc_inb(USHORT port)
{
    UCHAR val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static __inline USHORT
smc_inw(USHORT port)
{
    USHORT val;
    __asm__ __volatile__("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static __inline VOID
smc_outb(USHORT port, UCHAR val)
{
    __asm__ __volatile__("outb %0, %1" ::"a"(val), "Nd"(port));
}

static __inline VOID
smc_outw(USHORT port, USHORT val)
{
    __asm__ __volatile__("outw %0, %1" ::"a"(val), "Nd"(port));
}

static BOOLEAN
smc_io(UCHAR command, UCHAR read, PUCHAR data)
{
    ULONG spin, retry;

    for (spin = 0; spin < 0x20000; spin++)
    {
        if (!(smc_inb(SMBUS_STATUS) & SMBUS_STATUS_BUSY))
            break;
    }

    for (retry = 0; retry < 10; retry++)
    {
        smc_outb(SMBUS_ADDRESS, (UCHAR)(XBOX_SMBUS_ADDR_SMC | (read ? 1 : 0)));
        smc_outb(SMBUS_COMMAND, command);
        smc_outw(SMBUS_STATUS, 0xFFFF);

        if (!read)
        {
            smc_outb(SMBUS_DATA, *data);
        }

        smc_outb(SMBUS_CONTROL, SMBUS_CONTROL_START_BYTE);

        for (spin = 0; spin < 0x20000; spin++)
        {
            if (!(smc_inb(SMBUS_STATUS) & SMBUS_STATUS_BUSY))
                break;
        }

        if (smc_inw(SMBUS_STATUS) & SMBUS_STATUS_COLLISION)
            continue;

        if (read)
        {
            *data = smc_inb(SMBUS_DATA);
        }
        return TRUE;
    }

    return FALSE;
}

VOID
xbox_smc_challenge_response(VOID)
{
    UCHAR bC = 0, bD = 0, bE = 0, bF = 0;

    smc_io(0x1C, 1, &bC);
    smc_io(0x1D, 1, &bD);
    smc_io(0x1E, 1, &bE);
    smc_io(0x1F, 1, &bF);

    UCHAR b1 = 0x33;
    UCHAR b2 = 0xED;
    UCHAR b3 = (UCHAR)((bC << 2) ^ (bD + 0x39) ^ (bE >> 2) ^ (bF + 0x63));
    UCHAR b4 = (UCHAR)((bC + 0x0B) ^ (bD >> 2) ^ (bE + 0x1B));
    UCHAR n = 4;

    while (n--)
    {
        b1 = (UCHAR)(b1 + (b2 ^ b3));
        b2 = (UCHAR)(b2 + (b1 ^ b4));
    }

    UCHAR w1 = b1;
    UCHAR w2 = b2;
    smc_io(0x20, 0, &w1);
    smc_io(0x21, 0, &w2);
}
