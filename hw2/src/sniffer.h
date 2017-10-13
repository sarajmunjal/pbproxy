//
// Created by Saraj Munjal on 10/2/17.
//
#include <sys/types.h>
#include <arpa/inet.h>
#ifndef HW2_SNIFFER_H
#define HW2_SNIFFER_H


#define ETHER_ADDR_LEN	6

/* Ethernet header */
typedef struct ethernet_header {
    u_char ether_dhost[ETHER_ADDR_LEN]; /* Destination host address */
    u_char ether_shost[ETHER_ADDR_LEN]; /* Source host address */
    u_short ether_type; /* IP? ARP? RARP? etc */
} ethernet_header_t;

/* IP header */
typedef struct ip_header {
    u_char ip_vhl;		/* version << 4 | header length >> 2 */
    u_char ip_tos;		/* type of service */
    u_short ip_len;		/* total length */
    u_short ip_id;		/* identification */
    u_short ip_off;		/* fragment offset field */
#define IP_RF 0x8000		/* reserved fragment flag */
#define IP_DF 0x4000		/* dont fragment flag */
#define IP_MF 0x2000		/* more fragments flag */
#define IP_OFFMASK 0x1fff	/* mask for fragmenting bits */
    u_char ip_ttl;		/* time to live */
    u_char ip_p;		/* protocol */
    u_short ip_sum;		/* checksum */
    struct in_addr ip_src,ip_dst; /* source and dest address */
} ip_header_t;


#define IP_HL(ip)		(((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)		(((ip)->ip_vhl) >> 4)

/* TCP header */
typedef u_int tcp_seq;

typedef struct tcp_header {
    u_short th_sport;	/* source port */
    u_short th_dport;	/* destination port */
    tcp_seq th_seq;		/* sequence number */
    tcp_seq th_ack;		/* acknowledgement number */
    u_char th_offx2;	/* data offset, rsvd */
#define TH_OFF(th)	(((th)->th_offx2 & 0xf0) >> 4)
    u_char th_flags;
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
    u_short th_win;		/* window */
    u_short th_sum;		/* checksum */
    u_short th_urp;		/* urgent pointer */
} tcp_header_t;

typedef struct udp_header {
    u_short uh_sport;	/* source port */
    u_short uh_dport;	/* destination port */
    u_short uh_len;		/* length */
    u_short uh_sum;		/* checksum */
} udp_header_t;

typedef struct icmp_header {
    u_char type;
    u_char code;
    u_short sum; // checksum
    u_int roh; // rest of header
} icmp_header_t;

typedef struct custom_args {
    char *interface_name;
    char *input_file_name;
    char *payload_search_string;
    char *expression;
} custom_args;
#endif //HW2_SNIFFER_H
