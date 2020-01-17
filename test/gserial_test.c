/*
 Copyright (c) 2016 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <getopt.h>

#include <gimxserial/include/gserial.h>
#include <gimxinput/include/ginput.h>
#include <gimxpoll/include/gpoll.h>
#include <gimxtimer/include/gtimer.h>
#include <gimxtime/include/gtime.h>
#include <gimxprio/include/gprio.h>
#include <gimxlog/include/glog.h>

#include <gimxcommon/test/common.h>
#include <gimxcommon/test/handlers.c>
#include <gimxcommon/test/timer.c>

#define PERIOD 10000//microseconds

static int debug = 0;
static int prio = 0;

static char * port = NULL;

static struct gserial_device * serial = NULL;

static unsigned char * packet;
static unsigned char * result;
static unsigned int read = 0;
static unsigned int count = 0;

static unsigned int baudrate = 0;
static unsigned int samples = 0;
static unsigned short packet_size = 0;

static unsigned int verbose = 0;

static unsigned int duration = 0;
static unsigned int allocated = 1024; // default allocation when duration is used

static gtime t0, t1;
static gtime * tRead = NULL;
static gtime wread = 0;

static char * port2 = NULL;
static struct gserial_device * serial2 = NULL;
static unsigned int read2 = 0;

void results(gtime * tdiff, unsigned int cpt) {
  gtime sum = 0;
  gtime average;
  gtime temp = 0;
  gtime nbval;
  gtime worst = 0;

  unsigned int i;
  for (i = 0; i < cpt && tdiff[i] > 0; i++) {
    sum = sum + tdiff[i];
    if (tdiff[i] > worst) {
      worst = tdiff[i];
    }
  }

  nbval = i;

  printf(""GTIME_FS"\t", GTIME_USEC(worst));

  if (nbval < 2) {
    return;
  }

  average = sum / nbval;

  printf(""GTIME_FS"\t", GTIME_USEC(average));

  for (i = 0; i < nbval; i++) {
    gtimediff diff = tdiff[i] - average;
    temp = temp + pow(llabs(diff), 2);
  }

  temp = pow(temp / (nbval - 1), 0.5);

  printf(""GTIME_FS"\t", GTIME_USEC(temp));
}

static void usage() {
  fprintf(stderr, "Usage: ./gserial_test [-p port] [-b baudrate] [-d duration] [-n samples] [-s packet size] -v -g\n");
  exit(EXIT_FAILURE);
}

/*
 * Reads command-line arguments.
 */
static int read_args(int argc, char* argv[]) {

  int opt;
  while ((opt = getopt(argc, argv, "b:d:ghn:p:s:v")) != -1) {
    switch (opt) {
    case 'b':
      baudrate = atoi(optarg);
      break;
    case 'd':
      duration = atoi(optarg) * 1000000UL / PERIOD;
      break;
    case 'g':
      debug = 1;
      break;
    case 'h':
      prio = 1;
      break;
    case 'n':
      samples = atoi(optarg);
      break;
    case 'p':
      if (port == NULL) {
        port = optarg;
      } else if (port2 == NULL) {
        port2 = optarg;
      }
      break;
    case 's':
      packet_size = atoi(optarg);
      break;
    case 'v':
      verbose = 1;
      break;
    default: /* '?' */
      usage();
      break;
    }
  }
  return 0;
}

int serial_read(void * user __attribute__((unused)), const void * buf, int status) {

  if (status < 0) {
    set_done();
    return 1;
  }

  memcpy(result + read, buf, status);
  read += status;

  if (read < packet_size) {
    gserial_set_read_size(serial, packet_size - read);
    return 0;
  }

  t1 = gtime_gettime();

  gserial_set_read_size(serial, packet_size);

  int ret = 0;

  if (memcmp(result, packet, packet_size)) {

    fprintf(stderr, "bad packet content\n");
    set_done();
    ret = -1;

  } else {

    unsigned int i;
    for (i = 0; i < packet_size; ++i) {
      packet[i]++;
    }

    if (samples == 0 && count == allocated) {
        void * ptr = realloc (tRead, 2 * allocated * sizeof(*tRead));
        if (ptr == NULL) {
            fprintf(stderr, "realloc failed\n");
            set_done();
            ret = -1;
        } else {
            allocated *= 2;
            tRead = ptr;
        }
    }

    if (samples != 0 || count < allocated) {

        tRead[count] = t1 - t0;

        if (tRead[count] > wread) {
          wread = tRead[count];
        }

        ++count;
        if (count == samples) {

          set_done();

        } else {

          t0 = gtime_gettime();

          int status = gserial_write(serial, packet, packet_size);
          if (status < 0) {
            set_done();
          }
        }

        read = 0;
    }
  }

  if (is_done()) {
    ret = -1;
  }

  return ret;
}

int serial_close(void * user __attribute__((unused))) {
  set_done();
  return 1;
}

int serial_read2(void * user __attribute__((unused)), const void * buf, int status) {

  if (status < 0) {
    set_done();
    return 1;
  }

  read2 += status;

  if (read2 < packet_size) {
    gserial_set_read_size(serial2, packet_size - read2);
  } else {
    gserial_set_read_size(serial2, packet_size);
  }

  if (gserial_write(serial2, buf, status) < 0) {
    set_done();
  }

  if (is_done()) {
    return -1;
  }

  return 0;
}

int serial_close2(void * user __attribute__((unused))) {
  set_done();
  return 1;
}

int main(int argc, char* argv[]) {

  setup_handlers();

  read_args(argc, argv);

  if (debug) {
      glog_set_all_levels(E_GLOG_LEVEL_DEBUG);
  }

  if(port == NULL || baudrate == 0 || (samples == 0 && duration == 0) || packet_size == 0)
  {
    usage();
    return -1;
  }

  packet = calloc(packet_size, sizeof(*packet));
  result = calloc(packet_size, sizeof(*result));

  tRead = calloc(samples ? samples : allocated, sizeof(*tRead));

  if(packet == NULL || result == NULL || tRead == NULL) {
    fprintf(stderr, "can't allocate memory to store samples\n");
    return -1;
  }

  // start a timer to periodically check the 'done' variable
  GTIMER_CALLBACKS timer_callbacks = {
          .fp_read = timer_read,
          .fp_close = timer_close,
          .fp_register = REGISTER_FUNCTION,
          .fp_remove = REMOVE_FUNCTION,
  };
  struct gtimer * timer = gtimer_start(NULL, PERIOD, &timer_callbacks);
  if (timer == NULL) {
    set_done();
  }

  serial = gserial_open(port, baudrate);
  if (serial == NULL) {
    return -1;
  }

  gserial_set_read_size(serial, packet_size);

  GSERIAL_CALLBACKS serial_callbacks = {
          .fp_read = serial_read,
          .fp_write = NULL,
          .fp_close = serial_close,
          .fp_register = REGISTER_FUNCTION,
          .fp_remove = REMOVE_FUNCTION,
  };
  gserial_register(serial, NULL, &serial_callbacks);

  if (port2 != NULL) {

    serial2 = gserial_open(port2, baudrate);
    if (serial2 == NULL) {
      set_done();
    } else {
      gserial_set_read_size(serial2, packet_size);

      GSERIAL_CALLBACKS serial_callbacks2 = {
            .fp_read = serial_read2,
            .fp_write = NULL,
            .fp_close = serial_close2,
            .fp_register = REGISTER_FUNCTION,
            .fp_remove = REMOVE_FUNCTION,
      };
      gserial_register(serial2, NULL, &serial_callbacks2);
    }
  }

  t0 = gtime_gettime();

  int ret = gserial_write(serial, packet, packet_size);
  if (ret < 0) {
    set_done();
  }

  if (prio && gprio_init() < 0) {
    set_done();
  }

  unsigned int period_count = 0;

  while (!is_done()) {
    gpoll();
    ++period_count;
    if (duration > 0 && period_count >= duration) {
      set_done();
    }
  }

  if (prio) {
    gprio_clean();
  }

  if (timer != NULL) {
    gtimer_close(timer);
  }

  gserial_close(serial);

  if (serial2 != NULL) {
      gserial_close(serial2);
  }

  if(verbose) {
    printf("baudrate: %d ", baudrate);
    printf("samples: %d ", count);
    printf("packet size: %d\n", packet_size);
    printf("worst\tavg\tstdev\n");
  }
  results(tRead, count);
  printf("\n");

  free(packet);
  free(result);
  free(tRead);

  return 0;
}
