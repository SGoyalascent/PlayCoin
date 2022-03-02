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

    char *file_names[25]={  "/opt/raida0/",
                            "/opt/raida1/",
                            "/opt/raida2/",
                            "/opt/raida3/",
                            "/opt/raida4/",
                            "/opt/raida5/",
                            "/opt/raida6/",
                            "/opt/raida7/",
                            "/opt/raida8/",
                            "/opt/raida9/",
                            "/opt/raida10/",
                            "/opt/raida11/",
                            "/opt/raida12/",
                            "/opt/raida13/",
                            "/opt/raida14/",
                            "/opt/raida15/",
                            "/opt/raida16/",
                            "/opt/raida17/",
                            "/opt/raida18/",
                            "/opt/raida19/",
                            "/opt/raida20/",
                            "/opt/raida21/",
                            "/opt/raida22/",
                            "/opt/raida23/",
                            "/opt/raida24/"};

  for(int i=0; i <25; i++) {

    char cpdir_path[256];

    strcpy(cpdir_path, "cp playcoin ");
    strcat(cpdir_path,file_names[i]);
    strcat(cpdir_path, "playcoin");
    system(cpdir_path);
  }

}
