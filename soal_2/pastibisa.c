#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <openssl/evp.h>

// Fungsi untuk mencatat log
void write_log(const char *status, const char *tag, const char *info) {
    FILE *log_file = fopen("logs-fuse.log", "a");
    if (log_file == NULL) return;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timestamp[100];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y-%H:%M:%S", tm);

    fprintf(log_file, "[%s]::%s::[%s]::[%s]\n", status, timestamp, tag, info);
    fclose(log_file);
}

// Fungsi untuk decode base64
char* decode_base64(const char* input) {
    // Placeholder untuk hasil decoding base64
    return strdup(input);
}

// Fungsi untuk decode rot13
char* decode_rot13(const char* input) {
    char *output = strdup(input);
    for (int i = 0; output[i]; i++) {
        if ((output[i] >= 'A' && output[i] <= 'Z') || (output[i] >= 'a' && output[i] <= 'z')) {
            if ((output[i] >= 'A' && output[i] <= 'M') || (output[i] >= 'a' && output[i] <= 'm')) {
                output[i] += 13;
            } else {
                output[i] -= 13;
            }
        }
    }
    return output;
}

// Fungsi untuk decode hex
char* decode_hex(const char* input) {
    size_t len = strlen(input);
    char *output = (char*)malloc((len/2) + 1);
    for (size_t i = 0; i < len; i += 2) {
        sscanf(input + i, "%2hhx", &output[i/2]);
    }
    output[len/2] = '\0';
    return output;
}

// Fungsi untuk reverse string
char* decode_rev(const char* input) {
    size_t len = strlen(input);
    char *output = (char*)malloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        output[i] = input[len - 1 - i];
    }
    output[len] = '\0';
    return output;
}

// Fungsi untuk membaca file
static int fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), ".%s", path);

    int res = 0;
    int fd = open(full_path, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

// Fungsi untuk menampilkan isi direktori
static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    DIR *dp;
    struct dirent *de;

    dp = opendir(".");
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, 0))
            break;
    }

    closedir(dp);
    return 0;
}

// Fungsi untuk menampilkan informasi file
static int fuse_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    int res;
    res = lstat(path, stbuf);
    if (res == -1)
        return -errno;
    return 0;
}

// Fungsi untuk membaca file dan mendecode jika memiliki prefix tertentu
static int fuse_open(const char *path, struct fuse_file_info *fi) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), ".%s", path);

    int fd = open(full_path, fi->flags);
    if (fd == -1)
        return -errno;

    close(fd);
    return 0;
}

// Fungsi untuk membaca dan memproses file berdasarkan prefix
static int fuse_read_file(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char *content = NULL;
    size_t content_len = 0;
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), ".%s", path);

    FILE *f = fopen(full_path, "r");
    if (!f) return -errno;

    fseek(f, 0, SEEK_END);
    content_len = ftell(f);
    fseek(f, 0, SEEK_SET);

    content = malloc(content_len + 1);
    fread(content, 1, content_len, f);
    fclose(f);
    content[content_len] = '\0';

    char *decoded_content = NULL;

    if (strncmp(path + 1, "base64_", 7) == 0) {
        decoded_content = decode_base64(content);
    } else if (strncmp(path + 1, "rot13_", 6) == 0) {
        decoded_content = decode_rot13(content);
    } else if (strncmp(path + 1, "hex_", 4) == 0) {
        decoded_content = decode_hex(content);
    } else if (strncmp(path + 1, "rev_", 4) == 0) {
        decoded_content = decode_rev(content);
    } else {
        decoded_content = strdup(content);
    }

    free(content);

    if (offset < strlen(decoded_content)) {
        if (offset + size > strlen(decoded_content))
            size = strlen(decoded_content) - offset;
        memcpy(buf, decoded_content + offset, size);
    } else {
        size = 0;
    }

    free(decoded_content);

    write_log("SUCCESS", "readFile", full_path);
    return size;
}

// Fungsi untuk membuka direktori dan memeriksa izin akses untuk folder rahasia
static int fuse_opendir(const char *path, struct fuse_file_info *fi) {
    if (strncmp(path, "/rahasia", 8) == 0) {
        char password[256];
        printf("Masukkan password untuk mengakses folder rahasia: ");
        scanf("%255s", password);
        // Password yang diterima, implementasi sederhana, tambahkan logika verifikasi jika perlu
        if (strcmp(password, "password_bebas") != 0) {
            write_log("FAILED", "openDir", "Unauthorized access attempt to /rahasia");
            return -EACCES;
        }
    }
    return 0;
}

// Definisi fungsi-fungsi FUSE
static struct fuse_operations fuse_example_operations = {
    .getattr = fuse_getattr,
    .readdir = fuse_readdir,
    .open = fuse_open,
    .read = fuse_read_file,
    .opendir = fuse_opendir,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &fuse_example_operations, NULL);
}
