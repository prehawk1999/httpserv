/*
 * utils.cpp
 *
 *  Created on: Feb 28, 2014
 *      Author: prehawk
 */

#include "utils.h"


void setnonblocking(int fd){
    int oldop = fcntl( fd, F_GETFL );
    int newop = oldop | O_NONBLOCK;
    fcntl( fd, F_SETFL, newop );
}

void fd_add(int epollfd, int fd){

	epoll_event events;
	events.data.fd = fd;
	events.events = EPOLLIN | EPOLLET;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &events);
}

//epollfd, fd, ev
void fd_mod(int epollfd, int fd, int ev){

	epoll_event events;
	events.data.fd = fd;
	events.events = ev | EPOLLET;
	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &events);
}

void fd_mod_in(int epollfd, int fd){
	fd_mod(epollfd, fd, EPOLLIN);
}

void fd_mod_out(int epollfd, int fd){
	fd_mod(epollfd, fd, EPOLLOUT );	// you need further read, but at the same time you don't wanna trigger two events.
}

void fd_mod_hup(int epollfd, int fd){
	fd_mod(epollfd, fd, EPOLLRDHUP);
}

void fd_rmv(int epollfd, int fd){

	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
	shutdown(fd, SHUT_RDWR);
}

void addsig( int sig, void( handler )(int), bool restart = true )
{
    struct sigaction sa;
    memset( &sa, '\0', sizeof( sa ) );
    sa.sa_handler = handler;
    if( restart )
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset( &sa.sa_mask );
    assert( sigaction( sig, &sa, NULL ) != -1 );
}



