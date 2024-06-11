#ifndef FILLDATA_H
#define FILLDATA_H

static void fill_breed_names();

static int file_exists(const char *filename);
static int get_file_size(const char *filename, off_t *size);

static int write_file(const char *filename, const char *buf);
static int read_file(const char *filename, char *buf, size_t size);

#endif // FILLDATA_H
