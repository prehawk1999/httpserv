/*
 * Event.cpp
 *
 *  Created on: 2014年7月18日
 *      Author: prehawkylin
 */

#include "Event.h"

typedef vector< shared_ptr<HttpRequest>	>				reqpool_t;
reqpool_t	m_reqpool;

Event::Event(const char * ip, int port) {
	//init(ip, port);
	// 初始化
}

Event::~Event() {
}

int Event::init(const char *ip, int port) {

	m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert( m_listenfd >= 0);

    sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, (const char *)ip, &address.sin_addr);
    address.sin_port = htons(port);

    int ret = 0;
    ret = bind(m_listenfd, (sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(m_listenfd, 128);
    assert(ret != -1);

    setnonblocking(m_listenfd);
    return 0;
}

void onTime(int sockfd, short what, void * arg) {
	cout << "Game Over!" << endl;
}


void onRead(int sockfd, short what, void * arg) {

}

void onAccept(int sockfd, short what, void * arg) {
	sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	int connfd = accept(sockfd, (sockaddr *)&client_addr, &addr_len);
	if(connfd != -1){
		struct event * ev_accept = event_new( (struct event_base *)arg, connfd, EV_READ, onRead, NULL );
		setnonblocking(connfd);
		m_reqpool[connfd] = make_shared<HttpRequest>(connfd);
	}
}




void Event::dispatch() {
	cout << "dispatch started ... " << endl;
	struct event_base * base = event_base_new();

	struct event * ev_listen = event_new(base, m_listenfd, EV_READ|EV_PERSIST, onAccept, NULL);
	event_add(ev_listen, NULL);
	event_base_dispatch(base);
}

void Event::dispatch_timer() {
	cout << "dispatch_timer started .." << endl;
	struct event_base * base = event_base_new();

	int tmp = 0, ret = 0;
	struct timeval tv = {1, 0};
	struct event * ev_time;
	ev_time = event_new(base, tmp, EV_TIMEOUT|EV_READ|EV_PERSIST, onTime, NULL);
	event_add(ev_time, &tv);
	event_base_dispatch(base);
}

