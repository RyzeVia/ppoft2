/*
 * FI_base.c
 *
 *  Created on: 2012/06/28
 *      Author: RyzeVia
 */

//#define DEBUG_ENABLE
#define DEBUG_LEVEL 0
#define GLOBAL_DEFINITION
#include "libfi.h"


static sigset_t dmask;
/*
 * FI_init should execute before thread initialization
 */
int FI_init(FI_info_t *sh, const char* event_name){
	sigset_t timer;
	struct sigaction satimer;
	struct sigevent evp;

	satimer.sa_sigaction = FI_timerhandler;
	satimer.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&satimer.sa_mask);
	ERRCHK(sigaction(FI_TICKING_SIGNAL, &satimer, NULL) < 0, "TimerSIG_init");

	sigemptyset(&timer);
	sigaddset(&timer, FI_TICKING_SIGNAL);
	sigprocmask(SIG_BLOCK, &timer, &dmask);

	evp.sigev_notify = SIGEV_SIGNAL;
	evp.sigev_signo = FI_TICKING_SIGNAL;
	ERRCHK(timer_create(CLOCK_REALTIME, &evp, &(sh->tick_id)) < 0, "timer_create");

	sh->st.tv_sec = 0;
	sh->st.tv_usec = 1;

	XPT_init(&(sh->xptinfo), event_name);

	XPT_CP_srv_init(&(sh->xptinfo));


	/* FIXME: TENTATIVE */
	sh->xptinfo.ckpt_rdy = XPT_TRUE;
//	FI_VC_init_i(&(sh->vcinfo), event_name);


	return EXIT_SUCCESS;
}

int FI_declare_event(FI_info_t *sh, char* evname){
//	int rv = FI_VC_declare_publishable_events_i(&(sh->vcinfo), evname, "INFO");
	int rv = XPT_declare_publishable_events(&(sh->xptinfo), evname, "INFO");
	return rv;
}

void FI_timerhandler(int num, siginfo_t* sinfo, void* ucont){
	printf("Timer tick\n");
}

/*
 * Scenario should open after thread initialization
 * if multi-threading was used
 */
int FI_open_scenario(FI_info_t *sh, char* filename){
	char* fname;

	if(filename == NULL){
		if(NULL == (fname = getenv(FI_ENV_NAME))){
			fname = FI_SCE_FILENAME;
		}
	}else{
		fname = filename;
	}

	sh->sfh = fopen(fname, "r");

	ERRCHK(sh->sfh == NULL, "Scenario Open");
	fgetpos(sh->sfh, &(sh->ppos));
	sh->survive = FI_TRUE;
	misc_dbl2tv(FI_DEFAULT_TICK, &(sh->tick));
	sh->st.tv_sec = 0;
	sh->st.tv_usec = 1;
	sh->restart = FI_FALSE;
	sh->iter = 0;

	FI_SIGPROCMASK(SIG_SETMASK, &dmask, NULL);

	sh->cfunc = NULL;
	sh->sfunc = NULL;

	return EXIT_SUCCESS;
}

FI_sce_connectivity_e FI_fetch_scenario(FI_info_t *sh, FI_command_t *cmd){
	struct timeval prefetched, cur;
	if(sh->survive == FI_FALSE)
		return FI_SCC_END;

	char buf[MAX_FI_COMMAND + MAX_FI_ARGS];

	FI_timer_get_from_start(sh, &cur);

fetch:
	if(NULL != fgets(buf, MAX_FI_COMMAND + MAX_FI_ARGS, sh->sfh)){

		fgetpos(sh->sfh, &(sh->ppos));
		if(
				(strncmp(buf, "\n", 1) == 0) ||
				(strncmp(buf, "#", 1) == 0)
				){
			goto fetch;
		}

		sscanf(buf, "%ld %ld %s %[^\n]", &(cmd->tp.tv_sec), &(cmd->tp.tv_usec), cmd->command, cmd->args);
		if(timercmp(&(cmd->tp), &cur, >)){
			fsetpos(sh->sfh, &(sh->ppos));
			return FI_SCC_NOEVENT;
		}

		fgetpos(sh->sfh, &(sh->ppos));

prefetch:
		if(NULL != fgets(buf, MAX_FI_COMMAND + MAX_FI_ARGS, sh->sfh)){
			if(
					(strncmp(buf, "\n", 1) == 0) ||
					(strncmp(buf, "#", 1) == 0)
					){
				goto prefetch;
			}

			sscanf(buf, "%ld %ld", &(prefetched.tv_sec), &(prefetched.tv_usec));
			fsetpos(sh->sfh, &(sh->ppos));

		}else{
			fsetpos(sh->sfh, &(sh->ppos));
			return FI_SCC_NEXTTICK;
		}
	}else{
		sh->survive = FI_FALSE;
		return FI_SCC_END;
	}

	if(misc_tv2dbl(&prefetched) < misc_tv2dbl(&(cmd->tp)) + misc_tv2dbl(&(sh->tick))){
		return FI_SCC_IMMEDIATE;
	}

	return FI_SCC_NEXTTICK;
}

int FI_ticked_loop(FI_info_t *sh){
	FI_sce_connectivity_e sc;
	int cc;
	FI_command_t scmd;
	sigset_t timer;
//	char *ccmd;
	int f_rsce = FI_TRUE, f_rcon = FI_TRUE;
	int catched_sig = -1;
	char ccmd[XPT_CP_MAXCOMLEN];
	char payload[XPT_CP_MAXPAYLOAD];

	sigemptyset(&timer);
	sigaddset(&timer, FI_TICKING_SIGNAL);
//	while(sh->survive == FI_TRUE){
	while(1){
		if(f_rsce == FI_TRUE){
			sc = FI_fetch_scenario(sh, &scmd);
			if(sc != FI_SCC_NOEVENT)
				FI_event_handler_for_sce(sh, &(scmd));
			if((sc == FI_SCC_NEXTTICK) || (sc == FI_SCC_NOEVENT) || (sc == FI_SCC_END))
				f_rsce = FI_FALSE;
		}

		if(f_rcon == FI_TRUE){

			//cc = XPT_poll_pull_event(&(sh->xptinfo));
			cc = XPT_CP_poll_pull_event(&(sh->xptinfo), ccmd, payload);
//			cc = FI_VC_poll_pull_event_i(&(sh->vcinfo), &ccmd);

			DEBUGF(100, "POLL:%d\n", cc);
			if (cc == XPT_TRUE){
				DEBUGF(10, "ccmd=%s, payload=%s\n", ccmd, payload);
				FI_event_handler_for_con(sh, ccmd, payload);
			}else if (cc == XPT_FALSE){
				f_rcon = FI_FALSE;
			}
		}

		DEBUGF(100, "TICK: %d, %d(%d)\n", f_rcon, f_rsce, sc);
		if((f_rsce == FI_FALSE) && (f_rcon == FI_FALSE)){
			DEBUGF(100,"TICK: %d, %d(%d)\n", f_rcon, f_rsce, sc);
			f_rsce = FI_TRUE;
			f_rcon = FI_TRUE;
			sigwait(&timer, &catched_sig);
			DEBUGF(100,"TICKOUT: %d, %d(%d)\n", f_rcon, f_rsce, sc);
		}

	}
	return EXIT_SUCCESS;
}

#define __HDL_ENTRY__
/* tentative version */
int FI_event_handler_for_sce(FI_info_t *sh, FI_command_t *cmd){
	DEBUGF(10, "DEBUG: %s", cmd->command);

	FI_SCE_EVENT_CMP(cmd, "BLT_FAILURERATE"){
		FI_evhdls_blt_failurerate(sh, cmd);
	}
	else FI_SCE_EVENT_CMP(cmd, "BLT_SETTICK"){
		FI_evhdls_blt_settick(sh, cmd);
	}
	else FI_SCE_EVENT_CMP(cmd, "BLT_STARTTICK"){
		FI_evhdls_blt_starttick(sh, 0);
	}

	else{
		if(sh->sfunc != NULL){
			sh->sfunc(sh, cmd);
		}
	}

	return FI_TRUE;
}

int FI_event_handler_for_con(FI_info_t *sh, char* cmd, char* payload){
//	FI_VC_info_t *vch = &(sh->vcinfo);
//	XPT_info_t *xch = &(sh->xptinfo);
//	char* payload = FI_VC_get_pull_event_payload_pc(vch);
	FI_CON_EVENT_CMP(cmd, "BLT_INVOKE"){
		FI_evhdlc_blt_invoke(sh, cmd, payload);
	}
	else FI_CON_EVENT_CMP(cmd, "BLT_RESTART"){
		FI_evhdlc_blt_restart(sh, cmd, payload);
	}

	else{
		if(sh->cfunc != NULL){
			sh->cfunc(sh, cmd, payload);
		}
	}
	return FI_TRUE;
}
#undef __HDL_ENTRY__

#define __EVHDLS__
/* evhdls */
int FI_evhdls_blt_settick(FI_info_t *sh, FI_command_t *cmd){
	sscanf(cmd->args, "%ld %ld", &(sh->tick.tv_sec), &(sh->tick.tv_usec));
	return FI_TRUE;
}

int FI_evhdls_blt_failurerate(FI_info_t *sh, FI_command_t *cmd){
//	DEBUGF(0, "failure rate %s\n", cmd->args);
	sscanf(cmd->args, "%lf", &(sh->fr));
//	DEBUGF(0, "failure rate %lf\n", sh->fr);
	return FI_TRUE;
}

/* timer start by SCENARIO_APPROVAL && APPLICATION_APPROVAL (2count) */
int FI_evhdls_blt_starttick(FI_info_t *sh, int starttime_reset){
	struct itimerspec ispec;
	static int count = 0;
	static int sreset = 0;

/*
	if(count != 1){
		count++;
		return FI_TRUE;
	}
	*/

	//fprintf(stderr,"starttick\n");

	ispec.it_value.tv_sec = sh->tick.tv_sec;
	ispec.it_value.tv_nsec = sh->tick.tv_usec * 1000;
	ispec.it_interval.tv_sec = sh->tick.tv_sec;
	ispec.it_interval.tv_nsec = sh->tick.tv_usec * 1000;

	if((sreset+starttime_reset) != 0){
		FI_timer_set_start(sh);
	}

	ERRCHK(timer_settime(sh->tick_id, 0, &ispec, NULL) < 0, "timer_settime");
	sreset = starttime_reset;
	count++;
	return FI_TRUE;
}
#undef __EVHDLS__

#define __EVHDLC__
/* evhdlc */
/* now these built-in event is not necessary */
int FI_evhdlc_blt_invoke(FI_info_t *vch, char* cmd, char* payload){
	assert(0);
	return FI_TRUE;
}
int FI_evhdlc_blt_restart(FI_info_t *vch, char* cmd, char* payload){
	assert(0);
	return FI_TRUE;
}
#undef __EVHDLC__

#define __MISCS__
void FI_timer_set_start(FI_info_t *sh){
	gettimeofday(&(sh->st), NULL);
}
void FI_timer_get_from_start(FI_info_t *sh, struct timeval *tv){
	struct timeval now;
	gettimeofday(&now, NULL);
	timersub(&now, &(sh->st), tv);
}

#undef __MISCS__


