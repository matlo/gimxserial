/*
 Copyright (c) 2015 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#include <gserial.h>
#include <gimxcommon/include/gerror.h>
#include <gimxcommon/include/async.h>
#include <gimxlog/include/glog.h>

#include <stdio.h>
#include <linux/spi/spidev.h>
#include <sys/select.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

GLOG_INST(GLOG_NAME)

static int tty_set_params(struct gserial_device * device, speed_t baudrate)
{
  int fd = async_get_fd((struct async_device *) device);
  if (fd < 0) {
      return -1;
  }

  struct termios options;

  if(tcgetattr(fd, &options) < 0)
  {
    PRINT_ERROR_ERRNO("tcgetattr");
    return -1;
  }
  cfsetispeed(&options, baudrate);
  cfsetospeed(&options, baudrate);
  cfmakeraw(&options);
  if(tcsetattr(fd, TCSANOW, &options) < 0)
  {
    PRINT_ERROR_ERRNO("tcsetattr");
    return -1;
  }
  tcflush(fd, TCIFLUSH);

  return 0;
}

static int spi_set_params(struct gserial_device * device, unsigned int baudrate)
{
    int fd = async_get_fd((struct async_device *) device);
    if (fd < 0) {
        return -1;
    }

  unsigned char bits = 8;
  unsigned char mode = 0;

  if(ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &baudrate) < 0)
  {
    PRINT_ERROR_ERRNO("ioctl SPI_IOC_WR_MAX_SPEED_HZ");
    return -1;
  }
  else if(ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0)
  {
    PRINT_ERROR_ERRNO("ioctl SPI_IOC_WR_BITS_PER_WORD");
    return -1;
  }
  else if(ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0)
  {
    PRINT_ERROR_ERRNO("ioctl SPI_IOC_RD_BITS_PER_WORD");
    return -1;
  }
  else if (ioctl (fd, SPI_IOC_WR_MODE, &mode) < 0)
  {
    PRINT_ERROR_ERRNO("ioctl SPI_IOC_WR_MODE");
    return -1;
  }
  else if (ioctl (fd, SPI_IOC_RD_MODE, &mode) < 0)
  {
    PRINT_ERROR_ERRNO("ioctl SPI_IOC_RD_MODE");
    return -1;
  }

  return 0;
}

speed_t get_baudrate(unsigned int baudrate) {
  switch(baudrate) {
  case 50:
    return B50;
  case 75:
    return B75;
  case 110:
    return B110;
  case 134:
    return B134;
  case 150:
    return B150;
  case 200:
    return B200;
  case 300:
    return B300;
  case 600:
    return B600;
  case 1200:
    return B1200;
  case 1800:
    return B1800;
  case 2400:
    return B2400;
  case 4800:
    return B4800;
  case 9600:
    return B9600;
  case 19200:
    return B19200;
  case 38400:
    return B38400;
  case 57600:
    return B57600;
  case 115200:
    return B115200;
  case 230400:
    return B230400;
  case 460800:
    return B460800;
  case 500000:
    return B500000;
  case 576000:
    return B576000;
  case 921600:
    return B921600;
  case 1000000:
    return B1000000;
  case 1152000:
    return B1152000;
  case 1500000:
    return B1500000;
  case 2000000:
    return B2000000;
  case 2500000:
    return B2500000;
  case 3000000:
    return B3000000;
  case 3500000:
    return B3500000;
  case 4000000:
    return B4000000;
  default:
    return 0;
  }
}

struct gserial_device * gserial_open(const char * port, unsigned int baudrate) {

  struct gserial_device * device = (struct gserial_device *) async_open_path(port, 1);
  
  if(device == NULL) {
    return NULL;
  }

  int ret = 0;
  
  if(strstr(port, "tty"))
  {
    speed_t speed = get_baudrate(baudrate);

    if(speed) {
      ret = tty_set_params(device, speed);
    }
    else {
      if (GLOG_LEVEL(GLOG_NAME,ERROR)) {
        fprintf(stderr, "%s:%d %s: invalid baudrate (%u)\n", __FILE__, __LINE__, __func__, baudrate);
      }
      ret = -1;
    }
  }
  else if(strstr(port, "spi"))
  {
    ret = spi_set_params(device, baudrate);
  }
  
  if(ret < 0)
  {
    async_close((struct async_device *) device);
    return NULL;
  }

  return device;
}

int gserial_read_timeout(struct gserial_device * device, void * buf, unsigned int count, unsigned int timeout) {

  return async_read_timeout((struct async_device *) device, buf, count, timeout);
}

int gserial_set_read_size(struct gserial_device * device, unsigned int size) {

  return async_set_read_size((struct async_device *) device, size);
}

int gserial_register(struct gserial_device * device, void * user, const GSERIAL_CALLBACKS * callbacks) {

    ASYNC_CALLBACKS async_callbacks = {
            .fp_read = callbacks->fp_read,
            .fp_write = callbacks->fp_write,
            .fp_close = callbacks->fp_close,
            .fp_register = callbacks->fp_register,
            .fp_remove = callbacks->fp_remove,
    };

    return async_register((struct async_device *) device, user, &async_callbacks);
}

int gserial_write_timeout(struct gserial_device * device, void * buf, unsigned int count, unsigned int timeout) {

    return async_write_timeout((struct async_device *) device, buf, count, timeout);
}

int gserial_write(struct gserial_device * device, const void * buf, unsigned int count) {

    return async_write((struct async_device *) device, buf, count);
}

int gserial_close(struct gserial_device * device) {

    usleep(10000);//sleep 10ms to leave enough time for the last packet to be sent
    
    return async_close((struct async_device *) device);
}

