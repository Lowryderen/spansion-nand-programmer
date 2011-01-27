// proxy for outside vpn access
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>

#define INVALID_SOCKET -1
#define SOCKET int
#define BUFSIZE   1024      //Buffer size

typedef struct {
    unsigned char version;
    unsigned char cmd_code;
    unsigned short port;
    unsigned int ip_addr;
} SOCK_HEADER;

SOCKET start_proxy_server(char *port)
{
    SOCKET listenfd;
    struct addrinfo hints, *res, *p;  
  
    // getaddrinfo for host  
    memset (&hints, 0, sizeof(hints));  
    hints.ai_family = AF_INET;  
    hints.ai_socktype = SOCK_STREAM;  
    hints.ai_flags = AI_PASSIVE;  
    if (getaddrinfo( NULL, port, &hints, &res) != 0)  
    {  
        perror ("getaddrinfo() error");  
        exit(1);  
    }  

    // socket and bind  
    for (p = res; p!=NULL; p=p->ai_next)  
    {  
        listenfd = socket (p->ai_family, p->ai_socktype, 0);  
        if (listenfd == -1) continue;  
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;  
    }  
    if (p==NULL)  
    {  
        perror ("socket() or bind()");  
        exit(1);  
    }  
  
    freeaddrinfo(res);  
  
    // listen for incoming connections  
    if ( listen (listenfd, 1000000) != 0 )  
    {  
        perror("listen() error");  
        exit(1);  
    }  
    return listenfd;
}

void close_proxy_server(SOCKET fd)
{
    close(fd);
}

void dump_packet(char *data, char *filename, int len)
{
    FILE *file = fopen(filename, "wb");
    fwrite(data, 1, len, file);
    fclose(file);
}

int connect_to_host(unsigned long iphost, unsigned short port, char *buf, int len)
{
    struct sockaddr_in serv_addr;
    SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        fprintf(stderr,"ERROR opening socket\n");
        exit(-2);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = iphost;
    serv_addr.sin_port = port;

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(-2);
    }

    return sockfd;
}

void print_ip_addr(unsigned long ipaddr)
{
    unsigned char *ipptr = (unsigned char*)&ipaddr;
    printf("IP Address %d.%d.%d.%d (%X)\n", (unsigned int)ipptr[0], (unsigned int)ipptr[1], (unsigned int)ipptr[2], (unsigned int)ipptr[3], (unsigned int)ipaddr);
}

int init_socks(int fd, unsigned long *ipaddr, unsigned short *port)
{
    SOCK_HEADER *sockhdr;
    char buf[BUFSIZE];
    int len = recv(fd,buf,BUFSIZE,0);
    if (len == 0) return -1;

    sockhdr = (SOCK_HEADER*)buf;
    if (sockhdr->version == 0x04) {
        printf("SOCKS v4 client connected\n");
    }

    *ipaddr = sockhdr->ip_addr;
    *port = sockhdr->port;
    printf("requested port: %d\n", (unsigned int)ntohs(sockhdr->port));
    print_ip_addr(sockhdr->ip_addr);

    // send back request granted
    buf[0] = 0x00; buf[1] = 0x5A;
    if (send(fd, buf, 8, 0) != 8)
    {
        printf("Failed to send SOCKS response\n");
    }

    return 1;
}

#define MAX_THREAD_CNT 50

typedef struct
{
    pthread_t tid;
    SOCKET s;
    unsigned long ipaddr;
    unsigned short port;
} THREAD_DATA;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
THREAD_DATA threads[MAX_THREAD_CNT];

void *client_handler(void *arg)
{
    char buf[BUFSIZE];
    SOCKET msg_socket, hostsock = INVALID_SOCKET;
    SOCK_HEADER sockhdr;
    int len, flags, thread_num = *((int*)arg);
    fd_set read_set;
    struct timeval tv;
    unsigned short port;
    unsigned long ipaddr;

    pthread_mutex_lock(&mutex);
    msg_socket = threads[thread_num].s;
    pthread_mutex_unlock(&mutex);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    // init SOCKS v4 interface
    init_socks(msg_socket, &ipaddr, &port);

    hostsock = connect_to_host(ipaddr, port, buf, len);

    flags = fcntl(msg_socket, F_GETFL, 0);
    fcntl(msg_socket, F_SETFL, flags | O_NONBLOCK);

    flags = fcntl(hostsock, F_GETFL, 0);
    fcntl(hostsock, F_SETFL, flags | O_NONBLOCK);

    FD_ZERO(&read_set);


    while(1) {
        // check for packet coming from client
        FD_SET(msg_socket, &read_set);
        select(msg_socket+1,&read_set,NULL,NULL,&tv);
        if(FD_ISSET(msg_socket,&read_set)) {
            len = recv(msg_socket,buf,BUFSIZE,0);
            printf("Received packet size %d from client\n", len);
            if (len == 0) {
                close(hostsock);
                close(msg_socket);
                break;
            } else {
                // forward to host
                len = send(hostsock, buf, len, 0);
            }
        }

        // check for packet coming from host
        FD_SET(hostsock, &read_set);
        select(hostsock+1,&read_set,NULL,NULL,&tv);
        if(FD_ISSET(hostsock,&read_set)) {
            len = recv(hostsock, buf, BUFSIZE, 0);
            if (len == 0) {
                close(hostsock);
                close(msg_socket);
                break;
            }
            printf("Received packet size %d from server\n", len);
            // forward to client
            len = send(msg_socket, buf, len, 0);
        }
    }

    pthread_mutex_lock(&mutex);
    threads[thread_num].s = 0;
    threads[thread_num].tid = 0;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(void)
{
    SOCKET msg_socket = INVALID_SOCKET, listen_sock;
    struct sockaddr_in from;
    fd_set read_set;
    struct timeval tv;
    int i, fromlen, flags, thread_num;

    memset(threads, 0, sizeof(THREAD_DATA)*MAX_THREAD_CNT);

    printf("Starting proxy server...\n");
    listen_sock = start_proxy_server("8001");

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    //flags = fcntl(listen_sock, F_GETFL, 0);
    //fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK);

    while(1)
    {
        fromlen = sizeof(from);

        FD_ZERO(&read_set);
        FD_SET(listen_sock, &read_set);          

        select(listen_sock+1,&read_set,NULL,NULL,&tv);
        if(FD_ISSET(listen_sock,&read_set)) {
            printf("incoming connection...\n");
            // wait for user to connect
            msg_socket = accept(listen_sock,(struct sockaddr*)&from,&fromlen);
            if (msg_socket == INVALID_SOCKET)
            {
                printf("Received error during accept()\n");
                break;
            }

            for (i=0;i<MAX_THREAD_CNT;i++)
            {
                if ( threads[i].tid == 0)
                {
                    thread_num = i;
                    pthread_mutex_lock(&mutex);
                    threads[i].s = msg_socket;
                    if (pthread_create( &threads[i].tid, NULL, client_handler, &thread_num) ) {
                        pthread_mutex_unlock(&mutex);
                        printf("error creating thread.\n");
                        exit(-1);
                    }
                    else break;
                }
            }
            pthread_mutex_unlock(&mutex);
        }
    }

    close_proxy_server(listen_sock);
}

