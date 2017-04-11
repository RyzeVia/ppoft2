/*
 * libfi.h
 *
 *  Created on: 2012/06/28
 *      Author: RyzeVia
 */

#ifndef LIBFI_H_
#define LIBFI_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

#ifdef PTHREAD_ONCE_INIT
#define FI_PTHREAD_ENABLE
#endif

#ifdef FI_PTHREAD_ENABLE
#include <pthread.h>
#define FI_SIGPROCMASK	pthread_sigmask
#else
#define FI_SIGPROCMASK	sigprocmask
#endif

#include "libfivc.h"
#include "utilmacros.h"

#define FI_TRUE		XPTI_TRUE
#define FI_FALSE 	XPTI_FALSE

#define MAX_FI_COMMAND	32
#define MAX_FI_ARGS		128

#define FI_ENV_NAME			"FI_SCENARIO"
#define FI_SCE_FILENAME		"./scenario"
#define FI_DEFAULT_TICK		0.1
#define FI_TICKING_SIGNAL	(SIGRTMAX-1)

typedef struct FI_info_ FI_info_t;

typedef enum FI_sce_connectivity_ {
	FI_SCC_UNKNOWN,
	FI_SCC_NOEVENT,
	FI_SCC_NEXTTICK,
	FI_SCC_IMMEDIATE,
	FI_SCC_END,

	MAX_FI_SCC
} FI_sce_connectivity_e;


typedef struct FI_command_ {
	struct timeval tp;
	char command[MAX_FI_COMMAND];
	char args[MAX_FI_ARGS];
} FI_command_t;

typedef int (*FI_evhdls_func)(struct FI_info_ *sh, FI_command_t *cmd);
typedef int (*FI_evhdlc_func)(struct FI_info_ *sh, char *cmd, char* payload);

struct FI_info_ {
	FILE* sfh;
	struct timeval st;
	struct timeval tick;
	timer_t tick_id;
	fpos_t ppos;
	int survive;
	FI_evhdls_func sfunc;
	FI_evhdlc_func cfunc;
	char** target_cmd;
	int target_argc;
	pid_t target_pid;
	int restart;
	int iter;
	int ckptc;
	struct timeval *stamp_epoch;
	struct timeval *stamp_ckpt;

	double fr;
	struct timeval ckpt_interval;

	XPT_info_t xptinfo;
//	FI_VC_info_t vcinfo;
};


#define FI_SCE_EVENT_CMP(cmd, eventname) if (strncmp((cmd)->command, eventname, strlen(eventname)) == 0)
#define FI_CON_EVENT_CMP(cmd, eventname) if (strncmp(cmd, eventname, strlen(eventname)) == 0)

int FI_init(FI_info_t *sh, const char* event_name);
int FI_declare_event(FI_info_t *sh, char* evname);
int FI_open_scenario(FI_info_t *sh, char* filename);
FI_sce_connectivity_e FI_fetch_scenario(FI_info_t *sh, FI_command_t *cmd);
int FI_ticked_loop(FI_info_t *sh);
int FI_event_handler_for_sce(FI_info_t *sh, FI_command_t *cmd);
int FI_event_handler_for_con(FI_info_t *sh, char* cmd, char* payload);
int FI_evhdls_blt_failurerate(FI_info_t *sh, FI_command_t *cmd);
int FI_evhdls_blt_settick(FI_info_t *sh, FI_command_t *cmd);
int FI_evhdls_blt_starttick(FI_info_t *sh, int starttime_reset);
int FI_evhdlc_blt_invoke(FI_info_t *vch, char* cmd, char* payload);
int FI_evhdlc_blt_restart(FI_info_t *vch, char* cmd, char* payload);
void FI_timer_set_start(FI_info_t *sh);
void FI_timer_get_from_start(FI_info_t *sh, struct timeval *tv);
void FI_timerhandler(int num, siginfo_t* sinfo, void* ucont);

#endif /* LIBFI_H_ */
