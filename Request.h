/*
 * client.h
 *
 *  Created on: Feb 27, 2014
 *      Author: prehawk
 */
/*
 *
 *
 *
 * */

#ifndef CLIENT_H_
#define CLIENT_H_
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iterator>
#include <cstdio>

#include "config.hpp"
#include "utils.h"

class Request
{
public:
	static int m_epollfd;
	static int m_user_count;

public:
	Request();
	Request(Request * const & hr);
	virtual ~Request();

	void init(int connfd );
	virtual void process(unsigned tid, bool isNew) = 0; // better invoke fd_mod_out(m_epollfd, sockfd_); at the end
	virtual void response() = 0;	   // // better invoke fd_mod_in(m_epollfd, sockfd_); at the end
	virtual void destroy() = 0;

protected:
	void zerobuffer();

	int sockfd_;

};


#endif /* CLIENT_H_ */
