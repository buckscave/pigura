/* ----------------------------------------------------------------------------------------------
 * pigura inti: dialog_file.c - dialog file native
 * ----------------------------------------------------------------------------------------------
 * Implementasi pg_dialog_file() untuk berbagai platform:
 *
 *   HAVE_X11 / POSIX: panggil zenity atau kdialog via popen().
 *                     Zenity dipakai lebih dulu (umum di GNOME/X11);
 *                     bila tidak ada, fallback ke kdialog (KDE).
 *                     Bila dua-duanya tidak ada, return NULL +
 *                     PG_GALAT_TANPA.
 *
 *   _WIN32:           GetOpenFileName / GetSaveFileName dari
 *                     comdlg32. (Stub minimal untuk kompilasi silang;
 *                     implementasi penuh menggunakan OPENFILENAMEW.)
 *
 *   Fallback:         return NULL + PG_GALAT_TANPA.
 *
 * Path yang dibaca dari popen() di-trim newline + whitespace.
 * Buffer statis cukup untuk PATH_MAX (4096). Return path malloc'd
 * dengan panjang tepat; caller wajib free().
 *
 * Catatan keamanan: judul + filter di-escape sederhana (single
 * quote ditutup) supaya injeksi shell tidak trivial. Tidak ada
 * proteksi penuh — penggunaan dialog file adalah input user
 * lokal, sehingga risiko injeksi terbatas pada pengguna sendiri.
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/dialog_file.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>
#include <string.h>

/* POSIX detection: pigura selalu define _POSIX_C_SOURCE, jadi cek
 * itu. Win32 juga define _POSIX_ secara default jadi pastikan
 * mengecualikan _WIN32. */
#if (defined(_POSIX_C_SOURCE) || defined(HAVE_X11) || \
     defined(__unix__) || defined(__linux__)) && !defined(_WIN32)
#  include <stdio.h>
#  define PG_DIALOG_HAVE_POSIX 1
#endif

#if defined(_WIN32)
#  include <windows.h>
/* comdlg32 untuk GetOpenFileName. */
#  include <commdlg.h>
#endif

/* Batas panjang path yang dibaca. */
#define PG_DIALOG_PATH_MAX 4096

/* Helper: cek apakah executable tersedia di PATH. */
#ifdef PG_DIALOG_HAVE_POSIX
static pg_bool pg_which(const char *prog)
{
        char cmd[128];
        FILE *fp;
        int rc;
        /* `command -v` POSIX. */
        snprintf(cmd, sizeof(cmd), "command -v %s >/dev/null 2>&1",
                  prog);
        fp = popen(cmd, "r");
        if (!fp) return PG_SALAH;
        rc = pclose(fp);
        /* rc == 0 berarti command berhasil = program ditemukan. */
        return (rc == 0) ? PG_BENAR : PG_SALAH;
}

/* Helper: baca satu baris dari fp, trim whitespace + newline. */
static int pg_baca_path(FILE *fp, char *out, int maks)
{
        int n = 0;
        int c;
        /* Baca sampai newline atau EOF. */
        while ((c = fgetc(fp)) != EOF && c != '\n' && c != '\r') {
                if (n < maks - 1) out[n++] = (char)c;
        }
        /* Trim trailing whitespace. */
        while (n > 0 && (out[n-1] == ' ' || out[n-1] == '\t'))
                n--;
        out[n] = '\0';
        return n;
}

/* Escape single-quote dalam string: ganti ' -> '"'"'
 * Output ke buf, return panjang. Tidak overflow bila buf cukup. */
static int pg_escape_sh(const char *in, char *out, int maks)
{
        int n = 0;
        char ch;
        const char *p;
        if (!in) {
                if (maks > 0) out[0] = '\0';
                return 0;
        }
        for (p = in; *p; p++) {
                ch = *p;
                if (ch == '\'') {
                        /* ' -> '"'"'  (5 char) */
                        const char *seq = "'\"'\"'";
                        int i;
                        for (i = 0; seq[i]; i++) {
                                if (n < maks - 1) out[n++] = seq[i];
                        }
                } else {
                        if (n < maks - 1) out[n++] = ch;
                }
        }
        out[n] = '\0';
        return n;
}
#endif /* PG_DIALOG_HAVE_POSIX */

#ifdef PG_DIALOG_HAVE_POSIX
/* Jalankan zenity atau kdialog, baca path, trim, return malloc'd.
 * cmd_full: command shell lengkap (sudah di-escape). */
static char *pg_dialog_posix(const char *cmd_full)
{
        FILE *fp;
        char  buf[PG_DIALOG_PATH_MAX];
        int   n;

        fp = popen(cmd_full, "r");
        if (!fp) {
                pg_set_galat(PG_GALAT_IO,
                        "dialog_file: popen gagal - %s",
                        cmd_full);
                return NULL;
        }
        n = pg_baca_path(fp, buf, (int)sizeof(buf));
        pclose(fp);
        if (n <= 0) {
                /* User cancel atau dialog gagal. */
                pg_set_galat(PG_GALAT_TIDAKADA,
                        "dialog_file: dibatalkan");
                return NULL;
        }
        return strdup(buf);
}
#endif

char *pg_dialog_file(pg_layar_t *layar, pg_dialog_tipe_t tipe,
                      const char *judul, const char *filter)
{
        (void)layar; /* X11 parent tidak diperlukan untuk zenity. */

        if (tipe != PG_DIALOG_BUKA && tipe != PG_DIALOG_SIMPAN) {
                pg_set_galat(PG_GALAT_ARGUMEN,
                        "dialog_file: tipe invalid %d", (int)tipe);
                return NULL;
        }

#ifdef PG_DIALOG_HAVE_POSIX
        /* Coba zenity lebih dulu, lalu kdialog. */
        if (pg_which("zenity") == PG_BENAR) {
                char  cmd[1024];
                char  judul_esc[256];
                char  filter_part[256];
                const char *save_flag;
                const char *judul_arg;

                save_flag = (tipe == PG_DIALOG_SIMPAN) ?
                        "--save" : "";
                judul_arg = judul ? judul : "Pilih file";
                pg_escape_sh(judul_arg, judul_esc,
                              (int)sizeof(judul_esc));
                filter_part[0] = '\0';
                if (filter && *filter) {
                        /* zenity --file-filter=Name | *.ext1 *.ext2 */
                        char f_esc[200];
                        pg_escape_sh(filter, f_esc,
                                      (int)sizeof(f_esc));
                        /* Filter format zenity: --file-filter='*.png' */
                        /* Sederhana: bila ada comma, ganti ke
                         * multiple --file-filter. Untuk minimal,
                         * tampilkan satu filter "*.ext". */
                        snprintf(filter_part,
                                  sizeof(filter_part),
                                  "--file-filter='%s|%s'",
                                  f_esc, f_esc);
                }
                snprintf(cmd, sizeof(cmd),
                          "zenity --file-selection %s --title='%s' %s "
                          "2>/dev/null",
                          save_flag, judul_esc, filter_part);
                return pg_dialog_posix(cmd);
        }

        if (pg_which("kdialog") == PG_BENAR) {
                char  cmd[1024];
                char  judul_esc[256];
                char  filter_part[256];
                const char *save_flag;
                const char *judul_arg;

                save_flag = (tipe == PG_DIALOG_SIMPAN) ?
                        "--getsavefilename" :
                        "--getopenfilename";
                judul_arg = judul ? judul : "Pilih file";
                pg_escape_sh(judul_arg, judul_esc,
                              (int)sizeof(judul_esc));
                filter_part[0] = '\0';
                if (filter && *filter) {
                        char f_esc[200];
                        pg_escape_sh(filter, f_esc,
                                      (int)sizeof(f_esc));
                        snprintf(filter_part, sizeof(filter_part),
                                  "%s", f_esc);
                }
                snprintf(cmd, sizeof(cmd),
                          "kdialog %s . '%s' %s 2>/dev/null",
                          save_flag, judul_esc, filter_part);
                return pg_dialog_posix(cmd);
        }

        pg_set_galat(PG_GALAT_TANPA,
                "dialog_file: tidak tersedia "
                "(pasang zenity atau kdialog)");
        return NULL;

#elif defined(_WIN32)
        {
                OPENFILENAMEA ofn;
                char buf[PG_DIALOG_PATH_MAX];
                const char *default_ext;
                BOOL ok;

                memset(&ofn, 0, sizeof(ofn));
                buf[0] = '\0';
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = NULL;
                ofn.lpstrFile = buf;
                ofn.nMaxFile = (DWORD)sizeof(buf);
                ofn.lpstrTitle = judul ? judul : NULL;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
                if (tipe == PG_DIALOG_SIMPAN) {
                        ofn.Flags |= OFN_OVERWRITEPROMPT;
                        default_ext = (filter && *filter) ?
                                filter : "txt";
                        ofn.lpstrDefExt = default_ext;
                        ok = GetSaveFileNameA(&ofn);
                } else {
                        ofn.Flags |= OFN_FILEMUSTEXIST;
                        ok = GetOpenFileNameA(&ofn);
                }
                if (!ok) return NULL;
                return strdup(buf);
        }
#else
        pg_set_galat(PG_GALAT_TANPA,
                "dialog_file: tidak tersedia di platform ini");
        return NULL;
#endif
}
