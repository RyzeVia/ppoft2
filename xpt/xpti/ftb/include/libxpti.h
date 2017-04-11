/*
 * libxpti.h
 *
 *  Created on: 2012/06/21
 *      Author: RyzeVia
 */

#ifndef LIBXPTI_H_
#define LIBXPTI_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "utilmacros.h"
#include "libftb.h"

#ifdef _WIN32
	#include <process.h>
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/wait.h>
#endif


#define XPTI_MAXEVENT	28
#define XPTI_MAXATTRIB	(128 - XPTI_MAXEVENT)
#define XPTI_MAXPAYLOAD	FTB_MAX_PAYLOAD_DATA

#define XPTI_TRUE 		FTB_SUCCESS
#define XPTI_FALSE 		-1
#define XPTI_UNEXPECTED -99

#define XPTI_ROLE_ONEOFTHE_WORLD	0x4
#define XPTI_ROLE_ONEOFTHE_JOB		0x2
#define XPTI_ROLE_ONEOFTHE_NODE		0x1
#define XPTI_ROLE_OTHERS			0xF

typedef struct XPTI_ehdlset_ {
	FTB_subscribe_handle_t hdl;
	FTB_receive_event_t evt;
	int used;
//	const char* name;
} XPTI_ehdlset_t;

typedef struct XPTI_info_ {
	FTB_client_handle_t chandle;
	FTB_client_t cinfo;
	int n_phdl;
	XPTI_ehdlset_t *phdl;
	XPTI_ehdlset_t shdl[1];
	const char* event_space_name;

} XPTI_info_t;

typedef enum XPTI_recv_event_state_ {
	XPTI_RES_NULL,
	XPTI_RES_PUSHED,

	MAX_XPTI_RES
} XPTI_recv_event_state_e;

/* static functions */
//int SXPTI_FTB_exist_programs(char* pname);
int XPTI_init(XPTI_info_t *info, const char * event_name);
int XPTI_phdl_init(XPTI_info_t *info, int n_phdl);
int XPTI_shdl_init(XPTI_info_t *info, int n_shdl);
//static int SXPTI_subscribe(FTB_client_handle_t ch, FTB_subscribe_handle_t* sh, char* signal, char* extra_attribute);
int XPTI_pull_subscribe(XPTI_info_t *info, char* signal, char* extra_attribute);
int XPTI_pull_subscribe_flexible(XPTI_info_t *info, char* attributes);
int XPTI_push_subscribe(XPTI_info_t *info, char* signal, char* extra_attribute);
int XPTI_pull_unsubscribe(XPTI_info_t *info);
int XPTI_push_unsubscribe(XPTI_info_t *info);
int XPTI_declare_publishable_events(XPTI_info_t *info, char* signal, char* serverity);
int XPTI_declare_publishable_events_with_schemafile(XPTI_info_t *info, char* filename);
int XPTI_publish(XPTI_info_t *info, char* signal, int tag, char* payload);
//static int SXPTI_poll_event(FTB_subscribe_handle_t sh, FTB_receive_event_t* re, char** recved_name);
int XPTI_poll_pull_event(XPTI_info_t *info, char** recved_name);
int XPTI_poll_push_event(XPTI_info_t *info, char** recved_name);
char* XPTI_get_pull_event_payload(XPTI_info_t *info);
char* XPTI_get_push_event_payload(XPTI_info_t *info);
int XPTI_fini(XPTI_info_t *info);
//void SXPTI_exec_ftbserver();
//void SXPTI_exec_agent();
//void SXPTI_exec_coserver_oneofthejob();
//void SXPTI_killnwait_coserver_oneofthejob();
//void SXPTI_exec_coserver_oneofthenode();
//void SXPTI_killnwait_coserver_oneofthenode();
void XPTI_exec_coserver(int role);
void XPTI_killnwait_coserver(int role);
void XPTI_coserver_exec_complete();

#endif /* LIBXPTI_H_ */
