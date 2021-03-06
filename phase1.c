#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <libgen.h>
#include <arpa/inet.h>

#define time_size 60
#define name_loc 14
#define AAAA_id 28
#define to_adress 12


/* function prototypes */
void print_bytes(int bt_read, unsigned char* buffer);
void keep_log(FILE* log,int bt_read, unsigned char* buffer, int state);

int main(int argc, char* argv[]) {
	
	int state = 0;
	if (strcmp(argv[1],"query")==0 || strcmp(argv[1],"[query]" )==0){
		state = 0;
	}
	else if (strcmp(argv[1],"response")==0 || strcmp(argv[1],"[response]")==0){
		state = 1;
	}
	
	unsigned char *len_buffer = malloc(sizeof(unsigned char)*2);
	size_t nbytes = 2;
	read(STDIN_FILENO,len_buffer,nbytes);
	
	int all_size = (int)len_buffer[1] + 2;
	
	unsigned char *info_buffer = malloc(sizeof(unsigned char)* all_size);
	nbytes = all_size;
	ssize_t bt_read = read(STDIN_FILENO,info_buffer,nbytes);
	
	
	unsigned char *buffer = malloc(sizeof(unsigned char)* all_size);
    buffer[0] = len_buffer[0];
    buffer[1] = len_buffer[1];
	for (int i = 2; i<all_size;i++){
    	buffer[i] = info_buffer[i-2];
    }
	
	/*
	while((bt_read+bt_read1) < all_size){
    	bt_read = read(STDIN_FILENO,buffer,nbytes);
    }
    */
    // print the buffer
    //printf("%zd\n",bt_read);
    //print_bytes(bt_read, buffer);
    
    FILE *log;
    log = fopen("dns_svr.log","w");
    
    // create and write the log
    keep_log(log,bt_read,buffer,state);
	
    
    return 0;
}


void keep_log(FILE* log,int bt_read, unsigned char* buffer, int state){
	
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
    	//fprintf(log,"%s",name);
    	
    	for(int n = 0; n<total_len; n++){
    		fprintf(log,"%c",name[n]);
    		fflush(log);
    	}
    }
    
    // logging response
    else if (state == 1){
    	//fprintf(log,"%s",name);
    	for(int n = 0; n<total_len; n++){
    		fprintf(log,"%c",name[n]);
    		fflush(log);
    	}
    	fprintf(log," is at ");
    	fflush(log);
    	
    	fprintf(log,"%s",ip_string);
    	fflush(log);
    		
		
    }
    
    else if (state == -1){
    	fprintf(log,"requested ");
    	fflush(log);
    	//fprintf(log,"%s",name);
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
    }   
    
    fclose(log);
}

// print bytes stored in buffer
void print_bytes(int bt_read, unsigned char* buffer){

    unsigned char char_to_print;
    
    for (int i = 0; i< bt_read; i++){
    	 
    	char_to_print = buffer[i];
    	
     	printf("%02x",char_to_print);
             
     	printf(" ");
    }
    printf("\n");
}




