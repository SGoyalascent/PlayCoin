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

    char *file_names[25]={  "/opt/raida0/Data/playcoin",
                            "/opt/raida1/Data/playcoin",
                            "/opt/raida2/Data/playcoin",
                            "/opt/raida3/Data/playcoin",
                            "/opt/raida4/Data/playcoin",
                            "/opt/raida5/Data/playcoin",
                            "/opt/raida6/Data/playcoin",
                            "/opt/raida7/Data/playcoin",
                            "/opt/raida8/Data/playcoin",
                            "/opt/raida9/Data/playcoin",
                            "/opt/raida10/Data/playcoin",
                            "/opt/raida11/Data/playcoin",
                            "/opt/raida12/Data/playcoin",
                            "/opt/raida13/Data/playcoin",
                            "/opt/raida14/Data/playcoin",
                            "/opt/raida15/Data/playcoin",
                            "/opt/raida16/Data/playcoin",
                            "/opt/raida17/Data/playcoin",
                            "/opt/raida18/Data/playcoin",
                            "/opt/raida19/Data/playcoin",
                            "/opt/raida20/Data/playcoin",
                            "/opt/raida21/Data/playcoin",
                            "/opt/raida22/Data/playcoin",
                            "/opt/raida23/Data/playcoin",
                            "/opt/raida24/Data/playcoin"};

  for(int i=0; i <25; i++) {
    
    if (remove(file_names[i]) == 0) {
        printf("The file is deleted successfully.");
    } else {
        printf("The file is not deleted.");
    }

  }
}