#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"

void parseArgs(int argc, char *argv[])
{
    // struct config *conf = NULL;
    conf = (struct config *)malloc(sizeof(struct config));
    bzero(conf, sizeof(struct config));
    if (argc == 1) {
        printf("Usage: %s -p <port> -a <addr> -m <tcp/udp>\n", argv[0]);
        exit(1);
    }
    char ch;
    while ((ch = getopt(argc, argv, "p:a:o:e:t:m:b:d")) != -1)
    {
        switch (ch)
        {
        case 'p':
            conf->port = atoi(optarg);
            break;
        case 'a':
            conf->addr = strdup(optarg);
            break;
        case 'o':
            conf->stdout = strdup(optarg);
            break;
        case 'e':
            conf->stderr = strdup(optarg);
            break;
        case 'd':
            conf->debug = 1;
            break;
        case 'm':
            conf->model = strdup(optarg);
            break;
        case 'b':
            conf->bind = strdup(optarg);
            break;
        default:
            break;
        }
    }
    //conf->path = strdup(argv[optind]);
    //conf->proc = strrchr(conf->path, '/');
    //conf->proc++;
}

void freeArgs()
{
    free(conf->addr);
    //free(conf->path);
    free(conf->stdout);
    free(conf->stderr);
    free(conf);
}

int processPid(const char *proc)
{
    FILE* fp = NULL;
    int count;
    int BUFSZ = 100;
    char buf[BUFSZ];
    char command[150];

    sprintf(command, "pidof %s", proc);

    if ((fp = popen(command, "r")) == NULL)
    {
        printf("popen err\n");
        return -1;
    }
    if ((fgets(buf, BUFSZ, fp)) != NULL)
    {
        count = atoi(buf);
    }
    pclose(fp);
    fp = NULL;
    return count;
}

void stopProcess(const char *proc)
{
    char command[150];
    sprintf(command, "pkill %s", proc);
    system(command);
}

void tcpSend(char *addr, int port)
{
    int server_fd;
    char *buf;
    struct sockaddr_in server_listen_addr;
    bzero(&server_listen_addr, sizeof(server_listen_addr));
    server_listen_addr.sin_family = AF_INET;
    server_listen_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr, (void*)&server_listen_addr.sin_addr);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int ret = connect(
                  server_fd, (const struct sockaddr *)&server_listen_addr,
                  sizeof(struct sockaddr)
              );
    printf("send ret %d\n", ret);
    if (ret < 0)
    {
        crash();
    }
    buf = (char*)malloc(4096);
    while (fread(buf, 1, sizeof(buf), stdin)) {
        int num = send(server_fd, (void*)buf, strlen(buf), 0);
        if (conf->debug) {
            printf("send bytes[%d][%s]\n", num, buf);
            recv(server_fd, buf, 4096, 0);
            printf("recv bytes %s\n", buf);
        }
    }
    close(server_fd);
    free(buf);
    buf = NULL;
}

void checkalive(char *addr, int port)
{
    int server_fd;
    char *buf;
    struct sockaddr_in server_listen_addr;
    bzero(&server_listen_addr, sizeof(server_listen_addr));
    server_listen_addr.sin_family = AF_INET;
    server_listen_addr.sin_port = htons(port);
    inet_pton(AF_INET, addr, (void*)&server_listen_addr.sin_addr);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int ret = connect(
                  server_fd, (const struct sockaddr *)&server_listen_addr,
                  sizeof(struct sockaddr)
              );
    printf("send ret %d\n", ret);
    if (ret < 0)
    {
        crash();
    }
    close(server_fd);
    free(buf);
    buf = NULL;
}

void udpSend(char *addr, int port)
{
    int client_fd;
    char *buf;
    struct sockaddr_in ser_addr, cli_addr;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    printf("send ret %d\n", client_fd);
    if(client_fd < 0)
    {
        printf("create socket fail!\n");
        crash();
    }

    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(9693);
    cli_addr.sin_addr.s_addr = inet_addr(bind_addr);
    int ret = bind(client_fd, (struct sockaddr* )&cli_addr, sizeof(struct sockaddr_in));
    if (ret < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(addr);
    ser_addr.sin_port = htons(port);
    buf = (char*)malloc(4096);
    while (fread(buf, 1, sizeof(buf), stdin)) {
        sendto(client_fd, buf, strlen(buf), 0, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr));
        checkalive(monitor_ip,monitor_port);
        if (conf->debug) {
            printf("client:%s\n",buf);
            recvfrom(client_fd, buf, strlen(buf), 0, NULL, NULL);
            printf("recv bytes %s\n", buf);
        }
    }
    close(client_fd);
    free(buf);
    buf = NULL;
}

void run(char *path, int argc, char *argv[], int optbind)
{
    int child = fork();
    int status;
    int ret;

    if (child < 0) {
        perror("fork");
        exit(1);
    }

    if (child == 0) {
        printf("argc %d optind %d\n", argc, optind);
        int count;
        int fd;
        pid_t sid = 0;
        char* chargs[argc - optind + 1];
        count = 0;
        while (optind + count < argc) {
            chargs[count] = argv[optind + count];
            printf("chargs %d %s\n", count, chargs[count]);
            count++;
        }
        chargs[count] = NULL;

        sid = setsid();

        if (sid < 0) {
            perror("setsid");
        }

        signal(SIGHUP, SIG_IGN);

        // double fork
        child = fork();
        if (child < 0) {
            exit(1);
        }
        if (child > 0) {
            // ret = waitpid(child, NULL, 0);
            exit(0);
        }

        if (chdir("/") < 0) {
            perror("chdir");
            exit(1);
        }

        umask(0);

        fd = open("/dev/null", O_RDONLY);
        if (fd != fileno(stdin)) {
            dup2(fd, fileno(stdin));
            close(fd);
        }
        if (conf->stdout) {
            fd = open(conf->stdout, O_WRONLY | O_CREAT | O_APPEND, 0600);
        } else {
            fd = open("/dev/null", O_WRONLY);
        }
        if (fd != fileno(stdout)) {
            dup2(fd, fileno(stdout));
            close(fd);
        }
        if (conf->stderr) {
            fd = open(conf->stderr, O_WRONLY);
        } else {
            fd = open("/dev/null", O_WRONLY);
        }
        if (fd != fileno(stderr)) {
            dup2(fd, fileno(stderr));
            close(fd);
        }

        execv(path, chargs);
        exit(0);
    } else {
        ret = waitpid(child, &status, 0);
        if (!ret) {
            fprintf(stderr, "wait pid error with code (%d)\n", status);
        }
        // wait for the server to start
        usleep(conf->utime);
        return;
    }
}

void crash()
{
    perror("crash");
    void (*x)();
    x();
    exit(0);
}