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

#define   SERVER_CONFIG_BYTES      20
#define   COIN_CONFIG_BYTES         5
#define   COIN_TABLES_CNT           1

struct coin_config {
	unsigned char coin_id;
	unsigned int page_size;
	unsigned int no_of_pages;
	unsigned char *pages_changed;
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
	
  char *file_names[25]={"/opt/raida0/Data/coin_config.bin",
                        "/opt/raida1/Data/coin_config.bin",
                        "/opt/raida2/Data/coin_config.bin",
                        "/opt/raida3/Data/coin_config.bin",
                        "/opt/raida4/Data/coin_config.bin",
                        "/opt/raida5/Data/coin_config.bin",
                        "/opt/raida6/Data/coin_config.bin",
                        "/opt/raida7/Data/coin_config.bin",
                        "/opt/raida8/Data/coin_config.bin",
                        "/opt/raida9/Data/coin_config.bin",
                        "/opt/raida10/Data/coin_config.bin",
                        "/opt/raida11/Data/coin_config.bin",
                        "/opt/raida12/Data/coin_config.bin",
                        "/opt/raida13/Data/coin_config.bin",
                        "/opt/raida14/Data/coin_config.bin",
                        "/opt/raida15/Data/coin_config.bin",
                        "/opt/raida16/Data/coin_config.bin",
                        "/opt/raida17/Data/coin_config.bin",
                        "/opt/raida18/Data/coin_config.bin",
                        "/opt/raida19/Data/coin_config.bin",
                        "/opt/raida20/Data/coin_config.bin",
                        "/opt/raida21/Data/coin_config.bin",
                        "/opt/raida22/Data/coin_config.bin",
                        "/opt/raida23/Data/coin_config.bin",
                        "/opt/raida24/Data/coin_config.bin"};


  //getexepath();
  struct coin_config *coin_config_obj;

  for(int k = 0; k < 25; k++) {

    unsigned char buffer[COIN_CONFIG_BYTES] = {3, 4, 0, 0,3};
    char path[256];
    strcat(path, file_names[k]);
    printf("path: %s\n", path);

    if ((fp_inp = fopen(path, "wb")) == NULL) {
      perror("fwrite: shards.bin Cannot be opened , exiting \n");
    }
    fwrite(buffer, 1, COIN_CONFIG_BYTES, fp_inp);
    fclose(fp_inp);

//----------------------------------------------------------------

  unsigned int cnt=0,size=0,i=0,index=0,coin_id_max=0;
  unsigned char buff[SERVER_CONFIG_BYTES];
  unsigned int coin_id_cnt;

  if ((fp_inp = fopen(path, "rb")) == NULL) {
    perror("fread: coin_config.bin Cannot be opened , exiting \n");
  }
  fseek(fp_inp, 0L, SEEK_END);
  size = ftell(fp_inp);
  fseek(fp_inp, 0L, SEEK_SET);
  if(fread(buff, 1, size, fp_inp)<size){
    printf("Configuration parameters missing in coin_config.bin \n");
  }
  //printf("size: %u\n",size);

  coin_config_obj = (struct coin_config *) malloc(sizeof(struct coin_config)*(size/COIN_CONFIG_BYTES));
  coin_id_cnt = size/COIN_CONFIG_BYTES;
  if (coin_id_cnt != COIN_TABLES_CNT){
    printf("Configuration parameters missing in coin_config.bin \n");
  }
  index=0;
  printf("------------------------------\n");
  printf("Coin's Configuration Details..\n");
  printf("------------------------------\n");
  printf("Coin id count -: %d\n", coin_id_cnt);
  for(i=0;i<(size/COIN_CONFIG_BYTES);i++){
    if(coin_config_obj[i].coin_id>coin_id_max){
      coin_id_max = coin_config_obj[i].coin_id;
    }
    coin_config_obj[i].coin_id = buff[index+0];
    coin_config_obj[i].page_size = buff[index+2];
    coin_config_obj[i].page_size |= (((uint16_t)buff[index+1])<<8);
    coin_config_obj[i].no_of_pages = buff[index+4];
    coin_config_obj[i].no_of_pages |= (((uint16_t)buff[index+3])<<8);
    //printf("buff_%d: %d %d %d %d %d\n",i, buff[index+0], buff[index+1],buff[index+2], buff[index+3], buff[index+4]);
    printf("Coin Id  -: %d ", coin_config_obj[i].coin_id);
    printf("\t AN's per page  -: %d", coin_config_obj[i].page_size);
    printf("\t No of Pages    -: %d\n", coin_config_obj[i].no_of_pages);
    index+=COIN_CONFIG_BYTES;
  }
  //coin_id_obj = (struct coin_id *) malloc(sizeof(struct coin_id)*(coin_id_max+1));
  fclose(fp_inp);
  }
    

}