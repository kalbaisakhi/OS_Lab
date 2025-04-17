#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define PLAYING 1
#define HAVEBALL 2
#define BALLMISSED 3
#define OUTOFGAME 4

int n, nextplr, nplr;
int cpid[1024];
int state[1024];

void createchild ( )
{
   int i;
   FILE *fp;
   char cno[16];

   fp = (FILE *)fopen("childpid.txt", "w");
   fprintf(fp, "%d\n", n); fflush(fp);

   for (i=1; i<=n; ++i) {
      cpid[i] = fork();
      if (cpid[i] == 0) {
         sprintf(cno, "%d", i);
         execlp("child", "./child", cno, NULL);
      } else {
         state[i] = PLAYING;
         fprintf(fp, "%d\n", cpid[i]); fflush(fp);
      }
   }
   fclose(fp);
   printf("Parent: %d child processes created\n", n);
   printf("Parent: Waiting for child processes to read child database\n\n");
   for (i=1; i<=n; ++i) if (i < 10) printf("       %d", i); else printf("      %d", i);
   printf("\n");
   printf("+");
   for (i=0; i<=n; ++i) printf("--------");
   printf("+\n");
   sleep(2);
}

void endgame ( )
{
   int i;

   for (i=1; i<=n; ++i) if (i < 10) printf("       %d", i); else printf("      %d", i);
   printf("\n");
   for (i=1; i<=n; ++i) {
      kill(cpid[i], SIGINT);
      waitpid(cpid[i], NULL, 0);
   }
   exit(0);
}

void printline ( )
{
   int dummycpid;
   FILE *fp;

   dummycpid = fork();
   if (dummycpid == 0) execlp("./dummy", "./dummy", NULL);
   else {
      fp = (FILE *)fopen("dummycpid.txt", "w");
      fprintf(fp, "%d", dummycpid);
      fclose(fp);
   }
   printf("|    "); fflush(stdout);
   kill(cpid[1], SIGUSR1);
   waitpid(dummycpid, NULL, 0);
}

void signalfromchild ( int sig )
{
   printline();
   if (sig == SIGUSR2) {
      state[nextplr] = OUTOFGAME;
      --nplr;
      if (nplr == 1) endgame();
   }
   while (1) {
      if (nextplr == n) nextplr = 1; else ++nextplr;
      if (state[nextplr] == PLAYING) break;
   }
   kill(cpid[nextplr], SIGUSR2);
}

int main ( int argc, char *argv[] )
{
   n = (argc > 1) ? atoi(argv[1]) : 10;

   signal(SIGUSR1, signalfromchild);
   signal(SIGUSR2, signalfromchild);

   createchild();
   nplr = n; nextplr = 1;
   kill(cpid[nextplr], SIGUSR2);
   while (1) { pause(); }

   exit(0);
}
