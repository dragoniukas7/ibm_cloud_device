#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <syslog.h>
#include "invoke.h"

int rc = 0;

enum {
	TOTAL_MEMORY,
	FREE_MEMORY,
	SHARED_MEMORY,
	BUFFERED_MEMORY,
	__MEMORY_MAX,
};

enum {
	MEMORY_DATA,
	__INFO_MAX,
};

static const struct blobmsg_policy info_policy[__INFO_MAX] = {
	[MEMORY_DATA] = { .name = "memory", .type = BLOBMSG_TYPE_TABLE },
};

static const struct blobmsg_policy memory_policy[__MEMORY_MAX] = {
	[TOTAL_MEMORY] = { .name = "total", .type = BLOBMSG_TYPE_INT64 },
	[FREE_MEMORY] = { .name = "free", .type = BLOBMSG_TYPE_INT64 },
	[SHARED_MEMORY] = { .name = "shared", .type = BLOBMSG_TYPE_INT64 },
	[BUFFERED_MEMORY] = { .name = "buffered", .type = BLOBMSG_TYPE_INT64 },
};

static void mem_cb(struct ubus_request *req, int type, struct blob_attr *msg) {
	struct memory *mem = (struct memory *)req->priv;
	struct blob_attr *tb[__INFO_MAX];
	struct blob_attr *memory[__MEMORY_MAX];

	blobmsg_parse(info_policy, __INFO_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[MEMORY_DATA]) {
		syslog(LOG_ERR, "No memory data received from ubus\n");
		rc=-1;
		return;
	}

	blobmsg_parse(memory_policy, __MEMORY_MAX, memory,
			blobmsg_data(tb[MEMORY_DATA]), blobmsg_data_len(tb[MEMORY_DATA]));

	mem->freeMemory = blobmsg_get_u64(memory[FREE_MEMORY]);
	mem->totalMemory = blobmsg_get_u64(memory[TOTAL_MEMORY]);
	mem->sharedMemory = blobmsg_get_u64(memory[SHARED_MEMORY]);
	mem->bufferedMemory = blobmsg_get_u64(memory[BUFFERED_MEMORY]);
}

int get_usage(struct memory *mem)
{
	struct ubus_context *ctx;
	uint32_t id;

	ctx = ubus_connect(NULL);
	if (!ctx) {
		syslog(LOG_ERR, "Failed to connect to ubus\n");
		return -1;
	}

	if (ubus_lookup_id(ctx, "system", &id) ||
	        ubus_invoke(ctx, id, "info", NULL, mem_cb, mem, 3000)) {
		syslog(LOG_ERR, "Cannot request memory info from procd\n");
		rc=-1;
	}

	ubus_free(ctx);
	
	return rc;
}
