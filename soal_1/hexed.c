#define FUSE_USE_VERSION 30
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <fuse3/fuse.h>
#include <unistd.h>
#include <pwd.h>

#define MAX_PATH 1024 
#define MAX_FILE_SIZE 4500000 

uid_t user_id;
gid_t group_id;

// Fungsi untuk cek apakah string adalah hex valid
int is_valid_hex(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (!isxdigit((unsigned char)str[i])) {
            return 0; // Bukan hex
        }
    }
    return 1; // Hex valid
}

int fetch_anomaly_texts() {
    const char *download_cmd = "wget --no-check-certificate \"https://drive.google.com/uc?export=download&id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5\" -O anomaly.zip";
    int ret = system(download_cmd);
    if (ret != 0) {
        fprintf(stderr, "Gagal download file zip: %s\n", strerror(errno));
        return -1;
    }

    ret = system("unzip -o anomaly.zip -d .");
    if (ret != 0) {
        fprintf(stderr, "Gagal unzip file: %s\n", strerror(errno));
        return -1;
    }

    ret = system("rm anomaly.zip");
    if (ret != 0) {
        fprintf(stderr, "Gagal hapus file zip: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int convert_hex_to_image() {
    struct passwd *pw = getpwuid(user_id);
    if (pw == NULL) {
        fprintf(stderr, "Gagal mendapatkan informasi pengguna: %s\n", strerror(errno));
        return -1;
    }

    if (mkdir("anomali/image", 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Gagal buat folder anomali/image: %s\n", strerror(errno));
        return -1;
    }

    if (chown("anomali/image", user_id, group_id) != 0) {
        fprintf(stderr, "Gagal mengubah kepemilikan anomali/image: %s\n", strerror(errno));
        return -1;
    }
    if (chmod("anomali/image", 0755) != 0) {
        fprintf(stderr, "Gagal mengubah izin anomali/image: %s\n", strerror(errno));
        return -1;
    }

    DIR *dp = opendir("anomali"); 
    if (dp == NULL) {
        fprintf(stderr, "Gagal buka folder anomali: %s\n", strerror(errno));
        return -1;
    }

    FILE *log_file = fopen("anomali/conversion.log", "a");
    if (log_file == NULL) {
        fprintf(stderr, "Gagal buka file anomali/conversion.log: %s\n", strerror(errno));
        closedir(dp);
        return -1;
    }

    if (chown("anomali/conversion.log", user_id, group_id) != 0) {
        fprintf(stderr, "Gagal mengubah kepemilikan anomali/conversion.log: %s\n", strerror(errno));
        fclose(log_file);
        closedir(dp);
        return -1;
    }
    if (chmod("anomali/conversion.log", 0644) != 0) {
        fprintf(stderr, "Gagal mengubah izin anomali/conversion.log: %s\n", strerror(errno));
        fclose(log_file);
        closedir(dp);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL) {
        if (strstr(entry->d_name, ".txt") == NULL) continue;

        char input_path[MAX_PATH];
        snprintf(input_path, sizeof(input_path), "anomali/%s", entry->d_name);
        printf("Mencoba buka file: %s\n", input_path); 
        FILE *input_file = fopen(input_path, "r");
        if (input_file == NULL) {
            fprintf(stderr, "Gagal buka file %s: %s\n", input_path, strerror(errno));
            continue;
        }

        fseek(input_file, 0, SEEK_END);
        long file_size = ftell(input_file);
        if (file_size < 0 || file_size > MAX_FILE_SIZE) {
            fprintf(stderr, "Ukuran file %s (%ld) melebihi batas %d, dilewati\n", input_path, file_size, MAX_FILE_SIZE);
            fclose(input_file);
            continue;
        }
        fseek(input_file, 0, SEEK_SET);

        char *hex_buffer = malloc(file_size + 1);
        if (hex_buffer == NULL) {
            fprintf(stderr, "Gagal alokasi memori untuk file %s (ukuran %ld bytes), coba ulang atau skip\n", input_path, file_size);
            fclose(input_file);
            continue;
        }

        size_t bytes_read = fread(hex_buffer, 1, file_size, input_file);
        if (bytes_read != (size_t)file_size) {
            fprintf(stderr, "Gagal baca file %s: Hanya terbaca %zu dari %ld bytes\n", input_path, bytes_read, file_size);
            free(hex_buffer);
            fclose(input_file);
            continue;
        }
        hex_buffer[file_size] = '\0';
        fclose(input_file);

        if (!is_valid_hex(hex_buffer, file_size)) {
            fprintf(stderr, "File %s berisi data non-hex, dilewati\n", input_path);
            free(hex_buffer);
            continue;
        }

        char base_name[MAX_PATH];
        strncpy(base_name, entry->d_name, sizeof(base_name) - 1);
        base_name[sizeof(base_name) - 1] = '\0';
        char *dot = strrchr(base_name, '.');
        if (dot) *dot = '\0'; 

        time_t rawtime;
        struct tm *timeinfo;
        char timestamp[20];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H:%M:%S", timeinfo);

        char output_name[MAX_PATH];
        int len = snprintf(output_name, sizeof(output_name), "%s_image_%s.png", base_name, timestamp);
        if (len >= (int)sizeof(output_name)) {
            fprintf(stderr, "Nama file %s terlalu panjang, dipotong\n", output_name);
            output_name[sizeof(output_name) - 1] = '\0';
        }

        char output_path[MAX_PATH];
        len = snprintf(output_path, sizeof(output_path), "anomali/image/%s", output_name);
        if (len >= (int)sizeof(output_path)) {
            fprintf(stderr, "Path file %s terlalu panjang, dipotong\n", output_path);
            output_path[sizeof(output_path) - 1] = '\0';
        }

        size_t hex_len = strlen(hex_buffer);
        size_t bin_len = hex_len / 2; 
        unsigned char *bin_buffer = malloc(bin_len);
        if (bin_buffer == NULL) {
            fprintf(stderr, "Gagal alokasi memori untuk biner file %s (ukuran %zu bytes), coba ulang atau skip\n", output_name, bin_len);
            free(hex_buffer);
            continue;
        }

        for (size_t i = 0; i < bin_len; i++) {
            unsigned int byte;
            int result = sscanf(&hex_buffer[i * 2], "%2x", &byte);
            if (result != 1) {
                fprintf(stderr, "Gagal konversi hex pada posisi %zu di file %s\n", i * 2, input_path);
                free(hex_buffer);
                free(bin_buffer);
                goto skip_file;
            }
            bin_buffer[i] = (unsigned char)byte;
        }

        FILE *output_file = fopen(output_path, "wb");
        if (output_file == NULL) {
            fprintf(stderr, "Gagal buat file gambar %s: %s\n", output_path, strerror(errno));
            free(hex_buffer);
            free(bin_buffer);
            continue;
        }
        size_t bytes_written = fwrite(bin_buffer, 1, bin_len, output_file);
        if (bytes_written != bin_len) {
            fprintf(stderr, "Gagal tulis file gambar %s: Hanya tertulis %zu dari %zu bytes\n", output_path, bytes_written, bin_len);
        }
        fclose(output_file);

        if (chown(output_path, user_id, group_id) != 0) {
            fprintf(stderr, "Gagal mengubah kepemilikan %s: %s\n", output_path, strerror(errno));
        }
        if (chmod(output_path, 0644) != 0) {
            fprintf(stderr, "Gagal mengubah izin %s: %s\n", output_path, strerror(errno));
        }

        char log_entry[1024];
        strftime(log_entry, sizeof(log_entry), "[%Y-%m-%d][%H:%M:%S]", timeinfo);
        snprintf(log_entry + strlen(log_entry), sizeof(log_entry) - strlen(log_entry),
                 ": Successfully converted hexadecimal text %s to %s\n", entry->d_name, output_name);
        fprintf(log_file, "%s", log_entry);

        printf("Berhasil konversi %s ke %s\n", entry->d_name, output_name);

    skip_file:
        free(hex_buffer);
        free(bin_buffer);
    }

    fclose(log_file);
    closedir(dp);
    return 0;
}

static int fuse_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    char real_path[MAX_PATH];
    snprintf(real_path, sizeof(real_path), "anomali%s", path);
    if (lstat(real_path, stbuf) == -1) {
        return -errno;
    }
    return 0;
}

static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;
    if (strcmp(path, "/") != 0) return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    DIR *dp = opendir("anomali");
    if (dp == NULL) {
        fprintf(stderr, "Gagal buka folder anomali di FUSE: %s\n", strerror(errno));
        return -errno;
    }

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        struct stat st;
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "anomali/%s", entry->d_name);
        if (lstat(full_path, &st) == -1) continue;
        filler(buf, entry->d_name, &st, 0, 0);
    }
    closedir(dp);
    return 0;
}

static int fuse_open(const char *path, struct fuse_file_info *fi) {
    char real_path[MAX_PATH];
    snprintf(real_path, sizeof(real_path), "anomali%s", path);
    int fd = open(real_path, fi->flags);
    if (fd == -1) return -errno;
    fi->fh = fd;
    return 0;
}

static int fuse_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    return pread(fi->fh, buf, size, offset);
}

static int fuse_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
    return pwrite(fi->fh, buf, size, offset);
}

static int fuse_truncate(const char *path, off_t size,
                        struct fuse_file_info *fi) {
    (void) fi;
    char real_path[MAX_PATH];
    snprintf(real_path, sizeof(real_path), "anomali%s", path);
    return truncate(real_path, size);
}

static int fuse_chmod(const char *path, mode_t mode,
                     struct fuse_file_info *fi) {
    (void) fi;
    char real_path[MAX_PATH];
    snprintf(real_path, sizeof(real_path), "anomali%s", path);
    return chmod(real_path, mode);
}

static int fuse_release(const char *path, struct fuse_file_info *fi) {
    close(fi->fh);
    return 0;
}

static struct fuse_operations fuse_ops = {
    .getattr = fuse_getattr,
    .readdir = fuse_readdir,
    .open = fuse_open,
    .read = fuse_read,
    .write = fuse_write,
    .truncate = fuse_truncate,
    .chmod = fuse_chmod,
    .release = fuse_release,
};

int main(int argc, char *argv[]) {
    user_id = getuid();
    group_id = getgid();

    printf("Mengambil sampel anomali teks...\n");
    if (fetch_anomaly_texts() != 0) {
        fprintf(stderr, "Proses langkah A gagal, keluar...\n");
        return 1;
    }
    printf("Sampel anomali teks berhasil diambil dan file zip dihapus.\n");

    printf("Mengonversi teks hex ke gambar...\n");
    if (convert_hex_to_image() != 0) {
        fprintf(stderr, "Proses langkah B, C, dan D gagal, keluar...\n");
        return 1;
    }
    printf("Konversi hex ke gambar selesai.\n");

    printf("Isi anomali sebelum mount:\n");
    system("ls -l anomali");
    system("ls -l anomali/image");

    char *new_argv[argc + 5]; 
    int new_argc = 0;

   
    for (int i = 0; i < argc; i++) {
        new_argv[new_argc++] = argv[i];
    }

  
    new_argv[new_argc++] = strdup("-o");
    new_argv[new_argc++] = strdup("allow_root");
    new_argv[new_argc++] = strdup("-f"); 
    new_argv[new_argc] = NULL; 

   
    int ret = fuse_main(new_argc, new_argv, &fuse_ops, NULL);

    for (int i = argc; i < new_argc; i++) {
        free(new_argv[i]);
    }

    return ret;
}