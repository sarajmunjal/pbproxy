# Homework 2: CSE 508, Fall 2017
## MyDump : TCPDUMP-like sniffer

### Protocols Supported
1. Link Layer: Ethernet
2. Network Layer: IP, ARP
3. Transport Layer: TCP, UDP, ICMP

### Build instructions
1. `cd PROJECT_DIR`
2. `make clean`
3. `make`
4. Run using `sudo ./bin/mydump [options]`

### Arguments supported
1. [-i] (optional): specify name of interface to listen on
2. [-s] (optional): a string filter for payloads. Packets with no payloads are not printed.
3. [-r] (optional): specify filename for input .pcap file
4. expression: expression signifying BPF filter in tcpdump format

Example:

`sudo ./bin/mydump -i en0 -s MyAdmin -r ../docs/hw1.pcap "port 80"`

### Sample output
The output of 
`sudo ./bin/mydump -r ./docs/hw1.pcap tcp | head -n 50`
is <br>

2013-01-12 14:35:49.356201 c4:3d:c7:17:6f:9b -> 00:0c:29:e9:94:8e type 0x800 len 0 <br/>
122.154.101.54:39437 -> 192.168.0.200:443 TCP <br/>

2013-01-12 14:35:49.356201 c4:3d:c7:17:6f:9b -> 00:0c:29:e9:94:8e type 0x800 len 105 <br/>
122.154.101.54:39437 -> 192.168.0.200:443 TCP <br/>
80 67 01 03 01 00 4E 00 00 00 10 00 00 39 00 00    .g....N......9.. <br/>
38 00 00 35 00 00 16 00 00 13 00 00 0A 07 00 C0    8..5............ <br/>
00 00 33 00 00 32 00 00 2F 03 00 80 00 00 05 00    ..3..2../....... <br/>
00 04 01 00 80 00 00 15 00 00 12 00 00 09 06 00    ................ <br/>
40 00 00 14 00 00 11 00 00 08 00 00 06 04 00 80    @............... <br/>
00 00 03 02 00 80 00 00 FF 0D 21 3B D5 B5 7B 08    ..........!;..{. <br/>
01 50 0D C5 A5 C2 C1 AF 38                         .P......8 <br/>

The output of 
`sudo ./bin/mydump -r ./docs/hw1.pcap udp | head -n 23`
is <br>
2013-01-12 11:38:02.243275 c4:3d:c7:17:6f:9b -> 01:00:5e:7f:ff:fa type 0x800 len 300 <br/>
192.168.0.1:1901 -> 239.255.255.250:1900 UDP <br/>
4E 4F 54 49 46 59 20 2A 20 48 54 54 50 2F 31 2E    NOTIFY * HTTP/1. <br/>
31 0D 0A 48 4F 53 54 3A 20 32 33 39 2E 32 35 35    1..HOST: 239.255 <br/>
2E 32 35 35 2E 32 35 30 3A 31 39 30 30 0D 0A 43    .255.250:1900..C <br/>
61 63 68 65 2D 43 6F 6E 74 72 6F 6C 3A 20 6D 61    ache-Control: ma <br/>
78 2D 61 67 65 3D 33 36 30 30 0D 0A 4C 6F 63 61    x-age=3600..Loca <br/>
74 69 6F 6E 3A 20 68 74 74 70 3A 2F 2F 31 39 32    tion: http://192 <br/>
2E 31 36 38 2E 30 2E 31 3A 38 30 2F 52 6F 6F 74    .168.0.1:80/Root <br/>
44 65 76 69 63 65 2E 78 6D 6C 0D 0A 4E 54 3A 20    Device.xml..NT:  <br/>
75 75 69 64 3A 75 70 6E 70 2D 49 6E 74 65 72 6E    uuid:upnp-Intern <br/>
65 74 47 61 74 65 77 61 79 44 65 76 69 63 65 2D    etGatewayDevice- <br/>
31 5F 30 2D 63 34 33 64 63 37 31 37 36 66 39 62    1_0-c43dc7176f9b <br/>
0D 0A 55 53 4E 3A 20 75 75 69 64 3A 75 70 6E 70    ..USN: uuid:upnp <br/>
2D 49 6E 74 65 72 6E 65 74 47 61 74 65 77 61 79    -InternetGateway <br/>
44 65 76 69 63 65 2D 31 5F 30 2D 63 34 33 64 63    Device-1_0-c43dc <br/>
37 31 37 36 66 39 62 0D 0A 4E 54 53 3A 20 73 73    7176f9b..NTS: ss <br/>
64 70 3A 61 6C 69 76 65 0D 0A 53 65 72 76 65 72    dp:alive..Server <br/>
3A 20 55 50 6E 50 2F 31 2E 30 20 55 50 6E 50 2F    : UPnP/1.0 UPnP/ <br/>
31 2E 30 20 55 50 6E 50 2D 44 65 76 69 63 65 2D    1.0 UPnP-Device- <br/>
48 6F 73 74 2F 31 2E 30 0D 0A 0D 0A                Host/1.0....     <br/>


(PS: the "br" tags are just for markdown formatting purpose, not in real output)
### Student details
* Name: Saraj Munjal
* NetID: smunjal
* ID #: 111497962