/*************************************************************************
	> File Name: logger.h
	> Author: prehawk
	> Mail: prehawk@gmail.com 
	> Created Time: 2014年07月11日 星期五 15时49分17秒
 ************************************************************************/
#ifndef LOGGER_H_
#define LOGGER_H_


#include <iostream>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <cstring>


using namespace std;

class Logger{
public:

	enum LEVEL {L_DEBUG, L_INFO, L_WARNING, L_ERROR};

	bool enable;

	// params
	int		 	port;
	int		 	threads;
	string	 	home;
	string	 	logfile;
	string 	 	ip;
	int		 	timeout;
	int		 	concurrent;
	unsigned 	readbuf;
	char    	p_loctime[40];
	// logwriter



	Logger(const char * path = NULL): enable(true){
		readConfig(path);

	}

	~Logger(){
	}


	void writeLogs(int le, const char * message, int sockfd, unsigned tid){

		if(enable){
			fstream writer(logfile.c_str(), ios::out | ios::app);
			string level;
			switch(le){
			case 0:
				level = "debug: ";
				break;
			case 1:
				level = "info: ";
				break;
			case 2:
				level = "warn: ";
				break;
			case 3:
				level = "err: ";
				break;
			}
			if(sockfd == 0){
				time_t rawtime = time(NULL);
				ctime_r(&rawtime, p_loctime);
				writer << endl << level << " ** " << message << " ** " << p_loctime;
			}
			else{
				writer << level << " -- " << "sock: " << sockfd << " thread: " << tid << " -- " << message << endl;
			}
			writer.close();
		}
	}


private:

	void readConfig(const char * path = NULL){
		string key, e, value;
		fstream reader;

		if(path){
			reader.open(path, ios_base::in);
		}
		else{
			reader.open("../configuration", ios_base::in);
		}

		while(reader >> key >> e >> value){

			assert(e == string("="));

			if(key == string("port")){
				stringstream fm;
				fm << value;
				fm >> port;
			}
			else if(key == string("threads")){
				stringstream fm;
				fm << value;
				fm >> threads;
			}
			else if(key == string("home")){
				home = value;
			}
			else if(key == string("logfile")){
				logfile = value;
			}
			else if(key == string("ip")){
				ip = value;
			}
			else if(key == string("timeout")){
				stringstream fm;
				fm << value;
				fm >> timeout;
			}
			else if(key == string("concurrent")){
				stringstream fm;
				fm << value;
				fm >> concurrent;
			}
			else if(key == string("readbuf")){
				stringstream fm;
				fm << value;
				fm >> readbuf;
			}


		}

		reader.close();

	}


};


#endif
