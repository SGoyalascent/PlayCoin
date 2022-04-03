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


void main() {

    char *file_names[25]={  "/opt/raida0/Data/",
                            "/opt/raida1/Data/",
                            "/opt/raida2/Data/",
                            "/opt/raida3/Data/",
                            "/opt/raida4/Data/",
                            "/opt/raida5/Data/",
                            "/opt/raida6/Data/",
                            "/opt/raida7/Data/",
                            "/opt/raida8/Data/",
                            "/opt/raida9/Data/",
                            "/opt/raida10/Data/",
                            "/opt/raida11/Data/",
                            "/opt/raida12/Data/",
                            "/opt/raida13/Data/",
                            "/opt/raida14/Data/",
                            "/opt/raida15/Data/",
                            "/opt/raida16/Data/",
                            "/opt/raida17/Data/",
                            "/opt/raida18/Data/",
                            "/opt/raida19/Data/",
                            "/opt/raida20/Data/",
                            "/opt/raida21/Data/",
                            "/opt/raida22/Data/",
                            "/opt/raida23/Data/",
                            "/opt/raida24/Data/"};

  for(int i=0; i <25; i++) {

    char path[256],mkdir_path[256];
    
    strcpy(path,file_names[i]);
    strcpy(mkdir_path,"mkdir -p ");
    strcat(mkdir_path,path);
    system(mkdir_path);

    strcpy(path,file_names[i]);
    strcpy(mkdir_path,"chmod -R 777 ");
    strcat(mkdir_path,path);
    system(mkdir_path);
  }

}