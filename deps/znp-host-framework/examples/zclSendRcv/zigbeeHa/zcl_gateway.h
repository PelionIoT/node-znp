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
 *  ======== zcl_gateway.h ========
 */

#ifndef ZCL_SAMPLESW_H
#define ZCL_SAMPLESW_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "mtAf.h"
#include "zcl_port.h"
#include "zcl.h"

/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/
//! \brief get state (on/off) callback typedef
//!
typedef uint8_t (*zclGw_zclGetInfoCb_t)(uint8_t *manu, uint16_t nwkAddr,
        uint8_t endpoint, uint8_t zclTransId);

//! \brief get state (on/off) callback typedef
//!
typedef uint8_t (*zclGw_zclGetStateCb_t)(uint8_t state, uint16_t nwkAddr,
        uint8_t endpoint, uint8_t zclTransId);

//! \brief get level callback typedef
//!
typedef uint8_t (*zclGw_zclGetLevelCb_t)(uint8_t level, uint16_t nwkAddr,
        uint8_t endpoint, uint8_t zclTransId);

//! \brief get hue callback typedef
//!
typedef uint8_t (*zclGw_zclGetHueCb_t)(uint8_t hue, uint16_t nwkAddr,
        uint8_t endpoint, uint8_t zclTransId);

//! \brief get saturation callback typedef
//!
typedef uint8_t (*zclGw_zclGetSatCb_t)(uint8_t sat, uint16_t nwkAddr,
        uint8_t endpoint, uint8_t zclTransId);

//! \brief get dev model ID callback typedef
//!
typedef uint8_t (*zclGw_zclGetModelCb_t)(uint8_t *ModelId);

//! \brief get level callback typedef
//!
typedef uint8_t (*pfnZclGetSetPointCb_t)(int16_t setPoint, uint16_t nwkAddr,
        uint8_t endpoint, uint8_t zclTransId);

//! \brief zcl (data plain) callbacks
//!
typedef struct
{
    zclGw_zclGetInfoCb_t pfnZclGetInfoCb; // ZCL response callback for get State
    zclGw_zclGetStateCb_t pfnZclGetStateCb; // ZCL response callback for get State
    zclGw_zclGetLevelCb_t pfnZclGetLevelCb; // ZCL response callback for get Level
    zclGw_zclGetHueCb_t pfnZclGetHueCb; // ZCL response callback for get Hue
    zclGw_zclGetSatCb_t pfnZclGetSatCb; // ZCL response callback for get Sat
    zclGw_zclGetModelCb_t pfnZclGetModelCb; //
    pfnZclGetSetPointCb_t pfnZclGetSetPointCb;
} zclGw_callbacks_t;

/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

//! \brief Gateway Application Endpoint ID
//!
#define ZGW_EP                6

/**************************************************************************************************
 * GLOBALS
 **************************************************************************************************/

/*********************************************************************
 * FUNCTIONS
 */

//*****************************************************************************
// Device control helper functions
//*****************************************************************************

//! \brief          Device control API: Add a device to a group
//! \param[in]      groupId - ID of the group to be added to
//! \param[in]      dstAddr - nwk addr of device to be added
//! \param[in]      endpoint - end point on the device to be added to the group
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_addGroup(uint16_t groupId, uint16_t dstAddr, uint8_t endpoint,
        uint8_t addrMode);

//! \brief          Device control API: store a scene on a device
//! \param[in]      groupId - group ID of the scene to be stored
//! \param[in]      sceneId - scene ID of the scene to be stored
//! \param[in]      dstAddr - nwk addr of device to store the scene
//! \param[in]      endpoint - end point on the device to store the scene
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_storeScene(uint16_t groupId, uint8_t sceneId,
        uint16_t dstAddr, uint8_t endpoint, afAddrMode_t addrMode);

//! \brief          Device control API: recall a scene on a device
//! \param[in]      groupId - group ID of the scene to be recalled
//! \param[in]      sceneId - scene ID of the scene to be recalled
//! \param[in]      dstAddr - nwk addr of device to recall the scene
//! \param[in]      endpoint - end point on the device to recall the scene
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_recallScene(uint16_t groupId, uint8_t sceneId,
        uint16_t dstAddr, uint8_t endpoint, afAddrMode_t addrMode);

//! \brief          Device control API: binds a device output cluster (src) to
//                  another devices input cluster (dst)
//! \param[in]      srcNwkAddr - source devices nwk addr
//! \param[in]      srcEndpoint - source devices IEEE address
//! \param[in]      srcIEEE - nwk addr of device to store the scene
//! \param[in]      dstEndpoint - destination devices IEEE address
//! \param[in]      dstIEEE - destination addr of device to store the scene
//! \return        	status
uint_least8_t zGw_bind(uint16_t srcNwkAddr, uint8_t srcEndpoint,
        uint8_t srcIEEE[8], uint8_t dstEndpoint, uint8_t dstIEEE[8],
        uint16_t clusterID);

//! \brief          Device control API: sends an identify command to a device
//! \param[in]      identifyTime - time to identify for (10ms units)
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_sendIdentify(uint16_t identifyTime, uint16_t dstAddr,
        uint8_t endpoint, afAddrMode_t addrMode);

//! \brief          Device control API: sends an identify effect command to a device
//! \param[in]      effect - effect (see ZCL spec)
//! \param[in]      effect - effectVarient (see ZCL spec)
//! \param[in]      identifyTime - time to identify for (10ms units)
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_sendIdentifyEffect(uint8_t effect, uint8_t effectVarient,
        uint16_t dstAddr, uint8_t endpoint, afAddrMode_t addrMode);

//*****************************************************************************
// Device set API prototypes
//*****************************************************************************

//! \brief          Device set API: sends a set state command to a device
//! \param[in]      0:off, 1:on, 2:toggle
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setState(uint8_t state, uint16_t dstAddr, uint8_t endpoint,
        uint8_t addrMode);

//! \brief          Device set API: sends a set level command to a device
//! \param[in]      level - 0-255
//! \param[in]      time - transition time in 10th's of a second
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setLevel(uint8_t level, uint16_t time, uint16_t dstAddr,
        uint8_t endpoint, afAddrMode_t addrMode);

//! \brief          Device set API: sends a hue level command to a device
//! \param[in]      hue - 0-255
//! \param[in]      time - transition time in 10th's of a second
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setHue(uint8_t hue, uint16_t time, uint16_t dstAddr,
        uint8_t endpoint, afAddrMode_t addrMode);

//! \brief          Device set API: sends a set sturation command to a device
//! \param[in]      sat - saturation 0-255
//! \param[in]      time - transition time in 10th's of a second
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setSat(uint8_t sat, uint16_t time, uint16_t dstAddr,
        uint8_t endpoint, afAddrMode_t addrMode);

//! \brief          Device set API: sends a set hue and sturation command to a device
//! \param[in]      hue - 0-255
//! \param[in]      sat - saturation 0-255
//! \param[in]      time - transition time in 10th's of a second
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setHueSat(uint8_t hue, uint8_t sat, uint16_t time,
        uint16_t dstAddr, uint8_t endpoint, afAddrMode_t addrMode);

//! \brief          Device set setpoint API: sends a set level command to a device
//! \param[in]      setPointValue - 0-255
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_setSetPoint(int16_t setPointValue, uint16_t dstAddr,
        uint8_t endpoint, afAddrMode_t addrMode);

//*****************************************************************************
// Device get API prototypes
//*****************************************************************************

//! \brief          Device get API: sends a get info command to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_getInfo(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block);

//! \brief          Device get API: sends a get model ID command to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \return        	status
uint_least8_t zGw_getModel(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode);

//! \brief          Device get API: sends a get state command to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \param[in]      block - block on response, if set this function will return after
//!                 timeout or after response has been processed and call back returns
//! \return        	status
uint_least8_t zGw_getState(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block);

//! \brief          Device get API: sends a get level to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \param[in]      block - block on response, if set this function will return after
//!                 timeout or after response has been processed and call back returns
//! \return        	status
uint_least8_t zGw_getLevel(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block);

//! \brief          Device get API: sends a get hue to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \param[in]      block - block on response, if set this function will return after
//!                 timeout or after response has been processed and call back returns
//! \return        	status
uint_least8_t zGw_getHue(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block);

//! \brief          Device get API: sends a get saturation to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \param[in]      block - block on response, if set this function will return after
//!                 timeout or after response has been processed and call back returns
//! \return        	status
uint_least8_t zGw_getSat(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block);

//! \brief          Device get API: sends a get saturation to a device
//! \param[in]      dstAddr - nwk addr of device to send command to
//! \param[in]      endpoint - end point on the device to send command to
//! \param[in]      addrMode - address mode of dstAddr (afAddr16Bit or afAddrGroup)
//! \param[in]      block - block on response, if set this function will return after
//!                 timeout or after response has been processed and call back returns
//! \return        	status
uint_least8_t zGw_getSetPoint(uint16_t dstAddr, uint8_t endpoint,
        afAddrMode_t addrMode, uint8_t block);


//! \brief          Initialize MT_AF callbacks
//! \param          none
//! \return        	none
void zclGw_Init(void);

//! \brief          Initialize Zcl and registers application endpoint
//! \param          none
//! \return        	none
void zclGw_InitZcl(void);

//! \brief          Register zcl (data plain) callbacks
//! \param[in]      zclGwCallbacks: callback table
//! \return        	none
extern void zclGw_registerCallbacks(zclGw_callbacks_t zclGwCallbacks);

//! \brief          Process incoming ZCL commands specific to this profile
//! \param[in]      pInMsg: pointer to the incoming message
//! \return        	none
void zclGw_processInCmds(zclIncoming_t *pInMsg);

/*********************************************************************
 *********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZCL_SAMPLESW_H */
