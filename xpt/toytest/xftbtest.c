/*
 * xftbtest.c
 *
 *  Created on: 2012/07/25
 *      Author: RyzeVia
 */

#include "libxpti.h"
#include <string.h>
#include <ctype.h>


int main(int argc, char** argv){
	XPTI_info_t info;
	char *recvname;
	int j;
	int res;

	XPTI_init(&info, NULL);
//	XPTI_phdl_init(&info, 1);
//	XPTI_pull_subscribe_flexible(&info, "serverity=info");

	XPTI_phdl_init(&info, 3);
	XPTI_pull_subscribe(&info, "XPT_SRV2CLI", NULL);
	XPTI_pull_subscribe(&info, "XPT_CLI2SRV", NULL);
	XPTI_pull_subscribe(&info, "XPT_TOALL", NULL);

	while(1){
		res = XPTI_poll_pull_event(&info, &recvname);
		if(res == XPTI_TRUE){
			fprintf(stdout, "RECV: %s, PL: %s\n",
					recvname, XPTI_get_pull_event_payload(&info));
		}
	}

}
