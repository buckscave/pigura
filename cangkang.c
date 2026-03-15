/*******************************************************************************
 * CANGKANG.C - Implementasi Shell Manager
 *
 * Modul ini menangani eksekusi shell dengan PTY (Pseudo Terminal).
 *
 * Copyright (c) 2024 Pigura OS Project
 ******************************************************************************/

#include "pigura.h"

/*******************************************************************************
 * KONSTANTA INTERNAL
 ******************************************************************************/

#define CANGKANG_BUFFER_BACA   4096
#define CANGKANG_DEFAULT_SHELL "/bin/sh"

/*******************************************************************************
 * FUNGSI INTERNAL
 ******************************************************************************/

/* Set termios ke mode raw */
static void cangkang_set_raw_mode(struct termios *tio) {
    if (!tio) {
        return;
    }
    
    tio->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | 
                      IGNCR | ICRNL | IXON);
    tio->c_oflag &= ~OPOST;
    tio->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tio->c_cflag &= ~(CSIZE | PARENB);
    tio->c_cflag |= CS8;
    
    tio->c_cc[VMIN] = 1;
    tio->c_cc[VTIME] = 0;
}

/* Wait handler untuk SIGCHLD */
static void cangkang_sigchld_handler(int sig) {
    (void)sig;
    /* Handler kosong, hanya untuk interrupt select() */
}

/*******************************************************************************
 * FUNGSI PUBLIK
 ******************************************************************************/

/* Inisialisasi manager shell */
int cangkang_init(ManagerCangkang *shell, const char *jalur_shell) {
    
    if (!shell) {
        return 0;
    }
    
    /* Inisialisasi struktur */
    memset(shell, 0, sizeof(ManagerCangkang));
    
    shell->pid = -1;
    shell->fd_master = -1;
    shell->fd_slave = -1;
    shell->nama_slave = NULL;
    shell->mode = CANGKANG_MODE_NORMAL;
    shell->berjalan = 0;
    shell->selesai = 0;
    shell->status_keluar = 0;
    shell->baris = TERMINAL_BARIS_DEFAULT;
    shell->kolom = TERMINAL_KOLOM_DEFAULT;
    shell->jumlah_arg = 0;
    shell->jumlah_lingk = 0;
    
    /* Set shell default */
    shell->jalur_shell = pigura_strdup(jalur_shell ? jalur_shell : 
                                       CANGKANG_DEFAULT_SHELL);
    if (!shell->jalur_shell) {
        return 0;
    }
    
    /* Inisialisasi argumen dengan shell name */
    shell->arg[0] = pigura_strdup(shell->jalur_shell);
    if (!shell->arg[0]) {
        free(shell->jalur_shell);
        return 0;
    }
    shell->jumlah_arg = 1;
    
    /* Inisialisasi termios */
    if (tcgetattr(STDIN_FILENO, &shell->termios_simpan) == 0) {
        shell->termios_shell = shell->termios_simpan;
        cangkang_set_raw_mode(&shell->termios_shell);
    }
    
    return 1;
}

/* Tambahkan argumen shell */
int cangkang_tambah_arg(ManagerCangkang *shell, const char *arg) {
    if (!shell || !arg) {
        return 0;
    }
    
    if (shell->jumlah_arg >= CANGKANG_ARG_MAKS - 1) {
        return 0;
    }
    
    shell->arg[shell->jumlah_arg] = pigura_strdup(arg);
    if (!shell->arg[shell->jumlah_arg]) {
        return 0;
    }
    
    shell->jumlah_arg++;
    return 1;
}

/* Set argumen shell */
int cangkang_set_arg(ManagerCangkang *shell, char *arg[], int jumlah) {
    int i;
    
    if (!shell || !arg || jumlah <= 0) {
        return 0;
    }
    
    /* Bebaskan argumen lama */
    for (i = 0; i < shell->jumlah_arg; i++) {
        if (shell->arg[i]) {
            free(shell->arg[i]);
            shell->arg[i] = NULL;
        }
    }
    shell->jumlah_arg = 0;
    
    /* Salin argumen baru */
    for (i = 0; i < jumlah && i < CANGKANG_ARG_MAKS - 1; i++) {
        if (!cangkang_tambah_arg(shell, arg[i])) {
            return 0;
        }
    }
    
    return 1;
}

/* Tambahkan environment variable */
int cangkang_tambah_lingk(ManagerCangkang *shell, const char *lingk) {
    if (!shell || !lingk) {
        return 0;
    }
    
    if (shell->jumlah_lingk >= CANGKANG_LINGK_MAKS - 1) {
        return 0;
    }
    
    shell->lingkungan[shell->jumlah_lingk] = pigura_strdup(lingk);
    if (!shell->lingkungan[shell->jumlah_lingk]) {
        return 0;
    }
    
    shell->jumlah_lingk++;
    return 1;
}

/* Set ukuran terminal */
void cangkang_set_ukuran(ManagerCangkang *shell, int baris, int kolom) {
    if (!shell) {
        return;
    }
    
    shell->baris = baris;
    shell->kolom = kolom;
    
    /* Update ukuran PTY jika sudah terbuka */
    if (shell->fd_master >= 0) {
        struct winsize ws;
        memset(&ws, 0, sizeof(ws));
        ws.ws_row = (unsigned short)baris;
        ws.ws_col = (unsigned short)kolom;
        ws.ws_xpixel = (unsigned short)(kolom * HURUF_LEBAR);
        ws.ws_ypixel = (unsigned short)(baris * HURUF_TINGGI);
        ioctl(shell->fd_master, TIOCSWINSZ, &ws);
    }
}

/* Set mode shell */
void cangkang_set_mode(ManagerCangkang *shell, int mode) {
    if (shell) {
        shell->mode = mode;
    }
}

/* Set callback output */
void cangkang_set_callback_output(ManagerCangkang *shell,
                                   void (*callback)(const char*, int, void*),
                                   void *user) {
    if (shell) {
        shell->callback_output = callback;
        shell->user_data = user;
    }
}

/* Set callback keluar */
void cangkang_set_callback_keluar(ManagerCangkang *shell,
                                   void (*callback)(int, void*), void *user) {
    if (shell) {
        shell->callback_keluar = callback;
        shell->user_data = user;
    }
}

/* Jalankan shell */
int cangkang_jalankan(ManagerCangkang *shell) {
    int master_fd, slave_fd;
    char *slave_name;
    pid_t pid;
    struct winsize ws;
    struct sigaction sa;
    
    if (!shell || shell->berjalan) {
        return 0;
    }
    
    /* Buka PTY master */
    master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd < 0) {
        return 0;
    }
    
    /* Grant dan unlock PTY */
    if (grantpt(master_fd) < 0 || unlockpt(master_fd) < 0) {
        close(master_fd);
        return 0;
    }
    
    /* Dapatkan nama slave */
    slave_name = ptsname(master_fd);
    if (!slave_name) {
        close(master_fd);
        return 0;
    }
    
    /* Buka PTY slave */
    slave_fd = open(slave_name, O_RDWR);
    if (slave_fd < 0) {
        close(master_fd);
        return 0;
    }
    
    /* Set ukuran window */
    memset(&ws, 0, sizeof(ws));
    ws.ws_row = (unsigned short)shell->baris;
    ws.ws_col = (unsigned short)shell->kolom;
    ws.ws_xpixel = (unsigned short)(shell->kolom * HURUF_LEBAR);
    ws.ws_ypixel = (unsigned short)(shell->baris * HURUF_TINGGI);
    ioctl(master_fd, TIOCSWINSZ, &ws);
    
    /* Fork proses */
    pid = fork();
    if (pid < 0) {
        close(slave_fd);
        close(master_fd);
        return 0;
    }
    
    if (pid == 0) {
        /* Proses anak */
        int i;
        
        /* Tutup master */
        close(master_fd);
        
        /* Buat slave sebagai controlling terminal */
        if (setsid() < 0) {
            _exit(1);
        }
        
        /* Duplikasi fd slave */
        if (dup2(slave_fd, STDIN_FILENO) < 0 ||
            dup2(slave_fd, STDOUT_FILENO) < 0 ||
            dup2(slave_fd, STDERR_FILENO) < 0) {
            _exit(1);
        }
        
        /* Tutup fd slave asli jika berbeda */
        if (slave_fd > STDERR_FILENO) {
            close(slave_fd);
        }
        
        /* Set environment */
        for (i = 0; i < shell->jumlah_lingk; i++) {
            if (shell->lingkungan[i]) {
                putenv(shell->lingkungan[i]);
            }
        }
        
        /* Set TERM */
        setenv("TERM", "xterm-256color", 1);
        
        /* Set ukuran terminal di environment */
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", shell->kolom);
            setenv("COLUMNS", buf, 1);
            snprintf(buf, sizeof(buf), "%d", shell->baris);
            setenv("LINES", buf, 1);
        }
        
        /* Terminasi array argumen */
        shell->arg[shell->jumlah_arg] = NULL;
        
        /* Eksekusi shell */
        execv(shell->jalur_shell, shell->arg);
        
        /* Jika gagal */
        _exit(127);
    }
    
    /* Proses parent */
    shell->pid = pid;
    shell->fd_master = master_fd;
    shell->fd_slave = slave_fd;
    shell->nama_slave = pigura_strdup(slave_name);
    shell->berjalan = 1;
    shell->selesai = 0;
    
    /* Setup signal handler untuk SIGCHLD */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = cangkang_sigchld_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);
    
    /* Set mode raw pada master */
    if (shell->mode == CANGKANG_MODE_RAW) {
        tcsetattr(master_fd, TCSANOW, &shell->termios_shell);
    }
    
    return 1;
}

/* Baca output dari shell */
int cangkang_baca(ManagerCangkang *shell, char *buffer, int ukuran) {
    ssize_t n;
    
    if (!shell || !buffer || ukuran <= 0) {
        return 0;
    }
    
    if (!shell->berjalan || shell->fd_master < 0) {
        return 0;
    }
    
    n = read(shell->fd_master, buffer, (size_t)(ukuran - 1));
    if (n > 0) {
        buffer[n] = '\0';
        
        /* Panggil callback jika ada */
        if (shell->callback_output) {
            shell->callback_output(buffer, (int)n, shell->user_data);
        }
        
        return (int)n;
    }
    
    return 0;
}

/* Tulis input ke shell */
int cangkang_tulis(ManagerCangkang *shell, const char *buffer, int ukuran) {
    ssize_t n;
    
    if (!shell || !buffer || ukuran <= 0) {
        return 0;
    }
    
    if (!shell->berjalan || shell->fd_master < 0) {
        return 0;
    }
    
    n = write(shell->fd_master, buffer, (size_t)ukuran);
    
    return (n > 0) ? (int)n : 0;
}

/* Kirim sinyal ke shell */
int cangkang_sinyal(ManagerCangkang *shell, int sinyal) {
    if (!shell || shell->pid <= 0) {
        return 0;
    }
    
    return (kill(shell->pid, sinyal) == 0);
}

/* Cek apakah shell berjalan */
int cangkang_berjalan(ManagerCangkang *shell) {
    int status;
    pid_t result;
    
    if (!shell || shell->pid <= 0) {
        return 0;
    }
    
    /* Cek status proses non-blocking */
    result = waitpid(shell->pid, &status, WNOHANG);
    
    if (result == 0) {
        /* Proses masih berjalan */
        return 1;
    }
    
    if (result == shell->pid) {
        /* Proses sudah selesai */
        shell->selesai = 1;
        shell->berjalan = 0;
        shell->status_keluar = WEXITSTATUS(status);
        
        if (shell->callback_keluar) {
            shell->callback_keluar(shell->status_keluar, shell->user_data);
        }
        
        return 0;
    }
    
    /* Error */
    return 0;
}

/* Cek apakah shell selesai */
int cangkang_selesai(ManagerCangkang *shell) {
    return shell ? shell->selesai : 0;
}

/* Dapatkan status keluar */
int cangkang_status_keluar(ManagerCangkang *shell) {
    return shell ? shell->status_keluar : -1;
}

/* Dapatkan PID shell */
pid_t cangkang_pid(ManagerCangkang *shell) {
    return shell ? shell->pid : -1;
}

/* Dapatkan fd master */
int cangkang_fd_master(ManagerCangkang *shell) {
    return shell ? shell->fd_master : -1;
}

/* Tunggu shell selesai */
int cangkang_tunggu(ManagerCangkang *shell, int opsi) {
    int status;
    pid_t result;
    
    if (!shell || shell->pid <= 0) {
        return -1;
    }
    
    if (shell->selesai) {
        return shell->status_keluar;
    }
    
    result = waitpid(shell->pid, &status, opsi);
    
    if (result == shell->pid) {
        shell->selesai = 1;
        shell->berjalan = 0;
        shell->status_keluar = WEXITSTATUS(status);
        
        if (shell->callback_keluar) {
            shell->callback_keluar(shell->status_keluar, shell->user_data);
        }
        
        return shell->status_keluar;
    }
    
    return -1;
}

/* Paksa shell keluar */
int cangkang_hentikan(ManagerCangkang *shell, int sinyal) {
    if (!shell || shell->pid <= 0) {
        return 0;
    }
    
    if (kill(shell->pid, sinyal) != 0) {
        return 0;
    }
    
    /* Tunggu proses benar-benar mati */
    cangkang_tunggu(shell, 0);
    
    return 1;
}

/* Reset manager shell */
void cangkang_reset(ManagerCangkang *shell) {
    
    if (!shell) {
        return;
    }
    
    /* Tutup file descriptor */
    if (shell->fd_master >= 0) {
        close(shell->fd_master);
        shell->fd_master = -1;
    }
    
    if (shell->fd_slave >= 0) {
        close(shell->fd_slave);
        shell->fd_slave = -1;
    }
    
    /* Bebaskan nama slave */
    if (shell->nama_slave) {
        free(shell->nama_slave);
        shell->nama_slave = NULL;
    }
    
    /* Reset state */
    shell->pid = -1;
    shell->berjalan = 0;
    shell->selesai = 0;
    shell->status_keluar = 0;
}

/* Bebaskan manager shell */
void cangkang_bebas(ManagerCangkang *shell) {
    int i;
    
    if (!shell) {
        return;
    }
    
    /* Hentikan shell jika masih berjalan */
    if (shell->berjalan) {
        cangkang_hentikan(shell, SIGTERM);
    }
    
    /* Reset */
    cangkang_reset(shell);
    
    /* Bebaskan jalur shell */
    if (shell->jalur_shell) {
        free(shell->jalur_shell);
        shell->jalur_shell = NULL;
    }
    
    /* Bebaskan argumen */
    for (i = 0; i < shell->jumlah_arg; i++) {
        if (shell->arg[i]) {
            free(shell->arg[i]);
            shell->arg[i] = NULL;
        }
    }
    shell->jumlah_arg = 0;
    
    /* Bebaskan environment */
    for (i = 0; i < shell->jumlah_lingk; i++) {
        if (shell->lingkungan[i]) {
            free(shell->lingkungan[i]);
            shell->lingkungan[i] = NULL;
        }
    }
    shell->jumlah_lingk = 0;
}
