/*
 * PIGURA LIBC - NETWORK/SEND.C
 * ============================
 * Implementasi fungsi send/recv socket.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Implementasi untuk Pigura OS dengan dukungan
 * BSD socket I/O.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>

/* ============================================================
 * DEKLARASI SYSCALL
 * ============================================================
 */

/* Syscall interface ke kernel */
extern long __syscall_send(int sockfd, const void *buf, size_t len,
                           int flags);
extern long __syscall_recv(int sockfd, void *buf, size_t len, int flags);
extern long __syscall_sendto(int sockfd, const void *buf, size_t len,
                             int flags, const void *dest_addr,
                             socklen_t addrlen);
extern long __syscall_recvfrom(int sockfd, void *buf, size_t len,
                               int flags, void *src_addr,
                               socklen_t *addrlen);
extern long __syscall_sendmsg(int sockfd, const struct msghdr *msg,
                              int flags);
extern long __syscall_recvmsg(int sockfd, struct msghdr *msg, int flags);

/* ============================================================
 * FUNGSI SEND/RECV DASAR
 * ============================================================
 */

/*
 * send - Kirim data melalui socket
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   buf    - Buffer data yang dikirim
 *   len    - Panjang data
 *   flags  - Flags (MSG_*)
 *
 * Return: Jumlah byte terkirim, atau -1 jika error
 */
ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (buf == NULL && len > 0) {
        errno = EINVAL;
        return -1;
    }

    /* Handle zero-length send */
    if (len == 0) {
        return 0;
    }

    /* Panggil syscall */
    result = __syscall_send(sockfd, buf, len, flags);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (ssize_t)result;
}

/*
 * recv - Terima data dari socket
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   buf    - Buffer untuk menyimpan data
 *   len    - Ukuran buffer
 *   flags  - Flags (MSG_*)
 *
 * Return: Jumlah byte diterima, 0 jika connection closed, -1 error
 */
ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (buf == NULL && len > 0) {
        errno = EINVAL;
        return -1;
    }

    /* Handle zero-length recv */
    if (len == 0) {
        return 0;
    }

    /* Panggil syscall */
    result = __syscall_recv(sockfd, buf, len, flags);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (ssize_t)result;
}

/* ============================================================
 * FUNGSI SENDTO/RECVFROM
 * ============================================================
 */

/*
 * sendto - Kirim data ke alamat tertentu
 *
 * Parameter:
 *   sockfd    - Socket file descriptor
 *   buf       - Buffer data yang dikirim
 *   len       - Panjang data
 *   flags     - Flags (MSG_*)
 *   dest_addr - Alamat tujuan
 *   addrlen   - Panjang struktur alamat
 *
 * Return: Jumlah byte terkirim, atau -1 jika error
 */
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (buf == NULL && len > 0) {
        errno = EINVAL;
        return -1;
    }

    /* dest_addr boleh NULL untuk connected socket */
    if (dest_addr != NULL && addrlen == 0) {
        errno = EINVAL;
        return -1;
    }

    /* Handle zero-length send */
    if (len == 0) {
        return 0;
    }

    /* Panggil syscall */
    result = __syscall_sendto(sockfd, buf, len, flags,
                              dest_addr, addrlen);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (ssize_t)result;
}

/*
 * recvfrom - Terima data dengan alamat pengirim
 *
 * Parameter:
 *   sockfd   - Socket file descriptor
 *   buf      - Buffer untuk menyimpan data
 *   len      - Ukuran buffer
 *   flags    - Flags (MSG_*)
 *   src_addr - Buffer untuk alamat pengirim (boleh NULL)
 *   addrlen  - Input: ukuran buffer; Output: ukuran alamat
 *
 * Return: Jumlah byte diterima, atau -1 jika error
 */
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    long result;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (buf == NULL && len > 0) {
        errno = EINVAL;
        return -1;
    }

    /* src_addr boleh NULL */
    if (src_addr != NULL && addrlen == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Handle zero-length recv */
    if (len == 0) {
        return 0;
    }

    /* Panggil syscall */
    result = __syscall_recvfrom(sockfd, buf, len, flags,
                                src_addr, addrlen);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (ssize_t)result;
}

/* ============================================================
 * FUNGSI SENDMSG/RECVMSG
 * ============================================================
 */

/*
 * sendmsg - Kirim pesan dengan struktur msghdr
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   msg    - Pointer ke struktur msghdr
 *   flags  - Flags (MSG_*)
 *
 * Return: Jumlah byte terkirim, atau -1 jika error
 */
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    long result;
    size_t total_len = 0;
    size_t i;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (msg == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi iovec */
    if (msg->msg_iov != NULL && msg->msg_iovlen > 0) {
        /* Cek overflow total length */
        total_len = 0;
        for (i = 0; i < msg->msg_iovlen; i++) {
            if (msg->msg_iov[i].iov_len > 0 &&
                total_len > (size_t)-1 - msg->msg_iov[i].iov_len) {
                errno = EMSGSIZE;
                return -1;
            }
            total_len += msg->msg_iov[i].iov_len;
        }
    }

    /* Handle zero-length send */
    if (total_len == 0) {
        return 0;
    }

    /* Panggil syscall */
    result = __syscall_sendmsg(sockfd, msg, flags);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (ssize_t)result;
}

/*
 * recvmsg - Terima pesan dengan struktur msghdr
 *
 * Parameter:
 *   sockfd - Socket file descriptor
 *   msg    - Pointer ke struktur msghdr
 *   flags  - Flags (MSG_*)
 *
 * Return: Jumlah byte diterima, atau -1 jika error
 */
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    long result;
    size_t i;

    /* Validasi parameter */
    if (sockfd < 0) {
        errno = EBADF;
        return -1;
    }

    if (msg == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Validasi iovec */
    if (msg->msg_iov == NULL && msg->msg_iovlen > 0) {
        errno = EINVAL;
        return -1;
    }

    /* Hitung total buffer size */
    for (i = 0; i < msg->msg_iovlen; i++) {
        if (msg->msg_iov[i].iov_base == NULL &&
            msg->msg_iov[i].iov_len > 0) {
            errno = EINVAL;
            return -1;
        }
    }

    /* Panggil syscall */
    result = __syscall_recvmsg(sockfd, msg, flags);

    if (result < 0) {
        errno = -result;
        return -1;
    }

    return (ssize_t)result;
}

/* ============================================================
 * FUNGSI UTILITAS I/O
 * ============================================================
 */

/*
 * sendfile - Transfer data antara file descriptor
 *
 * Parameter:
 *   out_fd   - Output file descriptor (socket)
 *   in_fd    - Input file descriptor
 *   offset   - Offset file (boleh NULL)
 *   count    - Jumlah byte yang ditransfer
 *
 * Return: Jumlah byte terkirim, atau -1 jika error
 */
ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
    char buf[8192];
    ssize_t bytes_read;
    ssize_t bytes_sent;
    ssize_t total_sent;
    size_t to_read;

    /* Validasi parameter */
    if (out_fd < 0 || in_fd < 0) {
        errno = EBADF;
        return -1;
    }

    total_sent = 0;

    /* Jika offset diberikan, seek ke posisi tersebut */
    if (offset != NULL) {
        if (lseek(in_fd, *offset, SEEK_SET) < 0) {
            return -1;
        }
    }

    /* Loop hingga selesai atau error */
    while (count > 0) {
        /* Hitung berapa yang akan dibaca */
        to_read = (count > sizeof(buf)) ? sizeof(buf) : count;

        /* Baca dari input */
        bytes_read = read(in_fd, buf, to_read);
        if (bytes_read < 0) {
            if (total_sent > 0) {
                break;
            }
            return -1;
        }

        /* EOF */
        if (bytes_read == 0) {
            break;
        }

        /* Kirim ke output */
        bytes_sent = send(out_fd, buf, (size_t)bytes_read, 0);
        if (bytes_sent < 0) {
            if (total_sent > 0) {
                break;
            }
            return -1;
        }

        /* Update counters */
        total_sent += bytes_sent;
        count -= (size_t)bytes_sent;

        /* Update offset jika diminta */
        if (offset != NULL) {
            *offset += bytes_sent;
        }

        /* Jika tidak semua terkirim, berhenti */
        if (bytes_sent < bytes_read) {
            break;
        }
    }

    return total_sent;
}

/* End of file */
