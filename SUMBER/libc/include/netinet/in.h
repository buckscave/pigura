/*
 * PIGURA LIBC - NETINET/IN.H
 * ===========================
 * Header untuk Internet address family.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.1 - Perbaikan duplikasi tipe
 */

#ifndef LIBC_NETINET_IN_H
#define LIBC_NETINET_IN_H

/* ============================================================
 * DEPENDENSI
 * ============================================================
 */
#include <stdint.h>
#include <sys/socket.h>

/* ============================================================
 * TIPE DATA
 * ============================================================
 */

/* Struktur ipv6_mreq */
struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr;
    unsigned int ipv6mr_interface;
};

/* ============================================================
 * KONSTANTA IP
 * ============================================================
 */

/* IP address */
#define INADDR_ANY        ((in_addr_t)0x00000000)
#define INADDR_LOOPBACK   ((in_addr_t)0x7f000001)
#define INADDR_BROADCAST  ((in_addr_t)0xffffffff)
#define INADDR_NONE       ((in_addr_t)0xffffffff)
#define INADDR_UNSPEC_GROUP   ((in_addr_t)0xe0000000)
#define INADDR_ALLHOSTS_GROUP ((in_addr_t)0xe0000001)
#define INADDR_ALLRTRS_GROUP  ((in_addr_t)0xe0000002)
#define INADDR_MAX_LOCAL_GROUP ((in_addr_t)0xe00000ff)

/* IPv6 address */
#define IN6ADDR_ANY_INIT \
    { { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } }

#define IN6ADDR_LOOPBACK_INIT \
    { { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } } }

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;

/* ============================================================
 * IP PROTOCOL
 * ============================================================
 */
#define IPPROTO_IP       0
#define IPPROTO_HOPOPTS  0
#define IPPROTO_ICMP     1
#define IPPROTO_IGMP     2
#define IPPROTO_IPIP     4
#define IPPROTO_TCP      6
#define IPPROTO_EGP      8
#define IPPROTO_PUP      12
#define IPPROTO_UDP      17
#define IPPROTO_IDP      22
#define IPPROTO_TP       29
#define IPPROTO_DCCP     33
#define IPPROTO_IPV6     41
#define IPPROTO_ROUTING  43
#define IPPROTO_FRAGMENT 44
#define IPPROTO_RSVP     46
#define IPPROTO_GRE      47
#define IPPROTO_ESP      50
#define IPPROTO_AH       51
#define IPPROTO_ICMPV6   58
#define IPPROTO_NONE     59
#define IPPROTO_DSTOPTS  60
#define IPPROTO_MTP      92
#define IPPROTO_ENCAP    98
#define IPPROTO_PIM      103
#define IPPROTO_COMP     108
#define IPPROTO_SCTP     132
#define IPPROTO_UDPLITE  136
#define IPPROTO_RAW      255
#define IPPROTO_MAX      256

/* ============================================================
 * IP OPTIONS
 * ============================================================
 */
#define IP_TOS             1
#define IP_TTL             2
#define IP_HDRINCL         3
#define IP_OPTIONS         4
#define IP_ROUTER_ALERT    5
#define IP_RECVOPTS        6
#define IP_RETOPTS         7
#define IP_PKTINFO         8
#define IP_PKTOPTIONS      9
#define IP_MTU_DISCOVER    10
#define IP_RECVERR         11
#define IP_RECVTTL         12
#define IP_RECVTOS         13
#define IP_MTU             14
#define IP_FREEBIND        15
#define IP_IPSEC_POLICY    16
#define IP_XFRM_POLICY     17
#define IP_PASSSEC         18
#define IP_TRANSPARENT     19
#define IP_ORIGDSTADDR     20
#define IP_RECVORIGDSTADDR IP_ORIGDSTADDR
#define IP_MINTTL          21
#define IP_NODEFRAG        22
#define IP_CHECKSUM        23
#define IP_BIND_ADDRESS_NO_PORT 24
#define IP_RECVFRAGSIZE    25
#define IP_MULTICAST_IF    32
#define IP_MULTICAST_TTL   33
#define IP_MULTICAST_LOOP  34
#define IP_ADD_MEMBERSHIP  35
#define IP_DROP_MEMBERSHIP 36
#define IP_UNBLOCK_SOURCE  37
#define IP_BLOCK_SOURCE    38
#define IP_ADD_SOURCE_MEMBERSHIP 39
#define IP_DROP_SOURCE_MEMBERSHIP 40
#define IP_MSFILTER        41
#define IP_MULTICAST_ALL   49
#define IP_UNICAST_IF      50

/* ============================================================
 * IPV6 OPTIONS
 * ============================================================
 */
#define IPV6_ADDRFORM      1
#define IPV6_2292PKTINFO   2
#define IPV6_2292HOPOPTS   3
#define IPV6_2292DSTOPTS   4
#define IPV6_2292RTHDR     5
#define IPV6_2292PKTOPTIONS 6
#define IPV6_CHECKSUM      7
#define IPV6_2292HOPLIMIT  8
#define IPV6_NEXTHOP       9
#define IPV6_AUTHHDR       10
#define IPV6_UNICAST_HOPS  16
#define IPV6_MULTICAST_IF  17
#define IPV6_MULTICAST_HOPS 18
#define IPV6_MULTICAST_LOOP 19
#define IPV6_JOIN_GROUP    20
#define IPV6_LEAVE_GROUP   21
#define IPV6_ROUTER_ALERT  22
#define IPV6_MTU_DISCOVER  23
#define IPV6_MTU           24
#define IPV6_RECVERR       25
#define IPV6_V6ONLY        26
#define IPV6_JOIN_ANYCAST  27
#define IPV6_LEAVE_ANYCAST 28
#define IPV6_IPSEC_POLICY  34
#define IPV6_XFRM_POLICY   35
#define IPV6_HDRINCL       36

#endif /* LIBC_NETINET_IN_H */
