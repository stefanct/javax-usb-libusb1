#ifndef USB_WRAPPER_H
#define USB_WRAPPER_H

#include <libusb.h>

void usbw_set_trace_calls(int on);

const char *usbw_error_to_string(enum libusb_error error);

/*************************************************************************
 * Library initialization/deinitialization
 */

void usbw_set_debug(struct libusb_context *context, int level);
int usbw_init(struct libusb_context **context);
void usbw_exit(struct libusb_context *context);

/*************************************************************************
 * Device handling and enumeration
 */

ssize_t usbw_get_device_list(struct libusb_context *context, libusb_device ***list);
void usbw_free_device_list(struct libusb_device **list, int unref_devices);
uint8_t usbw_get_bus_number(struct libusb_device *device);
uint8_t usbw_get_device_address(struct libusb_device *device);
enum libusb_speed usbw_get_speed(struct libusb_device *device);
void usbw_unref_device(struct libusb_device *device);
int usbw_open(struct libusb_device *device, struct libusb_device_handle **handle);
struct libusb_device_handle* usbw_open_device_with_vid_pid(struct libusb_context* context, uint16_t vendor_id, uint16_t product_id);
void usbw_close(struct libusb_device_handle *handle);
int usbw_get_configuration(struct libusb_device_handle *handle, int *config);
int usbw_set_configuration(struct libusb_device_handle *handle, int configuration);
int usbw_claim_interface(struct libusb_device_handle *device, int interface_number);
int usbw_reset_device(struct libusb_device_handle *device);

/*************************************************************************
 * USB descriptors
 */

int usbw_get_device_descriptor(struct libusb_device *device, struct libusb_device_descriptor *descriptor);
int usbw_get_active_config_descriptor(struct libusb_device *device, struct libusb_config_descriptor **configuration);
int usbw_get_config_descriptor(struct libusb_device *device, uint8_t config_index, struct libusb_config_descriptor **configuration);
void usbw_free_config_descriptor(struct libusb_config_descriptor *configuration);
int usbw_get_string_descriptor_ascii(struct libusb_device_handle *handle, uint8_t desc_index, unsigned char *data, int length);

/*************************************************************************
 * Synchronous device I/O
 */

int usbw_control_transfer(struct libusb_device_handle *handle, uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength, unsigned int timeout);
int usbw_bulk_transfer(struct libusb_device_handle *handle, unsigned char endpoint, unsigned char *data, int length, int *transferred, unsigned int timeout);
int usbw_interrupt_transfer(struct libusb_device_handle *handle, unsigned char endpoint, unsigned char *data, int length, int *transferred, unsigned int timeout);

/*************************************************************************
 * Asynchronous device I/O
 */

struct libusb_transfer *usbw_alloc_transfer(int iso_packets);
int usbw_submit_transfer(struct libusb_transfer *transfer);
int usbw_cancel_transfer(struct libusb_transfer *transfer);
void usbw_free_transfer(struct libusb_transfer *transfer);
int usbw_handle_events_timeout(libusb_context *ctx, struct timeval *tv);

#endif
