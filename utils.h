/*
 * utils.hpp
 *
 *  Created on: Feb 28, 2014
 *      Author: prehawk
 */

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <sys/epoll.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

void setnonblocking(int fd);

void fd_add(int epollfd, int fd);

void fd_mod(int epollfd, int fd, int ev);

void fd_mod_in(int epollfd, int fd);

void fd_mod_out(int epollfd, int fd);

void fd_mod_hup(int epollfd, int fd);

void fd_rmv(int epollfd, int fd);

void addsig( int sig, void( handler )(int), bool restart );


#endif /* UTILS_HPP_ */
