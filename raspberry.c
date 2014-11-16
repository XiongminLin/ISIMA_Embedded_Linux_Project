/*
* raspberry.c is running in raspberry board(server), there are two threads   *
* the main thread: receive message from server, such as ACK, frequency       * 
* message and stop message.                                                  *
* the SendThread: send message to the sensor.                                *
* two threads can communicate with each other using global variables (fd,stop*
* fre, needsendACK, haverecvACK), fd variable is for serial port; server can *
* stop sensor by setting stop variable equals to 1; server can modify the s- *
* ending frequency by setting fre variable; needsendACK will be set to 1 if  *
* server receive a temperature frame to tell the sendthread to send an ACK   *
* frame, after the sendthread have sent the ACK frame, needsendACK will be   *
* set back to 0; When server sends a control message(frequency or stop), to  *
* assure that the sensor have received it, server need to check haverecvACK. *
* if server have received the ACK from sensor, it will set haverecvACK = 1.  *
* I use five mutexes to protect those five global variables.                 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <pthread.h>
#include <mysql.h>
#include "SerialPort.h"

int fd;
int stop;
int fre;
int needsendACK;
int haverecvACK;
pthread_mutex_t stop_mutex;
pthread_mutex_t fre_mutex;
pthread_mutex_t need_send_ack_mutex;
pthread_mutex_t have_recv_ack_mutex;
pthread_mutex_t fd_mutex;

/*InsertToSQL function is used to store the temperature data into mysql  *
* the input frame should be something like "*01+21.501330150210/"        */
int InsertToSQL(char *frame)
{
        char start[2] = {'\0'};
        char type[3]  = {'\0'};
        char value[7] = {'\0'};
        char hour[5]  = {'\0'};
        char date[7]  = {'\0'};
        char end[2]   = {'\0'};
	printf("frame : %s\n", frame);

        memcpy(start, frame     , 1);
        memcpy(type,  frame + 1 , 2);
        memcpy(value, frame + 3 , 6);
        memcpy(hour,  frame + 9 , 4);
        memcpy(date,  frame + 13, 6);
        memcpy(end,   frame + 19, 1);
	printf("type : %s\n", type);
	printf("value: %s\n", value);
	printf("hour : %s\n", hour);
	printf("date : %s\n", date);
  	MYSQL      *conn;
  	//MYSQL_RES  *res;
  	MYSQL_ROW  row;
  	char *server = "localhost";
  	char *user = "root";
  	char *password = "123456"; /* set me first */
  	char *database = "test";
  	conn = mysql_init(NULL);
  	//char *query = "insert into temperature(start,type,value,hour,date,end) values('*','01','+21.50','1330','150210','/')";
  	int t, r;
	char RxChar[21]="*01+21.501330150210/";
  	char InsertSQL[200]= {'\0'};
	printf("insert sql-1: %s\n", InsertSQL);
  	sprintf(InsertSQL, "insert into temperature(start,type,value,hour,date,end) values('*','%s','%s','%s','%s','/')", 
                type, value, hour, date);
	printf("insert sql-2: %s\n", InsertSQL);
 
  	//link into mysql
  	if(!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0))
  	{
  		printf("Error connecting to database: %s\n", mysql_error(conn));
                return -1;
  	}
  	else
  	{
//  		printf("Connected....\n");
  	}
  	t = mysql_query(conn, InsertSQL);
 	if(t)
        {
         	printf("Error making query: %s\n", mysql_error(conn));
                return -1;
        }
        else
        {
		printf("Write the data: %s into mysql successfully!\n", RxChar);
        }
  	mysql_close(conn);
  	return t;
}
/* show_state function is used to show the system state:  *
*  the frequency,                                         *
*  whether need send ACK or not,                          *
*  whether have receive ACK or not,                       *
*  whether receive stop message                           */
void show_state()
{
  printf("fd = %d, fre = %d, needsendACK = %d, haverecvACK = %d, stop = %d \n", fd, fre, needsendACK, haverecvACK, stop);
}

/*AnalyseFrame function is used to analyse the received message *
* is it ACK?                                   if YES return 0  *
* is it temperature data?                      if YES return 1  *
* is it frequency control message?             if YES return 2  *
* is it stop message?                          if YES return 3  */
int AnalyseFrame(const char *frame)
{
  if(frame[2] == '0')      //ACK
  {
//     printf("hi, recv ACK %s\n", frame);
     return 0;

  }else if(frame[2] == '1') //temp data
  {
    return 1;

  }else if(frame[2] == '2')// change the sample frequency
  {
    return 2;
  }
  else if(frame[2] == '3')//switch off the sensor
  {
    return 3;
  }
  return -1;
}
/* RecvThreadFun function is called by recv-thread, it is used to *
*  receive messages sending from the sensor.                      * 
*  if it receives a tempe-message, it will set needsendACK = 1    *
*  (to tell send thread that he should send an ACK to the sensor  *
*   saying "got it"). then call the InsertToSQL function to store *
*  if receive a ACK message, it will set haverecvACK = 1, to let  *
*  the send thread knows that the sensor had received the control *
*  message.                                                       */
void RecvThreadFun(void *ptr)
{
    printf("start recv thread\n");
    show_state();
    char recvframe[MAX_FRAME_SIZE + 1];
    while(stop != 1)
    {
      memset(recvframe, '\0', MAX_FRAME_SIZE + 1);

      pthread_mutex_lock(&fd_mutex);
      int result = RecvFrame(fd, recvframe);
      pthread_mutex_unlock(&fd_mutex);
      if(result == 0)
      {
        int datamode = AnalyseFrame(recvframe);
        if(datamode == 1)
        {
           printf("\nrecv temp frame: %s\n", recvframe);
           pthread_mutex_lock(&need_send_ack_mutex);
           needsendACK = 1; //recv frame, remind main thread to send ack
           show_state();
           pthread_mutex_unlock(&need_send_ack_mutex);
           InsertToSQL(recvframe);
           sleep(fre);
        }
        else if(datamode == 0)
        {
           printf("recv ACK frame: %s\n", recvframe);
           pthread_mutex_lock(&have_recv_ack_mutex);
           haverecvACK = 1;
           show_state();
           pthread_mutex_unlock(&have_recv_ack_mutex);
        }
        else if(datamode == 2)
        {
          
          printf("recv: change frequency %s\n",recvframe);
          //char delayAscii[7];
          /*extract the number from the frame */
          //memcpy(delayAscii, recvframe + 3, 6);
          //delayAscii[6] = '\0';
          //pthread_mutex_lock(&fre_mutex);
          //fre = atol((const char *) delayAscii);
          //pthread_mutex_unlock(&fre_mutex);
          //needsendACK = 1;
          //show_state();
          
        }
        else if(datamode == 3)
        {
          printf("recv: STOP frame: %s\n", recvframe);
          //pthread_mutex_lock(&stop_mutex);
          //stop = 1;
          //pthread_mutex_unlock(&stop_mutex);
          //needsendACK = 1;
          //show_state();
        }
        else if(datamode == -1)
        {
          printf("but it is wrong message\n");
        }
     }
     else
     {
       printf("recv nothing\n");
       sleep(1);
     }
   }
}
/*main thread is a send-thread, it is responsible for sending ACK message  * 
* and control-message in a loop. at the beginning of the loop, main thread *
* read frequency and stop-value from /home/pi/server/www/temperature.txt   *
* and then sent it */
int main()
{
  pthread_t recvthread;
  char ackframe[8]  = "*00ACK/\n";
  char freframe[11] = "*02000001/";
  char stopframe[9] = "*03STOP/"; 
//  char *device = "/dev/ttyAMA0";
//  char *device = "/dev/ttyUSB0";
  char *device = "/dev/pts/5";
  pthread_mutex_init(&stop_mutex, NULL);
  pthread_mutex_init(&fre_mutex, NULL);
  pthread_mutex_init(&need_send_ack_mutex, NULL);
  pthread_mutex_init(&have_recv_ack_mutex, NULL);
  pthread_mutex_init(&fd_mutex, NULL);
  FILE *file;
  int read_fre;
  printf("trying to open serial port...\n");
  fd = OpenPort(device);
//  fd = OpenVirtualPort(device);
  if(fd <= 0)
  {
    printf("open serial port failed\n");
  }
  else
  {
    printf("open serial port success\n");

  }
  //char delayAscii[7];



//  while((read = getline(&line, &len, file)) != -1) {
//           printf("Retrieved line of length %zu :\n", read);
//           printf("%s", line);
//       }
  fre  = 0;
  stop = 0;
  needsendACK = 0;
  haverecvACK = 0;

  pthread_create(&recvthread, NULL, (void *)&RecvThreadFun, NULL);
  usleep(50000);

  while(1)
  {
    /*read frequency value and stop info from file */
    //read frequency stop info from file//
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    file = fopen("/home/lin/server/www/temperature.txt", "r");
    if (file == NULL)
    {
      printf("open file error\n");
      return -1;
    }
//    printf("read frequency and stop info from file: /home/lin/server/www/temperature.txt\n");
    read = getline(&line, &len, file);
    read_fre = atol((const char *) line);
//    printf("fre: %d\n", fre);
    read = getline(&line, &len, file);
    stop = atol((const char *) line);
//    printf("stop: %d\n", stop);
    fclose(file);

    /*if user wants to stop the sensor, server will send a stop-frame to sensor */

    if(stop == 1)
    {
       printf("sending frequency-message...\n");
       pthread_mutex_lock(&have_recv_ack_mutex);
       haverecvACK = 0;
       pthread_mutex_unlock(&have_recv_ack_mutex);

       pthread_mutex_lock(&fd_mutex);
       SendFrame(fd, stopframe, 8);
       pthread_mutex_unlock(&fd_mutex);
       sleep(1);
       while(haverecvACK != 1)
       {

         SendFrame(fd, stopframe, 8);
         printf("no receive ACK, will send frequency-message again...\n");
         show_state();
         sleep(1);
       }
       printf("system stop!\n");
       break;
    }

    /*if user modify the frequency, server will send the new frequency value to sensor*/

    if(read_fre != fre)
    {
       printf("sending frequency-message...\n");
       pthread_mutex_lock(&have_recv_ack_mutex);
       haverecvACK = 0;
       pthread_mutex_unlock(&have_recv_ack_mutex);
 
       pthread_mutex_lock(&fre_mutex);
       fre = read_fre;
       sprintf(freframe, "*02%06d/", fre);
       SendFrame(fd, freframe, 10);
       pthread_mutex_unlock(&fre_mutex);
       
       sleep(1);
       
       while(haverecvACK != 1)
       {

         SendFrame(fd, freframe, 11);
         printf("no receive ACK, will send frequency-message again...\n");
         sleep(1);
       }
    }



    /*if need send ACK, then send an ACK, and set needsendACK back to 0*/
    if(needsendACK == 1)
    {
      printf("sending ACK...\n");

      pthread_mutex_lock(&fd_mutex);
      SendFrame(fd, ackframe, 7);
      pthread_mutex_unlock(&fd_mutex);

      tcflush(fd,TCIOFLUSH); //???
      pthread_mutex_lock(&need_send_ack_mutex);
      needsendACK = 0;
      show_state();
      pthread_mutex_unlock(&need_send_ack_mutex);
    }
    usleep(100000);

  }

  pthread_join(recvthread, NULL);
  return 0;
}
