#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <fctnl.h>
#include <unistd.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#define MAX_BREEDS 120
#define MAX_BREED_NAME_LEN 50
#define MAX_URL_LEN 100

static const char *BREEDS_URL = "https://catfact.ninja/breeds";
// TODO: Dynamically allocate the arrays below
static char breeds[MAX_BREEDS][MAX_BREED_NAME_LEN];
static char countries[MAX_BREEDS][MAX_BREED_NAME_LEN];
static char origins[MAX_BREEDS][MAX_BREED_NAME_LEN];
static char coats[MAX_BREEDS][MAX_BREED_NAME_LEN];
static char patterns[MAX_BREEDS][MAX_BREED_NAME_LEN];

static int n_breeds = 0;

struct curl_buf {
	char *data;
	size_t size;
};

static size_t write_buf(
	void *contents,
	size_t size,
	size_t nmemb,
	void *userp
) {
	size_t real_size = size * nmemb;
	struct curl_buf *buf = (struct curl_buf *)userp;
	
	char *ptr = realloc(buf->data, buf->size + real_size + 1);
	buf->data = ptr;

	memcpy(buf->data[buf->size], contents, real_size);
	buf->size += real_size;
	buf->data[buf->size] = 0;

	return real_size;
}

static cJSON *get_payload(struct curl_buf *chunk, const char *url) {
    cJSON *payload = NULL;
    CURL *curl_handle = NULL;
    CURLcode response;
    cJSON *parsed = NULL;

    chunk->data = malloc(1);  /* grown as needed by realloc */
    chunk->size = 0;

    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_buf);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *) chunk);
    response = curl_easy_perform(curl_handle);
    if (response != CURLE_OK) {
        return NULL;
    }
    curl_easy_cleanup(curl_handle);
    parsed = cJSON_Parse(chunk->data);
    free(chunk->data);
    return parsed;
}

static void fill_breed_names() {
    struct curl_buf chunk = { 0 };
    cJSON *payload = NULL;
    const cJSON *last_page = NULL;
    const cJSON *breed_data = NULL;
    const cJSON *breed = NULL;

    int n_pages = 0;
    char url[MAX_URL_LEN] = "";
    char query[10];
    int page = 0;
    int i = 0;

    payload = get_payload(&chunk, BREEDS_URL);

    last_page = cJSON_GetObjectItemCaseSensitive(payload, "last_page");
    n_pages = last_page->valueint;

    page = 1;
    while (1) {
        breed_data = cJSON_GetObjectItemCaseSensitive(payload, "data");
        cJSON_ArrayForEach(breed, breed_data) {
            // TODO: make it look less crowded
            const cJSON *name = cJSON_GetObjectItemCaseSensitive(breed, "breed");
            const cJSON *country = cJSON_GetObjectItemCaseSensitive(breed, "country");
            const cJSON *origin = cJSON_GetObjectItemCaseSensitive(breed, "origin");
            const cJSON *coat = cJSON_GetObjectItemCaseSensitive(breed, "coat");
            const cJSON *pattern = cJSON_GetObjectItemCaseSensitive(breed, "pattern");
            strncpy(breeds[n_breeds], name->valuestring, MAX_BREED_NAME_LEN);
            strncpy(countries[n_breeds], country->valuestring, MAX_BREED_NAME_LEN);
            strncpy(origins[n_breeds], origin->valuestring, MAX_BREED_NAME_LEN);
            strncpy(coats[n_breeds], coat->valuestring, MAX_BREED_NAME_LEN);
            strncpy(patterns[n_breeds], pattern->valuestring, MAX_BREED_NAME_LEN);
            n_breeds++;
        }
        cJSON_Delete(payload);

        page++;
        if (page > n_pages) {
            break;
        }
        strncpy(url, BREEDS_URL, MAX_URL_LEN);
        sprintf(query, "?page=%d", page);
        strcat(url, query);
        payload = get_payload(&chunk, url);
    }
}

static int write_file(const char *path) {
	FILE *fp = fopen(path, "w");
	if (fp == NULL) {
		return -errno;
	}

	char data[10 * MAX_BREED_NAME_LEN];


	return 0;
}

static int read_file(const char *path) {
	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		return -errno;
	}

	return 0;
}

int main(int argc, char *argv[]) {
    curl_global_init(CURL_GLOBAL_ALL);
    fill_breed_names();
    /* cur_global_cleanup(); */
    // FILE OPERATIONS
    return 0;
}