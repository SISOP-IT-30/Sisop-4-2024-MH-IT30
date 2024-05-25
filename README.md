# Sisop-3-2024-MH-IT30
Praktikum Sistem Operasi Modul 3

## Praktikum [Modul 3](https://github.com/lab-kcks/Modul-Sisop/tree/main/Modul-3)

Mata Kuliah Sistem Operasi

Dosen pengampu : Ir. Muchammad Husni, M.Kom.


## Kelompok Praktikum [IT-30]

- [Fico Simhanandi - 50272310](https://github.com/PuroFuro)
- [Jody Hezekiah - 5027221050](https://github.com/imnotjs)
- [Nabiel Nizar Anwari - 5027231087](https://github.com/bielnzar)

## Soal 3
3) Seorang arkeolog menemukan sebuah gua yang didalamnya tersimpan banyak relik dari zaman praaksara, sayangnya semua barang yang ada pada gua tersebut memiliki bentuk yang terpecah belah akibat bencana yang tidak diketahui. Sang arkeolog ingin menemukan cara cepat agar ia bisa menggabungkan relik-relik yang terpecah itu, namun karena setiap pecahan relik itu masih memiliki nilai tersendiri, ia memutuskan untuk membuat sebuah file system yang mana saat ia mengakses file system tersebut ia dapat melihat semua relik dalam keadaan utuh, sementara relik yang asli tidak berubah sama sekali.
Ketentuan :

a) Buatlah sebuah direktori dengan ketentuan seperti pada tree berikut
```.
├── [nama_bebas]
├── relics
│   ├── relic_1.png.000
│   ├── relic_1.png.001
│   ├── dst dst…
│   └── relic_9.png.010
└── report
```

Berikut ada gambar tree yang telah disesuaikan dengan yang diinginkan modul:

(gambar)

b) Direktori [nama_bebas] adalah direktori FUSE dengan direktori asalnya adalah direktori relics. Ketentuan Direktori [nama_bebas] adalah sebagai berikut :
Ketika dilakukan listing, isi dari direktori [nama_bebas] adalah semua relic dari relics yang telah tergabung.

- Ketika dilakukan copy (dari direktori [nama_bebas] ke tujuan manapun), file yang disalin adalah file dari direktori relics yang sudah tergabung.

- Ketika ada file dibuat, maka pada direktori asal (direktori relics) file tersebut akan dipecah menjadi sejumlah pecahan dengan ukuran maksimum tiap pecahan adalah 10kb.

- File yang dipecah akan memiliki nama [namafile].000 dan seterusnya sesuai dengan jumlah pecahannya.
- Ketika dilakukan penghapusan, maka semua pecahannya juga ikut terhapus.

c) Direktori report adalah direktori yang akan dibagikan menggunakan Samba File Server. Setelah kalian berhasil membuat direktori [nama_bebas], jalankan FUSE dan salin semua isi direktori [nama_bebas] pada direktori report.


d) Catatan:
- pada contoh terdapat 20 relic, namun pada zip file hanya akan ada 10 relic
- [nama_bebas] berarti namanya dibebaskan
- pada soal 3c, cukup salin secara manual. File Server hanya untuk membuktikan bahwa semua file pada direktori [nama_bebas] dapat dibuka dengan baik.

e) [discoveries.zip](https://drive.google.com/file/d/1BJkaBvGaxqiwPWvXRdYNXzxxmIYQ8FKf/view)

### Jawaban secara kesuluruhan untuk soal nomor 3

Dikarenakan code berjalan secara berkesinambungan dan saling memakai fungsi satu sama lain, maka kode akan dijelaskan disini secara terpisah-pisah setiap fungsi

**Code secara keseluruhan**

```c
#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/stat.h>

static const char *base_dir = "/home/purofuro/Fico/M4S3/relics";
#define MAX_BYTES 1024
#define MAX_SIZE 10240
#define MAX_SUFFIX_LEN 5  // Length for ".%03d" + null terminator
#ifndef S_IFREG
#define S_IFREG 0100000
#endif
#ifndef DT_REG
#define DT_REG 8
#endif
#ifndef S_IFDIR
#define S_IFDIR 0040000
#endif

static void construct_full_path(char *destination, const char *relative_path) {
    strcpy(destination, base_dir);
    strncat(destination, relative_path, MAX_BYTES - strlen(destination) - 1);
    printf("The string is %s\n", destination);
}
```
**Fungsi 1 (construct_full_path)**

Untuk bagian awal terdapat include-include yang diperlukan untuk berjalannya code, lalu terdapat definition yang sesuai dengan namanya, kegunaannya berupa ukuran, path, dan definisi untuk beberapa bagian khusus.

`construct_full_path` disini berguna untuk mengcopy base_dir kedalam address destination dan tambahan lainnya untuk null terminator

**Fungsi 2 (get_file_part_size)**

```c
static size_t get_file_part_size(const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) return 0;
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fclose(file);
    return size;
}
```

Inti dari fungsi ini yaitu mengambil ukuran dari pecahan file yang ingin dicari di fungsi tersebut (contohnya: relic_1.png.001)

**Fungsi 3 (get_all_file_parts)**

```c
static void get_all_file_parts(const char *base_path, char part_paths[][MAX_BYTES], int *part_count) {
    int index = 0;
    char part_index[5]; // for ".%03d"
    char part_path[MAX_BYTES];

    while (1) {
        sprintf(part_index, ".%03d", index);
        strcpy(part_path, base_path);
        strcat(part_path, part_index);
        FILE *file = fopen(part_path, "rb");
        if (!file) break;
        fclose(file);
        strcpy(part_paths[*part_count], part_path);
        (*part_count)++;
        index++;
    }
}
```

Fungsi ini akan membaca file yang terdapat di relics lalu ia akan menghitung berapa banyak part yang ada di dalam folder relics tersebut dengan cara meng `strcat` kan index berinkremen terhadap part_path sehingga dalam setiap pengecekan, ia akan mengecek parts yang berbeda (000, 001, 002...)

**Fungsi 4 (my_read)**

```c
static int my_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    char full_path[MAX_BYTES];
    construct_full_path(full_path, path);

    char part_paths[100][MAX_BYTES];
    int part_count = 0;
    get_all_file_parts(full_path, part_paths, &part_count);
    printf("The full path is %s\n", full_path);

    size_t read_bytes = 0;

    for (int i = 0; i < part_count && size > 0; ++i) {
        FILE *file = fopen(part_paths[i], "rb");
        if (!file) break;

        fseek(file, 0L, SEEK_END);
        size_t part_size = ftell(file);
        fseek(file, 0L, SEEK_SET);

        if (offset >= part_size) {
            offset -= part_size;
            fclose(file);
            continue;
        }

        fseek(file, offset, SEEK_SET);
        size_t to_read = size;
        if (to_read > part_size - offset) {
            to_read = part_size - offset;
        }
        size_t temp = fread(buf, 1, to_read, file);
        fclose(file);

        buf += temp;
        size -= temp;
        read_bytes += temp;
        offset = 0;
    }
    return read_bytes;
}
```

Fungsi `my_read` ini membaca array of string [][] dan offset disini maksudnya adalah bytes yang kita baca, bila lebih besar dari size berarti bytes yang kita baca sudah melebihi dalam kata lain kita sudah membaca file selanjutnya dan to_read disini cuman buat menentukan start (bytes awal) dari file yang akan dibaca.

**Fungsi 5 (my_readdir)**

```c
static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    DIR *dir = opendir(base_dir);
    if (!dir) return -errno;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            filler(buf, entry->d_name, NULL, 0);
        } else if (entry->d_type == DT_REG) {
            char *dot = strrchr(entry->d_name, '.');
            if (dot && strcmp(dot, ".000") == 0) {
                char name[500];
                size_t name_length = dot - entry->d_name;
                memcpy(name, entry->d_name, name_length);
                name[name_length] = '\0'; // Add null terminator manually
                printf("Filename is %s\n", entry->d_name);
                filler(buf, name, NULL, 0);
            }
        }
    }
    closedir(dir);
    return 0;
}
```

Simpel saja, fungsi ini membaca salah satu file dan melakukan pemotongan terhadap tulisan akhirnya yaitu .000 (hanya salah satu saja) dan akan dibaca untuk dimasukan kedalam folder fuse dan ini dilakukan sampai relic_n.png (sampai habis)

**Fungsi 6 (get_attr)**

```c
static int my_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        char full_path[MAX_BYTES];
        construct_full_path(full_path, path);

        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 0;

        char part_paths[100][MAX_BYTES];
        int part_count = 0;
        get_all_file_parts(full_path, part_paths, &part_count);

        for (int i = 0; i < part_count; ++i) {
            stbuf->st_size += get_file_part_size(part_paths[i]);
            printf("Iterations: %d\n", i);
        }

        if (part_count == 0) return -ENOENT;
    }
    return 0;
}
```

Kegunaan dari fungsi ini sendiri adalah ia membaca semua file parts yang terdapat di folder relics lalu ia menghitung size-size dari setiap file parts tersebut yang pada akhrinya akan ditambahkan terus menerus ke variable st->buf yang dimana satu ukuran besar tersebut akan menentukkan file akhir yang berada di folder fuse akan berukuran berapa

**Fungsi 7 (my_open)**

```c
static int my_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}
```

Fungsi untuk membuka folder

**Fungsi 8 (my_write)**

```c
static int my_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    char full_path[MAX_BYTES - MAX_SUFFIX_LEN];
    int part_index = offset / MAX_SIZE;
    size_t part_offset = offset % MAX_SIZE;
    size_t written_bytes = 0;
    char part_path[MAX_BYTES];
    construct_full_path(full_path, path);
    printf("Full path is %s\n", full_path);

    while (size > 0) {
        strcpy(part_path, full_path);
        sprintf(part_path + strlen(full_path), ".%03d", part_index);
        printf("Path: %s\n", part_path);

        FILE *file = fopen(part_path, "r+b");
        if (!file) {
            file = fopen(part_path, "wb");
            if (!file) return -errno;
        }

        fseek(file, part_offset, SEEK_SET);
       size_t to_write;
        if (size > (MAX_SIZE - part_offset)) {
            to_write = MAX_SIZE - part_offset;
            size -= to_write;
        } else {
            to_write = size;
            size -= to_write;
        }
        printf("size to write %zu\n", to_write);
        part_offset = 0;
        fwrite(buf, 1, to_write, file);
        fclose(file);

        buf += to_write;
        part_index++;
        written_bytes += to_write;
    }
    return written_bytes;
}
```

Secara garis besar, hal yang dilakukan oleh fungsi ini ialah ia mengambil bytes dari setiap file parts yang ada lalu ia menuliskan bytes-bytes data yang telah ia baca tersebut kedalam file yang berhubungan di dalam folder fuse nanti. Inti dari cara kerja fungsi ini juga ia mengecek apabila sizenya sudah melebihi batas atau belum, jika iya akan dikurangi dan akan dijadikan variable yang akan di write atau ditulis

**Fungsi 9 (my_truncate)**

```c
static int my_truncate(const char *path, off_t size) {
    char full_path[MAX_BYTES - MAX_SUFFIX_LEN];
    construct_full_path(full_path, path);
    int part_index = 0;
    char part_path[MAX_BYTES];
    off_t remaining_size = size;

    while (remaining_size > 0) {
        strcpy(part_path, full_path);
        sprintf(part_path + strlen(full_path), ".%03d", part_index++);
        size_t part_size;
        if (remaining_size > MAX_SIZE) {
            part_size = MAX_SIZE;
        } else {
            part_size = remaining_size;
        }
        int res = truncate(part_path, part_size);
        if (res == -1) return -errno;
        remaining_size -= part_size;
    }

    while (1) {
        strcpy(part_path, full_path);
        sprintf(part_path + strlen(full_path), ".%03d", part_index);
        int res = unlink(part_path);
        if (res == -1 && errno == ENOENT) break;
        else if (res == -1) return -errno;
        part_index++;
    }

    return 0;
}
```

Truncate disini berguna untuk memotong-motong file relic besar yang berada di fuse folder yang dimana hasil bytes pecahan tersebut akan menjadi sebuah file pecahan baru yang nantinya akan diletakan ke dalam folder relics yang asli dan berapa banyak file atau pecahan yang ada juga bergantung kepada seberapa besar file yang ada di fuse folder juga.

**Fungsi 10 (my_unlink)**

```c
static int my_unlink(const char *path) {
    char full_path[MAX_BYTES - MAX_SUFFIX_LEN];
    construct_full_path(full_path, path);

    int part_index = 0;
    char part_path[MAX_BYTES];

    while (1) {
        strcpy(part_path, full_path);
        sprintf(part_path + strlen(full_path), ".%03d", part_index);
        int res = unlink(part_path);
        if (res == -1 && errno == ENOENT) break;
        else if (res == -1) return -errno;
        part_index++;
    }
    return 0;
}
```

Fungsi ini akan menjadi penghubung (atau dalam konteks ini pemutus) dari file relic di fuse dan di relics, yang dimana jika file di salah satu folder hilang maka akan berpengaruh di file lain.

**Fungsi 11 (my_create)**

```c
static int my_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;
    char full_path[MAX_BYTES - MAX_SUFFIX_LEN];
    strcpy(full_path, base_dir);
    strcat(full_path, path);
    char suffix[10];
    sprintf(suffix, ".%03d", 0);  // Create the suffix ".000"
    strcat(full_path, suffix);
    FILE *file = fopen(full_path, "wb");
    if (!file) return -errno;
    fclose(file);
    return 0;
}
```

Fungsi ini hanya berguna untuk mengecek dan mengcreate dari parts file yang ia baca di folder relics

**Akhiran Code**

```c
static struct fuse_operations operations = {
    .getattr = my_getattr,
    .readdir = my_readdir,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .truncate = my_truncate,
    .unlink = my_unlink,
    .create = my_create,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &operations, NULL);
}
```

Seperti yang sudah dijelaskan di modul, `fuse_operations` berguna untuk menginisiasikan fungsi-fungsi yang telah dibuat dan int main meng return `fuse_operations` tersebut.
