/*
 * PIGURA OS - KANVAS_HAPUS.C
 * ==========================
 * Implementasi penghancuran dan pembersihan kanvas.
 *
 * Berkas ini berisi fungsi-fungsi untuk menghancurkan kanvas,
 * membebaskan memori buffer piksel, membersihkan state gambar,
 * dan mengamankan data piksel sebelum dealokasi.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) ketat
 * - Tidak menggunakan fitur C99/C11
 * - Maksimal 80 karakter per baris
 * - Semua nama dalam Bahasa Indonesia
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * DEFINISI STRUKTUR (STRUCT DEFINITIONS)
 * ===========================================================================
 * Struktur data kanvas dan buffer piksel. Definisi harus sesuai dengan
 * berkas kanvas lainnya (kanvas_buat.c, kanvas_blit.c, dll).
 */

#define KANVAS_MAGIC        0x4B564153
#define KANVAS_NAMA_MAKS    64
#define BUFFER_MAGIC        0x42554642
#define BUFFER_FLAG_ALOKASI 0x01
#define BUFFER_FLAG_EKSTERNAL 0x02

struct buffer_piksel {
    tak_bertanda32_t magic;
    tak_bertanda16_t versi;
    tak_bertanda32_t lebar;
    tak_bertanda32_t tinggi;
    tak_bertanda8_t  format;
    tak_bertanda8_t  bit_per_piksel;
    tak_bertanda32_t pitch;
    ukuran_t         ukuran;
    tak_bertanda8_t *data;
    tak_bertanda32_t flags;
    tak_bertanda32_t referensi;
};

struct kanvas_transformasi {
    tak_bertanda32_t jenis;
    tanda32_t        tx;
    tanda32_t        ty;
    tak_bertanda32_t sx;
    tak_bertanda32_t sy;
};

struct kanvas_klip {
    bool_t           aktif;
    tak_bertanda32_t x;
    tak_bertanda32_t y;
    tak_bertanda32_t lebar;
    tak_bertanda32_t tinggi;
};

struct kanvas {
    tak_bertanda32_t    magic;
    tak_bertanda16_t    versi;
    struct buffer_piksel *buffer;
    char                nama[KANVAS_NAMA_MAKS];
    tak_bertanda32_t    id;
    bool_t              aktif;
    tak_bertanda32_t    flags;
    bool_t              terkunci;
    tak_bertanda32_t    pemilik_kunci;
    tak_bertanda8_t     warna_r;
    tak_bertanda8_t     warna_g;
    tak_bertanda8_t     warna_b;
    tak_bertanda8_t     warna_a;
    struct kanvas_transformasi transformasi;
    struct kanvas_klip klip;
    tak_bertanda32_t    jumlah_gambar;
    bool_t              kotor;
};

/*
 * ===========================================================================
 * FUNGSI PEMBANTU INTERNAL (INTERNAL HELPER FUNCTIONS)
 * ===========================================================================
 */

/*
 * bersihkan_kanvas_state
 * ----------------------
 * Reset seluruh state kanvas ke nilai default tanpa memodifikasi
 * pointer buffer atau data piksel.
 *
 * Parameter:
 *   kv - Pointer ke struktur kanvas
 */
static void bersihkan_kanvas_state(struct kanvas *kv)
{
    kv->magic = KANVAS_MAGIC;
    kv->versi = 1;
    kv->nama[0] = '\0';
    kv->id = 0;
    kv->aktif = SALAH;
    kv->flags = 0;
    kv->terkunci = SALAH;
    kv->pemilik_kunci = 0;

    /*
     * Reset warna default: putih penuh (R=255, G=255, B=255, A=255)
     */
    kv->warna_r = 255;
    kv->warna_g = 255;
    kv->warna_b = 255;
    kv->warna_a = 255;

    /*
     * Reset transformasi ke identitas (tanpa transformasi)
     */
    kv->transformasi.jenis = 0;
    kv->transformasi.tx = 0;
    kv->transformasi.ty = 0;
    kv->transformasi.sx = 1;
    kv->transformasi.sy = 1;

    /*
     * Reset klip ke area penuh (tidak aktif berarti tanpa batasan)
     */
    kv->klip.aktif = SALAH;
    kv->klip.x = 0;
    kv->klip.y = 0;
    kv->klip.lebar = 0;
    kv->klip.tinggi = 0;

    kv->jumlah_gambar = 0;
    kv->kotor = SALAH;
}

/*
 * ===========================================================================
 * kanvas_hapus
 * ===========================================================================
 */

/*
 * kanvas_hapus
 * ------------
 * Menghancurkan kanvas sepenuhnya dan membebaskan semua memori yang
 * terkait, termasuk buffer piksel dan struktur kanvas itu sendiri.
 *
 * Fungsi ini melakukan langkah-langkah berikut:
 * 1. Validasi pointer kanvas dan magic number
 * 2. Cek apakah kanvas terkunci (STATUS_BUSY)
 * 3. Bebaskan buffer piksel jika ada dan dimiliki
 * 4. Nol-kan seluruh struktur kanvas
 * 5. Bebaskan memori struktur kanvas
 *
 * Parameter:
 *   kv - Pointer ke struktur kanvas yang akan dihancurkan
 *
 * Return:
 *   STATUS_BERHASIL    - Kanvas berhasil dihancurkan
 *   STATUS_PARAM_NULL  - Pointer kv adalah NULL
 *   STATUS_PARAM_INVALID - Magic number tidak sesuai
 *   STATUS_BUSY        - Kanvas sedang terkunci
 */
status_t kanvas_hapus(struct kanvas *kv)
{
    struct buffer_piksel *buf;

    /* Validasi pointer kanvas */
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi magic number kanvas */
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah kanvas sedang terkunci oleh proses lain */
    if (kv->terkunci == BENAR) {
        return STATUS_BUSY;
    }

    /* Tangani buffer piksel jika ada */
    buf = kv->buffer;
    if (buf != NULL) {
        /* Validasi magic number buffer */
        if (buf->magic == BUFFER_MAGIC) {
            /*
             * Jika buffer memiliki flag alokasi internal,
             * bebaskan data piksel terlebih dahulu
             */
            if ((buf->flags & BUFFER_FLAG_ALOKASI) != 0) {
                if (buf->data != NULL) {
                    kernel_memset(buf->data, 0, buf->ukuran);
                    kfree(buf->data);
                    buf->data = NULL;
                }
            }
            /*
             * Invalidate magic sebelum membebaskan buffer
             * untuk mencegah use-after-free
             */
            buf->magic = 0;
            kfree(buf);
        }
        kv->buffer = NULL;
    }

    /* Nol-kan seluruh struktur kanvas sebelum pembebasan */
    kernel_memset(kv, 0, sizeof(struct kanvas));

    /* Bebaskan memori struktur kanvas itu sendiri */
    kfree(kv);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * kanvas_hapus_buffer_saja
 * ===========================================================================
 */

/*
 * kanvas_hapus_buffer_saja
 * ------------------------
 * Menghancurkan hanya buffer piksel kanvas tanpa menghancurkan
 * struktur kanvas itu sendiri. Berguna untuk realokasi buffer
 * dengan ukuran atau format yang berbeda.
 *
 * Setelah pemanggilan fungsi ini, kanvas tetap valid namun
 * tidak memiliki buffer piksel (kv->buffer == NULL).
 *
 * Parameter:
 *   kv - Pointer ke struktur kanvas
 *
 * Return:
 *   STATUS_BERHASIL    - Buffer berhasil dihapus
 *   STATUS_PARAM_NULL  - Pointer kv adalah NULL
 *   STATUS_PARAM_INVALID - Magic number tidak sesuai
 */
status_t kanvas_hapus_buffer_saja(struct kanvas *kv)
{
    struct buffer_piksel *buf;

    /* Validasi pointer kanvas */
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi magic number kanvas */
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    buf = kv->buffer;
    if (buf != NULL) {
        /* Validasi magic number buffer */
        if (buf->magic == BUFFER_MAGIC) {
            /*
             * Hanya bebaskan data piksel jika buffer dimiliki
             * secara internal (flag alokasi), bukan eksternal
             */
            if ((buf->flags & BUFFER_FLAG_ALOKASI) != 0) {
                if (buf->data != NULL) {
                    kernel_memset(buf->data, 0, buf->ukuran);
                    kfree(buf->data);
                    buf->data = NULL;
                }
            }
            /* Invalidate magic buffer */
            buf->magic = 0;
            kfree(buf);
        }
        kv->buffer = NULL;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * kanvas_bersihkan_pixel
 * ===========================================================================
 */

/*
 * kanvas_bersihkan_pixel
 * ----------------------
 * Mengosongkan (zero-fill) seluruh buffer piksel kanvas tanpa
 * menghancurkan atau membebaskan apapun. Fungsi ini berguna
 * untuk menghapus semua gambar yang telah digambar pada kanvas
 * tanpa perlu mengalokasikan ulang buffer.
 *
 * Semua piksel akan diatur ke nilai 0 (hitam transparan).
 *
 * Parameter:
 *   kv - Pointer ke struktur kanvas
 *
 * Return:
 *   STATUS_BERHASIL    - Pixel berhasil dibersihkan
 *   STATUS_PARAM_NULL  - Pointer kv atau buffer adalah NULL
 *   STATUS_PARAM_INVALID - Magic number tidak sesuai
 */
status_t kanvas_bersihkan_pixel(struct kanvas *kv)
{
    struct buffer_piksel *buf;

    /* Validasi pointer kanvas */
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi magic number kanvas */
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Validasi keberadaan buffer */
    buf = kv->buffer;
    if (buf == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi keberadaan data piksel */
    if (buf->data == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi ukuran buffer */
    if (buf->ukuran == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Zero-fill seluruh area piksel */
    kernel_memset(buf->data, 0, buf->ukuran);

    /* Tandai kanvas sebagai kotor (perlu flip/redraw) */
    kv->kotor = BENAR;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * kanvas_bersihkan_state
 * ===========================================================================
 */

/*
 * kanvas_bersihkan_state
 * ----------------------
 * Me-reset seluruh state gambar kanvas ke nilai default tanpa
 * menyentuh data piksel. Fungsi ini berguna untuk memulihkan
 * kanvas ke kondisi awal tanpa kehilangan konten yang sudah
 * digambar.
 *
 * State yang direset:
 * - Warna gambar: (255, 255, 255, 255) putih penuh
 * - Transformasi: identitas (tanpa translasi/skala)
 * - Klip: nonaktif (area penuh)
 * - Jumlah gambar: 0
 * - Flag kotor: SALAH
 *
 * Parameter:
 *   kv - Pointer ke struktur kanvas
 *
 * Return:
 *   STATUS_BERHASIL    - State berhasil direset
 *   STATUS_PARAM_NULL  - Pointer kv adalah NULL
 *   STATUS_PARAM_INVALID - Magic number tidak sesuai
 */
status_t kanvas_bersihkan_state(struct kanvas *kv)
{
    /* Validasi pointer kanvas */
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi magic number kanvas */
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Reset warna ke putih penuh */
    kv->warna_r = 255;
    kv->warna_g = 255;
    kv->warna_b = 255;
    kv->warna_a = 255;

    /* Reset transformasi ke identitas */
    kv->transformasi.jenis = 0;
    kv->transformasi.tx = 0;
    kv->transformasi.ty = 0;
    kv->transformasi.sx = 1;
    kv->transformasi.sy = 1;

    /* Reset klip ke area penuh (nonaktif = tanpa batasan) */
    kv->klip.aktif = SALAH;
    kv->klip.x = 0;
    kv->klip.y = 0;
    kv->klip.lebar = 0;
    kv->klip.tinggi = 0;

    /* Reset jumlah gambar yang telah digambar */
    kv->jumlah_gambar = 0;

    /* Tandai kanvas sebagai tidak kotor */
    kv->kotor = SALAH;

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * kanvas_bersihkan_semua
 * ===========================================================================
 */

/*
 * kanvas_bersihkan_semua
 * ----------------------
 * Reset lengkap kanvas: membersihkan pixel DAN state secara bersamaan.
 * Fungsi ini memanggil kanvas_bersihkan_pixel terlebih dahulu, kemudian
 * kanvas_bersihkan_state. Kanvas tetap utuh dan dapat digunakan kembali
 * setelah pemanggilan fungsi ini.
 *
 * Parameter:
 *   kv - Pointer ke struktur kanvas
 *
 * Return:
 *   STATUS_BERHASIL     - Kanvas berhasil dibersihkan
 *   STATUS_PARAM_NULL   - Pointer kv adalah NULL
 *   STATUS_PARAM_INVALID - Magic number tidak sesuai
 *   (error lain dari kanvas_bersihkan_pixel)
 */
status_t kanvas_bersihkan_semua(struct kanvas *kv)
{
    status_t st;

    /* Validasi pointer kanvas */
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi magic number kanvas */
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Bersihkan seluruh pixel terlebih dahulu */
    st = kanvas_bersihkan_pixel(kv);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    /* Kemudian reset state gambar ke default */
    st = kanvas_bersihkan_state(kv);
    if (st != STATUS_BERHASIL) {
        return st;
    }

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * kanvas_hapus_semua_daftar
 * ===========================================================================
 */

/*
 * kanvas_hapus_semua_daftar
 * -------------------------
 * Menghancurkan seluruh array kanvas yang diberikan. Setiap pointer
 * kanvas non-NULL dalam array akan dihancurkan menggunakan kanvas_hapus.
 * Setelah semua kanvas dihancurkan, array daftar itu sendiri juga
 * dibebaskan dari memori.
 *
 * Fungsi ini aman dipanggil dengan elemen NULL dalam array; elemen
 * tersebut akan dilewati tanpa error.
 *
 * Parameter:
 *   daftar - Pointer ke array pointer kanvas
 *   jumlah - Jumlah elemen dalam array
 *
 * Return:
 *   STATUS_BERHASIL    - Semua kanvas berhasil dihancurkan
 *   STATUS_PARAM_NULL  - Pointer daftar adalah NULL
 *   STATUS_PARAM_INVALID - Jumlah bernilai 0
 */
status_t kanvas_hapus_semua_daftar(struct kanvas **daftar,
                                    tak_bertanda32_t jumlah)
{
    tak_bertanda32_t i;

    /* Validasi pointer daftar */
    if (daftar == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi jumlah elemen */
    if (jumlah == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Iterasi dan hancurkan setiap kanvas non-NULL */
    for (i = 0; i < jumlah; i++) {
        if (daftar[i] != NULL) {
            kanvas_hapus(daftar[i]);
            daftar[i] = NULL;
        }
    }

    /* Bebaskan array daftar itu sendiri */
    kfree(daftar);

    return STATUS_BERHASIL;
}

/*
 * ===========================================================================
 * kanvas_bebersih_memori
 * ===========================================================================
 */

/*
 * kanvas_bebersih_memori
 * ----------------------
 * Mengamankan (secure wipe) data piksel kanvas dengan mengisi seluruh
 * buffer menggunakan pola byte tertentu. Fungsi ini berguna untuk
 * keperluan keamanan, misalnya membersihkan konten sensitif yang telah
 * di-render sebelum memori dikembalikan ke sistem atau sebelum buffer
 * dipakai ulang oleh proses lain.
 *
 * Parameter:
 *   kv   - Pointer ke struktur kanvas
 *   pola - Byte pola pengisian (0-255), akan diulang pada seluruh
 *          buffer piksel
 *
 * Return:
 *   STATUS_BERHASIL    - Memori berhasil di-bebersih
 *   STATUS_PARAM_NULL  - Pointer kv, buffer, atau data adalah NULL
 *   STATUS_PARAM_INVALID - Magic number tidak sesuai atau ukuran 0
 */
status_t kanvas_bebersih_memori(struct kanvas *kv,
                                 tak_bertanda8_t pola)
{
    struct buffer_piksel *buf;

    /* Validasi pointer kanvas */
    if (kv == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi magic number kanvas */
    if (kv->magic != KANVAS_MAGIC) {
        return STATUS_PARAM_INVALID;
    }

    /* Validasi keberadaan buffer */
    buf = kv->buffer;
    if (buf == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi keberadaan data piksel */
    if (buf->data == NULL) {
        return STATUS_PARAM_NULL;
    }

    /* Validasi ukuran buffer */
    if (buf->ukuran == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Isi seluruh buffer dengan pola byte keamanan */
    kernel_memset(buf->data, (int)pola, buf->ukuran);

    /* Tandai kanvas sebagai kotor setelah pengamanan memori */
    kv->kotor = BENAR;

    return STATUS_BERHASIL;
}
