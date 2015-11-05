#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/uio.h>

// NOTE: all variables are global across all instances unless created otherwise

// set to 1 when grading (sends bad responses)
int grademode = 0;

// also always set to zero for single threaded when grading
int multithread = 1;

// also always set to zero for fixed RING drop during grading
int randring = 1;

// this one gives a unique ID to each instance
int idnum = 0;

// fixed number of RINGs to drop
#define FIXEDRING 3

#define LOCKCODE 8

#define TRYMAX 4

#define SVRPORT "55001" // the server port

#define MAXDATASIZE 1000

// how many pending connections queue will hold
#define MAX_CLIENTS 10000

// how long to keep waiting for input
#define USERWAIT 120

typedef enum {
  IDLE,
  HOPEN,
  OPEN,
  LOCKED,
  HCLOSE,
  FAIL
} statetype;

typedef struct funcargs {
  int fdval;
  int idval;
} funcarg_t;

void *server_func(void *);

void printtime(int id);

int main(int argc, char **argv)
{
  int sockfd, new_fd,yes=1;  
  struct sockaddr_in server,client;
  int sockaddr_len=sizeof(struct sockaddr_in); 
  
// don't declare these as externs; it confuses the compiler
//  extern char *optarg;
//  extern int optind;
  int c;
  char *pname=SVRPORT;
  static char usage[] = "usage: %s [-g] [-p port]\n";
  
  server.sin_family=AF_INET;
  
  while ((c = getopt(argc, argv, "gp:")) != -1)
    switch (c) {
    case 'g':
      multithread = 0;
      randring = 0;
      grademode = 1;
      break;
    case 'p':
      pname = optarg;
      break;
    case '?':
      printtime(idnum);
      fprintf(stderr, usage, argv[0]);
      exit(1);
      break;
    }

  server.sin_family = AF_INET;
  server.sin_port = htons(atoi(pname));
  server.sin_addr.s_addr = INADDR_ANY;
  bzero(&server.sin_zero,8);
  
  if ((sockfd = socket(AF_INET, SOCK_STREAM,0)) == -1) {
    perror("Server: socket");
  }
  
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) { 
    perror("Server: setsockopt");
    exit(1);
  }
  if (bind(sockfd, (struct sockaddr *)&server, sockaddr_len) == -1) {
    close(sockfd);
    perror("Server: bind");
  }
  
  if (listen(sockfd, MAX_CLIENTS) == -1) {
    perror("Server: listen");
    exit(1);
  }
  
  printtime(idnum);
  printf("Server at %s:%s is ready.\n", inet_ntoa((server.sin_addr)), pname);
  fflush(stdout);
  if (grademode == 1) {
    printtime(idnum);
    printf(" -- grading mode enabled. Running single-threaded, deterministic, with errors.\n");
  } else {
    printtime(idnum);
    printf(" -- regular mode. Running multi-threaded, nondeterminstic.\n");
  }
  fflush(stdout);
  while(1) {  
    new_fd = accept(sockfd, (struct sockaddr *)&client,(socklen_t *) &sockaddr_len);
    if (new_fd == -1) {
      perror("Server: accept");
      continue;
    }
    idnum++;
    printtime(idnum);
    printf("Server connected to %s.\n", inet_ntoa((client.sin_addr)));
    fflush(stdout);

    funcarg_t *fargs = (funcarg_t *)malloc(sizeof(funcarg_t));

//    int *fdarg = (int *)malloc(sizeof(int));
//    *fdarg = new_fd;
    (*fargs).fdval = new_fd;
    (*fargs).idval = idnum;

    if (multithread == 1) {
      //multithreaded for different client connections
      pthread_t server_thread;
      if( pthread_create(&server_thread , NULL , server_func, fargs) < 0)
	{
	  perror("Server: could not create thread");
	  return 1;
	}
    } else {
      while (1) {
//	server_func(fdarg);
	server_func(fargs);
      }
    }
    
  }
  
  return 0;
}

///////////////////////////////////////////////////////
// use this before output messages so we can track time
///////////////////////////////////////////////////////
void printtime(int id)
{
  char tbuf[100];
  time_t curtime;
  struct tm *curtimestruc;
  time(&curtime);
  curtimestruc = localtime(&curtime);
  strftime(tbuf, 100, "%Y-%m-%d %H:%M:%S",curtimestruc);
  printf("ID %d %s\t", id, tbuf);
}

//////////////////////////////////////
// send a string ending in \n
//////////////////////////////////////
void mealysend(int sendsock, char *b, int bsize, int id)
{
  char sendbuf[MAXDATASIZE + 3] = "";

  strncpy(sendbuf, b, MAXDATASIZE);
  strncat(sendbuf, "\n", MAXDATASIZE+2);

  if (send(sendsock,sendbuf,strlen(sendbuf), 0) == -1){
    perror("Server: send");
    exit(1);
  }
  printtime(id);
  printf("Server \t\tsent: %s",b);
  printf("  -- and it was %d bytes.\n",bsize);
  fflush(stdout);
}

//////////////////////////////////////
// receive up to the first \n
// (but omit the \n)
//	return -2 on EOF (socket closed)
//	return -1 on timeout
//////////////////////////////////////
int mealyrecvto(int recvsock, char *b, int bsize, int to, int id)
{
  int num;
  int selectresult;
  fd_set myset;
  struct timeval tv;  // Time out

  int count = 0;
  char c = '\127';

  memset(b,0,MAXDATASIZE);
  
  while ((count < (bsize-2)) && (c != '\n') && (c != '\0')) {
    FD_ZERO(&myset);
    FD_SET(recvsock, &myset);
    tv.tv_sec = to;
    tv.tv_usec = 0;
    if ((to > 0) &&
	((selectresult = select(recvsock+1, &myset, NULL, NULL, &tv)) <= 0)) {
      // timeout happened (drop what you were waiting for -
      // if it was delimited by \n, it didn't come!
      return -1;
    }
    // got here because select returned >0, i.e., no timeout
    if ((num = recv(recvsock, &c, 1, 0)) == -1) {
      perror("Server: recv failed");
      exit(1);
    }
    if (num == 0) {
      // nothing left to read (socket closed)
      // no need to wait for a timeout; you're already done by now
      return -2;
    }
    b[count] = c;
    count++;
  }
  // at this point, either c is \n, \r or bsize has been reached
  // so just add a string terminator
  char *place;
  place = strchr(b,'\r');
  if (place != NULL) {
    *place = '\0';
  }
  place = strchr(b,'\n');
  if (place != NULL) {
    *place = '\0';
  }

  printtime(id);
  printf("Server \t\treceived :%s: and it was %d bytes.\n",b,(int)strlen(b));
  fflush(stdout);
  return strlen(b);
}

void printstate(statetype thisstate) {
  char idlename[] = "IDLE";
  char hopenname[] = "H-OPEN";
  char openname[] = "OPEN";
  char lockedname[] = "LOCKED";
  char hclosename[] = "H-CLOSE";
  char failname[] = "FAIL";
  char *thisname;

  switch(thisstate) {
  case IDLE:
    thisname = idlename;
    break;
  case HOPEN:
    thisname = hopenname;
    break;
  case OPEN:
    thisname = openname;
    break;
  case LOCKED:
    thisname = lockedname;
    break;
  case HCLOSE:
    thisname = hclosename;
    break;
  case FAIL:
    thisname = failname;
    break;
  }
  printf("Server - STATE = %s\n", thisname);
  fflush(stdout);
}

void quitthread(char *msg, int fd, int id) {
  printtime(id);
  printf(msg);
  fflush(stdout);
  close(fd);
  pthread_exit(NULL);
}

void *server_func(void *new)
{
  char buf1[MAXDATASIZE] = "\0";
//  int *fd = (int *)new;
  funcarg_t *fa = (funcarg_t *)new;
  funcarg_t flocal = *fa;

  int new_fd = flocal.fdval;
  int new_id = flocal.idval;

//  free(fd);
  free(fa);

  /* things we send */
  char dude[MAXDATASIZE] = "DUDE";
  char click[MAXDATASIZE] = "CLICK";
  char clack[MAXDATASIZE] = "CLACK";
  char bzzt[MAXDATASIZE] = "BZZT";
  char ta[MAXDATASIZE] = "TA";

  /* tests we send */
  char baddud[MAXDATASIZE] = "DUD";
  char badcluck[MAXDATASIZE] = "CLUCK";
  char badtata[MAXDATASIZE] = "TATA";


  /* things we receive */
  char yo[MAXDATASIZE] = "YO";
  char sup[MAXDATASIZE] = "SUP";
  char lock[MAXDATASIZE] = "LOCK";
  char trystr[MAXDATASIZE] = "TRY";
  char later[MAXDATASIZE] = "LATER";
  char bye[MAXDATASIZE] = "BYE";
  char magic[MAXDATASIZE] = "MAGIC";

  statetype mystate = IDLE;

  int errorcount = 0;

  // some counts to make sure we test a bunch of cases

  int ringlost;

  int dudtest = 0; // wrong response to YO
  int tatatest = 0; // wrong response to LATER
  int clucktest = 0; // wrong response to TRY#

  int trycount = -1; // max number of tries to allow

  int ringdice; // for testing random rings

  int mtoresult; // mealyto result codes

#define ERRORMAX 20

  // initialize before we start and re-initialize if we get back to IDLE
  if (randring == 1) {
    ringlost = 1; // make sure we lose the first RING
  } else {
    ringlost = FIXEDRING; // make sure we lose the first FIXEDRING RINGs
  }

  while (1) {
    printtime(new_id);
    printstate(mystate); 
    mtoresult = mealyrecvto(new_fd, buf1, MAXDATASIZE, USERWAIT, new_id);
    if (mtoresult == -1) {
      quitthread("Server quit - read timeout.\n", new_fd, new_id);
    }
    if (mtoresult == -2) {
      quitthread("Server quit - socket closed.\n", new_fd, new_id);
    }
    if (strncmp(buf1, magic, strlen(magic)) == 0) {
      quitthread("Server: ending.\n", new_fd, new_id);
    }
    switch (mystate) {
    case IDLE:
      if (strncmp(buf1, yo, strlen(yo)) == 0) {
	printtime(new_id);
	printf("Server got YO.\n");
	fflush(stdout);
	if (randring == 1) {
	  // definitely lose a ring or randomly do so
	  ringdice = rand() % 10;
	  // always lose the first RINGLOST number of rings,
	  // then we have a 50/50 shot
	  if ((ringlost > 0) || (ringdice < 5)) {
	    ringlost--;
	    printtime(new_id);
	    printf("Server lost a YO.\n");
	    fflush(stdout);
	    break;
	  }
	} else {
	  // deterministicially lose the rings
	  if (ringlost > 0) {
	    ringlost--;
	    printtime(new_id);
	    printf("Server lost a YO.\n");
	    fflush(stdout);
	    break;
	  }
	}
	if ((grademode == 1) && (dudtest == 0)) {
	  // give a wrong response too - when grading
	  dudtest = 1;
	  mealysend(new_fd, baddud, strlen(baddud), new_id);
	} else {
	  mystate = HOPEN;
	  mealysend(new_fd, dude, strlen(dude), new_id);
	}
      } else {
	printtime(new_id);
	printf("Server ERROR: MESSAGE NOT EXPECTED: %s\n",buf1);
	fflush(stdout);
	if (errorcount++ > ERRORMAX) {
	  quitthread("Server: error count exceeded. Halting.\n", new_fd, new_id);
	}
      }
      break;
    case HOPEN:
      if (strncmp(buf1, sup, strlen(sup)) == 0) {
	printtime(new_id);
	printf("Server got SUP.\n");
	fflush(stdout);
	mystate = OPEN;
      } else {
	printtime(new_id);
	printf("Server ERROR: MESSAGE NOT EXPECTED: %s\n",buf1);
	fflush(stdout);
	if (errorcount++ > ERRORMAX) {
	  quitthread("Server: error count exceeded. Halting.\n", new_fd, new_id);
	}
      }
      break;
    case OPEN:
      if (strncmp(buf1, lock, strlen(lock)) == 0) {
	printtime(new_id);
	printf("Server got LOCK.\n");
	fflush(stdout);
	mystate = LOCKED;
	trycount = TRYMAX;
      } else {
	printtime(new_id);
	printf("Server ERROR: MESSAGE NOT EXPECTED: %s\n",buf1);
	fflush(stdout);
	if (errorcount++ > ERRORMAX) {
	  quitthread("Server: error count exceeded. Halting.\n", new_fd, new_id);
	}
      }
      break;
    case LOCKED:
      if (strncmp(buf1, later, strlen(later)) == 0) {
	printtime(new_id);
	printf("Server got LATER.\n");
	fflush(stdout);
	if ((grademode == 1) && (tatatest == 0)) {
	  // send wrong ta response - when grading
	  tatatest = 1;
	  mealysend(new_fd, badtata, strlen(badtata), new_id);
	} else {
	  mealysend(new_fd, ta, strlen(ta), new_id);
	  mystate = HCLOSE;
	}
      } else if (strncmp(buf1, trystr, strlen(trystr)) == 0) {
	printtime(new_id);
	printf("Server got TRY.\n");
	fflush(stdout);
	// first make sure the next thing is a single digit number
	int tryval = -1; // invalid value
	tryval = atoi(buf1 + 3);
	if (errno == EINVAL) {
	  printtime(new_id);
	  printf("Server ERROR: invalid TRY value in: %s\n",buf1);
	  fflush(stdout);
	  if (errorcount++ > ERRORMAX) {
	    quitthread("Server: error count exceeded. Halting.\n", new_fd, new_id);
	  }
	  break;
	}
	if ((tryval < 0) || (tryval > 9)) {
	  printtime(new_id);
	  printf("Server ERROR: TRY value out of range: %s\n",buf1);
	  fflush(stdout);
	  if (errorcount++ > ERRORMAX) {
	    quitthread("Server: error count exceeded. Halting.\n", new_fd, new_id);
	  }
	  break;
	}
	// at this point, we know we have a valid TRY attempt...
	// send a bad response maybe first
	if ((grademode == 1) && (clucktest == 0)) {
	  // send wrong cluck response - when grading
	  clucktest = 1;
	  mealysend(new_fd, badcluck, strlen(badcluck), new_id);
	} else {
	  // now see if the try value matches and respond accordingly
	  if (tryval == LOCKCODE) {
	    // winner!
	    mealysend(new_fd, click, strlen(click), new_id);
	    mystate = OPEN;
	  } else {
	    trycount--;
	    if (trycount > 0) {
	      mealysend(new_fd, clack, strlen(clack), new_id);
	      // keep same state - stay here 
	    } else {
	      // exceeded allowed tries - fail
	      printtime(new_id);
	      printf("Server: try code count exceeded.\n");
	      fflush(stdout);
	      mealysend(new_fd, bzzt, strlen(bzzt), new_id);
	      mystate = FAIL;
	    }
	  }
	}
      } else {
	printtime(new_id);
	printf("Server ERROR: MESSAGE NOT EXPECTED: %s\n",buf1);
	fflush(stdout);
	if (errorcount++ > ERRORMAX) {
 	  quitthread("Server: error count exceeded. Halting.\n", new_fd, new_id);
 	}
       }
       break;
     case HCLOSE:
       if (strncmp(buf1, bye, strlen(bye)) == 0) {
	 printtime(new_id);
	 printf("Server got BYE.\n");
	 fflush(stdout);
	 mystate = IDLE;
	 if (randring == 1) {
	   ringlost = 1; // make sure we lose the first RING
	 } else {
	   ringlost = FIXEDRING; // make sure we lose the first FIXEDRING RINGs
	 }
       } else {
	 printtime(new_id);
	 printf("Server ERROR: MESSAGE NOT EXPECTED: %s\n",buf1);
	 fflush(stdout);
	 if (errorcount++ > ERRORMAX) {
	   quitthread("Server: error count exceeded. Halting.\n", new_fd, new_id);
	 }
       }
       break;
    case FAIL:
      if (strncmp(buf1, later, strlen(later)) == 0) {
	printtime(new_id);
	printf("Server got LATER.\n");
	fflush(stdout);
	if ((grademode == 1) && (tatatest == 0)) {
	  // send wrong ta response - when grading
	  tatatest = 1;
	  mealysend(new_fd, badtata, strlen(badtata), new_id);
	} else {
	  mealysend(new_fd, ta, strlen(ta), new_id);
	  mystate = HCLOSE;
	}
      } else {
	printtime(new_id);
	printf("Server ERROR: MESSAGE NOT EXPECTED: %s\n",buf1);
	fflush(stdout);
	if (errorcount++ > ERRORMAX) {
	  quitthread("Server: error count exceeded. Halting.\n", new_fd, new_id);
	}
      }
      break;
    }
  }
}
