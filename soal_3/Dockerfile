FROM gcc:12.2.0

# Install FUSE dan library
RUN apt-get update && apt-get install -y \
    libfuse3-dev fuse3 \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY antink.c /app/

# Compile antink.c menjadi binary /usr/local/bin/antink
RUN gcc antink.c -o /usr/local/bin/antink -lfuse3

# Buat mount point dan log dir
RUN mkdir -p /antink_mount /var/log

# Jalankan antink FUSE
ENTRYPOINT ["antink"]
