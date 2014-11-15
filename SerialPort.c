/* Written by Xiongmin Lin <linxiongmin@gmail.com>, ISIMA, Clermont-Ferrand *
 * (c) 2014. All rights reserved.                                           */

#include "SerialPort.h"

/* OpenPort function is used to open a srrial port                  */
/* device is the name of the serial port, for example /dev/ttyUSB0  */
int OpenPort(const char *device)
{
  struct termios options;
  int fd,i;
//  fd = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);
  fd = open(device, O_RDWR); // open device for read&write
//  fd = open("/dev/ttyp5", O_RDWR); // open device for read&write
  if(fd==-1)
    printf("Open port failed...\n");
  else
  {
    fcntl(fd,F_SETFL,0);
//    fcntl(fd, F_SETFL, FNDELAY); //ne pas bloquer sur le read
    tcgetattr(fd,&options);
    usleep(10000);
    cfsetospeed(&options,B115200);
    cfsetispeed(&options,B115200);
	options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB; /* Parite   : none */
    options.c_cflag &= ~CSTOPB; /* Stop bit : 1    */
    options.c_cflag &= ~CSIZE;  /* Bits     : 8    */
    options.c_cflag |= CS8;
    options.c_cflag &= ~CRTSCTS;
   // options.c_iflag &= ~(IXON);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST; // raw output
    options.c_lflag &= ~(ICANON | ECHO | ECHONL|IEXTEN | ISIG);
    // c_cc
    options.c_cc[VMIN]= 1;
    options.c_cc[VTIME]= 4;
    tcflush(fd,TCIOFLUSH); // flushIO buffer
    tcsetattr(fd, TCSANOW,&options); // set new configure immediately
//    tcflush(fd,TCIOFLUSH);
    usleep(10000);
  }

  return fd;	
}
/*SendFrame is used to send frame, for example, send ACK, tempperature message or control message*/
int SendFrame(int fd, char *frame, int size)
{
  if(fd < 0)
  {
    return -1;
  }
  int i;
  int car;
  char end;
  i   = 0;
  car = 'a';
  end = '/';
  while(frame[i] != '/' && i < MAX_FRAME_SIZE)
  {
    if(write(fd, &frame[i], 1) < 0)
    {
      printf("write failed\n");
      return -1;
    }
    else
    {
      i++;
    }
  }
  if(write(fd, &end, 1) < 0)
  {
    printf("write failed\n");
    return -1;
  }

  return 0;
}
/*RecvFrame is used to receive frame, for example, receive ACK, tempperature message or control message*/
int RecvFrame(int fd, char *frame)
{
  if(fd < 0)
  {
    return -1;
  }
  int i;
  char car;
  i   = 1;
  car = 'a';
  while (car != '*')
  {
    read(fd, &car, 1);
    //printf("%c ", car);
  }
  frame[0] = '*';
  while(car != '/' && i < MAX_FRAME_SIZE && car != EOF)
  {
    if(read(fd, &car, 1) < 0)
    {
      printf("recv failed\n");
      return -1;
    }
    frame[i] = car;
    i++;
  }
  return 0;
}
