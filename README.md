# LAPRES Praktikum Sistem Operasi Modul 4 - IT14

## Anggota
1. Muhammad Fatihul Qolbi Ash Shiddiqi (5027241023)
2. Mutiara Diva Jaladitha (5027241083)
3. M. Faqih Ridho (5027241123)

## DAFTAR ISI
- [Soal 1](#soal-1)
- [Soal 2](#soal-2)
- [Soal 3](#soal-3)
- [Soal 4](#soal-4)

# soal-1
**Dikerjakan oleh Muhammad Fatihul Qolbi Ash Shiddiqi (5027241023)**

## Deskripsi Soal 

The Shorekeeper adalah entitas misterius yang memimpin dan menjaga Black Shores untuk mencegah kekacauan atau krisis, terutama setelah kemunculan Fallacy of No Return. Saat menjelajahi Tethys' Deep, Shorekeeper menemukan anomali berupa teks acak yang tampaknya tidak memiliki arti. Setelah analisis, ia menyadari bahwa teks tersebut adalah string hexadecimal yang dapat dikonversi menjadi file gambar. Untuk membantu Shorekeeper, dibuatkan sistem berbasis FUSE (Filesystem in Userspace) yang melakukan tugas berikut:

A. Mengambil sampel anomali teks: Mengunduh file zip dari link tertentu, mengekstraknya, dan menghapus file zip setelahnya.
B. Konversi teks hexadecimal ke gambar: Mengubah file teks berisi string hexadecimal menjadi file gambar dan menyimpannya di direktori `anomali/image` dengan format penamaan `[nama_file]_image_[YYYY-mm-dd]_[HH:MM:SS].png.`
C. Logging konversi: Mencatat setiap konversi ke file `anomali/conversion.log` dengan format `[YYYY-mm-dd][HH:MM:SS]`: Successfully converted hexadecimal text `[nama_file_teks]` to `[nama_file_gambar]`.
D. Mount direktori menggunakan FUSE: Memungkinkan akses ke direktori anomali melalui mount point yang ditentukan, dengan pengaturan izin dan kepemilikan sesuai pengguna asli.

#### A. Membuat System.c dan Hunter.c dengan catatan hunter.c bisa dijalankan ketika sistem sudah dijalankan.

#### hexed.c
```C
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
```
- `const char *download_cmd = ...` → Mendefinisikan perintah untuk mengunduh file zip dari URL yang diberikan menggunakan wget dengan opsi --no-check-certificate untuk mengabaikan validasi SSL.
- `int ret = system(download_cmd);` → Menjalankan perintah unduh melalui fungsi system. Jika gagal (kode kembali bukan 0), mencetak pesan error menggunakan strerror(errno).
- `ret = system("unzip -burgo -o anomaly.zip -d .");` → Mengekstrak file zip ke direktori saat ini dengan opsi -o untuk menimpa file yang sudah ada.
- `ret = system("rm anomaly.zip");` → Menghapus file zip setelah ekstraksi untuk memenuhi kebutuhan soal.

#### Output 

#### hexed.c (Download Zip , Ekstrak zip, dan hapus zip)
![Screenshot 2025-05-22 204715](https://github.com/user-attachments/assets/c10b1891-54b5-4bf2-b38c-63f33cadb223)

#### B. Convert Hexadecimal ke png 

#### hexed.c

```C
// Cek apakah data adalah hex valid
if (!is_valid_hex(hex_buffer, file_size)) {
    fprintf(stderr, "File %s berisi data non-hex, dilewati\n", input_path);
    free(hex_buffer);
    continue;

// Konversi hex ke biner
size_t hex_len = strlen(hex_buffer);
size_t bin_len = hex_len / 2;
unsigned char *bin_buffer = malloc(bin_len);
for (size_t i = 0; i < bin_len; i++) {
    unsigned int byte;
    sscanf(&hex_buffer[i * 2], "%2x", &byte);
    bin_buffer[i] = (unsigned char)byte;
}

 // Tulis biner ke file gambar
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

        // Ubah izin dan kepemilikan file gambar
        if (chown(output_path, user_id, group_id) != 0) {
            fprintf(stderr, "Gagal mengubah kepemilikan %s: %s\n", output_path, strerror(errno));
        }
        if (chmod(output_path, 0644) != 0) {
            fprintf(stderr, "Gagal mengubah izin %s: %s\n", output_path, strerror(errno));
        }
```

- `if (!is_valid_hex(hex_buffer, file_size))` → Memvalidasi bahwa isi file teks hanya berisi karakter hexadecimal yang valid menggunakan fungsi isxdigit.
- `unsigned char *bin_buffer = malloc(bin_len);` → Mengalokasikan memori untuk data biner, di mana ukuran biner adalah setengah dari panjang string hex (2 karakter hex = 1 byte).
- `sscanf(&hex_buffer[i * 2], "%2x", &byte);` → Mengonversi setiap pasangan karakter hex menjadi byte biner menggunakan sscanf.
- `fwrite(bin_buffer, 1, bin_len, output_file);` → Menulis data biner ke file gambar dalam mode tulis biner ("wb").

#### Output 

#### hexed.c (Convert hexadecimal to png) 
![Screenshot 2025-05-22 205458](https://github.com/user-attachments/assets/d647c7ef-9ec2-45fc-900f-916efeb10300)


#### Gambar Anomali 
![Screenshot 2025-05-22 205611](https://github.com/user-attachments/assets/ac2ec132-1e11-4c76-a1fd-354492385d6e)

#### C. Penamaan File

```C
// Ambil waktu saat ini untuk penamaan
        time_t rawtime;
        struct tm *timeinfo;
        char timestamp[20];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H:%M:%S", timeinfo);

        // Buat output_name
        char output_name[MAX_PATH];
        len = snprintf(output_name, sizeof(output_name), "%s_image_%s.png", base_name, timestamp);
        if (len < 0 || len >= (int)sizeof(output_name)) {
            fprintf(stderr, "Nama file %s terlalu panjang, dipotong\n", output_name);
            output_name[sizeof(output_name) - 1] = '\0';
        }
```

- `time(&rawtime);` → Mengambil waktu sistem saat ini dalam format detik sejak epoch dan menyimpannya dalam variabel rawtime.
- `timeinfo = localtime(&rawtime);` → Mengonversi waktu epoch ke waktu lokal dalam struktur tm untuk digunakan dalam format penamaan dan logging.
- `strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H:%M:%S", timeinfo);` → Memformat waktu lokal ke string dengan format YYYY-mm-dd_HH:MM:SS untuk digunakan dalam nama file gambar.
- `snprintf(output_name, sizeof(output_name), "%s_image_%s.png", base_name, timestamp);` → Membuat nama file gambar dengan format [nama_file]_image_[YYYY-mm-dd]_[HH:MM:SS].png, menggunakan base_name (nama file tanpa .txt) dan timestamp.
- `if (len < 0 || len >= (int)sizeof(output_name))` → Memeriksa apakah nama file yang dihasilkan terlalu panjang; jika ya, memotong string untuk mencegah buffer overflow.

#### Output 
![image](https://github.com/user-attachments/assets/ce818e06-e1d7-4fec-85bf-74dfad853fc4)

#### D. Log File

```C
// Tulis ke log file
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
```
- `strftime(log_entry, sizeof(log_entry), "[%Y-%m-%d][%H:%M:%S]", timeinfo);` → Memformat waktu untuk entri log sesuai format [YYYY-mm-dd][HH:MM:SS].
- `snprintf(log_entry + strlen(log_entry), ...);` → Menambahkan pesan log dengan format : Successfully converted hexadecimal text [nama_file_teks] to [nama_file_gambar] ke buffer log_entry.
- `fprintf(log_file, "%s", log_entry);` → Menulis entri log ke file conversion.log

#### Output

![image](https://github.com/user-attachments/assets/d16d25c9-4841-42e8-bd2c-921d65ff35c9)



#### Struktur Akhir
![image](https://github.com/user-attachments/assets/5293e848-0c38-4cc5-81f9-de4b938488e7)

# soal-3
- Dockerfile

Menyusun image yang berisi:

- Base image GCC (untuk kompilasi C)

- Instalasi FUSE (libfuse3-dev, fuse3)

- Copy antink.c ke dalam /app dan kompilasi menjadi binary /usr/local/bin/antink

- Pembuatan direktori mount point dan log di dalam container

- ENTRYPOINT menjalankan binary FUSE

```
FROM gcc:12.2.0
RUN apt-get update && apt-get install -y libfuse3-dev fuse3 && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY antink.c /app/
RUN gcc antink.c -o /usr/local/bin/antink -lfuse3
RUN mkdir -p /antink_mount /var/log
ENTRYPOINT ["/usr/local/bin/antink"]
```

- docker-compose.yml
Mengatur dua service:

1. antink-server

- Build dari folder server/

- Privileged + cap_add SYS_ADMIN + device /dev/fuse

- Bind-mount:

   - ./it24_host:/it24_host:ro (soal asli)

   - antink_mount:/antink_mount (mount point FUSE)

   - ./var/log:/var/log (bind-mount log)

2. antink-logger

- Image BusyBox

- Command tail -F /var/log/it24.log untuk monitoring real-time

- Bind-mount ./var/log:/var/log

```
version: '3.8'
services:
  antink-server:
    build: ./server
    privileged: true
    cap_add:
      - SYS_ADMIN
    devices:
      - /dev/fuse:/dev/fuse
    volumes:
      - ./it24_host:/it24_host:ro
      - antink_mount:/antink_mount
      - ./var/log:/var/log
  antink-logger:
    image: busybox:stable
    command: sh -c "tail -F /var/log/it24.log"
    volumes:
      - ./var/log:/var/log
volumes:
  antink_mount:
``` 

### Soal 3 A
“Menyiapkan sistem AntiNK menggunakan Docker + FUSE dalam container terisolasi…”

- Fungsi utama: main() memanggil fuse_main() dengan mount point /antink_mount

- Entry point:
```C
int main(int argc, char *argv[]) {
    char *fuse_argv[] = { "antink", "/antink_mount", "-f", NULL };
    return fuse_main(3, fuse_argv, &antink_ops, NULL);
}
Konfigurasi Compose:

privileged: true, cap_add: SYS_ADMIN, --device /dev/fuse

Bind-mount it24_host (folder soal) agar container bisa baca file asli.
```

### output
![gambar 3a](assets/soal%203%20a.png)

### Soal 3 B
“Mendeteksi file dengan kata kunci ‘nafis’/‘kimcun’ dan membalikkan nama file saat ditampilkan.”

- Callback: antink_readdir()

Deteksi:
```C
if (is_danger(de->d_name)) {
    log_msg("[ALERT] Anomaly detected %s in file: %s", de->d_name, dirp);
    char rev[256];
    reverse_name(de->d_name, rev);
    log_msg("[REVERSE] File %s has been reversed : %s", de->d_name, rev);
    filler(buf, rev, NULL, 0, 0);
}
```
- Helper:
  
```C
static int is_danger(const char *name) { … }
static void reverse_name(const char *orig, char *out) {
    strcpy(out, orig);
    str_reverse(out);
}
```
### output

![gambar 3b](assets/soal%203%20b%20revisi%202.png)

### Soal 3 C
“Enkripsi isi file teks normal dengan ROT13; file berbahaya tidak dienkripsi.”

- Callback: antink_read()

- Logika ROT13:

```C
if (strstr(p, ".txt") && !is_danger(p)) {
    // loop tiap karakter
    buf[i] = encrypt_rot13(data[i]);
    log_msg("[ENCRYPT] File %s has been encrypted", p, NULL);
} else {
    memcpy(buf, data, res);
    log_msg("[READ] File %s no encryption", p, NULL);
}
```
### output

![gambar 3c](assets/soal%203%20c.png)

### Soal 3 D
“Semua aktivitas dicatat ke dalam /var/log/it24.log, dimonitor real-time oleh logger.”

- Fungsi: log_msg()

- Format dengan timestamp:

```C
static void log_msg(const char *fmt, const char *arg1, const char *arg2) {
    time_t now = time(NULL);
    localtime_r(&now, &tm);
    strftime(timestr, sizeof(timestr), "[%Y-%m-%d %H:%M:%S]", &tm);
    fprintf(f, "%s ", timestr);
    fprintf(f, fmt, arg1, arg2);
    fprintf(f, "\n");
    …
}
```
- Logger Service: tail -F /var/log/it24.log di container antink-logger.

### output
![gambar 3d](assets/soal%203%20d%20revisi.png)

### Soal 3 E
“Perubahan file hanya terjadi di dalam container; direktori host tidak terpengaruh.”

- it24_host tetap berisi file asli:
kimcun.jpg  nafis.jpg  normal.txt

- antink_mount menunjukkan hasil virtual FS (nama terbalik & ROT13)

- Bukti:

```
ls it24_host/         # file asli
ls /antink_mount      # file dimanipulasi
head it24_host/normal.txt  # teks asli
```
### output
![gambar 3e](assets/soal%203%20e.png)


# soal-4
**Dikerjakan oleh Muhammad Fatihul Qolbi Ash Shiddiqi (5027241023)**

## Deskripsi Soal 

Sebagai administrator sistem in-game untuk game maimai di SEGA, Anda bertugas mengelola mekanisme progres chiho yang merupakan bagian dari area dalam universe maimai. Terdapat 7 area (Starter, Metropolis, Dragon, Black Rose, Heaven, Skystreet, 7sRef), masing-masing dengan aturan penyimpanan file yang berbeda. Sistem ini menggunakan FUSE (Filesystem in Userspace) untuk mengelola direktori chiho dan memetakannya ke fuse_dir dengan operasi dasar seperti read, write, create, unlink, getattr, dan readdir. 

Detail Area

A. Starter Chiho: File disimpan dengan tambahan ekstensi .mai di direktori asli (chiho/starter), tetapi tanpa ekstensi di fuse_dir/starter.
B. Metropolis Chiho: File disimpan dengan ekstensi .ccc di direktori asli, dengan nama file dienkripsi menggunakan pergeseran karakter berdasarkan posisi (misal, E tetap, n menjadi o (+1), e menjadi g (+2), dst.).
C. Dragon Chiho: File disimpan dengan ekstensi .rot di direktori asli, dengan isi dienkripsi menggunakan ROT13.
D. Black Rose Chiho: File disimpan dengan ekstensi .bin di direktori asli, dalam format biner murni tanpa enkripsi atau encoding tambahan.
E. Heaven Chiho: File disimpan dengan ekstensi .enc di direktori asli, dienkripsi menggunakan AES-256-CBC dengan IV yang disimpan di awal file.
F. Skystreet Chiho: File disimpan dengan ekstensi .gz di direktori asli, dikompresi menggunakan gzip.
G. 7sRef Chiho: Area spesial yang memetakan file dari semua area lain dengan format penamaan [area]_[nama_file] (misal, starter_guide.txt di fuse_dir/7sref memetakan ke fuse_dir/starter/guide.txt).
 
#### A. Starter Chiho

#### 
```C
void get_chiho_path(const char *fuse_path, char *chiho_path) {
    snprintf(chiho_path, 256, "%s", CHIHO_BASE);
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
        }
    }
}
```
- `snprintf(chiho_path, 256, "%s", CHIHO_BASE);` → Menginisialisasi path tujuan dengan direktori dasar chiho/ untuk memulai pembentukan path asli.
- `area = strstr(fuse_path, "/starter");` → Memeriksa apakah path yang diminta berada di direktori starter pada fuse_dir.
- `strcat(chiho_path, "starter/");` → Menambahkan subdirektori starter/ ke path asli untuk memetakan ke chiho/starter.
- `strncat(chiho_path, filename, 256 - strlen(chiho_path) - 1);` → Menambahkan nama file dari path FUSE ke path asli, memastikan tidak melebihi batas buffer.
- `strncat(chiho_path, STARTER_EXT, 256 - strlen(chiho_path) - 1);` → Menambahkan ekstensi .mai ke nama file di direktori asli sesuai aturan Starter Chiho.
  
#### Output 
![image](https://github.com/user-attachments/assets/cf51e323-eb1c-41a0-8005-a71ea2cff3ea)

#### B. Metropolis Chiho

#### Metroplis Chiho

```C
void get_chiho_path(const char *fuse_path, char *chiho_path) {
    snprintf(chiho_path, 256, "%s", CHIHO_BASE);
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
        if (strcmp(target_area, "metro") == 0) {
            snprintf(chiho_path, 256, "%smetro/%s%s", CHIHO_BASE, actual_filename, METRO_EXT);
        }
    } else {
        area = strstr(fuse_path, "/metro");
        if (area) {
            area += 6;
            strcat(chiho_path, "metro/");
            if (*area == '\0' || strcmp(area, "/") == 0) {
                chiho_path[strlen(chiho_path) - 1] = '\0';
                return;
            }
            const char *filename = area + 1;
            strncat(chiho_path, filename, 256 - strlen(chiho_path) - 1);
            strncat(chiho_path, METRO_EXT, 256 - strlen(chiho_path) - 1);
        }
    }
}
```

- `snprintf(chiho_path, 256, "%s", CHIHO_BASE);` → Menginisialisasi path tujuan dengan direktori dasar chiho/ untuk memulai pembentukan path asli.
- `area = strstr(fuse_path, "/metro");` → Memeriksa apakah path yang diminta berada di direktori metro pada fuse_dir.
- `strcat(chiho_path, "metro/");` → Menambahkan subdirektori metro/ ke path asli untuk memetakan ke chiho/metro.
- `strncat(chiho_path, filename, 256 - strlen(chiho_path) - 1);` → Menambahkan nama file dari path FUSE ke path asli, memastikan tidak melebihi batas buffer.
- `strncat(chiho_path, METRO_EXT, 256 - strlen(chiho_path) - 1);` → Menambahkan ekstensi .ccc ke nama file di direktori asli sesuai aturan Metropolis Chiho.

#### Output 
![image](https://github.com/user-attachments/assets/c85cb01f-0f7a-45b4-a3cf-e3eccfe5eb10)

#### C. Dragon Chiho

```C
void rot13_transform(const char *input, char *output, size_t len) {
    for (size_t i = 0; i < len && input[i] != '\0'; i++) {
        if ((input[i] >= 'A' && input[i] <= 'Z')) {
            output[i] = 'A' + ((input[i] - 'A' + 13) % 26);
        } else if ((input[i] >= 'a' && input[i] <= 'z')) {
            output[i] = 'a' + ((input[i] - 'a' + 13) % 26);
        } else {
            output[i] = input[i];
        }
    }
    output[len] = '\0';
}
```

- `for (size_t i = 0; i < len && input[i] != '\0'; i++)` → Mengiterasi setiap karakter dalam string masukan hingga panjang yang ditentukan atau hingga karakter null.
- `if ((input[i] >= 'A' && input[i] <= 'Z'))` → Memeriksa apakah karakter adalah huruf besar untuk diterapkan transformasi ROT13.
- `output[i] = 'A' + ((input[i] - 'A' + 13) % 26);` → Menggeser huruf besar sebanyak 13 posisi dalam alfabet (modulo 26 untuk siklus).
- `output[i] = 'a' + ((input[i] - 'a' + 13) % 26);` → Menggeser huruf kecil sebanyak 13 posisi dalam alfabet, serupa dengan huruf besar.
- `output[i] = input[i];` → Menyalin karakter non-huruf tanpa perubahan untuk mempertahankan format asli.

#### Output 
![image](https://github.com/user-attachments/assets/bbcd07af-4d45-4f93-a123-7b1f8a185b1c)

#### D. Black Rose Chiho

```C
void get_chiho_path(const char *fuse_path, char *chiho_path) {
    snprintf(chiho_path, 256, "%s", CHIHO_BASE);
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
        if (strcmp(target_area, "blackrose") == 0) {
            snprintf(chiho_path, 256, "%sblackrose/%s%s", CHIHO_BASE, actual_filename, BLACKROSE_EXT);
        }
    } else {
        area = strstr(fuse_path, "/blackrose");
        if (area) {
            area += 10;
            strcat(chiho_path, "blackrose/");
            if (*area == '\0' || strcmp(area, "/") == 0) {
                chiho_path[strlen(chiho_path) - 1] = '\0';
                return;
            }
            const char *filename = area + 1;
            strncat(chiho_path, filename, 256 - strlen(chiho_path) - 1);
            strncat(chiho_path, BLACKROSE_EXT, 256 - strlen(chiho_path) - 1);
        }
    }
}
```
- `snprintf(chiho_path, 256, "%s", CHIHO_BASE);` → Menginisialisasi path tujuan dengan direktori dasar chiho/ untuk memulai pembentukan path asli.
- `area = strstr(fuse_path, "/blackrose");` → Memeriksa apakah path yang diminta berada di direktori blackrose pada fuse_dir.
- `strcat(chiho_path, "blackrose/");` → Menambahkan subdirektori blackrose/ ke path asli untuk memetakan ke chiho/blackrose.
- `strncat(chiho_path, filename, 256 - strlen(chiho_path) - 1);` → Menambahkan nama file dari path FUSE ke path asli, memastikan tidak melebihi batas buffer.
- `strncat(chiho_path, BLACKROSE_EXT, 256 - strlen(chiho_path) - 1);` → Menambahkan ekstensi .bin ke nama file di direktori asli sesuai aturan Black Rose Chiho.

#### Output

![image](https://github.com/user-attachments/assets/81f161d8-3e37-4264-9efb-e2d7afadcfe8)

#### E. Heaven Chiho

```C
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
```

- `EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();` → Membuat konteks enkripsi baru untuk operasi AES-256-CBC menggunakan OpenSSL.
- `EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);` → Menginisialisasi enkripsi dengan algoritma AES-256-CBC, menggunakan kunci dan IV yang diberikan.
- `EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len);` → Mengenkripsi data plaintext ke ciphertext, menyimpan panjang data yang dienkripsi ke len.
- `EVP_EncryptFinal_ex(ctx, ciphertext + len, &len);` → Menyelesaikan proses enkripsi, menangani padding dan menambahkan sisa data ke ciphertext.
- `EVP_CIPHER_CTX_free(ctx);` → Membebaskan konteks enkripsi untuk mencegah kebocoran memori.

#### Output 
![image](https://github.com/user-attachments/assets/b1a1d24d-39a8-483c-87ac-8927ec0d2b9e)

#### F. Skystreet Chiho
```C
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
```

- `deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);` → Menginisialisasi kompresi gzip menggunakan zlib dengan pengaturan default dan window bits untuk format gzip.
- `strm.next_in = (unsigned char *)input; strm.avail_in = input_len;` → Mengatur data masukan untuk kompresi, dengan panjang data yang diberikan.
- `*output = realloc(*output, *output_len + have);` → Mengalokasikan ulang memori untuk menyimpan data terkompresi, menambahkan data baru dari setiap iterasi.
- `memcpy(*output + *output_len, out, have);` → Menyalin data terkompresi dari buffer sementara ke buffer keluaran akhir.
- `deflateEnd(&strm);` → Membersihkan struktur kompresi untuk mencegah kebocoran memori setelah selesai.

#### Output 

![image](https://github.com/user-attachments/assets/a3fe5f2b-0af8-4f0b-b342-f194f5cbcce4)

#### G. 7sRef Chiho
```C
static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    ensure_chiho_dir();

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    if (strcmp(path, "/7sref") == 0) {
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
}
```

- `filler(buf, ".", NULL, 0); filler(buf, "..", NULL, 0);` → Menambahkan entri direktori standar . dan .. ke daftar direktori yang ditampilkan.
- `if (strcmp(path, "/7sref") == 0)` → Memeriksa apakah path yang diminta adalah direktori 7sref untuk menangani logika khususnya.
- `snprintf(area_path, 256, "%s%s", CHIHO_BASE, areas[i]);` → Membuat path ke direktori asli untuk setiap area (misal, chiho/starter) untuk membaca file.
- `strncpy(name, de->d_name, ext - de->d_name);` → Menghapus ekstensi spesifik area dari nama file untuk mendapatkan nama dasar.
- `snprintf(display_name, 512, "%s_%s", areas[i], name);` → Membuat nama file dalam format [area]_[nama_file] untuk ditampilkan di fuse_dir/7sref..


#### Output 
![image](https://github.com/user-attachments/assets/cf316acc-eb0f-4438-954d-84077537867a)


> Note:
> - Program menggunakan FUSE untuk memetakan direktori chiho ke fuse_dir dengan transformasi sesuai aturan masing-masing chiho.
> - Fungsi get_chiho_path adalah inti dari pemetaan path antara fuse_dir dan chiho, menangani ekstensi dan format nama file.
> - Enkripsi AES-256-CBC untuk Heaven Chiho menggunakan IV yang disimpan di awal file, dengan opsi untuk mempertahankan IV lama atau membuat yang baru.
> - Error handling dilakukan dengan mencetak pesan error menggunakan strerror(errno) untuk debugging.
