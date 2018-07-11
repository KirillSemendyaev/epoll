#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define PR_NUM 2

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("Usage: POLL_SERVER [address] [port]\n");
		return -1;
	}

	int prsocks[PR_NUM], con_fd, ret, on = 1, epoll_fd = epoll_create(PR_NUM);
	int exitFlag = 1;

	size_t len;

	char buf[16] = {0};
	len = sizeof(buf);
	prsocks[0] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	prsocks[1] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	struct sockaddr_in target, server;
	socklen_t target_size = sizeof(target), server_size = sizeof(server);
	memset(&server, 0, server_size);
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[2]));
	server.sin_addr.s_addr = inet_addr(argv[1]);
	for (int i = 0; i < PR_NUM; ++i) {
		if (setsockopt(prsocks[i], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
			perror("setsockopt-reuseaddr");
			return -4;
		}
		if (setsockopt(prsocks[i], SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
			perror("setsockopt-reuseport");
			return -4;
		}
		ret = bind(prsocks[i], (struct sockaddr *) &server, server_size);
		if (ret == -1) {
			perror("bind");
			return -3;
		}
	}

	struct epoll_event prevent, out_events[2];
	prevent.events = EPOLLIN;
	prevent.data.fd = prsocks[0];
	ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, prsocks[0], &prevent);
	if (ret < 0) {
		perror("epoll_ctl");
		return -6;
	}
	prevent.events = EPOLLIN | EPOLLET;
	prevent.data.fd = prsocks[1];
	ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, prsocks[1], &prevent);
	if (ret < 0) {
		perror("epoll_ctl");
		return -6;
	}


	listen(prsocks[1], 20);
	printf("ready\n");
	do	{
		memset(buf, 0, len);
		memset(&target, 0, target_size);
		ret = epoll_wait(epoll_fd, out_events, 2, -1);
		if (ret == -1) {
			perror("epoll_wait");
			return -34;
		}
		printf("Got smth (ret = %d)\n", ret);
		for (int i = 0; i < ret; ++i) {
			if (out_events[i].data.fd == prsocks[0]) {
				recvfrom(prsocks[0], buf, len, 0, (struct sockaddr *) &target, &target_size);
				printf("UDP Connection from %s:%d\n", inet_ntoa(target.sin_addr), ntohs(target.sin_port));
				if (strcmp(buf, "Quit") != 0) {
					strcpy(buf + 5, " world!");
					sendto(prsocks[0], buf, len, MSG_CONFIRM, (struct sockaddr *) &target, target_size);
					printf("Replied\n");
					continue;
				} else {
					strcpy(buf, "OK!");
					sendto(prsocks[0], buf, len, MSG_CONFIRM, (struct sockaddr *) &target, target_size);
					printf("I QUIT\n");
					exitFlag = 0;
				}
			}

			if (out_events[i].data.fd == prsocks[1]) {
				con_fd = accept(prsocks[1], (struct sockaddr *) &target, &target_size);
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
					exitFlag = 0;
				}
			}

		}

	} while (exitFlag);
	for (int i = 0; i < PR_NUM; ++i) {
		close(prsocks[i]);
	}

	close(epoll_fd);

	return 0;
}



