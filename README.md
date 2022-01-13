# DNS-Server

This is a miniature DNS server that will serve AAAA queries. 
It accepts a DNS AAAA query over TCP on port 8053 and forwards it to a server whose IPv4 address is the first command line argument and whose port is the second command line argument.
The program is ready to accept another query as soon as it has processed the previous one. All the seperate TCP connection for each query/response with the client is logged to standard output.

The program can be run via the Makefile which will produce the executable named 'dns_svr' 
