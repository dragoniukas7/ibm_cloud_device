#include <syslog.h>
#include <iotp_device.h>
#include "watson.h"

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