#include <iostream>
#include <pcap.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "sniffer.h"

#define SIZE_ETHERNET 14

void print_payload(u_char *payload, int size_payload) {
    printf("Payload(size:%d) : %s\n", size_payload, payload);
}

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {

    static int count = 1;                   /* packet counter */
    FILE *fp = (FILE *) args;
    /* declare pointers to packet headers */
    const ethernet_header_t *ethernet_header;  /* The ethernet header [1] */
    const ip_header_t *ip;              /* The IP header */
    const tcp_header_t *tcp;            /* The TCP header */
    u_char *payload;                    /* Packet payload */

    int size_ip;
    int size_tcp;
    int size_payload;

    fprintf(fp, "\nPacket number %d:\n", count);
    count++;

    /* define ethernet header */
    ethernet_header = (ethernet_header_t *) (packet);

    /* define/compute ip header offset */
    ip = (ip_header_t *) (packet + SIZE_ETHERNET);
    size_ip = IP_HL(ip) * 4;
    if (size_ip < 20) {
        fprintf(fp, "   * Invalid IP header length: %u bytes\n", size_ip);
        return;
    }

    /* print source and destination IP addresses */
    fprintf(fp, "       From: %s\n", inet_ntoa(ip->ip_src));
    fprintf(fp, "         To: %s\n", inet_ntoa(ip->ip_dst));

    /* determine protocol */
    switch (ip->ip_p) {
        case IPPROTO_TCP:
            fprintf(fp, "   Protocol: TCP\n");
            break;
        case IPPROTO_UDP:
            fprintf(fp, "   Protocol: UDP\n");
            return;
        case IPPROTO_ICMP:
            fprintf(fp, "   Protocol: ICMP\n");
            return;
        case IPPROTO_IP:
            fprintf(fp, "   Protocol: IP\n");
            return;
        default:
            fprintf(fp, "   Protocol: Other\n");
            return;
    }

    /*
     *  OK, this packet is TCP.
     */

    /* define/compute tcp header offset */
    tcp = (tcp_header_t *) (packet + SIZE_ETHERNET + size_ip);
    size_tcp = TH_OFF(tcp) * 4;
    if (size_tcp < 20) {
        fprintf(fp, "   * Invalid TCP header length: %u bytes\n", size_tcp);
        return;
    }

    printf("   Src port: %d\n", ntohs(tcp->th_sport));
    printf("   Dst port: %d\n", ntohs(tcp->th_dport));

    /* define/compute tcp payload (segment) offset */
    payload = (u_char *) (packet + SIZE_ETHERNET + size_ip + size_tcp);

    /* compute tcp payload (segment) size */
    size_payload = ntohs(ip->ip_len) - (size_ip + size_tcp);

    /*
     * Print payload data; it might be binary, so don't just
     * treat it as a string.
     */
    if (size_payload > 0) {
        fprintf(fp, "   Payload (%d bytes):\n", size_payload);
        print_payload(payload, size_payload);
    }

    return;
}

custom_args parse_cli_arguments(int argc, char **argv) {
    custom_args *args = (custom_args *) malloc(sizeof(custom_args));
    int index;
    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "i:r:s:")) != -1) {
        switch (c) {
            case 'i':
                args->interface_name = optarg;
                break;
            case 'r':
                args->input_file_name = optarg;
                break;
            case 's':
                args->payload_search_string = optarg;
                break;
            case '?':
                if (optopt == 'i' || optopt == 'r' || optopt == 's') {
                    fprintf(stderr, "Option -%c requires an argument. Char : %c\n", optopt, c);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
            default:
                abort();
        }
    }


    printf("interface = %s, file_name = %s, string = %s\n",
           args->interface_name, args->input_file_name, args->payload_search_string);
    if (optind < argc -1) {
        args->expression = argv[optind];
    }
    return *args;
}

int main(int argc, char **argv) {
    custom_args args = parse_cli_arguments(argc, argv);

    char errbuf[PCAP_ERRBUF_SIZE];
    char *device = (args.interface_name == NULL) ? pcap_lookupdev(errbuf) : args.interface_name;
    bpf_u_int32 net;
    bpf_u_int32 mask;
    if (device == NULL) {
        std::cout << "Error occurred:" << errbuf << std::endl;
    } else {
        std::cout << "Device found: " << device << std::endl;
    }
    if (pcap_lookupnet(device, &net, &mask, errbuf) == -1) {
        fprintf(stderr, "Can't get netmask for device %s\n", device);
        net = 0;
        mask = 0;
    }
    pcap_t *session;
    if (args.input_file_name != NULL) {
        FILE *input_file = fopen(args.input_file_name, "r");
        session = pcap_fopen_offline(input_file, errbuf);
    } else {
        session = pcap_open_live(device, BUFSIZ, 1, 1000, errbuf);
    }

    if (session == NULL) {
        printf("Failed to open session for device:%s", device);
        return 2;
    }
    struct bpf_program filter;
    if (args.expression != NULL) {
        if (pcap_compile(session, &filter, args.expression, 0, net) == -1) {
            fprintf(stderr, "Couldn't parse filter %s: %s\n", args.expression, pcap_geterr(session));
            return (2);
        } else {
            if (pcap_setfilter(session, &filter) == -1) {
                fprintf(stderr, "Couldn't install filter %s: %s\n", args.expression, pcap_geterr(session));
                return (2);
            }
        }
    }

    struct pcap_pkthdr header;
//    const u_char* packet = pcap_next(session, &header);
    pcap_next(session, &header);
    /* Print its length */
    printf("Jacked a packet with length of [%d]\n", header.len);
    FILE *fp = stdout;
    pcap_loop(session, 100000, got_packet, (u_char *) fp);
    pcap_close(session);
    return 0;
}