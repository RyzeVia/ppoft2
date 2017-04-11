/**********************************************************************************/
/* This file is part of FTB (Fault Tolerance Backplance) - the core of CIFTS
 * (Co-ordinated Infrastructure for Fault Tolerant Systems)
 *
 * See http://www.mcs.anl.gov/research/cifts for more information.
 *
 */
/* This software is licensed under BSD. See the file FTB/misc/license.BSD for
 * complete details on your rights to copy, modify, and use this software.
 */
/*********************************************************************************/

/*
 * Description:
 * This example component periodically publishes events of event name
 * SIMPLE_EVENT. It does not subscribe to any events during its lifetime
 *
 * Usage: ./ftb_simple_publisher
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "libftb.h"

int main(int argc, char *argv[]) {
	FTB_client_t cinfo;
	FTB_client_handle_t handle;
	FTB_event_handle_t ehandle;
	int ret = 0, i = 0;
	FTB_event_info_t event_info[2] = { { "RST", "INFO" }, { "NRST", "INFO" } };
	FTB_event_info_t event_info2[2] ={ { "CPT", "INFO" },{ "NCPT", "INFO" } };
	char *c;
	int f;

	f = atoi(argv[1]);

	printf("Begin\n");
	/* Specify the client information and call FTB_Connect */
	memset(&cinfo, 0, sizeof(cinfo));
	strcpy(cinfo.event_space, "FTB.TEST.ftbtest");
	strcpy(cinfo.client_subscription_style, "FTB_SUBSCRIPTION_BOTH");

	ret = FTB_Connect(&cinfo, &handle);
	if (ret != FTB_SUCCESS) {
		printf("FTB_Connect did not return a success\n");
		exit(-1);
	}

	/*
	 * Declare the events that this client wants to publispublishh. The events and
	 * their severity are defined in the event_info structure
	 */
	ret = FTB_Declare_publishable_events(handle, 0, event_info, 2);
	ret = FTB_Declare_publishable_events(handle, 0, event_info2, 2);
	if (ret != FTB_SUCCESS) {
		printf("FTB_Declare_publishable_events failed ret=%d!\n", ret);
		exit(-1);
	}
	if(f == 0){
		ret = FTB_Publish(handle, "RST", NULL, &ehandle);
	}
	else if(f == 1){
		ret = FTB_Publish(handle, "CPT", NULL, &ehandle);
	}
	else if(f == 2){
		ret = FTB_Publish(handle, "NRST", NULL, &ehandle);
	}
	else if(f == 3){
		ret = FTB_Publish(handle, "NCPT", NULL, &ehandle);
	}
	if (ret != FTB_SUCCESS) {
		printf("FTB_Publish did not return a success\n");
		exit(-1);
	}
	printf("FTB_Disconnect\n");
	FTB_Disconnect(handle);

	printf("End\n");
	return 0;
}
