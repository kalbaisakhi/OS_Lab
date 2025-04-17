#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int *readdep ( int *n, int u )
{
   FILE *fp;
   char line[1024], *p;
   int i, *D, ndep;

   fp = (FILE *)fopen("foodep.txt", "r");
   fgets(line, 1024, fp);
   sscanf(line, "%d", n);
   for (i=1; i<u; ++i) fgets(line, 1024, fp);
   D = (int *)malloc((*n) * sizeof(int));
   fgets(line, 1024, fp);
   sscanf(line, "%d", &i);
   if (i != u) {
      fprintf(stderr, "*** Error: malformed input file\n");
      exit(2);
   }
   p = strchr(line, ':') + 1;
   ndep = 0;
   if (*p == ' ') {
      while (p) {
         ++ndep;
         sscanf(p, "%d", &D[ndep]);
         p = strchr(p+1, ' ');
      }
   }
   D[0] = ndep;
   D = (int *)realloc(D, (ndep + 1) * sizeof(int));
   return D;
}

void initdone ( int n )
{
   FILE *fp;
   int i;

   fp = (FILE *)fopen("done.txt", "w");
   for (i=0; i<n; ++i) fprintf(fp,"%d",0);
   fprintf(fp, "\n");
   fflush(fp);
   fclose(fp);
}

void builddeps ( int n, int u, int *D )
{
   FILE *fp;
   int d, i, *done;
   char c, *dep;

   done = (int *)malloc((n + 1) * sizeof(int));
   dep = (char *)malloc(8 * sizeof(char));
   for (d=1; d<=D[0]; ++d) {
      fp = (FILE *)fopen("done.txt", "r");
      for (i=1; i<=n; ++i) {
         fscanf(fp, "%c", &c);
         done[i] = c - '0';
      }
      fclose(fp);
      if (done[D[d]] == 0) {
         if (fork()) {
            wait(NULL);
         } else {
            sprintf(dep, "%d", D[d]);
            execlp("./rebuild", "./rebuild", dep, "NOTROOT", NULL);
         }
      }
   }
   free(done);
   free(dep);
}

void updatedone ( int n, int u, int *D )
{
   FILE *fp;
   int i;
   int *done;
   char c;

   done = (int *)malloc((n+1)*sizeof(int));
   fp = (FILE *)fopen("done.txt", "r");
   for (i=1; i<=n; ++i) {
      fscanf(fp, "%c", &c);
      done[i] = c - '0';
   }
   fclose(fp);
   done[u] = 1;
   fp = (FILE *)fopen("done.txt", "w");
   for (i=1; i<=n; ++i) fprintf(fp, "%d", done[i]);
   fprintf(fp, "\n");
   fflush(fp);
   fclose(fp);
   printf("foo%d rebuilt", u);
   if (D[0]) {
      printf(" from foo%d", D[1]);
      for (i=2; i<=D[0]; ++i) printf(", foo%d", D[i]);
   }
   printf("\n");
}

int main ( int argc, char *argv[] )
{
   int u, n, *D;

   if (argc == 1) {
      fprintf(stderr, "*** Error: Too few arguments\n");
      exit(1);
   }

   u = atoi(argv[1]);
   D = readdep(&n, u);

   if (argc == 2) initdone(n);

   builddeps(n,u,D);

   updatedone(n,u,D);

   exit(0);
}