all:	
	gcc -Wall -o EPOLL_SERVER epoll_server.c
	gcc -Wall -o UDP_CLIENT udp_client.c
	gcc -Wall -o TCP_CLIENT tcp_client.c
clean:
	rm UDP_CLIENT EPOLL_SERVER TCP_CLIENT

