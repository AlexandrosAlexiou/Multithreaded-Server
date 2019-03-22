#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main( int argc, char *argv[]){

  pid_t pid=fork();
  pid_t childPid;
  if(pid<0){
    perror("fork failed.");
    exit(1);
  }
  if(pid== 0){

    childPid = fork();
    if (childPid < 0)
    {
      perror("second fork failed.");
      exit(1);
    }
    else if(childPid == 0){
    // inside of the child
      char * args[]= {"./client","-a","localhost","-i","1","-p",NULL};
      execv(args[0], args);
    }else{
    // inside of the parent
      char * args[]= {"./client","-a","localhost","-i","1","-g",NULL};
      execv(args[0], args);
    }

  }else{
    char * args[]= {"./client","-a","localhost","-i","1","-p",NULL};
    wait(NULL);

  }


  return 1;
}
