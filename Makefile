#compile

all : Sensor.o SerialPort.o raspberry.o
	gcc Sensor.o SerialPort.o -o Send -lpthread
	gcc raspberry.o SerialPort.o -o Recv -lpthread -lmysqlclient
Sensor.o : Sensor.c
	gcc -c Sensor.c
raspberry.o : raspberry.c
	gcc -c raspberry.c -I /usr/include/mysql 
SerialPort.o : SerialPort.c SerialPort.h
	gcc -c SerialPort.c
clean : Sensor.o SerialPort.o raspberry.o
	rm Sensor.o SerialPort.o raspberry.o
