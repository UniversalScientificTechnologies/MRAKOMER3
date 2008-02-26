#include <termio.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

// how many measurements to average?
#define FIFOLEN 120
#define EVERY 20		// issue a printout every N seconds

int
main (int argc, char **argv)
{
  struct termios tel_termios;
  char rbuf[10], buf[256], schar, credo = 1;

  int i, number = 0;
  int status;
  int port;
  FILE *portf, *crfile;

  int fifopos = 0, fifolenx = 0;
  float fifo[FIFOLEN];
  float fifo2[FIFOLEN];
  float fifo3[FIFOLEN];

  for (i = 0; i < FIFOLEN; i++)
    {
      fifo[i] = 0.0;
      fifo2[i] = 0.0;
      fifo3[i] = 0.0;
    }

  port = open (argv[1], O_RDWR);

  if (port < 0)
    {
      fprintf (stderr, "mrakomer: cant access the device\n");
      return -1;
    }

  if (tcgetattr (port, &tel_termios) < 0)
    {
      fprintf (stderr, "mrakomer: device not a port?\n");
      return -1;
    }

  if (cfsetospeed (&tel_termios, B2400) < 0 ||
      cfsetispeed (&tel_termios, B2400) < 0)
    {
      fprintf (stderr, "mrakomer: speed setting problem?\n");
      return -1;
    }

  tel_termios.c_iflag = IGNBRK & ~(IXON | IXOFF | IXANY);
  tel_termios.c_oflag = 0;
  tel_termios.c_cflag =
    ((tel_termios.c_cflag & ~(CSIZE)) | CS8) & ~(PARENB | PARODD);
  tel_termios.c_lflag = 0;
  tel_termios.c_cc[VMIN] = 0;
  tel_termios.c_cc[VTIME] = 5;

  if (tcsetattr (port, TCSANOW, &tel_termios) < 0)
    {
      fprintf (stderr, "mrakomer: port init failed at tcsetattr\n");
      return -1;
    }

// get current state of control signals 
  ioctl (port, TIOCMGET, &status);

// Drop DTR  
  status &= ~TIOCM_DTR;
  ioctl (port, TIOCMSET, &status);

//  fprintf(stderr, "mrakomer: Port init complete\n");

  portf = fdopen (port, "w+");

  schar = 'x';
  for (;;)
    {
      int tstat, tno;
      char *x;
      float temp0, temp1, avg, avg2, avg3;
      struct timeval tv;

// send the measurement request and mark up the time when it was sent.
// x=no heating, h=heating
      write (port, &schar, 1);
      gettimeofday (&tv, NULL);

// Wait a while for the reply and process it.
      usleep (480000);
      x = fgets (buf, 256, portf);
      sscanf (buf, "%d %f %f %d", &tno, &temp0, &temp1, &tstat);

// thermostat ;)
      if (temp0 < 5.0)
	schar = 'h';
      if (temp0 > 5.1)
	schar = 'x';

// decide if we agree to open. Only valid in the night, but that's not a problem anyway.
      fifo[fifopos] = temp0 - temp1;
      fifo2[fifopos] = temp0;
      fifo3[fifopos] = temp1;

      if (++fifopos > FIFOLEN)
	fifopos = 0;
      if (++fifolenx > FIFOLEN)
	fifolenx = FIFOLEN;

      for (avg2 = avg3 = avg = 0, i = 0; i < fifolenx; i++)
	{
	  avg += fifo[i];
	  avg2 += fifo2[i];
	  avg3 += fifo3[i];
	}
      avg /= fifolenx;
      avg2 /= fifolenx;
      avg3 /= fifolenx;
// decide whether to open or not ;)
      if (avg < 6.25)
	credo = 1;		// blocked
      if (avg > 6.75)
	credo = 0;		// unblocked


      crfile = fopen ("/home/standa/dcm/mrakomer.credo", "w+");
      if (crfile)
	fprintf (crfile, "%d\n", (int) credo);
      if (crfile)
	fclose (crfile);

// send to the stdout the result.
      if (!(++number % EVERY))
	{
	  fprintf (stdout, "%.2f %d %.2f %.2f %d %c %.2f %d\n",
		   (double) tv.tv_sec + (double) tv.tv_usec / 1000000, tno,
		   avg2, avg3, tstat, schar, avg, (int) credo);

	  fflush (stdout);
	}
//      sleep (DELAY-1);
    }
}
