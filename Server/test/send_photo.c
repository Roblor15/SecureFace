
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <fcntl.h>
#include <dirent.h>

#define MAX 8000
#define PORT 8081
#define SA struct sockaddr

void func(int sockfd)
{
    uint8_t buff[MAX];
    char dir_path[100];
    char foto_path[356];
    char *extension;

    printf("Enter the photo directory path: ");
    scanf("%s", dir_path);

    DIR *dir = opendir(dir_path);

    struct dirent *d;
    int bytes;

    while ((d = readdir(dir)) != NULL)
    {
        if ((extension = strrchr(d->d_name, '.')) != NULL)
        {
            if (strcmp(extension, ".jpg") == 0 ||
                strcmp(extension, ".jpeg") == 0 ||
                strcmp(extension, ".png") == 0)
            {
                sprintf(foto_path, "%s/%s", dir_path, d->d_name);

                printf("%s\n", foto_path);

                int fd = open(foto_path, O_RDONLY);

                while ((bytes = read(fd, buff, MAX)) > 0)
                {
                    write(sockfd, buff, bytes);
                }

                close(fd);
            }
        }
    }

    buff[0] = 0xff;
    buff[1] = 0xd9;

    write(sockfd, buff, 2);

    while ((bytes = read(sockfd, buff, sizeof(buff))) > 0)
    {
        buff[bytes] = '\0';
        printf("%s", buff);
    }
}

int main()
{
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");

    // function for chat
    func(sockfd);

    // close the socket
    close(sockfd);
}