/*
 * http_parser.cpp
 *
 *  Created on: Feb 27, 2014
 *      Author: prehawk
 */
#include "HttpRequest.h"


int HttpRequest::req_count = 0;


//const Logger HttpRequest::s_logger = Logger();
const char * HttpRequest::s_version[] = {"HTTP/1.0 ","HTTP/1.1 "};
const char * HttpRequest::s_connection[] = {"close ", "keep-alive "};

const char * HttpRequest::s_line_state[] = {
		"GET ",
		"Host: ",
		"User-Agent: ",
		"Referer: ",
		"Accept: ",
		"Connection: ",
		"Content-Length: ",
		"Date: "};

const char * HttpRequest::s_res_httpcode[]  = {
		"100 Continue",
		"200 OK",
		"400 Bad Request",
		"404 Not Found",
		"500 Internal Error",
		"505 HTTP Version Not Supported"};

const char * HttpRequest::s_pages_addr[] = {
		"404.html",
		"index.html",
		"400.html"};

HttpRequest::HttpRequest(int connfd): sockfd_(connfd), n_bytesRecv(0)
{
	req_count 		+= 1;
	p_bufRead 		= new char[READ_BUF_SIZE];
	p_loctime   	= new char[40];
	p_Dhome			= new char[40];
	p_fileSize 		= new char[20];
	p_fileAddr 		= NULL;
	sc_iov 		 	= new struct iovec[50];	// there may be a bug here.
	st_recv 		= T_NEWCONN;  //should never be invoke in destroy();
	is_keepalive	= true;    // There is no better doing.

	destroy();
}

HttpRequest::~HttpRequest(){
	req_count -= 1;
	delete [] p_loctime;
	delete [] p_Dhome;
	delete [] p_bufRead;
	delete [] p_fileSize;
	delete [] sc_iov;
	if(p_fileAddr){
		munmap( p_fileAddr, sc_fileStat.st_size );
	}
}

//reset the state of the object, it doesn't mean to reset the connection.
void HttpRequest::destroy(){

	n_bytesSend 	= 0;
	n_bytesRecv 	= 0;
	n_newLinepos	= 0;
	st_req 			= R_CLOSE;
	st_line 		= L_METHOD;
	st_parse		= P_REQ;

	st_httpCode = _100;		// here code 100 just for not parsing empty lines.
	n_ioc = 0;				// reset the writev pointer.

	p_parse = p_bufRead;

	//if this request have map a real file, then umap it;

}


// invoke by the threads in the threadpool. *MUST BE* implemented.
void HttpRequest::process(unsigned tid, bool isOut){

	if(isOut){		//epollout, write or close the peer.

		//receive first EPOLLOUT signal.
		if(st_recv == T_SENDING){
			st_recv = T_DESTROY;		// epollout twice will close the peer.

			response();
			//log.writeLogs(Logger::L_DEBUG, "response", sockfd_, tid);
			if(!is_keepalive){
				//fd_mod_out(m_epollfd, sockfd_);
				fd_rmv(m_epollfd, sockfd_);
				//log.writeLogs(Logger::L_DEBUG, "process > EPOLLOUT > T_SENDING > close connection", sockfd_, tid);
			}
		}
		return;			// if not st_recv T_SENDING, always doing nothing.
	}
	else{		// epollin, reading.
		if( st_recv == T_DESTROY ){
			st_recv = T_NEWCONN;
			destroy();
		}
		else if(st_recv == T_NEWCONN || st_recv == T_OLDCONN)
			st_recv = T_OLDCONN;
	}

	int terrno;
	bool first = true;
	ssize_t bytes = -1;
	while(bytes != 0){
		bytes = recv(sockfd_, p_bufRead + n_bytesRecv, log.readbuf - n_bytesRecv, 0);
		terrno = errno;
		//cout << "recv .. " << " fd: " << sockfd_ << " bytes: " << bytes << " errno: " << terrno << endl;
		if(bytes > 0) first = false;

		if(first && bytes == 0){

			//receive 0 normally closed by peer.
			st_recv = T_DESTROY;
			fd_rmv(m_epollfd, sockfd_);
			return;
		}

		if(bytes == -1){
			if(terrno == EAGAIN || terrno == EINTR)
				break;
			else{

				// Connection close by peer, ab test always met.
				fd_rmv(m_epollfd, sockfd_);
				return;
			}
		}
		n_bytesRecv += bytes;
		if(n_bytesRecv <= 0)
			break;

	}


	parseRequest();


	if(st_req == R_BAD){
		//log.writeLogs(Logger::L_WARNING, "process > EPOLLIN > R_BAD", sockfd_, tid);
		destroy();

		//this two can directly close the peer.
		st_recv = T_DESTROY;
		//fd_mod_out(m_epollfd, sockfd_);
		fd_rmv(m_epollfd, sockfd_);
	}
	else if(st_req == R_CLOSE){

		// signal the first EPOLLOUT here only if you meet the \r\n at the end.
		// if request is never been parselin, then it's code 100.
		if(st_httpCode != _100){

			
			//line 1: HTTP/1.1 200 OK
			addData(s_version[0]);
			addData(s_res_httpcode[st_httpCode]);
			addData("\r\n");

			//line 2: Date: dsfsdfsdfoieworfew
			addData(s_line_state[L_DATE]);
			addData(genDate()); //it contains \r\n inside the date string.

			//line 3: Connection: keep-alive;
			addData(s_line_state[L_CONNECTION]);
			addData(s_connection[is_keepalive]);
			addData("\r\n");

			//line final: file
			if(p_fileAddr){
				sprintf(p_fileSize, "%ld", sc_fileStat.st_size);
				addData(s_line_state[L_CONLEN]);
				addData(p_fileSize);
				addData("\r\n");

				addData("Content-Type: ");
				addData("text/html");			//classify types.
				addData("\r\n");

				addData("\r\n");
				addData(p_fileAddr, sc_fileStat.st_size);
			}

			st_recv = T_SENDING;
			fd_mod_out(m_epollfd, sockfd_);
		}

	}


}

//@set p_Lstart as the start of the line.
//parse every lines, set whether line is complete and which line it is.
void HttpRequest::parseRequest(){

	//log.writeLogs(Logger::L_DEBUG, "parseRequest", sockfd_, 1);
	// new connection and new request will result in new p_endl an p_Lstart;
	if(st_req == R_CLOSE){
		if(st_recv == T_NEWCONN){
			p_parse  = p_bufRead;
		}
		else{
			p_parse  = p_bufRead + n_newLinepos;
		}
	}
	p_Lstart = p_parse;


	//detect if line is invalid or it's an empty line. skip empty lines.
	if(*p_parse == '\r' and *(p_parse + 1) == '\n'){
		st_req = R_CLOSE;
		st_parse = P_REQ;
		n_newLinepos += 2;
		return;
	}

	// move pointer p_parse to where \r\n is.
	for(; (p_parse + 2 < p_bufRead + n_bytesRecv + 1); ++p_parse){
		if( *(p_parse) == '\r' ){
			if( *(p_parse + 1) == '\n'){
				st_req = R_END;

				//bad idea
				if(*(p_Lstart) == '\n')
					p_Lstart += 1;

				parseLine();
				p_Lstart = p_parse + 2;
				st_parse	= P_HEAD;		//should not set to head before parseline() !
			}
			else{
				st_req		= R_BAD;
				return;
			}
		}
		else if(*(p_parse - 1) == '\r' and
				*(p_parse) 	   == '\n' and
				*(p_parse + 1) == '\r' and
				*(p_parse + 2) == '\n'){
			st_req = R_CLOSE;
			return;
		}
		else {
			st_req = R_OPEN;
		}
	}

	return;
}

// inside parseLoop(); only called when the line is completed.  p_Lstart correctness is needed.
void HttpRequest::parseLine(){

	//getWord(p_Lstart, 1);

	st_line = L_UNKNOWN;
	for(int i = L_METHOD; i != L_UNKNOWN; ++i){
		if( cmpValue(s_line_state[i], 1) == 0){
			st_line = static_cast<LINE_STATE>(i);
			break;
		}
	}

	switch(st_line){

	case L_METHOD:
	{

		//if http version is recognized, do parse the path
		if( cmpValue(s_version[0], 3) == 0 || cmpValue(s_version[1], 3) == 0 ){

			if( cmpValue(s_version[0], 3) == 0)
				is_keepalive = false;


			if( cmpValue("", 2) == 0 ){	//no path, bad request
				st_httpCode = _400;
				is_keepalive = false;
			}
			else{
				st_httpCode = _200;


				//set '/' to '/index.html'
				if( cmpValue("/ ", 2) == 0 ){
					p_Wstart = catdir( s_pages_addr[1] );
				}
				else{
					makeWord(p_Lstart, 2);   // you need p_Wstart to be a c style string.
					p_Wstart = catdir( p_Wstart );
				}

				//set 404 file if file not exists or file is a dir.
				if( stat(p_Wstart, &sc_fileStat) < 0 || S_ISDIR( sc_fileStat.st_mode )){
					st_httpCode = _404;
					p_Wstart = catdir( s_pages_addr[0] );
					stat(p_Wstart, &sc_fileStat);
				}

			}

		}
		else{ //if version not recognized.
			st_httpCode = _400;
			is_keepalive = false;
			p_Wstart = catdir( s_pages_addr[2] );
			stat(p_Wstart, &sc_fileStat);

		}
		getFileAddr(p_Wstart);

		break;
	}
	case L_CONNECTION:
	{
		if( cmpValue(s_connection[0], 2) == 0){// get connection
			is_keepalive = false;
		}
		else{
			is_keepalive = true;
		}
		break;
	}
	case L_REFERER: // fang dao lian.

		break;
	case L_UNKNOWN:

		//if request line is unrecognizable, then it's a bad request.
		if(st_parse == P_REQ){
			st_httpCode = _400;
			is_keepalive = false;
			p_Wstart = catdir( s_pages_addr[2] );
			stat(p_Wstart, &sc_fileStat);
			getFileAddr(p_Wstart);
		}
		break;
	default:
		break;
	}
}

//cat the home and the dir
char * HttpRequest::catdir(const char * dir){
	memset(p_Dhome, '\0', sizeof p_Dhome);
	strcpy(p_Dhome, log.home.c_str());
	return strcat(p_Dhome, dir);
}

//map the file
int HttpRequest::getFileAddr(char * filename){
	int filed = open( filename, O_RDONLY );
	if(filed != -1){
		p_fileAddr = ( char* )mmap( 0, sc_fileStat.st_size, PROT_READ, MAP_PRIVATE, filed, 0 );
		close( filed );
//		p_fileAddr = new char[sc_fileStat.st_size];
//		int ret = read(filed, p_fileAddr, sc_fileStat.st_size);
	}
	return filed;
}

//generate current Date string.
char * HttpRequest::genDate(){
	time_t rawtime = time(NULL);
	ctime_r(&rawtime, p_loctime);
	return p_loctime;
}

//get words and compare the value with specific string.
int HttpRequest::cmpValue(const char * value, int count){
	getWord(p_Lstart, count);
	if(strlen(value) - 1 == p_Wend - p_Wstart){    //white space is added after every static strings,
		return strncmp(p_Wstart, value, p_Wend - p_Wstart);
	}
	else{
		return -1;
	}
}

//should never be invoke before getWord or any functions that invoke getWord !!!!
//get word number %count% and set the char after the word as '\0'
void HttpRequest::makeWord(char * pos, int count){
	getWord(pos, count);
	*p_Wend = '\0';
}

//@set w_startPos, w_endPos
//get word from p_Lstart with offset %count%.  count start from 1.
void HttpRequest::getWord(char * pos, int count){
	for(int i=0; i != count; ++i){
		p_Wstart = pos;
		for(; *pos != 0x20 and *pos != 0x0 and *pos != 0xd; ++pos); // move pos the word stopping.

		if(*pos == 0x0){
			p_Wend = pos - 2 ; // skip normal chars.  ?
		}
		else{
			p_Wend = pos;
		}

		for(; *pos == 0x20 and *pos != 0x0; ++pos); // skip spaces, move pos to the word beginning.
	}
}

//add header.
void HttpRequest::addData(const char * head, size_t size){
	sc_iov[n_ioc].iov_base = head ;
	if(!size){
		sc_iov[n_ioc].iov_len	= strlen(head);
		n_bytesSend += strlen(head);
		//std::cerr << head;
	}
	else{
		sc_iov[n_ioc].iov_len  = size;
		n_bytesSend += size;
		//std::cerr << "total bytes to send: " << n_bytesSend << endl;
	}
	n_ioc += 1;
}


//invoke by the main thread, that is WebServer .*MUST BE* implemented.
void HttpRequest::response(){

	int bytes, terrno;
	while(true){
		bytes = writev(sockfd_, sc_iov, n_ioc);
		terrno = errno;

		if(bytes == -1){
			if(errno == EAGAIN || errno == EINTR){
				continue;
			}
			else{

				// EPIPE always met.
				st_recv = T_DESTROY;
				fd_rmv(m_epollfd, sockfd_);
				return;
			}
		}

		n_bytesSend -= bytes;
        //cout << "writev .. " << " fd: " << sockfd_ << " errno: " << terrno << " remaining: " << n_bytesSend << endl;
		if(n_bytesSend <= 0){
			st_recv = T_DESTROY;
			break;
		}
	}
	fd_mod_in(m_epollfd, sockfd_);
}
