#include <stdio.h>
#include <iotp_device.h>
#include <argp.h>
#include <signal.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/file.h>
#include "invoke.h"

#define PIDFILE "/var/lock/ibm_cloud_device.lock"

volatile int interrupt = 0;

static struct argp_option options[] = {
    {"organization", 'o', "ORGANIZATION", 0, "Organization ID"},
    {"type", 't', "TYPE", 0, "Type ID"},
    {"device", 'd', "DEVICE", 0, "Device ID"},
    {"token", 'a', "TOKEN", 0, "Authentication token"},
    {0}};

static char doc[] = "IBM cloud device daemon program";

static char args_doc[] = "";

struct arguments
{
    char *organization, *type, *device, *token;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;

    switch (key)
    {
    case 'o':
        arguments->organization = arg;
        break;
    case 't':
        arguments->type = arg;
        break;
    case 'd':
        arguments->device = arg;
        break;
    case 'a':
        arguments->token = arg;
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num > 0)
            argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

void sigHandler(int signo)
{
    signal(SIGINT, NULL);
    syslog(LOG_INFO, "Received signal: %d\n", signo);
    interrupt = 1;
}

void MQTTTraceCallback(int level, char *message)
{
    if (level > 0)
        syslog(LOG_DEBUG, "%s\n", message ? message : "NULL");
}

void set_config(IoTPConfig *config, char *orgId, char *typeId, char *deviceId, char *token)
{
    IoTPConfig_setProperty(config, "identity.orgId", orgId);
    IoTPConfig_setProperty(config, "identity.typeId", typeId);
    IoTPConfig_setProperty(config, "identity.deviceId", deviceId);
    IoTPConfig_setProperty(config, "auth.token", token);
}

void send_data(IoTPDevice *device)
{
    struct memory mem;
    int rc = 0;

    while (!interrupt)
    {
        get_usage(&mem);
        char data[1024];
        sprintf(data, "{\"totalMemory\": \"%ld\", \"freeMemory\":\"%ld\", \"sharedMemory\":\"%ld\",\"bufferedMemory\":\"%ld\"}",
                mem.totalMemory, mem.freeMemory, mem.sharedMemory, mem.bufferedMemory);
        rc = IoTPDevice_sendEvent(device, "status", &data, "json", QoS0, NULL);

        if (rc != 0)
        {
            syslog(LOG_ERR, "Failed to send event");
        }
        else
        {
            syslog(LOG_INFO, "Successfully published data to IBM cloud");
        }
        sleep(10);
    }
}

void initiate_arguments(struct arguments *arguments)
{
    arguments->organization = "";
    arguments->type = "";
    arguments->device = "";
    arguments->token = "";
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
