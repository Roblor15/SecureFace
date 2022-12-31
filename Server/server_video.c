#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define READ 0
#define WRITE 1

#define MAX 10000
#define PORT 8080
#define SA struct sockaddr

void signal_handler(int signum);

int sockfd = -1;
// Driver function
int main()
{
    if (signal(SIGKILL, signal_handler) == SIG_ERR)
        printf("can't intercept SIGKILL\n");

    if (signal(SIGINT, signal_handler) == SIG_ERR)
        printf("can't intercept SIGINT\n");

    int connfd, len;
    struct sockaddr_in servaddr;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
    {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        printf("listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");

    int counter = 0;

    for (;;)
    {
        struct sockaddr_in cli;
        len = sizeof(cli);
        // Accept the data packet from client and verification
        connfd = accept(sockfd, (SA *)&cli, &len);
        if (connfd < 0)
        {
            printf("server accept failed...\n");
            exit(0);
        }
        else
            printf("server accept the client...\n");

        ++counter;

        int fid = fork();

        if (fid == 0)
        {
            dup2(connfd, 0);
            char filename[20];
            sprintf(filename, "video-%d.avi", counter);
            execlp("ffmpeg", "ffmpeg", "-loglevel", "debug", "-y", "-f", "image2pipe", "-vcodec", "mjpeg", "-r",
                   "10", "-i", "-", "-vcodec", "mpeg4", "-qscale", "5", "-r", "10", filename, NULL);
        }

        close(connfd);
    }
}

void signal_handler(int signum)
{
    kill(-getpid(), 10);

    if (sockfd != -1)
        close(sockfd);
}