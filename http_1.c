/* an extremely simple TCP server */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<netinet/in.h>
#define NOTFOUND 404
#define FORBIDDEN 403


void err_response();

void read_analyze(int fd, char buf[])
{
	int file_fd, buf_len, idx,j;
	int nrw, len;
	char *fstr;
	
	nrw = read(fd,buf,2047);
	if(nrw < 0)
	{
		err_response("Failed to read!\n",fd,FORBIDDEN);
	}
	if(nrw>0&&nrw<2047) { buf[nrw]=0;}
	else {buf[0] = 0;}
	while(idx<nrw)
	{
		if(buf[idx] == '\r' || buf[idx] == '\n')
		{
			buf[idx] = '*';
		}
		idx++;
	}		
	if(strncmp(buf,"GET",3)&&strncmp(buf,"get",3))
	{
		err_response("Invalid Get operation!\n",buf);
	}
	
}
void clear_block(int newsocketfd)
{
	int fd_flg = fcntl(newsocketfd,F_GETFL);
	if(fd_flg<0)
	{
		fprintf(stderr,"Failed to get flag!\n");
		exit(1);	
	}
	fd_flg = fd_flg|O_NONBLOCK;
	if(fcntl(newsocketfd,F_SETFL,fd_flg)<0)
	{
		fprintf(stderr,"Failed to set flag!\n");
		exit(1);
	}
}

int main (int argc, char**argv)
{
	/* Variables Declaration */
	int socketfd, newsocketfd, portnum; //socket file descriptor, new socket file descriptor, port number
	int client_addrsize, nrw; // size of client address, nrw is number of characters read or written
	char buf[2048]; //characters read into this buffer from socket
	struct sockaddr_in server_addr, client_addr; //address of server and client
	
	//declaration for epoll
	int epfd; //returned by epoll_create, a file descriptor
	int nfds; //returned by epoll_waite, number of events to deal with
	struct epoll_event evnt,evnts[20];
	int idx;
		
	/* Check to see if a port number is provided */
	if(argc < 2)
	{
		fprintf(stderr,"Please provide a port number!\n"); 
		exit(1);
	}
	
	/* Establish a connection */
	/* First, create a socket */
	socketfd = socket(AF_INET, SOCK_STREAM, 0); // Internet domain, TCP
	if(socketfd < 0)
	{
		fprintf(stderr,"Failed to create a socket!\n");
		exit(1);
	}
	
	epfd = epoll_create(256);
	evnt.data.fd = socketfd;
	evnt.events = EPOLLIN | EPOLLET;
	
	epoll_ctl(epfd,EPOLL_CTL_ADD,socketfd,&evnt);
	
	bzero((char*)&server_addr,sizeof(server_addr)); //initialize server address to all zeros
	portnum = atoi(argv[1]); //conver the port number from string to integers
	
	server_addr.sin_family = AF_INET; // address family is internet domain
	server_addr.sin_port = htons(portnum); // convert port number from host byte order to network byte order
	server_addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANT get the IP address of the host machine
	
	/* Second, bind the socket to the address*/
	int bindreturn; //return value of bind
	bindreturn = bind(socketfd,(struct sockaddr*)&server_addr,sizeof(server_addr));
	if(bindreturn < 0)
	{
		fprintf(stderr,"Failed to bind!\n");
		exit(1);
	}
	
	/*Third, listen for connections*/
	listen(socketfd,5); //This function will not fail as long as socketfd is valid, no need to check
	
	/*Fourth, accecpt a connection*/
	client_addrsize = sizeof(client_addr);
	while(1)
	{
		nfds = epoll_wait(epfd,evnts,20,-1);
		for(idx=0;idx<nfds;idx++)
		{
			if(evnts[idx].data.fd == socketfd)
			{
				newsocketfd = accept(socketfd, (struct sockaddr*)&client_addr, &client_addrsize);
				if(newsocketfd < 0)
				{
					fprintf(stderr,"Failed to accept!\n");
					exit(1);
				}
				clear_block(newsocketfd);
				evnt.data.fd = newsocketfd;
				evnt.events = EPOLLIN|EPOLLET;
				epoll_ctl(epfd,EPOLL_CTL_ADD,newsocketfd,&evnt);
			}
			else if(evnts[idx].events&EPOLLIN)
			{
				/*if((evnts[idx].events&EPOLLERR) || (evnts[idx].events&EPOLLHUP))
				{
					close(evnts[idx].data.fd);
					continue;
				}
				/*Last, Send and recieve data*/
				bzero(buf,2048);
				nrw = read(evnts[idx].data.fd,buf,2047);
				/*if(errno == EAGAIN)
				{
					continue;
				}*/
				if(nrw < 0)
				{
					fprintf(stderr,"Failed to read!\n");
					exit(1);
				}
				printf("Connection established!\nHere is what the client just said: %s",buf);
				char *success_str = "HTTP/1.1 200 OK\r\nContent-Length:21\r\nContent-Type:text:html\r\n\r\n<h1>hello world!</h1>";
				nrw = write(evnts[idx].data.fd,success_str,strlen(success_str));
				if(nrw<0)
				{
					fprintf(stderr,"Failed to write!\n");
					exit(1);
				}
				close(evnts[idx].data.fd);
			}
		}
	}
	close(socketfd);
	return 0;
		
}
