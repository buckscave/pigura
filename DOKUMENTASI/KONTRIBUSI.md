# PANDUAN KONTRIBUSI PIGURA OS

## Daftar Isi

1. [Pendahuluan](#pendahuluan)
2. [Kode Etik](#kode-etik)
3. [Memulai Kontribusi](#memulai-kontribusi)
4. [Standar Kode](#standar-kode)
5. [Proses Pull Request](#proses-pull-request)
6. [Pelaporan Bug](#pelaporan-bug)
7. [Permintaan Fitur](#permintaan-fitur)
8. [Dokumentasi](#dokumentasi)
9. [Pengujian](#pengujian)
10. [Komunikasi](#komunikasi)

---

## Pendahuluan

Terima kasih atas ketertarikan Anda untuk berkontribusi pada Pigura OS! Proyek ini adalah sistem operasi original yang ditulis dari scratch dengan filosofi minimalis dan efisiensi maksimal. Kami menyambut segala bentuk kontribusi, baik berupa kode, dokumentasi, pengujian, maupun saran konstruktif.

### Visi Proyek

Pigura OS dibangun dengan visi:

- **Original** - Bukan fork Linux/Unix, identitas sendiri
- **Minimalis** - Hanya komponen yang diperlukan
- **Portabel** - Multi-arsitektur dengan HAL
- **Aman** - Fungsi bounded, compositor isolasi
- **Indonesia** - Penamaan dan dokumentasi dalam bahasa Indonesia

---

## Kode Etik

### Standar Perilaku

Sebagai kontributor, Anda diharapkan:

1. **Saling Menghormati**
   - Hormati perbedaan pendapat
   - Gunakan bahasa yang sopan dan inklusif
   - Tidak ada pelecehan, diskriminasi, atau ujaran kebencian

2. **Konstruktif**
   - Berikan kritik yang membangun
   - Fokus pada kode, bukan orang
   - Terima kritik dengan lapang dada

3. **Kolaboratif**
   - Bantu kontributor lain
   - Berbagi pengetahuan
   - Dokumentasikan pekerjaan Anda

4. **Profesional**
   - Patuhi standar kode
   - Test kode sebelum submit
   - Respons feedback dengan tepat

---

## Memulai Kontribusi

### Prasyarat

Sebelum berkontribusi, pastikan Anda memiliki:

| Prasyarat | Minimum | Disarankan |
|-----------|---------|------------|
| Compiler | GCC 7+ | GCC 11+ |
| Make | 4.0+ | Latest |
| QEMU | 4.0+ | 7.0+ |
| Git | 2.20+ | Latest |
| Editor | Any | VS Code/Vim |

### Setup Lingkungan

```bash
# 1. Fork repository
# Kunjungi halaman Git dan klik "Fork"

# 2. Clone fork Anda
git clone https://github.com/USERNAME/pigura.git
cd pigura

# 3. Tambahkan upstream remote
git remote add upstream https://github.com/ORIGINAL/pigura.git

# 4. Install dependensi
./ALAT/install_deps.sh

# 5. Build untuk verifikasi
make ARCH=x86

# 6. Jalankan di QEMU
./ALAT/qemu_run.sh x86 text
```

### Menjaga Fork Tetap Update

```bash
# Fetch perubahan dari upstream
git fetch upstream

# Merge ke branch lokal
git checkout main
git merge upstream/main

# Push ke fork Anda
git push origin main
```

---

## Standar Kode

### Pigura C90

Pigura OS menggunakan **Pigura C90** yang didefinisikan sebagai:

```
PIGURA C90 = C89 Core + POSIX Safe Functions
```

### Aturan Penamaan

#### Bahasa Indonesia

Semua penamaan **HARUS** dalam bahasa Indonesia:

```c
/* BENAR */
int hitung_total(void);
status_t proses_mulai(void);
void *alokasi_memori(ukuran_t ukuran);

/* SALAH */
int calculate_total(void);
status_t start_process(void);
void *allocate_memory(size_t size);
```

#### Konvensi Penamaan

| Tipe | Format | Contoh |
|------|--------|--------|
| Fungsi | snake_case | `proses_mulai()` |
| Variabel | snake_case | `jumlah_proses` |
| Konstanta | UPPER_CASE | `UKURAN_HALAMAN` |
| Tipe | snake_case_t | `proses_t` |
| Struct | snake_case_t | `proses_t` |
| Enum | snake_case_t | `status_t` |
| Enum value | UPPER_CASE | `STATUS_BERHASIL` |
| Macro | UPPER_CASE | `MIN(a, b)` |
| File | snake_case.c | `proses_mulai.c` |

#### Tipe Data Kustom

Gunakan tipe data Pigura OS:

```c
/* Tipe bertanda */
tanda8_t, tanda16_t, tanda32_t, tanda64_t

/* Tipe tak bertanda */
tak_bertanda8_t, tak_bertanda16_t, tak_bertanda32_t, tak_bertanda64_t

/* Ukuran */
ukuran_t, jarak_t

/* Alamat */
alamat_fisik_t, alamat_virtual_t

/* Proses */
pid_t, tid_t, uid_t, gid_t

/* Boolean */
bool_t (SALAH = 0, BENAR = 1)

/* Status */
status_t (STATUS_BERHASIL, STATUS_GAGAL, ...)
```

### Batas Karakter Per Baris

**MAKSIMAL 80 KARAKTER PER BARIS**

```c
/* BENAR - Maksimal 80 karakter */
status_t proses_buat(const char *nama,
                     pid_t ppid,
                     tak_bertanda32_t flags)
{
    /* ... */
}

/* SALAH - Lebih dari 80 karakter */
status_t proses_buat(const char *nama, pid_t ppid, tak_bertanda32_t flags, sangat_panjang_param_t param_lagi)
{
    /* ... */
}
```

### Fungsi Aman

**WAJIB** menggunakan fungsi bounded:

```c
/* BENAR - Fungsi aman */
char buffer[256];
snprintf(buffer, sizeof(buffer), "Nilai: %d", nilai);
strncpy(dest, src, sizeof(dest) - 1);
fgets(line, sizeof(line), file);

/* SALAH - Fungsi berbahaya */
char buffer[256];
sprintf(buffer, "Nilai: %d", nilai);  /* Buffer overflow risk! */
strcpy(dest, src);                     /* Buffer overflow risk! */
gets(line);                            /* NEVER use this! */
```

### Komentar

Komentar **HARUS** dalam bahasa Indonesia:

```c
/*
 * proses_init - Inisialisasi subsistem proses
 *
 * Fungsi ini menginisialisasi semua struktur data yang diperlukan
 * untuk manajemen proses. Dipanggil sekali saat boot.
 *
 * Return: STATUS_BERHASIL jika sukses, kode error jika gagal
 */
status_t proses_init(void)
{
    /* Alokasi tabel proses */
    proses_tab = kmalloc(sizeof(proses_t) * MAKS_PROSES);
    if (proses_tab == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Inisialisasi scheduler */
    status_t status = scheduler_init();
    if (status != STATUS_BERHASIL) {
        kfree(proses_tab);
        return status;
    }

    return STATUS_BERHASIL;
}
```

### Header File

Setiap file C **HARUS** memiliki header yang jelas:

```c
/*
 * PIGURA OS - PROSES_INIT.C
 * --------------------------
 * Inisialisasi subsistem proses.
 *
 * Berkas ini berisi fungsi-fungsi untuk menginisialisasi
 * manajemen proses kernel.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */
```

### Error Handling

Selalu handle error dengan benar:

```c
/* BENAR */
status_t proses_buat(const char *nama, pid_t ppid)
{
    if (nama == NULL) {
        return STATUS_PARAM_INVALID;
    }

    proses_t *p = kmalloc(sizeof(proses_t));
    if (p == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    status_t status = proses_inisialisasi(p);
    if (status != STATUS_BERHASIL) {
        kfree(p);
        return status;
    }

    return STATUS_BERHASIL;
}

/* SALAH - Tidak ada error handling */
status_t proses_buat(const char *nama, pid_t ppid)
{
    proses_t *p = kmalloc(sizeof(proses_t));
    proses_inisialisasi(p);
    return STATUS_BERHASIL;
}
```

### Tidak Ada Stubs/TODOS

**DILARANG** mengcommit kode dengan stubs atau TODOs:

```c
/* SALAH */
void fungsi_baru(void)
{
    /* TODO: implement this */
}

/* SALAH */
status_t fungsi_lain(void)
{
    return STATUS_BERHASIL;  /* stub */
}

/* BENAR - Implementasi lengkap atau tidak sama sekali */
status_t fungsi_lengkap(void)
{
    /* Implementasi nyata */
    return STATUS_BERHASIL;
}
```

---

## Proses Pull Request

### Alur Kerja

```
1. Fork repository
      ↓
2. Buat branch baru
      ↓
3. Implementasi perubahan
      ↓
4. Test perubahan
      ↓
5. Commit dengan pesan jelas
      ↓
6. Push ke fork
      ↓
7. Buat Pull Request
      ↓
8. Address review feedback
      ↓
9. Merge
```

### Membuat Branch

```bash
# Buat branch dengan nama deskriptif
git checkout -b fitur/scheduler-prioritas

# Atau untuk bugfix
git checkout -b bugfix/memory-leak-heap
```

### Commit Message

Format commit message:

```
<jenis>: <deskripsi singkat>

<penjelasan detail jika perlu>

Fixes #<issue-number> (jika ada)
```

Jenis commit:
- `fitur:` - Fitur baru
- `bugfix:` - Perbaikan bug
- `refactor:` - Refactoring kode
- `docs:` - Perubahan dokumentasi
- `test:` - Penambahan/modifikasi test
- `style:` - Perubahan formatting

Contoh:

```
fitur: implementasi scheduler priority-based

Tambahkan dukungan untuk priority-based scheduling
dengan 4 level prioritas: RENDAH, NORMAL, TINGGI, REALTIME.
Proses dengan prioritas tinggi akan diproses lebih dulu.

Fixes #42
```

### Pull Request Template

```markdown
## Deskripsi
Jelaskan perubahan yang Anda buat secara singkat.

## Jenis Perubahan
- [ ] Bugfix
- [ ] Fitur baru
- [ ] Refactoring
- [ ] Dokumentasi
- [ ] Test

## Checklist
- [ ] Kode mengikuti standar Pigura C90
- [ ] Semua nama dalam bahasa Indonesia
- [ ] Tidak ada stubs atau TODOs
- [ ] Maksimal 80 karakter per baris
- [ ] Error handling lengkap
- [ ] Test telah dijalankan
- [ ] Dokumentasi diperbarui

## Testing
Jelaskan bagaimana Anda menguji perubahan ini.

## Screenshot (jika relevan)
```

---

## Pelaporan Bug

### Template Bug Report

```markdown
## Deskripsi Bug
Jelaskan bug dengan jelas.

## Langkah Reproduksi
1. Jalankan perintah '...'
2. Lihat output '...'
3. Error muncul

## Perilaku yang Diharapkan
Jelaskan apa yang seharusnya terjadi.

## Perilaku Aktual
Jelaskan apa yang benar-benar terjadi.

## Lingkungan
- Arsitektur: [x86/x86_64/arm/armv7/arm64]
- Versi GCC: [contoh: 11.2.0]
- Versi QEMU: [contoh: 7.0.0]
- OS Host: [contoh: Ubuntu 22.04]

## Log/Error Message
```
Paste log atau error message di sini
```

## Screenshot
Jika relevan, tambahkan screenshot.

## Informasi Tambahan
Tambahkan informasi lain yang relevan.
```

---

## Permintaan Fitur

### Template Feature Request

```markdown
## Ringkasan Fitur
Jelaskan fitur yang Anda inginkan secara singkat.

## Motivasi
Mengapa fitur ini diperlukan? Masalah apa yang dipecahkan?

## Deskripsi Detail
Jelaskan bagaimana fitur ini seharusnya bekerja.

## Alternatif yang Dipertimbangkan
Adakah alternatif lain yang Anda pertimbangkan?

## Implementasi yang Disarankan
Jika Anda punya ide implementasi, jelaskan di sini.

## Dampak
- Ukuran kode: [perkiraan]
- Kompleksitas: [rendah/sedang/tinggi]
- Kompatibilitas: [perubahan API?]
```

---

## Dokumentasi

### Standar Dokumentasi

1. **Bahasa Indonesia** - Semua dokumentasi dalam bahasa Indonesia
2. **Markdown** - Gunakan format Markdown
3. **Struktur Jelas** - Gunakan heading dan daftar isi
4. **Contoh Kode** - Sertakan contoh penggunaan
5. **Update Berkala** - Update dokumentasi sesuai perubahan kode

### Komentar Kode

```c
/*
 * fungsi_nama - Deskripsi singkat satu baris
 *
 * Deskripsi lebih detail tentang fungsi ini. Jelaskan
 * apa yang dilakukan, bagaimana cara kerjanya, dan
 * hal-hal penting yang perlu diketahui pemanggil.
 *
 * Parameter:
 *   param1 - Deskripsi parameter pertama
 *   param2 - Deskripsi parameter kedua
 *
 * Return:
 *   Deskripsi nilai kembalian
 *
 * Contoh:
 *   status_t s = fungsi_nama(nilai1, nilai2);
 *   if (s != STATUS_BERHASIL) {
 *       // handle error
 *   }
 */
status_t fungsi_nama(tipe_t param1, tanda32_t param2);
```

---

## Pengujian

### Build Test

```bash
# Build semua arsitektur
make ARCH=x86 clean all
make ARCH=x86_64 clean all
make ARCH=arm clean all
make ARCH=armv7 clean all
make ARCH=arm64 clean all
```

### Runtime Test

```bash
# Jalankan di QEMU
./ALAT/qemu_run.sh x86 text

# Test dengan debug
./ALAT/debug.sh x86 server
./ALAT/debug.sh x86 gdb
```

### Unit Test

```c
/* test_proses.c */
void test_proses_buat(void)
{
    status_t s = proses_buat("test", 0, 0);
    assert(s == STATUS_BERHASIL);
    
    pid_t pid = proses_cari_nama("test");
    assert(pid != PID_INVALID);
    
    s = proses_hapus(pid);
    assert(s == STATUS_BERHASIL);
}

void test_proses_limit(void)
{
    /* Test batas maksimum proses */
    for (int i = 0; i < MAKS_PROSES + 1; i++) {
        status_t s = proses_buat("test", 0, 0);
        if (i < MAKS_PROSES) {
            assert(s == STATUS_BERHASIL);
        } else {
            assert(s == STATUS_GAGAL);
        }
    }
}
```

---

## Komunikasi

### Saluran Komunikasi

| Saluran | Penggunaan |
|---------|------------|
| GitHub Issues | Bug reports, feature requests |
| GitHub Discussions | Q&A, diskusi umum |
| Pull Request | Code review, diskusi teknis |

### Etika Komunikasi

1. **Cari dulu** - Cek issues/discussions sebelum bertanya
2. **Jelas** - Tulis pertanyaan dengan jelas dan lengkap
3. **Sabar** - Tunggu response, jangan spam
4. **Terima kasih** - Ucapkan terima kasih atas bantuan

---

## Checklist Kontributor

Sebelum submit Pull Request, pastikan:

- [ ] Kode mengikuti Pigura C90
- [ ] Semua nama dalam bahasa Indonesia
- [ ] Maksimal 80 karakter per baris
- [ ] Fungsi bounded (snprintf, strncpy, dll)
- [ ] Error handling lengkap
- [ ] Tidak ada stubs atau TODOs
- [ ] Komentar dalam bahasa Indonesia
- [ ] Build sukses untuk semua arsitektur target
- [ ] Test runtime di QEMU
- [ ] Dokumentasi diperbarui
- [ ] Commit message jelas
- [ ] PR template diisi lengkap

---

## Terima Kasih!

Terima kasih telah berkontribusi pada Pigura OS. Setiap kontribusi, sekecil apapun, sangat berarti bagi perkembangan proyek ini. Bersama-sama kita membangun sistem operasi original Indonesia!

---

*"Pigura - Bingkai Digital"*