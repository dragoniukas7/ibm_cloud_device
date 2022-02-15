#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <iotp_device.h>
#include <syslog.h>
#include "invoke.h"

volatile int interrupt = 0;

void sigHandler(int signo) {
    signal(SIGINT, NULL);
    syslog(LOG_INFO, "Received signal: %d\n", signo);
    interrupt = 1;
}

void MQTTTraceCallback (int level, char * message){
    if ( level > 0 )
        syslog(LOG_DEBUG, "%s\n", message? message:"NULL");
}

/* Main program */
int main(int argc, char *argv[]){

    int rc = 0;
    int cycle = 0;
    struct memory mem;
    
    openlog("ibm_cloud_device", LOG_PID, LOG_USER);

    IoTPConfig *config = NULL;
    IoTPDevice *device = NULL;
    
    if(argc != 5){
        syslog(LOG_ERR, "Wrong argument countd\n");
    }

    /* Set signal handlers */
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    /* Set IoTP Client log handler */
    rc = IoTPConfig_setLogHandler(IoTPLog_FileDescriptor, stdout);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "Failed to set IoTP Client log handler: rc=%d\n", rc);
        exit(1);
    }
    
    /* Create IoTPConfig object using configuration options defined in the configuration file. */
    rc = IoTPConfig_create(&config, NULL);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "ERROR: Failed to initialize configuration: rc=%d\n", rc);
        exit(1);
    }
    
    IoTPConfig_setProperty(config, "identity.orgId", argv[1]);
    IoTPConfig_setProperty(config, "identity.typeId", argv[2]);
    IoTPConfig_setProperty(config, "identity.deviceId", argv[3]);
    IoTPConfig_setProperty(config, "auth.token", argv[4]);

    /* Create IoTPDevice object */
    rc = IoTPDevice_create(&device, config);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "ERROR: Failed to configure IoTP device: rc=%d\n", rc);
        exit(1);
    }

    /* Set MQTT Trace handler */
    rc = IoTPDevice_setMQTTLogHandler(device, &MQTTTraceCallback);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "WARN: Failed to set MQTT Trace handler: rc=%d\n", rc);
    }

    /* Invoke connection API IoTPDevice_connect() to connect to WIoTP. */
    rc = IoTPDevice_connect(device);
    if ( rc != 0 ) {
        syslog(LOG_ERR, "ERROR: Failed to connect to Watson IoT Platform: rc=%d\n", rc);
        syslog(LOG_ERR, "ERROR: Returned error reason: %s\n", IOTPRC_toString(rc));
        exit(1);
    }

    while(!interrupt)
    {
    	get_usage(&mem);
    	char data[1024]; 
    	sprintf(data, "{\"totalMemory\": \"%ld\", \"freeMemory\":\"%ld\", \"sharedMemory\":\"%ld\", \"bufferedMemory\":\"%ld\"}", mem.totalMemory, mem.freeMemory, mem.sharedMemory, mem.bufferedMemory);
        rc = IoTPDevice_sendEvent(device,"status", &data, "json", QoS0, NULL);
        
        if ( rc != 0 ) {
            syslog(LOG_ERR, "ERROR: Failed to send event: rc=%d\n", rc);
        }
        else {
            syslog(LOG_INFO, "Successfully published data to IBM cloud");
        }
        sleep(10);
    }

    syslog(LOG_INFO, "Publish event cycle is complete.\n");

    rc = IoTPDevice_disconnect(device);
    if ( rc != IOTPRC_SUCCESS ) {
        syslog(LOG_ERR, "ERROR: Failed to disconnect from  Watson IoT Platform: rc=%d\n", rc);
        exit(1);
    }

    IoTPDevice_destroy(device);
    IoTPConfig_clear(config);
    closelog();

    return 0;
}
