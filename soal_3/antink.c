#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

static const char *src_dir   = "/it24_host";
static const char *mount_dir = "/antink_mount";
static const char *log_path  = "/var/log/it24.log";
static const char *keywords[] = { "nafis", "kimcun" };
static const int   n_keys    = 2;

// helper: cek apakah name mengandung salah satu keyword (case-insensitive)
static int is_danger(const char *name) {
    for (int i = 0; i < n_keys; i++) {
        size_t kwlen = strlen(keywords[i]);
        for (const char *p = name; *p; p++) {
            if (strncasecmp(p, keywords[i], kwlen) == 0)
                return 1;
        }
    }
    return 0;
}

// reverse string in-place
static void str_reverse(char *s) {
    int i = 0, j = strlen(s) - 1;
    while (i < j) {
        char t = s[i]; s[i] = s[j]; s[j] = t;
        i++; j--;
    }
}

// reverse entire filename including extension
static void reverse_name(const char *orig, char *out) {
    strcpy(out, orig);
    str_reverse(out);
}

// logging with timestamp
static void log_msg(const char *fmt, const char *arg1, const char *arg2) {
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);

    char timestr[32];
    strftime(timestr, sizeof(timestr), "[%Y-%m-%d %H:%M:%S]", &tm);

    FILE *f = fopen(log_path, "a");
    if (!f) return;
    fprintf(f, "%s ", timestr);
    if (arg2)
        fprintf(f, fmt, arg1, arg2);
    else
        fprintf(f, fmt, arg1);
    fprintf(f, "\n");
    fclose(f);
}

// FUSE callbacks
static void *antink_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    (void) conn;
    cfg->kernel_cache = 1;
    return NULL;
}

static int antink_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
    (void) fi;
    char full[1024];
    snprintf(full, sizeof(full), "%s%s", src_dir, path);
    int res = lstat(full, st);
    return res == -1 ? -errno : 0;
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;
    char dirp[1024];
    snprintf(dirp, sizeof(dirp), "%s%s", src_dir, path);
    DIR *d = opendir(dirp);
    if (!d) return -errno;

    struct dirent *de;
    while ((de = readdir(d))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        if (is_danger(de->d_name)) {
            log_msg("[ALERT] Anomaly detected %s in file: %s", de->d_name, dirp);

            char rev[256];
            reverse_name(de->d_name, rev);
            log_msg("[REVERSE] File %s has been reversed : %s", de->d_name, rev);

            filler(buf, rev, NULL, 0, 0);
        } else {
            filler(buf, de->d_name, NULL, 0, 0);
        }
    }
    closedir(d);
    return 0;
}

static int antink_open(const char *path, struct fuse_file_info *fi) {
    const char *p = path + 1;
    char real[256];
    strcpy(real, p);
    if (is_danger(real)) {
        char tmp[256];
        reverse_name(real, tmp);
        strcpy(real, tmp);
    }
    char full[1024];
    snprintf(full, sizeof(full), "%s/%s", src_dir, real);
    int fd = open(full, fi->flags);
    if (fd == -1) return -errno;
    fi->fh = fd;
    return 0;
}

static int antink_read(const char *path, char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi) {
    (void) path;
    int fd = fi->fh;
    char data[size];
    int res = pread(fd, data, size, offset);
    if (res < 0) return -errno;

    const char *p = path + 1;
    if (strstr(p, ".txt") && !is_danger(p)) {
        for (int i = 0; i < res; i++) {
            char c = data[i];
            if (c >= 'a' && c <= 'z')      c = 'a' + (c - 'a' + 13) % 26;
            else if (c >= 'A' && c <= 'Z') c = 'A' + (c - 'A' + 13) % 26;
            buf[i] = c;
        }
        log_msg("[ENCRYPT] File %s has been encrypted", p, NULL);
    } else {
        memcpy(buf, data, res);
        log_msg("[READ] File %s no encryption", p, NULL);
    }
    return res;
}

static int antink_release(const char *path, struct fuse_file_info *fi) {
    close(fi->fh);
    return 0;
}

static struct fuse_operations antink_ops = {
    .init    = antink_init,
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open    = antink_open,
    .read    = antink_read,
    .release = antink_release,
};

int main(int argc, char *argv[]) {
    char *fuse_argv[] = { "antink", "/antink_mount", "-f", NULL };
    return fuse_main(3, fuse_argv, &antink_ops, NULL);
}
