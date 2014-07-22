/*
 * http_parser.h
 *
 *  Created on: Feb 27, 2014
 *      Author: prehawk
 */

#ifndef HTTP_PARSER_H_
#define HTTP_PARSER_H_

#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>

#include "logger.h"
#include "Request.h"
#include "utils.h"
#include "config.hpp"
#include "IOwrapper.hpp"

using std::string;
using std::cout;
using std::endl;



extern Logger log;

class HttpRequest: public Request
{

	//GAP
	enum TRIG_STATE{T_DESTROY, T_NEWCONN, T_OLDCONN, T_SENDING};
	enum HTTP_STATE{_100, _200, _400, _404, _500, _505};
	enum REQ_STATE{R_OPEN, R_BAD, R_END, R_CLOSE};
	enum PARSE_STATE{P_REQ, P_HEAD, P_BODY};
	enum LINE_STATE{L_METHOD, L_HOST, L_UA, L_ACCEPT,
		L_REFERER, L_CONNECTION, L_CONLEN, L_DATE, L_UNKNOWN};

	//static const Logger s_logger;
	static const char * s_version[];
	static const char * s_connection[];
	static const char * s_line_state[];
	static const char * s_res_httpcode[];
	static const char * s_pages_addr[];

public:

	HttpRequest(int connfd);
	HttpRequest(HttpRequest * const & h);
	~HttpRequest();

	void process(unsigned tid, bool isNew); //call iowrapper to read a complete line, then fill members.
	void response();
	void destroy();
	bool isClosed(){return (st_recv == T_DESTROY);}

private:

	void writeResponse(stringstream & ss, char * file_addr, ssize_t file_size);

	void parseRequest( );
	void parseLine( );
	void getWord(char * p, int count);
	int getFileAddr(char * filename);
	void addData(const char * head, size_t size=0);
	char * genDate();
	int cmpValue(const char * value, int count);
	void makeWord(char * pos, int count);
	void getConnection();
	char * catdir(const char * dir);

private:
	//request test
	static int			req_count;

	//connected socket descriptor
	int					sockfd_;

    //pointer of read buffer.
    char * 				p_bufRead;

	//a number recording how many bytes have received. *destroy*
	ssize_t 			n_bytesRecv;

	//a number recording how many bytes have send. *Not used.*
	ssize_t				n_bytesSend;

	//a number indicating offset of p_bufRead. *destroy*
	ssize_t				n_newLinepos;


	char *				p_parse;
	char * 				p_Lstart;
	char *				p_Lend;
	char * 				p_Wstart;
	char * 				p_Wend;
	char *				p_Dhome;

	//recording the state and determine whether it should parse or response.
	TRIG_STATE			st_recv;

	//indicate whether this object have receive a line or a whole request.
	REQ_STATE			st_req;

	//indicate which line is under parsing. method? User-Agent? Connection?
	LINE_STATE			st_line;

	//indicate whether it's parsing head, body or just request line.
	PARSE_STATE			st_parse;

	// you know what it means.
	bool				is_keepalive;


	//httpcode to be return.
	HTTP_STATE			st_httpCode;

	//file location, a path.
	char *				p_fileName;

	//file pointer that point to the beginning of the file.
	char *				p_fileAddr;

	//file status used to detect authority and file size.
	struct stat			sc_fileStat;
	char *				p_fileSize;


	struct iovec * 		sc_iov;
	int					n_ioc;

	//local time buffer.
	char *				p_loctime;


};



#endif /* HTTP_PARSER_H_ */
