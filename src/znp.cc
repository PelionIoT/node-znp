/*    
    Copyright (c) 2018, Arm Limited and affiliates.
    
    SPDX-License-Identifier: Apache-2.0
    
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    
        http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*
* Copyright (c) 2015 WigWag Inc.
*/


#include <unistd.h>
#include <pthread.h>
#include <list>
#include <queue>
#include <iostream>

#include <node.h>
#include <v8.h>
#include <nan.h>

#include "zclSendRcv.h"
#include "rpc.h"
#include "dbgPrint.h"
#include "znp_node.h"
#include "znp_cfuncs.h"
#include "zcl_gateway.h"
#include "zcl.h"

using namespace v8;	

__thread ZNP *myZnp = NULL;
uv_async_t v8async;
uv_async_t znpasync;
uv_mutex_t _control;
uv_cond_t _start_cond;
uv_thread_t znp_thread;
bool threadUp = false;

// typedef struct {
// 	work_code code;
// 	int _errno;
// 	void *data;
// 	int size;
// } workReq;

/*
 * Message passing queue from v8 and znp.
 */
static pthread_mutex_t workqueue_mutex = PTHREAD_MUTEX_INITIALIZER;
static std::queue<ZNP::zclTransport *> workqueue;

enum event_code {
	NETWORK_UP,
	NETWORK_DOWN,
	DISCOVERED,
	ZCL_COMMAND_RESPONSE,
	ZCL_ATTR_RESPONSE,
	NETWORK_TOPOLOGY,
	ONLINE_DEVICE
};

typedef struct {
	event_code code;
	int _errno;
	void *data;
	int size;
} eventReq;

/*
 * Message passing queue from znp to v8.
 */
static pthread_mutex_t eventqueue_mutex = PTHREAD_MUTEX_INITIALIZER;
static std::queue<eventReq *> eventqueue;

//Pthread cond for critical failure
static pthread_cond_t cond_critical_failure = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t critical_failure_mutex = PTHREAD_MUTEX_INITIALIZER;

//*********************************************************************************************************************

/*
 * Async handler, triggered by the ZNP callback.
 */
void v8async_cb_handler(uv_async_t *handle, int status)
{
	Nan::HandleScope scope;

	eventReq *req;
	Local<Value> args[16];

	ZNP *zb = (ZNP *)handle->data;

	pthread_mutex_lock(&eventqueue_mutex);

	while (!eventqueue.empty())
	{
		req = eventqueue.front();

		switch (req->code) {
			case NETWORK_UP:
			{
				if(zb->onNetworkReadyCB) {
					zb->onNetworkReadyCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
				}
				break;
			}

			case NETWORK_DOWN: 
			{
				if(zb->onNetworkFailedCB) {
					zb->onNetworkFailedCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
				}
				break;
			}

			case DISCOVERED: 
			{
				epInfo_t *nodeInfo = (epInfo_t*)req->data;
				Local<Object> buf;
				v8::Local<v8::Object> info = Nan::New<v8::Object>();

				info->Set(Nan::New("srcAddr").ToLocalChecked(), Nan::New(nodeInfo->srcAddr));
				info->Set(Nan::New("nwkAddr").ToLocalChecked(), Nan::New(nodeInfo->nwkAddr));
				info->Set(Nan::New("endpoint").ToLocalChecked(), Nan::New(nodeInfo->endpoint));
				info->Set(Nan::New("profileID").ToLocalChecked(), Nan::New(nodeInfo->profileID));
				info->Set(Nan::New("deviceID").ToLocalChecked(), Nan::New(nodeInfo->deviceID));
				info->Set(Nan::New("version").ToLocalChecked(), Nan::New(nodeInfo->version));
				info->Set(Nan::New("status").ToLocalChecked(), Nan::New(nodeInfo->status));
				info->Set(Nan::New("flags").ToLocalChecked(), Nan::New(nodeInfo->flags));
				info->Set(Nan::New("numOutClusters").ToLocalChecked(), Nan::New(nodeInfo->numOutClusters));
				info->Set(Nan::New("numInClusters").ToLocalChecked(), Nan::New(nodeInfo->numInClusters));


				// printf("\tsrcAddr: %d\n",			nodeInfo->srcAddr		);
				// printf("\tnwkAddr: %d\n",			nodeInfo->nwkAddr		);

				args[0] = info;
				toBuffer(buf, nodeInfo->IEEEAddr, sizeof(nodeInfo->IEEEAddr));
				args[1] = buf->ToObject();
				toBuffer(buf, nodeInfo->inClusterList, nodeInfo->numInClusters * sizeof(uint16_t));
				args[2] = buf->ToObject();
				toBuffer(buf, nodeInfo->outClusterList, nodeInfo->numOutClusters * sizeof(uint16_t));
				args[3] = buf->ToObject();
				if(zb->onNodeDiscoveredCB) {
					zb->onNodeDiscoveredCB->Call(Nan::GetCurrentContext()->Global(), 4, args);
				}
				break;
			}

			case ZCL_COMMAND_RESPONSE:
			{
				dbg_print(PRINT_LEVEL_VERBOSE, "GOT ZCL_COMMAND_RESPONSE\n");
				uint8_t status = *((uint8_t*)(req->data));
				// if(zb->waitForResponse) {
					args[0] = Nan::New(status);
					args[1] = Nan::New(zb->currentCmdSeqId);
					if(zb->onCmdResponseCB) {
			    		zb->onCmdResponseCB->Call(Nan::GetCurrentContext()->Global(), 2, args);
					}
			    	zb->waitForResponse = false;
				// }
				break;
			}

			case ZCL_ATTR_RESPONSE:
			{
				dbg_print(PRINT_LEVEL_VERBOSE, "GOT ZCL_ATTR_RESPONSE\n");
				attr_response *resp = (attr_response*)req->data;
				Local<Object> buf;
				v8::Local<v8::Object> info = Nan::New<v8::Object>();

					// printf("\tsrcAddr: %d\n",			resp->srcAddr		);
					// printf("\tendPoint: %d\n",			resp->endPoint		);
					// printf("\taddrMode: %d\n",			resp->addrMode		);
					// printf("\tclusterId: %d\n",			resp->clusterId		);
					// printf("\ttransId: %d\n",				resp->transId		);
					// printf("\tpayloadLen: %d\n",		resp->payloadLen	);
					// int i = 0;
					// printf("\tpayload: ");
					// for(i = 0; i < resp->payloadLen; i++) {
					// 	printf("%02x ", resp->payload[i]);
					// }
					// printf("\n");

					if(resp->payloadLen <= 255) {
						info->Set(Nan::New("srcAddr").ToLocalChecked(), Nan::New(resp->srcAddr));
						info->Set(Nan::New("endPoint").ToLocalChecked(), Nan::New(resp->endPoint));
						info->Set(Nan::New("addrMode").ToLocalChecked(), Nan::New(resp->addrMode));
						info->Set(Nan::New("transId").ToLocalChecked(), Nan::New(resp->transId));
						info->Set(Nan::New("clusterId").ToLocalChecked(), Nan::New(resp->clusterId));
						info->Set(Nan::New("payloadLen").ToLocalChecked(), Nan::New(resp->payloadLen));

						args[0] = info;
						toBuffer(buf, resp->payload, resp->payloadLen * sizeof(uint8_t));
						args[1] = buf->ToObject();
						args[2] = Nan::New(zb->currentCmdSeqId);
						if(zb->onAttrResponseCB) {
							zb->onAttrResponseCB->Call(Nan::GetCurrentContext()->Global(), 3, args);
						}
						zb->waitForResponse = false;
					} else {
						dbg_print(PRINT_LEVEL_ERROR, "ZCL_ATTR_RESPONSE got payload of len >255: %d\n", resp->payloadLen);
					}
				break;
			}

			case NETWORK_TOPOLOGY: 
			{
				Node_t *nodeLqi = (Node_t*)req->data;
				Local<Object> buf;

				toBuffer(buf, nodeLqi, (sizeof(uint16_t) + 2 * sizeof(uint8_t) + nodeLqi->ChildCount * sizeof(ChildNode_t)));
				args[0] = buf->ToObject();
				if(zb->onNetworkTopologyCB) {
					zb->onNetworkTopologyCB->Call(Nan::GetCurrentContext()->Global(), 1, args);
				}
				break;
			}

			case ONLINE_DEVICE: 
			{
				EndDeviceAnnceIndFormat_t *msg = (EndDeviceAnnceIndFormat_t*)req->data;
				v8::Local<v8::Object> info = Nan::New<v8::Object>();

				info->Set(Nan::New("srcAddr").ToLocalChecked(), Nan::New(msg->SrcAddr));
				info->Set(Nan::New("nwkAddr").ToLocalChecked(), Nan::New(msg->NwkAddr));
				// info->Set(Nan::New("ieeeAddr").ToLocalChecked(), Nan::New(msg->IEEEAddr));
				info->Set(Nan::New("capabilities").ToLocalChecked(), Nan::New(msg->Capabilities));

				args[0] = info;
				if(zb->onDeviceJoinedNetworkCB) {
					zb->onDeviceJoinedNetworkCB->Call(Nan::GetCurrentContext()->Global(), 1, args);
				}
				break;
			}

			default:
				dbg_print(PRINT_LEVEL_ERROR, "Unhandled Event Request: %d\n", req->code);
				break;

		}
		eventqueue.pop();
		delete req;
	}

	pthread_mutex_unlock(&eventqueue_mutex);
}

void submitToV8(event_code code, void *data, int size, int err)
{
	eventReq *req = new eventReq();

	req->code = code;
	req->data = data;
	req->size = size;
	req->_errno = err;

	pthread_mutex_lock(&eventqueue_mutex);
	eventqueue.push(req);
	pthread_mutex_unlock(&eventqueue_mutex);

	uv_async_send(&v8async);
}


//*********************************************************************************************************************
/*
 * Async handler, triggered by the v8 calls.
 */
void znpasync_cb_handler(uv_async_t *handle, int status)
{
	Nan::HandleScope scope;

	ZNP::zclTransport *req;
	Local<Value> args[16];

	ZNP *zb = (ZNP *)handle->data;

	pthread_mutex_lock(&workqueue_mutex);

	while (!workqueue.empty())
	{
		req = workqueue.front();

		switch(req->workCode) {

			case ZNP::ZCL_SEND_COMMAND:
			{
				ZNP::sendCmd_t *command = (ZNP::sendCmd_t*)req->command;

				afAddrType_t afDstAddr;
				int stat;

			    afDstAddr.addr.shortAddr = command->dstAddr;
			    afDstAddr.endPoint = command->endPoint;
			    afDstAddr.addrMode = command->addrMode;


	    		myZnp->waitForResponse = true;
	    		myZnp->currentCmdSeqId = command->seqNumber;

			    stat = zcl_SendCommand(command->srcEp, &afDstAddr, command->clusterId, command->cmdId, command->specific, 
			    	command->direction, command->disableDefaultRsp, command->manuCode, command->seqNumber, command->cmdFormatLen, (uint8_t*)command->cmdFormat);

				if(stat == 0x00) { //SUCCESS
		    		//wait for the request to resolve
		    	} else {
		    		myZnp->waitForResponse = false;
		    	}

				args[0] = Nan::New(stat);
				args[1] = Nan::New(command->msgId);
				args[2] = Nan::New(command->seqNumber);
				if(req->statusCB){
		    		req->statusCB->Call(Nan::GetCurrentContext()->Global(), 3, args);
				}

				delete req->command;
				break;
			}

			case ZNP::ZCL_READ_ATTR:
			{
				ZNP::readAttr_t *command = (ZNP::readAttr_t*)req->command;

				afAddrType_t afDstAddr;
    			zclReadCmd_t* readCmd;
    			int stat;

			    afDstAddr.addr.shortAddr = command->dstAddr;
			    afDstAddr.endPoint = command->endPoint;
			    afDstAddr.addrMode = command->addrMode;

    			readCmd = (zclReadCmd_t*)malloc(sizeof(zclReadCmd_t) + sizeof(uint16) * command->numAttr);

			    if (readCmd != NULL)
			    {
			        readCmd->numAttr = command->numAttr;
			        
			        int i = 0; 
			        for(i = 0; i < readCmd->numAttr; i++) {
			        	readCmd->attrID[i] = command->attrId[i];
			        }

		    		myZnp->waitForResponse = true;
		    		myZnp->currentCmdSeqId = command->seqNumber;

			        stat = zcl_SendRead( command->srcEp, &afDstAddr,
						        command->clusterId, readCmd,
						        command->direction, command->disableDefaultRsp, command->seqNumber);

			        free(readCmd);

			        // if(block)
			        // {
			        //     if (waitZclGetRsp() == -1)
			        //     {
			        //         status = ZFailure;
			        //     }
			        // }

					if(stat == 0x00) { //SUCCESS
			    		//wait for the request to resolve
			    	} else {
			    		myZnp->waitForResponse = false;
			    	}
		   	 	}

				args[0] = Nan::New(stat);
				args[1] = Nan::New(command->msgId);
				args[2] = Nan::New(command->seqNumber);
				if(req->statusCB){
		    		req->statusCB->Call(Nan::GetCurrentContext()->Global(), 3, args);
				}
				delete req->command;
				break;
			}

			case ZNP::ZCL_WRITE_ATTR:
			{
				ZNP::writeAttr_t *command = (ZNP::writeAttr_t*)req->command;

				dbg_print(PRINT_LEVEL_VERBOSE, "Got write attribute\n");
				afAddrType_t afDstAddr;
    			zclWriteCmd_t* writeCmd;
    			// zclWriteRec_t cmdRecord;
    			int stat;
    			//printf("1\n");
			    afDstAddr.addr.shortAddr = command->dstAddr;
			    afDstAddr.endPoint = command->endPoint;
			    afDstAddr.addrMode = command->addrMode;
    			//printf("2\n");

    			writeCmd = (zclWriteCmd_t*)malloc( sizeof(uint8) + sizeof(zclWriteRec_t)*command->numAttr);
    			// cmdRecord = (zclWriteRec_t*)malloc( sizeof(uint16) + sizeof(uint8) + sizeof(uint8));

			    if (writeCmd != NULL)
			    {
    			//printf("3\n");
    			//

					// printf("\tsrcAddr: %d\n",			command->dstAddr		);
					// printf("\tendPoint: %d\n",			command->endPoint		);
					// printf("\taddrMode: %d\n",			command->addrMode		);
					// printf("\tclusterId: %d\n",			command->numAttr		);
					// printf("\tcmdFormatLen: %d\n",			command->cmdFormatLen		);
					// int i = 0;
					// printf("\tpayload: ");
					// for(i = 0; i < command->cmdFormatLen; i++) {
					// 	printf("%d ", command->cmdFormat[i]);
					// }
					// printf("\n");
			        writeCmd->numAttr = command->numAttr;

			        int index = 0;
			        int listIndex = 0;
			        int k = 0;
			        // uint8 *attributeData[50];
			        while(index <= (command->cmdFormatLen - 1)) {
			        	// printf("\tindex: %d\n", index);
			        	writeCmd->attrList[listIndex].attrID = (command->cmdFormat[index] << 8) + command->cmdFormat[index+1];
						// printf("\tattrId: %d\n",			writeCmd->attrList[listIndex].attrID		);
			        	writeCmd->attrList[listIndex].dataType = command->cmdFormat[index+2];
						// printf("\tdataType: %d\n",			writeCmd->attrList[listIndex].dataType		);
						// printf("\tdata: ");
						writeCmd->attrList[listIndex].attrData = (uint8*)malloc(sizeof(uint8) * command->cmdFormat[index+3]);
			        	for(k = 0; k < command->cmdFormat[index+3]; k++) {
			        		writeCmd->attrList[listIndex].attrData[k] = command->cmdFormat[index + 4 + k];
			        	// memcpy(writeCmd->attrList[listIndex].attrData, command->cmdFormat[index + 4 + k, command->cmdFormat[index+3]);
			        		// printf("inx- %d, value- %d ", index+4+k, writeCmd->attrList[listIndex].attrData[k]);
			        		
			        	}
			        	// writeCmd->attrList[listIndex].attrData = attributeData[listIndex];
			        	// printf("\n");
			        	listIndex++;
			        	index = index + 2 + 1 + 1 + command->cmdFormat[index+3];
			        }
			        // cmdRecord.attrID = command->attrId;
			        // cmdRecord.dataType = command->dataType;
			        // cmdRecord.attrData = (uint8_t*)command->cmdFormat;
			        
			        // writeCmd->attrList[0] = cmdRecord;
    			//printf("4\n");
			        // for(int i = 0 ; i < command->cmdFormatLen; i++) {
			        // 	cmdRecord->attrData[i] = command->cmdFormat[]
			        // }
			        // cmdRecord->attrData[0] = command->
			        // writeCmd->attrID[0] = command->attrId;
			        // cmdRecord

		    		myZnp->waitForResponse = true;
		    		myZnp->currentCmdSeqId = command->seqNumber;
    			//printf("5\n");

			        stat = zcl_SendWriteRequest( command->srcEp, &afDstAddr,
						        command->clusterId, writeCmd, command->cmdId,
						        command->direction, command->disableDefaultRsp, command->seqNumber);

			        // free(cmdRecord);
			        int j = 0;
			        for(j = 0; j < listIndex; j++) {
			        	free(writeCmd->attrList[j].attrData);
			        }
			        free(writeCmd);

			        // if(block)
			        // {
			        //     if (waitZclGetRsp() == -1)
			        //     {
			        //         status = ZFailure;
			        //     }
			        // }

					if(stat == 0x00) { //SUCCESS
			    		//wait for the request to resolve
			    	} else {
			    		myZnp->waitForResponse = false;
			    	}
		   	 	}

				args[0] = Nan::New(stat);
				args[1] = Nan::New(command->msgId);
				args[2] = Nan::New(command->seqNumber);
				if(req->statusCB){
		    		req->statusCB->Call(Nan::GetCurrentContext()->Global(), 3, args);
				}
				delete req->command;
				break;
			}

			default:
			{
				dbg_print(PRINT_LEVEL_ERROR, "znpasync_cb_handler: Unhandled ZCL WorkCode: %d\n", req->workCode);
				break;
			}
		}

		workqueue.pop();
		delete req;
	}

	pthread_mutex_unlock(&workqueue_mutex);
}

void submitToZNP(ZNP::zclTransport *req)
{
	pthread_mutex_lock(&workqueue_mutex);
	workqueue.push(req);
	pthread_mutex_unlock(&workqueue_mutex);

	uv_async_send(&znpasync);
}


//*********************************************************************************************************************
bool ZNP::setupThread()
{
	uv_mutex_lock(&_control);
	bool ret = threadUp;
	uv_mutex_unlock(&_control);
	if(!ret) {
		this->Ref();
		uv_mutex_lock(&_control);
		int r = uv_thread_create(&znp_thread, main_thread, this); // start thread
        if (r < 0) {  // old libuv returns -1 on failure
                dbg_print(PRINT_LEVEL_INFO, "NON-RECOVERABLE: failed to create znp main thread.\n");
                uv_mutex_unlock(&_control);
                return false;
        }else {
                dbg_print(PRINT_LEVEL_INFO, "znp_main_thread create called successfully\n");
        }
		uv_cond_wait(&_start_cond, &_control); // wait for thread's start...
		ret = threadUp;
		uv_mutex_unlock(&_control);
	}
	return ret;
}

void ZNP::sigThreadUp() 
{
	uv_mutex_lock(&_control);
	threadUp = true;
	uv_cond_signal(&_start_cond);
	uv_mutex_unlock(&_control);
}

void ZNP::sigThreadDown() 
{
	uv_mutex_lock(&_control);
	threadUp = false;
	uv_cond_signal(&_start_cond);
	uv_mutex_unlock(&_control);
}

//*********************************************************************************************************************
NAN_METHOD(ZNP::New)
{
	assert(info.IsConstructCall());
	ZNP* self = new ZNP();
	self->Wrap(info.This());

	myZnp = self;

	if(info.Length() > 0 && info[0]->IsObject()) {
		Local<Object> o = info[0]->ToObject();
		Local<Value> v;

		V8_IFEXIST_TO_DYN_CSTR("siodev",myZnp->siodev,v,o);
		V8_IFEXIST_TO_INT_CAST("devType",myZnp->zOpts.devType,v,o,int);
		V8_IFEXIST_TO_INT_CAST("channelMask",myZnp->zOpts.channelMask,v,o,int);
		V8_IFEXIST_TO_INT_CAST("baudRate",myZnp->zOpts.baudRate,v,o,int);
		V8_IFEXIST_TO_INT_CAST("panId",myZnp->zOpts.panId,v,o,int);
		V8_IFEXIST_TO_BOOLEAN_CAST("newNwk",myZnp->zOpts.newNwk,v,o,bool);
	}
	
	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(ZNP::Connect)
{
	uv_async_init(uv_default_loop(), &v8async, (uv_async_cb)v8async_cb_handler);
	uv_async_init(uv_default_loop(), &znpasync, (uv_async_cb)znpasync_cb_handler);
	uv_mutex_init(&_control);
	uv_cond_init(&_start_cond);

	v8async.data = myZnp;

	const unsigned argc = 1;
	Local<Value> argv[argc];

	if(info.Length() > 1) {

		Local<Object> o = info[0]->ToObject();
		Local<Value> v;
		V8_IFEXIST_TO_DYN_CSTR("siodev",myZnp->siodev,v,o);

		if(info[1]->IsFunction()) {

			ZNP* zb = ObjectWrap::Unwrap<ZNP>(info.This());
			zb->onConnectedCB = new Nan::Callback(Local<Function>::Cast(info[1]));

			bool ret = zb->setupThread();
			if(ret) {
				argv[0] = Nan::New(true);	//thread started successfully
			} else {
				argv[0] = Nan::New(false);	//thread failed
			}
			zb->onConnectedCB->Call(Nan::GetCurrentContext()->Global(), argc, argv);
		} else {
			Nan::ThrowTypeError("Passed arguments 2 should be a function.");
		}
	} else {
		Nan::ThrowTypeError("Passed arguments should be atleast two.");
	}
}

NAN_METHOD(ZNP::Disconnect)
{
	ZNP* zb = ObjectWrap::Unwrap<ZNP>(info.This());
	Nan::Callback *onSuccessCB, *onFailureCB;

	if(info.Length() > 1) {
		if(info[0]->IsFunction() && info[1]->IsFunction()) {
			onSuccessCB = new Nan::Callback(Local<Function>::Cast(info[0]));
			onFailureCB = new Nan::Callback(Local<Function>::Cast(info[1]));
		} else {
			Nan::ThrowTypeError("Passed argument 0,1 should be a function.");
		}
	} else {
		Nan::ThrowTypeError("Disconnect: Should pass atleast two argments. [successcb, failcb]");
	}

	char * selected_serial_port;
	selected_serial_port = myZnp->siodev;

	dbg_print(PRINT_LEVEL_INFO, "attempting to close %s\n\n", selected_serial_port);
	if(wZCloseRPC()) {
		onSuccessCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
		delete onSuccessCB;
	} else {
		onFailureCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
		delete onFailureCB;
	}
	
	myZnp->sigThreadDown();
	pthread_cond_signal(&cond_critical_failure);
}

NAN_METHOD(ZNP::AddDevice)
{
	ZNP* zb = ObjectWrap::Unwrap<ZNP>(info.This());
	Nan::Callback *onSuccessCB, *onFailureCB;
	int duration;

	if(info.Length() > 2) {
		duration = info[0]->ToNumber()->Value();
		if(info[1]->IsFunction() && info[2]->IsFunction()) {
			onSuccessCB = new Nan::Callback(Local<Function>::Cast(info[1]));
			onFailureCB = new Nan::Callback(Local<Function>::Cast(info[2]));
		} else {
			Nan::ThrowTypeError("Passed arguments 1,2 should be a function.");
		}
	} else {
		Nan::ThrowTypeError("AddDevice: Should pass atleast three argument. [duration, successcb, failcb]");
	}

	if(!wZAddDevice(duration)) {
		onSuccessCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
		delete onSuccessCB;
	} else {
		onFailureCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
		delete onFailureCB;
	}
}

NAN_METHOD(ZNP::SendLqiRequest)
{
	ZNP* zb = ObjectWrap::Unwrap<ZNP>(info.This());
	Nan::Callback *onSuccessCB, *onFailureCB;
	int dstAddr;

	if(info.Length() > 2) {
		dstAddr = info[0]->ToNumber()->Value();
		if(info[1]->IsFunction() && info[2]->IsFunction()) {
			onSuccessCB = new Nan::Callback(Local<Function>::Cast(info[1]));
			onFailureCB = new Nan::Callback(Local<Function>::Cast(info[2]));
		} else {
			Nan::ThrowTypeError("Passed arguments 1,2 should be a function.");
		}
	} else {
		Nan::ThrowTypeError("SendLqiRequest: Should pass atleast three argument. [dstAddr, successcb, failcb]");
	}

	if(!wZSendLqiReq(dstAddr)) {
		onSuccessCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
		delete onSuccessCB;
	} else {
		onFailureCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
		delete onFailureCB;
	}
}

NAN_METHOD(ZNP::GetNVItem)
{
	ZNP* zb = ObjectWrap::Unwrap<ZNP>(info.This());
	Nan::Callback *onSuccessCB, *onFailureCB;
	int nvId;

	if(info.Length() > 2) {
		nvId = info[0]->ToNumber()->Value();
		// printf("GOT NVID- %d\n", nvId);
		if(info[1]->IsFunction() && info[2]->IsFunction()) {
			onSuccessCB = new Nan::Callback(Local<Function>::Cast(info[1]));
			onFailureCB = new Nan::Callback(Local<Function>::Cast(info[2]));
		} else {
			Nan::ThrowTypeError("Passed arguments 1,2 should be a function.");
		}
	} else {
		Nan::ThrowTypeError("GetNVItem: Should pass atleast three argument. [id, successcb, failcb]");
	}

	// if(!wZgetNVItem(nvId)) {
	// 	onSuccessCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
	// 	delete onSuccessCB;
	// } else {
	// 	onFailureCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
	// 	delete onFailureCB;
	// }

	nvRead_response* response = (nvRead_response*)wZgetNVItem(nvId);
	v8::Local<v8::Object> obj = Nan::New<v8::Object>();
	Local<Object> buf;
	Local<Value> args[16];

	if(response->len > 0) {
		toBuffer(buf, response->data, response->len * sizeof(uint8_t));
		args[1] = buf->ToObject();
	}

	// printf("Got respones- %d\n", response->status);
	// printf("Got len- %d\n", response->len);
	// printf("Got data- ");
	// int i = 0;
	// for(i = 0; i < response->len; i++) {
	// 	printf("%d ", response->data[i]);
	// }
	// printf("\n");
					
	if(response->status == 0) { //success
		obj->Set(Nan::New("status").ToLocalChecked(), Nan::New(response->status));
		obj->Set(Nan::New("len").ToLocalChecked(), Nan::New(response->len));
		args[0] = obj;
		// if(response->len > 0) {
		// 	toBuffer(buf, response->data, response->len * sizeof(uint8_t));
		// 	args[1] = buf->ToObject();
		// }
		onSuccessCB->Call(Nan::GetCurrentContext()->Global(), 2, args);
		delete onSuccessCB;
	} else { //failure
		obj->Set(Nan::New("status").ToLocalChecked(), Nan::New(response->status));
		args[0] = obj;
		onFailureCB->Call(Nan::GetCurrentContext()->Global(), 1, args);
		delete onFailureCB;
	}
	free(response);
}

NAN_METHOD(ZNP::SetNVItem)
{
	ZNP* zb = ObjectWrap::Unwrap<ZNP>(info.This());
	Nan::Callback *onSuccessCB, *onFailureCB;
	uint16 nvId;
	uint8 len;
	char *value;

	if(info.Length() > 4) {
		nvId = info[0]->ToNumber()->Value();
		len = info[1]->ToNumber()->Value();
		if(len > 0) {
			if(info[2]->IsObject()) {
				value = node::Buffer::Data(info[2]->ToObject());
			} else {
				Nan::ThrowTypeError("SetNVItem: Passed arguments 3 should be an Object.");
			}
		}
		if(info[3]->IsFunction() && info[4]->IsFunction()) {
			onSuccessCB = new Nan::Callback(Local<Function>::Cast(info[3]));
			onFailureCB = new Nan::Callback(Local<Function>::Cast(info[4]));
		} else {
			Nan::ThrowTypeError("SetNVItem: Passed arguments 4,5 should be a function.");
		}
	} else {
		Nan::ThrowTypeError("SetNVItem: Should pass atleast five argument. [id, len, value, successcb, failcb]");
	}

	if(!wZsetNVItem(nvId, len, (uint8_t*)value)) {
		onSuccessCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
		delete onSuccessCB;
	} else {
		onFailureCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
		delete onFailureCB;
	}
}

NAN_METHOD(ZNP::RemoveDevice)
{

}

NAN_METHOD(ZNP::EndDeviceAnnce)
{
	ZNP* zb = ObjectWrap::Unwrap<ZNP>(info.This());
	Nan::Callback *onSuccessCB, *onFailureCB;

	EndDeviceAnnceIndFormat_t msg;

	if(info.Length() > 2) {
		if(info[0]->IsObject()) {
			Local<Object> o = info[0]->ToObject();
			Local<Value> v;

			V8_IFEXIST_TO_INT_CAST("srcAddr",msg.SrcAddr,v,o,int);
			V8_IFEXIST_TO_INT_CAST("nwkAddr",msg.NwkAddr,v,o,int);
			// V8_IFEXIST_TO_INT_CAST("IEEEAddr",msg->IEEEAddr,v,o,int);
			// V8_IFEXIST_TO_DYN_CSTR("IEEEAddr",()msg->IEEEAddr,v,o);

			if(info[1]->IsFunction() && info[2]->IsFunction()) {
				onSuccessCB = new Nan::Callback(Local<Function>::Cast(info[1]));
				onFailureCB = new Nan::Callback(Local<Function>::Cast(info[2]));
			} else {
				Nan::ThrowTypeError("EndDeviceAnnce: Passed arguments 1,2 should be a function.");
			}
		} else {
			Nan::ThrowTypeError("EndDeviceAnnce: Passed arguments 0 should be a object.");
		}
	} else {
		Nan::ThrowTypeError("EndDeviceAnnce: Should pass atleast three argument. [object, successcb, failcb]");
	}

	if(!wZEndDeviceAnnce(&msg)) {
		onSuccessCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
		delete onSuccessCB;
	} else {
		onFailureCB->Call(Nan::GetCurrentContext()->Global(), 0, NULL);
		delete onFailureCB;
	}
}

NAN_METHOD(ZNP::DoZCLWork)
{
	ZNP* zb = ObjectWrap::Unwrap<ZNP>(info.This());

	zclTransport *req = new zclTransport();
	Local<Object> o;
	Local<Value> v;

	if(info.Length() > 3) {
		if(info[0]->IsObject()) {

			o = info[0]->ToObject();

			V8_IFEXIST_TO_INT_CAST("workCode",req->workCode,v,o,ZNP::work_code);

			switch(req->workCode) {

				case ZNP::ZCL_SEND_COMMAND:
				{

					sendCmd_t *command = new sendCmd_t();

					//ZCL_SEND_COMMAND
					V8_IFEXIST_TO_INT_CAST("srcEp",					command->srcEp,					v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("dstAddr",				command->dstAddr,				v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("endPoint",				command->endPoint,				v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("addrMode",				command->addrMode,				v,	o,	afAddrMode_t);//enum
					V8_IFEXIST_TO_INT_CAST("clusterId",				command->clusterId,				v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("msgId",					command->msgId,					v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("cmdId",					command->cmdId,					v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("specific",				command->specific,				v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("direction",				command->direction,				v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("disableDefaultRsp",		command->disableDefaultRsp,		v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("manuCode",				command->manuCode,				v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("seqNumber",				command->seqNumber,				v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("cmdFormatLen",			command->cmdFormatLen,			v,	o,	int);//uint16

					if(command->cmdFormatLen > 0) {
						if(info[1]->IsObject()) {
							command->cmdFormat = node::Buffer::Data(info[1]->ToObject());
						} else {
							Nan::ThrowTypeError("DoZCLWork: Passed arguments 2 should be an Object.");
						}
					} else {
						V8_IFEXIST_TO_DYN_CSTR("cmdFormat",				command->cmdFormat,				v,	o 		);//uint8*
					}

					req->command = (void*)command;
					req->size = sizeof(sendCmd_t);

					// printf("\tsrcEp: %d\n", 			command->srcEp				);
					// printf("\tdstAddr: %d\n",			command->dstAddr		);
					// printf("\tendPoint: %d\n",			command->endPoint		);
					// printf("\taddrMode: %d\n",			command->addrMode		);
					// printf("\tclusterId: %d\n",			command->clusterId		);
					// printf("\tcmdId: %d\n",				command->cmdId		);
					// printf("\tspecific: %d\n",			command->specific		);
					// printf("\tdirection: %d\n",			command->direction		);
					// printf("\tdisableDefaultRsp: %d\n",	command->disableDefaultRsp);
					// printf("\tmanuCode: %d\n",			command->manuCode	);
					// printf("\tseqNumber: %d\n",			command->seqNumber	);
					// printf("\tcmdFormatLen: %d\n",		command->cmdFormatLen	);
					// printf("\tcmdFormat: %d\n",			command->cmdFormat	);

					break;
				}

				case ZNP::ZCL_READ_ATTR:
				{
					readAttr_t *command = new readAttr_t();

					//ZCL_READ_ATTR
					V8_IFEXIST_TO_INT_CAST("srcEp",					command->srcEp,					v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("dstAddr",				command->dstAddr,				v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("endPoint",				command->endPoint,				v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("addrMode",				command->addrMode,				v,	o,	afAddrMode_t);//enum
					V8_IFEXIST_TO_INT_CAST("clusterId",				command->clusterId,				v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("msgId",					command->msgId,					v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("numAttr",				command->numAttr,				v,	o,	int);//uint8
					// V8_IFEXIST_TO_INT_CAST("attrId",				command->attrId,				v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("direction",				command->direction,				v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("disableDefaultRsp",		command->disableDefaultRsp,		v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("seqNumber",				command->seqNumber,				v,	o,	int);//uint16
					char *aIds;
					if(command->numAttr > 0) {
						if(info[2]->IsObject()) {
							aIds = (char*)node::Buffer::Data(info[2]->ToObject());
						} else {
							Nan::ThrowTypeError("DoZCLWork: Passed arguments 2 should be an Object.");
						}
					} else {
						// V8_IFEXIST_TO_INT_CAST("attrId",				command->attrId,				v,	o,	int);//uint16
						// V8_IFEXIST_TO_DYN_CSTR("cmdFormat",				command->cmdFormat,				v,	o 		);//uint8*
					}

					int i = 0;
					// printf("\tAttrId ");
					for(i = 0; i < command->numAttr; i++) {
						command->attrId[i] = (aIds[i*2] << 8) + aIds[i*2 + 1];
						// printf("%d ", command->attrId[i]);
					}
					// printf("\n");

					req->command = (void*)command;
					req->size = sizeof(readAttr_t);

					// printf("\tsrcEp: %d\n", 			command->srcEp				);
					// printf("\tdstAddr: %d\n",			command->dstAddr		);
					// printf("\tendPoint: %d\n",			command->endPoint		);
					// printf("\taddrMode: %d\n",			command->addrMode		);
					// printf("\tclusterId: %d\n",			command->clusterId		);
					// printf("\tnumAttr: %d\n",				command->numAttr		);
					// // printf("\tattrId: %d\n",			command->attrId		);
					// printf("\tdirection: %d\n",			command->direction		);
					// printf("\tdisableDefaultRsp: %d\n",	command->disableDefaultRsp);
					// printf("\tseqNumber: %d\n",			command->seqNumber	);

					break;
				}

				case ZNP::ZCL_WRITE_ATTR:
				{

					writeAttr_t *command = new writeAttr_t();

					//ZCL_WRITE_ATTR
					V8_IFEXIST_TO_INT_CAST("srcEp",					command->srcEp,					v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("dstAddr",				command->dstAddr,				v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("endPoint",				command->endPoint,				v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("addrMode",				command->addrMode,				v,	o,	afAddrMode_t);//enum
					V8_IFEXIST_TO_INT_CAST("clusterId",				command->clusterId,				v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("msgId",					command->msgId,					v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("numAttr",				command->numAttr,				v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("cmdId",					command->cmdId,					v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("attrId",				command->attrId,				v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("dataType",				command->dataType,				v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("direction",				command->direction,				v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("disableDefaultRsp",		command->disableDefaultRsp,		v,	o,	int);//uint8
					V8_IFEXIST_TO_INT_CAST("seqNumber",				command->seqNumber,				v,	o,	int);//uint16
					V8_IFEXIST_TO_INT_CAST("cmdFormatLen",			command->cmdFormatLen,			v,	o,	int);//uint16

					char *cmd;
					if(command->cmdFormatLen > 0) {
						if(info[1]->IsObject()) {
							cmd = node::Buffer::Data(info[1]->ToObject());
						} else {
							Nan::ThrowTypeError("DoZCLWork: Passed arguments 2 should be an Object.");
						}
					} else {
						// V8_IFEXIST_TO_DYN_CSTR("cmdFormat",				command->cmdFormat,				v,	o 		);//uint8*
					}

					int i = 0;
					for(i = 0; i < command->cmdFormatLen; i++) {
						command->cmdFormat[i] = cmd[i];
					}

					req->command = (void*)command;
					req->size = sizeof(writeAttr_t);

					//printf("\tsrcEp: %d\n", 			command->srcEp				);
					//printf("\tdstAddr: %d\n",			command->dstAddr		);
					//printf("\tendPoint: %d\n",			command->endPoint		);
					//printf("\taddrMode: %d\n",			command->addrMode		);
					//printf("\tclusterId: %d\n",			command->clusterId		);
					//printf("\tnumAttr: %d\n",				command->numAttr		);
					//printf("\tcmdId: %d\n",				command->cmdId		);
					//printf("\tdataType: %d\n",				command->dataType		);
					//printf("\tattrId: %d\n",			command->attrId		);
					//printf("\tdirection: %d\n",			command->direction		);
					//printf("\tdisableDefaultRsp: %d\n",	command->disableDefaultRsp);
					//printf("\tseqNumber: %d\n",			command->seqNumber	);
					//printf("\tcmdFormatLen: %d\n",			command->cmdFormatLen	);

					break;
				}

				default:
				{
					dbg_print(PRINT_LEVEL_ERROR, "DoZCLWork: Unhandled Work Code: %d\n", req->workCode);
					break;
				}
			}

			if(info[3]->IsFunction()) {
				req->statusCB = new Nan::Callback(Local<Function>::Cast(info[3]));
			} else {
				Nan::ThrowTypeError("DoZCLWork: Passed arguments 3 should be a function.");
			}

			submitToZNP(req);

		} else {
			Nan::ThrowTypeError("DoZCLWork: Passed arguments 1 should be an Object.");
		}
	} else {
		Nan::ThrowTypeError("DoZCLWork: Should pass atleast 3 argument. [command, data, cb]");
	}
}

NAN_METHOD(ZNP::OnNetworkReady) {
	if(info.Length() > 0) {
		if(info[0]->IsFunction()) {
			ZNP* obj = ObjectWrap::Unwrap<ZNP>(info.This());
			obj->onNetworkReadyCB = new Nan::Callback(info[0].As<Function>());
		} else {
			Nan::ThrowTypeError("OnNetworkReady: Passed in argument must be a Function.");
		}
	}
}

NAN_METHOD(ZNP::OnNetworkFailed) {
	if(info.Length() > 0) {
		if(info[0]->IsFunction()) {
			ZNP* obj = ObjectWrap::Unwrap<ZNP>(info.This());
			obj->onNetworkFailedCB = new Nan::Callback(info[0].As<Function>());
		} else {
			Nan::ThrowTypeError("OnNetworkFailed: Passed in argument must be a Function.");
		}
	}
}

NAN_METHOD(ZNP::OnNodeDiscovered) {
	if(info.Length() > 0) {
		if(info[0]->IsFunction()) {
			ZNP* obj = ObjectWrap::Unwrap<ZNP>(info.This());
			obj->onNodeDiscoveredCB = new Nan::Callback(info[0].As<Function>());
		} else {
			Nan::ThrowTypeError("OnNodeDiscovered: Passed in argument must be a Function.");
		}
	}
}

NAN_METHOD(ZNP::OnNetworkTopology) {
	if(info.Length() > 0) {
		if(info[0]->IsFunction()) {
			ZNP* obj = ObjectWrap::Unwrap<ZNP>(info.This());
			obj->onNetworkTopologyCB = new Nan::Callback(info[0].As<Function>());
		} else {
			Nan::ThrowTypeError("OnNetworkTopology: Passed in argument must be a Function.");
		}
	}
}

NAN_METHOD(ZNP::OnDeviceJoinedNetwork) {
	if(info.Length() > 0) {
		if(info[0]->IsFunction()) {
			ZNP* obj = ObjectWrap::Unwrap<ZNP>(info.This());
			obj->onDeviceJoinedNetworkCB = new Nan::Callback(info[0].As<Function>());
		} else {
			Nan::ThrowTypeError("OnDeviceJoinedNetwork: Passed in argument must be a Function.");
		}
	}
}

NAN_METHOD(ZNP::OnCmdResponse) {
	if(info.Length() > 0) {
		if(info[0]->IsFunction()) {
			ZNP* obj = ObjectWrap::Unwrap<ZNP>(info.This());
			obj->onCmdResponseCB = new Nan::Callback(info[0].As<Function>());
		} else {
			Nan::ThrowTypeError("OnCmdResponse: Passed in argument must be a Function.");
		}
	}
}

NAN_METHOD(ZNP::OnAttrResponse) {
	if(info.Length() > 0) {
		if(info[0]->IsFunction()) {
			ZNP* obj = ObjectWrap::Unwrap<ZNP>(info.This());
			obj->onAttrResponseCB = new Nan::Callback(info[0].As<Function>());
		} else {
			Nan::ThrowTypeError("OnAttrResponse: Passed in argument must be a Function.");
		}
	}
}

//*********************************************************************************************************************
__thread int znp_thread_errno = 0;

void *rpcTask(void *argument)
{
	int ret = 0;
	while (1) {
		ret = rpcProcess();
		if(ret != 0) {
			znp_thread_errno = ret; 
			break;
		}
	}
	dbg_print(PRINT_LEVEL_ERROR, "rpcTask exited!\n");
	pthread_mutex_lock(&critical_failure_mutex);
	pthread_cond_signal(&cond_critical_failure);
	pthread_mutex_unlock(&critical_failure_mutex);
}

void *appTask(void *argument)
{
	int ret = 0;
	while(1) {
		ret = appProcess(argument);
		if(ret != 0) {
			znp_thread_errno = ret;
			break;
		}
	}
	dbg_print(PRINT_LEVEL_ERROR, "appTask exited!\n");
	pthread_mutex_lock(&critical_failure_mutex);
	pthread_cond_signal(&cond_critical_failure);
	pthread_mutex_unlock(&critical_failure_mutex);
}

void *appInMessageTask(void *argument)
{
	int ret = 0;
	while(1) {
		ret = appMsgProcess(NULL);
		if(ret != 0) {
			znp_thread_errno = ret;
			break;
		}
	}
	dbg_print(PRINT_LEVEL_ERROR, "appInMessageTask exited!\n");
	pthread_mutex_lock(&critical_failure_mutex);
	pthread_cond_signal(&cond_critical_failure);
	pthread_mutex_unlock(&critical_failure_mutex);
}

void ZNP::main_thread(void *d) 
{
	myZnp = (ZNP *)d;

	bool critical_failure = false;
	char * selected_serial_port;
	pthread_t rpcThread, appThread, inMThread;

	selected_serial_port = myZnp->siodev;
	dbg_print(PRINT_LEVEL_INFO, "attempting to use %s\n\n", selected_serial_port);

	int serialPortFd = rpcOpen(selected_serial_port, 0, myZnp->zOpts.baudRate);
	if (serialPortFd == -1) {
		dbg_print(PRINT_LEVEL_ERROR, "could not open serial port\n");
		myZnp->sigThreadDown();
		return;
	}

	rpcInitMq();

	//init the application thread to register the callbacks
	appInit();

	//Start the Rx thread
	dbg_print(PRINT_LEVEL_INFO, "creating RPC thread\n");
	pthread_create(&rpcThread, NULL, rpcTask, (void *) &serialPortFd);

	//Start the example thread
	dbg_print(PRINT_LEVEL_INFO, "creating example thread\n");
	pthread_create(&appThread, NULL, appTask, (void*)&myZnp->zOpts);
	pthread_create(&inMThread, NULL, appInMessageTask, NULL);

	myZnp->sigThreadUp();

	pthread_mutex_lock(&critical_failure_mutex);
	pthread_cond_wait(&cond_critical_failure, &critical_failure_mutex);
	//if any of the pthread dies
	switch(znp_thread_errno) {
		case -1:
			critical_failure = true;
			dbg_print(PRINT_LEVEL_ERROR, "critical failure\n");
			//report critical failure
			break;
	}
	pthread_mutex_unlock(&critical_failure_mutex);
	pthread_cancel(rpcThread);
	pthread_cancel(appThread);
	pthread_cancel(inMThread);
	dbg_print(PRINT_LEVEL_ERROR, "Exiting node thread!\n");
}
//*********************************************************************************************************************

void zWNetworkReady(void)
{
	event_code code = NETWORK_UP;
	submitToV8(code, NULL, 0, 0);
}

void zWNetworkFailed(void)
{
	event_code code = NETWORK_DOWN;
	submitToV8(code, NULL, 0, 0);
}

void zWDataResponseConfirm(uint8_t *status) 
{
    dbg_print(PRINT_LEVEL_VERBOSE, "Got zcl response - %d\n", status);
    submitToV8(ZCL_COMMAND_RESPONSE, (void*)status, sizeof(uint8_t), 0);
}

void zWInformReadAttritubeRsp(attr_response *resp)
{
    //process simple desc here
    dbg_print(PRINT_LEVEL_VERBOSE, "Got Attritube response\n");
    submitToV8(ZCL_ATTR_RESPONSE, (void*)resp, sizeof(attr_response), 0);
}

//ZCL callbacks
uint8_t zWZdoSimpleDescRspCb(epInfo_t *epInfo)
{
    //process simple desc here
    dbg_print(PRINT_LEVEL_VERBOSE, "Device joined network\n");
    submitToV8(DISCOVERED, (void*)epInfo, sizeof(epInfo_t), 0);
    return 0;
}

//ZCL callbacks
uint8_t zWUpdateNetworkTopology(Node_t *nodeList)
{
    dbg_print(PRINT_LEVEL_VERBOSE, "Got network topology\n");
    submitToV8(NETWORK_TOPOLOGY, (void*)nodeList, sizeof(Node_t), 0);
    return 0;
}

//ZCL callbacks
uint8_t zWDeviceJoinedNetwork(EndDeviceAnnceIndFormat_t *msg)
{
    dbg_print(PRINT_LEVEL_VERBOSE, "Got device joined network\n");
    submitToV8(ONLINE_DEVICE, (void*)msg, sizeof(EndDeviceAnnceIndFormat_t), 0);
    return 0;
}
//*********************************************************************************************************************

extern "C" void init(v8::Local<v8::Object> target)
{
	Nan::HandleScope scope;

	Local<FunctionTemplate> t = Nan::New<v8::FunctionTemplate>(ZNP::New);
	t->InstanceTemplate()->SetInternalFieldCount(1);
	t->SetClassName(Nan::New("ZNP").ToLocalChecked());

	Nan::SetPrototypeMethod(t, "connect", ZNP::Connect);
	Nan::SetPrototypeMethod(t, "disconnect", ZNP::Disconnect);
	Nan::SetPrototypeMethod(t, "addDevice", ZNP::AddDevice);
	Nan::SetPrototypeMethod(t, "removeDevice", ZNP::RemoveDevice);
	Nan::SetPrototypeMethod(t, "doZCLWork", ZNP::DoZCLWork);
	Nan::SetPrototypeMethod(t, "endDeviceAnnce", ZNP::EndDeviceAnnce);
	Nan::SetPrototypeMethod(t, "getNVItem", ZNP::GetNVItem);
	Nan::SetPrototypeMethod(t, "setNVItem", ZNP::SetNVItem);
	Nan::SetPrototypeMethod(t, "sendLqiRequest", ZNP::SendLqiRequest);


	//Callbacks
	Nan::SetPrototypeMethod(t, "onNetworkReady", ZNP::OnNetworkReady);
	Nan::SetPrototypeMethod(t, "onNetworkFailed", ZNP::OnNetworkFailed);
	Nan::SetPrototypeMethod(t, "onNodeDiscovered", ZNP::OnNodeDiscovered);
	Nan::SetPrototypeMethod(t, "onCmdResponse", ZNP::OnCmdResponse);
	Nan::SetPrototypeMethod(t, "onAttrResponse", ZNP::OnAttrResponse);
	Nan::SetPrototypeMethod(t, "onNetworkTopology", ZNP::OnNetworkTopology);
	Nan::SetPrototypeMethod(t, "onDeviceJoinedNetwork", ZNP::OnDeviceJoinedNetwork);

	target->Set(Nan::New("ZNP").ToLocalChecked(), t->GetFunction());
}

NODE_MODULE(znp, init)
