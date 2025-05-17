#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>
#include <stdbool.h>
#include <ctype.h>

#define RELIC_DIR "/home/mutiaradiva/soal_2/relics"
#define LOG_FILE "/home/mutiaradiva/soal_2/activity.log"
#define MAX_CHUNK_SIZE 1024

static const char *MOUNT_DIR = "/home/mutiaradiva/soal_2/mount_dir";

typedef struct {
    char *name;
    size_t chunk_count;
} VirtualFile;

static VirtualFile *virtual_files = NULL;
static size_t file_count = 0;

// Prototipe fungsi
static int baymax_getattr(const char *, struct stat *, struct fuse_file_info *);
static int baymax_readdir(const char *, void *, fuse_fill_dir_t, off_t, 
                         struct fuse_file_info *, enum fuse_readdir_flags);
static int baymax_open(const char *, struct fuse_file_info *);
static int baymax_read(const char *, char *, size_t, off_t, struct fuse_file_info *);
static int baymax_create(const char *, mode_t, struct fuse_file_info *);
static int baymax_write(const char *, const char *, size_t, off_t, struct fuse_file_info *);
static int baymax_unlink(const char *);
static void scan_relics();

static struct fuse_operations baymax_oper = {
    .getattr = baymax_getattr,
    .readdir = baymax_readdir,
    .open    = baymax_open,
    .read    = baymax_read,
    .create  = baymax_create,
    .write   = baymax_write,
    .unlink  = baymax_unlink,
};

void log_activity(const char *message) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    fprintf(log, "[%s] %s\n", timestamp, message);
    fclose(log);
}

int is_valid_chunk(const char *name) {
    const char *dot = strrchr(name, '.');
    if (!dot || strlen(dot) != 4) return 0;
    return isdigit(dot[1]) && isdigit(dot[2]) && isdigit(dot[3]);
}

void scan_relics() {
    DIR *dir = opendir(RELIC_DIR);
    if (!dir) return;

    struct dirent *ent;
    while ((ent = readdir(dir))) {
        if (ent->d_type == DT_REG) {
            char *dot = strrchr(ent->d_name, '.');
            if (dot && strlen(dot) == 4 && isdigit(dot[1]) && isdigit(dot[2]) && isdigit(dot[3])) {
                char *base = strndup(ent->d_name, dot - ent->d_name);
                int chunk_index = atoi(dot + 1);

                // Cek apakah file ini sudah ada
                bool found = false;
                for (size_t i = 0; i < file_count; i++) {
                    if (strcmp(virtual_files[i].name, base) == 0) {
                        if (chunk_index + 1 > virtual_files[i].chunk_count) {
                            virtual_files[i].chunk_count = chunk_index + 1;
                        }
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    virtual_files = realloc(virtual_files, (file_count + 1) * sizeof(VirtualFile));
                    virtual_files[file_count].name = base;
                    virtual_files[file_count].chunk_count = chunk_index + 1;
                    file_count++;
                } else {
                    free(base);
                }
            }
        }
    }
    closedir(dir);
}

static int baymax_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    const char *filename = path + 1;
    for (size_t i = 0; i < file_count; i++) {
        if (strcmp(virtual_files[i].name, filename) == 0) {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            
            size_t total_size = 0;
            for (int j = 0; j < virtual_files[i].chunk_count; j++) {
                char chunk_path[256];
                snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_DIR, filename, j);
                struct stat chunk_stat;
                if (stat(chunk_path, &chunk_stat) == 0) {
                    total_size += chunk_stat.st_size;
                }
            }
            
            stbuf->st_size = total_size;
            return 0;
        }
    }

    return -ENOENT;
}

static int baymax_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    if (strcmp(path, "/") != 0) return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    // Tampilkan file unik dari virtual_files
    for (size_t i = 0; i < file_count; i++) {
        filler(buf, virtual_files[i].name, NULL, 0, 0);
    }

    return 0;
}

static int baymax_open(const char *path, struct fuse_file_info *fi) {
    const char *filename = path + 1;
    for (size_t i = 0; i < file_count; i++) {
        if (strcmp(virtual_files[i].name, filename) == 0) {
            char logmsg[256];
            snprintf(logmsg, sizeof(logmsg), "READ: %s", filename);
            log_activity(logmsg);
            return 0;
        }
    }
    return -ENOENT;
}

static int baymax_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    const char *filename = path + 1;
    size_t total_read = 0;
    off_t current_offset = 0;
    int chunk_num = 0;

    while (total_read < size) {
        char chunk_path[256];
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_DIR, filename, chunk_num);
        
        FILE *fp = fopen(chunk_path, "rb");
        if (!fp) break;

        fseek(fp, 0, SEEK_END);
        long chunk_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (offset < current_offset + chunk_size) {
            off_t rel_offset = offset > current_offset ? offset - current_offset : 0;
            size_t to_read = chunk_size - rel_offset;
            if (to_read > size - total_read) to_read = size - total_read;

            fseek(fp, rel_offset, SEEK_SET);
            size_t read = fread(buf + total_read, 1, to_read, fp);
            total_read += read;
        }

        fclose(fp);
        current_offset += chunk_size;
        chunk_num++;
        
        if (current_offset > offset + size) break;
    }

    return total_read;
}

static int baymax_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char logmsg[256];
    snprintf(logmsg, sizeof(logmsg), "CREATE: %s", path + 1);
    log_activity(logmsg);
    
    // Tambahkan ke virtual files
    const char *filename = path + 1;
    virtual_files = realloc(virtual_files, (file_count + 1) * sizeof(VirtualFile));
    virtual_files[file_count].name = strdup(filename);
    virtual_files[file_count].chunk_count = 0;
    file_count++;
    
    return 0;
}

static int baymax_write(const char *path, const char *buf, size_t size, off_t offset, 
                       struct fuse_file_info *fi) {
    const char *filename = path + 1;
    size_t file_index = (size_t)-1;
    
    // Cari file di virtual_files
    for (size_t i = 0; i < file_count; i++) {
        if (strcmp(virtual_files[i].name, filename) == 0) {
            file_index = i;
            break;
        }
    }
    if (file_index == (size_t)-1) return -ENOENT;

    size_t chunk_count = (size + MAX_CHUNK_SIZE - 1) / MAX_CHUNK_SIZE;
    char chunk_list[1024] = {0};
    char *ptr = chunk_list;

    // Tulis ke chunk files dan bangun daftar chunk
    for (size_t i = 0; i < chunk_count; i++) {
        char chunk_path[256];
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03zu", RELIC_DIR, filename, i);
        
        FILE *fp = fopen(chunk_path, "wb");
        if (!fp) return -EIO;

        size_t to_write = (i == chunk_count - 1) ? 
                         size - (i * MAX_CHUNK_SIZE) : MAX_CHUNK_SIZE;
        fwrite(buf + (i * MAX_CHUNK_SIZE), 1, to_write, fp);
        fclose(fp);

        // Tambahkan ke daftar chunk
        if (i > 0) ptr += sprintf(ptr, ", ");
        ptr += sprintf(ptr, "%s.%03zu", filename, i);
    }

    // Update chunk count jika perlu
    if (chunk_count > virtual_files[file_index].chunk_count) {
        virtual_files[file_index].chunk_count = chunk_count;
    }

    // Format log sesuai permintaan
    char logmsg[2048];
    snprintf(logmsg, sizeof(logmsg), "WRITE: %s -> %s", filename, chunk_list);
    log_activity(logmsg);

    return size;
}

static int baymax_unlink(const char *path) {
    const char *filename = path + 1;
    int deleted = 0;
    
    // Hapus dari virtual_files
    for (size_t i = 0; i < file_count; i++) {
        if (strcmp(virtual_files[i].name, filename) == 0) {
            free(virtual_files[i].name);
            memmove(&virtual_files[i], &virtual_files[i+1], 
                   (file_count - i - 1) * sizeof(VirtualFile));
            file_count--;
            break;
        }
    }
    
    // Hapus chunk files
// Hapus chunk files
char chunk_path[256];
int i = 0;
while (1) {
    snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELIC_DIR, filename, i);
    
    // Jika file tidak ada, asumsikan semua chunk sudah dihapus
    if (access(chunk_path, F_OK) != 0) break;

    if (remove(chunk_path) == 0) {
        deleted++;
    }

    i++;
}
    
    char logmsg[256];
    snprintf(logmsg, sizeof(logmsg), "DELETE: %s ", filename);
    log_activity(logmsg);
    
    return 0;
}

int main(int argc, char *argv[]) {
    scan_relics();
    return fuse_main(argc, argv, &baymax_oper, NULL);
}
