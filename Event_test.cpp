/*
 * Event_test.cpp
 *
 *  Created on: 2014年7月18日
 *      Author: prehawkylin
 */

#include <iostream>
#include "Event.h"
using namespace std;



int tmain(){
	cout << "event_test started .." << endl;
	Event ev("127.0.0.1", 8080);
	ev.dispatch_timer();
}
