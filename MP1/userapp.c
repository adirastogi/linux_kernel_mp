#include "userapp.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
static char buffer[10];
int main(int argc, char* argv[])
{
    int i=0;
    FILE* proc_file = fopen("/proc/mp1/status", "w+");
    pid_t pid=getpid(); 
    sprintf(buffer,"%d\n",(int)pid);
    if(proc_file){
        int retval = fwrite(buffer,sizeof(char),strlen(buffer),proc_file);   
        printf("Wrote %d bytes to procfile for pid %d",retval,pid);
        fclose(proc_file);
    }

    for(i=0; i<10; i++){
        int j=0;
        for(j=0; j<1000000; j++);
        sleep(25);
    }
	return 0;
}
