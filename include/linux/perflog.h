/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 * System performance team / System Group
 *
 * All right reserved. This software is the confidential and proprietary
 * information of Samsung Electronics ("Confidential Information").
 * You shall not disclose such Confidential Information and shall use it
 * only in accordance with the terms of the license agreement you
 * entered into with Samsung Electronics.
 */

#ifndef PERFLOG_H_
#define PERFLOG_H_

#define PERFLOG_LOC __FILE__, __LINE__

#ifdef __cplusplus
extern "C" {
#endif

#include "olog.pb.h"

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;

// extern FILE* perflog_fout;
#define PERFLOG_PACKET_SIZE			256
#define PERFLOG_HEADER_SIZE 			24
#define PERFLOG_BUFF_STR_MAX_SIZE 		(PERFLOG_PACKET_SIZE - PERFLOG_HEADER_SIZE)
#define PERFLOG_BUFF_STR_MAX_SIZE_FOR_MULTILINE	4096
#define PERFLOG_BUFF_STR_MAX_SIZE_FOR_EVTI	PERFLOG_BUFF_STR_MAX_SIZE - PERFLOG_UINT16_SIZE
#define PERFLOG_BUFF_STR_MAX_SIZE_FOR_EVTII	PERFLOG_BUFF_STR_MAX_SIZE - PERFLOG_UINT16_SIZE - PERFLOG_UINT16_SIZE
#define PERFLOG_UINT16_SIZE			2

/* PerfLog Phase 2 :: header format modification 
   should be changed to protobuff type 
*/
typedef enum PerfLogAffectTag {
    AFFECT_K,
    AFFECT_F,
    AFFECT_A
}PerfLogAffect;

typedef enum EvtNamingTag {
    NAMING_LockC,
    NAMING_AppLaunch,    
}EvtNamingTag;

typedef enum PerfLevelTag{
    LOW,
    MID,
    HIGH,
    CRITICAL
}PerfLevelTag;

// ###############################################################################

#pragma pack(1) 

typedef union{

	struct {
		uint16 param1;
		char logbuffer[PERFLOG_BUFF_STR_MAX_SIZE_FOR_EVTI + 1];
	} EvtPacket;

	struct {
		uint16 param1;
		uint16 param2;
		char logbuffer[PERFLOG_BUFF_STR_MAX_SIZE_FOR_EVTII + 1];
	} EvtIIPacket;

		struct {
		int param1;
		int param2;
		char logbuffer[PERFLOG_BUFF_STR_MAX_SIZE_FOR_EVTII + 1];
	} EvtIntIntPacket;

	char logbuffer[PERFLOG_BUFF_STR_MAX_SIZE + 1];
} Payload;  

struct LogPacket {
	struct timespec logtime;
        uint16 logtype;
        uint16 logid;
        uint16 pid;
        uint16 tid;
		Payload payload;
};

struct _Timestamp {
	uint8  month;
	uint8  day;
	uint8  hour;
	uint8  minute;
	uint8  second;
	uint16 msecond;
};

struct _PLogPacket {
	struct _Timestamp timestamp;
        uint16 pid;
        uint16 tid;

	uint8  type;
	uint8  id;

	char   pname[10];
	uint8  context_length;
	char   context_buffer[PERFLOG_BUFF_STR_MAX_SIZE + 1];
};

union _uPLogPacket {
	struct _PLogPacket itemes;
	char stream[PERFLOG_HEADER_SIZE + PERFLOG_BUFF_STR_MAX_SIZE];
};
#pragma pack(0) 

// Start API
int perflog_write(char const * fmt, ...);
int perflog_write_log(uint16 type, uint16 logid, char const * fmt, ...);

int perflog_write_evt(uint16 maintype, uint16 logid, uint16 param1, char const * fmt, ...);

int perflog_write_evt_iis(uint16 maintype, uint16 logid, uint16 param1, uint16 param2, char const * fmt, ...);

int perflog_write_evt_IIS(uint16 maintype, uint16 logid, int param1, int param2, char const * fmt, ...);

// int perflog_getlog(char **buff);
// End API

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif
