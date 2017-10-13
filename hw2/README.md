# Homework 2: CSE 508, Fall 2017
## MyDump : TCPDUMP-like sniffer

### Protocols Supported
1. Link Layer: Ethernet
2. Network Layer: IP, ARP
3. Transport Layer: TCP, UDP, ICMP

### Format of packets


### Arguments supported
1. [-i] (optional): specify name of interface to listen on
2. [-s] (optional): a string filter for payloads. Packets with no payloads are not printed.
3. [-r] (optional): specify filename for input .pcap file
4. expression: expression signifying BPF filter in tcpdump format

Example:

`sudo ./mydump -i en0 -s MyAdmin -r ../docs/hw1.pcap "port 80"`
 
* Name: Saraj Munjal
* NetID: smunjal
* ID #: 111497962