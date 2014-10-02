//
//  monitoring.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 25/09/2014.
//
//

#ifndef mea_edomus_Header_h
#define mea_edomus_Header_h


//pthread_t *start_monitoringServer(char *nodejs_path, char *eventServer_path, int port_socketio, int port_socketdata, char *log_path);
pthread_t *start_monitoringServer(char **parms_list)
void stop_monitoringServer();

#endif
