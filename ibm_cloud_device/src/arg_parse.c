#include "arg_parse.h"
#include <argp.h>

static char args_doc[] = "";
static char doc[] = "IBM cloud device daemon program";

static struct argp_option options[] = {
    {"organization", 'o', "ORGANIZATION", 0, "Organization ID"},
    {"type", 't', "TYPE", 0, "Type ID"},
    {"device", 'd', "DEVICE", 0, "Device ID"},
    {"token", 'a', "TOKEN", 0, "Authentication token"},
    {0}};

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

void initiate_arguments(struct arguments *arguments)
{
    arguments->organization = "";
    arguments->type = "";
    arguments->device = "";
    arguments->token = "";
}