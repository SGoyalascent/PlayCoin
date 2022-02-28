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
	char *file_names[25]={"/opt/raida0/Data/shards.bin",
                        "/opt/raida1/Data/shards.bin",
                        "/opt/raida2/Data/shards.bin",
                        "/opt/raida3/Data/shards.bin",
                        "/opt/raida4/Data/shards.bin",
                        "/opt/raida5/Data/shards.bin",
                        "/opt/raida6/Data/shards.bin",
                        "/opt/raida7/Data/shards.bin",
                        "/opt/raida8/Data/shards.bin",
                        "/opt/raida9/Data/shards.bin",
                        "/opt/raida10/Data/shards.bin",
                        "/opt/raida11/Data/shards.bin",
                        "/opt/raida12/Data/shards.bin",
                        "/opt/raida13/Data/shards.bin",
                        "/opt/raida14/Data/shards.bin",
                        "/opt/raida15/Data/shards.bin",
                        "/opt/raida16/Data/shards.bin",
                        "/opt/raida17/Data/shards.bin",
                        "/opt/raida18/Data/shards.bin",
                        "/opt/raida19/Data/shards.bin",
                        "/opt/raida20/Data/shards.bin",
                        "/opt/raida21/Data/shards.bin",
                        "/opt/raida22/Data/shards.bin",
                        "/opt/raida23/Data/shards.bin",
                        "/opt/raida24/Data/shards.bin"};	


  for(int i = 0; i < 25; i++) {
  
    unsigned char buffer[SHARDS_MAX];
    char path[256];
    strcpy(path, file_names[i]);
    printf("path: %s\n", path);

    for(int j=0; j < SHARDS_MAX; j++) {
      buffer[j] = 0xff;
    }
    if ((fp_inp = fopen(path, "wb")) == NULL) {
      perror("fwrite: shards.bin Cannot be opened , exiting \n");
    }
    fwrite(buffer, 1, SHARDS_MAX, fp_inp);
    fclose(fp_inp);

//-------------------------------------------------------
    
    unsigned char buff[SHARDS_MAX];
  
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