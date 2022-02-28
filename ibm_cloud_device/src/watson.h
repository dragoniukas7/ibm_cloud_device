void MQTTTraceCallback(int level, char *message);

void set_config(IoTPConfig *config, char *orgId, char *typeId, char *deviceId, char *token);

void send_data(IoTPDevice *device);