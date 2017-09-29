/* $Id: dgtboard.cc,v 1.17 2008/02/08 15:10:46 bergo Exp $ */

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

/* 
   AUTHORS += Pierre Boulenguez 
 */
#include "config.h"

#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include "mainwindow.h"
#include "global.h"
#include "sound.h"
#include "help.h"
#include "eboard.h"



#include <dlfcn.h>
 /************************/
  /* Constant definitions */
  /************************/
  /* message emitted when a piece is added onto the board */
#define DGTNIX_MSG_MV_ADD 0x00
  /* message emitted when a piece is removed from the board */
#define DGTNIX_MSG_MV_REMOVE 0x01
  /* message emitted when the clock send information */
#define DGTNIX_MSG_TIME  0x02
  /* serial tag for the dgtnixQueryString function */
#define DGTNIX_SERIAL_STRING  0x1F00
  /* busadress tag for the dgtnixQueryString function */
#define DGTNIX_BUSADDRESS_STRING 0x1F01

  /* version tag for the dgtnixQueryString function */
#define DGTNIX_VERSION_STRING 0x1F02
  /* trademark tag for the dgtnixQueryString function */
#define DGTNIX_TRADEMARK_STRING 0x1F03
  
  /* options of dgtnixInit */
#define DGTNIX_BOARD_ORIENTATION 0x01
#define DGTNIX_DEBUG 0x02

#define DGTNIX_BOARD_ORIENTATION_CLOCKLEFT 0x01
#define DGTNIX_BOARD_ORIENTATION_CLOCKRIGHT 0x02
   
#define DGTNIX_DEBUG_ON 0x01
#define DGTNIX_DEBUG_OFF 0x04
#define DGTNIX_DEBUG_WITH_TIME 0x08
#define DGTNIX_DRIVER_VERSION 0x1F04
  /*
    Contains saved errno value after a failed system call 
  */
/* extern int dgtnix_errno; */

/******************************/
/* API Functions list         */
/******************************/
/*
  int dgtnixInit(const char *);
  int dgtnixClose();
  const char *dgtnixGetBoard();
  const char *dgtnixToPrintableBoard(const char *board)
  int dgtnixTestBoard(const char *);
  float dgtnixQueryDriverVersion();
  const char *dgtnixQueryString(unsigned int);
  int dgtnixGetClockData(int *, int *, int *);
  void dgtnixSetOption(unsigned long, unsigned int);
*/
int *ptr_dgtnix_errno;
int (*dgtnixInit)(const char *);
int (*dgtnixClose)();
const char *(*dgtnixGetBoard)();
const char *(*dgtnixToPrintableBoard)(const char *);
int (*dgtnixTestBoard)(const char *);
const char *(*dgtnixQueryString)(unsigned int);
int (*dgtnixGetClockData)(int *, int *, int *);
void (*dgtnixSetOption)(unsigned long, unsigned int);
void *dgtnix_dll_handle;
bool loadDgtnix() 
{
  
  dgtnix_dll_handle = dlopen ("libdgtnix.so", RTLD_LAZY);
  char fErrorString[512];
  int count = 0;
  const char *error;
  if (!dgtnix_dll_handle) 
    { 
      count += snprintf(fErrorString+count,512-count,_("Unable to load dgtnix library.\n"));
      count+= snprintf(fErrorString+count,512-count,"dlerror: %s\n",dlerror());
      global.output->append(fErrorString,global.Colors.TextDefault,IM_NORMAL);
      return false;
    }
  count += snprintf(fErrorString+count,512-count,_("Unable to load dgtnix library symbol.\n"));
  dlerror();
  
  ptr_dgtnix_errno=(int *)dlsym(dgtnix_dll_handle, "dgtnix_errno");
  if(error = dlerror())
    {
      count+= snprintf(fErrorString+count,512-count,"dgtnix_errno: %s\n",error);
      global.output->append(fErrorString,
			    global.Colors.TextDefault,IM_NORMAL);
      dlclose(dgtnix_dll_handle);
      return false;
    }
  
  *(void **)(&dgtnixQueryString) =  dlsym(dgtnix_dll_handle, "dgtnixQueryString");
  if(error = dlerror())
    {
      count+= snprintf(fErrorString+count,512-count,"dgtnixQueryString: %s\n",error);
      global.output->append(fErrorString,
			    global.Colors.TextDefault,IM_NORMAL);
      dlclose(dgtnix_dll_handle);
      return false;
    }
  float version = atof(dgtnixQueryString(DGTNIX_DRIVER_VERSION));
  float requiredVersion = 1.8137; 
  if( version < requiredVersion)
    {
      char str[256];
      snprintf(str,256,_("dgtnix version too old: %f, must be >= %f"), 
	       version, requiredVersion);
      global.output->append(str,global.Colors.TextDefault,IM_NORMAL);
      dlclose(dgtnix_dll_handle);
      return false;
    }
  
  *(void **) (&dgtnixInit)=  dlsym(dgtnix_dll_handle, "dgtnixInit");
  if(error = dlerror())
    {
      count+= snprintf(fErrorString+count,512-count,"dgtnixInit:%s\n",error);
      global.output->append(fErrorString,
			    global.Colors.TextDefault,IM_NORMAL);
      dlclose(dgtnix_dll_handle);
      return false;
    }
  *(void **)(&dgtnixClose)= dlsym(dgtnix_dll_handle, "dgtnixClose");
  
  if(error = dlerror())
    {
       count+= snprintf(fErrorString+count,512-count,"dgtnixClose:%s\n",error);
       global.output->append(fErrorString,
			    global.Colors.TextDefault,IM_NORMAL);
     
      dlclose(dgtnix_dll_handle);
      return false;
    }
  *(void **)(&dgtnixGetBoard)= dlsym(dgtnix_dll_handle, "dgtnixGetBoard");
  
  if(error = dlerror())
    {
      count+= snprintf(fErrorString+count,512-count,"dgtnixGetBoard:%s\n",error);
      global.output->append(fErrorString,
			    global.Colors.TextDefault,IM_NORMAL);
      dlclose(dgtnix_dll_handle);
      return false;
    }
  *(void **)(&dgtnixToPrintableBoard)= dlsym(dgtnix_dll_handle, "dgtnixToPrintableBoard");
  
  if(error = dlerror())
    {
      count+= snprintf(fErrorString+count,512-count,"dgtnixToPrintableBoard:%s\n",error);
      global.output->append(fErrorString,
			    global.Colors.TextDefault,IM_NORMAL);
      dlclose(dgtnix_dll_handle);
      return false;
    }
  *(void **)(&dgtnixTestBoard) = dlsym(dgtnix_dll_handle, "dgtnixTestBoard");
  
  if(error = dlerror())
    {
      count+= snprintf(fErrorString+count,512-count,"dgtnixTestBoard:%s\n",error);
      global.output->append(fErrorString,
			    global.Colors.TextDefault,IM_NORMAL);
      
      dlclose(dgtnix_dll_handle);
      return false;
    }
   
  *(void **)(&dgtnixGetClockData) =  dlsym(dgtnix_dll_handle,"dgtnixGetClockData");
  
  if(error = dlerror())
    {
      count+= snprintf(fErrorString+count,512-count,"dgtnixGetClockData:%s\n",error);
      global.output->append(fErrorString,
			    global.Colors.TextDefault,IM_NORMAL);
      dlclose(dgtnix_dll_handle);
      return false;
    }
  *(void **)(&dgtnixSetOption) = dlsym(dgtnix_dll_handle,"dgtnixSetOption");
  
  if(error = dlerror())
    {
      count+= snprintf(fErrorString+count,512-count,"dgtnixSetOption:%s\n",error);
      global.output->append(fErrorString,
			    global.Colors.TextDefault,IM_NORMAL);
      dlclose(dgtnix_dll_handle);
      return false;
    }
   return true;
}


Board *DgtBoard = NULL;
bool   DgtInit  = false;

/* Main function for dgtnix support under eboard, 
 * this function is called in the main event loop 
 * each time something occur on the descriptor of the board 
 * indentified by parameter channel
 * data is a pointer to the MainWindow mw.
 */
gboolean gtkDgtnixEvent(GIOChannel* channel, GIOCondition cond, gpointer data)
{
  char msg[256];
  // code : first char of any message
  char code;
  // number of chars read
  gsize read=0;
  // read one char one the descriptor to find out the message type
  g_io_channel_read_chars( channel, &code, 1,&read,NULL);
  MainWindow *mw=(MainWindow *)data;
 
  if(mw->notebook->getCurrentPageId() != -1) {
    g_strlcpy(msg,_("DGT support error: wrong page"),256);
    global.output->append(msg,global.Colors.TextBright,IM_NORMAL);
    return false;
  }
  
  // Code DGTNIX_MSG_MV_ADD or DGTNIX_MSG_MV_REMOVE
  // a piece was move on the board
  if((code == DGTNIX_MSG_MV_ADD) || (code == DGTNIX_MSG_MV_REMOVE))
    {
      // Read and identify the move
      char column, piece;
      int color;
      int line;
      int x,y;
      // message : body of message with code DGTNIX_MSG_MV_ADD 
      // or DGTNIX_MSG_MV_REMOVE
      char message[4];
      read=0;
      gsize nread=0;
      while (read <3)
        {
	  g_io_channel_read_chars(channel, message, 3, &nread, NULL);
	  read += nread;
	  nread = 3 -read;
        }
      column = message[0];
      line = message[1];
      piece = message[2];
      if(islower(piece))
	color = BLACK;
      else
	color = WHITE;
      y=8-line;
      x=column-'A';

      // generate a button events equivalent to the move 
      GdkEventButton be;
      be.button=1;
      be.state =0;
      Board *me=DgtBoard;
      // TODO: write a method in class Board to compute these. Joystick code needs it too.
      if (me->effectiveFlip()) 
        {x=7-x;  y=7-y;} 
      be.x=(x*me->sqside)+me->borx;
      be.y=(y*me->sqside)+me->bory+me->morey;
      // END TODO
      if(DgtBoard->hasGame()) {
	int myColor=DgtBoard->getGame()->MyColor&COLOR_MASK;
	if(myColor==BLACK) {
	  be.x=((7-x)*me->sqside)+me->borx;
	  be.y=((7-y)*me->sqside)+me->bory+me->morey;
	}
      }
      board_button_press_event(DgtBoard->widget,&be,DgtBoard);
      board_button_release_event(DgtBoard->widget,&be,DgtBoard);

      // end of mouse move event generation
      // Test if the boards are synced
      if(DgtBoard->hasGame()) {
	char board[64];
	int i,j;
	for(i=0; i<8; i++) {
	  for(j=0; j<8; j++) {
	    int p=DgtBoard->position.getPiece(j,i)&PIECE_MASK;
	    int color=DgtBoard->position.getPiece(j,i)&COLOR_MASK;
	    int indice =((7-i)*8)+j;
	    switch(p) {
	    case PAWN:    
	      if (color==WHITE)board[indice]='P';
	      else board[indice]='p';
	      break;
	    case ROOK:    
	      if (color==WHITE)board[indice]='R';
	      else board[indice]='r';
	      break;
	    case KNIGHT:  
	      if (color==WHITE)board[indice]='N';
	      else board[indice]='n';
	      break;
	    case BISHOP:  
	      if (color==WHITE)board[indice]='B';
	      else board[indice]='b';
	      break;
	    case QUEEN:   
	      if (color==WHITE)board[indice]='Q';
	      else board[indice]='q';
	      break;
	    case KING:    
	      if (color==WHITE)board[indice]='K';
	      else board[indice]='k';
	      break;
	    default:
	      board[indice]=' ';
	      break;
	    }
	  }
	}
	int myColor=DgtBoard->getGame()->MyColor&COLOR_MASK;
	if((myColor == color) && (code != DGTNIX_MSG_MV_REMOVE) && (dgtnixTestBoard(board) != 1))
	  {
	    g_strlcpy(msg,_("DGT warning: position mismatch between eboard and DGT board."),256);
	    global.output->append(msg,global.Colors.TextBright,IM_NORMAL);
	    
	 
	    snprintf(msg,256,"DGT:\n%s",dgtnixToPrintableBoard(dgtnixGetBoard()));
	    global.output->append(msg,global.Colors.TextDefault,IM_NORMAL);
	    
	   
	    snprintf(msg,256,"eboard:\n%s",dgtnixToPrintableBoard(dgtnixGetBoard()));
	    global.output->append(msg,global.Colors.TextDefault,IM_NORMAL);
	  }
      }
      return true;
    }
  
  // Code DGTNIX_MSG_TIME, time was sent by the DGT clock
  if(code == DGTNIX_MSG_TIME) {
    int wtime, btime, wturn;
    /* FIXME: dgtnix 1.7 doesn't have this, commented so I can compile eboard -- Felipe
      if(dgtnixGetClockData(&wtime, &btime, &wturn)) {
         cout << "White time :" << wtime ;
         cout << "s, Black time :" << btime;
	  if(wturn)
	    cout << ", white player turn\n";
	  else
	    cout << ", black player turn\n";
	}
       else
	 cerr << "Error in dgtnix time event." <<endl;
    */
    return true;
  }

  snprintf(msg,256,_("DGT: unrecognized code: %c (%d)"),code,(int)(code));
  global.output->append(msg,global.Colors.TextBright,IM_NORMAL);
}


void dgtInit(const char *port,  MainWindow *mainWindow)
{
  if(!loadDgtnix())
    {
      DgtInit = false;
      return; 
    }
  char msg[512];
  dgtnixSetOption(DGTNIX_DEBUG, DGTNIX_DEBUG_ON);
  snprintf(msg,512,_("dgtnix driver version: %s"),
	   dgtnixQueryString(DGTNIX_DRIVER_VERSION));
  global.output->append(msg,global.Colors.TextBright,IM_NORMAL);
  int fd = dgtnixInit(port);
  if(fd<0) 
    {
      snprintf(msg,512,_("Unable to find the DGT board on port %s."),
	       port);
      if(*ptr_dgtnix_errno > 0)
	snprintf(msg + strlen(msg), 512 - strlen(msg), "\n%s.", strerror(*ptr_dgtnix_errno));
      global.output->append(msg,global.Colors.TextDefault,IM_NORMAL);
      DgtInit = false;
    }
  else {    
    snprintf(msg,512,_("DGT board found on port %s."), port);
    global.output->append(msg,global.Colors.TextDefault,IM_NORMAL);
    snprintf(msg,512,_("Serial :%s"),dgtnixQueryString(DGTNIX_SERIAL_STRING) );
    global.output->append(msg,global.Colors.TextDefault,IM_NORMAL);
    snprintf(msg,512,_("Bus address :%s"),dgtnixQueryString(DGTNIX_BUSADDRESS_STRING));
    global.output->append(msg,global.Colors.TextDefault,IM_NORMAL);
    snprintf(msg,512,_("Board version :%s"),dgtnixQueryString(DGTNIX_VERSION_STRING));
    global.output->append(msg,global.Colors.TextDefault,IM_NORMAL);
    snprintf(msg,512,_("Trademark :%s"),dgtnixQueryString(DGTNIX_TRADEMARK_STRING) );
    global.output->append(msg,global.Colors.TextDefault,IM_NORMAL);
    strncpy(msg,dgtnixToPrintableBoard(dgtnixGetBoard()),512);
    global.output->append(msg,global.Colors.TextDefault,IM_NORMAL);
    g_io_add_watch (g_io_channel_unix_new(fd),G_IO_IN ,gtkDgtnixEvent,mainWindow);
    DgtInit = true;
  }
}

void dgtSetBoard(Board *b) {
  if (DgtInit) DgtBoard = b;
}
