#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main( int argc, char *argv[]){
    signal(SIGCHLD,SIG_IGN);
  // Creating first child
  int n1 = fork();

  // Creating second child. First child
  // also executes this line and creates
  // grandchild.
  int n2 = fork();

  if (n1 > 0 && n2 > 0) {
    //parent
    wait(NULL);
  }
  else if (n1 == 0 && n2 > 0)
  {
    //first child
    char * args[]= {"./client","-a","localhost","-i","1","-p",NULL};
    execv(args[0], args);
  }
  else if (n1 > 0 && n2 == 0)
  {
    //second child
    char * args[]= {"./client","-a","localhost","-i","1","-g",NULL};
    execv(args[0], args);
  }
  else {
    //third child
    char * args[]= {"./client","-a","localhost","-i","1","-p",NULL};
    execv(args[0], args);
  }
  return 0;

}
