#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdlib.h> // Added for malloc and free
#include <openssl/evp.h> // For AES-256-CBC
#include <openssl/rand.h> // For random IV
#include <zlib.h> // For gzip compression

#define CHIHO_BASE "chiho/"
#define STARTER_EXT ".mai"
#define METRO_EXT ".ccc"
#define DRAGON_EXT ".rot"
#define BLACKROSE_EXT ".bin"
#define HEAVEN_EXT ".enc"
#define SKYSTREET_EXT ".gz"

// Helper function to create chiho directory if it doesn't exist
void ensure_chiho_dir() {
    struct stat st = {0};
    if (stat(CHIHO_BASE, &st) == -1) {
        mkdir(CHIHO_BASE, 0755);
    }
    if (stat(CHIHO_BASE "starter", &st) == -1) {
        mkdir(CHIHO_BASE "starter", 0755);
    }
    if (stat(CHIHO_BASE "metro", &st) == -1) {
        mkdir(CHIHO_BASE "metro", 0755);
    }
    if (stat(CHIHO_BASE "dragon", &st) == -1) {
        mkdir(CHIHO_BASE "dragon", 0755);
    }
    if (stat(CHIHO_BASE "blackrose", &st) == -1) {
        mkdir(CHIHO_BASE "blackrose", 0755);
    }
    if (stat(CHIHO_BASE "heaven", &st) == -1) {
        mkdir(CHIHO_BASE "heaven", 0755);
    }
    if (stat(CHIHO_BASE "skystreet", &st) == -1) {
        mkdir(CHIHO_BASE "skystreet", 0755);
    }
}

// Helper function for ROT13 transformation (for Dragon Chiho)
void rot13_transform(const char *input, char *output, size_t len) {
    for (size_t i = 0; i < len && input[i] != '\0'; i++) {
        if ((input[i] >= 'A' && input[i] <= 'Z')) {
            output[i] = 'A' + ((input[i] - 'A' + 13) % 26);
        } else if ((input[i] >= 'a' && input[i] <= 'z')) {
            output[i] = 'a' + ((input[i] - 'a' + 13) % 26);
        } else {
            output[i] = input[i]; // Non-letters unchanged
        }
    }
    output[len] = '\0';
}

// AES-256-CBC Encryption
int aes_encrypt(const unsigned char *plaintext, int plaintext_len, unsigned char *key,
                unsigned char *iv, unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len;
    int ciphertext_len;

    if (!ctx) {
        fprintf(stderr, "aes_encrypt: failed to create EVP_CIPHER_CTX\n");
        return -1;
    }

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        fprintf(stderr, "aes_encrypt: EVP_EncryptInit_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
        fprintf(stderr, "aes_encrypt: EVP_EncryptUpdate failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
        fprintf(stderr, "aes_encrypt: EVP_EncryptFinal_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

// AES-256-CBC Decryption
int aes_decrypt(const unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                unsigned char *iv, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len;
    int plaintext_len;

    if (!ctx) {
        fprintf(stderr, "aes_decrypt: failed to create EVP_CIPHER_CTX\n");
        return -1;
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        fprintf(stderr, "aes_decrypt: EVP_DecryptInit_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        fprintf(stderr, "aes_decrypt: EVP_DecryptUpdate failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        fprintf(stderr, "aes_decrypt: EVP_DecryptFinal_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}

// Gzip Compression
int gzip_compress(const unsigned char *input, size_t input_len, unsigned char **output, size_t *output_len) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        fprintf(stderr, "gzip_compress: deflateInit2 failed\n");
        return -1;
    }

    strm.next_in = (unsigned char *)input;
    strm.avail_in = input_len;
    *output = NULL;
    *output_len = 0;

    unsigned char out[1024];
    do {
        strm.next_out = out;
        strm.avail_out = sizeof(out);
        if (deflate(&strm, Z_FINISH) == Z_STREAM_ERROR) {
            fprintf(stderr, "gzip_compress: deflate failed\n");
            deflateEnd(&strm);
            return -1;
        }
        size_t have = sizeof(out) - strm.avail_out;
        *output = realloc(*output, *output_len + have);
        if (!*output) {
            fprintf(stderr, "gzip_compress: realloc failed\n");
            deflateEnd(&strm);
            return -1;
        }
        memcpy(*output + *output_len, out, have);
        *output_len += have;
    } while (strm.avail_out == 0);

    deflateEnd(&strm);
    return Z_OK;
}

// Gzip Decompression
int gzip_decompress(const unsigned char *input, size_t input_len, unsigned char **output, size_t *output_len) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK) {
        fprintf(stderr, "gzip_decompress: inflateInit2 failed\n");
        return -1;
    }

    strm.next_in = (unsigned char *)input;
    strm.avail_in = input_len;
    *output = NULL;
    *output_len = 0;

    unsigned char out[1024];
    int ret;
    do {
        strm.next_out = out;
        strm.avail_out = sizeof(out);
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR) {
            fprintf(stderr, "gzip_decompress: inflate failed\n");
            inflateEnd(&strm);
            return -1;
        }
        size_t have = sizeof(out) - strm.avail_out;
        *output = realloc(*output, *output_len + have);
        if (!*output) {
            fprintf(stderr, "gzip_decompress: realloc failed\n");
            inflateEnd(&strm);
            return -1;
        }
        memcpy(*output + *output_len, out, have);
        *output_len += have;
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : -1;
}


void get_chiho_path(const char *fuse_path, char *chiho_path) {
    snprintf(chiho_path, 256, "%s", CHIHO_BASE);

    // Handle root
    if (strcmp(fuse_path, "/") == 0) {
        chiho_path[strlen(CHIHO_BASE) - 1] = '\0'; 
        return;
    }

    const char *area = strstr(fuse_path, "/7sref");
    if (area) {
        area += 6;
        if (*area == '\0' || strcmp(area, "/") == 0) {
            snprintf(chiho_path, 256, "%s", CHIHO_BASE);
            chiho_path[strlen(CHIHO_BASE) - 1] = '\0'; 
            return;
        }
        const char *filename = area + 1; 
        char target_area[32] = {0};
        const char *underscore = strchr(filename, '_');
        if (!underscore) {
            snprintf(chiho_path, 256, "%s", CHIHO_BASE);
            chiho_path[strlen(CHIHO_BASE) - 1] = '\0'; 
            return;
        }
        size_t area_len = underscore - filename;
        if (area_len >= sizeof(target_area)) {
            snprintf(chiho_path, 256, "%s", CHIHO_BASE);
            chiho_path[strlen(CHIHO_BASE) - 1] = '\0'; 
            return;
        }
        strncpy(target_area, filename, area_len);
        const char *actual_filename = underscore + 1;

    
        if (strcmp(target_area, "starter") == 0) {
            snprintf(chiho_path, 256, "%sstarter/%s%s", CHIHO_BASE, actual_filename, STARTER_EXT);
        } else if (strcmp(target_area, "metro") == 0) {
            snprintf(chiho_path, 256, "%smetro/%s%s", CHIHO_BASE, actual_filename, METRO_EXT);
        } else if (strcmp(target_area, "dragon") == 0) {
            snprintf(chiho_path, 256, "%sdragon/%s%s", CHIHO_BASE, actual_filename, DRAGON_EXT);
        } else if (strcmp(target_area, "blackrose") == 0) {
            snprintf(chiho_path, 256, "%sblackrose/%s%s", CHIHO_BASE, actual_filename, BLACKROSE_EXT);
        } else if (strcmp(target_area, "heaven") == 0) {
            snprintf(chiho_path, 256, "%sheaven/%s%s", CHIHO_BASE, actual_filename, HEAVEN_EXT);
        } else if (strcmp(target_area, "skystreet") == 0) {
            snprintf(chiho_path, 256, "%sskystreet/%s%s", CHIHO_BASE, actual_filename, SKYSTREET_EXT);
        } else {
            snprintf(chiho_path, 256, "%s", CHIHO_BASE);
            chiho_path[strlen(CHIHO_BASE) - 1] = '\0'; 
            return;
        }
    } else {
        
        area = strstr(fuse_path, "/starter");
        if (area) {
            area += 8; 
            strcat(chiho_path, "starter/");
            if (*area == '\0' || strcmp(area, "/") == 0) {
                chiho_path[strlen(chiho_path) - 1] = '\0'; 
                return;
            }
            const char *filename = area + 1; 
            strncat(chiho_path, filename, 256 - strlen(chiho_path) - 1);
            strncat(chiho_path, STARTER_EXT, 256 - strlen(chiho_path) - 1);
        } else if ((area = strstr(fuse_path, "/metro"))) {
            area += 6;
            strcat(chiho_path, "metro/");
            if (*area == '\0' || strcmp(area, "/") == 0) {
                chiho_path[strlen(chiho_path) - 1] = '\0'; 
                return;
            }
            const char *filename = area + 1; 
            strncat(chiho_path, filename, 256 - strlen(chiho_path) - 1);
            strncat(chiho_path, METRO_EXT, 256 - strlen(chiho_path) - 1); 
        } else if ((area = strstr(fuse_path, "/dragon"))) {
            area += 7; 
            strcat(chiho_path, "dragon/");
            if (*area == '\0' || strcmp(area, "/") == 0) {
                chiho_path[strlen(chiho_path) - 1] = '\0'; 
                return;
            }
            const char *filename = area + 1; 
            strncat(chiho_path, filename, 256 - strlen(chiho_path) - 1);
            strncat(chiho_path, DRAGON_EXT, 256 - strlen(chiho_path) - 1); 
        } else if ((area = strstr(fuse_path, "/blackrose"))) {
            area += 10; 
            strcat(chiho_path, "blackrose/");
            if (*area == '\0' || strcmp(area, "/") == 0) {
                chiho_path[strlen(chiho_path) - 1] = '\0'; 
                return;
            }
            const char *filename = area + 1; 
            strncat(chiho_path, filename, 256 - strlen(chiho_path) - 1);
            strncat(chiho_path, BLACKROSE_EXT, 256 - strlen(chiho_path) - 1); 
        } else if ((area = strstr(fuse_path, "/heaven"))) {
            area += 7; 
            strcat(chiho_path, "heaven/");
            if (*area == '\0' || strcmp(area, "/") == 0) {
                chiho_path[strlen(chiho_path) - 1] = '\0'; 
                return;
            }
            const char *filename = area + 1; 
            strncat(chiho_path, filename, 256 - strlen(chiho_path) - 1);
            strncat(chiho_path, HEAVEN_EXT, 256 - strlen(chiho_path) - 1); 
        } else if ((area = strstr(fuse_path, "/skystreet"))) {
            area += 10; 
            strcat(chiho_path, "skystreet/");
            if (*area == '\0' || strcmp(area, "/") == 0) {
                chiho_path[strlen(chiho_path) - 1] = '\0'; 
                return;
            }
            const char *filename = area + 1; 
            strncat(chiho_path, filename, 256 - strlen(chiho_path) - 1);
            strncat(chiho_path, SKYSTREET_EXT, 256 - strlen(chiho_path) - 1); 
        } else {
            chiho_path[strlen(CHIHO_BASE) - 1] = '\0'; 
            return;
        }
    }

    fprintf(stderr, "get_chiho_path: %s -> %s\n", fuse_path, chiho_path);
}

static int maimai_getattr(const char *path, struct stat *stbuf) {
    char chiho_path[256];
    get_chiho_path(path, chiho_path);

    int res = lstat(chiho_path, stbuf);
    if (res == -1) {
        if (strcmp(path, "/") == 0 || strstr(path, "/starter") == path + 1 ||
            strstr(path, "/metro") == path + 1 || strstr(path, "/dragon") == path + 1 ||
            strstr(path, "/blackrose") == path + 1 || strstr(path, "/heaven") == path + 1 ||
            strstr(path, "/skystreet") == path + 1 || strstr(path, "/7sref") == path + 1) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
        fprintf(stderr, "getattr failed for %s (%s): %s\n", path, chiho_path, strerror(errno));
        return -errno;
    }
    return 0;
}

static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    ensure_chiho_dir();

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    if (strcmp(path, "/") == 0) {
        filler(buf, "starter", NULL, 0);
        filler(buf, "metro", NULL, 0);
        filler(buf, "dragon", NULL, 0);
        filler(buf, "blackrose", NULL, 0);
        filler(buf, "heaven", NULL, 0);
        filler(buf, "skystreet", NULL, 0);
        filler(buf, "7sref", NULL, 0);
        return 0;
    } else if (strcmp(path, "/starter") == 0) {
        DIR *dp = opendir(CHIHO_BASE "starter");
        if (dp == NULL) {
            fprintf(stderr, "readdir failed to open %sstarter: %s\n", CHIHO_BASE, strerror(errno));
            return -errno;
        }
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (de->d_name[0] == '.') continue;
            char *ext = strstr(de->d_name, STARTER_EXT);
            if (ext && ext[strlen(STARTER_EXT)] == '\0') {
                char name[256];
                strncpy(name, de->d_name, ext - de->d_name);
                name[ext - de->d_name] = '\0';
                filler(buf, name, NULL, 0);
            }
        }
        closedir(dp);
        return 0;
    } else if (strcmp(path, "/metro") == 0) {
        DIR *dp = opendir(CHIHO_BASE "metro");
        if (dp == NULL) {
            fprintf(stderr, "readdir failed to open %smetro: %s\n", CHIHO_BASE, strerror(errno));
            return -errno;
        }
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (de->d_name[0] == '.') continue;
            char *ext = strstr(de->d_name, METRO_EXT);
            if (ext && ext[strlen(METRO_EXT)] == '\0') {
                char name[256];
                strncpy(name, de->d_name, ext - de->d_name);
                name[ext - de->d_name] = '\0';
                filler(buf, name, NULL, 0);
            }
        }
        closedir(dp);
        return 0;
    } else if (strcmp(path, "/dragon") == 0) {
        DIR *dp = opendir(CHIHO_BASE "dragon");
        if (dp == NULL) {
            fprintf(stderr, "readdir failed to open %sdragon: %s\n", CHIHO_BASE, strerror(errno));
            return -errno;
        }
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (de->d_name[0] == '.') continue;
            char *ext = strstr(de->d_name, DRAGON_EXT);
            if (ext && ext[strlen(DRAGON_EXT)] == '\0') {
                char name[256];
                strncpy(name, de->d_name, ext - de->d_name);
                name[ext - de->d_name] = '\0';
                filler(buf, name, NULL, 0);
            }
        }
        closedir(dp);
        return 0;
    } else if (strcmp(path, "/blackrose") == 0) {
        DIR *dp = opendir(CHIHO_BASE "blackrose");
        if (dp == NULL) {
            fprintf(stderr, "readdir failed to open %sblackrose: %s\n", CHIHO_BASE, strerror(errno));
            return -errno;
        }
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (de->d_name[0] == '.') continue;
            char *ext = strstr(de->d_name, BLACKROSE_EXT);
            if (ext && ext[strlen(BLACKROSE_EXT)] == '\0') {
                char name[256];
                strncpy(name, de->d_name, ext - de->d_name);
                name[ext - de->d_name] = '\0';
                filler(buf, name, NULL, 0);
            }
        }
        closedir(dp);
        return 0;
    } else if (strcmp(path, "/heaven") == 0) {
        DIR *dp = opendir(CHIHO_BASE "heaven");
        if (dp == NULL) {
            fprintf(stderr, "readdir failed to open %sheaven: %s\n", CHIHO_BASE, strerror(errno));
            return -errno;
        }
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (de->d_name[0] == '.') continue;
            char *ext = strstr(de->d_name, HEAVEN_EXT);
            if (ext && ext[strlen(HEAVEN_EXT)] == '\0') {
                char name[256];
                strncpy(name, de->d_name, ext - de->d_name);
                name[ext - de->d_name] = '\0';
                filler(buf, name, NULL, 0);
            }
        }
        closedir(dp);
        return 0;
    } else if (strcmp(path, "/skystreet") == 0) {
        DIR *dp = opendir(CHIHO_BASE "skystreet");
        if (dp == NULL) {
            fprintf(stderr, "readdir failed to open %sskystreet: %s\n", CHIHO_BASE, strerror(errno));
            return -errno;
        }
        struct dirent *de;
        while ((de = readdir(dp)) != NULL) {
            if (de->d_name[0] == '.') continue;
            char *ext = strstr(de->d_name, SKYSTREET_EXT);
            if (ext && ext[strlen(SKYSTREET_EXT)] == '\0') {
                char name[256];
                strncpy(name, de->d_name, ext - de->d_name);
                name[ext - de->d_name] = '\0';
                filler(buf, name, NULL, 0);
            }
        }
        closedir(dp);
        return 0;
    } else if (strcmp(path, "/7sref") == 0) {
        const char *areas[] = {"starter", "metro", "dragon", "blackrose", "heaven", "skystreet"};
        const char *exts[] = {STARTER_EXT, METRO_EXT, DRAGON_EXT, BLACKROSE_EXT, HEAVEN_EXT, SKYSTREET_EXT};
        for (int i = 0; i < 6; i++) {
            char area_path[256];
            snprintf(area_path, 256, "%s%s", CHIHO_BASE, areas[i]);
            DIR *dp = opendir(area_path);
            if (dp == NULL) {
                fprintf(stderr, "readdir failed to open %s: %s\n", area_path, strerror(errno));
                continue;
            }
            struct dirent *de;
            while ((de = readdir(dp)) != NULL) {
                if (de->d_name[0] == '.') continue;
                char *ext = strstr(de->d_name, exts[i]);
                if (ext && ext[strlen(exts[i])] == '\0') {
                    char name[256];
                    strncpy(name, de->d_name, ext - de->d_name);
                    name[ext - de->d_name] = '\0';
                    char display_name[512];
                    snprintf(display_name, 512, "%s_%s", areas[i], name);
                    filler(buf, display_name, NULL, 0);
                }
            }
            closedir(dp);
        }
        return 0;
    }
    return -ENOENT;
}

static int maimai_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    char chiho_path[256];
    get_chiho_path(path, chiho_path);

    int fd = open(chiho_path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "read failed for %s (%s): %s\n", path, chiho_path, strerror(errno));
        return -errno;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return -errno;
    }
    size_t file_size = st.st_size;

    unsigned char *raw_buf = malloc(file_size + 1);
    if (!raw_buf) {
        close(fd);
        return -ENOMEM;
    }
    int res = pread(fd, raw_buf, file_size, 0);
    if (res <= 0) {
        fprintf(stderr, "read error for %s (%s): %s\n", path, chiho_path, strerror(errno));
        close(fd);
        free(raw_buf);
        return -errno;
    }
    raw_buf[res] = '\0'; 

    // Decrypt or decompress based on area
    if (strstr(path, "/metro") || (strstr(path, "/7sref") && strstr(path, "metro_"))) {
        for (int i = 0; i < res; i++) {
            buf[i] = raw_buf[i] ^ (i % 256); 
        memcpy(buf, buf + offset, size);
        buf[size] = '\0';
    } else if (strstr(path, "/dragon") || (strstr(path, "/7sref") && strstr(path, "dragon_"))) {
        rot13_transform((char *)raw_buf, buf, res);
        memcpy(buf, buf + offset, size);
    } else if (strstr(path, "/heaven") || (strstr(path, "/7sref") && strstr(path, "heaven_"))) {
        if (res <= 16) {
            fprintf(stderr, "read error for %s: file too small to contain IV\n", path);
            close(fd);
            free(raw_buf);
            return -EIO;
        }
        unsigned char key[32];
        memset(key, 0, 32); 
        strncpy((char *)key, "secretkey1234567890", 16); 
        unsigned char iv[16];
        memcpy(iv, raw_buf, 16); 
        unsigned char *ciphertext = raw_buf + 16;
        int ciphertext_len = res - 16;
        unsigned char *plaintext = malloc(ciphertext_len + 1);
        if (!plaintext) {
            close(fd);
            free(raw_buf);
            return -ENOMEM;
        }
        int plaintext_len = aes_decrypt(ciphertext, ciphertext_len, key, iv, plaintext);
        if (plaintext_len < 0) {
            fprintf(stderr, "decryption failed for %s\n", path);
            close(fd);
            free(raw_buf);
            free(plaintext);
            return -EIO;
        }
        plaintext[plaintext_len] = '\0';
        if (offset > plaintext_len) {
            close(fd);
            free(raw_buf);
            free(plaintext);
            return 0;
        }
        size_t read_size = (size > plaintext_len - offset) ? (plaintext_len - offset) : size;
        memcpy(buf, plaintext + offset, read_size);
        res = read_size;
        free(plaintext);
    } else if (strstr(path, "/skystreet") || (strstr(path, "/7sref") && strstr(path, "skystreet_"))) {
        unsigned char *decompressed;
        size_t decompressed_len;
        int ret = gzip_decompress(raw_buf, res, &decompressed, &decompressed_len);
        if (ret != Z_OK || !decompressed) {
            fprintf(stderr, "decompression failed for %s\n", path);
            close(fd);
            free(raw_buf);
            return -EIO;
        }
        if (offset > decompressed_len) {
            close(fd);
            free(raw_buf);
            free(decompressed);
            return 0;
        }
        size_t read_size = (size > decompressed_len - offset) ? (decompressed_len - offset) : size;
        memcpy(buf, decompressed + offset, read_size);
        res = read_size;
        free(decompressed);
    } else {
        memcpy(buf, raw_buf + offset, size); 
        res = size;
    }

    close(fd);
    free(raw_buf);
    return res;
}

static int maimai_write(const char *path, const char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi) {
    char chiho_path[256];
    get_chiho_path(path, chiho_path);

    unsigned char *existing_data = NULL;
    size_t existing_size = 0;
    int fd = open(chiho_path, O_RDONLY);
    if (fd != -1) {
        struct stat st;
        if (fstat(fd, &st) == 0) {
            existing_size = st.st_size;
            existing_data = malloc(existing_size);
            if (existing_data) {
                if (pread(fd, existing_data, existing_size, 0) != (ssize_t)existing_size) {
                    free(existing_data);
                    existing_data = NULL;
                }
            }
        }
        close(fd);
    }

    unsigned char *write_buf = NULL;
    size_t write_size = size;
    if (strstr(path, "/metro") || (strstr(path, "/7sref") && strstr(path, "metro_"))) {
        write_buf = malloc(size + 1);
        if (!write_buf) {
            if (existing_data) free(existing_data);
            return -ENOMEM;
        }
        for (size_t i = 0; i < size; i++) {
            write_buf[i] = buf[i] ^ (i % 256); 
        }
        write_buf[size] = '\0';
    } else if (strstr(path, "/dragon") || (strstr(path, "/7sref") && strstr(path, "dragon_"))) {
        write_buf = malloc(size + 1);
        if (!write_buf) {
            if (existing_data) free(existing_data);
            return -ENOMEM;
        }
        rot13_transform(buf, (char *)write_buf, size);
    } else if (strstr(path, "/heaven") || (strstr(path, "/7sref") && strstr(path, "heaven_"))) {
        unsigned char key[32];
        memset(key, 0, 32); 
        strncpy((char *)key, "secretkey1234567890", 16); 
        unsigned char iv[16];

        if (existing_data && existing_size >= 16) {
            memcpy(iv, existing_data, 16);
        } else {
            if (RAND_bytes(iv, 16) != 1) {
                fprintf(stderr, "RAND_bytes failed for %s\n", path);
                if (existing_data) free(existing_data);
                return -EIO;
            }
        }

        // Encrypt the new data
        int ciphertext_len = size + EVP_MAX_BLOCK_LENGTH; 
        write_buf = malloc(16 + ciphertext_len + 1); // 
        if (!write_buf) {
            if (existing_data) free(existing_data);
            return -ENOMEM;
        }
        memcpy(write_buf, iv, 16); 
        ciphertext_len = aes_encrypt((unsigned char *)buf, size, key, iv, write_buf + 16);
        if (ciphertext_len < 0) {
            fprintf(stderr, "encryption failed for %s\n", path);
            free(write_buf);
            if (existing_data) free(existing_data);
            return -EIO;
        }
        write_size = 16 + ciphertext_len; 
        write_buf[write_size] = '\0';
    } else if (strstr(path, "/skystreet") || (strstr(path, "/7sref") && strstr(path, "skystreet_"))) {
        int ret = gzip_compress((unsigned char *)buf, size, &write_buf, &write_size);
        if (ret != Z_OK || !write_buf) {
            fprintf(stderr, "compression failed for %s\n", path);
            if (existing_data) free(existing_data);
            return -EIO;
        }
    } else {
        write_buf = malloc(size + 1);
        if (!write_buf) {
            if (existing_data) free(existing_data);
            return -ENOMEM;
        }
        memcpy(write_buf, buf, size); // No transformation for blackrose or starter
        write_buf[size] = '\0';
    }

    fd = open(chiho_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        fprintf(stderr, "write failed to open %s: %s\n", chiho_path, strerror(errno));
        free(write_buf);
        if (existing_data) free(existing_data);
        return -errno;
    }

    int res = pwrite(fd, write_buf, write_size, 0); // Ignore offset, write from start
    close(fd);
    free(write_buf);
    if (existing_data) free(existing_data);
    if (res == -1) {
        fprintf(stderr, "write error for %s: %s\n", chiho_path, strerror(errno));
        return -errno;
    }
    return size; // Return the number of bytes requested to be written
}

static int maimai_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char chiho_path[256];
    get_chiho_path(path, chiho_path);

    int fd = open(chiho_path, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (fd == -1) {
        fprintf(stderr, "create failed for %s (%s): %s\n", path, chiho_path, strerror(errno));
        return -errno;
    }
    close(fd);
    return 0;
}

static int maimai_unlink(const char *path) {
    char chiho_path[256];
    get_chiho_path(path, chiho_path);

    int res = unlink(chiho_path);
    if (res == -1) {
        fprintf(stderr, "unlink failed for %s (%s): %s\n", path, chiho_path, strerror(errno));
        return -errno;
    }
    return 0;
}

static struct fuse_operations maimai_oper = {
    .getattr = maimai_getattr,
    .readdir = maimai_readdir,
    .read = maimai_read,
    .write = maimai_write,
    .create = maimai_create,
    .unlink = maimai_unlink,
};

int main(int argc, char *argv[]) {
    ensure_chiho_dir();
    OpenSSL_add_all_algorithms(); // Initialize OpenSSL
    return fuse_main(argc, argv, &maimai_oper, NULL);
}