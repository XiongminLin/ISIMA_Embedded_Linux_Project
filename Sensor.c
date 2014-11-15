/*
* Written by Xiongmin Lin <linxiongmin@gmail.com>, ISIMA, Clermont-Ferrand *
* (c) 2014. All rights reserved.                                           *
* Sensor.c is running in temperature sensorm, there are two threads        *
* the main thread: receive message from server, such as ACK, frequency     *
* message and stop message. the SendThread: send message to the server.    */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <pthread.h>
#include "SerialPort.h"

int fd;
int stop;
int fre;
int haverecvACK;
int needsendACK;
pthread_mutex_t stop_mutex;
pthread_mutex_t fre_mutex;
pthread_mutex_t have_recv_ack_mutex;
pthread_mutex_t need_send_ack_mutex;
void GetFrame(char *frame)
{
   time_t rawtime;
   struct tm * timeinfo;
   time (&rawtime);
   timeinfo = localtime ( &rawtime );
   double temp = -10.50 + random()%30;
   sprintf(frame,"*01%s%02d.%02d%02d%02d%02d%02d%02d/",
           (temp < 0 ? "":"+"), (int)temp,(int)((temp - (int)temp)*100),timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_mday, timeinfo->tm_mon, timeinfo->tm_year - 100);

}

/* show_state function is used to show the system state:  *
*  the frequency,                                         *
*  whether need send ACK or not,                          *
*  whether have receive ACK or not,                       *
*  whether receive stop message                           */
void show_state()
{
  printf("fd = %d, fre = %d, needsendACK = %d, haverecvACK = %d, stop = %d\n", fd, fre, needsendACK, haverecvACK, stop);
}

/*AnalyseFrame function is used to analyse the received message *
* is it ACK?                                   if YES return 0  *
* is it temperature data?                      if YES return 1  *
* is it frequency control message?             if YES return 2  *
* is it stop message?                          if YES return 3  */
int AnalyseFrame(char *frame)
{
  if(frame[2] == '0')      //ACK
  {
     
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

/* SendThreadFun function is called by send-thread, it is used to *
*  send messages to the server.                                   * 
*  if it sends a temperature message, it will set haverecvACK = 0 *
*  (to tell recv thread that he should recv an ACK from server    *
*   saying "got it"). it will always send the temperature message *
*  until recv-thread receive an ACK message                       */
void SendThreadFun(void *ptr)
{
    printf("start send thread...");
    char frame[MAX_FRAME_SIZE];
    char ackframe[7] = "*00ACK/";
    show_state();

    while(stop != 1)
    {
      //if sendACK == 1, it means that system has receive a control message, so you should send an ACK, then set sendACK back to 0//
      if(needsendACK == 1)
      {
        SendFrame(fd, ackframe, 7);
        needsendACK = 0;
      }

      //get a temp and send //
      GetFrame(frame);
      printf("sending %s, fre: %d s\n", frame, fre);
      SendFrame(fd, frame, MAX_FRAME_SIZE);
      
      /*reset haverecvACK to 0, make sure that new frame with new ACK*/
      pthread_mutex_lock(&have_recv_ack_mutex);
      haverecvACK = 0;
      pthread_mutex_unlock(&have_recv_ack_mutex);
      show_state();
      sleep(fre);

      while(haverecvACK != 1 ) //ACK
      {
        if(stop == 1) break;   // stop
        printf("no recv ACK, will send again: %s, fre: %d\n", frame, fre);  
        SendFrame(fd, frame, MAX_FRAME_SIZE);
        show_state();
        sleep(fre);
        if(needsendACK == 1)
        {
          SendFrame(fd, ackframe, 7);
          needsendACK = 0;
        }
      }

    }

}
/* main function is a recv-thread, it is used to receive messages   *
*  sending from the senrver.                                        * 
*  if it receives a control-message, it will set needsendACK = 1    *
*  (to tell send thread that he should send an ACK to the server    *
*   saying "got it").                                               *
*  if receive a ACK message, it will set haverecvACK = 1, to let    *
*  the send thread knows that the server had received a temperature *
*  message.                                                         */
int main()
{
  pthread_t sendthread;
  char recvframe[MAX_FRAME_SIZE] = "ABCDEFG";
//  char *device = "/dev/ttyAMA0";
  char *device = "/dev/ttyUSB0";
  fre  = 1;
  stop = 0;
  haverecvACK = 0;
  needsendACK = 0;
  pthread_mutex_init(&stop_mutex, NULL);
  pthread_mutex_init(&fre_mutex, NULL);
  pthread_mutex_init(&have_recv_ack_mutex, NULL);
  pthread_mutex_init(&need_send_ack_mutex, NULL);
  fd = OpenPort(device);
  if(fd < 0) return -1;
  pthread_create(&sendthread, NULL, (void *)&SendThreadFun, NULL);

  while(stop != 1)
  {

    /*receive a frame and analyse it*/
    RecvFrame(fd, recvframe);
    int recvmode = AnalyseFrame(recvframe);
                                         
    if(recvmode == 0)
    {
       printf("recv: ACK\n");
       pthread_mutex_lock(&have_recv_ack_mutex);
       haverecvACK = 1;
       show_state();
       pthread_mutex_unlock(&have_recv_ack_mutex);
    }

    /* if it receives a frequency-message, it will set needsendACK = 1 *
     *  (to tell send thread that he should send an ACK to the server  *
     *   saying "got it").                                             */
    if(recvmode == 2 ) 
    {
      printf("recv: change frequency %d\n", fre);
      char delayAscii[7];
      /*extract the number from the recv frame */
      memcpy(delayAscii, recvframe + 3, 6);
      delayAscii[6] = '\0';
      pthread_mutex_lock(&fre_mutex);
      fre = atol((const char *) delayAscii);
      pthread_mutex_unlock(&fre_mutex);

      pthread_mutex_lock(&need_send_ack_mutex);
      needsendACK = 1;
      show_state();
      pthread_mutex_unlock(&need_send_ack_mutex);
    }

    /* if it receives a stop-message, it will set needsendACK = 1      *
     *  (to tell send thread that he should send an ACK to the server  *
     *   saying "got it").                                             */
    if(recvmode == 3)
    {

      printf("recv: STOP!\n");
      pthread_mutex_lock(&stop_mutex);
      stop = 1;
      pthread_mutex_unlock(&stop_mutex);

      pthread_mutex_lock(&need_send_ack_mutex);
      needsendACK = 1;
      show_state();
      pthread_mutex_unlock(&need_send_ack_mutex);
    }

    memset(recvframe, '\0', MAX_FRAME_SIZE);
    usleep(500000);
  }
  pthread_join(sendthread, NULL);
  return 0;
}
