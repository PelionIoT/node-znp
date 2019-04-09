/*
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== AF.h ========
 */
#ifndef __AF_H__
#define __AF_H__
// Default Radius Count value
#define AF_DEFAULT_RADIUS                  32

/**
 * Network Latency values, for now only use
 * NO_LATENCY_REQS
 */
typedef enum
{
  NO_LATENCY_REQS = 0,
  FAST_BEACONS = 1,
  SLOW_BEACONDS = 2,
} AF_NETWORK_LATENCY;


#define NWK_BROADCAST_SHORTADDR_DEVZCZR 		   0xFFFC //Routers and Coordinators
#define NWK_BROADCAST_SHORTADDR_DEVRXON            0xFFFD //Everyone with RxOnWhenIdle == TRUE
#define NWK_BROADCAST_SHORTADDR_DEVALL             0xFFFF

#define AF_BROADCAST_ENDPOINT              0xFF

/* Inlude zcl_port.h, which is the ZCL_STANDALONE equivalent */
#include "zcl_port.h"

#endif //__AF_H__
