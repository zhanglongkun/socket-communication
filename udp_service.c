/**
  ******************************************************************************
  * @FileName:     inet_service.c
  * @Author:       zlk
  * @Version:      V1.0
  * @Date:         2017-4-22 16:24:20
  * @Description:  This file provides all the inet_service.c functions. 
  ******************************************************************************
  */ 


#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <errno.h>

/**
  ******************************************************************************
  int socket(int domain, int type, int protocol);
  
  domain：  即协议域，又称为协议族（family）。常用的协议族有，AF_INET、AF_INET6、AF_LOCAL（或称AF_UNIX，Unix域socket）、
            AF_ROUTE等等。协议族决定了socket的地址类型，在通信中必须采用对应的地址，如AF_INET决定了要用ipv4地址（32位的）
            与端口号（16位的）的组合、AF_UNIX决定了要用一个绝对路径名作为地址。
  type：    指定socket类型。常用的socket类型有，SOCK_STREAM、SOCK_DGRAM、SOCK_RAW、SOCK_PACKET、SOCK_SEQPACKET等等（socket的类型有哪些？）。
  protocol：故名思意，就是指定协议。常用的协议有，IPPROTO_TCP、IPPTOTO_UDP、IPPROTO_SCTP、IPPROTO_TIPC等，它们分别对
            应TCP传输协议、UDP传输协议、STCP传输协议、TIPC传输协议,当protocol为0时，会自动选择type类型对应的默认协议。
  ******************************************************************************
  */ 

#define BACKLOG 10
#define BUFFER_SIZE 1024


/**
  ******************************************************************************
  * Function:     ProcessFunction()
  * Description:  具体的处理函数，这里是打印比返回给客户端
  * Date:         2017-04-22
  * Others:       add by zlk
  ******************************************************************************
  */ 
int ProcessFunction(unsigned char *message, int fd, struct sockaddr_in cliaddr, int cliaddrLen)
{
    char buf[] = "success";
    int len = 0;
    
    printf("message = %s\n", message);

    len = sizeof(buf);

#if 0
    if (write(fd, buf, len) < len) {
        printf("write() error, len = %d\n", len);
        return -1;
    }
#endif
    if ((len = sendto(fd, buf, len, 0, (struct sockaddr *)&cliaddr, cliaddrLen)) < len) {
        printf("sendto() error; %s(errno: %d)\n",strerror(errno),errno);
    }

    printf("sendto %d bytes from socket: %d\n", len, fd);
    
    return 0;
}

/**
  ******************************************************************************
  * Function:     manager()
  * Description:  选择对应的处理函数
  * Date:         2017-04-22
  * Others:       add by zlk
  ******************************************************************************
  */ 
int manager(unsigned char *message, int fd, struct sockaddr_in cliaddr, int cliaddrLen) 
{
    ProcessFunction(message, fd, cliaddr, cliaddrLen);

    return 0;
}

/**
  ******************************************************************************
  * Function:     ThreadRecv()
  * Description:  接收字符串
  * Date:         2017-04-22
  * Others:       add by zlk
  ******************************************************************************
  */ 
int ThreadRecv(int fd)
{
	unsigned char packbuf[BUFFER_SIZE];
	unsigned char *buffer = packbuf;
	int len;
    int cliaddrLen = 0;
    struct sockaddr_in cliaddr;

    memset(&cliaddr, 0, sizeof(struct sockaddr_in));
	memset(buffer, 0, BUFFER_SIZE);

    cliaddrLen = sizeof(struct sockaddr_in);
    
again:
	len = recvfrom(fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &cliaddrLen);
	if (len < 0) {
		if (errno == EINTR || errno == EAGAIN) {
			goto again;
		}
		
		goto ERR_QUIT;
	}

	if (len == 0) {
		printf("recv len = 0, EOF");
		goto ERR_QUIT;
	}
        
	manager(buffer, fd, cliaddr, cliaddrLen);
    
	close(fd);
	return 0;
	
ERR_QUIT:
	close(fd);
	return -1;
}

/**
  ******************************************************************************
  * Function:     unmsg_accept()
  * Description:  accept socket
  * Date:         2017-04-22
  * Others:       add by zlk
  ******************************************************************************
  */ 
static int unmsg_accept(int nfd)
{
    int ret = 0;
    int fd;
    struct sockaddr_in cliaddr;
    int addrlen;

again:
    addrlen = sizeof(struct sockaddr_in);
    fd = accept(nfd, (struct sockaddr *)&cliaddr, &addrlen);
    if (fd < 0) {
        if (errno == EAGAIN || errno == EINTR) {
            goto again;
        }
        return fd;
    }

    ret = ThreadRecv(fd);
    if (-1 == ret) {
        printf("ThreadRecv error\n");
        return -1;
    }
    
    return 0;
}

int OpenServiceSocket(char *serviceAddr, int servicePort)
{
    int ret = 0;
    int recvlen = 0;
    int flag = 1, len = sizeof(int);
    int socketfd = -1;
    struct sockaddr_in servaddr;
    fd_set fdset;
	
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (-1 == socketfd) {
        printf("Creat socket error; %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }

    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(serviceAddr);
//    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(servicePort);
    
    ret = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &flag, len);
    if (-1 == ret) {
        printf("setsockopt() error; %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }

    ret = bind(socketfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in));
    if (-1 == ret) {
        printf("bind() error; %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }

#if 0
    ret = listen(socketfd, BACKLOG);
    if (-1 == ret) {
        printf("connect() error; %s(errno: %d)\n",strerror(errno),errno);
        return -1;
    }
#endif


	for (;;)
	{
		FD_ZERO(&fdset);
		FD_SET(socketfd, &fdset);

		ret = select(socketfd + 1, &fdset, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN) {
			    continue;
			}
			continue;
		}
        printf("line = %d\n", __LINE__);
		if (FD_ISSET(socketfd, &fdset)) {
		#if 0
			unmsg_accept(socketfd);
		#endif
            ret = ThreadRecv(socketfd);
            if (-1 == ret) {
                printf("ThreadRecv error\n");
                return -1;
            }
		}
	}
    

    close(socketfd);
    return 0;
}

int main()
{
    OpenServiceSocket("192.168.153.201", 6666);
}
