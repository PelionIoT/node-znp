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
 *  ======== zcl_port.h ========
 */
#ifndef ZCL_PORT_H
#define ZCL_PORT_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/

#include <stdint.h>
#include <stdbool.h>
//#include <xdc\std.h>
#include <stdlib.h>
#include <string.h>

#include "mtAf.h"
#include "AF.h"
#include "mtParser.h"

/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/

/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

#define CONST const
//#define true 1
//#define false 0
#define TRUE true
#define FALSE false

#define ZMemError 							afStatus_MEM_FAIL
#define ZSuccess							afStatus_SUCCESS
#define ZFailure							afStatus_FAILED
#define ZInvalidParameter					afStatus_INVALID_PARAMETER

/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

//! \brief We are using zcl in HA profile, so it is SECURE
//!
#define ZG_SECURE_ENABLED					TRUE

//! \brief Cluster Options flags
//!
#define AF_WILDCARD_PROFILEID              0x02   // Will force the message to use Wildcard ProfileID
#define AF_PREPROCESS                      0x04   // Will force APS to callback to preprocess before calling NWK layer
#define AF_LIMIT_CONCENTRATOR              0x08
#define AF_ACK_REQUEST                     0x10
#define AF_SUPRESS_ROUTE_DISC_NETWORK      0x20   // Supress Route Discovery for intermediate routes
// (route discovery preformed for initiating device)
#define AF_EN_SECURITY                     0x40
#define AF_SKIP_ROUTING                    0x80

//! \brief Backwards support for afAddOrSendMessage / afFillAndSendMessage.
//!
#define AF_TX_OPTIONS_NONE                 0
#define AF_MSG_ACK_REQUEST                 AF_ACK_REQUEST

//! \brief Needed for ZCL_DATATYPE_128_BIT_SEC_KEY, but not used in HA profile.
//!
#define SEC_KEY_LEN  16  // 128/8 octets (128-bit key is standard for ZigBee)

/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/

//! \brief types used by ZCL
//!
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int8_t int8;
typedef uint16_t int16;
typedef uint32_t int32;
typedef uint32_t ZStatus_t;
typedef uint8_t afStatus_t;

//! \brief Simple Description Format Structure
//!
typedef struct
{
    uint8_t EndPoint;
    uint16_t AppProfId;
    uint16_t AppDeviceId;
    uint8_t AppDevVer:4;
    uint8_t Reserved:4;             // AF_V1_SUPPORT uses for AppFlags:4.
    uint8_t AppNumInClusters;
    cId_t *pAppInClusterList;
    uint8_t AppNumOutClusters;
    cId_t *pAppOutClusterList;
}SimpleDescriptionFormat_t;

//! \brief Endpoint descriptor used for registering endpoints
typedef struct
{
    uint8_t endPoint;
    uint8_t *task_id;  // Pointer to location of the Application task ID.
    SimpleDescriptionFormat_t *simpleDesc;
    AF_NETWORK_LATENCY latencyReq;
}endPointDesc_t;

//! \brief osal_event_hdr_t is used in typedef for
//! zclIncomingMsg_t, defined in zcl.h. This should
//! only be compiled in when ZCL_STANDALONE in not
//! define. Define osal_event_hdr_t here so current
//! ZCL will compile.
#define osal_event_hdr_t uint8_t

//! \brief Find EP
//! \param[in]      EndPoint - Ep ID to find
//! \return         Endpoint
endPointDesc_t *afFindEndPointDesc( uint8 EndPoint );

//! \brief Common functionality for invoking APSDE_DataReq() for both
//! SendMulti and MSG-Send.
//! \param[in]      endpoint - ep Id that the scene belongs to
//! \param[in]      *dstAddr - Full ZB destination address: Nwk Addr + End Point.
//! \param[in]      *srcEP - Origination (i.e. respond to or ack to) End Point Descr.
//! \param[in]      cID - A valid cluster ID as specified by the Profile.
//! \param[in]      bufLen - Number of bytes of data pointed to by next param.
//! \param[in]      *buf - A pointer to the data bytes to send.
//! \param[in]      *transID - A pointer to a byte which can be modified and which will
//!                  be used as the transaction sequence number of the msg.
//! \param[in]      options - Valid bit mask of Tx options.
//! \param[in]      radius - Normally set to AF_DEFAULT_RADIUS.
//! \param[out]     *transID - Incremented by one if the return value is success.
//! \return         afStatus_t - See previous definition of afStatus_... types.
afStatus_t AF_DataRequest(afAddrType_t *dstAddr, endPointDesc_t *srcEP,
uint16 cID, uint16 bufLen, uint8 *buf, uint8 *transID, uint8 options,
uint8 radius);

#ifdef __cplusplus
}
#endif

#endif /* ZCL_PORT_H */
