#ifndef CAT_H
#define CAT_H

static size_t write_buf(void *contents, size_t size, size_t nmemb, void *userp);
static cJSON *get_payload(struct curl_buf *chunk, const char *url);
static void fill_breed_names();

static int write_file(const char *path);
static int read_file(const char *path);

#endif // CAT_H