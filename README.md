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
- 
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
