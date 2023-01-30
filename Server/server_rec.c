/*
 * Create a socket that receives connections for face recognition.
 * The client sends JPG photos (many as it wants) and at the end it send another
 * End Of Photo (EOP, 0xFFD9).
 * This program saves them in a temporary directory, calls the recognition program
 * and sends back the name of the person in the photos
 */

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

// Max numer of bytes the server can receive every read
#define MAX 8000
// Port where the server listen for new connections
#define PORT 8081

// Length of array of arguments to pass to the face recognition program
// Better minimum one greater respect the real arguments' length
#define ARG_LEN 6
// Path to the recognition program
#define REC_PROGRAM "../facial_rec/run_rec.py"
// Path to the .pickle file returned from facial recognition training
#define PICKLE_FILE "../facial_rec/encodings.pickle"

// Kills all children processes
void signal_handler(int signum);
// Save photos contained in buff (with bytes size), in dir.
// Returns 0 if the photos' stream is finished, 0 otherwise.
//
// It can automatically detect if a photo is finished, and saves the other in another file.
int save_photos(u_int8_t *buff, int bytes, char *dir);

// Arguments to pass to the recognition program
char *argv_recognition[ARG_LEN] = {"python3", REC_PROGRAM, PICKLE_FILE, "-d"};

// Template for photos' temporary directories
char template[] = "XXXXXX";

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

        // Start new child process that handles the connection
        int fid = fork();

        // If child process
        if (fid == 0)
        {
            // Buffer used to receive photos
            uint8_t buff[MAX];

            // Creation of temporary directory for incoming photos
            char *dir_name = mkdtemp(template);
            // char *dir_name = "ciao";
            // mkdir(dir_name, O_CREAT | O_RDWR);
            // Add directory to face recognition arguments
            argv_recognition[ARG_LEN - 2] = dir_name;

            // Bytes read
            int bytes;
            // Read untill EOF or an error
            while ((bytes = read(connfd, buff, MAX)) > 0)
            {
                if (!save_photos(buff, bytes, dir_name))
                    // Break if the photos are finished
                    break;
            }

            // Overwrite the stdout with the connection fd to send the response
            dup2(connfd, STDOUT_FILENO);
            // Execute the recognition program
            execvp("python3", argv_recognition);
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

int save_photos(u_int8_t *buff, int bytes, char *dir)
{
    // File descriptor for photos
    static int fd = -1;
    // To control for end of photo between two buffers
    static int end_photo = 0;
    // To control for end of photos' stream between two buffers
    static int end_sending = 0;
    // Name of photo
    static char path[20] = "foto-0.jpg";
    // Counter for photos
    static int counter = 0;

    if (bytes == 0)
    {
        return 1;
    }
    // Open file if not already opened
    if (fd == -1)
    {
        char photo_path[100];
        sprintf(photo_path, "%s/%s", dir, path);
        fd = open(photo_path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    }
    // Control if photos are finished
    // TODO: case bytes = 1
    if (end_sending)
    {
        // If another end of photo return 0
        if (buff[0] == 0xff && buff[1] == 0xd9)
        {
            return 0;
        }
        // If photos are not finished open a new file for the next photo
        else
        {
            sprintf(path, "foto-%d.jpg", ++counter);
            char photo_path[100];
            sprintf(photo_path, "%s/%s", dir, path);

            fd = open(photo_path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);

            end_sending = 0;
        }
    }
    // Control for end of photo
    if (end_photo)
    {
        end_photo = 0;

        if (buff[0] == 0xd9)
        {
            write(fd, buff, 1);
            close(fd);
            end_sending = 1;

            // If buffer contains bytes of another photo save them
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

                return save_photos(b, len, dir);
            }
            else
            {
                return 1;
            }
        }
    }

    // Search for end of photo
    for (int i = 0; i < bytes; i++)
    {
        if (buff[i] == 0xff)
        {
            // If partial end of photo search the other byte in the next buffer
            if (i == bytes - 1)
            {
                end_photo = 1;
            }
            else if (buff[++i] == 0xd9)
            {
                write(fd, buff, i + 1);

                close(fd);

                end_sending = 1;

                // If buffer contains bytes of another photo save them
                if (i != bytes - 1)
                {
                    int len = bytes - i - 1;
                    uint8_t b[len];
                    for (int j = i + 1; j < bytes; j++)
                    {
                        b[j - i - 1] = buff[j];
                    }

                    return save_photos(b, len, dir);
                }

                return 1;
            }
        }
    }

    // Write buffer to fd
    write(fd, buff, bytes);
    return 1;
}