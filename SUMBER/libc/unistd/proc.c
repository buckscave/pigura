/*
 * PIGURA LIBC - UNISTD/PROC.C
 * ============================
 * Implementasi fungsi manajemen proses POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 2.0 - Disinkronkan dengan syscall Pigura OS
 *
 * Fungsi yang diimplementasikan:
 * - getpid()   - Mendapatkan ID proses
 * - getppid()  - Mendapatkan ID proses parent
 * - getuid()   - Mendapatkan user ID real
 * - geteuid()  - Mendapatkan user ID efektif
 * - getgid()   - Mendapatkan group ID real
 * - getegid()  - Mendapatkan group ID efektif
 * - setuid()   - Mengatur user ID
 * - setgid()   - Mengatur group ID
 * - fork()     - Membuat proses baru
 * - execve()   - Eksekusi program
 * - execv()    - Eksekusi dengan argumen vektor
 * - execvp()   - Eksekusi dengan pencarian PATH
 * - execvpe()  - Eksekusi dengan PATH dan environment
 */

#include <sys/types.h>
#include <sys/syscall.h>   /* Syscall numbers dari header terpusat */
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================
 * FUNGSI HELPER
 * ============================================================
 */

/*
 * __set_errno - Set errno berdasarkan error code syscall
 *
 * Parameter:
 *   result - Hasil syscall (negatif = error)
 *
 * Return: -1 untuk menandakan error
 */
static int __set_errno(long result) {
    if (result < 0 && result >= -4095) {
        errno = (int)(-result);
        return -1;
    }
    return (int)result;
}

/*
 * __set_errno_pid - Set errno untuk hasil pid_t
 *
 * Parameter:
 *   result - Hasil syscall (negatif = error)
 *
 * Return: -1 untuk menandakan error
 */
static pid_t __set_errno_pid(long result) {
    if (result < 0 && result >= -4095) {
        errno = (int)(-result);
        return (pid_t)-1;
    }
    return (pid_t)result;
}

/*
 * __set_errno_uid - Set errno untuk hasil uid_t/gid_t
 *
 * Parameter:
 *   result - Hasil syscall (negatif = error)
 *
 * Return: -1 untuk menandakan error
 */
static uid_t __set_errno_uid(long result) {
    if (result < 0 && result >= -4095) {
        errno = (int)(-result);
        return (uid_t)-1;
    }
    return (uid_t)result;
}

/* ============================================================
 * IMPLEMENTASI FUNGSI GET ID
 * ============================================================
 */

/*
 * getpid - Mendapatkan ID proses saat ini
 *
 * Return: Process ID proses yang memanggil
 */
pid_t getpid(void) {
    long result;

    result = syscall0(SYS_GETPID);

    /* getpid tidak pernah gagal */
    return (pid_t)result;
}

/*
 * getppid - Mendapatkan ID proses parent
 *
 * Return: Process ID dari parent proses
 */
pid_t getppid(void) {
    long result;

    result = syscall0(SYS_GETPPID);

    /* getppid tidak pernah gagal */
    return (pid_t)result;
}

/*
 * getuid - Mendapatkan user ID real
 *
 * Return: Real user ID proses
 */
uid_t getuid(void) {
    long result;

    result = syscall0(SYS_GETUID);

    /* getuid tidak pernah gagal */
    return (uid_t)result;
}

/*
 * geteuid - Mendapatkan effective user ID
 *
 * Return: Effective user ID proses
 *
 * Catatan: Pigura OS menggunakan single UID, jadi geteuid = getuid
 */
uid_t geteuid(void) {
    /* Pigura OS: single UID model */
    return getuid();
}

/*
 * getgid - Mendapatkan group ID real
 *
 * Return: Real group ID proses
 */
gid_t getgid(void) {
    long result;

    result = syscall0(SYS_GETGID);

    /* getgid tidak pernah gagal */
    return (gid_t)result;
}

/*
 * getegid - Mendapatkan effective group ID
 *
 * Return: Effective group ID proses
 *
 * Catatan: Pigura OS menggunakan single GID, jadi getegid = getgid
 */
gid_t getegid(void) {
    /* Pigura OS: single GID model */
    return getgid();
}

/* ============================================================
 * IMPLEMENTASI FUNGSI SET ID
 * ============================================================
 */

/*
 * setuid - Mengatur user ID efektif
 *
 * Parameter:
 *   uid - User ID baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int setuid(uid_t uid) {
    long result;

    result = syscall1(SYS_SETUID, (long)uid);

    return __set_errno(result);
}

/*
 * setgid - Mengatur group ID efektif
 *
 * Parameter:
 *   gid - Group ID baru
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int setgid(gid_t gid) {
    long result;

    result = syscall1(SYS_SETGID, (long)gid);

    return __set_errno(result);
}

/* ============================================================
 * IMPLEMENTASI FUNGSI FORK
 * ============================================================
 */

/*
 * fork - Membuat proses baru dengan menyalin proses saat ini
 *
 * Return: 0 di child process, PID child di parent, -1 jika error
 */
pid_t fork(void) {
    long result;

    result = syscall0(SYS_FORK);

    return __set_errno_pid(result);
}

/* ============================================================
 * IMPLEMENTASI FUNGSI EXEC
 * ============================================================
 */

/*
 * execve - Eksekusi program baru
 *
 * Parameter:
 *   pathname - Path ke file executable
 *   argv     - Array argumen (terminated dengan NULL)
 *   envp     - Array environment (terminated dengan NULL)
 *
 * Return: -1 jika error (tidak kembali jika berhasil)
 */
int execve(const char *pathname, char *const argv[],
           char *const envp[]) {
    long result;

    /* Validasi parameter */
    if (pathname == NULL || argv == NULL) {
        errno = EINVAL;
        return -1;
    }

    result = syscall3(SYS_EXEC, (long)pathname,
                      (long)argv, (long)envp);

    /* Jika sampai di sini, execve gagal */
    return __set_errno(result);
}

/*
 * execv - Eksekusi program dengan argumen vektor
 *
 * Parameter:
 *   pathname - Path ke file executable
 *   argv     - Array argumen (terminated dengan NULL)
 *
 * Return: -1 jika error
 *
 * Catatan: Menggunakan environment saat ini
 */
int execv(const char *pathname, char *const argv[]) {
    /* Deklarasi environ - external variable */
    extern char **environ;

    /* Validasi parameter */
    if (pathname == NULL || argv == NULL) {
        errno = EINVAL;
        return -1;
    }

    return execve(pathname, argv, environ);
}

/*
 * __find_in_path - Mencari executable dalam PATH
 *
 * Parameter:
 *   file    - Nama file yang dicari
 *   pathbuf - Buffer untuk hasil path lengkap
 *   bufsize - Ukuran buffer
 *
 * Return: 0 jika ditemukan, -1 jika tidak
 */
static int __find_in_path(const char *file, char *pathbuf,
                          size_t bufsize) {
    char *path_env;
    char *path_copy;
    char *dir;
    size_t file_len;
    size_t dir_len;
    char *saveptr;

    /* Dapatkan PATH dari environment */
    path_env = getenv("PATH");
    if (path_env == NULL) {
        /* PATH default */
        path_env = "/bin:/usr/bin";
    }

    /* Salin PATH karena strtok_r memodifikasi string */
    path_copy = strdup(path_env);
    if (path_copy == NULL) {
        errno = ENOMEM;
        return -1;
    }

    file_len = strlen(file);

    /* Iterasi setiap direktori di PATH */
    dir = strtok_r(path_copy, ":", &saveptr);
    while (dir != NULL) {
        dir_len = strlen(dir);

        /* Cek apakah buffer cukup */
        if (dir_len + 1 + file_len + 1 <= bufsize) {
            /* Bentuk path lengkap */
            memcpy(pathbuf, dir, dir_len);
            pathbuf[dir_len] = '/';
            memcpy(pathbuf + dir_len + 1, file, file_len + 1);

            /* Cek apakah file executable */
            if (access(pathbuf, X_OK) == 0) {
                free(path_copy);
                return 0;
            }
        }

        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(path_copy);
    errno = ENOENT;
    return -1;
}

/*
 * execvp - Eksekusi program dengan pencarian PATH
 *
 * Parameter:
 *   file - Nama file executable (atau path)
 *   argv - Array argumen (terminated dengan NULL)
 *
 * Return: -1 jika error
 *
 * Catatan: Jika file mengandung '/', tidak mencari di PATH
 */
int execvp(const char *file, char *const argv[]) {
    char pathbuf[1024];
    extern char **environ;

    /* Validasi parameter */
    if (file == NULL || argv == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Jika file mengandung '/', gunakan langsung */
    if (strchr(file, '/') != NULL) {
        return execve(file, argv, environ);
    }

    /* Cari file di PATH */
    if (__find_in_path(file, pathbuf, sizeof(pathbuf)) != 0) {
        return -1;
    }

    return execve(pathbuf, argv, environ);
}

/*
 * execvpe - Eksekusi dengan PATH dan environment kustom
 *
 * Parameter:
 *   file - Nama file executable (atau path)
 *   argv - Array argumen (terminated dengan NULL)
 *   envp - Array environment (terminated dengan NULL)
 *
 * Return: -1 jika error
 *
 * Catatan: Menggunakan envp sebagai environment baru
 */
int execvpe(const char *file, char *const argv[], char *const envp[]) {
    char pathbuf[1024];
    char *old_path;
    char *path_copy;
    int result;

    /* Validasi parameter */
    if (file == NULL || argv == NULL || envp == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Jika file mengandung '/', gunakan langsung */
    if (strchr(file, '/') != NULL) {
        return execve(file, argv, envp);
    }

    /* Simpan PATH lama */
    old_path = getenv("PATH");

    /* Cari PATH di envp */
    path_copy = NULL;
    {
        int i;
        for (i = 0; envp[i] != NULL; i++) {
            if (strncmp(envp[i], "PATH=", 5) == 0) {
                path_copy = strdup(envp[i] + 5);
                break;
            }
        }
    }

    /* Set PATH sementara untuk __find_in_path */
    if (path_copy != NULL) {
        setenv("PATH", path_copy, 1);
    }

    /* Cari file di PATH */
    result = __find_in_path(file, pathbuf, sizeof(pathbuf));

    /* Kembalikan PATH lama jika ada */
    if (old_path != NULL) {
        setenv("PATH", old_path, 1);
    }
    if (path_copy != NULL) {
        free(path_copy);
    }

    if (result != 0) {
        return -1;
    }

    return execve(pathbuf, argv, envp);
}
