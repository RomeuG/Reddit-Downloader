#include "rvg_reddit.h"

#include "cJSON/cJSON.h"

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <argp.h>
#include <getopt.h>

struct arg_opts {
    char* argr;
    char* argo;
    int optr;
    int opto;
};

struct arg_opts pargs = { 0 };

const char* argp_program_version = "redditdownloader.0.0.1";
const char* argp_program_bug_address = "<romeu.bizz@gmail.com>";
static char doc[] = "Software to download reddit threads/subreddits/etc.";
static char args_doc[] = "[FILENAME]...";
static struct argp_option options[] = {
    { "outdir", 'o', "dir", 0, "Output directory." },
    { "subreddit", 'r', "sub", 0, "Select subreddit." },
    { 0 }
};

error_t argp_parseopts(int key, char* arg, struct argp_state* state)
{
    switch (key) {
        case 'o':
            pargs.opto = 1;
            pargs.argo = strdup(arg);
            break;
        case 'r':
            pargs.optr = 1;
            pargs.argr = strdup(arg);
            break;
        case ARGP_KEY_ARG:
            return 0;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

void write_to_file(char* filepath, char* data)
{
    FILE* fp = fopen(filepath, "ab");
    if (fp != NULL) {
        fputs(data, fp);
        fclose(fp);
    }
}

char* generate_full_path(char* dirname, char* thread_name)
{
    int duplicate = 0;
    int tries = 1;

    char* full_path = malloc(1024);

    memset(full_path, 0, 1024);
    sprintf(full_path, "%s%s.json", dirname, thread_name);

    if (access(full_path, F_OK) != -1) {
        duplicate = 1;
    }

    while (duplicate == 1) {
        memset(full_path, 0, 1024);
        sprintf(full_path, "%s%s_%d.json", dirname, thread_name, tries);

        if (access(full_path, F_OK) != -1) {
            duplicate = 1;
            tries++;
        } else {
            duplicate = 0;
        }
    }

    return full_path;
}

int main(int argc, char** argv, char** envp)
{
    static struct argp argp = { options, argp_parseopts, args_doc, doc, 0, 0, 0 };
    argp_parse(&argp, argc, argv, 0, 0, &pargs);

    if (!pargs.opto || !pargs.optr) {
        fprintf(stdout, "Required options missing!\n");
        exit(1);
    }

    struct stat path_stat;
    stat(pargs.argo, &path_stat);

    if (!S_ISDIR(path_stat.st_mode)) {
        fprintf(stdout, "Invalid path! Not a directory!\n");
        exit(1);
    }

    char dirname[512] = { 0 };
    int dirlen = strlen(pargs.argo);

    if (pargs.argo[dirlen - 1] != '/') {
        sprintf(dirname, "%s/", pargs.argo);
    } else {
        sprintf(dirname, "%s", pargs.argo);
    }

    fprintf(stdout, "Output directory: %s\n", dirname);

    char* reddit_id = getenv("REDDIT_ID");
    char* reddit_secret = getenv("REDDIT_SECRET");
    char* reddit_user = getenv("REDDIT_USER");
    char* reddit_pass = getenv("REDDIT_PASS");
    char* reddit_useragent = getenv("REDDIT_USERAGENT");

    struct reddit_ctx reddit;

    rvg_reddit_init(&reddit);

    rvg_reddit_set_id(&reddit, reddit_id);
    rvg_reddit_set_secret(&reddit, reddit_secret);
    rvg_reddit_set_username(&reddit, reddit_user);
    rvg_reddit_set_password(&reddit, reddit_pass);
    rvg_reddit_set_useragent(&reddit, reddit_useragent);

    __rvg_get_access_token(&reddit);

    struct string_list* thread_names = malloc(sizeof(struct string_list));
    struct string_list* thread_list = __rvg_get_subreddit_threads(&reddit, pargs.argr, thread_names);

    for (int i = 0; i < thread_list->size; i++) {
        cJSON* json = cJSON_Parse(thread_list->list[i]);
        char* json_fmt = cJSON_Print(json);

        char* full_path = generate_full_path(dirname, thread_names->list[i]);

        write_to_file(full_path, json_fmt);
    }

    return 0;
}
