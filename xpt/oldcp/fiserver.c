/*
 * fiserver.c
 *
 *  Created on: 2012/07/03
 *      Author: RyzeVia
 */
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libfi.h"

#define INT_EPS 0.8

FI_info_t fiinfo;

int sce_event_handler(FI_info_t *sh, FI_command_t *cmd);
int con_event_handler(FI_info_t *sh, char* cmd, char* payload);
int ehdl_failure(FI_info_t *sh);
int ehdl_timerstart(FI_info_t *sh);
int ehdl_setckpt(FI_info_t *sh, char* payload);
int ehdl_ckpttime(FI_info_t *sh, char* payload);
int ehdl_lastline(FI_info_t *sh, char* payload);
int ehdl_timestamp(FI_info_t *sh, char* payload);
int ehdl_restart_ready(FI_info_t *sh);
int ehdl_finish_all(FI_info_t *sh);
void exec_target(FI_info_t *sh);
double f_opt(double time, double fr, double ctime);
double df_opt(double time, double fr, double ctime);
int ckpt_interval_optimization(FI_info_t *sh, double* newval);
void chld_handler(int sig_no);

int recalc = FI_FALSE;

int main(int argc, char **argv){
	struct sigaction act_child;

	if(argc < 3){
		ERRORF("usage: fiserver <scenario file> <target command>\n");
		exit(EXIT_FAILURE);
	}


    act_child.sa_handler = chld_handler;
    sigemptyset(&act_child.sa_mask);
    act_child.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction(SIGCHLD, &act_child, NULL);

	FI_init(&fiinfo, NULL);

	/*
	FI_declare_event(&fiinfo, "FI_INTERVAL_CHANGE");
	FI_declare_event(&fiinfo, "FI_RESTART");
	FI_VC_pull_subscribe_i(&(fiinfo.vcinfo), "FI_TIMERSTART", NULL);
	FI_VC_pull_subscribe_i(&(fiinfo.vcinfo), "FI_SETCKPT", NULL);
	FI_VC_pull_subscribe_i(&(fiinfo.vcinfo), "FI_CKPTTIME", NULL);
	FI_VC_pull_subscribe_i(&(fiinfo.vcinfo), "FI_LASTLINE", NULL);
	FI_VC_pull_subscribe_i(&(fiinfo.vcinfo), "FI_RESTART_READY", NULL);
	FI_VC_pull_subscribe_i(&(fiinfo.vcinfo), "FI_FINISH_ALL", NULL);
*/


	FI_open_scenario(&fiinfo, argv[1]);


	fiinfo.sfunc = sce_event_handler;
	fiinfo.cfunc = con_event_handler;

	fiinfo.target_cmd = &(argv[2]);
	fiinfo.target_argc = argc-2;
	exec_target(&fiinfo);


	FI_ticked_loop(&fiinfo);

	return EXIT_SUCCESS;
}

int sce_event_handler(FI_info_t *sh, FI_command_t *cmd){
	FI_SCE_EVENT_CMP(cmd, "FI_FAILURE"){
		ehdl_failure(sh);
	}
	return FI_TRUE;
}

int con_event_handler(FI_info_t *sh, char* cmd, char* payload){
	// turn on flag for timerstart (application ready)
	FI_CON_EVENT_CMP(cmd, "FI_TIMERSTART"){
		ehdl_timerstart(sh);
	}
	else FI_CON_EVENT_CMP(cmd, "FI_SETCKPT"){
		ehdl_setckpt(sh, payload);
	}
	// re-calculation request from application
	else FI_CON_EVENT_CMP(cmd, "FI_CKPTTIME"){
		ehdl_ckpttime(sh, payload);
	}
	// last line on loop
	else FI_CON_EVENT_CMP(cmd, "FI_LASTLINE"){
		ehdl_lastline(sh, payload);
	}
	// output elapsed time
	else FI_CON_EVENT_CMP(cmd, "FI_TIMESTAMP"){
		ehdl_timestamp(sh, payload);
	}
	// application start to checkpoint mechanism and restart enabled
	else FI_CON_EVENT_CMP(cmd, "FI_RESTART_READY"){
		ehdl_restart_ready(sh);
	}
	else FI_CON_EVENT_CMP(cmd, "FI_FINISH_ALL"){
		ehdl_finish_all(sh);
	}
	return FI_TRUE;
}

int ehdl_failure(FI_info_t *sh){
	/* kill process & restart process */
	kill(sh->target_pid, SIGKILL);
	sh->restart = FI_TRUE;
	exec_target(sh);
	return FI_TRUE;
}


int ehdl_timerstart(FI_info_t *sh){
	FI_evhdls_blt_starttick(&fiinfo, 1);
	return FI_TRUE;
}

int ehdl_setckpt(FI_info_t *sh, char* payload){
	sscanf(payload, "CKPT_COUNT=%d", &(sh->ckptc));
	CALLOC(struct timeval*, sh->stamp_epoch, sh->ckptc, sizeof(struct timeval));
	CALLOC(struct timeval*, sh->stamp_ckpt, sh->ckptc, sizeof(struct timeval));

	XPT_CP_publish_to_cli(&(sh->xptinfo), "FI_STARTAPP", NULL);

	return FI_TRUE;
}

int ehdl_ckpttime(FI_info_t *sh, char* payload){
	int nckpt;
	int iter;
	struct timeval etv, ctv;
	sscanf(payload, "ITER=%d,NCKPT=%d,ESEC=%ld,EUSEC=%ld,CSEC=%ld,CUSEC=%ld",
			&iter, &nckpt, &(etv.tv_sec), &(etv.tv_usec), &(ctv.tv_sec), &(ctv.tv_usec));

	sh->stamp_epoch[nckpt] = etv;
	sh->stamp_ckpt[nckpt] = ctv;

	return FI_TRUE;
}

int ehdl_lastline(FI_info_t *sh, char* payload){
	int nckpt;
	double eps = INT_EPS;
	struct timeval etv, ctv;
	double newval, oldval;
	char sendpayload[XPT_CP_MAXPAYLOAD];
	sscanf(payload, "ITER=%d,NCKPT=%d,ESEC=%ld,EUSEC=%ld,CSEC=%ld,CUSEC=%ld",
			&(sh->iter), &nckpt, &(etv.tv_sec), &(etv.tv_usec), &(ctv.tv_sec), &(ctv.tv_usec));

	sh->stamp_epoch[nckpt] = etv;
	sh->stamp_ckpt[nckpt] = ctv;


	ckpt_interval_optimization(sh, &newval);

	oldval = misc_tv2dbl(&(sh->ckpt_interval));

	if(fabs(newval - oldval)/oldval > eps){
		misc_dbl2tv(newval, &(sh->ckpt_interval));
		sprintf(sendpayload, "ITER=%d,SEC=%ld,USEC=%ld", sh->iter, sh->ckpt_interval.tv_sec, sh->ckpt_interval.tv_usec);
		XPT_CP_publish_to_cli(&(sh->xptinfo), "FI_INTERVAL_CHANGE", sendpayload);
	}

//	FI_VC_publish_i(&(sh->vcinfo), "FI_INTERVAL_CHANGE", sendpayload);

	return FI_TRUE;
}

int ehdl_timestamp(FI_info_t *sh, char* payload){
	int ioh;
	struct timeval cur;
	static double prevs = 0.;
	double curd, elpd;
	FILE *iostream;
	char buf[FI_VC_MAXPAYLOAD];
	sscanf(payload, "STREAM=%d,MSG=%s", &ioh, buf);

	switch(ioh){
	case 0:
		iostream = stdout;
		break;
	case 1:
		iostream = stderr;
		break;
	default:
		iostream = stdout;
		break;
	}

	FI_timer_get_from_start(sh, &cur);
	curd = misc_tv2dbl(&cur);
	elpd = curd - prevs;
	fprintf(iostream, "%10f(%10f):%s\n", curd, elpd, buf);

	prevs = curd;

	return FI_TRUE;
}

int ehdl_restart_ready(FI_info_t *sh){
	/* restert operation */
	if(sh->restart == FI_TRUE){
		FILE *fp;
		char buf[16];
		int phase;
		fp = fopen("scephase", "r");
		fread(&phase, sizeof(int), 1, fp);
		fclose(fp);

		sprintf(buf, "%d", phase);

		XPT_CP_publish_to_cli(&(sh->xptinfo), "FI_RESTART", buf);
//		FI_VC_publish_i(&(sh->vcinfo), "FI_RESTART", buf);
		sh->restart = FI_FALSE;
	}
	return FI_TRUE;
}

int ehdl_finish_all(FI_info_t *sh){
	exit(EXIT_SUCCESS);
	return FI_TRUE;
}


void exec_target(FI_info_t *sh){
	sh->target_pid = fork();
	ERRCHK(sh->target_pid == -1, "exec target");
	if(sh->target_pid == 0) {
		execvp((sh->target_cmd)[0], sh->target_cmd);
		ERRORF("execvp failed");
		exit(EXIT_FAILURE);
	}
}

#define NEWTON_MAX 1000
#define NEWTON_EPS 1.0e-5
#define NEWTON_START 1.

double f_opt(double time, double fr, double ctime){
	return  exp(fr*(time+ctime))*(1 - fr * time) - 1;
}

double df_opt(double time, double fr, double ctime){
	double fst = fr * fr * time;

	return -fr * fr * time * exp( fr * (ctime+time));
}

int newton(double init, double fr, double ctime, double *result){
	int iter = 0;
	int done = 0;
	static int flag = 0;
	double cand = init;
	double fo;
	double dfo;
	*result = 0;

	if(flag == 0){
		cand = init = NEWTON_START;
		flag = 1;
	}


	while(!done){
		iter++;
		fo = f_opt(cand, fr, ctime);
		dfo = df_opt(cand, fr, ctime);
		*result = cand - fo / dfo;

		if(fabs(*result - cand)  < NEWTON_EPS){
			done = 1;
		}
		cand = *result;

		if(!done && (iter > NEWTON_MAX)){
			return -1;
		}
	}

	return iter;
}

int ckpt_interval_optimization(FI_info_t *sh, double *new){
	int i;
	struct timeval maxtv;
	maxtv.tv_sec = -1;
	maxtv.tv_usec = 0;

	for(i = 0; i < sh->ckptc; i++){
		if(timercmp(&maxtv, &(sh->stamp_ckpt[i]), <) != 0){
			maxtv = sh->stamp_ckpt[i];
		}
	}

	newton(misc_tv2dbl(&(sh->ckpt_interval)), sh->fr, misc_tv2dbl(&maxtv), new);

	return FI_TRUE;
}

void chld_handler(int sig_no)
{
    pid_t   pid_child = 0;

    do {
        int ret_child;
        pid_child = waitpid(-1, &ret_child, WNOHANG);
    } while(pid_child > 0);
}
