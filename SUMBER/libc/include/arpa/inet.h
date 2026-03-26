/*
 * PIGURA LIBC - ARPA/INET.H
 * ==========================
 * Header untuk Internet manipulation functions.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#ifndef LIBC_ARPA_INET_H
#define LIBC_ARPA_INET_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stdint.h>
#include <sys/socket.h>

/* ============================================================
 * KONSTANTA
 * ============================================================
 */

/* Konstanta untuk inet_pton/inet_ntop */
#define INET_ADDRSTRLEN   16
#define INET6_ADDRSTRLEN  46

/* ============================================================
 * DEKLARASI FUNGSI
 * ============================================================
 */

/*
 * htonl - Host to network long
 */
extern uint32_t htonl(uint32_t hostlong);

/*
 * htons - Host to network short
 */
extern uint16_t htons(uint16_t hostshort);

/*
 * ntohl - Network to host long
 */
extern uint32_t ntohl(uint32_t netlong);

/*
 * ntohs - Network to host short
 */
extern uint16_t ntohs(uint16_t netshort);

/*
 * inet_addr - Convert dotted decimal to address
 */
extern in_addr_t inet_addr(const char *cp);

/*
 * inet_aton - Convert dotted decimal to address (with result)
 */
extern int inet_aton(const char *cp, struct in_addr *inp);

/*
 * inet_ntoa - Convert address to dotted decimal
 */
extern char *inet_ntoa(struct in_addr in);

/*
 * inet_ntop - Convert address to presentation format
 */
extern const char *inet_ntop(int af, const void *src, char *dst,
                             socklen_t size);

/*
 * inet_pton - Convert presentation to address
 */
extern int inet_pton(int af, const char *src, void *dst);

/*
 * inet_network - Convert dotted decimal to network number
 */
extern in_addr_t inet_network(const char *cp);

/*
 * inet_netof - Get network part of address
 */
extern in_addr_t inet_netof(struct in_addr in);

/*
 * inet_lnaof - Get local network address part
 */
extern in_addr_t inet_lnaof(struct in_addr in);

/*
 * inet_makeaddr - Make address from network and local parts
 */
extern struct in_addr inet_makeaddr(in_addr_t net, in_addr_t lna);

#endif /* LIBC_ARPA_INET_H */
