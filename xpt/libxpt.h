#ifndef LIBXPT_H
#define LIBXPT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <pthread.h>
#include <assert.h>

#include "utilmacros.h"

#include "libxpti.h"

//#define XPT_SIGNAL_STYLE //Old Version flag
#define XPT_TRUE 		XPTI_TRUE
#define XPT_FALSE 		XPTI_FALSE
#define XPT_UNEXPECTED  XPTI_UNEXPECTED
#define XPT_JMPBACK		1

#define MAX_XPT_PAYLOAD XPTI_MAXPAYLOAD
#define MAX_XPT_EVENT	XPTI_MAXEVENT

#define XPT_ROLE_ONEOFTHE_WORLD 	XPTI_ROLE_ONEOFTHE_WORLD
#define XPT_ROLE_ONEOFTHE_JOB		XPTI_ROLE_ONEOFTHE_JOB
#define XPT_ROLE_ONEOFTHE_NODE		XPTI_ROLE_ONEOFTHE_NODE
#define XPT_ROLE_OTHERS				XPTI_ROLE_OTHERS

#define DEF_XPT_ROOTIP	"127.0.0.1"
// Macro for Directive
/* usage TRY-CATCH
 * XPT_TRY(xptinfo){
 *   ..........
 * } XPT_CATCH(xptinfo, "EVENT1") {
 *   ..........
 *   XPT_CATCHED(xptinfo);
 * } XPT_CATCH(xptinfo, "EVENT2") {
 *   ..........
 *   XPT_CATCHED(xptinfo);
 * }
 */
#define XPT_TRY(xptinfo_p) if(sigsetjmp((xptinfo_p)->sjmp, 1) == 0)
#define XPT_CATCH(xptinfo_p, event_p) else if(strcmp((xptinfo_p)->pushed_event, (event_p)) == XPT_JMPBACK)
#define XPT_CATCHED(xptinfo_p) XPT_push_done(xptinfo_p)

/* usage POLL-CHECK
 * XPT_POLL(xptinfo){
 *   XPT_CHECK(xptinfo, "EVENT1") {
 *      ..........
 *      XPT_CHECKED(xptinfo);
 *   }else XPT_CHECK(xptinfo, "EVENT2") {
 *      ..........
 *      XPT_CHECKED(xptinfo);
 *   }
 * }
 */
#define XPT_POLL(xptinfo_p) while(XPT_poll_pull_event(xptinfo_p) == XPT_TRUE)
#define XPT_CHECK(xptinfo_p, event_p) if(strcmp((xptinfo_p)->pulled_event, (event_p)) == 0)
#define XPT_CHECKED(xptinfo_p) XPT_pull_done(xptinfo_p)


typedef enum XPT_recv_event_state_ {
	XPT_RES_NULL,
	XPT_RES_RECIEVED,
	XPT_RES_PROCESSING, // does it need ??

	MAX_XPT_RES,
} XPT_recv_event_state_e;

typedef struct XPT_info_ {
	pthread_t parent_tid;
	sigjmp_buf sjmp;
	int ckpt_rdy;
	volatile sig_atomic_t pushed;
	char* pushed_event;
	char* pulled_event;
	char* pushed_payload;
	char* pulled_payload;
	XPT_recv_event_state_e restate_push;
	XPT_recv_event_state_e restate_pull;
	XPTI_info_t iinfo;
} XPT_info_t;

int XPT_init(XPT_info_t *xptinfo, const char* event_name);
int XPT_phdl_init(XPT_info_t *xptinfo, int n_phdl);
int XPT_pull_subscribe(XPT_info_t *xptinfo, char* signal, char* extra_attribute);
int XPT_push_subscribe(XPT_info_t *xptinfo, char* signal, char* extra_attribute);
int XPT_pull_unsubscribe(XPT_info_t *xptinfo);
int XPT_push_unsubscribe(XPT_info_t *xptinfo);
int XPT_declare_publishable_events(XPT_info_t *xptinfo, char* signal, char* serverity);
int XPT_declare_publishable_events_with_schemafile(XPT_info_t *xptinfo, char* filename);
int XPT_publish(XPT_info_t *xptinfo, char* signal, int tag, char* payload);
int XPT_poll_pull_event(XPT_info_t *xptinfo);
int XPT_poll_pull_event_with_decode(XPT_info_t *sh, char* signal, char* payload);
char* XPT_poll_get_payload(XPT_info_t *sh);
char* XPT_poll_get_event(XPT_info_t *sh);
int XPT_poll_push_event(XPT_info_t *xptinfo);
char* XPT_push_get_payload(XPT_info_t *sh);
char* XPT_push_get_event(XPT_info_t *sh);
int XPT_pull_done(XPT_info_t *xptinfo);
int XPT_push_done(XPT_info_t *xptinfo);
int XPT_fini(XPT_info_t *xptinfo);
void XPT_exec_coprocess(int process_role);
void XPT_killnwait_coprocess(int process_role);
void XPT_coserver_exec_complete();
//void XPT_checknread(FILE *fp, int *checker);
//void XPT_tentative_serializer(FILE *fp, HPL_T_pmat * A);
//void XPT_tentative_deserializer(FILE *fp, HPL_T_pmat * A);

//static void SXPT_init_info(XPT_info_t *xptinfo);

#endif
