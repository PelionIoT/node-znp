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
 *  ======== znp_mngt.h ========
 */

#ifndef ZNPMNGT_H
#define ZNPMNGT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#include "mtAf.h"
#include "mtZdo.h"

//*****************************************************************************
// typedefs
//*****************************************************************************

//! \brief Endpoint information record entry
//!
typedef struct
{
    uint8_t IEEEAddr[8];
    uint16_t srcAddr;
    uint16_t nwkAddr; // Network address
    uint8_t endpoint; // Endpoint identifier
    uint16_t profileID; // Profile identifier
    uint16_t deviceID; // Device identifier
    uint8_t version; // Version
    char* deviceName;
    uint8_t status;
    uint8_t flags;
    uint8_t numInClusters;
    uint16_t inClusterList[16];
    uint8_t numOutClusters;
    uint16_t outClusterList[16];
} epInfo_t;

#define MAX_CHILDREN 20
#define MAX_NODE_LIST 100

typedef struct
{
    uint16_t ChildAddr;
    uint8_t Type;
    uint8_t Lqi;
    uint8_t ExtendedAddress[8];
} ChildNode_t;

typedef struct
{
    uint16_t NodeAddr;
    uint8_t Type;
    uint8_t ChildCount;
    ChildNode_t childs[MAX_CHILDREN];
} Node_t;

//! \brief simple descriptor (new devie) callback typedef
//!
typedef uint8_t (*zMngt_zdoSimpleDescRspCb_t)(epInfo_t *epInfo);

//! \brief device leave callback typedef
//!
typedef uint8_t (*zMngt_zdoLeaveIndCb_t)(uint16_t nwkAddr);

//! \brief get dev info callback typedef
//!
typedef uint8_t (*zMngt_utilGetDevInfoRspcb_t)(uint8_t status, uint16_t nwkAddr,
        uint8_t ieeeAddr[8], uint8_t devType, uint8_t devState);

//! \brief ZDO and Util (control plain) callbacks
//!
typedef struct
{
    zMngt_zdoSimpleDescRspCb_t pfnZdoSimpleDescRspCb; // ZCL callback for simple desc response
    zMngt_zdoLeaveIndCb_t pfnZdoLeaveIndCb; // ZCL callback for simple desc response
    zMngt_utilGetDevInfoRspcb_t pfnUtilGetDevInfoRsp;
} zMngt_callbacks_t;

//*****************************************************************************
// the function prototypes
//*****************************************************************************

//! \brief          Initialises the ZNP management module
//! \param          none
//! \return        	status:
//                      SUCCESS
//                      FAILURE
extern uint_least8_t zMngt_init(void);

//! \brief          module management API: register application callbacks
//! \param[in]      zMngtCallbacks: callback table
//! \return        	none
extern void zMngt_registerCallbacks(zMngt_callbacks_t zMngtCallbacks);

//! \brief          Starts or joins network
//! \param[in]      devType: Deviace type (enum)
//!                     DEVICETYPE_COORDINATOR
//!                     DEVICETYPE_ROUTER
//!                     DEVICETYPE_ENDDEVICE
//! \param[in]      Chan: Channel Mask (bits 11-25 represent channel scanned)
//! \return        	status:
//                      SUCCESS
//                      FAILURE
extern uint_least8_t zMngt_start(uint_least8_t devType, uint_least32_t chan, uint8_t newNwk, uint16_t panId);

extern //! \brief          Open the network
//
// \param[in]       duration: time to open the network for (0 to close)
//! \return        	status:
//                      SUCCESS
//                      FAILURE
uint_least8_t zMngt_openNetwork(uint_least8_t duration);

//! \brief         Instruction ZNP on the next boot to clear network params
//!                and start a new network
//!
//! \param[in]      reset: True - reset ZNP for nwk clear to take effect
//!                        False - wait for next rest for nwk clear to take effect
//! \return         status:
//                      SUCCESS
//                      FAILURE
uint_least8_t zMngt_clearNetwork(uint_least8_t reset);

//! \brief         Instruction ZNP on the next boot to restore network params
//!
//! \return         status:
//                      SUCCESS
//                      FAILURE
uint_least8_t zMngt_restoreNetwork(void);

//! \brief         MT_ZDO_TC_END_DEVICE_ANNCE_IND 
//!
//! \return         status:
//                      SUCCESS
//                      FAILURE
uint_least8_t zMngt_endDeviceAnnouncement(EndDeviceAnnceIndFormat_t*);

//! \brief          Find device(s) with matching cluster
//
// \param[in]       ClusterId: Cluster to find matching devices for
// \param[in]       dstAddrTbl: Table to add matching devices to
// \param[in]       numMatches: Maximum devices to find
// \param[in]       timeout_ms: Multiple device could respopnd
//! \return        	status:
//                      SUCCESS
//                      FAILURE
extern uint_least8_t zMngt_findInMatch(uint_least16_t profileId,
        uint_least16_t clusterId, afAddrType_t dstAddrTbl[],
        uint_least8_t maxMatches);

void* zMngt_getNVItem(uint16_t);
uint8_t zMngt_setNVItem(uint16_t, uint8_t, uint8_t *);

void printNodeTopology(uint8_t);
void resetNodeTopology();

#ifdef __cplusplus
}
#endif

#endif /* ZNPMNGT_H */
