/**
 * This is a wrapper of the PVXS C++ library to provide a C interface to other programming languages.
 * Three simple interfaces are implemented,
 * 1. simpleget
 * 2. simpleput
 * 3. simplemonitor
 * 
 * This file is intended to be placed into the pvxs/src directory and compiled into pvxs.dll, libpvxs.so or libpvxs.dylib
 */

#include <sstream>
#include <iostream>
#include <list>
#include <atomic>
#include <cstring>

#include <epicsVersion.h>
#include <epicsGetopt.h>
#include <epicsThread.h>
#include "libComAPI.h"

#include <pvxs/client.h>
#include <pvxs/log.h>
#include <pvxs/server.h>
#include <pvxs/sharedpv.h>
#include <pvxs/nt.h>
#include "utilpvt.h"
#include "evhelper.h"
#include "dataimpl.h"

using namespace pvxs;

typedef void (*monitorCallBackFunc) (const char *args);

typedef struct pvInfo {
    char pvname[100];
	monitorCallBackFunc pfunc;
} pvInfo;

extern "C" {
	LIBCOM_API int simpleget(char *pvname, char** value, double timeout);
    LIBCOM_API int simpleput(char *pvname, char **fields, char **values, int size, double timeout);
	LIBCOM_API int simplemonitor(char *pvname, monitorCallBackFunc pfunc);
}

// Traverse the PVXS tree structure and generate data in JSON format
void top(const std::string& member, const FieldDesc *desc, const FieldStorage* store, std::stringstream& strm)
{
	bool _showValue = true;
	size_t _limit = 0u;

    strm<<indent{};
    if(!desc) {
        strm<<"null,\n";
        return;
    }

	if(!member.empty() && store->code != StoreType::Compound)
		strm<<"\""<<member<<"\": ";

    switch(store->code) {
        case StoreType::Null:
            if(desc->code==TypeCode::Struct) {
                strm<<" {\n";
                if(!desc->id.empty()) {
                    strm<<"\"id\": "<<"\""<<desc->id<<"\","<<"\n";
                }
                for(auto& pair : desc->miter) {
                    auto cdesc = desc + pair.second;
                    Indented I(strm);
                    top(pair.first, cdesc, store + pair.second, strm);
                }
                strm<<indent{}<<"}";
                if(desc->parent_index != 0)
                    strm<<",";
                strm<<"\n";
            } else {
                strm<<"\n";
            }
            break;
        case StoreType::Real:     if(_showValue) { strm<<store->as<double>(); } strm<<",\n"; break;
        case StoreType::Integer:  if(_showValue) { strm<<store->as<int64_t>(); } strm<<",\n"; break;
        case StoreType::UInteger: if(_showValue) { strm<<store->as<uint64_t>(); } strm<<",\n"; break;
        case StoreType::Bool:     if(_showValue) { strm<<(store->as<bool>() ? true : false); } strm<<",\n"; break;
        case StoreType::String:   if(_showValue) { strm<<"\""<<escape(store->as<std::string>())<<"\""; } strm<<",\n"; break;
        case StoreType::Compound: {
            strm<<"\""<<member;
            auto& fld = store->as<Value>();
            if(fld.valid() && desc->code==TypeCode::Union) {
                for(auto& pair : desc->miter) {
                    if(&desc->members[pair.second] == Value::Helper::desc(fld)) {
                        strm<<"."<<pair.first;
                        break;
                    }
                }
            }
            strm<<"\": ";
            Indented I(strm);
            top(std::string(),
                Value::Helper::desc(fld),
                Value::Helper::store_ptr(fld),
                strm);
        }
            break;
        case StoreType::Array: {
            auto& varr = store->as<shared_array<const void>>();
            if(!_showValue) {
                strm<<"\n";
            } else if(varr.original_type()!=ArrayType::Value) {
                strm<<varr.format().limit(_limit)<<",\n";
            } else {
                auto arr = varr.castTo<const Value>();
                strm<<"[\n";
                for(auto& val : arr) {
                    Indented I(strm);
                    top(std::string(),
                        Value::Helper::desc(val),
                        Value::Helper::store_ptr(val),
                        strm);
                }
                strm<<indent{}<<"]";
                if(desc->parent_index != 0)
                    strm<<",";
                strm<<"\n";
            }
        }
            break;
        default:
            strm<<"!!Invalid StoreType!! "<<int(store->code)<<"\n";
            break;
    }
}

// Simple blocking pvget
int simpleget(char *pvname, char** value, double timeout) {
	try {
		auto ctxt(client::Context::fromEnv());
		Value reply = ctxt.get(pvname).exec()->wait(timeout);

		std::stringstream strm;
		top("", Value::Helper::desc(reply), Value::Helper::store_ptr(reply), strm);

		const std::string tmp = strm.str();
		*value = new char[tmp.length() + 1];
		strcpy(*value, tmp.c_str());

    	return 0;
    } catch(std::exception& e){
        std::cerr<<"Error: "<<e.what()<<"\n";
        return -1;
    }
}

// Simple blocking pvput
int simpleput(char *pvname, char **fields, char **values, int size, double timeout) {
    char *field;
    char *value;
    try {
        auto ctxt(client::Context::fromEnv());
        auto pb = ctxt.put(pvname);
        for(int i = 0; i < size; i++) {
            field = *(fields + i);
            value = *(values + i);
            pb = pb.set(field, value);
        }
        auto result = pb.exec()->wait(timeout);
        return 0;
    } catch(std::exception& e){
        std::cerr<<"Error: "<<e.what()<<"\n";
        return -1;
    }
}

// Worker thread for pvmonitor
void monitorThread(void *pvt) {
    pvInfo *ppvInfo = (pvInfo *)pvt;
    char *pvname = ppvInfo->pvname;
    monitorCallBackFunc cb = ppvInfo->pfunc;
    try {
        auto ctxt(client::Context::fromEnv());
		MPMCFIFO<std::shared_ptr<client::Subscription>> workqueue(42u);

		auto op = ctxt.monitor(pvname)
				       .event([&workqueue](client::Subscription& sub) {
				           workqueue.push(sub.shared_from_this());
				       })
				       .exec();

		while(auto sub = workqueue.pop()) {
			try {
				Value update = sub->pop();
				if(!update)
				    continue;
                std::stringstream strm;
		        top("", Value::Helper::desc(update), Value::Helper::store_ptr(update), strm);
                // Invoke the Node.js callback and pass the JSON data
                cb(strm.str().c_str());
			} catch(std::exception& e) {
				std::cerr<<"Error "<<e.what()<<"\n";
			}
			workqueue.push(sub);
		}
		// store op until completion
        
    } catch(std::exception& e){
        std::cerr<<"Error: "<<e.what()<<"\n";
    }
}

// Create worker thread to monitor
int simplemonitor(char *pvname, monitorCallBackFunc pfunc) {
	if(!pvname || !pfunc) {
        std::cout<<"pvname and pfunc should not be null\n";
        return -1;
    }
	pvInfo *ppvInfo = new pvInfo;
	memcpy(ppvInfo->pvname, pvname, strlen(pvname) + 1);
	ppvInfo->pfunc = pfunc;

	epicsThreadId threadId;
	threadId = epicsThreadCreate(pvname,
                epicsThreadPriorityMedium,
                epicsThreadGetStackSize(epicsThreadStackSmall),
                monitorThread,
                ppvInfo);
	if(threadId == 0) {
		std::cout<<pvname<<": unable to create PV monitor daemon thread\n";
		return -1;
    }

	return 0;
}