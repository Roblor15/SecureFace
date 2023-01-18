/*
 * Create a socket that receives connections for making video from photos.
 * The client sends photos (many as it wants) and the program calls ffmpeg
 * to make the video.
 */

#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

// Port where the server listen for new connections
#define PORT 8080

// Kills all children processes
void signal_handler(int signum);

// File descriptor for the socket
int sockfd = -1;

int main()
{
    // Kill all children processes if SIGINT
    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        printf("can't intercept SIGINT\n");
        exit(0);
    }

    // Socket creation and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("socket successfully created..\n");

    // Server address initialisation
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));

    // Assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
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

    // Videos counter
    int counter = 0;

    for (;;)
    {
        // File descriptor of incoming connection
        int connfd;
        int len;
        struct sockaddr_in cli;

        len = sizeof(cli);

        // Accept the data packet from client and verification
        connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
        if (connfd < 0)
        {
            printf("server accept failed...\n");
            exit(0);
        }
        else
            printf("server accept the client...\n");

        // Increment videos counter
        ++counter;

        // Start new child process that handles the connection
        int fid = fork();

        // If child process
        if (fid == 0)
        {
            // Overwrite the stdin with the connection fd to receive the photos
            dup2(connfd, 0);

            // Create the filename for the video
            char filename[20];
            sprintf(filename, "video-%d.avi", counter);

            // Execute ffmpeg with images as input
            execlp("ffmpeg", "ffmpeg", "-loglevel", "debug", "-y", "-f", "image2pipe", "-vcodec", "mjpeg", "-r",
                   "10", "-i", "-", "-vcodec", "mpeg4", "-qscale", "5", "-r", "10", filename, NULL);
        }

        // close the connection fd
        close(connfd);
    }
}

void signal_handler(int signum)
{
    // Close the socket fd if opened
    if (sockfd != -1)
        close(sockfd);

    // Kills all children processes
    kill(-getpid(), 10);
}