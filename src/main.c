#include "rvg_reddit.h"

#include "cJSON/cJSON.h"

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

void write_to_file(char* filepath, char* data)
{
    FILE* fp = fopen(filepath, "ab");
    if (fp != NULL) {
        fputs(data, fp);
        fclose(fp);
    }
}

int main(int argc, char** argv, char** envp)
{
    if (argc != 2) {
        fprintf(stdout, "Invalid number of arguments!\n");
        exit(1);
    }

    struct stat path_stat;
    stat(argv[1], &path_stat);

    if (!S_ISDIR(path_stat.st_mode)) {
        fprintf(stdout, "Invalid path! Not a directory!\n");
        exit(1);
    }

    char* reddit_id = getenv("REDDIT_ID");
    char* reddit_secret = getenv("REDDIT_SECRET");
    char* reddit_user = getenv("REDDIT_USER");
    char* reddit_pass = getenv("REDDIT_PASS");

    struct reddit_ctx reddit;

    rvg_reddit_init(&reddit);

    rvg_reddit_set_id(&reddit, reddit_id);
    rvg_reddit_set_secret(&reddit, reddit_secret);
    rvg_reddit_set_username(&reddit, reddit_user);
    rvg_reddit_set_password(&reddit, reddit_pass);
    rvg_reddit_set_useragent(&reddit, "shell:Typescript-App:v0.0.1 (by /u/plasticooo)");

    __rvg_get_access_token(&reddit);

    struct string_list* thread_names = malloc(sizeof(struct string_list));
    struct string_list* thread_list = __rvg_get_subreddit_threads(&reddit, "emacs", thread_names);

    for (int i = 0; i < thread_list->size; i++) {
        cJSON* json = cJSON_Parse(thread_list->list[i]);
        char* json_fmt = cJSON_Print(json);

        write_to_file("", json_fmt);

        fprintf(stdout, "%s\n", json_fmt);
    }

    return 0;
}
