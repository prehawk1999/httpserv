/*
 * serverIO.h
 *
 *  Created on: Feb 27, 2014
 *      Author: prehawk
 */

#ifndef SERVERIO_H_
#define SERVERIO_H_

#include <syslog.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/logic/tribool.hpp>
#include <vector>
#include <string.h>
#include <errno.h>
#include <clocale>

#include "threadpool.hpp"
#include "utils.h"
#include "config.hpp"
#include "logger.h"
#include "heaptimer.h"

namespace ws{
	using std::vector;
	using boost::shared_ptr;
	using boost::make_shared;
}

using namespace ws;


extern Logger log;

// T is the type of request, whose ptr form can be push to request queue.
// *MUST* implement process();  response(); destroy(); method by T class.
// *SHOULD HAVE* m_epollfd static member.
// main thread.
template<typename T>
class WebServer
{
public:
	typedef shared_ptr<T> 							req_t;
	typedef vector< req_t > 						reqpool_t;
	typedef typename vector< req_t >::iterator 		req_iter;

public:
	// @ip address, @port, @thread number create, @max request number.
	explicit WebServer(const char * ip, int port, int threadnum, int reqqueue);
	int initialize(const char * ip, int port);
 	~WebServer();
	void run();
private:
	void loopwait();
private:
	threadpool< req_t > 		m_threadpool;
	reqpool_t 					m_reqpool;

private:
	int 						m_listenfd;
	int 						m_epollfd;
	epoll_event 				m_events[MAX_EVENTS];
	ShutdownTimer 				m_shutdowntimer;

};

template<typename T>
WebServer<T>::~WebServer(){
}

template<typename T>
WebServer<T>::WebServer(const char * ip, int port, int threadnum, int reqqueue):
	m_threadpool( threadpool< req_t >(threadnum, reqqueue) ), m_reqpool(MAX_USER), m_shutdowntimer(log.timeout){
	initialize(ip, port);
}

template<typename T>
int WebServer<T>::initialize(const char * ip, int port){

	// this would happen when peer is down you still sending, resulting in abnormal program exit.
	signal(SIGPIPE, SIG_IGN);

	m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert( m_listenfd >= 0);

    struct linger tmp = { 1, 0 };
    setsockopt( m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof( tmp ) );

    sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, (const char *)ip, &address.sin_addr);
    address.sin_port = htons(port);

    int ret = 0;
    ret = bind(m_listenfd, (sockaddr *)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(m_listenfd, MAX_USER/2);
    assert(ret != -1);

    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    setnonblocking(m_listenfd);
    fd_add(m_epollfd, m_listenfd);

    T::m_epollfd = m_epollfd;
    return 0;
}

template<typename T>
void WebServer<T>::run(){
	loopwait();
}


template<typename T>
void WebServer<T>::loopwait(){
	int number = -1;
	time_t expire = -1;
	timer_unit t;
	while(1){

		number = epoll_wait(m_epollfd, m_events, MAX_EVENTS, expire);
		assert(( number >= 0 ) || ( errno == EINTR ));

		// timeout triggered. pop out every expired events.
		if( number == 0){
			timer_unit t;
			while( !m_shutdowntimer.empty() ){
				time_t tmp, nowt = time(NULL);
				if((tmp = m_shutdowntimer.getExpire()) < nowt){
					t = m_shutdowntimer.pop();
					//cout << "timeout end: " << t.connfd << endl;
					m_reqpool[t.connfd].reset();
					fd_rmv(m_epollfd, t.connfd);
					expire = m_shutdowntimer.getExpire() - time(NULL);
					(expire <= 0)?expire = -1:expire *= 1000;
				}
				else{
					break;
				}
			}
			continue;
		}

		int terrno, count = 0;
		for(int i=0; i<number; ++i){
			int sockfd = m_events[i].data.fd;
			if(sockfd == m_listenfd){

				while (1) {

					//receive new connection.
					sockaddr_in client_addr;
					socklen_t addr_len = sizeof(client_addr);
					int connfd = accept(m_listenfd, (sockaddr *)&client_addr, &addr_len);
					terrno = errno;
					
					if(connfd != -1){
						++ count;

						setnonblocking(connfd);
						fd_add(m_epollfd, connfd);
						m_reqpool[connfd] = make_shared<T>(connfd);
						//cout << "new fd: " << connfd << endl;

						m_shutdowntimer.record(connfd);
						expire = m_shutdowntimer.getExpire() - time(NULL);
						expire *= 1000;

						//concurrent connections. too many connections should not block this thread.
						if (count % log.concurrent == 0){
							break;
						}

					}
					else{
						if(terrno != EAGAIN && terrno != EINTR)
							cout << "fd accept failed: " << terrno << endl;
						break;
					}

				}
				count = 0;

			}
			else if( m_events[i].events & EPOLLOUT ){			// write event
				//cout << "EPOLLOUT write: " << sockfd << endl;
				m_threadpool.append( m_reqpool[sockfd], true);
				continue;
			}
			else if( m_events[i].events & EPOLLIN ){  // read event
				//cout << "EPOLLIN read: " << sockfd << endl;
				m_threadpool.append( m_reqpool[sockfd], false );
				m_shutdowntimer.refresh(sockfd);
				expire = m_shutdowntimer.getExpire() - time(NULL);
				expire *= 1000;

			}


		}//for

	}//while
}
#endif /* SERVERIO_H_ */
