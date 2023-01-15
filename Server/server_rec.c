#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX 8000
#define PORT 8081
#define SA struct sockaddr

#define ARG_LEN 6
#define PICKLE_FILE "../facial_req/encodings.pickle"
#define DIR_TEMPLATE "XXXXXX"

void signal_handler(int signum);
int send_photos(u_int8_t *buff, int bytes, char *dir);

int fd = -1;
int end_photo = 0;
int end_sending = 0;
char path[20] = "foto-0.jpg";
int counter = 0;

char *argv_recognition[ARG_LEN] = {"python3", PICKLE_FILE, "../facial_req/run_req.py", "-d"};

int sockfd = -1;

int main()
{
    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        printf("can't intercept SIGINT\n");
        exit(0);
    }

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

        int fid = fork();

        if (fid == 0)
        {
            uint8_t buff[MAX];

            char *dir_name = mkdtemp(DIR_TEMPLATE);
            argv_recognition[ARG_LEN - 2] = dir_name;

            int bytes;
            while ((bytes = read(connfd, buff, MAX)) > 0)
            {
                if (!send_photos(buff, bytes, dir_name))
                    break;
            }

            printf("fine\n");

            dup2(connfd, STDOUT_FILENO);
            execvp("python3", argv_recognition);
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

int send_photos(u_int8_t *buff, int bytes, char *dir)
{
    if (fd == -1)
    {
        char photo_path[100];
        sprintf(photo_path, "%s/%s", dir, path);
        fd = open(photo_path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    }
    if (end_sending)
    {
        if (buff[0] == 0xff && buff[1] == 0xd9)
        {
            return 0;
        }
        else
        {
            sprintf(path, "foto-%d.jpg", ++counter);
            char photo_path[100];
            sprintf(photo_path, "%s/%s", dir, path);

            fd = open(photo_path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);

            end_sending = 0;
        }
    }
    if (end_photo)
    {
        end_photo = 0;
        printf("end\n");

        if (buff[0] == 0xd9)
        {
            printf("end 2");
            write(fd, buff, 1);
            close(fd);
            end_sending = 1;
            if (bytes > 1)
            {
                sprintf(path, "foto-%d.jpg", ++counter);
                char photo_path[100];
                sprintf(photo_path, "%s/%s", dir, path);

                fd = open(photo_path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);

                int len = bytes - 1;
                uint8_t b[len];
                for (int j = 1; j < bytes; j++)
                {
                    b[j - 1] = buff[j];
                }

                return send_photos(b, len, dir);
            }
            else
            {
                return 1;
            }
        }
    }

    for (int i = 0; i < bytes; i++)
    {
        if (buff[i] == 0xff)
        {
            if (i == bytes - 1)
            {
                end_photo = 1;
            }
            else if (buff[++i] == 0xd9)
            {
                write(fd, buff, i + 1);

                close(fd);

                end_sending = 1;

                if (i != bytes - 1)
                {
                    int len = bytes - i - 1;
                    uint8_t b[len];
                    for (int j = i + 1; j < bytes; j++)
                    {
                        b[j - i - 1] = buff[j];
                    }

                    return send_photos(b, len, dir);
                }

                return 1;
            }
        }
    }

    write(fd, buff, bytes);
    return 1;
}