# Homework 3: CSE 508, Fall 2017
## PbProxy : Plugboard proxy

### Description
An encrypted proxy client and server that makes an encrypted tunnel to access a remote service

### Build instructions
1. `cd PROJECT_DIR`
2. `make clean`
3. `make`
4. Run using `./bin/pbproxy [options]`

### Arguments supported
1. [-k] (compulsory): path to file containing encryption key
2. [-l] (optional): incoming connection port for listening in server mode 
3. host (compulsory): hostname of the service to connect to (in both client/server mode)
4. port (compulsory): port of the service to connect to (in both client/server mode)

####Example: <br>
##### In client mode
1. If connecting to a pbproxy service on localhost:2234 <br>
`./bin/pbproxy -k mykey localhost 2234`
2. If tunneling SSH (-v for verbose SSH) via PbProxy service on localhost:2234<br>
`ssh -o 'ProxyCommand ./bin/pbproxy -k  mykey localhost 2234' -v localhost`

##### In server mode
1. Listen to connection on port 2234 and forward to localhost:1234
`./bin/pbproxy -k mykey -l 2234 localhost 1234`
2. If tunneling SSH <br>
`./bin/pbproxy -k mykey -l 2234 localhost 22`

### How it works
##### 1. Client Mode
First, the client connects to remote server and sends 8 random bytes of IV (plaintext, non-encrypted).
This IV is sent once by each client at the start of its session and is used for AES128CTR mode encryption and decryption. <br> 
Henceforth, the client works with two threads: one to infinitely read from a non-blocking stdin file descriptor and forward any input to remote server,
and another that poll the remote server socket and reads data to stdout if it is ready.

##### 2. Server Mode
The PbProxy server first establishes a connection with the target service.<br> 
Then, in a while(1) loop, it waits for a connection from a client.
On establishing a client connection, it waits to receive an 8 byte IV from the client for this session's encryption.
This IV is received once per client session and is used for AES128CTR mode encryption and decryption. <br>
Henceforth, the PbProxy server infintely does the following 2 steps: <br>
a.) polls the client for data. If ready, it reads this data, decrypts it, and forwards it to the target service.<br>
b.) polls the target service for data. If ready, it reads this data, encrypts it and forwards it to the client.<br>
These poll() calls have a timeout to prevent busy waiting.

##### Parameters
Key length: 16 bytes (found in file myfile) <br>
IV length: 8 bytes (this is the partial IV that is communicated once per session) <br>
Buffer size for reading/writing: 4 KB
 
### Dev/test environment details
1. OS: Ubuntu 16.04.3 LTS, Linux 4.10.0-37-generic x86_64
2. Compiler: gcc version 5.4.0 20160609 (Ubuntu 5.4.0-6ubuntu1~16.04.4), GNU Make 4.1

(PS: the "br" tags are just for markdown formatting purpose, not in real output)
### Student details
* Name: Saraj Munjal
* NetID: smunjal
* ID #: 111497962

### References
[1] Multiple posts on [www.stackoverflow.com](https://www.stackoverflow.com) <br>
[2] Socket communication tutorial [http://www.thegeekstuff.com/2011/12/c-socket-programming/](http://www.thegeekstuff.com/2011/12/c-socket-programming/) <br>
[3] Linux man pages for read, recv, poll, select like [http://man7.org/linux/man-pages/man2/read.2.html](http://man7.org/linux/man-pages/man2/read.2.html) <br>
[4] Tutorial for AES CTR encryption [http://www.gurutechnologies.net/blog/aes-ctr-encryption-in-c/](http://www.gurutechnologies.net/blog/aes-ctr-encryption-in-c/) <br>