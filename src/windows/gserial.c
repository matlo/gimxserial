/*
 Copyright (c) 2016 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#include <gserial.h>
#include <gimxcommon/include/gerror.h>
#include <gimxcommon/include/async.h>
#include <gimxlog/include/glog.h>

#include <stdio.h>
#include <unistd.h>
#include <windows.h>

GLOG_INST(GLOG_NAME)

typedef struct {
    unsigned char restoreParams;
    DCB prevParams;
    unsigned char restoreTimeouts;
    COMMTIMEOUTS prevTimeouts;
} s_serial_params;

static int set_serial_params(struct gserial_device * device, unsigned int baudrate) {

    s_serial_params * params = (s_serial_params *) calloc(1, sizeof(*params));

    if (params == NULL) {
        PRINT_ERROR_ALLOC_FAILED("malloc");
        return -1;
    }

    HANDLE * handle = async_get_handle((struct async_device *) device);

    /*
     * disable timeouts
     */
    if (GetCommTimeouts(handle, &params->prevTimeouts) == 0) {
        PRINT_ERROR_GETLASTERROR("GetCommTimeouts");
        free(params);
        return -1;
    }
    params->restoreTimeouts = 1;
    COMMTIMEOUTS newTimeouts = { 0 };
    if (SetCommTimeouts(handle, &newTimeouts) == 0) {
        PRINT_ERROR_GETLASTERROR("SetCommTimeouts");
        free(params);
        return -1;
    }
    /*
     * set baudrate
     */
    params->prevParams.DCBlength = sizeof(params->prevParams);
    if (GetCommState(handle, &params->prevParams) == 0) {
        PRINT_ERROR_GETLASTERROR("GetCommState");
        free(params);
        return -1;
    }
    params->restoreParams = 1;
    DCB newSerialParams = params->prevParams;
    newSerialParams.BaudRate = baudrate;
    newSerialParams.ByteSize = 8;
    newSerialParams.StopBits = ONESTOPBIT;
    newSerialParams.Parity = NOPARITY;
    if (SetCommState(handle, &newSerialParams) == 0) {
        PRINT_ERROR_GETLASTERROR("SetCommState");
        free(params);
        return -1;
    }
    async_set_private((struct async_device *) device, params);
    return 0;
}

struct gserial_device * gserial_open(const char * port, unsigned int baudrate) {

    char scom[sizeof("\\\\.\\") + strlen(port)];
    snprintf(scom, sizeof(scom), "\\\\.\\%s", port);

    struct gserial_device * device = (struct gserial_device *) async_open_path(scom, 1);
    if (device == NULL) {
        return NULL;
    }

    async_set_device_type((struct async_device *) device, E_ASYNC_DEVICE_TYPE_SERIAL);

    if (set_serial_params(device, baudrate) < 0) {
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

    usleep(10000); //sleep 10ms to leave enough time for the last packet to be sent

    HANDLE * handle = async_get_handle((struct async_device *) device);
    s_serial_params * params = (s_serial_params *) async_get_private((struct async_device *) device);

    if (handle != NULL && params != NULL) {
        if (params->restoreParams && SetCommState(handle, &params->prevParams) == 0) {
            PRINT_ERROR_GETLASTERROR("SetCommState");
        }
        if (params->restoreTimeouts && SetCommTimeouts(handle, &params->prevTimeouts) == 0) {
            PRINT_ERROR_GETLASTERROR("SetCommTimeouts");
        }
    }
    
    free(params);

    return async_close((struct async_device *) device);
}
