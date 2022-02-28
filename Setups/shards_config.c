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

#define   SHARDS_MAX      32
char execpath[256];
unsigned char shards_config[SHARDS_MAX];
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
	char *file_names[25]={"/deployment_data/raida0/data/shards.bin",
                        "/deployment_data/raida1/data/shards.bin",
						            "/deployment_data/raida2/data/shards.bin",
                        "/deployment_data/raida3/data/shards.bin",
						            "/deployment_data/raida4/data/shards.bin",
                        "/deployment_data/raida5/data/shards.bin",
						            "/deployment_data/raida6/data/shards.bin",
                        "/deployment_data/raida7/data/shards.bin",
						            "/deployment_data/raida8/data/shards.bin",
                        "/deployment_data/raida9/data/shards.bin",
						            "/deployment_data/raida10/data/shards.bin",
                        "/deployment_data/raida11/data/shards.bin",
						            "/deployment_data/raida12/data/shards.bin",
                        "/deployment_data/raida13/data/shards.bin",
						            "/deployment_data/raida14/data/shards.bin",
                        "/deployment_data/raida15/data/shards.bin",
						            "/deployment_data/raida16/data/shards.bin",
                        "/deployment_data/raida17/data/shards.bin",
						            "/deployment_data/raida18/data/shards.bin",
                        "/deployment_data/raida19/data/shards.bin",
						            "/deployment_data/raida20/data/shards.bin",
                        "/deployment_data/raida21/data/shards.bin",
						            "/deployment_data/raida22/data/shards.bin",
                        "/deployment_data/raida23/data/shards.bin",
						            "/deployment_data/raida24/data/shards.bin"};



  getexepath();

  for(int i = 0; i < 25; i++) {

    //printf("RAIDA-Id: %d\n", i);
  //--------------------------------------
  //WRITE
  //---------------------------------------
    
    //printf("WRITE IN FILE\n");
    //FILE *fp_inp = NULL;
    unsigned char buffer[SHARDS_MAX];
    char path[256];
    strcpy(path,execpath);
    strcat(path, file_names[i]);
    printf("path: %s\n", path);
    //strcat(path,"/Data/shards.bin");

    for(int j=0; j < SHARDS_MAX; j++) {
      buffer[j] = 0xff;
    }
    if ((fp_inp = fopen(path, "wb")) == NULL) {
      perror("fwrite: shards.bin Cannot be opened , exiting \n");
    }
    fwrite(buffer, 1, SHARDS_MAX, fp_inp);
    fclose(fp_inp);

  
  //--------------------------------------
  //READ
  //---------------------------------------
    
    //printf("READ FROM FILE\n");
    //FILE *fp_inp = NULL;
    unsigned char buff[SHARDS_MAX];
    //char path[256];
    //strcpy(path,execpath);
    //strcat(path,file_names[i]);
    if ((fp_inp = fopen(path, "rb")) == NULL) {
      perror("fread: shards.bin Cannot be opened , exiting \n");
    }
    if(fread(buff, 1, SHARDS_MAX, fp_inp)<SHARDS_MAX){
      printf("Configuration parameters missing in shards.bin \n");
    }
    printf("------------------------------\n");
    printf(" RAIDA-%d Shards Configuration Details..\n", i);
    printf("------------------------------\n");
    for (int j = 0;j < SHARDS_MAX;j++) {
      shards_config[j] = buff[j];
      if (shards_config[j] == 0xff) {
        printf("Shard no :- %d available\n", j+1);
      }else {
        printf("Shard no :- %d not-available\n", j+1);
      }
    }
    fclose(fp_inp);
  }

}