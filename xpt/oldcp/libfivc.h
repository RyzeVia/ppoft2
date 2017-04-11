/*
 * libfivc.h
 *
 *  Created on: 2012/07/02
 *      Author: RyzeVia
 */

#ifndef LIBFIVC_H_
#define LIBFIVC_H_

#include "libxpt.h"
#include "libxpt_cp.h"

/* const */
#define FI_VC_MAXATTRIB		XPTI_MAXATTRIB
#define FI_VC_MAXPAYLOAD	XPTI_MAXPAYLOAD
#define FI_VC_TRUE			XPTI_TRUE
#define FI_VC_FALSE			XPTI_FALSE
#define FI_VC_UNEXPECTED	XPTI_UNEXPECTED

/* struct*/
#define FI_VC_info_t XPTI_info_t

/* function */
#define FI_VC_init_i(fivcinfo_pt, eventname_pc) \
	XPTI_init(fivcinfo_pt, eventname_pc)
#define FI_VC_pull_subscribe_i(fivcinfo_pt, signal_pc, attribute_pc) \
	XPTI_pull_subscribe(fivcinfo_pt, signal_pc, attribute_pc)
#define FI_VC_push_subscribe_i(fivcinfo_pt, signal_pc, attribute_pc) \
	XPTI_push_subscribe(fivcinfo_pt, signal_pc, attribute_pc)
#define FI_VC_pull_unsubscribe_i(fivcinfo_pt) \
	XPTI_pull_unsubscribe(fivcinfo_pt)
#define FI_VC_push_unsubscribe_i(fivcinfo_pt) \
	XPTI_push_unsubscribe(fivcinfo_pt)
#define FI_VC_declare_publishable_events_i(fivcinfo_pt, signal_pc, serverity_pc) \
	XPTI_declare_publishable_events(fivcinfo_pt, signal_pc, serverity_pc);
#define FI_VC_publish_i(fivcinfo_pt, signal_pc, payload_pc) \
	XPTI_publish(fivcinfo_pt, signal_pc, 1, payload_pc);
#define FI_VC_poll_pull_event_i(fivcinfo_pt, recved_name_ppc) \
	XPTI_poll_pull_event(fivcinfo_pt, recved_name_ppc);
#define FI_VC_get_pull_event_payload_pc(fivcinfo_pt) \
	XPTI_get_pull_event_payload(fivcinfo_pt)
#define FI_VC_fini_i(fivcinfo_pt) \
	XPTI_fini(fivcinfo_pt)


#endif /* LIBFIVC_H_ */
