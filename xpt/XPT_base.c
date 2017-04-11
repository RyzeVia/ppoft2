//#define XPT_ENABLE
//#ifdef XPT_ENABLE
/*
 * Include XPT hard-code header
 */


#define FTBDEBUG
#include "libxpt.h"

/*
 * Hard Coded XPT Prototype
 */

static void SXPT_init_info(XPT_info_t *xptinfo){
	memset(xptinfo, 0, sizeof(XPT_info_t));
	xptinfo->parent_tid = pthread_self();
	xptinfo->restate_pull = XPT_RES_NULL;
	xptinfo->restate_push = XPT_RES_NULL;
	xptinfo->pushed = XPTI_RES_NULL;
	xptinfo->pushed_event = NULL;
	xptinfo->pulled_event = NULL;
	xptinfo->ckpt_rdy = XPT_FALSE;

	return;
}

int XPT_init(XPT_info_t *xptinfo, const char* event_name){
	char* rootip = getenv("XPT_ROOT_IP");
	if(rootip == NULL){
		rootip = DEF_XPT_ROOTIP;
	}
	setenv("XPTI_ROOT_IP", rootip, 1);

	SXPT_init_info(xptinfo);
	return XPTI_init(&(xptinfo->iinfo), event_name);
}

int XPT_phdl_init(XPT_info_t *xptinfo, int n_phdl){
	return XPTI_phdl_init(&(xptinfo->iinfo), n_phdl);
}

int XPT_pull_subscribe(XPT_info_t *xptinfo, char* signal, char* extra_attribute){
	return XPTI_pull_subscribe(&(xptinfo->iinfo), signal, extra_attribute);
}

int XPT_push_subscribe(XPT_info_t *xptinfo, char* signal, char* extra_attribute){
	return XPTI_push_subscribe(&(xptinfo->iinfo), signal, extra_attribute);
}

int XPT_pull_unsubscribe(XPT_info_t *xptinfo){
	return XPTI_pull_unsubscribe(&(xptinfo->iinfo));
}

int XPT_push_unsubscribe(XPT_info_t *xptinfo){
	return XPTI_push_unsubscribe(&(xptinfo->iinfo));
}

int XPT_declare_publishable_events(XPT_info_t *xptinfo, char* signal, char* serverity){
	return XPTI_declare_publishable_events(&(xptinfo->iinfo), signal, serverity);
}

int XPT_declare_publishable_events_with_schemafile(XPT_info_t *xptinfo, char* filename){
	return XPTI_declare_publishable_events_with_schemafile(&(xptinfo->iinfo), filename);
}

int XPT_publish(XPT_info_t *xptinfo, char* signal, int tag, char* payload){
	return XPTI_publish(&(xptinfo->iinfo), signal, tag, payload);
}

int XPT_poll_pull_event(XPT_info_t *xptinfo){
	//ERRCHK(xptinfo->ckpt_rdy == XPT_FALSE, "XPT is not initialized.\n");

	if(xptinfo->restate_pull != XPT_RES_NULL){
		DEBUGF(50, "polling event is not processed.\n");
		return XPT_FALSE;
	}
	if(XPTI_poll_pull_event(&(xptinfo->iinfo), &(xptinfo->pulled_event)) == XPTI_FALSE){
		DEBUGF(50, "polling event was fail.\n");
		return XPT_FALSE;
	}

	xptinfo->pulled_payload = XPTI_get_pull_event_payload(&(xptinfo->iinfo));
	xptinfo->restate_pull = XPT_RES_RECIEVED;
	return XPT_TRUE;
}

/*
int XPT_poll_pull_event_with_decode(XPT_info_t *sh, char* signal, char* payload){
	int cc = XPT_poll_pull_event(sh);
	if(cc == XPT_TRUE){
		if(signal != NULL){
			strncpy(signal, sh->pulled_event, MAX_XPT_EVENT);
		}
		if(payload != NULL){
			strncpy(payload, sh->pulled_payload, MAX_XPT_PAYLOAD);
		}
	}
	return cc;
}
*/

char* XPT_poll_get_payload(XPT_info_t *sh){
	if(sh->restate_pull == XPT_RES_NULL){
		return NULL;
	}
	return sh->pulled_payload;
}

char* XPT_poll_get_event(XPT_info_t *sh){
	if(sh->restate_pull == XPT_RES_NULL){
		return NULL;
	}
	return sh->pulled_payload;
}

int XPT_poll_push_event(XPT_info_t *xptinfo){

	//ERRCHK(xptinfo->ckpt_rdy == XPT_FALSE, "XPT is not initialized.\n");

	if(xptinfo->restate_push != XPT_RES_NULL)
		return XPT_FALSE;
	if(XPTI_poll_push_event(&(xptinfo->iinfo), &(xptinfo->pushed_event)) == XPTI_FALSE)
		return XPT_FALSE;

	xptinfo->restate_push = XPT_RES_RECIEVED;
	siglongjmp(xptinfo->sjmp, XPT_JMPBACK);

	return XPT_TRUE; // not execute
}

char* XPT_push_get_payload(XPT_info_t *sh){
	if(sh->restate_push == XPT_RES_NULL){
		return NULL;
	}
	return sh->pushed_payload;
}

char* XPT_push_get_event(XPT_info_t *sh){
	if(sh->restate_push == XPT_RES_NULL){
		return NULL;
	}
	return sh->pushed_payload;
}


int XPT_pull_done(XPT_info_t *xptinfo){
	xptinfo->restate_pull = XPT_RES_NULL;
	return XPT_TRUE;
}
int XPT_push_done(XPT_info_t *xptinfo){
	xptinfo->restate_push = XPT_RES_NULL;
	return XPT_TRUE;
}


int XPT_fini(XPT_info_t *xptinfo){
	return XPTI_fini(&(xptinfo->iinfo));
}

void XPT_exec_coprocess(int process_role){
	char* rootip = getenv("XPT_ROOT_IP");
	if(rootip == NULL){
		rootip = DEF_XPT_ROOTIP;
	}
	setenv("XPTI_ROOT_IP", rootip, 1);
	XPTI_exec_coserver(process_role);
}

void XPT_killnwait_coprocess(int process_role){
	XPTI_killnwait_coserver(process_role);
}

void XPT_coserver_exec_complete(){
	XPTI_coserver_exec_complete();
}
/*
void XPT_checknread(FILE *fp, int *checker){
	int tmp;
	fread(&tmp, sizeof(int), 1, fp);
	if(tmp != *checker){
		HPL_fprintf(stderr, "HPL.dat is different for CP File's one.^n");
		MPI_Abort();
		exit(EXIT_FAILURE);
	}
	*checker = tmp;
}

void XPT_tentative_serializer(FILE *fp, HPL_T_pmat * A){
	int differ;
	fwrite(&(A->info), sizeof(int), 1, fp);
	fwrite(&(A->ld), sizeof(int), 1, fp);
	fwrite(&(A->mp), sizeof(int), 1, fp);
	fwrite(&(A->n), sizeof(int), 1, fp);
	fwrite(&(A->nb), sizeof(int), 1, fp);
	fwrite(&(A->nq), sizeof(int), 1, fp);
	fwrite(A->A, sizeof(double), (A->ld+1)*A->nq, fp);
	differ = A->X - A->A;
	fwrite(&differ, sizeof(int), 1, fp);
}

void XPT_tentative_deserializer(FILE *fp, HPL_T_pmat * A){
	int differ;
	XPT_checknread(fp, &(A->info));
	XPT_checknread(fp, &(A->ld));
	XPT_checknread(fp, &(A->mp));
	XPT_checknread(fp, &(A->n));
	XPT_checknread(fp, &(A->nb));
	XPT_checknread(fp, &(A->nq));
	fread(A->A, sizeof(double), (A->ld+1)*A->nq, fp);
	fread(&differ, sizeof(int), 1, fp);
	A->X = A->A + differ;
}
*/

//#endif
