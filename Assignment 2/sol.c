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

int c, n, state;
int cpid[1024], nextcpid, ppid;

void readchild ( )
{
   int i;
   FILE *fp;

   fp = (FILE *)fopen("childpid.txt", "r");
   fscanf(fp, "%d", &n);
   for (i=1; i<=n; ++i) fscanf(fp, "%d", &cpid[i]);
   fclose(fp);
   nextcpid = (c == n) ? cpid[1] : cpid[c+1];
   sleep(1);
}

void printstate ( int sig )
{
   FILE *fp;
   int i, wcpid;

   switch (state) {
      case PLAYING: printf("....    "); break;
      case HAVEBALL: printf("CATCH   "); break;
      case BALLMISSED: printf("MISS    "); break;
      case OUTOFGAME: printf("        "); break;
      default: printf("???     ");
   }
   fflush(stdout);
   if (state == HAVEBALL) state = PLAYING;
   if (state == BALLMISSED) state = OUTOFGAME;
   if (c < n) kill(nextcpid, SIGUSR1);
   else {
      printf("    |\n");
      printf("+");
      for (i=0; i<=n; ++i) printf("--------");
      printf("+\n");
      fp = (FILE *)fopen("dummycpid.txt", "r");
      fscanf(fp, "%d", &wcpid);
      fclose(fp);
      kill(wcpid, SIGINT);
   }
}

void receiveball ( int sig )
{
   if (state == PLAYING) {
      state = ( ((double)rand() / (double)RAND_MAX) < 0.2) ? BALLMISSED : HAVEBALL;
      if (state == HAVEBALL) kill(ppid, SIGUSR1);
      if (state == BALLMISSED) kill(ppid, SIGUSR2);
   } else {
      fprintf(stderr, "*** Child %d: Why did I receive tha ball???\n", c);
   }
}

void endgame ( int sig )
{
   if (state == PLAYING)
      printf("+++ Child %d: Yay! I am the winner!\n", c);
   exit(0);
}

int main ( int argc, char *argv[] )
{
   c = atoi(argv[1]);
   srand((unsigned int)time(NULL) + 10 * c);
   state = PLAYING;
   ppid = getppid();
   signal(SIGUSR1, printstate);
   signal(SIGUSR2, receiveball);
   signal(SIGINT, endgame);
   sleep(1);
   readchild();
   while (1) { pause(); }
   exit(0);
}
