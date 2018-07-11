#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define CL_NUM 2

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("Usage: POLL_SERVER [address] [port]\n");
		return -1;
	}

	struct pollfd clients[CL_NUM];

	int con_fd, ret;
	size_t len;

	char buf[16] = {0};
	len = sizeof(buf);
	clients[0].fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	clients[1].fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	clients[0].events = clients[1].events = POLLIN;
	int on = 1;


	
	struct sockaddr_in target, server;
	socklen_t target_size = sizeof(target), server_size = sizeof(server);
	memset(&server, 0, server_size);
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[2]));
	server.sin_addr.s_addr = inet_addr(argv[1]);
	for (int i = 0; i < CL_NUM; ++i) {
		if (setsockopt(clients[i].fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
			perror("setsockopt-reuseaddr");
			return -4;
		}
		if (setsockopt(clients[i].fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
			perror("setsockopt-reuseport");
			return -4;
		}
		ret = bind(clients[i].fd, (struct sockaddr *) &server, server_size);
		if (ret == -1) {
			perror("bind");
			return -3;
		}
	}

	listen(clients[1].fd, 20);
	while (1)	{
		for (int i = 0; i < CL_NUM; ++i)
			clients[i].revents = 0;
		memset(buf, 0, len);
		memset(&target, 0, target_size);
		poll(clients, CL_NUM, -1);
		if (clients[0].revents & POLLIN) {
			recvfrom(clients[0].fd, buf, len, 0, (struct sockaddr *) &target, &target_size);
			printf("UDP Connection from %s:%d\n", inet_ntoa(target.sin_addr), ntohs(target.sin_port));
			if (strcmp(buf, "Quit") != 0) {
				strcpy(buf + 5, " world!");
				sendto(clients[0].fd, buf, len, MSG_CONFIRM, (struct sockaddr *) &target, target_size);
				printf("Replied\n");
				continue;
			} else {
				strcpy(buf, "OK!");
				sendto(clients[0].fd, buf, len, MSG_CONFIRM, (struct sockaddr *) &target, target_size);
				printf("I QUIT\n");
				break;
			}
		}
		if (clients[1].revents & POLLIN) {
			con_fd = accept(clients[1].fd, (struct sockaddr *) &target, &target_size);
			recv(con_fd, buf, len, 0);
			printf("TCP Connection from %s:%d\n", inet_ntoa(target.sin_addr), ntohs(target.sin_port));
			if (strcmp(buf, "Quit") != 0) {
				strcpy(buf + 5, " world!");
				send(con_fd, buf, len, 0);
				printf("Replied\n");
				close(con_fd);
			} else {
				strcpy(buf, "OK!");
				send(con_fd, buf, len, 0);
				printf("I QUIT\n");
				close(con_fd);
				break;
			}
		}

	}
	for (int i = 0; i < CL_NUM; ++i) {
		close(clients[i].fd);
	}

	return 0;
}



