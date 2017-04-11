/*
 * xftbtest.c
 *
 *  Created on: 2012/07/25
 *      Author: RyzeVia
 */

#include "../libxpti.h"

static char* ename[] =
		{ "XPTI_TEST1", "XPTI_TEST2", "XPTI_TEST3", "XPTI_TEST4" };
void serv(XPTI_info_t *info);
void cli(XPTI_info_t *info);

int main(int argc, char** argv) {
	XPTI_info_t info;
	char argv0[256];
	int j;

	fprintf(stdout, "%s\n", argv[0]);
	if (strchr(argv[0], '\\') != NULL )
		strcpy(argv0, strrchr(argv[0], '\\') + 1);
	else if (strchr(argv[0], '/') != NULL )
		strcpy(argv0, strrchr(argv[0], '/') + 1);
	else
		strcpy(argv0, argv[0]);
	if (strchr(argv0, '.') != NULL )
		*strchr(argv0, '.') = '\0';
	for (j = 0; j < strlen(argv0); j++)
		argv0[j] = tolower(argv0[j]);

	if (strncmp(argv0, "tsrv", 4) == 0) {
		XPTI_exec_coserver(XPTI_ROLE_ONEOFTHE_JOB | XPTI_ROLE_ONEOFTHE_NODE);
		XPTI_init(&info, NULL );
		serv(&info);
	} else {
//		XPTI_exec_coserver(XPTI_ROLE_ONEOFTHE_NODE);
		XPTI_init(&info, NULL );
		cli(&info);
	}

}

void serv(XPTI_info_t *info) {
	int i, res, count = 0;
	char *recvname;
	XPTI_phdl_init(info, 4);
	for (i = 0; i < 4; i++) {
		XPTI_pull_subscribe(info, ename[i], NULL );
	}

	while (1) {
		res = XPTI_poll_pull_event(info, &recvname);
		if (res == XPTI_TRUE) {
			fprintf(stdout, "RECV: %s, PL: %s\n", recvname,
					XPTI_get_pull_event_payload(info));
			count++;
		}
		if (count == 5) {
			XPTI_killnwait_coserver(
					XPTI_ROLE_ONEOFTHE_JOB | XPTI_ROLE_ONEOFTHE_NODE);
			exit(EXIT_SUCCESS);
		}
	}
}

void cli(XPTI_info_t *info) {
	int i, res, count = 0;
	char pl[384];
	for (i = 0; i < 4; i++) {
		XPTI_declare_publishable_events(info, ename[i], "INFO");
	}

	while (1) {
		int r = rand() % 4;
		sprintf(pl, "PAYLOAD %d", r);
		fprintf(stdout, "publish %s with %s\n", ename[r], pl);
		res = XPTI_publish(info, ename[r], 1, pl);
		sleep(rand() % 3);
		count++;

		if (count == 5) {
			XPTI_killnwait_coserver(XPTI_ROLE_ONEOFTHE_NODE);
			exit(EXIT_SUCCESS);
		}
	}

}
