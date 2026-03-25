/*
 * PIGURA LIBC - NETWORK/SOCKET.C
 * ==============================
 * Implementasi fungsi socket.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Implementasi untuk Pigura OS dengan dukungan
 * BSD socket API.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

/* ============================================================
 * DEKLARASI SYSCALL
 * ============================================================
 */

/* Syscall interface ke kernel */
extern long __syscall_socket(int domain, int type, int protocol);
extern long __syscall_bind(int sockfd, const void *addr, socklen_t addrlen);
extern long __syscall_listen(int sockfd, int backlog);
extern long __syscall_accept(int sockfd, void *addr, socklen_t *addrlen);
extern long __syscall_connect(int sockfd, const void *addr, socklen_t addrlen);
extern long __syscall_shutdown(int sockfd, int how);
extern long __syscall_getsockname(int sockfd, void *addr, socklen_t *addrlen);
extern long __syscall_getpeername(int sockfd, void *addr, socklen_t *addrlen);
extern long __syscall_getsockopt(int sockfd, int level, int optname,
                                 void *optval, socklen_t *optlen);
extern long __syscall_setsockopt(int sockfd, int level, int optname,
                                 const void *optval, socklen_t optlen);

/* ============================================================
 * FUNGSI SOCKET UTAMA
 * ============================================================
 */

/*
 * socket - Buat endpoint untuk komunikasi
 *
 * Parameter:
 *   domain   - Address family (AF_INET, AF_UNIX, dll)
 *   type     - Socket type (SOCK_STREAM, SOCK_DGRAM, dll)
 *   protocol - Protocol (biasanya 0)
 *
 * Return: File descriptor socket, atau -1 jika error
 */
int socket(int domain, int type, int protocol) {
    long result;

    /* Panggil syscall */
    result = __syscall_socket(domain, type, protocol);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (int)result;
}

/*
 * bind - Bind socket ke alamat
 *
 * Parameter:
 *   sockfd   - Socket file descriptor
 *   addr     - Pointer ke struktur alamat
 *   addrlen  - Panjang struktur alamat
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    long result;

    /* Validasi parameter */
    if (addr == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    /* Panggil syscall */
    result = __syscall_bind(sockfd, addr, addrlen);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/*
 * listen - Listen untuk koneksi pada socket
 *
 * Parameter:
 *   sockfd  - Socket file descriptor
 *   backlog - Batas antrian koneksi pending
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int listen(int sockfd, int backlog) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (backlog < 0) {
        errno = EINVAL;
        return -1;
    }

    /* Batasi backlog ke nilai reasonable */
    if (backlog > SOMAXCONN) {
        backlog = SOMAXCONN;
    }

    /* Panggil syscall */
    result = __syscall_listen(sockfd, backlog);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/*
 * accept - Terima koneksi pada socket
 *
 * Parameter:
 *   sockfd   - Socket file descriptor
 *   addr     - Buffer untuk alamat client (boleh NULL)
 *   addrlen  - Input: ukuran buffer; Output: ukuran alamat
 *
 * Return: File descriptor baru untuk koneksi, atau -1
 */
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    /* addr dan addrlen bisa NULL jika tidak diperlukan */
    if (addr != NULL && addrlen == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall */
    result = __syscall_accept(sockfd, addr, addrlen);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (int)result;
}

/*
 * connect - Inisiasi koneksi pada socket
 *
 * Parameter:
 *   sockfd   - Socket file descriptor
 *   addr     - Alamat tujuan
 *   addrlen  - Panjang struktur alamat
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (addr == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall */
    result = __syscall_connect(sockfd, addr, addrlen);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/*
 * shutdown - Shutdown socket send/receive
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   how    - SHUT_RD, SHUT_WR, atau SHUT_RDWR
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int shutdown(int sockfd, int how) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    /* Validasi how */
    if (how != SHUT_RD && how != SHUT_WR && how != SHUT_RDWR) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall */
    result = __syscall_shutdown(sockfd, how);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/* ============================================================
 * FUNGSI GETSOCKNAME/GETPEERNAME
 * ============================================================
 */

/*
 * getsockname - Dapatkan alamat lokal socket
 *
 * Parameter:
 *   sockfd   - Socket file descriptor
 *   addr     - Buffer untuk alamat
 *   addrlen  - Input: ukuran buffer; Output: ukuran alamat
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (addr == NULL || addrlen == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall */
    result = __syscall_getsockname(sockfd, addr, addrlen);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/*
 * getpeername - Dapatkan alamat remote socket
 *
 * Parameter:
 *   sockfd   - Socket file descriptor
 *   addr     - Buffer untuk alamat
 *   addrlen  - Input: ukuran buffer; Output: ukuran alamat
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (addr == NULL || addrlen == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall */
    result = __syscall_getpeername(sockfd, addr, addrlen);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/* ============================================================
 * FUNGSI SOCKET OPTIONS
 * ============================================================
 */

/*
 * getsockopt - Dapatkan opsi socket
 *
 * Parameter:
 *   sockfd   - Socket file descriptor
 *   level    - Level protocol (SOL_SOCKET, IPPROTO_TCP, dll)
 *   optname  - Nama opsi
 *   optval   - Buffer untuk nilai opsi
 *   optlen   - Input: ukuran buffer; Output: ukuran nilai
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int getsockopt(int sockfd, int level, int optname, void *optval,
               socklen_t *optlen) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (optval == NULL || optlen == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall */
    result = __syscall_getsockopt(sockfd, level, optname, optval, optlen);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/*
 * setsockopt - Set opsi socket
 *
 * Parameter:
 *   sockfd   - Socket file descriptor
 *   level    - Level protocol (SOL_SOCKET, IPPROTO_TCP, dll)
 *   optname  - Nama opsi
 *   optval   - Pointer ke nilai opsi
 *   optlen   - Panjang nilai opsi
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    /* optval boleh NULL untuk beberapa opsi */
    if (optval == NULL && optlen > 0) {
        errno = EINVAL;
        return -1;
    }

    /* Panggil syscall */
    result = __syscall_setsockopt(sockfd, level, optname, optval, optlen);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/* ============================================================
 * FUNGSI VARIADIC (COMPATIBILITY)
 * ============================================================
 */

/*
 * accept4 - Accept dengan flags (Linux extension)
 *
 * Parameter:
 *   sockfd   - Socket file descriptor
 *   addr     - Buffer untuk alamat client
 *   addrlen  - Panjang buffer alamat
 *   flags    - Flags (SOCK_NONBLOCK, SOCK_CLOEXEC)
 *
 * Return: File descriptor baru, atau -1
 */
int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen,
            int flags) {
    int newfd;
    int ret;

    /* Gunakan accept biasa */
    newfd = accept(sockfd, addr, addrlen);
    if (newfd < 0) {
        return -1;
    }

    /* Set flags setelahnya */
    if (flags & SOCK_NONBLOCK) {
        ret = fcntl(newfd, F_GETFL, 0);
        if (ret >= 0) {
            fcntl(newfd, F_SETFL, ret | O_NONBLOCK);
        }
    }

    if (flags & SOCK_CLOEXEC) {
        ret = fcntl(newfd, F_GETFD, 0);
        if (ret >= 0) {
            fcntl(newfd, F_SETFD, ret | FD_CLOEXEC);
        }
    }

    return newfd;
}

/*
 * socketpair - Buat pair socket yang terhubung
 *
 * Parameter:
 *   domain   - Address family (AF_UNIX atau AF_LOCAL)
 *   type     - Socket type
 *   protocol - Protocol (biasanya 0)
 *   sv       - Array untuk menyimpan dua file descriptor
 *
 * Return: 0 jika berhasil, -1 jika error
 */
int socketpair(int domain, int type, int protocol, int sv[2]) {
    long result;

    /* Validasi parameter */
    if (sv == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Syscall socketpair */
    extern long __syscall_socketpair(int, int, int, int *);
    result = __syscall_socketpair(domain, type, protocol, sv);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

/*
 * sockatmark - Cek apakah socket at out-of-band mark
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *
 * Return: 1 jika at mark, 0 jika tidak, -1 jika error
 */
int sockatmark(int sockfd) {
    int atmark;
    socklen_t len;

    len = sizeof(atmark);

    if (ioctl(sockfd, SIOCATMARK, &atmark) < 0) {
        return -1;
    }

    return atmark;
}
