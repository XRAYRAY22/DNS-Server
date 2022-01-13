// Socket code and server frame are sourced from COMP30023 workshop 9

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <ctype.h>


#define TCP_PORT "8053"

#define time_size 60
#define name_loc 14
#define AAAA_id 28
#define to_adress 12
#define SIZE 256

/* function prototypes */
int keep_log(FILE* log,int bt_read, unsigned char* buffer, int state);
void print_bytes(int bt_read, unsigned char* buffer);

int main(int argc, char *argv[]) {
	int sockfd, newsockfd, re, s;
	
	struct addrinfo hints, *res;
	struct sockaddr_storage client_addr;
	socklen_t client_addr_size;
	
	// Create address we're going to listen on
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;       // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept
	// node (NULL means any interface), service (port), hints, res
	s = getaddrinfo(NULL, TCP_PORT, &hints, &res);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	// Create socket
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Reuse port if possible
	re = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	
	// Bind address to the socket
	if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(res);

	// Listen on socket - means we're ready to accept connections,
	// incoming connection requests will be queued, man 3 listen
	if (listen(sockfd, 5) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	int sockfd1, up_s;
	struct addrinfo hints1, *servinfo, *rp;
		
	// Create address
	memset(&hints1, 0, sizeof hints1);
	hints1.ai_family = AF_INET;
	hints1.ai_socktype = SOCK_STREAM;
		
	// Get adress of upsteam server
	up_s = getaddrinfo(argv[1], argv[2], &hints1, &servinfo);
	if (up_s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(up_s));
		exit(EXIT_FAILURE);
	}
	
	// Connect to first valid result
	// Why are there multiple results? see man page (search 'several reasons')
	// How to search? enter /, then text to search for, press n/N to navigate
	for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
		sockfd1 = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd1 == -1)
			continue;
	
		if (connect(sockfd1, rp->ai_addr, rp->ai_addrlen) != -1)
			break; // success
	
		close(sockfd1);
	}
	
	if (rp == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(servinfo);
	
	FILE *log;
    log = fopen("dns_svr.log","w");
	int state = 0, state1 = 0;
	
	// read from the connection, then process
	while (1) {
		
		unsigned char buff[SIZE];
		unsigned char buffer[SIZE];
		unsigned char up_buffer[SIZE];
		
		// Accept a connection - blocks until a connection is ready to be accepted
		// Get back a new file descriptor to communicate on
		client_addr_size = sizeof client_addr;
		newsockfd =
			accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
		if (newsockfd < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		
		size_t nbytes = sizeof(buffer);
		int bt_read = read(newsockfd,buff,nbytes);
		
		// Keeping query clean
		for (int j = 0; j<bt_read;j++){
			buffer[j] = buff[j];
		}
		
		// Add request to log
		if(bt_read!=0){
			state = keep_log(log,bt_read,buffer,0);
		}
		
		
		if (bt_read < 0) {
			perror("ERROR reading from socket");
			exit(EXIT_FAILURE);
		}
		
		// Send message to upstream server
		int n = write(sockfd1, buff, bt_read);
		if (n < 0) {
			perror("socket");
			exit(EXIT_FAILURE);
		}
		
		// Read respone from upstream server
		size_t nbytes1 = sizeof(up_buffer);
		int bt_read1 = read(sockfd1 ,up_buffer,nbytes1);
		
		// Add response to log
		if (bt_read1!=0 && state == 0){
			state1 = keep_log(log,bt_read1,up_buffer,1);
		}
		
		if (bt_read1 < 0) {
			perror("ERROR reading from socket");
			exit(EXIT_FAILURE);
		}
		
		
		// send response from upstream to client (dig)
		int up_n = write(newsockfd, up_buffer, bt_read1);
		if (up_n < 0) {
			perror("write");
			exit(EXIT_FAILURE);
		}		
	}
	
	fclose(log);
	
	close(sockfd);
	close(newsockfd);
	
	return 0;
}

//handles the log file
int keep_log(FILE* log,int bt_read, unsigned char* buffer, int state){
	
	// get current time
	char format_time[time_size];
	time_t t = time(NULL);
	struct tm *time = localtime(&t);
	strftime(format_time,sizeof(format_time), "%FT%T%z", time);
    
    // add timestamp
    fprintf(log,"%s ",format_time);
    fflush(log);
    
    // get the name of the request/response
    int total_len = 0,name_len = 0;
    int len = (int)buffer[name_loc];
    int loc = name_loc+1;
    int done = 0;
    
    char *name = malloc(sizeof(char)*0);
    	
    while((int)buffer[loc]!= 0 && len != 0){
    	total_len+=len;
    	name = realloc(name, sizeof(char)*total_len);
    		
    	for (;name_len<total_len;name_len++){
    		name[name_len] = (char)buffer[loc];
    		loc++;
    	}
    	len = (int)buffer[loc];
    	loc++;
    		
    	if((int)buffer[loc]== 0 && len == 0){
    		done = 1;
    	} 
    		
    	if (done == 0){
    		name = realloc(name, (sizeof(char)*total_len) + sizeof(char));
    		name[total_len]='.';
    		total_len+=1;
    		name_len+=1;
    	}			
    }
    
    // check if request is of type AAAA
    loc++;
    if ((int)buffer[loc]!= AAAA_id || (int)buffer[loc-1]!= 0){
    	state =  -1;
    }
    loc++;	
    
    // Get IP adress
    loc += to_adress;
    loc++;
    int ip_len = (int)buffer[loc];
    loc++;
    int ad_len = ip_len + loc;
    int cc = 0;
    unsigned char* ip_buffer = malloc(sizeof(unsigned char)*ip_len); 
    
    for(int ip_loc = loc;ip_loc < ad_len;ip_loc++){
    	ip_buffer[cc] = buffer[ip_loc];
    	cc++;
    }
    
    char ip_string[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6,ip_buffer,ip_string,INET6_ADDRSTRLEN);
    
    
    // logging query
    if (state == 0){
    	fprintf(log,"requested ");
    	fflush(log);
    	
    	for(int n = 0; n<total_len; n++){
    		fprintf(log,"%c",name[n]);
    		fflush(log);
    	}
    	
    	fprintf(log,"\n");
    	fflush(log);
    }
    
    // logging response
    else if (state == 1){
    	
    	for(int n = 0; n<total_len; n++){
    		fprintf(log,"%c",name[n]);
    		fflush(log);
    	}
    	fprintf(log," is at ");
    	fflush(log);
    	
    	fprintf(log,"%s",ip_string);
    	fflush(log);
    	
    	fprintf(log,"\n");
    	fflush(log);
    }
    
    // logging none AAAA
    else if (state == -1){
    	fprintf(log,"requested ");
    	fflush(log);
    	
    	for(int n = 0; n<total_len; n++){
    		fprintf(log,"%c",name[n]);
    		fflush(log);
    	}
    	
    	fprintf(log,"\n");
    	fflush(log);
    	
    	fprintf(log,"%s ",format_time);
    	fflush(log);
    	
    	fprintf(log,"unimplemented request");
    	fflush(log);
    	
    	fprintf(log,"\n");
    	fflush(log);
    	
    	return 1;
    }
    return 0;
}

// print bytes stored in buffer in hex format
void print_bytes(int bt_read, unsigned char* buffer){

    unsigned char char_to_print;
    
    for (int i = 0; i< bt_read; i++){
    	 
    	char_to_print = buffer[i];
    	
     	printf("%02x",char_to_print);
             
     	printf(" ");
    }
    printf("\n");
}
