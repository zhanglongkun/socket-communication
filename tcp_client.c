/**
  ******************************************************************************
  * @FileName:     inet_client.c
  * @Author:       zlk
  * @Version:      V1.0
  * @Date:         2017-4-22 16:23:59
  * @Description:  This file provides all the inet_client.c functions. 
  ******************************************************************************
  */ 


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <errno.h>

/**
  ******************************************************************************
  * Function:     OpenClientSocket()
  * Description:  ÃèÊö
  * Date:         2017-04-22
  * Others:       add by zlk
  ******************************************************************************
  */ 
int OpenClientSocket(char *serviceAddr, const int servicePort, void *buffer, unsigned int bufLen, unsigned int timeout)
{
    char *buf = (char *)buffer;
    
    int ret = 0;
    int len = 0;
    int socketfd = -1;
    struct sockaddr_in servaddr;
    fd_set fdset;
	struct timeval tv;

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == socketfd) {
        printf("Creat socket error; %s(errno: %d)\n",strerror(errno),errno);
        goto EXIT;
    }

    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(serviceAddr);
    servaddr.sin_port = htons(servicePort);

    ret = connect(socketfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        printf("connect() error; %s(errno: %d)\n",strerror(errno),errno);
        goto EXIT;
    }

    if (((len = write(socketfd, buf, bufLen)) < 0)) {
        printf("write() error; %s(errno: %d)\n",strerror(errno),errno);
        goto EXIT;
    }
    if (len == 0) {
        printf("Connection closed by peer.\n");
        goto EXIT;
    } else {
        printf("write %d bytes from socket: %d\n", len, socketfd);
    }

    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    FD_ZERO(&fdset);
    FD_SET(socketfd, &fdset);
    
again:
	ret = select(socketfd+1, &fdset, NULL, NULL, &tv);
	if (ret < 0) {
		if (errno == EINTR || errno == EAGAIN){
			goto again;
		}
        printf("select() error; %s(errno: %d)\n",strerror(errno),errno);
		goto EXIT;
	}
	if (ret == 0) {
		printf("recv timeout.\n");
		goto EXIT;
	}
	if (ret > 0) {
        len = read(socketfd, buf, bufLen);
        printf("len = %d\n", len);
        
		if ((len) < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				goto again;
			}
            printf("write() error; %s(errno: %d)\n",strerror(errno),errno);
			goto EXIT;
		} if (len == 0) {
			printf("Connection closed by peer.\n");
			goto EXIT;
		}
	}
    printf("buf = %s\n", buf);
	close(socketfd);
	return (len > 0 ? 0 : -1);

EXIT:
    if (socketfd > 0) {
        close(socketfd);
    }
    return -1;
}

int main()
{
    char buf[] = "adfff";
    int len = sizeof(buf);

    OpenClientSocket("127.0.0.1", 6666, buf, len, 1000);

    printf("buf = %s\n", buf);

    return 0;
}
