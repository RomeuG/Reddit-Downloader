#include "rvg_reddit.h"

int main(int argc, char** argv, char** envp)
{
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

    struct string_list* thread_list = __rvg_get_subreddit_threads(&reddit, "emacs");

    return 0;
}
