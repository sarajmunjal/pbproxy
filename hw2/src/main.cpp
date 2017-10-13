#include <iostream>
#include <pcap.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "sniffer.h"
#include <ctime>
#include <cmath>

#define SIZE_ETHERNET 14

#define ETHER_TYPE_IP 0x800
#define ETHER_TYPE_ARP 0x806
#define UDP_HEADER_SIZE 8
#define ICMP_HEADER_SIZE 8

typedef struct packet_rcvd_args {
    char *payload_search_string;
    FILE *output_fp;
} packet_rcvd_args_t;

typedef struct print_packet_info {
    FILE *output_fp;
    timeval ts;
    u_char *ether_src;
    u_char *ether_dst;
    u_short ether_type;
    char *protocol;
    int packet_len;
    u_short s_port;
    u_short d_port;
    struct in_addr ip_src;
    struct in_addr ip_dst;
    int size_payload;
    u_char *payload;
} print_packet_info_t;

void print_payload(FILE *fp, u_char *payload, int payload_size) {
    char *str = (char *) payload;
    int num_lines = (int) ceil((float) payload_size / 16);
    char buf[17];
    for (int i = 0; i < num_lines; i++) {
        memcpy(buf, str + (16 * i), 16);
        buf[16] = '\0';
        int j = 0;
        int line_length = (i == num_lines - 1) ? payload_size % 16 : 16;
        for (j = 0; j < line_length; j++) {
            fprintf(fp, "%02X ", (unsigned char) buf[j]);
        }
        int num_spaces = 3 * (16 - line_length) + 3;
        for (int k = 0; k < num_spaces; k++) {
            fprintf(fp, " ");
        }
        for (j = 0; j < line_length; j++) {
            fprintf(fp, "%c", isprint(buf[j]) ? buf[j] : '.');
        }

        fprintf(fp, "\n");
    }
}

bool is_payload_present(u_char *payload, int payload_size, char *pattern) {
    int n = payload_size;
    int m = strlen(pattern);
    int i = 0;
    int j = 0;
    while (i < n) {
        if (payload[i] == pattern[j]) {
            if (j == m - 1) {
                return true;
            }
            j++;
        } else {
            j = 0;
        }
        i++;
    }
    return false;
}

char *get_mac_address(u_char *arr) {
    char *buffer = (char *) malloc(sizeof(char) * 20);
    snprintf(buffer, 20, "%02x:%02x:%02x:%02x:%02x:%02x", arr[0], arr[1], arr[2], arr[3], arr[4], arr[5]);
    return buffer;
}

char *get_formatted_time(timeval ts) {
    struct timeval tv;
    time_t nowtime;
    struct tm *nowtm;
    char *tmbuf = (char *) malloc(64 * sizeof(char));
    char *buf = (char *) malloc(64 * sizeof(char));
    nowtime = ts.tv_sec;
    nowtm = localtime(&nowtime);
    strftime(tmbuf, 64, "%Y-%m-%d %H:%M:%S", nowtm);
    unsigned long usec = (unsigned long) tv.tv_usec;
    while (usec >= 1000000 - 1) {
        usec = usec / 10;
    }
    snprintf(buf, 64, "%s.%ld", tmbuf,usec);
    return buf;
}

void print_packet_data(print_packet_info_t *info) {
    /* define/compute ip header offset */
    FILE *fp = info->output_fp;
    fprintf(fp, "%s %s -> %s type 0x%03x len %d\n", get_formatted_time(info->ts), get_mac_address(info->ether_src),
            get_mac_address(info->ether_dst), info->ether_type, info->packet_len);

    /* print source and destination IP addresses */
    if (info->ether_type == ETHER_TYPE_IP) {
        if (info->s_port != (u_short) -1 && info->d_port != (u_short) -1) {
            fprintf(fp, "%s:%d -> %s:%d %s\n", inet_ntoa(info->ip_src), ntohs(info->s_port),
                    inet_ntoa(info->ip_dst), ntohs(info->d_port), info->protocol);
        } else {
            fprintf(fp, "%s -> %s %s\n", inet_ntoa(info->ip_src), inet_ntoa(info->ip_dst), info->protocol);
        }
    }
    if (info->size_payload > 0) {
        print_payload(fp, info->payload, info->size_payload);
    }
    fprintf(fp, "\n");
}

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {

    packet_rcvd_args_t *packet_rcvd_args = (packet_rcvd_args_t *) args;
    char *payload_search_string = packet_rcvd_args->payload_search_string;
    const ethernet_header_t *ethernet_header;  /* The ethernet header [1] */

    print_packet_info_t *info = (print_packet_info_t *) malloc(sizeof(print_packet_info_t));
    info->output_fp = packet_rcvd_args->output_fp;
    ethernet_header = (ethernet_header_t *) (packet);
    info->ts = header->ts;
    info->ether_src = (u_char *) ethernet_header->ether_shost;
    info->ether_dst = (u_char *) ethernet_header->ether_dhost;
    info->ether_type = ntohs(ethernet_header->ether_type);
    info->s_port = (u_short) -1;
    info->d_port = (u_short) -1;
    switch (info->ether_type) {
        default:
            return;

        case ETHER_TYPE_ARP: {
            int total_len = header->len;
            info->packet_len = header->len;
            info->size_payload = total_len - SIZE_ETHERNET;
            info->payload = (u_char *) (packet + SIZE_ETHERNET);
        }
            break;

        case ETHER_TYPE_IP: {
            int size_ip;
            int size_protocol = 0;

            const ip_header_t *ip;
            ip = (ip_header_t *) (packet + SIZE_ETHERNET);
            size_ip = IP_HL(ip) * 4;
            info->packet_len = size_ip + SIZE_ETHERNET;
            const u_char *packet_base_addr = packet + SIZE_ETHERNET + size_ip;
            switch (ip->ip_p) {
                case IPPROTO_TCP: {
                    info->protocol = (char *) "TCP";
                    tcp_header_t *tcp = (tcp_header_t *) packet_base_addr;
                    size_protocol = TH_OFF(tcp) * 4;
                    info->s_port = tcp->th_sport;
                    info->d_port = tcp->th_dport;
                }
                    break;
                case IPPROTO_UDP: {
                    info->protocol = (char *) "UDP";
                    udp_header_t *udp = (udp_header_t *) packet_base_addr;
                    size_protocol = UDP_HEADER_SIZE;
                    info->s_port = udp->uh_sport;
                    info->d_port = udp->uh_dport;
                }
                    break;
                case IPPROTO_ICMP:
                    info->protocol = (char *) "ICMP";
                    size_protocol = ICMP_HEADER_SIZE;
                    break;
                case IPPROTO_IP:
                    info->protocol = (char *) "IP";
                    break;
                default:
                    info->protocol = (char *) "OTHER";
                    break;
            }
            info->packet_len += size_protocol;
            info->ip_src = ip->ip_src;
            info->ip_dst = ip->ip_dst;
            info->payload = (u_char *) (packet + SIZE_ETHERNET + size_ip + size_protocol);
            info->size_payload = ntohs(ip->ip_len) - (size_ip + size_protocol);
        }
            break;

    }

    if (payload_search_string != NULL && strlen(payload_search_string) > 0) {
        if (info->size_payload == 0 || !is_payload_present(info->payload, info->size_payload, payload_search_string)) {
            return;
        }
    }
    print_packet_data(info);
}

custom_args parse_cli_arguments(int argc, char **argv) {
    custom_args *args = (custom_args *) malloc(sizeof(custom_args));
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
    args->expression = argv[optind];
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
            } else {
                fprintf(stdout, "Installed expression %s\n", args.expression);
            }
        }
    }

    packet_rcvd_args_t *packet_rcvd_args = (packet_rcvd_args_t *) malloc(sizeof(packet_rcvd_args));
    packet_rcvd_args->output_fp = stdout;
    packet_rcvd_args->payload_search_string = args.payload_search_string;
    pcap_loop(session, 100000, got_packet, (u_char *) packet_rcvd_args);
    pcap_close(session);
    return 0;
}