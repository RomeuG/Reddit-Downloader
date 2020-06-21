#ifndef __RVG_REDDIT_H__
#define __RVG_REDDIT_H__

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <unistd.h>

#include <curl/curl.h>

#define REDDIT_ACCESS_TOKEN_URL_FORMAT "https://www.reddit.com/api/v1/access_token?grant_type=password&username=%s&password=%s"
#define REDDIT_SUBREDDIT_JSON_URL_FORMAT "https://oauth.reddit.com/r/%s/.json"
#define REDDIT_SUBREDDIT_AFTER_JSON_URL_FORMAT "https://oauth.reddit.com/r/%s/.json?after=%s"
#define REDDIT_PERMALINK_URL_FORMAT "https://oauth.reddit.com%s.json"

#define HEADER_USERAGENT_FORMAT "User-Agent: %s"
#define HEADER_AUTHORIZATION_FORMAT "Authorization: Basic %s"
#define HEADER_AUTHORIZATION_BEARER_FORMAT "Authorization: Bearer %s"

#define REDDIT_ACCESS_TOKEN_SZ 37

struct string_list {
    char** list;
    int size;
};

void rvg_string_list_free(struct string_list* list)
{
    if (list->size > 0) {
        for (int i = 0; i < list->size; i++) {
            free(list->list[i]);
        }
    }
}

struct reddit_accesstoken {
    int valid;
    int timeout;
    int remaining_requests;
    int reset_seconds;
    int used_requests;
    char* token;
};

struct reddit_ctx {
    struct reddit_accesstoken access_token;

    char* id;
    char* secret;
    char* user_agent;
    char* username;
    char* password;
};

static inline void access_token_check_sleep(struct reddit_ctx* ctx)
{
    fprintf(stdout, "Remaining requests: %d | Reset seconds: %d\n", ctx->access_token.remaining_requests, ctx->access_token.reset_seconds);

    if (ctx->access_token.remaining_requests == 0) {
        fprintf(stdout, "Sleeping for %d seconds...\n", ctx->access_token.reset_seconds);
        sleep(ctx->access_token.reset_seconds);
    }
}

static char* get_thread_name(char* str)
{
    int slash_count = 0;
    char* name = 0;

    while (str++) {
        if (*str == '/') {
            slash_count++;
        }

        if (slash_count == 4) {
            name = strdup(str + 1);
            name[strlen(name) - 1] = 0;
            break;
        }
    }

    return name;
}

static const uint8_t base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const uint8_t base64_index[256] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
                                           56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6,
                                           7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,
                                           0, 0, 0, 63, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                                           41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

static uint8_t* rvg_b64_enc(uint8_t* src, size_t len, uint8_t newline, size_t* out_len)
{
    uint8_t* out;
    uint8_t* pos;

    const uint8_t* end;
    const uint8_t* in;

    size_t olen;
    int line_len;

    olen = len * 4 / 3 + 4;
    olen += olen / 72;
    olen++;
    if (olen < len) {
        return 0;
    }

#ifdef __cplusplus
    out = (uint8_t*)malloc(olen);
#else
    out = malloc(olen);
#endif
    if (out == 0) {
        return 0;
    }

    end = src + len;
    in = src;
    pos = out;
    line_len = 0;
    while (end - in >= 3) {
        *pos++ = base64_table[in[0] >> 2];
        *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        *pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        *pos++ = base64_table[in[2] & 0x3f];
        in += 3;
        line_len += 4;
        if (line_len >= 72) {
            if (newline) {
                *pos++ = '\n';
            }

            line_len = 0;
        }
    }

    if (end - in) {
        *pos++ = base64_table[in[0] >> 2];
        if (end - in == 1) {
            *pos++ = base64_table[(in[0] & 0x03) << 4];
            *pos++ = '=';
        } else {
            *pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
            *pos++ = base64_table[(in[1] & 0x0f) << 2];
        }
        *pos++ = '=';
        line_len += 4;
    }

    if (line_len && newline) {
        *pos++ = '\n';
    }

    *pos = '\0';
    if (out_len) {
        *out_len = pos - out;
    }

    return out;
}

static uint8_t* rvg_b64_dec(void* data, size_t len)
{
    unsigned char* p = (unsigned char*)data;
    int pad = len > 0 && (len % 4 || p[len - 1] == '=');
    const size_t L = ((len + 3) / 4 - pad) * 4;

    int str_len = L / 4 * 3 + pad;

#ifdef __cplusplus
    uint8_t* str = (uint8_t*)malloc(str_len);
#else
    uint8_t* str = malloc(str_len);
#endif

    memset(str, '\0', str_len);

    for (size_t i = 0, j = 0; i < L; i += 4) {
        int n = base64_index[p[i]] << 18 | base64_index[p[i + 1]] << 12 | base64_index[p[i + 2]] << 6 | base64_index[p[i + 3]];
        str[j++] = n >> 16;
        str[j++] = n >> 8 & 0xFF;
        str[j++] = n & 0xFF;
    }
    if (pad) {
        int n = base64_index[p[L]] << 18 | base64_index[p[L + 1]] << 12;
        str[str_len - 1] = n >> 16;

        if (len > L + 2 && p[L + 2] != '=') {
            n |= base64_index[p[L + 2]] << 6;
#ifdef __cplusplus
            str = (uint8_t*)realloc(str, ++str_len);
#else
            str = realloc(str, ++str_len);
#endif
            str[str_len - 1] = n >> 8 & 0xFF;
        }
    }

    return str;
}

// curl
struct curl_response {
    char* memory;
    size_t size;
};

static size_t curl_cb_response(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t total = size * nmemb;
    struct curl_response* mem = (struct curl_response*)userp;

    char* ptr = (char*)realloc(mem->memory, mem->size + total + 1);
    if (ptr == 0) {
        fprintf(stdout, "An error has ocurred during HTTP communication...\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, total);
    mem->size += total;
    mem->memory[mem->size] = 0;

    return total;
}

static size_t curl_cb_headers(char* header, size_t size, size_t nitems, void* userp)
{
    size_t total = size * nitems;
    struct reddit_ctx* ctx = (struct reddit_ctx*)userp;

    sscanf(header, "x-ratelimit-remaining: %d", &ctx->access_token.remaining_requests);
    sscanf(header, "x-ratelimit-reset: %d", &ctx->access_token.reset_seconds);
    sscanf(header, "x-ratelimit-used: %d", &ctx->access_token.used_requests);

    return total;
}

static int curl_execute_request_subreddit_aux(struct reddit_ctx* ctx, char* url, struct curl_slist* headers, char* next, struct string_list* results)
{
    CURL* curl_handle;
    CURLcode res;

    long response_code = -1;

    struct curl_response response;
    response.memory = malloc(1);
    response.size = 0;

    access_token_check_sleep(ctx);

    fprintf(stdout, "Initializing request for url: %s\n", url);

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, curl_cb_headers);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void*)ctx);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_cb_response);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&response);

    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);

    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        fprintf(stdout, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    }

    int size = ++results->size;
    fprintf(stdout, "new size: %d\n", size);
    results->list = realloc(results->list, sizeof(char*) * size);
    results->list[size - 1] = strdup(response.memory);

    char* after_ptr = 0;
    char* after_ptr_tmp = response.memory;

    do {
        after_ptr_tmp = strstr(after_ptr_tmp + 8, "\"after\":");
        if (after_ptr_tmp != 0) {
            after_ptr = after_ptr_tmp;
        }
    } while (after_ptr_tmp != 0);

    if (after_ptr != 0) {
        char after[32] = { 0 };
        sscanf(after_ptr, "\"after\": \"%s\", \"before\": null } } ", after);
        after[9] = '\0';

        if (after[0] != 0) {
            memcpy(next, after, 32);
        } else {
            memset(next, 0, 32);
        }
    } else {
        memset(next, 0, 32);
    }

    curl_easy_cleanup(curl_handle);

    return response_code;
}

static int curl_execute_request(struct reddit_ctx* ctx, char* url, struct curl_slist* headers, int type, struct string_list* results)
{
    CURL* curl_handle;
    CURLcode res;

    long response_code = -1;

    struct curl_response response;
    response.memory = malloc(1);
    response.size = 0;

    fprintf(stdout, "Initializing request for url: %s\n", url);

    access_token_check_sleep(ctx);

    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    if (type == 0) {
        curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, "");
    }

    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, curl_cb_headers);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void*)ctx);

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_cb_response);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&response);

    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);

    res = curl_easy_perform(curl_handle);

    if (res != CURLE_OK) {
        fprintf(stdout, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    }

    if (type == 0) {
        ctx->access_token.token = malloc(REDDIT_ACCESS_TOKEN_SZ);
        sscanf(response.memory, "{\"access_token\": \"%s\", \"token_type\": \"bearer\", \"expires_in\": 3600, \"scope\": \"*\"}", ctx->access_token.token);
        ctx->access_token.token[36] = '\0';
    } else if (type == 1) {
        results->list = malloc(sizeof(char*) * 1);
        results->size = 1;

        results->list[0] = strdup(response.memory);

        char* after_ptr = strstr(response.memory, "\"after\":");

        char after_str[32] = { 0 };
        sscanf(after_ptr, "\"after\": \"%s\", \"before\": null } } ", after_str);
        after_str[9] = '\0';

        while (after_str[0] != 0) {
            char new_url[1024] = { 0 };
            sprintf(new_url, REDDIT_SUBREDDIT_AFTER_JSON_URL_FORMAT, "emacs", after_str);

            free(response.memory);

            response.memory = (char*)malloc(1);
            response.size = 0;

            curl_execute_request_subreddit_aux(ctx, new_url, headers, after_str, results);
        }
    } else if (type == 2) {
        int size = ++results->size;
        results->list = realloc(results->list, sizeof(char*) * size);

        results->list[size - 1] = strdup(response.memory);
    }

    curl_easy_cleanup(curl_handle);
    curl_slist_free_all(headers);

    return response_code;
}

void __rvg_get_access_token(struct reddit_ctx* ctx)
{
    char url[1024];

    char header_useragent[1024] = { 0 };
    char header_authorization[1024] = { 0 };

    char authorization_plain[1024] = { 0 };
    char* authorization_b64 = 0;

    sprintf(url, REDDIT_ACCESS_TOKEN_URL_FORMAT, ctx->username, ctx->password);
    sprintf(header_useragent, HEADER_USERAGENT_FORMAT, ctx->user_agent);

    sprintf(authorization_plain, "%s:%s", ctx->id, ctx->secret);

    authorization_b64 = (char*)rvg_b64_enc((uint8_t*)authorization_plain, strlen(authorization_plain), 0, 0);
    sprintf(header_authorization, HEADER_AUTHORIZATION_FORMAT, authorization_b64);

    struct curl_slist* headers = 0;

    headers = curl_slist_append(headers, header_useragent);
    headers = curl_slist_append(headers, header_authorization);
    headers = curl_slist_append(headers, "Content-Type:");

    curl_execute_request(ctx, url, headers, 0, 0);

    free(authorization_b64);
}

struct string_list* __rvg_get_subreddit(struct reddit_ctx* ctx, char* subreddit)
{
    struct string_list* result_list;

    char url[1024];
    char header_useragent[1024] = { 0 };
    char header_authorization[1024] = { 0 };

    sprintf(url, REDDIT_SUBREDDIT_JSON_URL_FORMAT, subreddit);
    sprintf(header_useragent, HEADER_USERAGENT_FORMAT, ctx->user_agent);
    sprintf(header_authorization, HEADER_AUTHORIZATION_BEARER_FORMAT, ctx->access_token.token);

    struct curl_slist* headers = 0;

    headers = curl_slist_append(headers, header_useragent);
    headers = curl_slist_append(headers, header_authorization);

    result_list = (struct string_list*)malloc(sizeof(struct string_list));

    curl_execute_request(ctx, url, headers, 1, result_list);

    return result_list;
}

struct string_list* __rvg_get_subreddit_threads(struct reddit_ctx* ctx, char* subreddit)
{
    struct string_list thread_list;
    thread_list.size = 0;

    struct string_list* subreddit_pages = __rvg_get_subreddit(ctx, subreddit);

    fprintf(stdout, "There are %d pages in %s subreddit\n", subreddit_pages->size, subreddit);

    for (int i = 0; i < subreddit_pages->size; i++) {
        char* page_json = subreddit_pages->list[i];

        char* permalink_ptr = page_json;
        while ((permalink_ptr = strstr(permalink_ptr + 16, "\"permalink\":")) != 0) {
            char permalink[128] = { 0 };
            sscanf(permalink_ptr, "\"permalink\": \"%s%*[^\"]", permalink);

            permalink[strlen(permalink) - 2] = 0;

            thread_list.list = realloc(thread_list.list, sizeof(char*) * (++thread_list.size));

            thread_list.list[thread_list.size - 1] = strdup(permalink);
        }
    }

    rvg_string_list_free(subreddit_pages);

    struct string_list* thread_list_json = (struct string_list*)malloc(sizeof(struct string_list));
    thread_list_json->size = 0;

    for (int i = 0; i < thread_list.size; i++) {
        char url[256];
        sprintf(url, REDDIT_PERMALINK_URL_FORMAT, thread_list.list[i]);

        char* thread_name = get_thread_name(thread_list.list[i]);

        char header_useragent[1024] = { 0 };
        char header_authorization[1024] = { 0 };

        struct curl_response result;
        result.memory = malloc(1);
        result.size = 0;

        sprintf(header_useragent, HEADER_USERAGENT_FORMAT, ctx->user_agent);
        sprintf(header_authorization, HEADER_AUTHORIZATION_BEARER_FORMAT, ctx->access_token.token);

        struct curl_slist* headers = 0;

        headers = curl_slist_append(headers, header_useragent);
        headers = curl_slist_append(headers, header_authorization);

        curl_execute_request(ctx, url, headers, 2, thread_list_json);
    }

    rvg_string_list_free(&thread_list);

    fprintf(stdout, "Thread quantity: %d\n", thread_list_json->size);

    return thread_list_json;
}

void rvg_reddit_init(struct reddit_ctx* ctx)
{
    memset(ctx, 0, sizeof(struct reddit_ctx));

    ctx->access_token.remaining_requests = -1;
    ctx->access_token.reset_seconds = -1;
    ctx->access_token.used_requests = -1;
}

void rvg_reddit_set_id(struct reddit_ctx* ctx, char* id)
{
    ctx->id = strdup(id);
}

void rvg_reddit_set_secret(struct reddit_ctx* ctx, char* secret)
{
    ctx->secret = strdup(secret);
}

void rvg_reddit_set_username(struct reddit_ctx* ctx, char* username)
{
    ctx->username = strdup(username);
}

void rvg_reddit_set_password(struct reddit_ctx* ctx, char* password)
{
    ctx->password = strdup(password);
}

void rvg_reddit_set_useragent(struct reddit_ctx* ctx, char* user_agent)
{
    ctx->user_agent = strdup(user_agent);
}

void rvg_reddit_free(struct reddit_ctx* ctx)
{
    free(ctx->id);
    free(ctx->secret);
    free(ctx->user_agent);
}

#endif
