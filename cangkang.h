#ifndef SHELL_H
#define SHELL_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>
#include <pty.h>

/* Konstanta untuk shell integration */
#define SHELL_MAX_PATH 256
#define SHELL_MAX_ARGS 64
#define SHELL_BUFFER_SIZE 4096
#define SHELL_MAX_ENV 128

/* Mode shell */
#define SHELL_MODE_NORMAL 0
#define SHELL_MODE_RAW    1

/* Struktur untuk shell manager */
typedef struct {
    pid_t shell_pid;           /* PID proses shell */
    int master_fd;            /* File descriptor master PTY */
    int slave_fd;             /* File descriptor slave PTY */
    char *slave_name;         /* Nama slave PTY */
    char *shell_path;         /* Path ke executable shell */
    char *shell_args[SHELL_MAX_ARGS]; /* Argumen shell */
    int shell_arg_count;       /* Jumlah argumen shell */
    char *env[SHELL_MAX_ENV]; /* Environment variables */
    int env_count;            /* Jumlah environment variables */
    struct termios saved_termios; /* Termios asli */
    struct termios shell_termios; /* Termios untuk shell */
    int mode;                 /* Mode shell saat ini */
    int running;              /* Apakah shell sedang berjalan */
    int exited;               /* Apakah shell sudah keluar */
    int exit_status;          /* Status exit shell */
    int rows;                 /* Jumlah baris terminal */
    int cols;                 /* Jumlah kolom terminal */
    void (*output_callback)(const char *data, int size); /* Callback untuk output */
    void (*exit_callback)(int status);                 /* Callback untuk exit */
} ShellManager;

/* Fungsi utilitas */
static int shell_is_valid_fd(int fd) {
    return fd >= 0;
}

static char* shell_strdup(const char *str) {
    if (!str) return NULL;
    char *copy = malloc(strlen(str) + 1);
    if (copy) strcpy(copy, str);
    return copy;
}

/* Inisialisasi shell manager */
static int shell_init(ShellManager *shell, const char *shell_path) {
    int i;
    
    /* Inisialisasi dengan nilai default */
    shell->shell_pid = -1;
    shell->master_fd = -1;
    shell->slave_fd = -1;
    shell->slave_name = NULL;
    shell->shell_path = NULL;
    shell->shell_arg_count = 0;
    shell->env_count = 0;
    shell->mode = SHELL_MODE_NORMAL;
    shell->running = 0;
    shell->exited = 0;
    shell->exit_status = 0;
    shell->rows = 25;
    shell->cols = 80;
    shell->output_callback = NULL;
    shell->exit_callback = NULL;
    
    /* Reset argumen */
    for (i = 0; i < SHELL_MAX_ARGS; i++) {
        shell->shell_args[i] = NULL;
    }
    
    /* Reset environment */
    for (i = 0; i < SHELL_MAX_ENV; i++) {
        shell->env[i] = NULL;
    }
    
    /* Set shell path */
    if (shell_path) {
        shell->shell_path = shell_strdup(shell_path);
        if (!shell->shell_path) {
            fprintf(stderr, "Memory allocation failed for shell path\n");
            return 0;
        }
    } else {
        /* Gunakan shell default */
        shell->shell_path = shell_strdup("/bin/bash");
        if (!shell->shell_path) {
            fprintf(stderr, "Memory allocation failed for default shell\n");
            return 0;
        }
    }
    
    /* Set argumen default */
    shell->shell_args[0] = shell_strdup(shell->shell_path);
    if (!shell->shell_args[0]) {
        fprintf(stderr, "Memory allocation failed for shell args\n");
        free(shell->shell_path);
        return 0;
    }
    shell->shell_arg_count = 1;
    
    /* Set environment default */
    shell->env[shell->env_count++] = shell_strdup("TERM=xterm-256color");
    shell->env[shell->env_count++] = shell_strdup("COLORTERM=truecolor");
    shell->env[shell->env_count++] = shell_strdup("LC_ALL=C");
    
    /* Simpan termios saat ini */
    if (tcgetattr(STDIN_FILENO, &shell->saved_termios) < 0) {
        perror("tcgetattr failed");
        /* Lanjutkan saja, ini tidak fatal */
    }
    
    return 1;
}

/* Tambahkan argumen shell */
static int shell_add_arg(ShellManager *shell, const char *arg) {
    if (!arg || shell->shell_arg_count >= SHELL_MAX_ARGS - 1) {
        return 0;
    }
    
    shell->shell_args[shell->shell_arg_count] = shell_strdup(arg);
    if (!shell->shell_args[shell->shell_arg_count]) {
        return 0;
    }
    
    shell->shell_arg_count++;
    shell->shell_args[shell->shell_arg_count] = NULL; /* Terminasi NULL */
    
    return 1;
}

/* Set argumen shell */
static int shell_set_args(ShellManager *shell, char *args[], int count) {
    int i;
    
    /* Hapus argumen lama */
    for (i = 0; i < shell->shell_arg_count; i++) {
        if (shell->shell_args[i]) {
            free(shell->shell_args[i]);
            shell->shell_args[i] = NULL;
        }
    }
    
    /* Set argumen baru */
    shell->shell_arg_count = 0;
    for (i = 0; i < count && i < SHELL_MAX_ARGS - 1; i++) {
        if (!shell_add_arg(shell, args[i])) {
            return 0;
        }
    }
    
    return 1;
}

/* Tambahkan environment variable */
static int shell_add_env(ShellManager *shell, const char *env) {
    if (!env || shell->env_count >= SHELL_MAX_ENV - 1) {
        return 0;
    }
    
    shell->env[shell->env_count] = shell_strdup(env);
    if (!shell->env[shell->env_count]) {
        return 0;
    }
    
    shell->env_count++;
    shell->env[shell->env_count] = NULL; /* Terminasi NULL */
    
    return 1;
}

/* Set environment variable */
static int shell_set_env(ShellManager *shell, char *env[], int count) {
    int i;
    
    /* Hapus environment lama */
    for (i = 0; i < shell->env_count; i++) {
        if (shell->env[i]) {
            free(shell->env[i]);
            shell->env[i] = NULL;
        }
    }
    
    /* Set environment baru */
    shell->env_count = 0;
    for (i = 0; i < count && i < SHELL_MAX_ENV - 1; i++) {
        if (!shell_add_env(shell, env[i])) {
            return 0;
        }
    }
    
    return 1;
}

/* Set ukuran terminal */
static void shell_set_size(ShellManager *shell, int rows, int cols) {
    shell->rows = rows;
    shell->cols = cols;
    
    /* Jika shell sudah berjalan, update ukuran PTY */
    if (shell_is_valid_fd(shell->master_fd)) {
        struct winsize ws;
        ws.ws_row = rows;
        ws.ws_col = cols;
        ws.ws_xpixel = 0;
        ws.ws_ypixel = 0;
        
        if (ioctl(shell->master_fd, TIOCSWINSZ, &ws) < 0) {
            perror("ioctl TIOCSWINSZ failed");
        }
    }
}

/* Set mode shell */
static void shell_set_mode(ShellManager *shell, int mode) {
    shell->mode = mode;
    
    /* Jika shell sudah berjalan, update mode PTY */
    if (shell_is_valid_fd(shell->master_fd)) {
        if (mode == SHELL_MODE_RAW) {
            struct termios raw_termios = shell->shell_termios;
            
            /* Set mode raw */
            cfmakeraw(&raw_termios);
            raw_termios.c_oflag |= OPOST; /* Tetap lakukan output processing */
            
            if (tcsetattr(shell->master_fd, TCSANOW, &raw_termios) < 0) {
                perror("tcsetattr raw failed");
            }
        } else {
            /* Kembalikan ke mode normal */
            if (tcsetattr(shell->master_fd, TCSANOW, &shell->shell_termios) < 0) {
                perror("tcsetattr normal failed");
            }
        }
    }
}

/* Set callback untuk output */
static void shell_set_output_callback(ShellManager *shell, void (*callback)(const char *, int)) {
    shell->output_callback = callback;
}

/* Set callback untuk exit */
static void shell_set_exit_callback(ShellManager *shell, void (*callback)(int)) {
    shell->exit_callback = callback;
}

/* Buat pseudoterminal */
static int shell_create_pty(ShellManager *shell) {
    /* Buat master PTY */
    shell->master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (!shell_is_valid_fd(shell->master_fd)) {
        perror("posix_openpt failed");
        return 0;
    }
    
    /* Grant access ke slave PTY */
    if (grantpt(shell->master_fd) < 0) {
        perror("grantpt failed");
        close(shell->master_fd);
        shell->master_fd = -1;
        return 0;
    }
    
    /* Unlock slave PTY */
    if (unlockpt(shell->master_fd) < 0) {
        perror("unlockpt failed");
        close(shell->master_fd);
        shell->master_fd = -1;
        return 0;
    }
    
    /* Dapatkan nama slave PTY */
    shell->slave_name = ptsname(shell->master_fd);
    if (!shell->slave_name) {
        perror("ptsname failed");
        close(shell->master_fd);
        shell->master_fd = -1;
        return 0;
    }
    
    return 1;
}

/* Jalankan shell */
static int shell_run(ShellManager *shell) {
    pid_t pid;
    
    /* Buat PTY */
    if (!shell_create_pty(shell)) {
        return 0;
    }
    
    /* Fork proses */
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        close(shell->master_fd);
        shell->master_fd = -1;
        return 0;
    }
    
    if (pid == 0) { /* Child process */
        /* Buat sesi baru */
        if (setsid() < 0) {
            perror("setsid failed");
            exit(1);
        }
        
        /* Buka slave PTY */
        shell->slave_fd = open(shell->slave_name, O_RDWR);
        if (!shell_is_valid_fd(shell->slave_fd)) {
            perror("open slave failed");
            exit(1);
        }
        
        /* Jadikan slave PTY sebagai controlling terminal */
        if (ioctl(shell->slave_fd, TIOCSCTTY, 0) < 0) {
            perror("TIOCSCTTY failed");
            exit(1);
        }
        
        /* Duplikasi file descriptors */
        if (dup2(shell->slave_fd, STDIN_FILENO) < 0 ||
            dup2(shell->slave_fd, STDOUT_FILENO) < 0 ||
            dup2(shell->slave_fd, STDERR_FILENO) < 0) {
            perror("dup2 failed");
            exit(1);
        }
        
        /* Tutup file descriptors yang tidak perlu */
        close(shell->slave_fd);
        close(shell->master_fd);
        
        /* Set environment variables */
        for (int i = 0; shell->env[i]; i++) {
            putenv(shell->env[i]);
        }
        
        /* Set ukuran terminal */
        struct winsize ws;
        ws.ws_row = shell->rows;
        ws.ws_col = shell->cols;
        ws.ws_xpixel = 0;
        ws.ws_ypixel = 0;
        
        if (ioctl(STDIN_FILENO, TIOCSWINSZ, &ws) < 0) {
            perror("ioctl TIOCSWINSZ failed");
            exit(1);
        }
        
        /* Jalankan shell */
        execvp(shell->shell_path, shell->shell_args);
        
        /* Jika execvp gagal */
        perror("execvp failed");
        exit(1);
    } else { /* Parent process */
        shell->shell_pid = pid;
        shell->running = 1;
        shell->exited = 0;
        
        /* Tutup slave PTY di parent */
        /* (slave PTY sudah ditutup di child) */
        
        /* Set mode master PTY */
        if (tcgetattr(shell->master_fd, &shell->shell_termios) < 0) {
            perror("tcgetattr failed");
            /* Lanjutkan saja, ini tidak fatal */
        }
        
        /* Set non-blocking I/O */
        if (fcntl(shell->master_fd, F_SETFL, O_NONBLOCK) < 0) {
            perror("fcntl O_NONBLOCK failed");
            /* Lanjutkan saja, ini tidak fatal */
        }
        
        /* Set mode shell */
        shell_set_mode(shell, shell->mode);
        
        /* Set ukuran terminal */
        shell_set_size(shell, shell->rows, shell->cols);
        
        return 1;
    }
}

/* Baca output dari shell */
static int shell_read(ShellManager *shell, char *buffer, int size) {
    if (!shell_is_valid_fd(shell->master_fd) || !shell->running) {
        return 0;
    }
    
    ssize_t n = read(shell->master_fd, buffer, size);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; /* Tidak ada data tersedia */
        }
        perror("read failed");
        return -1;
    } else if (n == 0) {
        /* Shell keluar */
        shell->running = 0;
        shell->exited = 1;
        
        /* Dapatkan status exit */
        if (waitpid(shell->shell_pid, &shell->exit_status, WNOHANG) < 0) {
            perror("waitpid failed");
        }
        
        /* Panggil callback jika ada */
        if (shell->exit_callback) {
            shell->exit_callback(shell->exit_status);
        }
        
        return 0;
    }
    
    /* Panggil callback jika ada */
    if (shell->output_callback) {
        shell->output_callback(buffer, n);
    }
    
    return n;
}

/* Tulis input ke shell */
static int shell_write(ShellManager *shell, const char *buffer, int size) {
    if (!shell_is_valid_fd(shell->master_fd) || !shell->running) {
        return 0;
    }
    
    ssize_t n = write(shell->master_fd, buffer, size);
    if (n < 0) {
        perror("write failed");
        return -1;
    }
    
    return n;
}

/* Kirim sinyal ke shell */
static int shell_signal(ShellManager *shell, int signum) {
    if (!shell_is_valid_fd(shell->master_fd) || !shell->running) {
        return 0;
    }
    
    /* Kirim sinyal ke proses shell */
    if (kill(shell->shell_pid, signum) < 0) {
        perror("kill failed");
        return 0;
    }
    
    return 1;
}

/* Cek apakah shell sedang berjalan */
static int shell_is_running(ShellManager *shell) {
    return shell->running;
}

/* Cek apakah shell sudah keluar */
static int shell_has_exited(ShellManager *shell) {
    return shell->exited;
}

/* Dapatkan status exit shell */
static int shell_get_exit_status(ShellManager *shell) {
    return shell->exit_status;
}

/* Dapatkan PID shell */
static pid_t shell_get_pid(ShellManager *shell) {
    return shell->shell_pid;
}

/* Dapatkan file descriptor master PTY */
static int shell_get_master_fd(ShellManager *shell) {
    return shell->master_fd;
}

/* Tunggu shell selesai */
static int shell_wait(ShellManager *shell, int options) {
    if (!shell->running || shell->exited) {
        return shell->exit_status;
    }
    
    pid_t pid = waitpid(shell->shell_pid, &shell->exit_status, options);
    if (pid < 0) {
        perror("waitpid failed");
        return -1;
    }
    
    if (pid == shell->shell_pid) {
        shell->running = 0;
        shell->exited = 1;
        
        /* Panggil callback jika ada */
        if (shell->exit_callback) {
            shell->exit_callback(shell->exit_status);
        }
        
        return shell->exit_status;
    }
    
    return -1; /* Masih berjalan */
}

/* Paksa shell keluar */
static int shell_kill(ShellManager *shell, int signum) {
    if (!shell->running || shell->exited) {
        return 0;
    }
    
    /* Kirim sinyal ke proses shell */
    if (kill(shell->shell_pid, signum) < 0) {
        perror("kill failed");
        return 0;
    }
    
    /* Tunggu proses selesai */
    if (waitpid(shell->shell_pid, &shell->exit_status, 0) < 0) {
        perror("waitpid failed");
        return 0;
    }
    
    shell->running = 0;
    shell->exited = 1;
    
    /* Panggil callback jika ada */
    if (shell->exit_callback) {
        shell->exit_callback(shell->exit_status);
    }
    
    return 1;
}

/* Reset shell manager */
static void shell_reset(ShellManager *shell) {
    /* Jika shell sedang berjalan, hentikan */
    if (shell->running && !shell->exited) {
        shell_kill(shell, SIGTERM);
    }
    
    /* Tutup file descriptors */
    if (shell_is_valid_fd(shell->master_fd)) {
        close(shell->master_fd);
        shell->master_fd = -1;
    }
    
    if (shell_is_valid_fd(shell->slave_fd)) {
        close(shell->slave_fd);
        shell->slave_fd = -1;
    }
    
    /* Free memory */
    if (shell->slave_name) {
        /* slave_name tidak perlu di-free karena milik ptsname */
        shell->slave_name = NULL;
    }
    
    if (shell->shell_path) {
        free(shell->shell_path);
        shell->shell_path = NULL;
    }
    
    for (int i = 0; i < shell->shell_arg_count; i++) {
        if (shell->shell_args[i]) {
            free(shell->shell_args[i]);
            shell->shell_args[i] = NULL;
        }
    }
    shell->shell_arg_count = 0;
    
    for (int i = 0; i < shell->env_count; i++) {
        if (shell->env[i]) {
            free(shell->env[i]);
            shell->env[i] = NULL;
        }
    }
    shell->env_count = 0;
    
    /* Reset state */
    shell->shell_pid = -1;
    shell->running = 0;
    shell->exited = 0;
    shell->exit_status = 0;
}

/* Bebas memori shell manager */
static void shell_free(ShellManager *shell) {
    shell_reset(shell);
    
    /* Kembalikan termios asli */
    if (tcsetattr(STDIN_FILENO, TCSANOW, &shell->saved_termios) < 0) {
        perror("tcsetattr restore failed");
        /* Lanjutkan saja, ini tidak fatal */
    }
}

#endif /* SHELL_H */
