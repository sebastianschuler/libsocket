# include <vector>
# include <map>
# include <utility>
# include <list>
# include <cstring>

# include <unistd.h>
# include <sys/select.h>
# include <errno.h>
# include <sys/time.h>
# include <sys/types.h>

/*
   The committers of the libsocket project, all rights reserved
   (c) 2012, dermesser <lbo@spheniscida.de>

   Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
   following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
   disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
   EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

*/

# define READ 1
# define WRITE 2

/*
 * DESCRIPTION FOR SELECT.CPP
 * 	The class selectset implements a wrapper for the syscall select() which allows
 * 	to accept connections on more than one socket or communicate with multiple clients
 * 	without multithreading.
 *
 * 	New sockets may be added with add_fd(), accepting a child class of libsocket::socket.
 *
 * 	When all sockets are added, the select() call can be triggered by calling wait(). This
 * 	function returns a pair of vector<int>s, the first with the sockets ready for reading and
 * 	the second containing the sockets ready for writing.
 */

# include "../headers/exception.hpp"
# include "../headers/select.hpp"

namespace libsocket
{
    int highestfd(std::vector<int>);

    selectset::selectset()
	: filedescriptors(0), set_up(false)
    {
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
    }

    void selectset::add_fd(socket& sock, int method)
    {
	int fd = sock.getfd();

	if ( method == READ )
	{
	    FD_SET(fd,&readset);
	    filedescriptors.push_back(fd);
	    fdsockmap[fd] = &sock;
	    set_up = true;

	} else if ( method == WRITE )
	{
	    FD_SET(fd,&writeset);
	    filedescriptors.push_back(fd);
	    fdsockmap[fd] = &sock;
	    set_up = true;
	}
    }

    std::pair<std::vector<socket*>, std::vector<socket*> > selectset::wait(long long microsecs)
    {
	int n = 0;

	struct timeval *timeout = NULL;

	if ( microsecs != 0 )
	{
	    struct timeval _timeout;

	    timeout = &_timeout;

	    long long micropart = microsecs % 1000000;
	    long long secpart   = microsecs - micropart;

	    _timeout.tv_sec  = secpart;
	    _timeout.tv_usec = microsecs;
	}

	n = select(highestfd(filedescriptors)+1,&readset,&writeset,NULL,timeout);

	std::pair<std::vector<socket*>, std::vector<socket*> > rwfds;

	if ( n < 0 )
	{
	    std::string err(strerror(errno));

	    throw socket_exception(__FILE__,__LINE__,"selectset::wait(): Error at select(): " + err);

	} else if ( n == 0 ) // time is over, no filedescriptor is ready
	{
	    rwfds.first.resize(0);
	    rwfds.second.resize(0);

	    return rwfds;
	}

	std::vector<int>::iterator end = filedescriptors.end();

	for ( std::vector<int>::iterator cur = filedescriptors.begin(); cur != end; cur++ )
	{
	    if ( FD_ISSET(*cur,&readset) )
		rwfds.first.push_back(fdsockmap[*cur]);

	    if ( FD_ISSET(*cur,&writeset) )
		rwfds.second.push_back(fdsockmap[*cur]);
	}

	return rwfds;
    }

    /***** UTIL *****/

    int highestfd(std::vector<int> v)
    {
	std::vector<int>::iterator end = v.end();
	int highestfd = 0;

	for ( std::vector<int>::iterator cur = v.begin(); cur != end; cur++ )
	{
	    if ( *cur > highestfd )
		highestfd = *cur;
	}

	return highestfd;
    }

}
