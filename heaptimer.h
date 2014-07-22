/*
 * heap_timer2.hpp
 *
 *  Created on: Feb 13, 2014
 *      Author: prehawk
 */
//	usage:
//	HeapTimer tm = HeapTimer(80);
//	tm.add_delay_timer(5, "hello timer");
//	int i=100;
//	while(i--){
//		tm.tick();
//	}



#ifndef HEAP_TIMER2_HPP_
#define HEAP_TIMER2_HPP_
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <boost/shared_ptr.hpp>
#include <time.h>

class TimerComp;

using std::priority_queue;
using std::vector;
using std::string;
using std::map;
using std::make_pair;
using std::cout;
using std::endl;

//计时器单元类，
class timer_unit{

public:
	time_t expire;
	std::string echo;
	int	   connfd;
	bool   valid;
public:

	timer_unit(): expire(0), echo(""), connfd(-1), valid(false){}

	timer_unit( time_t delay, std::string hint):
		expire(delay), echo(hint), connfd(-1), valid(true){}

	timer_unit( time_t delay, int connfd):
		expire(delay), echo(""), connfd(connfd), valid(true){}
};


//functor
class TimerComp
{
public:
	bool operator()(const timer_unit & lhs, const timer_unit & rhs) const{
		return lhs.expire > rhs.expire;
	}
};


// setup sec. and wait the time to kill it.
class ShutdownTimer{
	typedef priority_queue<timer_unit, vector<timer_unit>, TimerComp> prior_t;
	typedef map<int, time_t> map_t;

public:
	ShutdownTimer(time_t sec): m_sec(sec){}

	time_t getExpire(){
		timer_unit ret;
		map_t::iterator it;

		if(tm_queue.empty())
			return 0;

		// If the top time_unit is in refresh table,
		// then pop it out, refresh the time and push it back. Delete item from refresh table.
		while( ret = tm_queue.top(), (it = m_refreshTable.find(ret.connfd)) != m_refreshTable.end()){
			ret.expire = (*it).second + m_sec;
			tm_queue.pop();
			tm_queue.push(ret);
			m_refreshTable.erase(it);
		}

		return ret.expire;
	}


	timer_unit pop(){

		timer_unit ret;
		ret = tm_queue.top();
		tm_queue.pop();
		return ret;
	}

	bool empty(){return tm_queue.empty();}

	void record(int connfd){
		tm_queue.push(timer_unit(time(NULL) + m_sec, connfd));
	}

	void refresh(int connfd){
		m_refreshTable.insert( make_pair(connfd, time(NULL) + m_sec) );
	}

private:
	time_t m_sec;
	prior_t tm_queue;
	map_t m_refreshTable;
};





//// 谁的expire时间大，谁优先级较小
//bool operator<( const timer_unit & lhs, const timer_unit & rhs){
//
//	if( lhs.expire > rhs.expire){
//		return true;
//	}
//	return false;
//}

//
//// timer manager
//class HeapTimer
//{
//
//public:
//	HeapTimer(int capacity): m_cap(capacity){}
//	void tick();
//	void add_delay_timer(time_t delay, string echo);
//	void add_expire_timer(time_t expire, string echo);
//	size_t size(){return tm_queue.size();}
//	bool empty(){return tm_queue.empty();}
//private:
//	int m_cap;
//	priority_queue<timer_unit, vector<timer_unit>, TimerComp> tm_queue;
//};
//
//
//
//
//void HeapTimer::add_delay_timer(time_t delay, std::string echo){
//	tm_queue.push(timer_unit(time(NULL)+delay, echo));
//}
//
//void HeapTimer::add_expire_timer(time_t expire, std::string echo){
//	tm_queue.push(timer_unit(expire, echo));
//}
//
//void HeapTimer::tick(){
//	std::cout << "tick once" << std::endl;
//	if(!empty()){
//		timer_unit tm = tm_queue.top();
//		time_t nowt = time(NULL);
//		if(tm.expire < nowt){
//			std::cout << tm.echo << std::endl;
//			tm_queue.pop();
//		}
//	}
//
//}


#endif /* HEAP_TIMER2_HPP_ */
