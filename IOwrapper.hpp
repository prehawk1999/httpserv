/*
 * IOwraper.hpp
 *
 *  Created on: Mar 8, 2014
 *      Author: prehawk
 */

#ifndef IOWRAPER_HPP_
#define IOWRAPER_HPP_

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/positioning.hpp>
#include <time.h>

#include "config.hpp"



namespace io = boost::iostreams;


struct RecvHandler{
	typedef char 						char_type;
	typedef io::seekable_device_tag 	category;

	RecvHandler(int sockfd):
		sockfd_(sockfd), bytes_(0){}

	std::streamsize read(char_type * s, std::streamsize n)
	{
		int ret = recv(sockfd_, s + bytes_, n - bytes_, 0);
		if(ret == -1){
			return 0;
		}
		bytes_ += ret;
		return bytes_;
	}

    std::streamsize write(const char_type* s, std::streamsize n){return 0;}

	std::streampos seek(io::stream_offset off, std::ios_base::seekdir way )
	{
		// 定位
		int pNewPos;
		switch(way)
		{
		case std::ios_base::beg:
			pNewPos = 0;
			break;
		case std::ios_base::cur:
			pNewPos = bytes_;
			break;
		case std::ios_base::end: // should not be used
			pNewPos = MAX_BUF;
			break;
		}

		pNewPos += off;
		if(pNewPos > MAX_BUF)
			throw std::ios_base::failure("bad seek offset");

		bytes_ = pNewPos;
		return bytes_;
	}

private:
	int 				sockfd_;
	std::streamsize 	bytes_;
};

struct SendHandler{
	typedef char 						char_type;
	typedef io::seekable_device_tag 	category;

	SendHandler(int sockfd):
		sockfd_(sockfd), pos_(0), iovc_(0), bytes_(0){}

	std::streamsize read(char_type * s, std::streamsize n){return 0;}

    std::streamsize write(const char_type * s, std::streamsize n)
    {
    	// "cout << 0;"  to indicate the end of input, and send the iovec.
    	if( *(s + pos_) == 0x30 ){
    		int bytes_recorded = bytes_;
    		int ret;
    		while(true){  // loop in the main thread, may cause efficiency reduce
				ret = writev(sockfd_, iov_, iovc_);
				if(ret == -1){
					return -1;					// eof,
				}
				else{
					bytes_ -= ret;
					if(bytes_ <= 0){
						clear();
						return bytes_recorded;			//	return bytes that have written.
					}
				}
    		}
    	}
    	else{
    		iov_[iovc_++] = *( reinterpret_cast<const iovec *>(s + pos_) );
    		pos_ += sizeof(iovec);
    		bytes_ += iov_[iovc_ - 1].iov_len;
    		return 0; }

    }

	std::streampos seek(io::stream_offset off, std::ios_base::seekdir way)
	{
        // 定位
        int pNewPos;
        switch(way)
        {
        case std::ios_base::beg:
            pNewPos = 0;
            break;
        case std::ios_base::cur:
            pNewPos = pos_;
            break;
        case std::ios_base::end: // should not be used
            pNewPos = MAX_BUF;
            break;
        }

        pNewPos += off;
        if(pNewPos < pos_ || pNewPos > MAX_BUF)
            throw std::ios_base::failure("bad seek offset");

        pos_ = pNewPos;
        return pos_;
	}

private:
	void clear(){
		pos_ = 0;
		iovc_ = 0;
		bytes_ = 0;
	}
private:
	int 				sockfd_;
	std::streamsize 	pos_;					// used to specify the end of user input
	int					iovc_;
    struct iovec  		iov_[2];
    ssize_t				bytes_;					// bytes to send

};

#endif /* IOWRAPER_HPP_ */
