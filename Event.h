/*
 * Event.h
 *
 *  Created on: 2014年7月18日
 *      Author: prehawkylin
 */

#ifndef EVENT_H_
#define EVENT_H_


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <cassert>
#include <fcntl.h>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <event2/event.h>
#include <event2/bufferevent.h>

#include "HttpRequest.h"

using boost::shared_ptr;
using boost::make_shared;
using std::vector;
using std::cout;
using std::endl;

class Event {
public:

	Event(const char * ip, int port);
	virtual ~Event();

	int init(const char * ip, int port);
	void dispatch();
	void dispatch_timer();

	//demo
private:
	friend void onTime(int sockfd, short what, void * arg);
	friend void onRecv(int sockfd, short what, void * arg);
	friend void onAccept(int sockfd, short what, void * arg);

private:
	int			m_listenfd;
};

#endif /* EVENT_H_ */
