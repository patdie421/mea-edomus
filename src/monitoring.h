//
//  monitoring.h
//  mea-edomus
//
//  Created by Patrice Dietsch on 25/09/2014.
//
//

#ifndef mea_edomus_Header_h
#define mea_edomus_Header_h

pid_t start_nodejs(char *nodejs_path, char *eventServer_path, int port_socketio, int port_socketdata);
void stop_nodejs();
void startMonitoringServer(char *nodejs_path, char *eventServer_path, int port_socketio, int port_socketdata, char *log_path);

#endif
