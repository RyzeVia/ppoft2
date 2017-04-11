/*
 * libxpt_ckpt.h
 *
 *  Created on: 2012/07/07
 *      Author: RyzeVia
 */

#ifndef LIBXPT_CP_H_
#define LIBXPT_CP_H_

#include "libxpt.h"

#define XPT_CP_MAXCOMLEN	(18)

//"cmd= payload= " = 14char
#define XPT_CP_MAXPAYLOAD	(XPTI_MAXPAYLOAD-XPT_CP_MAXCOMLEN-14)



int XPT_CP_cli_init(XPT_info_t *sh);
int XPT_CP_srv_init(XPT_info_t *sh);
//static int SXPT_CP_publish(XPT_info_t *sh, char* msgtype, char* signal, char* payload);
int XPT_CP_publish_to_cli(XPT_info_t *sh, char* signal, char* payload);
int XPT_CP_publish_to_srv(XPT_info_t *sh, char* signal, char* payload);
int XPT_CP_publish_to_all(XPT_info_t *sh, char* signal, char* payload);

/*
int XPT_CP_cli_ready(XPT_info_t *sh, double thres_rate, int ckptcount);
int XPT_CP_sready(XPT_info_t *sh, double thres_rate, int ckptcount);
*/

int XPT_CP_ready_root(XPT_info_t *sh, double thres_rate, int ckptcount);
int XPT_CP_ready_nodes(XPT_info_t *sh, double thres_rate, int ckptcount);






int XPT_CP_preexec(XPT_info_t *sh, int iter, int phase);
int XPT_CP_postexec(XPT_info_t *sh, int iter, int phase);
int XPT_CP_commit(XPT_info_t *sh, int iter, int phase);
int XPT_CP_pull_poll(XPT_info_t *sh, int iter, int phase);
int XPT_CP_epoch_time(XPT_info_t *sh, struct timeval *tv, int phase);
int XPT_CP_ckpt_time(XPT_info_t *sh, struct timeval *tv, int phase);
int XPT_CP_ckpt(XPT_info_t *sh, char* fname, int iter, int phase, void (*cwrite)(FILE* fp, int phase));
int XPT_CP_poll_pull_event(XPT_info_t *sh, char* cmd, char* payload);


#endif /* LIBXPT_CKPT_H_ */
