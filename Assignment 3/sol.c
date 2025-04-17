#include <stdio.h>
#include <stdlib.h>

#define ARRIVAL 1
#define CPUBURSTEND 2
#define TIMEOUT 3

typedef struct {
   int srno;      /* Serial number of the process */
   int atime;     /* Arrival time */
   int time;      /* Event time */
   int nb;        /* No of CPU bursts */
   int CPUB[10];  /* CPU burst durations */
   int IOB[9];    /* IO burst durations */
   int state;     /* ARRIVED / CPUBURSTEND / TIMEOUT */
   int bno;       /* Next CPU burst number */
   int rbt;       /* Remaining CPU burst time (for preemptive scheduling) */
} job;

typedef struct {
   int *A;        /* Array storing the queue elements */
   int MAXSIZE;   /* Queue capacity + 1 */
   int F;         /* Front index in A */
   int B;         /* Back index in A */
} readyQ;

typedef struct {
   int size;      /* Number of elements in the minheap */
   int *H;        /* Contiguous representation of the minheap */
} eventQ;;

job *readjobs ( int *n )
{
   FILE *fp;
   job *J;
   int i, cpub, iob;

   fp = (FILE *)fopen("proc.txt", "r");
   fscanf(fp, "%d", n);
   J = (job *)malloc((*n) * sizeof(job));
   for (i=0; i<(*n); ++i) {
      fscanf(fp, "%d%d", &(J[i].srno), &(J[i].atime));
      J[i].nb = 0;
      while (1) {
         fscanf(fp, "%d%d", &cpub, &iob);
         J[i].CPUB[J[i].nb] = cpub;
         if (iob > 0) J[i].IOB[J[i].nb] = iob;
         ++(J[i].nb);
         if (iob <= 0) break;
      }
   }
   fclose(fp);
   return J;
}

readyQ initRQ ( int n )
{
   readyQ Q;

   Q.A = (int *)malloc((n+1) * sizeof(int));
   Q.MAXSIZE = n + 1;
   Q.F = 0;
   Q.B = n;
   return Q;
}

int isempty ( readyQ Q )
{
   return (Q.F == (Q.B + 1) % Q.MAXSIZE);
}

int isfull ( readyQ Q )
{
   return (Q.F == (Q.B + 2) % Q.MAXSIZE);
}

int front ( readyQ Q )
{
   if (isempty(Q)) {
      fprintf(stderr, "Error: front of an empty queue\n");
      exit(1);
   }
   return Q.A[Q.F];
}

readyQ enqueue ( readyQ Q , int srno )
{
   if (isfull(Q)) {
      fprintf(stderr, "Error: enqueue in a full queue\n");
      exit(1);
   }
   ++Q.B;
   if (Q.B == Q.MAXSIZE) Q.B = 0;
   Q.A[Q.B] = srno;
   return Q;
}

readyQ dequeue ( readyQ Q )
{
   if (isempty(Q)) {
      fprintf(stderr,"Erroe: dequeue feom an empty queue\n");
      exit(1);
   }
   ++Q.F;
   if (Q.F == Q.MAXSIZE) Q.F = 0;
   return Q;
}

int eventcmp ( job *J, int E1, int E2 )
{
   if (J[E1].time < J[E2].time) return -1;
   if (J[E1].time > J[E2].time) return 1;
   if (J[E1].state < J[E2].state) return -1;
   if (J[E1].state > J[E2].state) return 1;
   if (J[E1].srno < J[E2].srno) return -1;
   if (J[E1].srno > J[E2].srno) return 1;
   return 0;
}

eventQ initEQ ( int n )
{
   eventQ Q;
   int i;

   Q.size = n;
   Q.H = (int *)malloc(n * sizeof(int));
   for (i=0; i<n; ++i) Q.H[i] = i;
   return Q;
}

void heapify ( job *J, eventQ Q, int i )
{
   int l, r, min, t;

   while (1) {
      l = 2*i + 1; r = 2*i + 2;
      if (l >= Q.size) return;
      min = ((r == Q.size) || (eventcmp(J,Q.H[l],Q.H[r]) < 0)) ? l : r;
      if (eventcmp(J,Q.H[min],Q.H[i])>0) return;
      t = Q.H[i]; Q.H[i] = Q.H[min]; Q.H[min] = t;
      i = min;
   }
}

void makeheap ( job *J, eventQ Q )
{
   int i;

   for (i=Q.size/2-1; i>=0; --i) heapify(J,Q,i);
}

eventQ insert ( job *J, eventQ Q, int x )
{
   int i, p;

   Q.H[Q.size] = x;
   i = Q.size;
   ++Q.size;
   while (1) {
      if (i == 0) return Q;
      p = (i-1)/2;
      if (eventcmp(J,Q.H[p],Q.H[i]) < 0) return Q;
      Q.H[i] = Q.H[p]; Q.H[p] = x;
      i = p;
   }
   return Q;
}

int first ( eventQ Q )
{
   if (Q.size == 0) {
      fprintf(stderr, "Error: first in empty event queue\n");
      exit(2);
   }
   return Q.H[0];
}

eventQ delmin ( job *J, eventQ Q )
{
   if (Q.size == 0) {
      fprintf(stderr, "Error: delete from empty event queue\n");
      exit(2);
   }
   --Q.size;
   Q.H[0] = Q.H[Q.size];
   heapify(J,Q,0);
   return Q;
}

void schedule ( job *J, int n, int q )
{
   int tat, tbt, wtt, i, p, t, cputime, cpufree, twt, cpuutil;
   eventQ EQ;
   readyQ RQ;

   for (i=0; i<n; ++i) {
      J[i].time = J[i].atime;
      J[i].state = ARRIVAL;
      J[i].bno = 0;
      J[i].rbt = J[i].CPUB[0];
   }
   EQ = initEQ(n); makeheap(J,EQ);
   RQ = initRQ(n);
   t = 0; twt = 0; cpufree = 1;
   #ifdef VERBOSE
   printf("%-10d: Starting\n", t);
   #endif
   while (EQ.size > 0) {
      p = first(EQ);
      EQ = delmin(J,EQ);
      t = J[p].time;
      switch (J[p].state) {
         case ARRIVAL:
            RQ = enqueue(RQ, p);
            #ifdef VERBOSE
            if (J[p].bno == 0)
               printf("%-10d: Process %d joins ready queue upon arrival\n", t, J[p].srno);
            else
               printf("%-10d: Process %d joins ready queue after IO completion\n", t, J[p].srno);
            #endif
            break;
         case CPUBURSTEND:
         case TIMEOUT:
            if (J[p].rbt == 0) {
               if (J[p].bno == J[p].nb - 1) {
                  tat = t - J[p].atime;
                  tbt = 0;
                  for (i=0; i<J[p].nb; ++i) tbt += J[p].CPUB[i];
                  for (i=0; i<J[p].nb-1; ++i) tbt += J[p].IOB[i];
                  wtt = tat - tbt;
                  printf("%-10d: Process %6d exits. Turnaround time = %4d (%3.0lf%%), Wait time = %d\n",
                         t, J[p].srno, tat, (double)tat / (double)tbt * 100, wtt);
                  twt += wtt;
               } else {
                  J[p].state = ARRIVAL;
                  J[p].time = t + J[p].IOB[J[p].bno];
                  ++J[p].bno;
                  J[p].rbt = J[p].CPUB[J[p].bno];
                  EQ = insert(J,EQ,p);
               }
            } else {
               RQ = enqueue(RQ, p);
               #ifdef VERBOSE
               printf("%-10d: Process %d joins ready queue after timeout\n", t, J[p].srno);
               #endif
            }
            cpufree = 1;
            break;
         default:
            fprintf(stderr, "Error: Unknown event type for schedule\n");
            exit(3);
      }
      if (cpufree) {
         if (isempty(RQ)) {
            #ifdef VERBOSE
            printf("%-10d: CPU goes idle\n", t);
            #endif
         } else {
            p = front(RQ);
            RQ = dequeue(RQ);
            cputime = (J[p].rbt <= q) ? J[p].rbt : q;
            J[p].rbt -= cputime;
            J[p].state = (J[p].rbt > 0) ? TIMEOUT : CPUBURSTEND;
            J[p].time = t + cputime;
            EQ = insert(J,EQ,p);
            cpufree = 0;
            #ifdef VERBOSE
            printf("%-10d: Process %d is scheduled to run for time %d\n", t, J[p].srno, cputime);
            #endif
         }
      }
   }
   printf("\nAverage wait time = %.2lf\n", (double)twt / (double)n);
   cpuutil = 0;
   for (p=0; p<n; ++p)
      for (i=0; i<J[p].nb; ++i) cpuutil += J[p].CPUB[i];
   printf("Total turnaround time = %d\n", t);
   printf("CPU idle time = %d\n", t - cpuutil);
   printf("CPU utilization = %.2lf%%\n\n", (double)cpuutil / (double)t * 100.);
}

int main ()
{
   job *J;
   int n;

   J = readjobs(&n);

   /* FCFS scheduling */
   printf("**** FCFS Scheduling ****\n");
   schedule(J,n,1000000000);

   /* RR scheduling with q = 10 */
   printf("**** RR Scheduling with q = 10 ****\n");
   schedule(J,n,10);

   /* RR scheduling with q = 5 */
   printf("**** RR Scheduling with q = 5 ****\n");
   schedule(J,n,5);

   free(J);
   exit(0);
}