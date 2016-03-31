#ifndef _COMMON_H_
#define _COMMON_H_

#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>


#define LISTENQ 5
#define MAX_BUF 65536
#define PORT_ANY 0
#define BYTES_EACH_TIME 4096

void err_ret(const char *fmt, ...);
void err_sys(const char *fmt, ...);
void err_dump(const char *fmt, ...);
void err_msg(const char *fmt, ...);
void err_quit(const char *fmt, ...);

void bug(const char *fmt, ...);
#endif /* _COMMON_H_ */
