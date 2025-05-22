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
```
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
```
if (is_danger(de->d_name)) {
    log_msg("[ALERT] Anomaly detected %s in file: %s", de->d_name, dirp);
    char rev[256];
    reverse_name(de->d_name, rev);
    log_msg("[REVERSE] File %s has been reversed : %s", de->d_name, rev);
    filler(buf, rev, NULL, 0, 0);
}
```
- Helper:
  
```
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

```
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

```
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
