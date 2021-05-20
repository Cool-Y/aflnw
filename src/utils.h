#ifndef _UTILS_H_
#define _UTILS_H_

struct config {
    char *addr;
    char *proc;
    char *path;
    char *stdout;
    char *stderr;
    char *model;
    char *bind;
    int port;
    int debug;
    // time for usleep
    int utime;
};

struct config *conf;

static char* monitor_ip="192.168.2.2";
static int monitor_port = 5000;
static char* bind_addr = "192.168.2.1";

void parseArgs(int argc, char *argv[]);
void freeArgs();
int processPid(const char *proc);
void stopProcess(const char *proc);
void tcpSend(char *addr, int port);
void udpSend(char *addr, int port);
void run(char *path, int argc, char *argv[], int optbind);
void crash();

#endif