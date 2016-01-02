error.o: apue.h error.c
	gcc -g -c error.c -o error.o

errorlog.o: apue.h errorlog.c
	gcc -g -c errorlog.c -o errorlog.o

# signal()
10-18.o: apue.h 10-18.c
	gcc -g -c 10-18.c -o 10-18.o

# signal_intr()
10-19.o: apue.h 10-19.c
	gcc -g -c 10-19.c -o 10-19.o

# daemonize()
13-1.o: apue.h 13-1.c
	gcc -g -c 13-1.c -o 13-1.o

# lock_reg()
14-5.o: apue.h 14-5.c
	gcc -g -c 14-5.c -o 14-5.o

# readn() and writen()
14-24.o: apue.h 14-24.c
	gcc -g -c 14-24.c -o 14-24.o

# connect_retry()
16-11.o: apue.h 16-11.c
	gcc -g -c 16-11.c -o 16-11.o

# initserver()
16-22.o: apue.h 16-22.c
	gcc -g -c 16-22.c -o 16-22.o

# serv_listen()
17-8.o: apue.h 17-8.c
	gcc -g -c 17-8.c -o 17-8.o

# serv_accept()
17-9.o: apue.h 17-9.c
	gcc -g -c 17-9.c -o 17-9.o

# cli_conn()
17-10.o: apue.h 17-10.c
	gcc -g -c 17-10.c -o 17-10.o

# send_err()
17-12.o: apue.h 17-12.c
	gcc -g -c 17-12.c -o 17-12.o

# send_fd()
17-13.o: apue.h 17-13.c
	gcc -g -c 17-13.c -o 17-13.o

# recv_fd()
17-14.o: apue.h 17-14.c
	gcc -g -c 17-14.c -o 17-14.o

# open server process 1st version
open_serv1st:  error.o 14-24.o 17-2.o 17-12.o 17-13.o 17-14.o 17-18.o	\
	17-19.o 17-21.o 17-22.o 17-23.o 17-24.o
	gcc -g error.o 14-24.o 17-2.o 17-12.o 17-13.o 17-14.o		\
		17-18.o 17-19.o -o open_serv1st
	gcc -g error.o 14-24.o 17-12.o 17-13.o 17-21.o 17-22.o 17-23.o 	\
		17-24.o -o opend

# open server process 1st version, the client
17-18.o: open.h 17-18.c
	gcc -g -c 17-18.c -o 17-18.o

# csopen()
17-19.o: open.h 17-19.c
	gcc -g -c 17-19.c -o 17-19.o

# server process main() function
17-21.o: opend.h 17-21.c
	gcc -g -c 17-21.c -o 17-21.o

# handle_request() 1st version
17-22.o: opend.h 17-22.c
	gcc -g -c 17-22.c -o 17-22.o

# buf_args()
17-23.o: opend.h 17-23.c
	gcc -g -c 17-23.c -o 17-23.o

# cli_args()
17-24.o: opend.h 17-24.c
	gcc -g -c 17-24.c -o 17-24.o

# csopen() 2nd version
17-25.o: open.h 17-25.c
	gcc -g -c 17-25.c -o 17-25.o

# client_add(), client_del()
17-27.o: opend.h 17-27.c
	gcc -g -c 17-27.c -o 17-27.o

# open server process main() function 2nd version
17-28.o: opend.h 17-28.c
	gcc -g -c 17-28.c -o 17-28.o

# loop() using select()
17-29.o: opend.h 17-29.c
	gcc -g -c 17-29.c -o 17-29.o
# loop() using poll()
17-30.o: opend.h 17-30.c
	gcc -g -c 17-30.c -o 17-30.o

# handle_request() for open server 2nd version
17-31.o: opend.h 17-31.c
	gcc -g -c 17-31.c -o 17-31.o

# open server process 2nd version
open_serv2nd:  opend.h error.o 13-1.o 14-24.o 17-2.o 17-8.o 17-9.o	\
	17-10.o 17-12.o 17-13.o 17-14.o 17-18.o 17-23.o 17-24.o		\
	17-25.o 17-27.o 17-28.o 17-29.o 17-31.o
	gcc -g error.o 14-24.o 17-2.o 17-10.o 17-12.o 17-13.o 17-14.o	\
		17-18.o 17-25.o -o open_serv2nd
	gcc -g error.o errorlog.o 13-1.o 14-24.o 17-8.o 17-9.o 17-10.o 	\
		17-12.o 17-13.o 17-14.o	17-23.o 17-24.o 17-27.o 17-28.o	\
		17-29.o 17-31.o -o opend

c-24: apue.h c-24.c
	gcc -g error.o c-24.c -o c-24 -lpthread

# buf_args() use malloc() or realloc()
17.4.o: apue.h 17.4.c
	gcc -g -c 17.4.c -o 17.4.o

# loop() using select() optimized
17.5.1.o: opend.h 17.5.1.c
	gcc -g -c 17.5.1.c -o 17.5.1.o

# loop() using poll() optimized
17.5.2.o: opend.h 17.5.2.c
	gcc -g -c 17.5.2.c -o 17.5.2.o

18-10.o: apue.h 18-10.c
	gcc -g -c 18-10.c -o 18-10.o

18-10: 18-10.c error.o 18-10.o
	gcc -g error.o 18-10.o -o 18-10

18-11.o: apue.h 18-11.c
	gcc -g -c 18-11.c -o 18-11.o

18-11: 18-11.c error.o 18-11.o
	gcc -g error.o 18-11.o -o 18-11

# ctermid()
18-12.o: 18-12.c
	gcc -g -c 18-12.c -o 18-12.o

# isatty()
18-13.o: 18-13.c
	gcc -g -c 18-13.c -o 18-13.o

18-14: 18-14.c 18-13.o apue.h
	gcc -g 18-14.c 18-13.o -o 18-14

# ttyname()
18-15.o: 18-15.c
	gcc -g -c 18-15.c -o 18-15.o

18-16: apue.h 18-15.o 18-16.c
	gcc -g 18-15.o 18-16.c -o 18-16

# getpass()
18-17.o: 18-17.c
	gcc -g -c 18-17.c -o 18-17.o

18-18: apue.h error.o 18-17.o 18-18.c
	gcc -g error.o 18-17.o 18-18.c -o 18-18

# tty_cbreak(), ty_raw() tty_reset() and tty_atexit()
18-20.o: apue.h 18-20.c
	gcc -g -c 18-20.c -o 18-20.o

18-21: apue.h 18-20.o error.o 18-21.c
	gcc -g 18-20.o error.o 18-21.c -o 18-21

18-22: apue.h error.o 10-18.o 18-22.c
	gcc -g error.o 10-18.o 18-22.c -o 18-22

18.1: apue.h error.o 18-20.o 18.1.c
	gcc -g error.o 18-20.o 18.1.c -o 18.1

# ptym_open() and ptys_open()
19-9.o: apue.h 19-9.c
	gcc -g -c 19-9.c -o 19-9.o

# pty_fork()
19-10.o: apue.h 19-10.c
	gcc -g -c 19-10.c -o 19-10.o

pty: apue.h error.o 10-19.o 14-24.o 17-2.c 19-11.c 18-20.o 19-9.o	\
	19-10.o	19-12.c 19-16.c
	gcc -g error.o 10-19.o 14-24.o 17-2.c 19-11.c 18-20.o 19-9.o	\
		19-10.o	19-12.c	19-16.c -o pty

# loop() using select differs with loop() in 19-12.c
19.3.o: apue.h 19.3.c
	gcc -g -c 19.3.c -o 19.3.o

pty_: apue.h error.o 10-19.o 14-24.o 17-2.c 19-11.c 18-20.o 19-9.o	\
	19-10.o	19.3.c 19-16.c
	gcc -g error.o 10-19.o 14-24.o 17-2.c 19-11.c 18-20.o 19-9.o	\
		19-10.o	19.3.c	19-16.c -o pty_

20-3: apue.h apue_db.h 20-3.c error.o db.o 14-5.o
	gcc 20-3.c error.o db.o 14-5.o -o 20-3

print: ipp.h print.h apue.h util.c print.c error.o errorlog.o 14-24.o	\
	16-11.o
	gcc util.c print.c error.o errorlog.o 14-24.o 16-11.o -o print

printd: apue.h ipp.h print.h printd.c 13-1.o errorlog.o error.o 16-22.o	\
	14-24.o 16-11.o util.c 14-5.o
	gcc -g printd.c 13-1.o error.o errorlog.o 16-22.o 14-24.o 	\
		16-11.o util.c 14-5.o -o printd -lpthread
clean:
	rm -f *.o

