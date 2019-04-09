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
 *  ======== zcl_port.c ========
 */

/*********************************************************************
 * INCLUDES
 */
#include "stdio.h"

#include "zcl_port.h"
#include "zcl.h"
#include "zcl_general.h"

#include "dbgPrint.h"

#include "zcl_gateway.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */
extern endPointDesc_t zswEpDesc;

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
#if defined(ZCL_GROUPS)
static aps_Group_t foundGrp;
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * API Functions
 *********************************************************************/

//! \brief Function for processing commands not handled by ZCL
//! \param[in]      pInMsg - incoming message to process
//! \return         true
uint8 zcl_HandleExternal(zclIncoming_t *pInMsg)
{
    /* send message through task message */
    zclGw_processInCmds(pInMsg);

    return true;
}

//! \brief Abstraction function to allocate memory
//! \param[in]      size - size in bytes needed
//! \return         pointer to allocated buffer, NULL if nothing allocated.
void *zcl_mem_alloc( uint16 size)
{
    return ((void *) malloc(size));
}

//! \brief Abstraction function to set memory
//! \param[in]      dest - pointer to destination buffer
//! \param[in]      value - value to set
//! \param[in]      len - lenght to set in bytes
//! \return         status
void *zcl_memset(void *dest, uint8 value, int len)
{
    return ((void *) memset(dest, value, len));
}

//! \brief Abstraction function to copy memory
//! \param[in]      dest - pointer to destination buffer
//! \param[in]      src - pointer to source buffer
//! \param[in]      len - lenght to set in bytes
//! \return         status
void *zcl_memcpy(void *dst, void *src, unsigned int len)
{
    return ((void *) memcpy(dst, src, len));
}

//! \brief Abstraction function to copy the 64b IEEE Addr
//! \param[in]      dest - pointer to destination buffer
//! \param[in]      src - pointer to source buffer
//! \return         status
extern void *zcl_cpyExtAddr( uint8 * pDest, const uint8 * pSrc)
{
    return memcpy(pDest, pSrc, 8);
}

//! \brief Abstraction function to free memory
//! \param[in]      ptr - pointer to buffer to be freed
//! \return         none
void zcl_mem_free(void *ptr)
{
    if (ptr != NULL)
    {
        free(ptr);
    } else
    {
        printf("zcl_mem_free: NULL ptr\n");
    }
}

//! \brief function to to convert 32b value
//! to 4 byte array
//! \param[out]      buf - pointer to 4 byte array
//! \param[in]      val - 32b value to copy
//! \return         buf
uint8* zcl_buffer_uint32( uint8 *buf, uint32 val)
{
    *buf++ = BREAK_UINT32(val, 0);
    *buf++ = BREAK_UINT32(val, 1);
    *buf++ = BREAK_UINT32(val, 2);
    *buf++ = BREAK_UINT32(val, 3);

    return buf;
}

//! \brief function to swap bytes in a buffer
//! to 4 byte array
//! \param[in/out]  swapped - pointer to upto 4 byte array
//! \param[in]      len - length of array
//! \return         swapped
uint32 zcl_build_uint32( uint8 *swapped, uint8 len)
{
    if (len == 2)
        return (BUILD_UINT32(swapped[0], swapped[1], 0L, 0L));
    else if (len == 3)
        return (BUILD_UINT32(swapped[0], swapped[1], swapped[2], 0L));
    else if (len == 4)
        return (BUILD_UINT32(swapped[0], swapped[1], swapped[2], swapped[3]));
    else
        return ((uint32) swapped[0]);
}

//! \brief function to initialise an NV item (not used in this port)
//! \param[in]      id - NV Item ID
//! \param[in]      len - length of item
//! \param[in]      buf - pointer to item
//! \return         status
uint8 zcl_nv_item_init( uint16 id, uint16 len, void *buf)
{
    return (0);
}

//! \brief function to write an NV item (not used in this port)
//! \param[in]      id - NV Item ID
//! \param[in]      ndx - index (offset) into item
//! \param[in]      len - length of item
//! \param[in]      buf - pointer to item
//! \return         status
uint8 zcl_nv_write( uint16 id, uint16 ndx, uint16 len, void *buf)
{
    return (0);
}

//! \brief function to read an NV item (not used in this port)
//! \param[in]      id - NV Item ID
//! \param[in]      ndx - index (offset) into item
//! \param[in]      len - length of item
//! \param[in]      buf - pointer to item
//! \return         status
uint8 zcl_nv_read( uint16 id, uint16 ndx, uint16 len, void *buf)
{
    return (0);
}

//! \brief Find EP
//! \param[in]      EndPoint - Ep ID to find
//! \return         Endpoint
endPointDesc_t *afFindEndPointDesc( uint8 EndPoint)
{
    if (EndPoint == zswEpDesc.endPoint)
    {
        return (&zswEpDesc);
    } else
    {
        return ((endPointDesc_t *) NULL);
    }
}

//! \brief function to initialising the scene table
//! (not used in this port)
//! \param          none
//! \return         none
void zclGeneral_ScenesInit(void)
{
}

//! \brief Abstraction function to save a scene
//! (not used in this port)
//! \param          none
//! \return         none
void zclGeneral_ScenesSave(void)
{
}

//! \brief function to get the scene count
//! (not used in this port)
//! \param          none
//! \return         count
uint8 zclGeneral_CountAllScenes(void)
{
    return (0);
}

//! \brief function to find a scene
//! (not used in this port)
//! \param[in]      endpoint - ep Id that the scene belongs to
//! \param[in]      groupID - group Id that the scene belongs to
//! \param[in]      sceneList - pointer to a scene list
//! \return         sceneId
uint8 zclGeneral_FindAllScenesForGroup( uint8 endpoint, uint16 groupID,
uint8 *sceneList)
{
    return (0);
}

//! \brief function to remove all a scene
//! (not used in this port)
//! \param[in]      endpoint - ep Id that the scene belongs to
//! \param[in]      groupID - group Id that the scene belongs to
//! \return         none
void zclGeneral_RemoveAllScenes( uint8 endpoint, uint16 groupID)
{
}

//! \brief function to remove a scene
//! (not used in this port)
//! \param[in]      endpoint - ep Id that the scene belongs to
//! \param[in]      groupID - group Id that the scene belongs to
//! \param[in]      sceneID - scene Id to remove
//! \return         status
uint8 zclGeneral_RemoveScene( uint8 endpoint, uint16 groupID, uint8 sceneID)
{
    return (TRUE);
}

//! \brief function to find a scene
//! (not used in this port)
//! \param[in]      endpoint - ep Id that the scene belongs to
//! \param[in]      groupID - group Id that the scene belongs to
//! \param[in]      sceneID - scene Id to find
//! \return         scene
zclGeneral_Scene_t *zclGeneral_FindScene( uint8 endpoint, uint16 groupID,
uint8 sceneID)
{
    return ((zclGeneral_Scene_t *) NULL);
}

//! \brief function to add a scene
//! (not used in this port)
//! \param[in]      endpoint - ep Id that the scene belongs to
//! \param[in]      scene - pointer to the scene to add
//! \return         status
ZStatus_t zclGeneral_AddScene( uint8 endpoint, zclGeneral_Scene_t *scene)
{
    return (0);
}

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
uint8 radius)
{

    afStatus_t status;
    DataRequestExtFormat_t req;

    req.DstAddrMode = dstAddr->addrMode;
    if (req.DstAddrMode == Addr64Bit)
    {
        memcpy(req.DstAddr, dstAddr->addr.extAddr, 8);
    } else
    {
        memcpy(req.DstAddr, &dstAddr->addr.shortAddr, 2);
    }
    req.DstEndpoint = dstAddr->endPoint;
    req.SrcEndpoint = srcEP->endPoint;
    req.ClusterId = cID;
    req.TransId = (*transID)++;
    // printf("send req transid: %d\n", req.TransId);
    req.Options = 0;
    req.Radius = AF_DEFAULT_RADIUS;
    memcpy(req.Data, buf, bufLen);
    req.Len = bufLen;

    status = afDataRequestExt(&req);

    //dbg_print(PRINT_LEVEL_ERROR, "zcl_port: sending afDataRequest, addr:%x, status:%x\n", dstAddr->addr.shortAddr, status);
    return (status);
}

#if defined(ZCL_GROUPS)
/**************************************************************************************************
 * APS Interface messages
 **************************************************************************************************/

//! \brief function to remove a group (not used)
//! (not used in this port)
//! \param[in]      endpoint - ep Id that the group belongs to
//! \param[in]      groupID - group ID to be removed
//! \return         status
uint8 aps_RemoveGroup( uint8 endpoint, uint16 groupID )
{
    return (TRUE);
}

//! \brief function to remove all groups (not used)
//! (not used in this port)
//! \param[in]      endpoint - ep Id that the groups belong to
//! \return         none
void aps_RemoveAllGroup( uint8 endpoint )
{
}

//! \brief function to find group (not used)
//! (not used in this port)
//! \param[in]      endpoint - ep Id that the group belongs to
//! \param[in]      groupList - groupList
//! \return         status
uint8 aps_FindAllGroupsForEndpoint( uint8 endpoint, uint16 *groupList )
{
    return ( 0 );
}

//! \brief function to find group (not used)
//! (not used in this port)
//! \param[in]      endpoint - ep Id that the group belongs to
//! \param[in]      groupID - group Id to find
//! \return         group
aps_Group_t *aps_FindGroup( uint8 endpoint, uint16 groupID )
{
    return ( 0 );
}

//! \brief function to add group (not used)
//! (not used in this port)
//! \param[in]      endpoint - ep Id that the group belongs to
//! \param[in]      group
//! \return         status
ZStatus_t aps_AddGroup( uint8 endpoint, aps_Group_t *group )
{
    return (0);
}

//! \brief function to return group count (not used)
//! (not used in this port)
//! \param          none
//! \return         status
uint8 aps_CountAllGroups( void )
{
    return (0);
}
#endif // defined(ZCL_GROUPS)
/*********************************************************************
 *********************************************************************/

