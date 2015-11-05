/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h>

#define SVRPORT "55001"
#define SVRNAME "dns.postel.org"


#define USER_INPUT_LEN 80
#define MAXDATASIZE 1000

//////////////////////////////////////
// send a string ending in \n
//////////////////////////////////////
void mealysend(int sendsock, char *b, int bsize)
{
  char sendbuf[MAXDATASIZE + 3] = "";

  strncpy(sendbuf, b, MAXDATASIZE);
  strncat(sendbuf, "\n", MAXDATASIZE+2);

  if (send(sendsock,sendbuf,strlen(sendbuf), 0) == -1){
    perror("Client: send");
    exit(1);
  }
  printf("Client sent: %s",b);
  printf("  -- and it was %d bytes.\n",bsize);
  fflush(stdout);
}

//////////////////////////////////////
// receive up to the first \n
// (but omit the \n)
//	return 0 on EOF (socket closed)
//	return -1 on timeout
//////////////////////////////////////
int mealyrecvto(int recvsock, char *b, int bsize, int to)
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
      perror("Client: recv failed");
      exit(1);
    }
    if (num == 0) {
      // nothing left to read (socket closed)
      // no need to wait for a timeout; you're already done by now
      return 0;
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

  printf("Client received :%s: and it was %d bytes.\n",b,(int)strlen(b));
  return strlen(b);
}

void mealy(int msock)
{
  char buf[MAXDATASIZE] = "";
  int state = 0; //state 0 correspond to RED State
  char buf2[MAXDATASIZE] = "";
  char userInput[USER_INPUT_LEN] = "";
  int numbytes = 1;

  while (numbytes != 0){
    if (state == 0){ //RED State
      printf("Client state: RED\n");
      printf ("Are you ready to start? If yes, type go:\n");
      scanf ("%s", userInput);
      while (strcmp(userInput,"go")){
        printf ("Invalid Input. You entered: %s\n", userInput);
        printf ("Are you ready to start? If yes, type go:\n");
        scanf ("%s", userInput);
      }
      printf ("User Input Recieved: %s\n", userInput);
      memset(userInput, 0, sizeof userInput); 
      strcpy(buf,"YO");  
      mealysend(msock,buf,strlen(buf)); 
      printf("Client state transition: RED -> GREEN\n");
      state = 1; //
    }
    if (state == 1){ //GREEN state
      printf("Client state: GREEN\n");
      numbytes = mealyrecvto(msock, buf2, MAXDATASIZE, 5);
      strcpy(buf,"YO");
      while (strcmp(buf2,"DUDE")){
        printf("Time-out in GREEN state. Sending YO again\n");    
        mealysend(msock,buf,strlen(buf)); // send YO again
        numbytes = mealyrecvto(msock, buf2, MAXDATASIZE, 5); //check for recieved status from server
      }
      memset(buf2, 0, sizeof buf2);
      memset(buf, 0, sizeof buf);
      strcpy(buf,"SUP");
      mealysend(msock,buf,strlen(buf));
      printf("Client state transition: GREEN -> BLUE\n");
      memset(buf, 0, sizeof buf);
      state = 2; //BLUE
    }
    if (state == 2){ //BLUE state
      printf("Client state: BLUE\n");
      printf ("To shutdown the system, type shut:\n");
      scanf ("%s", userInput);
      while (strcmp(userInput,"shut")){
        printf ("Invalid Input. You entered: %s\n", userInput);
        printf ("To shutdown the system, type shut:\n");
        scanf ("%s", userInput);
      }
      printf ("User Input Recieved: %s\n", userInput);
      memset(userInput, 0, sizeof userInput);
      strcpy(buf,"LOCK");
      mealysend(msock,buf,strlen(buf));
      printf("Client state transition: BLUE -> ORANGE\n");
      memset(buf, 0, sizeof buf);
      state = 3; //ORANGE     
    } 
    if (state == 3){ //ORANGE state
      printf("Client state: ORANGE\n");
      printf ("Enter a single digit number(0-9). If you want to quit, type stop:\n");
      scanf ("%s", userInput);
      while (strcmp(userInput,"stop") && !(userInput[0] <= '9' && userInput[0] >= '0' && strlen(userInput)==1)){
        printf ("Invalid Input. You entered: %s\n", userInput);
        printf ("Enter a single digit number(0-9).If you want to quit, type stop:\n");
        scanf ("%s", userInput);
      }
      printf ("User Input Recieved: %s\n", userInput);
      if (userInput[0] <= '9' && userInput[0] >= '0'){
        char src[USER_INPUT_LEN];
        strcpy(src,  "TRY");
        strcat(src, userInput);
        strcpy(buf,src);
        mealysend(msock,buf,strlen(buf));
        printf("Client state transition: ORANGE -> PURPLE\n");
        state = 4; //PURPLE
      } else if(!strcmp(userInput,"stop")){
        strcpy(buf,"LATER");
        mealysend(msock,buf,strlen(buf));
        printf("Client state transition: ORANGE -> YELLOW\n");
        state = 5; //YELLOW
      } else {
        printf("ERROR: Both single digit number + stop detected\n");
      }
    memset(buf, 0, sizeof buf);
    memset(userInput, 0, sizeof userInput);
    }
    if (state == 4){ //PURPLE state
      printf("Client state: PURPLE\n");
      numbytes = mealyrecvto(msock, buf2, MAXDATASIZE, 5);
      if (numbytes == -1){
        printf("Time-out in PURPLE state. Client state transition: PURPLE -> ORANGE\n");
        state = 3;      
      }
      else if (!strcmp(buf2,"CLACK")){
        printf("You chose incorrect number, but you can have more tries!\n");
        printf("Client state transition: PURPLE -> ORANGE\n");
        state = 3;    
      }
      else if (!strcmp(buf2,"CLICK")){
        printf("You chose correct number!\n");
        printf("Client state transition: PURPLE -> BLUE\n");
        state = 2;    
      }
      else if (!strcmp(buf2,"BZZT")){
        printf("You chose incorrect number. You are locked out!\n");
        printf("Client state transition: PURPLE -> YELLOW\n");
        strcpy(buf,"LATER");
        mealysend(msock,buf,strlen(buf));  
        memset(buf, 0, sizeof buf);
        state = 5;
      }
      else{
        printf("ERROR: Server didnt send correct response. Still in state PURPLE\n");
      }
      memset(buf2, 0, sizeof buf2);
    }
    if (state == 5){ //YELLOW
     printf("Client state: YELLOW\n"); 
     numbytes = mealyrecvto(msock, buf2, MAXDATASIZE, 5);
      strcpy(buf,"LATER");
      while (strcmp(buf2,"TA")){
        printf("Time-out in YELLOW state. Sending LATER again\n");    
        mealysend(msock,buf,strlen(buf)); // send YO again
        numbytes = mealyrecvto(msock, buf2, MAXDATASIZE, 5); //check for recieved status from server
      }
      memset(buf2, 0, sizeof buf2);
      memset(buf, 0, sizeof buf);
      strcpy(buf,"BYE");
      mealysend(msock,buf,strlen(buf));
      printf("Client state transition: YELLOW -> RED\n");
      memset(buf, 0, sizeof buf);
      state = 0; //BLUE 
    }
  }
//  numbytes = mealyrecvto(msock, buf, MAXDATASIZE, 5);

  ////// Receive with timeout (returns 0 on timeout)
//  numbytes = mealyrecvto(msock, buf, MAXDATASIZE, 5);
  if (numbytes == -1) {
    printf("Client timeout.\n");
  } else if (numbytes == 0) {
    printf("Client stop; server socket closed.\n");
  }
}

int main(int argc, char *argv[])
{
  int sockfd, sen, valopt;  
  
  struct hostent *hp;
  struct sockaddr_in server;
  socklen_t lon;
  
  extern char *optarg;
  extern int optind;
  int c;
  char *sname=SVRNAME, *pname = SVRPORT;
  static char usage[] = "usage: %s [-s sname] [- p pname]\n";
  
  long arg;
  fd_set myset;
  struct timeval tv;  // Time out
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  // parse the command line arguments
  while ((c = getopt(argc, argv, "s:p:")) != -1)
    switch (c) {
    case 'p':
      pname = optarg;
      break;
    case 's':
      sname = optarg;
      break;
    case '?':
      fprintf(stderr, usage, argv[0]);
      exit(1);
      break;
    }

  // convert the port string into a network port number
  server.sin_family = AF_INET;
  server.sin_port = htons(atoi(pname));

  // convert the hostname to a network IP address
  hp = gethostbyname(sname);
  if ((hp = gethostbyname(sname)) == NULL) {  // get the host info
    herror("Client: gethostbyname failed");
    return 2;
  }
  bcopy(hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
  bzero(&server.sin_zero, 8);

  // get a socket to play with
  if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
    perror("Client: could not create socket");
    exit(-1);
  }
  
  //////////////////////////////////////
  // open the connection
  
  // set the connection to non-blocking 
  arg = fcntl(sockfd, F_GETFL, NULL);
  arg |= O_NONBLOCK;
  fcntl(sockfd, F_SETFL, arg);
  //
  if ((sen = connect(sockfd,(struct sockaddr *)&server,sizeof(server))) == -1) {
    if (errno == EINPROGRESS) {
      FD_ZERO(&myset);
      FD_SET(sockfd, &myset);
      if (select(sockfd + 1, NULL, &myset, NULL, &tv) > 0){
	lon = sizeof(int);
	getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);
	if (valopt) {
	  fprintf(stderr, "Client error in connection() %d - %s\n", valopt, strerror(valopt));
	  exit(-1);
	}
      }
      else {
	perror("Client: connection time out");
	exit(-1);
      }				
    }
    else {
      fprintf(stderr, "Client error connecting %d - %s\n", errno, strerror(errno));
      exit(0);
    }
  }
  //
  ///////////////////////////////
  
  // Set to blocking mode again
  arg = fcntl(sockfd, F_GETFL, NULL);
  arg &= (~O_NONBLOCK);
  fcntl(sockfd, F_SETFL, arg);

  //////////////////////////////

  mealy(sockfd);
  // and now it's done
  //////////////////////////////
  
  close(sockfd);
  return 0;
}
