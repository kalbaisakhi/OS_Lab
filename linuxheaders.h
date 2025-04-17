#ifndef LINUX_HEADERS_H
#define LINUX_HEADERS_H

/* Standard C Library Headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

/* POSIX/Unix Headers */
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

/* System Headers */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>

/* Network Headers */
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>

#endif /* LINUX_HEADERS_H */
