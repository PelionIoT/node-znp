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

#ifndef _ZNP_NODE_H_
#define _ZNP_NODE_H_

#include <v8.h>
#include <node.h>
#include <nan.h>

#include "znp_cfuncs.h"
#include "mtAf.h"

using namespace v8;
using namespace node;

#define UNI_BUFFER_NEW(size)                                                \
    Nan::NewBuffer(size).ToLocalChecked()
#define V8_IFEXIST_TO_INT32(v8field,cfield,val,v8obj) { val = v8obj->Get(Nan::New(v8field).ToLocalChecked());\
		if(!val->IsUndefined() && val->IsNumber()) cfield = val->ToInteger()->Int32Value(); }
#define V8_IFEXIST_TO_DYN_CSTR(v8field,cfield,val,v8obj) { val = v8obj->Get(Nan::New(v8field).ToLocalChecked());\
if(!val->IsUndefined() && val->IsString()) {\
	if(cfield) free(cfield);\
	v8::String::Utf8Value v8str(val);\
	cfield = strdup(v8str.operator *()); } }
#define V8_IFEXIST_TO_INT_CAST(v8field,cfield,val,v8obj,typ) { val = v8obj->Get(Nan::New(v8field).ToLocalChecked());\
		if(!val->IsUndefined() && val->IsNumber()) cfield = (typ) val->ToInteger()->IntegerValue(); }
#define V8_IFEXIST_TO_INT_CAST_THROWBOUNDS(v8field,cfield,val,v8obj,typ,lowbound,highbound) { val = v8obj->Get(Nan::New(v8field).ToLocalChecked());\
if(!val->IsUndefined() && val->IsNumber()) { \
int64_t cval = val->ToInteger()->IntegerValue();\
if(cval < ((int64_t) ((typ) lowbound)) || cval > ((int64_t) ((typ) highbound))) Nan::ThrowTypeError(v8field" is out of bounds.");\
else cfield = (typ) val->ToInteger()->IntegerValue(); } }
#define V8_IFEXIST_TO_BOOLEAN_CAST(v8field,cfield,val,v8obj,typ) { val = v8obj->Get(Nan::New(v8field).ToLocalChecked());\
		if(!val->IsUndefined()) cfield = (typ) val->BooleanValue(); }


#define toBuffer(buf, data, len) { if(len > 0) { buf = UNI_BUFFER_NEW(len); char *mem = node::Buffer::Data(buf); ::memcpy(mem,data,len); } }

class ZNP;

#ifdef __cplusplus
extern "C" {
#endif

extern __thread ZNP *myZnp;

#ifdef __cplusplus
};
#endif

class ZNP : public Nan::ObjectWrap {
	public: 
		static NAN_METHOD(New);
		static NAN_METHOD(Connect);
		static NAN_METHOD(Disconnect);
		static NAN_METHOD(AddDevice);
		static NAN_METHOD(RemoveDevice);
		static NAN_METHOD(DoZCLWork);
		static NAN_METHOD(EndDeviceAnnce);
		static NAN_METHOD(GetNVItem);
		static NAN_METHOD(SetNVItem);
		static NAN_METHOD(SendLqiRequest);

		static NAN_METHOD(OnNetworkReady);
		static NAN_METHOD(OnNetworkFailed);
		static NAN_METHOD(OnNodeDiscovered);
		static NAN_METHOD(OnCmdResponse);
		static NAN_METHOD(OnAttrResponse);
		static NAN_METHOD(OnNetworkTopology);
		static NAN_METHOD(OnDeviceJoinedNetwork);

		Nan::Callback *onConnectedCB;
		Nan::Callback *onNetworkReadyCB;
		Nan::Callback *onNetworkFailedCB;
		Nan::Callback *onNodeDiscoveredCB;
		Nan::Callback *onCmdResponseCB;
		Nan::Callback *onAttrResponseCB;
		Nan::Callback *onNetworkTopologyCB;
		Nan::Callback *onDeviceJoinedNetworkCB;

		config_options zOpts;
		char *siodev;

		uint16_t currentCmdSeqId;
		bool waitForResponse;

		typedef struct {
			uint8_t			srcEp;
			uint16_t		dstAddr;
			uint16_t		msgId;
			uint8_t			endPoint;
			afAddrMode_t	addrMode;
			uint16_t		clusterId;
			uint8_t			cmdId;
			uint8_t			specific;
			uint8_t 		direction;
			uint8_t 		disableDefaultRsp;
			uint16_t 		manuCode;
			uint16_t 		seqNumber;
			uint16_t 		cmdFormatLen;
			char	 		*cmdFormat;
		} sendCmd_t;

		typedef struct {
			uint8_t			srcEp;
			uint16_t		dstAddr;
			uint16_t		msgId;
			uint8_t			endPoint;
			afAddrMode_t	addrMode;
			uint16_t		clusterId;
			uint8_t			numAttr;
			uint16_t		attrId[50];
			uint8_t 		direction;
			uint8_t 		disableDefaultRsp;
			uint16_t 		seqNumber;
		} readAttr_t;

		typedef struct {
			uint8_t			srcEp;
			uint16_t		dstAddr;
			uint8_t			endPoint;
			uint16_t		msgId;
			afAddrMode_t	addrMode;
			uint16_t		clusterId;
			uint8_t			cmdId;
			uint8_t			dataType;
			uint8_t			numAttr;
			uint16_t		attrId;
			uint8_t 		direction;
			uint8_t 		disableDefaultRsp;
			uint16_t 		seqNumber;
			uint16_t 		cmdFormatLen;
			uint8_t	 		cmdFormat[300];
		} writeAttr_t;

		enum work_code {
			ZCL_SEND_COMMAND,
			ZCL_READ_ATTR,
			ZCL_WRITE_ATTR
		};

		typedef struct {
			work_code workCode;
			void *command;
			int size;
			int _errno;
			Nan::Callback *statusCB;
		} zclTransport;

	protected:
		static void main_thread(void *d);

		bool setupThread();
		void sigThreadUp();
		void sigThreadDown();
};

#endif //_ZNP_NODE_H_