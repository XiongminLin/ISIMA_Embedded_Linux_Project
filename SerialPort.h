#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#define MAX_FRAME_SIZE 20
int OpenPort(const char *device);
int OpenVirtualPort(const char *device);
int SendFrame(int fd, char *frame, int size);
int RecvFrame(int fd, char *frame);

