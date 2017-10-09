/* $Id: network.cc,v 1.50 2008/02/22 07:32:17 bergo Exp $ */

/*

  eboard - chess client
  http://eboard.sourceforge.net
  Copyright (C) 2000-2007 Felipe Paulo Guazzi Bergo
  bergo@seul.org

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/



#include <iostream>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "network.h"
#include "global.h"
#include "status.h"
#include "util.h"
#include "config.h"
#include "eboard.h"

// ===================================================================
// PROCESS CONTROL
// ===================================================================

Parent::Parent(PidIssuer *a,int b) {
	issuer=a; pid=b;
}

int Parent::operator==(int v) {
	return(pid==v);
}

void PidRing::add(PidIssuer *po, int pid) {
	parents.push_back(new Parent(po,pid));
}

void PidRing::remove(PidIssuer *po) {
	list<Parent *>::iterator i;
	Parent *x;

	for(i=parents.begin();i!=parents.end();i++)
		if ( (*i)->issuer == po ) {
			x = *i;
			parents.erase(i);
			delete(x);
			return;
		}
}

void PidRing::funeral(int pid) {
	list<Parent *>::iterator i;

	for(i=parents.begin();i!=parents.end();i++)
		if ( (**i) == pid ) {
			(*i)->issuer->farewellPid(pid);
			i=parents.begin();
		}
}

PidRing netring;

static pid_t last_dead_pid = 0;

void PidRing::ZombieNotification(int pid) {
	funeral(pid);
	last_dead_pid = pid; // for use in PipeConnection::open()
}

// ===================================================================
// NET
// ===================================================================

NetConnection::NetConnection() {
	strcpy(HostName,"abstract");
	strcpy(HostAddress,"abstract");
	TimeGuard=0;
	TagRead=-1;
	listener = NULL;
}

int NetConnection::isConnected() {
	return 0;
}

NetConnection::~NetConnection() {
	notifyReadReady(NULL);
}

void NetConnection::cleanUp() {
	if (TagRead>=0)
		gdk_input_remove(TagRead);
	TagRead = -1;
}

void NetConnection::notifyReadReady(IONotificationInterface *target) {
	if (TagRead>=0)
		gdk_input_remove(TagRead);
	TagRead = -1;
	listener = NULL;
	if (target != NULL) {
		listener = target;
		TagRead = gdk_input_add(getReadHandle(), GDK_INPUT_READ,
								(GdkInputFunction) netconn_read_notify,
								(gpointer) this );
	}
}

void netconn_read_notify(gpointer data, gint source,
						 GdkInputCondition cond) {
	NetConnection *me = (NetConnection *) data;
	if (me->listener != NULL)
		me->listener->readAvailable(source);
}

void NetConnection::sendReadNotify() {
	if (listener != NULL)
		listener->readAvailable(getReadHandle());
}

int  NetConnection::hasTimeGuard() {
	return TimeGuard;
}

// ===================================================================
// BUFFERED
// ===================================================================

int BufferedConnection::readPartial(char *tbuffer,int limit) {
	if (buffer.empty()) return -1;
	int i;
	memset(tbuffer,0,limit);
	for(i=0;!buffer.empty();) {
		if (buffer.front()>=32)
			tbuffer[i++]=buffer.front();
		buffer.pop_front();
	}
	return 0;
}

int BufferedConnection::bufferMatch(char *match) {
	char dump[512];
	list<char>::iterator li;
	int i;
	if (buffer.empty()) return 0;
	memset(dump,0,512);
	for(i=0,li=buffer.begin();li!=buffer.end();li++,i++) {
		dump[i]=*li;
		if (i>510)
			return 0;
	}
	return(strstr(dump,match)!=0);
}

int BufferedConnection::consume(int handle, int amount) {
	int i,j;
	char sm[2048];
	if (amount>2048) amount=2048;
	//  global.debug("I/O","consume-in");
	while(1) {
		i=read(handle,sm,amount);
		if ((i==0)&&(errno==0)) {
			if (buffer.empty()) {
				close();
				//  global.debug("I/O","consume-out");
				return -1;
			} else {
				if (buffer.back()!='\n')
					buffer.push_back('\n');
				break;
			}
		}
		if (i<=0)
			break;
		for(j=0;j<i;j++) {
			buffer.push_back(sm[j]);
			// cerr << sm[j] << flush;
		}
	}
	//  global.debug("I/O","consume-out");
	return 0;
}

int BufferedConnection::produce(char *tbuffer,int limit,int handle) {
	int i;
	char c;
	list<char>::iterator di;

	for(di=buffer.begin();di!=buffer.end();di++)
		if (*di=='\n')
			break;

	if (di!=buffer.end()) {
		memset(tbuffer,0,limit);
		i=0;
		while(di!=buffer.begin()) {
			c=buffer.front();
			buffer.pop_front();
			if (c>=0x20)
				tbuffer[i++]=c;
		}
		buffer.pop_front();
		global.LogAppend(tbuffer);
		return 0;
	}
	return -1;
}

int BufferedConnection::innerReadLine(char *tbuffer,int limit,int handle) {
	if (consume(handle))
		return -1;
	return(produce(tbuffer,limit,handle));
}

int BufferedConnection::bufferEmpty() {
	list<char>::iterator di;
	for(di=buffer.begin();di!=buffer.end();di++)
		if (*di=='\n')
			return 0;
	return 1;
}

// ===================================================================
// ALTBUFFERED
// ===================================================================

int AltBufferedConnection::innerReadLine(char *tbuffer,int limit,int handle) {
	int i,j;
	char sm[128];
	char c;
	list<char>::iterator di;

	fd_set mine,*mp;
	struct timeval tv;
	mp=&mine;

	while(1) {
		// do the select thing...
		FD_ZERO(mp);
		FD_SET(handle,mp);
		tv.tv_sec=0;
		tv.tv_usec=20000;

		if (select(handle+1,mp,0,0,&tv)<=0)
			break;

		i=read(handle,sm,128);
		if (i<=0) {
			if (buffer.empty()) {
				close();
				return -1;
			} else {
				if (buffer.back()!='\n')
					buffer.push_back('\n');
				break;
			}
		}
		for(j=0;j<i;j++) {
			buffer.push_back(sm[j]);
			// cerr << sm[j] << flush;
		}
	}

	for(di=buffer.begin();di!=buffer.end();di++)
		if (*di=='\n')
			break;

	if (di!=buffer.end()) {
		memset(tbuffer,0,limit);
		i=0;
		while(di!=buffer.begin()) {
			c=buffer.front();
			buffer.pop_front();
			if (c>=0x20)
				tbuffer[i++]=c;
		}
		global.LogAppend(tbuffer);
		buffer.pop_front();
		return 0;
	}

	return -1;
}

// ===================================================================
// DIRECT
// ===================================================================

DirectConnection::DirectConnection(char *hostname,int port) {
	strcpy(HostName,hostname);
	strcpy(HostAddress,"???");
	Port=port;
	Connected=0;
	g_strlcpy(errorMessage,_("No error."),128);
}

int DirectConnection::open() {
	char z[128];
	int i;

	if (global.CommLog) {
		char ls[512];
		snprintf(ls,512,"+ DirectConnection::open(%s,%d)",HostName,Port);
		global.LogAppend(ls);
	}

	snprintf(z,128,_("Looking up host %s..."),HostName);
	global.status->setText(z,30);

	he=gethostbyname(HostName);
	if (he==NULL) {
		snprintf(errorMessage,128,_("Host not found: %s"),HostName);
		return(-1);
	}

	snprintf(HostAddress,96,"%d.%d.%d.%d",
			 (guchar) he->h_addr_list[0][0],
			 (guchar) he->h_addr_list[0][1],
			 (guchar) he->h_addr_list[0][2],
			 (guchar) he->h_addr_list[0][3]);

	netsocket=socket(AF_INET,SOCK_STREAM,0);

#ifdef USE_SOCK_OPTS
	int nagle=1;

#ifdef USE_SOL_TCP
    setsockopt(netsocket,SOL_TCP,TCP_NODELAY,&nagle,sizeof(nagle));
#elif defined USE_IPPROTO_TCP
    setsockopt(netsocket,IPPROTO_TCP,TCP_NODELAY,&nagle,sizeof(nagle));
#endif
#endif

	sa.sin_family=he->h_addrtype;
	sa.sin_port=htons(Port);
	memcpy(&sa.sin_addr,he->h_addr_list[0],he->h_length);

	snprintf(z,128,_("Connecting to %s..."),HostAddress);
	global.status->setText(z,30);

	i=::connect(netsocket,(struct sockaddr *)&sa,sizeof(sa));

	if (i!=0) {
		snprintf(z,128,_("Connection to %s:%d failed: "),HostName,Port);
		switch(errno) {
		case EBADF:        g_strlcat(z,_("Bad descriptor"),128);         break;
		case EFAULT:       g_strlcat(z,_("Wrong address space"),128);    break;
		case ENOTSOCK:     g_strlcat(z,_("Not a socket ?!?"),128);       break;
		case EISCONN:      g_strlcat(z,_("Already connected ?!?"),128);  break;
		case ECONNREFUSED: g_strlcat(z,_("Connection refused"),128);     break;
		case ETIMEDOUT:    g_strlcat(z,_("Timeout"),128);                break;
		case ENETUNREACH:  g_strlcat(z,_("Network is unreachable"),128); break;
		case EADDRINUSE:   g_strlcat(z,_("Address already in use"),128); break;
		case EALREADY:
		case EINPROGRESS:  strcat(z,"EINPROGRESS");           break;
		default:           g_strlcat(z,_("Unknown error"),128);
		}
		g_strlcpy(errorMessage,z,128);
		return(-1);
	}
	Connected=1;
	fcntl(netsocket,F_SETFL,O_NONBLOCK);
	snprintf(z,128,_("Connected to %s (%s)"),HostName,HostAddress);
	global.status->setText(z,300);

	global.getChannels(HostAddress);

	return 0;
}

void DirectConnection::close() {
	shutdown(netsocket,2);
	Connected=0;
}

int DirectConnection::readLine(char *tbuffer,int limit) {
	return(Connected?innerReadLine(tbuffer,limit,netsocket):-1);
}


void DirectConnection::writeLine(char *obuffer) {
	if (Connected) {
		char *tmp;
		tmp = global.filter(obuffer);
		write(netsocket,tmp,strlen(tmp));
		write(netsocket,"\n",1);
		if (global.CommLog) {
			char z[4096];
			snprintf(z,4096,"WROTE: %s",tmp);
			global.LogAppend(z);
		}
		if (tmp != obuffer) free(tmp);
	}
}

int DirectConnection::isConnected() {
	return(Connected);
}

char * DirectConnection::getError() {
	return(errorMessage);
}

int DirectConnection::getReadHandle() {
	return(Connected?netsocket:-1);
}

// ===================================================================
// INCOMING CONNECTION (FOR P2P PLAYING)
// ===================================================================

IncomingConnection::IncomingConnection(int port) {
	strcpy(HostName,"localhost");
	strcpy(HostAddress,"???");
	Port = port;
	Connected = 0;
	netsocket = 0;
	g_strlcpy(errorMessage,_("No error."),128);
}

/* the first open call on this object only
   creates the socket, binds and listens,
   subsequent calls do a non-blocking accept() */
int IncomingConnection::open() {
	if (Connected) return 0;
	if (!netsocket)
		return(createSocket());
	else
		return(acceptConnection());
}

int IncomingConnection::createSocket() {
	struct sockaddr_in sin;

	netsocket = socket(AF_INET, SOCK_STREAM, 0);
	if (netsocket == -1) {
		g_strlcpy(errorMessage,_("Unable to create socket."),128);
		return -1;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(Port);

	if (bind(netsocket,(struct sockaddr *) &sin,sizeof(sin))==-1) {
		snprintf(errorMessage,128,_("Unable to bind on port %d."),Port);
		return(-1);
	}

	fcntl(netsocket,F_SETFL,O_NONBLOCK);
	if (listen(netsocket, 1)!=0) {
		snprintf(errorMessage,128,"Unable to listen on port %d.",Port);
		return(-1);
	}
	return 0;

}

int IncomingConnection::acceptConnection() {
	int       sock;
	socklen_t addrlen;
	struct    sockaddr_in pin;
	char z[128];

	addrlen = (socklen_t) sizeof(struct sockaddr_in);
	sock = accept(netsocket, (struct sockaddr *) &pin, &addrlen);

	if (sock == -1) {
		switch(errno) {
		case EAGAIN: strcpy(errorMessage,"Nobody called."); break;
		default:     strcpy(errorMessage,"Something broke."); break;
		}
		return -1;
	}

	g_strlcpy(HostName, inet_ntoa(pin.sin_addr),128);
	strcpy(HostAddress, HostName);

	::close(netsocket); /* kill the listening socket */
	netsocket = sock;

	g_strlcpy(errorMessage,_("No error."),128);
	Connected = 1;

	fcntl(netsocket,F_SETFL,O_NONBLOCK);
	snprintf(z,128,_("Accepted incoming connection from %s"),HostName);
	global.status->setText(z,300);

	return 0;
}

void IncomingConnection::close() {
	if (Connected) {
		shutdown(netsocket,2);
		Connected=0;
		netsocket=0;
		::close(netsocket);
	} else {
		::close(netsocket);
		netsocket=0;
	}
}

int IncomingConnection::readLine(char *tbuffer, int limit) {
	return(Connected?innerReadLine(tbuffer,limit,netsocket):-1);
}

void IncomingConnection::writeLine(char *obuffer) {
	if (Connected) {
		char *tmp;
		tmp = global.filter(obuffer);
		write(netsocket,tmp,strlen(tmp));
		write(netsocket,"\n",1);
		if (global.CommLog) {
			char z[4096];
			snprintf(z,4096,"WROTE: %s",tmp);
			global.LogAppend(z);
		}
		if (tmp!=obuffer) free(tmp);
	}
}

int  IncomingConnection::isConnected() {
	return(Connected);
}

char * IncomingConnection::getError() {
	return(errorMessage);
}

int IncomingConnection::getReadHandle() {
	return(Connected?netsocket:-1);
}

// ===================================================================
// PIPE
// ===================================================================

PipeConnection::PipeConnection(int _pin,int _pout) {
	pin=_pin;
	pout=_pout;
	pid=0;
	fcntl(pin,F_SETFL,O_NONBLOCK);
	Connected=1;
	strcpy(HostName,"local pipe");
	snprintf(HostAddress,96,"pipe[%d,%d]",pin,pout);
	Quiet=0;
	MaxWaitTime = 60000.0; // 1 minute
}

void PipeConnection::init() {
	Connected=0;
	pid=0;
	toid=-1;
	memset(HostName,0,128);
	strcpy(HostAddress,"unknown");
	memset(HelperBin,0,512);
	Quiet=0;
	handshake.erase();
	MaxWaitTime = 60000.0; // 1 minute
}

void PipeConnection::setMaxWaitTime(double msecs) {
	MaxWaitTime = msecs;
}

PipeConnection::PipeConnection(char *helperbin,char *arg1=0,char *arg2=0,
							   char *arg3=0,char *arg4=0) {
	init();
	opmode=0;
	g_strlcpy(HelperBin,helperbin,512);
	args.push_back(HelperBin);
	if (arg1) args.push_back(arg1);
	if (arg2) args.push_back(arg2);
	if (arg3) args.push_back(arg3);
	if (arg4) args.push_back(arg4);
}

void PipeConnection::setHandshake(char *s) {
	handshake=s;
}

PipeConnection::PipeConnection(char *host,int port,
							   char *helperbin,char *helpersuffix)
{
	char z[256];
	EboardFileFinder eff;

	init();
	opmode=1;
	Port=port;
	g_strlcpy(HostName,host,128);

	// build helper path
	if (helpersuffix)
		snprintf(z,256,"%s.%s",helperbin,helpersuffix);
	else
		g_strlcpy(z,helperbin,256);

	if (!eff.find(z,HelperBin))
		HelperBin[0] = 0;
}

PipeConnection::~PipeConnection() {
	if (toid>=0)
		gtk_timeout_remove(toid);
	netring.remove(this);
	args.clear();
}

int PipeConnection::isConnected() {
	checkChildren();
	return(Connected);
}

void PipeConnection::checkChildren() {
	pid_t r;
	if (!pid) {
		Connected=0;
		return;
	}
	r=waitpid((pid_t)pid,NULL,WNOHANG);
	if (r>0)
		Connected=0;
}

// 0 ok, -1 error, msg in getError
int PipeConnection::open() {
	char z[256],firstline[256],*p;
	struct hostent *he;
	int n2h[2], h2n[2];

	char *arguments[6];
	unsigned int i;

	if (Connected)
		return 0;

	if (global.CommLog) {
		char ls[512];
		snprintf(ls,512,"+ PipeConnection::open(%s)",HelperBin);
		global.LogAppend(ls);
	}

	for(i=0;i<6;i++)
		arguments[i]=0;

	if (HelperBin[0]==0) {
		snprintf(errorMessage,128,_("Helper program not found"));
		return -1;
	}

	switch(opmode) {
	case 0:
		for(i=0;i<args.size();i++)
			arguments[i]=args[i];
		break;
	case 1:
		snprintf(z,256,_("Looking up host %s..."),HostName);
		if (!Quiet)
			global.status->setText(z,30);
		he=gethostbyname(HostName);
		if (he==NULL) {
			snprintf(errorMessage,128,_("Host not found: %s"),HostName);
			return(-1);
		}

		snprintf(HostAddress,96,"%d.%d.%d.%d",
				 (guchar) he->h_addr_list[0][0],
				 (guchar) he->h_addr_list[0][1],
				 (guchar) he->h_addr_list[0][2],
				 (guchar) he->h_addr_list[0][3]);

		snprintf(z,256,_("Connecting to %s..."),HostAddress);
		if (!Quiet)
			global.status->setText(z,30);

		snprintf(z,256,"%d",Port);
		arguments[0]=HelperBin;
		arguments[1]=HostAddress;
		arguments[2]=z;
		break;
	}

	// step 2: run helper

	if (pipe(n2h)||pipe(h2n)) {
		g_strlcpy(errorMessage,_("IPC pipe creation failed."),128);
		return -1;
	}
	signal(SIGPIPE,SIG_IGN);

	pid=fork();
	if (pid < 0) {
		g_strlcpy(errorMessage,_("process creation failed."),128);
		return -1;
	}
	if (!pid) {
		dup2(n2h[0],0);
		dup2(h2n[1],1);

		::close(n2h[0]);
		::close(n2h[1]);
		::close(h2n[0]);
		::close(h2n[1]);

		dup2(1,2);

		setpgid(getpid(),0); // to broadcast SIGKILL later
		execvp(HelperBin,arguments);
		write(1,"exec failed\n",12);
		global.debug("exec failed",HelperBin);
		_exit(2); // eek
	} else {

		::close(n2h[0]);
		::close(h2n[1]);
		memset(firstline,0,256);
		p=firstline;

		// send xboard/protover before anything else, to handle
		// gnu chess 5.04
		if (! handshake.empty() ) {
			int wr,j,n;
			char hs[256];

			g_strlcpy(hs,handshake.c_str(),256);
			n=strlen(hs);
			j=0;

			do {
				wr=write(n2h[1],&hs[j],n);
				if (wr<0) break;
				n-=wr;
				j+=wr;
			} while(n);
		}

		fcntl(h2n[0],F_SETFL,O_NONBLOCK);
		int rs;
		double elapsed;
		struct timeval ref, now;
		gettimeofday(&ref,NULL);

		while(1) {
			rs = read(h2n[0],p,1);
			if (rs==1) {
				if (*p=='\n') break;
				++p;
			}
			if (rs==-1) {
				if (errno==EAGAIN) {
					usleep(10000); // 10 msec
					gettimeofday(&now,NULL);
					elapsed = (1000.0*now.tv_sec - 1000.0*ref.tv_sec) +
						(now.tv_usec/1000.0 - ref.tv_usec/1000.0);
					// MaxWaitTime msecs with no output from the process ? give up.
					if (elapsed > MaxWaitTime) {
						kill(-pid,SIGKILL);
						g_strlcpy(errorMessage,_("No output from program."),128);
						::close(h2n[0]);
						::close(n2h[1]);
						return -1;
					}
				} else {
					kill(-pid,SIGKILL);
					g_strlcpy(errorMessage,_("Read error from program."),128);
					::close(h2n[0]);
					::close(n2h[1]);
					return -1;
				}
			} // rs=-1
			// should fix splange's crashing bug
			if ( (last_dead_pid == pid) ||
				 (waitpid(pid, 0, WNOHANG) != 0) ) {
				last_dead_pid = 0;
				g_strlcpy(errorMessage,_("Program exited too soon"),128);
				::close(h2n[0]);
				::close(n2h[1]);
				return -1;
			}
		}
		*p=0;
		if (!strcmp(firstline,"exec failed")) {
			g_strlcpy(errorMessage,_("Failed to run helper program"),128);
			::close(h2n[0]);
			::close(n2h[1]);
			return -1;
		}
	}
	pin=h2n[0];
	pout=n2h[1];
	Connected=1;
	fcntl(pin,F_SETFL,O_NONBLOCK);
	global.TheOffspring.push_back(-pid);
	netring.add(this,pid);
	global.zombies.add(pid,&netring);

	switch(opmode) {
	case 0:
		g_strlcpy(z,_("Engine running"),256);
	case 1:
		snprintf(z,256,_("Connected to %s (%s)"),HostName,HostAddress);
		break;
	}
	if (!Quiet)
		global.status->setText(z,300);

	if (opmode==1)
		global.getChannels(HostAddress);

	return 0;
}

void PipeConnection::close() {
	global.debug("PipeConnection","close");
	cleanUp();
	::close(pin);
	::close(pout);
	netring.remove(this);
	if (pid)
		kill(-pid,SIGKILL);
	pid=0;
	Connected=0;
}

int PipeConnection::readLine(char *tbuffer,int limit) {
	return(Connected?innerReadLine(tbuffer,limit,pin):-1);
}

void PipeConnection::writeLine(char *obuffer) {
	int ew,rem,ec,tol=0;
	fd_set wfd;
	struct timeval tv;

	FD_ZERO(&wfd);
	FD_SET(pout,&wfd);
	tv.tv_sec=2;
	tv.tv_usec=0;

	if (select(pout+1,0,&wfd,0,&tv)<=0) {
		global.debug("PipeConnection","writeLine","write would block");
		close();
		return;
	}

	//  cerr << "writing [" << obuffer << "]\n";

	if (Connected) {
		char *tmp;
		tmp = global.filter(obuffer);

		rem=strlen(tmp);
		for(ew=0;rem;) {
			ec=write(pout,tmp+ew,rem);
			if (ec<0) { usleep(50000); ++tol; if (tol==4) break; } else { ew+=ec; rem-=ec; }
		}
		write(pout,"\n",1);
		if (global.CommLog) {
			char z[4096];
			snprintf(z,4096,"WROTE: %s",tmp);
			global.LogAppend(z);
		}
		if (tmp!=obuffer) free(tmp);
	}
}

char * PipeConnection::getError() {
	return(errorMessage);
}

void PipeConnection::farewellPid(int dpid) {
	consume(pin,1536);
	netring.remove(this);
	// wait for data on the pipe before shutting down
	toid=gtk_timeout_add(2000,sched_close,(gpointer)this);
}

int PipeConnection::getReadHandle() {
	return(Connected?pin:-1);
}

gboolean sched_close(gpointer data) {
	PipeConnection *pc;
	pc=(PipeConnection *)data;
	pc->close();
	pc->toid=-1;
	pc->sendReadNotify(); // cause disconnection to be detected
	return FALSE;
}

// ===========================================================
// FALLBACK
// ===========================================================

FallBackConnection::FallBackConnection() {
	current=candidates.end();
	Connected=0;
}

FallBackConnection::~FallBackConnection() {
	for(current=candidates.begin();current!=candidates.end();current++)
		delete(*current);
}

void FallBackConnection::append(NetConnection *nc) {
	candidates.push_back(nc);
	if (!Connected)
		current=candidates.begin();
}

int  FallBackConnection::isConnected() {
	if (Connected)
		Connected=(*current)->isConnected();
	return(Connected);
}

int  FallBackConnection::open() {
	int r;

	if (candidates.empty())
		return 0;

	for(current=candidates.begin();current!=candidates.end();current++) {
		r=(*current)->open();
		if (!r) {
			Connected=1;
			strcpy(HostName,(*current)->HostName);
			strcpy(HostAddress,(*current)->HostAddress);
			TimeGuard=(*current)->TimeGuard;
			return 0;
		}
		g_strlcpy(errorMessage,(*current)->getError(),128);
	}
	return -1;
}

void FallBackConnection::close() {
	if (Connected) {
		(*current)->close();
		Connected=(*current)->isConnected();
	}
}

int  FallBackConnection::readLine(char *tbuffer,int limit) {
	if (Connected)
		return( (*current)->readLine(tbuffer,limit) );
	else
		return -1;
}

void FallBackConnection::writeLine(char *tbuffer) {
	if (Connected)
		(*current)->writeLine(tbuffer);
}

int FallBackConnection::readPartial(char *tbuffer,int limit) {
	if (Connected)
		return( (*current)->readPartial(tbuffer,limit) );
	else
		return -1;
}

int FallBackConnection::bufferMatch(char *match) {
	if (Connected)
		return( (*current)->bufferMatch(match) );
	else
		return 0;
}

char * FallBackConnection::getError() {
	return(errorMessage);
}

int FallBackConnection::getReadHandle() {
	if (Connected)
		return( (*current)->getReadHandle() );
	else
		return -1;
}

void FallBackConnection::notifyReadReady(IONotificationInterface *target) {
	if (Connected)
		(*current)->notifyReadReady(target);
}

void FallBackConnection::sendReadNotify() {
	if (current != candidates.end())
		(*current)->sendReadNotify();
}
