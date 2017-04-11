/*
 * XPT_ckpt.c
 *
 *  Created on: 2012/07/07
 *      Author: RyzeVia
 */
#define DEBUG_ENABLE
#define DEBUG_LEVEL DEBUG_LEVEL_MUST

#include "libxpt.h"
#include "libxpt_cp.h"

/*
struct timeval *epoch_stamp;
struct timeval *ckpt_stamp;
struct timeval *cepoch_stamp;
struct timeval *cckpt_stamp;

struct timeval prev, epoch, ckpt;
double thres;
int count = 0;
int ckptc;
int renew = 0;
int phase_renew = 0;

int firstloop = 1;
*/
// new arch
#define LIM_SEC	30
#define LIM_USEC	0

int f_1st = 1;
struct timeval *epoch_term;
struct timeval *ckpt_term;
struct timeval *cur_epoch;
struct timeval *cur_ckpt;
struct timeval el_epoch;
struct timeval el_ckpt;
struct timeval prev_commit;
struct timeval laststamp;
struct timeval firststamp;
double publish_threshold;
int nckpt;
struct timeval sendlimit = { LIM_SEC, LIM_USEC };

int biter_ = 0, biter = 0;
double interval_ = 0., interval = 0.;
double curtime = 0.;
int renewal = 0;
int *commit;

int XPT_CP_cli_init(XPT_info_t *sh) {
	XPTI_phdl_init(&(sh->iinfo), 2);
	XPT_pull_subscribe(sh, "XPT_SRV2CLI", NULL);
	XPT_pull_subscribe(sh, "XPT_TOALL", NULL);
	XPT_declare_publishable_events(sh, "XPT_CLI2SRV", "INFO");
	XPT_declare_publishable_events(sh, "XPT_TOALL", "INFO");
	return XPT_TRUE;
}

int XPT_CP_srv_init(XPT_info_t *sh) {

	XPTI_phdl_init(&(sh->iinfo), 2);
	XPT_pull_subscribe(sh, "XPT_CLI2SRV", NULL);
	XPT_pull_subscribe(sh, "XPT_TOALL", NULL);
	XPT_declare_publishable_events(sh, "XPT_SRV2CLI", "INFO");
	XPT_declare_publishable_events(sh, "XPT_TOALL", "INFO");

	return XPT_TRUE;
}

static int SXPT_CP_publish(XPT_info_t *sh, char* msgtype, char* signal, char* payload){
	char pl[XPTI_MAXPAYLOAD];


	if(payload == NULL){
		DEBUGF(DEBUG_LEVEL_INFO , "publish: cmd=%s\n", signal);
		sprintf(pl, "cmd=%s", signal);
	}else{
		DEBUGF(DEBUG_LEVEL_INFO , "publish: cmd=%s payload=%s\n", signal, payload);
		sprintf(pl, "cmd=%s payload=%s", signal, payload);
	}
	return XPT_publish(sh, msgtype, 1, pl);
}

int XPT_CP_publish_to_cli(XPT_info_t *sh, char* signal, char* payload){
	return SXPT_CP_publish(sh, "XPT_SRV2CLI", signal, payload);
}
int XPT_CP_publish_to_srv(XPT_info_t *sh, char* signal, char* payload){
	return SXPT_CP_publish(sh, "XPT_CLI2SRV", signal, payload);
}
int XPT_CP_publish_to_all(XPT_info_t *sh, char* signal, char* payload){
	return SXPT_CP_publish(sh, "XPT_TOALL", signal, payload);
}

int XPT_CP_ready_root(XPT_info_t *sh, double thres_rate, int ckptcount) {
	char payload[XPTI_MAXPAYLOAD];

	sprintf(payload, "CKPT_COUNT=%d", ckptcount);
	XPT_CP_publish_to_srv(sh, "FI_SETCKPT", payload);

	XPT_CP_ready_wait(sh);

	XPT_CP_publish_to_srv(sh, "FI_RESTART_READY", NULL);

	nckpt = ckptcount;

	CALLOC(struct timeval*, cur_epoch, nckpt, sizeof(struct timeval));
	CALLOC(struct timeval*, cur_ckpt, nckpt, sizeof(struct timeval));
	CALLOC(struct timeval*, epoch_term, nckpt, sizeof(struct timeval));
	CALLOC(struct timeval*, ckpt_term, nckpt, sizeof(struct timeval));
	CALLOC(int*, commit, nckpt, sizeof(int));

	sh->ckpt_rdy = XPT_TRUE;
	publish_threshold = thres_rate;


	gettimeofday(&firststamp, NULL);
	laststamp = firststamp;

	DEBUGF(DEBUG_LEVEL_INFO+1, "laststamp %ld:%ld\n", TVPRINTF(&laststamp));

	return XPT_TRUE;

}

int XPT_CP_ready_nodes(XPT_info_t *sh, double thres_rate, int ckptcount) {
	nckpt = ckptcount;

	CALLOC(struct timeval*, cur_epoch, nckpt, sizeof(struct timeval));
	CALLOC(struct timeval*, cur_ckpt, nckpt, sizeof(struct timeval));
	CALLOC(struct timeval*, epoch_term, nckpt, sizeof(struct timeval));
	CALLOC(struct timeval*, ckpt_term, nckpt, sizeof(struct timeval));
	CALLOC(int*, commit, nckpt, sizeof(int));

	XPT_CP_ready_wait(sh);

	sh->ckpt_rdy = XPT_TRUE;
	publish_threshold = thres_rate;

	return XPT_TRUE;

}

int XPT_CP_preexec(XPT_info_t *sh, int iter, int phase) {
	struct timeval now;

	gettimeofday(&now, NULL);

	timersub(&now, &laststamp, &(cur_epoch[phase]));
	laststamp = now;

	return XPT_TRUE;
}



int XPT_CP_postexec(XPT_info_t *sh, int iter, int phase) {
	struct timeval now, resend;
	char *cmd;
	char payload[XPT_CP_MAXPAYLOAD];
	int i;

	gettimeofday(&now, NULL);
	laststamp = now;

	/* re-send algorithm */
	if (phase + 1 == nckpt) {
		timersub(&now, &prev_commit, &resend);
		if((f_1st == 1) || (timercmp(&resend, &sendlimit, >) != 0)){

			for(i = 0; i < nckpt; i++){
				if(i == nckpt - 1){
					cmd = "FI_LASTLINE";
				}else{
					cmd = "FI_CKPTTIME";
				}

				sprintf(payload, "ITER=%d,NCKPT=%d,ESEC=%ld,EUSEC=%ld,CSEC=%ld,CUSEC=%ld",
						iter, i, TVPRINTF(&(cur_epoch[i])), TVPRINTF(&(cur_ckpt[i])));

				/* FIXME: IF this needs */
				epoch_term[i] = cur_epoch[i];
				ckpt_term[i] = cur_ckpt[i];

				XPT_CP_publish_to_all(sh, cmd, payload);
			}

			prev_commit = now;
			f_1st = 0;
		}
	}

	laststamp = now;

	return XPT_TRUE;
}

int XPT_CP_commit(XPT_info_t *sh, int iter, int phase){
	int i;
	int check = 0;

	/*//FIXED: may be all ckpttime received before interval change
	while(check == 0){
		check = 1;
		for(i = 0; i < nckpt; i++){
			check *= commit[i];
		}
		XPT_CP_pull_poll(sh, iter, phase);
	}
	check = 0;
	 */

	for(i = 0; i < nckpt; i++){
			epoch_term[i] = cur_epoch[i];
			ckpt_term[i] = cur_ckpt[i];

			DEBUGF(0, "epoch_term[%d]:%ld.%ld\n", i, TVPRINTF(&epoch_term[i]));

			check = 1;
			commit[i] = 0;
	}

	interval = interval_;
	biter = biter_;

	return XPT_TRUE;
}

int XPT_CP_pull_poll(XPT_info_t *sh, int iter, int phase) {
	struct timeval etv, ctv, ival;
//	char *payload = sh->pulled_payload;
	char payload[XPT_CP_MAXPAYLOAD];
	char cmd[XPT_CP_MAXCOMLEN];

	int cpp;



	while (XPT_TRUE == XPT_CP_poll_pull_event(sh, cmd, payload)) {

		DEBUGF(DEBUG_LEVEL_TRIVIAL, "PAYLOAD=%s\n", payload);

		if ((strncmp(cmd, "FI_LASTLINE", 11) == 0)
		|| (strncmp(cmd, "FI_CKPTTIME", 11) == 0)){
			sscanf(payload,
					"ITER=%d,NCKPT=%d,ESEC=%ld,EUSEC=%ld,CSEC=%ld,CUSEC=%ld",
					&biter_, &cpp, TVSCANF(&etv), TVSCANF(&ctv));
			commit[cpp] = 1;
			cur_epoch[cpp] = etv;
			cur_ckpt[cpp] = ctv;
		}
		else if(strncmp(cmd, "FI_INTERVAL_CHANGE", 18) == 0){
			sscanf(payload, "ITER=%d,SEC=%ld,USEC=%ld",
					&biter_, TVSCANF(&ival));
			interval_ = misc_tv2dbl(&ival);

			DEBUGF(0, "new interval has comming %f\n", interval_);
			renewal = 1;
		}
	}

	if(renewal == 1){
		XPT_CP_commit(sh, iter, phase);
	}


	if(f_1st == 1)
		f_1st = 0;

	return XPT_TRUE;
}


int XPT_CP_ckpt(XPT_info_t *sh, char* fname, int iter, int phase, void (*cwrite)(FILE* fp, int phase)){
	char header[32];
	struct timeval now;
	FILE *fp;

	gettimeofday(&now, NULL);
	laststamp = now;


	if(renewal == 1){

		DEBUGF(0, "recalc interval with iter:%d siter:%d phase:%d nckpt:%d\n",
				iter, biter, phase, nckpt);


		if(iter < biter){
			return XPT_FALSE;
		}

		int i;
		int last = (iter - biter) * nckpt + phase - 1;
		curtime = 0;

		DEBUGF(0, "recalc phase calc:%d (it %d,ph %d)\n", last, iter, phase);

		for(i = 0; i < last; i++){
			curtime += misc_tv2dbl(&(epoch_term[i%nckpt]));
			if(curtime > interval)
				curtime -= interval;
		}

		DEBUGF(0, "result curtime: %f\n", curtime);
		renewal = 0;
	}

	curtime += misc_tv2dbl(&(epoch_term[phase]));
//	DEBUGF(0, "ckpt curtime: %f\n", curtime);

	if ((curtime > interval) || (f_1st == 1)){
		fp = fopen(fname, "w");

		sprintf(header, "%d,%d", phase, iter);
		fwrite(header, sizeof(char), 32, fp);
		cwrite(fp, phase);
		fclose(fp);

		curtime -= interval;

		gettimeofday(&now, NULL);
		timersub(&now, &laststamp, &(cur_ckpt[phase]));
		return XPT_TRUE;
	}


	return XPT_FALSE;

}

int XPT_CP_poll_pull_event(XPT_info_t *sh, char* cmd, char* payload){
	int cc = XPT_poll_pull_event(sh);
	if(cc == XPT_TRUE){

		DEBUGF(20, "PAYLOAD=%s\n", sh->pulled_payload);
		sscanf(sh->pulled_payload, "cmd=%s payload=%s", cmd, payload);
		DEBUGF(20, "PAYLOAD_CHK=%s\n", payload);
	}
	return cc;
}

int XPT_CP_ready_wait(XPT_info_t *sh){
	char cmd[XPT_CP_MAXCOMLEN];
	char payload[XPT_CP_MAXPAYLOAD];
	int cc = XPT_FALSE;
	int f_cmdcheck = 0;

	while((cc != XPT_TRUE) && (f_cmdcheck == 0)){
		cc = XPT_CP_poll_pull_event(sh, cmd, payload);
		if((cc == XPT_TRUE) && (strcmp(cmd, "FI_STARTAPP") == 0)){
			f_cmdcheck = 1;
			DEBUGF(DEBUG_LEVEL_TRIVIAL, "WELLCOME %s %s\n", cmd, payload);
		}
	}

	return XPT_TRUE;
}


/*
int XPT_CP_epoch_time(XPT_info_t *sh, struct timeval *tv, int phase) {
	*tv = cepoch_stamp[phase];
	return XPT_TRUE;
}

int XPT_CP_ckpt_time(XPT_info_t *sh, struct timeval *tv, int phase) {
	*tv = cckpt_stamp[phase];
	return XPT_TRUE;
}
*/
