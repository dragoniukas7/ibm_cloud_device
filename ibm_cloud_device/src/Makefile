BIN := ibm_cloud_device
OBJS := main.o invoke.o
CPPFLAGS := -Wall
LDFLAGS := -liotp-as-device -lpaho-mqtt3as -lblobmsg_json -lubox -lubus

.PHONY: clean all

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(BIN) $(OBJS)
