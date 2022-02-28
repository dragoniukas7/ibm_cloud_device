#include <stdio.h>
#include <iotp_device.h>
#include <signal.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/file.h>
#include "invoke.h"
#include "arg_parse.h"
#include "watson.h"

#define PIDFILE "/var/lock/ibm_cloud_device.lock"

volatile int interrupt = 0;

void sigHandler(int signo)
{
    signal(SIGINT, NULL);
    syslog(LOG_INFO, "Received signal: %d\n", signo);
    interrupt = 1;
}

void aquire_lock(int *fd, int *lock)
{
    if ((*fd = open(PIDFILE, (O_RDWR | O_CREAT))) == -1)
    {
        syslog(LOG_ERR, "Cannot open PID file in %s", PIDFILE);
        exit(1);
    }

    if (flock(*fd, LOCK_EX | LOCK_NB) == -1)
    {
        syslog(LOG_ERR, "File %s is locked, exiting", PIDFILE);
        exit(1);
    }
}

void relase_lock(int *fd, int *lock)
{
    syslog(LOG_INFO, "Relasing lock");
    if (flock(*fd, LOCK_UN) == -1)
    {
        exit(1);
    }
    close(*fd);
}

static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char *argv[])
{
    openlog("ibm_cloud_device", LOG_PID, LOG_USER);

    int fd, lock;

    aquire_lock(&fd, &lock);

    struct arguments arguments;
    int rc = 0;

    IoTPConfig *config = NULL;
    IoTPDevice *device = NULL;

    initiate_arguments(&arguments);

    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    /* Set signal handlers */
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    /* Set IoTP Client log handler */
    rc = IoTPConfig_setLogHandler(IoTPLog_FileDescriptor, stdout);
    if (rc != 0)
    {
        syslog(LOG_ERR, "Failed to set IoTP Client log handler");
        goto end;
    }

    /* Create IoTPConfig object using configuration options defined in the configuration file. */
    rc = IoTPConfig_create(&config, NULL);
    if (rc != 0)
    {
        syslog(LOG_ERR, "Failed to initialize configuration");
        goto end;
    }

    set_config(config, arguments.organization, arguments.type, arguments.device, arguments.token);

    /* Create IoTPDevice object */
    rc = IoTPDevice_create(&device, config);
    if (rc != 0)
    {
        syslog(LOG_ERR, "Failed to configure IoTP device");
        goto end;
    }

    /* Set MQTT Trace handler */
    rc = IoTPDevice_setMQTTLogHandler(device, &MQTTTraceCallback);
    if (rc != 0)
    {
        syslog(LOG_ALERT, "WARN: Failed to set MQTT Trace handler");
        goto end;
    }

    /* Invoke connection API IoTPDevice_connect() to connect to WIoTP. */
    rc = IoTPDevice_connect(device);
    if (rc != 0)
    {
        syslog(LOG_ERR, "Failed to connect to Watson IoT Platform");
        syslog(LOG_ERR, "Returned error reason: %s\n", IOTPRC_toString(rc));
        goto end;
    }

    send_data(device);

    syslog(LOG_INFO, "Publish event cycle is complete");

    rc = IoTPDevice_disconnect(device);
    if (rc != IOTPRC_SUCCESS)
    {
        syslog(LOG_ERR, "Failed to disconnect from  Watson IoT Platform");
        goto end;
    }

end:
    IoTPDevice_destroy(device);
    IoTPConfig_clear(config);
    closelog();
    relase_lock(&fd, &lock);

    return 0;
}
