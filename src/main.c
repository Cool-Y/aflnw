#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

int main(int argc, char *argv[], char *const envp[])
{
    // int pid;

    parseArgs(argc, argv);
    //pid = processPid(conf->proc);
    printf("Start fuzz %s protocol, bind on interface %s\n",conf->model,bind_addr);
    printf("Monitor target %s:%d\n",monitor_ip,monitor_port);
    //printf("pid %d\n", pid);
    //if (pid <= 0) {
    //    run(conf->path, argc, argv, optind);
    //    pid = processPid(conf->proc);
    //}
    if(strcmp(conf->model,"tcp")==0){
        printf("send tcp\n");
        tcpSend(conf->addr, conf->port);
    }
    if(strcmp(conf->model,"udp")==0){
        printf("send udp\n");
        udpSend(conf->addr, conf->port);
    }
    //if (processPid(conf->proc) != pid) {
    //    crash();
    //}
    printf("free\n");
    freeArgs(conf);
    return 0;
}