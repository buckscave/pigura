/*
 * PIGURA LIBC - NETDB.H
 * ======================
 * Header untuk network database functions sesuai standar POSIX.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_NETDB_H
#define LIBC_NETDB_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

/* ============================================================
 * TIPE DATA
 * ============================================================
 */

/* Struktur hostent */
struct hostent {
    char *h_name;         /* Official name of host */
    char **h_aliases;     /* Alias list */
    int h_addrtype;       /* Host address type */
    int h_length;         /* Length of address */
    char **h_addr_list;   /* List of addresses */
};

/* Alias for h_addr */
#define h_addr h_addr_list[0]

/* Struktur netent */
struct netent {
    char *n_name;         /* Official name of network */
    char **n_aliases;     /* Alias list */
    int n_addrtype;       /* Net address type */
    uint32_t n_net;       /* Network number */
};

/* Struktur servent */
struct servent {
    char *s_name;         /* Official service name */
    char **s_aliases;     /* Alias list */
    int s_port;           /* Port number */
    char *s_proto;        /* Protocol to use */
};

/* Struktur protoent */
struct protoent {
    char *p_name;         /* Official protocol name */
    char **p_aliases;     /* Alias list */
    int p_proto;          /* Protocol number */
};

/* Struktur addrinfo */
struct addrinfo {
    int ai_flags;         /* AI_PASSIVE, AI_CANONNAME, etc */
    int ai_family;        /* AF_INET, AF_INET6, etc */
    int ai_socktype;      /* SOCK_STREAM, SOCK_DGRAM, etc */
    int ai_protocol;      /* 0 for any */
    socklen_t ai_addrlen; /* Length of ai_addr */
    struct sockaddr *ai_addr; /* Binary address */
    char *ai_canonname;   /* Canonical name */
    struct addrinfo *ai_next; /* Next in list */
};

/* ============================================================
 * ADDRINFO FLAGS
 * ============================================================
 */
#define AI_PASSIVE     0x0001  /* Socket address is bind() */
#define AI_CANONNAME   0x0002  /* Return canonical name */
#define AI_NUMERICHOST 0x0004  /* No name resolution */
#define AI_V4MAPPED    0x0008  /* Map IPv4 to IPv6 */
#define AI_ALL         0x0010  /* Return both IPv4 and IPv6 */
#define AI_ADDRCONFIG  0x0020  /* Only if configured */

/* ============================================================
 * NAME/ADDRESS FLAGS
 * ============================================================
 */
#define NI_MAXHOST     1025    /* Max host name length */
#define NI_MAXSERV     32      /* Max service name length */

/* Flags for getnameinfo() */
#define NI_NOFQDN      0x0001  /* No FQDN */
#define NI_NUMERICHOST 0x0002  /* Return numeric address */
#define NI_NAMEREQD    0x0004  /* Name required */
#define NI_NUMERICSERV 0x0008  /* Return numeric port */
#define NI_DGRAM       0x0010  /* Datagram service */

/* ============================================================
 * ERROR CODES
 * ============================================================
 */
#define EAI_BADFLAGS    -1   /* Invalid flags */
#define EAI_NONAME      -2   /* Name or service not known */
#define EAI_AGAIN       -3   /* Temporary failure */
#define EAI_FAIL        -4   /* Permanent failure */
#define EAI_FAMILY      -6   /* Address family not supported */
#define EAI_SOCKTYPE    -7   /* Socket type not supported */
#define EAI_SERVICE     -8   /* Service not supported */
#define EAI_MEMORY      -10  /* Memory allocation failure */
#define EAI_SYSTEM      -11  /* System error */
#define EAI_OVERFLOW    -12  /* Argument buffer overflow */

/* ============================================================
 * DEKLARASI FUNGSI
 * ============================================================
 */

/*
 * gethostbyname - Get host by name
 */
extern struct hostent *gethostbyname(const char *name);

/*
 * gethostbyaddr - Get host by address
 */
extern struct hostent *gethostbyaddr(const void *addr, socklen_t len,
                                     int type);

/*
 * gethostent - Get next host entry
 */
extern struct hostent *gethostent(void);

/*
 * sethostent - Open host database
 */
extern void sethostent(int stayopen);

/*
 * endhostent - Close host database
 */
extern void endhostent(void);

/*
 * getnetbyname - Get network by name
 */
extern struct netent *getnetbyname(const char *name);

/*
 * getnetbyaddr - Get network by address
 */
extern struct netent *getnetbyaddr(uint32_t net, int type);

/*
 * getnetent - Get next network entry
 */
extern struct netent *getnetent(void);

/*
 * setnetent - Open network database
 */
extern void setnetent(int stayopen);

/*
 * endnetent - Close network database
 */
extern void endnetent(void);

/*
 * getservbyname - Get service by name
 */
extern struct servent *getservbyname(const char *name, const char *proto);

/*
 * getservbyport - Get service by port
 */
extern struct servent *getservbyport(int port, const char *proto);

/*
 * getservent - Get next service entry
 */
extern struct servent *getservent(void);

/*
 * setservent - Open service database
 */
extern void setservent(int stayopen);

/*
 * endservent - Close service database
 */
extern void endservent(void);

/*
 * getprotobyname - Get protocol by name
 */
extern struct protoent *getprotobyname(const char *name);

/*
 * getprotobynumber - Get protocol by number
 */
extern struct protoent *getprotobynumber(int proto);

/*
 * getprotoent - Get next protocol entry
 */
extern struct protoent *getprotoent(void);

/*
 * setprotoent - Open protocol database
 */
extern void setprotoent(int stayopen);

/*
 * endprotoent - Close protocol database
 */
extern void endprotoent(void);

/*
 * getaddrinfo - Get address info
 */
extern int getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **res);

/*
 * freeaddrinfo - Free address info
 */
extern void freeaddrinfo(struct addrinfo *res);

/*
 * gai_strerror - Get error string for getaddrinfo
 */
extern const char *gai_strerror(int errcode);

/*
 * getnameinfo - Get name info
 */
extern int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
                       char *host, socklen_t hostlen,
                       char *serv, socklen_t servlen, int flags);

/* ============================================================
 * KONSTANTA PORT
 * ============================================================
 */

/* Well-known ports */
#define IPPORT_ECHO       7
#define IPPORT_DISCARD    9
#define IPPORT_SYSTAT     11
#define IPPORT_DAYTIME    13
#define IPPORT_NETSTAT    15
#define IPPORT_FTP        21
#define IPPORT_TELNET     23
#define IPPORT_SMTP       25
#define IPPORT_TIMESERVER 37
#define IPPORT_NAMESERVER 42
#define IPPORT_WHOIS      43
#define IPPORT_MTP        57
#define IPPORT_TFTP       69
#define IPPORT_RJE        77
#define IPPORT_FINGER     79
#define IPPORT_TTYLINK    87
#define IPPORT_SUPDUP     95
#define IPPORT_EXECSERVER 512
#define IPPORT_LOGINSERVER 513
#define IPPORT_CMDSERVER  514
#define IPPORT_EFSSERVER  520
#define IPPORT_BIFFUDP    512
#define IPPORT_WHOSERVER  513
#define IPPORT_ROUTESERVER 520
#define IPPORT_RESERVED   1024

/* ============================================================
 * KONSTANTA LAINNYA
 * ============================================================
 */

/* h_errno values */
#define HOST_NOT_FOUND   1
#define TRY_AGAIN        2
#define NO_RECOVERY      3
#define NO_DATA          4
#define NO_ADDRESS       NO_DATA

/* Global h_errno */
extern int h_errno;

#endif /* LIBC_NETDB_H */
