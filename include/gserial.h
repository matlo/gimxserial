/*
 Copyright (c) 2016 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#ifndef GSERIAL_H_

#define GSERIAL_H_

#include <gimxpoll/include/gpoll.h>

typedef int (* GSERIAL_READ_CALLBACK)(void * user, const void * buf, int status);
typedef int (* GSERIAL_WRITE_CALLBACK)(void * user, int status);
typedef int (* GSERIAL_CLOSE_CALLBACK)(void * user);
#ifndef WIN32
typedef GPOLL_REGISTER_FD GSERIAL_REGISTER_SOURCE;
typedef GPOLL_REMOVE_FD GSERIAL_REMOVE_SOURCE;
#else
typedef GPOLL_REGISTER_HANDLE GSERIAL_REGISTER_SOURCE;
typedef GPOLL_REMOVE_HANDLE GSERIAL_REMOVE_SOURCE;
#endif

typedef struct {
    GSERIAL_READ_CALLBACK fp_read;       // called on data reception
    GSERIAL_WRITE_CALLBACK fp_write;     // called on write completion
    GSERIAL_CLOSE_CALLBACK fp_close;     // called on failure
    GSERIAL_REGISTER_SOURCE fp_register; // to register the device to event sources
    GSERIAL_REMOVE_SOURCE fp_remove;     // to remove the device from event sources
} GSERIAL_CALLBACKS;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * \brief Structure representing a serial device.
 */
struct gserial_device;

/*
 * \brief Open a serial device. The serial device is registered for further operations.
 *
 * \param port     the serial device to open, e.g. /dev/ttyUSB0, /dev/ttyACM0, /dev/spidev1.1 on Linux, COM9 on Windows
 * \param baudrate the baudrate in bytes per second
 *
 * \return the identifier of the opened device (to be used in further operations), \
 * or -1 in case of failure (e.g. no device found).
 */
struct gserial_device * gserial_open(const char * portname, unsigned int baudrate);

/*
 * \brief This function closes a serial device.
 *
 * \param device  the serial device
 *
 * \return 0 in case of a success, -1 in case of an error
 */
int gserial_close(struct gserial_device * device);

/*
 * \brief Read from a serial device, with a timeout. Use this function in a synchronous context.
 *
 * \param device  the identifier of the serial device
 * \param buf     the buffer where to store the data
 * \param count   the maximum number of bytes to read
 * \param timeout the maximum time to wait, in milliseconds
 *
 * \return the number of bytes actually read
 */
int gserial_read_timeout(struct gserial_device * device, void * buf, unsigned int count, unsigned int timeout);

/*
 * \brief Set the amount of bytes for the next read operation.
 *
 * \param device  the identifier of the serial device
 * \param size   the maximum number of bytes to read
 *
 * \return 0 in case of success, or -1 in case of error
 */
int gserial_set_read_size(struct gserial_device * device, unsigned int size);

/*
 * \brief Register the device as an event source, and set the external callbacks.
 *        This function triggers an asynchronous context.
 *        The fp_read callback is responsible for setting the next read size.
 *
 * \param device      the serial device
 * \param user        the user to pass to the external callback
 * \param callbacks   the device callbacks
 *
 * \return 0 in case of success, or -1 in case of error
 */
int gserial_register(struct gserial_device * device, void * user, const GSERIAL_CALLBACKS * callbacks);

/*
 * \brief Write to a serial device, with a timeout. Use this function in a synchronous context.
 *        On Windows, in case of timeout, the function request the cancellation of the write operation,
 *        and _blocks_ until either the cancellation or the write operation succeeds.
 *        Therefore don't expect the timeout to be very precise.
 *
 * \param device  the identifier of the serial device
 * \param buf     the buffer containing the data to write
 * \param count   the number of bytes in buf
 * \param timeout the maximum time to wait for the completion, in seconds
 *
 * \return the number of bytes actually written (0 in case of timeout)
 */
int gserial_write_timeout(struct gserial_device * device, void * buf, unsigned int count, unsigned int timeout);

/*
 * \brief Send data to a serial device. Use this function in an asynchronous context.
 *
 * \param device  the identifier of the serial device
 * \param buf     the buffer containing the data to send
 * \param count   the maximum number of bytes to send
 *
 * \return -1 in case of error, 0 in case of pending write, or the number of bytes written
 */
int gserial_write(struct gserial_device * device, const void * buf, unsigned int count);

#ifdef __cplusplus
}
#endif

#endif /* GSERIAL_H_ */
