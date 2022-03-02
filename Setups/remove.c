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

    char *file_names[25]={  "/opt/raida0/playcoin",
                            "/opt/raida1/playcoin",
                            "/opt/raida2/playcoin",
                            "/opt/raida3/playcoin",
                            "/opt/raida4/playcoin",
                            "/opt/raida5/playcoin",
                            "/opt/raida6/playcoin",
                            "/opt/raida7/playcoin",
                            "/opt/raida8/playcoin",
                            "/opt/raida9/playcoin",
                            "/opt/raida10/playcoin",
                            "/opt/raida11/playcoin",
                            "/opt/raida12/playcoin",
                            "/opt/raida13/playcoin",
                            "/opt/raida14/playcoin",
                            "/opt/raida15/playcoin",
                            "/opt/raida16/playcoin",
                            "/opt/raida17/playcoin",
                            "/opt/raida18/playcoin",
                            "/opt/raida19/playcoin",
                            "/opt/raida20/playcoin",
                            "/opt/raida21/playcoin",
                            "/opt/raida22/playcoin",
                            "/opt/raida23/playcoin",
                            "/opt/raida24/playcoin"};

  for(int i=0; i <25; i++) {
    
    if (remove(file_names[i]) == 0) {
        printf("The file is deleted successfully\n.");
    } else {
        printf("The file is not deleted.\n");
    }

  }
}