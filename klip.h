#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Konstanta untuk clipboard */
#define CLIPBOARD_MAX_ENTRIES 16
#define CLIPBOARD_DEFAULT_SIZE 4096
#define CLIPBOARD_MAX_SIZE 1048576  /* 1MB */

/* Struktur untuk entry clipboard */
typedef struct {
    char *data;         /* Data clipboard */
    size_t size;        /* Ukuran data dalam byte */
    size_t capacity;    /* Kapasitas buffer */
    int type;           /* Tipe data (0=teks, 1=binary) */
    unsigned int id;    /* ID unik untuk entry */
} ClipboardEntry;

/* Struktur untuk clipboard manager */
typedef struct {
    ClipboardEntry entries[CLIPBOARD_MAX_ENTRIES]; /* Entry clipboard */
    int count;                                  /* Jumlah entry */
    int current;                                /* Entry aktif saat ini */
    unsigned int next_id;                        /* ID berikutnya */
    size_t default_size;                         /* Ukuran default untuk entry baru */
    size_t max_size;                            /* Ukuran maksimum per entry */
    int auto_clear;                              /* Auto clear entry lama */
} ClipboardManager;

/* Fungsi utilitas */
static char* clipboard_strdup(const char *str, size_t len) {
    if (!str) return NULL;
    
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    
    memcpy(copy, str, len);
    copy[len] = '\0';
    return copy;
}

/* Inisialisasi clipboard manager */
static int clipboard_init(ClipboardManager *cb, size_t default_size, size_t max_size) {
    int i;
    
    /* Validasi ukuran */
    if (default_size < 256) default_size = CLIPBOARD_DEFAULT_SIZE;
    if (max_size < default_size) max_size = default_size * 2;
    if (max_size > CLIPBOARD_MAX_SIZE) max_size = CLIPBOARD_MAX_SIZE;
    
    /* Inisialisasi manager */
    cb->count = 0;
    cb->current = -1;
    cb->next_id = 1;
    cb->default_size = default_size;
    cb->max_size = max_size;
    cb->auto_clear = 1;
    
    /* Inisialisasi entry */
    for (i = 0; i < CLIPBOARD_MAX_ENTRIES; i++) {
        cb->entries[i].data = NULL;
        cb->entries[i].size = 0;
        cb->entries[i].capacity = 0;
        cb->entries[i].type = 0;
        cb->entries[i].id = 0;
    }
    
    return 1;
}

/* Bebas memori clipboard manager */
static void clipboard_free(ClipboardManager *cb) {
    int i;
    
    /* Bebaskan semua entry */
    for (i = 0; i < cb->count; i++) {
        if (cb->entries[i].data) {
            free(cb->entries[i].data);
            cb->entries[i].data = NULL;
        }
        cb->entries[i].size = 0;
        cb->entries[i].capacity = 0;
        cb->entries[i].type = 0;
        cb->entries[i].id = 0;
    }
    
    cb->count = 0;
    cb->current = -1;
}

/* Hapus entry clipboard tertentu */
static void clipboard_delete_entry(ClipboardManager *cb, int index) {
    if (index < 0 || index >= cb->count) {
        return;
    }
    
    /* Bebaskan memori */
    if (cb->entries[index].data) {
        free(cb->entries[index].data);
        cb->entries[index].data = NULL;
    }
    
    /* Geser entry ke kiri */
    for (int i = index; i < cb->count - 1; i++) {
        cb->entries[i] = cb->entries[i + 1];
    }
    
    /* Reset entry terakhir */
    cb->entries[cb->count - 1].data = NULL;
    cb->entries[cb->count - 1].size = 0;
    cb->entries[cb->count - 1].capacity = 0;
    cb->entries[cb->count - 1].type = 0;
    cb->entries[cb->count - 1].id = 0;
    
    cb->count--;
    
    /* Update current index */
    if (cb->current >= cb->count) {
        cb->current = cb->count - 1;
    }
}

/* Hapus semua entry clipboard */
static void clipboard_clear_all(ClipboardManager *cb) {
    clipboard_free(cb);
}

/* Salin data ke clipboard */
static int clipboard_copy(ClipboardManager *cb, const char *data, size_t size, int type) {
    if (!data || size == 0) {
        return 0;
    }
    
    /* Batasi ukuran */
    if (size > cb->max_size) {
        size = cb->max_size;
    }
    
    /* Auto clear jika penuh */
    if (cb->auto_clear && cb->count >= CLIPBOARD_MAX_ENTRIES) {
        clipboard_delete_entry(cb, 0);
    }
    
    /* Cek apakah masih ada ruang */
    if (cb->count >= CLIPBOARD_MAX_ENTRIES) {
        return 0;
    }
    
    /* Alokasikan memori */
    char *new_data = malloc(size + 1);
    if (!new_data) {
        return 0;
    }
    
    /* Salin data */
    memcpy(new_data, data, size);
    new_data[size] = '\0';
    
    /* Tambahkan entry baru */
    int index = cb->count;
    cb->entries[index].data = new_data;
    cb->entries[index].size = size;
    cb->entries[index].capacity = size + 1;
    cb->entries[index].type = type;
    cb->entries[index].id = cb->next_id++;
    
    cb->count++;
    cb->current = index;
    
    return 1;
}

/* Salin string ke clipboard (otomatis menghitung ukuran) */
static int clipboard_copy_string(ClipboardManager *cb, const char *str) {
    if (!str) {
        return 0;
    }
    
    size_t size = strlen(str);
    return clipboard_copy(cb, str, size, 0); /* Tipe 0 = teks */
}

/* Salin binary data ke clipboard */
static int clipboard_copy_binary(ClipboardManager *cb, const char *data, size_t size) {
    return clipboard_copy(cb, data, size, 1); /* Tipe 1 = binary */
}

/* Tempel data dari clipboard */
static const char* clipboard_paste(ClipboardManager *cb, size_t *size, int *type) {
    if (cb->current < 0 || cb->current >= cb->count) {
        if (size) *size = 0;
        if (type) *type = 0;
        return NULL;
    }
    
    ClipboardEntry *entry = &cb->entries[cb->current];
    
    if (size) *size = entry->size;
    if (type) *type = entry->type;
    
    return entry->data;
}

/* Tempel string dari clipboard */
static const char* clipboard_paste_string(ClipboardManager *cb) {
    size_t size;
    int type;
    const char *data = clipboard_paste(cb, &size, &type);
    
    /* Hanya kembalikan jika tipe teks */
    if (data && type == 0) {
        return data;
    }
    
    return NULL;
}

/* Tempel binary data dari clipboard */
static const char* clipboard_paste_binary(ClipboardManager *cb, size_t *size) {
    int type;
    const char *data = clipboard_paste(cb, size, &type);
    
    /* Hanya kembalikan jika tipe binary */
    if (data && type == 1) {
        return data;
    }
    
    if (size) *size = 0;
    return NULL;
}

/* Dapatkan ukuran clipboard saat ini */
static size_t clipboard_get_size(ClipboardManager *cb) {
    if (cb->current < 0 || cb->current >= cb->count) {
        return 0;
    }
    
    return cb->entries[cb->current].size;
}

/* Dapatkan tipe clipboard saat ini */
static int clipboard_get_type(ClipboardManager *cb) {
    if (cb->current < 0 || cb->current >= cb->count) {
        return -1;
    }
    
    return cb->entries[cb->current].type;
}

/* Dapatkan ID clipboard saat ini */
static unsigned int clipboard_get_id(ClipboardManager *cb) {
    if (cb->current < 0 || cb->current >= cb->count) {
        return 0;
    }
    
    return cb->entries[cb->current].id;
}

/* Pilih entry clipboard */
static int clipboard_select(ClipboardManager *cb, int index) {
    if (index < 0 || index >= cb->count) {
        return 0;
    }
    
    cb->current = index;
    return 1;
}

/* Pilih entry clipboard berdasarkan ID */
static int clipboard_select_by_id(ClipboardManager *cb, unsigned int id) {
    for (int i = 0; i < cb->count; i++) {
        if (cb->entries[i].id == id) {
            cb->current = i;
            return 1;
        }
    }
    
    return 0;
}

/* Dapatkan jumlah entry clipboard */
static int clipboard_get_count(ClipboardManager *cb) {
    return cb->count;
}

/* Dapatkan index entry saat ini */
static int clipboard_get_current_index(ClipboardManager *cb) {
    return cb->current;
}

/* Hapus entry clipboard saat ini */
static int clipboard_clear_current(ClipboardManager *cb) {
    if (cb->current < 0 || cb->current >= cb->count) {
        return 0;
    }
    
    clipboard_delete_entry(cb, cb->current);
    return 1;
}

/* Tukar dua entry clipboard */
static int clipboard_swap(ClipboardManager *cb, int index1, int index2) {
    if (index1 < 0 || index1 >= cb->count || index2 < 0 || index2 >= cb->count) {
        return 0;
    }
    
    if (index1 == index2) {
        return 1; /* Tidak perlu ditukar */
    }
    
    /* Tukar entry */
    ClipboardEntry temp = cb->entries[index1];
    cb->entries[index1] = cb->entries[index2];
    cb->entries[index2] = temp;
    
    /* Update current index */
    if (cb->current == index1) {
        cb->current = index2;
    } else if (cb->current == index2) {
        cb->current = index1;
    }
    
    return 1;
}

/* Pindahkan entry clipboard */
static int clipboard_move(ClipboardManager *cb, int from, int to) {
    if (from < 0 || from >= cb->count || to < 0 || to >= cb->count) {
        return 0;
    }
    
    if (from == to) {
        return 1; /* Tidak perlu dipindahkan */
    }
    
    /* Simpan entry sementara */
    ClipboardEntry temp = cb->entries[from];
    
    /* Geser entry */
    if (from < to) {
        for (int i = from; i < to; i++) {
            cb->entries[i] = cb->entries[i + 1];
        }
    } else {
        for (int i = from; i > to; i--) {
            cb->entries[i] = cb->entries[i - 1];
        }
    }
    
    /* Tempatkan entry di posisi baru */
    cb->entries[to] = temp;
    
    /* Update current index */
    if (cb->current == from) {
        cb->current = to;
    } else if (cb->current >= to && cb->current < from) {
        cb->current++;
    } else if (cb->current <= to && cb->current > from) {
        cb->current--;
    }
    
    return 1;
}

/* Set auto clear */
static void clipboard_set_auto_clear(ClipboardManager *cb, int auto_clear) {
    cb->auto_clear = auto_clear;
}

/* Dapatkan info entry clipboard */
static int clipboard_get_entry_info(ClipboardManager *cb, int index, 
                                   unsigned int *id, size_t *size, int *type) {
    if (index < 0 || index >= cb->count) {
        return 0;
    }
    
    ClipboardEntry *entry = &cb->entries[index];
    
    if (id) *id = entry->id;
    if (size) *size = entry->size;
    if (type) *type = entry->type;
    
    return 1;
}

/* Cari entry clipboard berdasarkan ID */
static int clipboard_find_by_id(ClipboardManager *cb, unsigned int id) {
    for (int i = 0; i < cb->count; i++) {
        if (cb->entries[i].id == id) {
            return i;
        }
    }
    
    return -1;
}

/* Cari entry clipboard berdasarkan prefix string */
static int clipboard_find_by_prefix(ClipboardManager *cb, const char *prefix, int start_index) {
    if (!prefix || start_index < 0 || start_index >= cb->count) {
        return -1;
    }
    
    size_t prefix_len = strlen(prefix);
    
    for (int i = start_index; i < cb->count; i++) {
        if (cb->entries[i].type == 0 && /* Hanya cari di entry teks */
            cb->entries[i].size >= prefix_len &&
            strncmp(cb->entries[i].data, prefix, prefix_len) == 0) {
            return i;
        }
    }
    
    return -1;
}

/* Simpan clipboard ke file */
static int clipboard_save_to_file(ClipboardManager *cb, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen failed");
        return 0;
    }
    
    /* Tulis header */
    fprintf(file, "CLIPBOARD\n");
    fprintf(file, "COUNT=%d\n", cb->count);
    fprintf(file, "CURRENT=%d\n", cb->current);
    fprintf(file, "NEXT_ID=%u\n", cb->next_id);
    
    /* Tulis entry */
    for (int i = 0; i < cb->count; i++) {
        ClipboardEntry *entry = &cb->entries[i];
        fprintf(file, "ENTRY\n");
        fprintf(file, "ID=%u\n", entry->id);
        fprintf(file, "TYPE=%d\n", entry->type);
        fprintf(file, "SIZE=%lu\n", (unsigned long)entry->size);
        fprintf(file, "DATA=");
        
        /* Tulis data dalam format hex */
        for (size_t j = 0; j < entry->size; j++) {
            fprintf(file, "%02x", (unsigned char)entry->data[j]);
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    return 1;
}

/* Muat clipboard dari file */
static int clipboard_load_from_file(ClipboardManager *cb, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen failed");
        return 0;
    }
    
    char line[1024];
    int entry_count = 0;
    int current = -1;
    unsigned int next_id = 1;
    
    /* Baca header */
    if (!fgets(line, sizeof(line), file) || strcmp(line, "CLIPBOARD\n") != 0) {
        fclose(file);
        return 0;
    }
    
    /* Baca count */
    if (!fgets(line, sizeof(line), file) || sscanf(line, "COUNT=%d", &entry_count) != 1) {
        fclose(file);
        return 0;
    }
    
    /* Baca current */
    if (!fgets(line, sizeof(line), file) || sscanf(line, "CURRENT=%d", &current) != 1) {
        fclose(file);
        return 0;
    }
    
    /* Baca next_id */
    if (!fgets(line, sizeof(line), file) || sscanf(line, "NEXT_ID=%u", &next_id) != 1) {
        fclose(file);
        return 0;
    }
    
    /* Clear clipboard saat ini */
    clipboard_clear_all(cb);
    
    /* Baca entry */
    for (int i = 0; i < entry_count && i < CLIPBOARD_MAX_ENTRIES; i++) {
        /* Baca header entry */
        if (!fgets(line, sizeof(line), file) || strcmp(line, "ENTRY\n") != 0) {
            break;
        }
        
        unsigned int id;
        int type;
        size_t size;
        
        /* Baca ID */
        if (!fgets(line, sizeof(line), file) || sscanf(line, "ID=%u", &id) != 1) {
            break;
        }
        
        /* Baca type */
        if (!fgets(line, sizeof(line), file) || sscanf(line, "TYPE=%d", &type) != 1) {
            break;
        }
        
        /* Baca size */
        if (!fgets(line, sizeof(line), file) || sscanf(line, "SIZE=%lu", (unsigned long*)&size) != 1) {
            break;
        }
        
        /* Baca data */
        if (!fgets(line, sizeof(line), file) || strncmp(line, "DATA=", 5) != 0) {
            break;
        }
        
        /* Parse hex data */
        char *data_ptr = line + 5;
        size_t data_len = strlen(data_ptr);
        if (data_len % 2 != 0) {
            break; /* Data hex harus genap */
        }
        
        /* Alokasikan memori */
        char *data = malloc(size + 1);
        if (!data) {
            break;
        }
        
        /* Konversi hex ke binary */
        for (size_t j = 0; j < size && j * 2 < data_len; j++) {
            char hex[3] = {data_ptr[j * 2], data_ptr[j * 2 + 1], 0};
            data[j] = (char)strtol(hex, NULL, 16);
        }
        data[size] = '\0';
        
        /* Tambahkan entry */
        cb->entries[cb->count].data = data;
        cb->entries[cb->count].size = size;
        cb->entries[cb->count].capacity = size + 1;
        cb->entries[cb->count].type = type;
        cb->entries[cb->count].id = id;
        cb->count++;
    }
    
    /* Set state */
    cb->current = (current >= 0 && current < cb->count) ? current : cb->count - 1;
    cb->next_id = next_id;
    
    fclose(file);
    return 1;
}

#endif /* CLIPBOARD_H */
