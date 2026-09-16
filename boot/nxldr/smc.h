/*
 * Xbox System Management Controller (SMC / PIC) interface for boot/nxldr.
 */

#ifndef NXLDR_SMC_H
#define NXLDR_SMC_H

#include "nxldr.h"

/*
 * Handle the Xbox SMC / PIC challenge-response handshake required
 * by original Xbox hardware to prevent the SMC watchdog from resetting.
 */
VOID xbox_smc_challenge_response(VOID);

#endif /* NXLDR_SMC_H */
