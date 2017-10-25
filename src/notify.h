//
//  notify.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 27/10/2014.
//
//

#ifndef __notify__
#define __notify__

#include <stdio.h>

int  mea_notify(char *hostname, int port, char *notif_str, char notif_type);

void mea_notify_set_port(int p);
int  mea_notify_printf(int notify_type, char const* fmt, ...);

void mea_notify_enable(void);
void mea_notify_disable(void);

#endif /* defined(__notify__) */
