/*
 * timer_test.cpp
 *
 *  Created on: Feb 11, 2014
 *      Author: prehawk
 */
#include "WebServer.hpp"
#include "HttpRequest.h"
#include "Request.h"
#include "IOwrapper.hpp"
#include "logger.h"
#include "heaptimer.h"

Logger log = Logger();


void test_timer(){
	ShutdownTimer sdtimer(5);		// expire time 10 secs.
	sdtimer.record(0);
	sleep(1);
	sdtimer.record(1);
	sleep(1);
	sdtimer.record(2);
	sleep(3);
	sdtimer.record(3);
	sleep(1);
	sdtimer.record(0);
	sdtimer.refresh(2);

	while( !sdtimer.empty() ){
		time_t tmp, nowt = time(NULL);
		if((tmp = sdtimer.getExpire()) < nowt){
			timer_unit t = sdtimer.pop();
			cout << nowt << " > " << tmp << " fd: " << t.connfd << endl;
		}
		else{
			cout << nowt << " <= " << tmp << endl;
		}
		sleep(1);
	}

}

int main(int argc, char * argv[])
{
	log.writeLogs(Logger::L_INFO, "main", 0, 0);
	WebServer<HttpRequest> wb = WebServer<HttpRequest>(log.ip.c_str(), log.port, log.threads, 1000000 );
	wb.run();

	//test_timer();
}
