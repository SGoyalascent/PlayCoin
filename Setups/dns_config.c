
#include <unistd.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#define   RAIDA_SERVER_MAX      25
#define   DNS_LEN_MAX           4
#define   DNS_PORT_MAX          2

char execpath[256];

struct server_config {
	unsigned char raida_id;
	unsigned int port_number;
};
struct server_config server_config_obj;

void getexepath() {
 unsigned char buff[256];
  int count = readlink( "/proc/self/exe", buff, 256);
  int i=0,slash_pos;
  while(buff[i]!='\0'){
	if(buff[i]=='/'){
		slash_pos = i;
	}
  i++;
  }	
  strncpy(execpath,buff,slash_pos);
}

void main(){

	FILE *fp_inp = NULL;
	
  char *file_names[25]={"/opt/raida0/Data/dns.bin",
                        "/opt/raida1/Data/dns.bin",
                        "/opt/raida2/Data/dns.bin",
                        "/opt/raida3/Data/dns.bin",
                        "/opt/raida4/Data/dns.bin",
                        "/opt/raida5/Data/dns.bin",
                        "/opt/raida6/Data/dns.bin",
                        "/opt/raida7/Data/dns.bin",
                        "/opt/raida8/Data/dns.bin",
                        "/opt/raida9/Data/dns.bin",
                        "/opt/raida10/Data/dns.bin",
                        "/opt/raida11/Data/dns.bin",
                        "/opt/raida12/Data/dns.bin",
                        "/opt/raida13/Data/dns.bin",
                        "/opt/raida14/Data/dns.bin",
                        "/opt/raida15/Data/dns.bin",
                        "/opt/raida16/Data/dns.bin",
                        "/opt/raida17/Data/dns.bin",
                        "/opt/raida18/Data/dns.bin",
                        "/opt/raida19/Data/dns.bin",
                        "/opt/raida20/Data/dns.bin",
                        "/opt/raida21/Data/dns.bin",
                        "/opt/raida22/Data/dns.bin",
                        "/opt/raida23/Data/dns.bin",
                        "/opt/raida24/Data/dns.bin"};


  //getexepath();
  union coversion{
    unsigned char data[4];
    unsigned char data2[2];
    uint32_t val32;
  }convObj;

  int dns_size = RAIDA_SERVER_MAX*(DNS_LEN_MAX+DNS_PORT_MAX);
  for(int i = 0; i < 25; i++) {

    unsigned char buffer[dns_size];
    char path[256];
    strcpy(path, file_names[i]);
    printf("path: %s\n", path);
    
    int port = 30000;
    int ind = 0;
    for(int j=0; j < RAIDA_SERVER_MAX; j++) {
      buffer[ind+0] = 127;
      buffer[ind+1] = 0;
      buffer[ind+2] = 0;
      buffer[ind+3] = 1;
      convObj.val32 = port+j;
      buffer[ind+4] = convObj.data2[1];
      buffer[ind+5] = convObj.data2[0];
      //printf("port: %d   port_bytes: %d  %d\n", port, buffer[ind+4],buffer[ind+5]);
      ind = ind + DNS_LEN_MAX+DNS_PORT_MAX;
    }
    if ((fp_inp = fopen(path, "wb")) == NULL) {
      perror("fwrite: shards.bin Cannot be opened , exiting \n");
    }
    fwrite(buffer, 1, dns_size, fp_inp);
    fclose(fp_inp);
    
//------------------------------------------------------------

  int j=0,index=0;
  unsigned int dns_port;
  char dns_ip[64],tmp[16];
  unsigned char buff[RAIDA_SERVER_MAX*(DNS_LEN_MAX+DNS_PORT_MAX)];
  
  if ((fp_inp = fopen(path, "rb")) == NULL) {
    printf("fread: dns.bin Cannot be opened , exiting \n");
  }
  if(fread(buff, 1, RAIDA_SERVER_MAX*(DNS_LEN_MAX+DNS_PORT_MAX), fp_inp)<(RAIDA_SERVER_MAX*(DNS_LEN_MAX+DNS_PORT_MAX))){
    printf("Configuration parameters missing in dns.bin \n");
  }
  printf("------------------------------\n");
  printf("RAIDA-%d   DNS Configuration Details..\n", i);
  printf("------------------------------\n");
  index =0;	
  for (int k=0;k<RAIDA_SERVER_MAX;k++) {
    strcpy(dns_ip,"");
    for (j=0; j<DNS_LEN_MAX; j++) {
      sprintf(tmp, "%d", buff[index+j]);
      strcat(dns_ip,tmp);
      if(j<DNS_LEN_MAX-1)
        strcat(dns_ip,"."); 
    }
    index=index+DNS_LEN_MAX;
    convObj.val32 = 0;
    for (j=0; j<DNS_PORT_MAX; j++) {
      convObj.data[DNS_PORT_MAX-1-j] = buff[index+j];	
    }
    index=index+DNS_PORT_MAX;
    dns_port = convObj.val32;
    /*
    if(server_config_obj.raida_id!=k){
      //-----init_dns_socket(i,dns_port,dns_ip);
    } */
    
    if(i != k){
      printf("init_dns_socket\n");
      //-----init_dns_socket(i,dns_port,dns_ip);
    }
    printf("Raida :- %d \tdns :-%s    \t Port Number :-% d\n",k+1, dns_ip,dns_port);
  }
  fclose(fp_inp);
  }
}