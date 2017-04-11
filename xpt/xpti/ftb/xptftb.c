
#include "libxpti.h"

#define FTB_DEFAULT_DATABASE_SERVER		"ftb_database_server"
#define FTB_DEFAULT_DATABASE_SERVER_IP	"127.0.0.1"
#define FTB_DEFAULT_AGENT				"ftb_agent"
#define XPTI_EXEC_GAP	1
#define XPTI_MAX_PNAME	(256+23)

static char* def_ename = "FTB.TEST.ftbtest";

static pid_t dbserv = -1;
static pid_t agent = -1;

void SXPTI_getip(char* hostname, char* ip){
	struct sockaddr_in addr;
	struct hostent *host;

	host = gethostbyname(hostname);
	if (host != NULL ) {
		addr.sin_addr = *(struct in_addr *) (host->h_addr_list[0]);
		sprintf(ip, "%s", inet_ntoa(addr.sin_addr));
	}else{
		ip[0] = 0;
	}

}

void SXPTI_set_bstrap_ip(){
	char* ftb_database_server_ip;
	char ip[16];
	if ((ftb_database_server_ip = getenv("XPTI_ROOT_IP")) == NULL ) {
		ftb_database_server_ip = FTB_DEFAULT_DATABASE_SERVER_IP;
	}

	SXPTI_getip(ftb_database_server_ip, ip);
	DEBUGF(DEBUG_LEVEL_TRIVIAL, "[%d] FTB_BSTRAP_SERVER ip=%s\n", getpid(), ip);
	setenv("FTB_BSTRAP_SERVER", ip, 1);
}

void SXPTI_FTB_printerror(int en, char* caption){
	char* str = "UNDEFINED";
	switch(en){
	case FTB_SUCCESS:
		str="FTB_SUCCESS";
		break;
	case FTB_ERR_GENERAL:
		str="FTB_ERR_GENERAL";
		break;
	case FTB_ERR_EVENTSPACE_FORMAT:
		str="FTB_ERR_EVENTSPACE_FORMAT";
		break;
	case FTB_ERR_SUBSCRIPTION_STYLE:
		str="FTB_ERR_SUBSCRIPTION_STYLE";
		break;
	case FTB_ERR_INVALID_VALUE:
		str="FTB_ERR_INVALID_VALUE";
		break;
	case FTB_ERR_DUP_CALL:
		str="FTB_ERR_DUP_CALL";
		break;
	case FTB_ERR_NULL_POINTER:
		str="FTB_ERR_NULL_POINTER";
		break;
	case FTB_ERR_NOT_SUPPORTED:
		str="FTB_ERR_NOT_SUPPORTED";
		break;
	case FTB_ERR_INVALID_FIELD:
		str="FTB_ERR_INVALID_FIELD";
		break;
	case FTB_ERR_INVALID_HANDLE:
		str="FTB_ERR_INVALID_HANDLE";
		break;
	case FTB_ERR_DUP_EVENT:
		str="FTB_ERR_DUP_EVENT";
		break;
	case FTB_ERR_INVALID_SCHEMA_FILE:
		str="FTB_ERR_INVALID_SCHEMA_FILE";
		break;
	case FTB_ERR_INVALID_EVENT_NAME:
		str="FTB_ERR_INVALID_EVENT_NAME";
		break;
	case FTB_ERR_INVALID_EVENT_TYPE:
		str="FTB_ERR_INVALID_EVENT_TYPE";
		break;
	case FTB_ERR_SUBSCRIPTION_STR:
		str="FTB_ERR_SUBSCRIPTION_STR";
		break;
	case FTB_ERR_FILTER_ATTR:
		str="FTB_ERR_FILTER_ATTR";
		break;
	case FTB_ERR_FILTER_VALUE:
		str="FTB_ERR_FILTER_VALUE";
		break;
	case FTB_GOT_NO_EVENT:
		str="FTB_GOT_NO_EVENT";
		break;
	case FTB_FAILURE:
		str="FTB_FAILURE";
		break;
	case FTB_ERR_INVALID_PARAMETER:
		str="FTB_ERR_INVALID_PARAMETER";
		break;
	case FTB_ERR_NETWORK_GENERAL:
		str="FTB_ERR_NETWORK_GENERAL";
		break;
	case FTB_ERR_NETWORK_NO_ROUTE:
		str="FTB_ERR_NETWORK_NO_ROUTE";
		break;
	default:
		break;
	}

	DEBUGF(DEBUG_LEVEL_INFO, "[%d] %s: %s(%d)\n", getpid(), caption, str, en);

}


int SXPTI_FTB_exist_programs(char* pname_og){
	char buf[XPTI_MAX_PNAME];
	char pname[16];
	int ret;

	memset(pname, 0, sizeof(char)*16);
	strncpy(pname, pname_og, 15);

#ifdef DEBUG_PS_ENABLE
	sprintf(buf, "pgrep %s >> %s.%d 2>&1", pname, pname, getpid());
#else
	sprintf(buf, "pgrep %s >> /dev/null 2>&1", pname);
#endif

	ret = system(buf);
	if(WIFEXITED(ret) == 1){
//		printf("[%d]: %d,%d\n", getpid(), ret, WEXITSTATUS(ret));
#ifdef DEBUG_PS_ENABLE
		sprintf(buf, "ps >> %s.%d 2>&1", pname, getpid());
		system(buf);
#endif
		return (WEXITSTATUS(ret) == 0);
	}else{
		return -1;
	}
}


int XPTI_init(XPTI_info_t *info, const char * event_name) {
	FTB_client_handle_t *hd = &(info->chandle);
	FTB_client_t *cinfo = &(info->cinfo);
	int ret;
	char hostname[128];
	char buf[328];
	struct stat st;

	gethostname(hostname, 128);
	memset(info, 0, sizeof(XPTI_info_t));
	memset(cinfo, 0, sizeof(FTB_client_t));
	if (event_name == NULL ) {
		info->event_space_name = def_ename;
	} else {
		info->event_space_name = event_name;
	}

	strcpy(cinfo->event_space, info->event_space_name);
	strcpy(cinfo->client_subscription_style, "FTB_SUBSCRIPTION_BOTH");

	while((!SXPTI_FTB_exist_programs("ftb_agent")) && (stat(hostname, &st) != 0)){
//		system("pgrep -x ftb_agent");
		sleep(1);
	}
	sprintf(buf, "touch %s", hostname);
	system(buf);

	SXPTI_set_bstrap_ip();

	ret = FTB_Connect(cinfo, hd);
	SXPTI_FTB_printerror(ret, "Connect");

	return ret;
}

int XPTI_phdl_init(XPTI_info_t *info, int n_phdl) {
	CALLOC(XPTI_ehdlset_t*, info->phdl, n_phdl, sizeof(XPTI_ehdlset_t));
	info->n_phdl = n_phdl;

	int i;
	for (i = 0; i < n_phdl; i++) {
		info->phdl[i].used = 0;
	}

	return XPTI_TRUE;
}

/* FIXME push handler also implemented as like pull handler */
int XPTI_shdl_init(XPTI_info_t *info, int n_shdl) {
	ERRORF("This function is not implemented.\n");
	return XPTI_TRUE;
}

static int SXPTI_subscribe(FTB_client_handle_t ch, FTB_subscribe_handle_t* sh, char* signal, char* extra_attribute) {
	char atrb[XPTI_MAXATTRIB];
	int ret;
	if (extra_attribute != NULL ) {
		snprintf(atrb, XPTI_MAXATTRIB, "event_name=%s,%s", signal,
				extra_attribute);
	} else {
		snprintf(atrb, XPTI_MAXATTRIB, "event_name=%s", signal);
	}
	ret = FTB_Subscribe(sh, ch, atrb, NULL, NULL );
	SXPTI_FTB_printerror(ret, "Subscribe");
	DEBUGF(DEBUG_LEVEL_TRIVIAL, "[%d] SUBSCRIBE %s ret:%d\n", getpid(), atrb, ret);

	return ret;
}

int XPTI_pull_subscribe(XPTI_info_t *info, char* signal, char* extra_attribute) {
	int i = 0;

	while (info->phdl[i].used != 0) {
		i++;
		if (i >= info->n_phdl) {
			ERRORF("pull handler storage is exhausted.\n");
			ERROR_EXIT();
		}
	}
	info->phdl[i].used = 1;

	return SXPTI_subscribe(info->chandle, &(info->phdl[i].hdl), signal,
			extra_attribute);
}

int XPTI_pull_subscribe_flexible(XPTI_info_t *info, char* attributes) {
	int i = 0;
	int ret;
	while (info->phdl[i].used != 0) {
		i++;
		if (i >= info->n_phdl) {
			ERRORF("pull handler storage is exhausted.\n");
			ERROR_EXIT();
		}
	}
	info->phdl[i].used = 1;
	ret = FTB_Subscribe(&(info->shdl[i].hdl), info->chandle, attributes, NULL,NULL );
	DEBUGF(DEBUG_LEVEL_TRIVIAL, "[%d] SUBSCRIBE %s ret:%d\n", getpid(), attributes, ret);
	SXPTI_FTB_printerror(ret, "Subscribe");
	return ret;
}

int XPTI_push_subscribe(XPTI_info_t *info, char* signal, char* extra_attribute) {
	int i = 0;
	return SXPTI_subscribe(info->chandle, &(info->shdl[i].hdl), signal,
			extra_attribute);
}

/* FIXME: not supported "Partial Un-subscribe" */
int XPTI_pull_unsubscribe(XPTI_info_t *info) {
	int i;
	for (i = 0; i < info->n_phdl; i++) {
		if (info->phdl[i].used != 0)
			FTB_Unsubscribe(&(info->phdl[i].hdl));
		info->phdl[i].used = 0;
	}
	return XPTI_TRUE;
}

int XPTI_push_unsubscribe(XPTI_info_t *info) {
	int i = 0;
	return FTB_Unsubscribe(&(info->shdl[i].hdl));
}

int XPTI_declare_publishable_events(XPTI_info_t *info, char* signal, char* serverity) {
	FTB_event_info_t ei[1];
	int ret;
	strncpy(ei[0].event_name, signal, FTB_MAX_EVENT_NAME);
	strncpy(ei[0].severity, serverity, FTB_MAX_SEVERITY);
	ret = FTB_Declare_publishable_events(info->chandle, NULL, ei, 1);
	SXPTI_FTB_printerror(ret, "Event_declare");



	return ret;
}

int XPTI_declare_publishable_events_with_schemafile(XPTI_info_t *info, char* filename) {
	return FTB_Declare_publishable_events(info->chandle, filename, NULL, 1);
}

int XPTI_publish(XPTI_info_t *info, char* signal, int tag, char* payload) {
	FTB_event_handle_t ehandle;
	FTB_event_properties_t eprop, *peprop = NULL;
	int ret;

	if (payload != NULL ) {
		eprop.event_type = 1;
		strncpy(eprop.event_payload, payload, FTB_MAX_PAYLOAD_DATA);
		peprop = &eprop;
	}

	if (FTB_SUCCESS == (ret = FTB_Publish(info->chandle, signal, peprop, &(ehandle)))) {
		SXPTI_FTB_printerror(ret, "Publish");
		DEBUGF(DEBUG_LEVEL_INFO, "[%d] PUBLISH %s %s\n", getpid(), signal, payload);
		return XPTI_TRUE;
	}
	SXPTI_FTB_printerror(ret, "Publish");
	return XPTI_FALSE;
}

//DEBUGF(200, "checked %d, %d, %d, %d, %d, %d\n", ret, FTB_SUCCESS, FTB_ERR_NULL_POINTER, FTB_ERR_INVALID_HANDLE, FTB_ERR_NOT_SUPPORTED, FTB_GOT_NO_EVENT);

static int SXPTI_poll_event(FTB_subscribe_handle_t sh, FTB_receive_event_t* re, char** recved_name) {
	int ret;
	ret = FTB_Poll_event(sh, re);
	SXPTI_FTB_printerror(ret, "Poll");

	if (ret == FTB_SUCCESS) {
		*recved_name = re->event_name;
		return XPTI_TRUE;
	}
	recved_name = NULL;
	return XPTI_FALSE;
}

static int last_polling = -1;
int XPTI_poll_pull_event(XPTI_info_t *info, char** recved_name) {
	int i, res = XPTI_FALSE, chk;
	last_polling++;
	for (i = 0; (i < info->n_phdl) && (res == XPTI_FALSE); i++) {
		chk = (i + last_polling) % info->n_phdl;
		DEBUGF(DEBUG_LEVEL_INFO, "[%d] POLLING %d\n", getpid(), chk);
		res = SXPTI_poll_event(info->phdl[chk].hdl, &(info->phdl[chk].evt),
				recved_name);
	}
	last_polling = chk;
	return res;
}

int XPTI_poll_push_event(XPTI_info_t *info, char** recved_name) {
	return SXPTI_poll_event(info->shdl[0].hdl, &(info->shdl[0].evt),
			recved_name);
}

char* XPTI_get_pull_event_payload(XPTI_info_t *info) {
	return info->phdl[last_polling].evt.event_payload;
}

char* XPTI_get_push_event_payload(XPTI_info_t *info) {
	return info->shdl[0].evt.event_payload;
}

int XPTI_fini(XPTI_info_t *info) {
	int ret;

	ret = FTB_Disconnect(info->chandle);
	SXPTI_FTB_printerror(ret, "Disconnect");
	return ret;
}

//extern char **environ;
void SXPTI_exec_ftbserver() {
	char* ftb_database_server = NULL;

	//sleep(XPTI_EXEC_GAP);
	dbserv = fork();
	if (dbserv == -1) {
		ERRORF("fork error\n");
		exit(EXIT_FAILURE);
	} else if (dbserv == 0) {
/*
		int i = 0;
		while(environ[i] != NULL){
			printf("%s\n", environ[i]);
			i++;

		}
*/
		ftb_database_server = getenv("XPTI_DATABASE_SERVER");
		if (ftb_database_server == NULL ) {
			ftb_database_server = FTB_DEFAULT_DATABASE_SERVER;
		}

		SXPTI_set_bstrap_ip();

		DEBUGF(DEBUG_LEVEL_INFO, "[%d] Start DB(BSTRAP) Server: %s\n", getpid(), ftb_database_server);
		execlp(ftb_database_server, ftb_database_server, NULL );
		perror("ftb_database_server");
		exit(EXIT_FAILURE);
	}

	DEBUGF(DEBUG_LEVEL_INFO, "[%d] Forked for Server: %d\n", getpid(), dbserv);
	while(!SXPTI_FTB_exist_programs("ftb_database_server")){
		sleep(1);
	}
	DEBUGF(DEBUG_LEVEL_INFO, "[%d] Invoked for Server: %d\n", getpid(), dbserv);
#ifdef DEBUG_PS_ENABLE
	system("ps ux");
#endif
}

void SXPTI_exec_agent() {
	char* ftb_agent;

	if(SXPTI_FTB_exist_programs("ftb_agent")) goto agent_execed;
	agent = fork();
	if (agent == -1) {
		ERRORF("fork error\n");
		exit(EXIT_FAILURE);
	} else if (agent == 0) {

		if ((ftb_agent = getenv("XPTI_FTB_AGENT")) == NULL ) {
			ftb_agent = FTB_DEFAULT_AGENT;
		}

		SXPTI_set_bstrap_ip();

		DEBUGF(DEBUG_LEVEL_INFO, "[%d] Start Agent Server: %s\n", getpid(), ftb_agent);
		execlp(ftb_agent, ftb_agent, NULL );
		perror("ftb_agent");
		exit(EXIT_FAILURE);
	}
	DEBUGF(DEBUG_LEVEL_INFO, "[%d] Forked for Agent Server: %d\n", getpid(), agent);
agent_execed:
	while(!SXPTI_FTB_exist_programs("ftb_agent")){
		sleep(1);
	}
	DEBUGF(DEBUG_LEVEL_INFO, "[%d] Invoked for Agent Server or Passed it: %d\n", getpid(), agent);
#ifdef DEBUG_PS_ENABLE
	system("ps ux");
#endif
}

void SXPTI_exec_coserver_oneofthejob() {
	static int done = 0;
	if(!done){
		SXPTI_exec_ftbserver();
		done = 1;
	}else{
		ERRORF("Already servers were invoked.");
	}
}

void SXPTI_killnwait_coserver_oneofthejob(){
	if(dbserv != -1){
		kill(dbserv, SIGKILL);
		waitpid(dbserv, NULL, WNOHANG);
	}
}

void SXPTI_exec_coserver_oneofthenode() {
	static int done = 0;
	if(!done){
		SXPTI_exec_agent();
		done = 1;
	}else{
		ERRORF("Already servers were invoked.");
	}
}

void SXPTI_killnwait_coserver_oneofthenode() {
	if(agent != -1){
		kill(agent, SIGKILL);
		waitpid(agent, NULL, WNOHANG);
	}
}

void XPTI_exec_coserver(int role) {
	/*
	 if (role & XPTI_ROLE_ONEOFTHE_WORLD) {

	 }
	 */

//	DEBUGF(DEBUG_LEVEL_INFO, "[%d]: role=%d, 1oNode=%d, 1oJob=%d\n", getpid(), role, role & XPTI_ROLE_ONEOFTHE_NODE, role & XPTI_ROLE_ONEOFTHE_JOB);
//	fprintf(stdout, "[%d]: role=%d, 1oNode=%d, 1oJob=%d\n", getpid(), role, role & XPTI_ROLE_ONEOFTHE_NODE, role & XPTI_ROLE_ONEOFTHE_JOB);
	if (role & XPTI_ROLE_ONEOFTHE_JOB) {
		DEBUGF(DEBUG_LEVEL_INFO, "[%d][1OJ]: Start co-server invoking\n", getpid());
		SXPTI_exec_coserver_oneofthejob();
#ifdef DEBUG_PS_ENABLE
		system("ps ux");
#endif
		DEBUGF(DEBUG_LEVEL_INFO, "[%d][1OJ]: Finish co-server invoking\n", getpid());
	}
	if (role & XPTI_ROLE_ONEOFTHE_NODE) {
		DEBUGF(DEBUG_LEVEL_INFO, "[%d][1ON]: Start co-server invoking\n", getpid());
		SXPTI_exec_coserver_oneofthenode();
#ifdef DEBUG_PS_ENABLE
		system("ps ux");
#endif
		DEBUGF(DEBUG_LEVEL_INFO, "[%d][1ON]: Finish co-server invoking\n", getpid());
	}
	/*
	 if (role & XPTI_ROLE_OTHERS) {

	 }
	 */
//	DEBUGF(DEBUG_LEVEL_INFO, "[%d]: role=%d, 1oNode=%d, 1oJob=%d\n", getpid(), role, role & XPTI_ROLE_ONEOFTHE_NODE, role & XPTI_ROLE_ONEOFTHE_JOB);
}

void XPTI_killnwait_coserver(int role) {
	/*
	 if (role & XPTI_ROLE_ONEOFTHE_WORLD) {

	 }
	 */
	if (role & XPTI_ROLE_ONEOFTHE_NODE) {
		SXPTI_killnwait_coserver_oneofthenode();
	}
	if (role & XPTI_ROLE_ONEOFTHE_JOB) {
		SXPTI_killnwait_coserver_oneofthejob();
	}
	/*
	 if (role & XPTI_ROLE_OTHERS) {

	 }
	 */
}

void XPTI_coserver_exec_complete(){
	char hostname[128];
	//char buf[328];
	gethostname(hostname, 128);
	unlink(hostname);
}
