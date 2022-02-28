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

#define SERVER_CONFIG_BYTES 			20
#define SECURITY_CHECK_MUL_FACTOR  			1000
#define BACKUP_FREQ_MUL_FACTOR 			10
#define REFRESH_DNS_MUL_FACTOR 			1000

struct server_config {
	unsigned char raida_id;
	unsigned int port_number;
	unsigned int security_check_frequency;
	unsigned int backup_frequency;
	unsigned int refresh_dns;
	unsigned int show_regs_coins_max;
	unsigned int show_denom_coins_max;
	unsigned int show_change_coins_max;
	unsigned int bytes_per_frame;
	unsigned int del_ticket_time_intervel;
	unsigned int del_free_id_time_intervel;	//days
	unsigned char del_encryp2_time_intervel;	//in secs 255 (max), this is the deletion time of the encryption keys set using put key
	unsigned char deno_type;	// denomination type 0, for all , 1 for single 
	unsigned int my_id_coins_cnt;//count of id coins (for communcation between raida's
};

char execpath[256];
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
	struct server_config server_config_obj;
	unsigned char path[256];

	char *file_names[25]={"	/raida0/data/server.bin",
							"/raida1/data/server.bin",
						  "/raida2/data/server.bin",
						  "/raida3/data/server.bin",
						   "/raida4/data/server.bin",
						   "/raida5/data/server.bin",
						   "/raida6/data/server.bin",
						   "/raida7/data/server.bin",
						   "/raida8/data/server.bin",
						   "/raida9/data/server.bin",
						   "/raida10/data/server.bin",
						   "/raida11/data/server.bin",
						   "/raida12/data/server.bin",
						   "/raida13/data/server.bin",
						   "/raida14/data/server.bin",
						   "/raida15/data/server.bin",
						   "/raida16/data/server.bin",
						   "/raida17/data/server.bin",
						   "/raida18/data/server.bin",
						   "/raida19/data/server.bin",
						   "/raida20/data/server.bin",
						   "/raida21/data/server.bin",
						   "/raida22/data/server.bin",
						   "/raida23/data/server.bin",
						   "/raida24/data/server.bin"};	
						   
	
	unsigned char buff[SERVER_CONFIG_BYTES] = {0,0,0,10,6,6,0,150,0,150,0,150,4,0,60,6,30,1,0,1};
	
	getexepath();
	for(int i=0;i<25;i++){
		uint16_t port = 30000;
		getexepath();
		//strcpy(path,"");
		strcpy(path,execpath);
		strcat(path,file_names[i]);
		printf("\n%s\n",path);
		port = port + i;

		buff[0]=(unsigned char)( (i));
		buff[1]=(unsigned char)(port >> 8);
		buff[2]=(unsigned char)port;

		if ((fp_inp = fopen(path, "wb")) == NULL) {    //rb+
			perror("\n server.bin Cannot be opened , exiting \n");
		}
		fwrite(buff ,sizeof(buff),1,fp_inp);
		fclose(fp_inp);
//---------------------------------------------------------------------
		int cnt=0;
		unsigned char buff[SERVER_CONFIG_BYTES];
		//char path[256];
		//strcpy(path,execpath);
		//strcat(path,"/Data/server.bin");
		if ((fp_inp = fopen(path, "rb")) == NULL) {
			printf("server.bin Cannot be opened , exiting \n");
		}
		if(fread(buff, 1, SERVER_CONFIG_BYTES, fp_inp)<SERVER_CONFIG_BYTES){
			printf("Configuration parameters missing in server.bin \n");
		}
		server_config_obj.raida_id=buff[0];
		server_config_obj.port_number = buff[2];
		server_config_obj.port_number|= (((uint16_t)buff[1])<<8);
		server_config_obj.security_check_frequency = buff[3] * SECURITY_CHECK_MUL_FACTOR;
		server_config_obj.backup_frequency = buff[4] * BACKUP_FREQ_MUL_FACTOR;
		server_config_obj.refresh_dns = buff[5] * REFRESH_DNS_MUL_FACTOR;
		server_config_obj.show_regs_coins_max = buff[7];
		server_config_obj.show_regs_coins_max |= (((uint16_t)buff[6])<<8);
		server_config_obj.show_denom_coins_max = buff[9];
		server_config_obj.show_denom_coins_max |= (((uint16_t)buff[8])<<8);
		server_config_obj.show_change_coins_max = buff[11];
		server_config_obj.show_change_coins_max |= (((uint16_t)buff[10])<<8);
		server_config_obj.bytes_per_frame = buff[13];
		server_config_obj.bytes_per_frame |= (((uint16_t)buff[12])<<8);
		server_config_obj.del_ticket_time_intervel = (uint16_t)buff[14];
		server_config_obj.del_free_id_time_intervel = buff[15];
		server_config_obj.del_encryp2_time_intervel = buff[16];
		server_config_obj.deno_type = buff[17];
		server_config_obj.my_id_coins_cnt= buff[19];
		server_config_obj.my_id_coins_cnt |= (((uint16_t)buff[18])<<8);
		printf("------------------------------\n");
		printf("Server Configuration Details..\n");
		printf("------------------------------\n");
		printf("Raida_Id  :-%d \n", server_config_obj.raida_id);
		printf("Port Number :- %d \n", server_config_obj.port_number);
		printf("Security check frequency :-%d\n", server_config_obj.security_check_frequency);
		printf("Backup frequency :- %d \n", server_config_obj.backup_frequency);
		printf("Refresh DNS :-%d \n", server_config_obj.refresh_dns);
		printf("Show register coins max :- %d \n", server_config_obj.show_regs_coins_max);
		printf("Show denom coins max :- %d \n", server_config_obj.show_denom_coins_max);
		printf("Show change coins max :- %d \n", server_config_obj.show_change_coins_max);
		printf("Bytes per UDP Request body :- %d \n",server_config_obj.bytes_per_frame);
		printf("Delete Ticket time intervel in secs :- %d \n",server_config_obj.del_ticket_time_intervel);
		printf("Free ID time intervel in days :- %d \n",server_config_obj.del_free_id_time_intervel);
		printf("Encryption 2 time intervel in secs :- %d \n",server_config_obj.del_encryp2_time_intervel);
		printf("Denomination type :- %d \n",server_config_obj.deno_type);
		printf("My Id coins count :- %d \n", server_config_obj.my_id_coins_cnt);

		fclose(fp_inp);
	}
	return;
}