/*
 * PIGURA LIBC - SYS/SOCKET.H
 * ===========================
 * Header untuk socket operations sesuai standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.1 - Perbaikan urutan tipe data
 */

#ifndef LIBC_SYS_SOCKET_H
#define LIBC_SYS_SOCKET_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/* ============================================================
 * TIPE DATA DASAR
 * ============================================================
 */

/* Tipe untuk socket address length */
typedef size_t socklen_t;

/* Tipe address family */
typedef unsigned short sa_family_t;

/* Tipe port */
typedef unsigned short in_port_t;

/* Tipe IPv4 address */
typedef unsigned int in_addr_t;

/* ============================================================
 * STRUKTUR ADDRESS
 * ============================================================
 */

/* Struktur in_addr (IPv4 address) */
struct in_addr {
    in_addr_t s_addr;       /* IPv4 address */
};

/* Struktur in6_addr (IPv6 address) */
struct in6_addr {
    uint8_t s6_addr[16];    /* IPv6 address */
};

/* Struktur sockaddr generik */
struct sockaddr {
    sa_family_t sa_family;  /* Address family */
    char sa_data[14];       /* Address data */
};

/* Struktur sockaddr_storage (cukup besar untuk semua address family) */
struct sockaddr_storage {
    sa_family_t ss_family;  /* Address family */
    char __ss_padding[126]; /* Padding */
};

/* Struktur sockaddr_in (IPv4) */
struct sockaddr_in {
    sa_family_t sin_family; /* AF_INET */
    in_port_t sin_port;     /* Port number */
    struct in_addr sin_addr;/* IPv4 address */
    char sin_zero[8];       /* Padding */
};

/* Struktur sockaddr_in6 (IPv6) */
struct sockaddr_in6 {
    sa_family_t sin6_family;   /* AF_INET6 */
    in_port_t sin6_port;       /* Port number */
    uint32_t sin6_flowinfo;    /* Flow info */
    struct in6_addr sin6_addr; /* IPv6 address */
    uint32_t sin6_scope_id;    /* Scope ID */
};

/* Struktur sockaddr_un (Unix domain) */
struct sockaddr_un {
    sa_family_t sun_family;  /* AF_UNIX */
    char sun_path[108];      /* Path name */
};

/* ============================================================
 * ADDRESS FAMILIES
 * ============================================================
 */
#define AF_UNSPEC     0    /* Unspecified */
#define AF_UNIX       1    /* Unix domain */
#define AF_LOCAL      1    /* Local (alias for AF_UNIX) */
#define AF_INET       2    /* IPv4 */
#define AF_INET6      10   /* IPv6 */
#define AF_NETLINK    16   /* Netlink */
#define AF_PACKET     17   /* Packet */
#define AF_BLUETOOTH  31   /* Bluetooth */

/* ============================================================
 * PROTOCOL FAMILIES
 * ============================================================
 */
#define PF_UNSPEC     AF_UNSPEC
#define PF_UNIX       AF_UNIX
#define PF_LOCAL      AF_LOCAL
#define PF_INET       AF_INET
#define PF_INET6      AF_INET6
#define PF_NETLINK    AF_NETLINK
#define PF_PACKET     AF_PACKET

/* ============================================================
 * SOCKET TYPES
 * ============================================================
 */
#define SOCK_STREAM    1   /* Stream socket (TCP) */
#define SOCK_DGRAM     2   /* Datagram socket (UDP) */
#define SOCK_RAW       3   /* Raw socket */
#define SOCK_RDM       4   /* Reliably-delivered message */
#define SOCK_SEQPACKET 5   /* Sequenced packet */
#define SOCK_PACKET    10  /* Packet socket (deprecated) */

/* Flags for socket() and accept4() */
#define SOCK_NONBLOCK  0x800   /* Non-blocking socket */
#define SOCK_CLOEXEC   0x400   /* Close-on-exec */

/* ============================================================
 * PROTOCOL NUMBERS
 * ============================================================
 */
#define IPPROTO_IP      0    /* IPv4 */
#define IPPROTO_ICMP    1    /* ICMP */
#define IPPROTO_IGMP    2    /* IGMP */
#define IPPROTO_TCP     6    /* TCP */
#define IPPROTO_UDP     17   /* UDP */
#define IPPROTO_IPV6    41   /* IPv6 */
#define IPPROTO_ICMPV6  58   /* ICMPv6 */
#define IPPROTO_RAW     255  /* Raw */

/* ============================================================
 * SOCKET OPTIONS
 * ============================================================
 */

/* Socket level */
#define SOL_SOCKET     1

/* Socket options - generic */
#define SO_DEBUG       1     /* Debug */
#define SO_REUSEADDR   2     /* Reuse address */
#define SO_TYPE        3     /* Get socket type */
#define SO_ERROR       4     /* Get error status */
#define SO_DONTROUTE   5     /* Don't route */
#define SO_BROADCAST   6     /* Broadcast */
#define SO_SNDBUF      7     /* Send buffer size */
#define SO_RCVBUF      8     /* Receive buffer size */
#define SO_KEEPALIVE   9     /* Keep alive */
#define SO_OOBINLINE   10    /* Out-of-band inline */
#define SO_NO_CHECK    11    /* No checksum */
#define SO_PRIORITY    12    /* Priority */
#define SO_LINGER      13    /* Linger */
#define SO_BSDCOMPAT   14    /* BSD compatibility */
#define SO_REUSEPORT   15    /* Reuse port */
#define SO_PASSCRED    16    /* Pass credentials */
#define SO_PEERCRED    17    /* Peer credentials */
#define SO_RCVLOWAT    18    /* Receive low water mark */
#define SO_SNDLOWAT    19    /* Send low water mark */
#define SO_RCVTIMEO    20    /* Receive timeout */
#define SO_SNDTIMEO    21    /* Send timeout */
#define SO_ATTACH_FILTER 26  /* Attach filter */
#define SO_DETACH_FILTER 27  /* Detach filter */
#define SO_ACCEPTCONN   30   /* Accepting connections */

/* ============================================================
 * SEND/RECV FLAGS
 * ============================================================
 */
#define MSG_OOB        0x0001   /* Out-of-band data */
#define MSG_PEEK       0x0002   /* Peek at incoming data */
#define MSG_DONTROUTE  0x0004   /* Don't route */
#define MSG_CTRUNC     0x0008   /* Control data truncated */
#define MSG_PROXY      0x0010   /* Proxy */
#define MSG_TRUNC      0x0020   /* Data truncated */
#define MSG_DONTWAIT   0x0040   /* Don't block */
#define MSG_EOR        0x0080   /* End of record */
#define MSG_WAITALL    0x0100   /* Wait for full request */
#define MSG_FIN        0x0200   /* Sender finished */
#define MSG_NOSIGNAL   0x4000   /* Don't generate SIGPIPE */

/* ============================================================
 * SHUTDOWN HOW
 * ============================================================
 */
#define SHUT_RD        0    /* Shutdown read */
#define SHUT_WR        1    /* Shutdown write */
#define SHUT_RDWR      2    /* Shutdown both */

/* ============================================================
 * MISCELLANEOUS CONSTANTS
 * ============================================================
 */

/* Maximum connections for listen() */
#define SOMAXCONN      128

/* Maximum iov count for sendmsg/recvmsg */
#define IOV_MAX        1024

/* ============================================================
 * STRUKTUR MSGHDR
 * ============================================================
 */
struct iovec;  /* Forward declaration */

struct msghdr {
    void *msg_name;         /* Socket address */
    socklen_t msg_namelen;  /* Socket address length */
    struct iovec *msg_iov;  /* Scatter/gather array */
    size_t msg_iovlen;      /* Number of elements in msg_iov */
    void *msg_control;      /* Ancillary data */
    size_t msg_controllen;  /* Ancillary data length */
    int msg_flags;          /* Flags on received message */
};

/* Struktur cmsghdr */
struct cmsghdr {
    size_t cmsg_len;        /* Data byte count */
    int cmsg_level;         /* Originating protocol */
    int cmsg_type;          /* Protocol-specific type */
};

/* Struktur linger */
struct linger {
    int l_onoff;            /* Linger active */
    int l_linger;           /* How many seconds to linger */
};

/* ============================================================
 * DEKLARASI FUNGSI
 * ============================================================
 */

/*
 * socket - Buat socket
 */
extern int socket(int domain, int type, int protocol);

/*
 * bind - Bind socket ke address
 */
extern int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

/*
 * listen - Listen for connections
 */
extern int listen(int sockfd, int backlog);

/*
 * accept - Accept connection
 */
extern int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

/*
 * accept4 - Accept with flags
 */
extern int accept4(int sockfd, struct sockaddr *addr, socklen_t *addrlen,
                   int flags);

/*
 * connect - Connect to server
 */
extern int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

/*
 * shutdown - Shutdown socket
 */
extern int shutdown(int sockfd, int how);

/*
 * getsockname - Get socket address
 */
extern int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

/*
 * getpeername - Get peer address
 */
extern int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

/*
 * getsockopt - Get socket option
 */
extern int getsockopt(int sockfd, int level, int optname,
                     void *optval, socklen_t *optlen);

/*
 * setsockopt - Set socket option
 */
extern int setsockopt(int sockfd, int level, int optname,
                     const void *optval, socklen_t optlen);

/*
 * send - Send data
 */
extern ssize_t send(int sockfd, const void *buf, size_t len, int flags);

/*
 * sendto - Send data to address
 */
extern ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen);

/*
 * sendmsg - Send message
 */
extern ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);

/*
 * recv - Receive data
 */
extern ssize_t recv(int sockfd, void *buf, size_t len, int flags);

/*
 * recvfrom - Receive data from address
 */
extern ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);

/*
 * recvmsg - Receive message
 */
extern ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);

/*
 * socketpair - Create pair of sockets
 */
extern int socketpair(int domain, int type, int protocol, int sv[2]);

/*
 * sockatmark - Check if at out-of-band mark
 */
extern int sockatmark(int sockfd);

/* ============================================================
 * IOCTL CONSTANTS
 * ============================================================
 */
#define SIOCATMARK     0x8905   /* At out-of-band mark */
#define FIONREAD       0x541B   /* Get bytes to read */
#define FIONBIO        0x5421   /* Set non-blocking */
#define FIOASYNC       0x5452   /* Set async */

/* ============================================================
 * DEKLARASI IOCTL
 * ============================================================
 */
extern int ioctl(int fd, unsigned long request, ...);

#endif /* LIBC_SYS_SOCKET_H */
