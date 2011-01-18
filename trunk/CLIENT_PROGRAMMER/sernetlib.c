#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int open_serial_net(char *hostname, int portno)
{
    struct hostent *server;
    struct sockaddr_in serv_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) 
    {
        fprintf(stderr,"ERROR opening socket\n");
        exit(-2);
    }

    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(-1);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(-2);
    }

    return sockfd;
}

char read_serial_net(int fd)
{
    char data;

    if (read(fd, &data, 1) < 1) printf("Error occurred during serial read\n");

    return data;
}

void gets_serial_net(int fd, char *buf, int max_len)
{
   int i = 0;
   while (i < max_len) {
       buf[i] = read_serial_net(fd);
       if (buf[i] == '\n' || buf[i] == ' ') break;
       i++;
   }
   buf[i] = '\0';
}

void write_serial_net(int fd, char c)
{
    if (write(fd, &c, 1) < 1) printf("Error occurred during serial write\n");
}

void puts_serial_net(int fd, char *buf, int len)
{
    int i;
    for(i=0; i < len; i++) {
        write_serial_net(fd, buf[i]);
    }
}

void close_serial_net(int fd)
{
    close(fd);
}

