#include "javalibusb1_libusb1.h"
#include "usbw.h"
#include <stdlib.h>
#include <stdarg.h>
#include "javalibusb.h"

// TODO: make them static?

/* javalibusb1.Libusb1UsbDevice */
jclass libusb1UsbDeviceClass = NULL;
jmethodID libusb1UsbDeviceConstructor = NULL;
jmethodID libusb1UsbDeviceSetConfiguration = NULL;
jmethodID libusb1UsbDeviceSetActiveConfiguration = NULL;

/* javalibusb1.Libusb1UsbConfiguration */
jclass libusb1UsbConfigurationClass = NULL;
jmethodID libusb1UsbConfigurationConstructor = NULL;

/* javalibusb1.Libusb1UsbInterface */
jclass libusb1UsbInterfaceClass = NULL;
jmethodID libusb1UsbInterfaceConstructor = NULL;

/* javalibusb1.Libusb1UsbEndpoint */
jclass libusb1UsbEndpointClass = NULL;
jmethodID libusb1UsbEndpointConstructor = NULL;

/* javalibusb1.Libusb1UsbPipe */
jclass libusb1UsbPipeClass = NULL;
jmethodID libusb1UsbPipeAsyncCallback = NULL;

/* javax.usb.UsbIrp */
jclass usbIrpClass = NULL;
jmethodID usbIrpSetActualLength = NULL;
jmethodID usbIrpSetUsbException = NULL;
//jmethodID usbIrpComplete = NULL;

/* javax.usb.UsbDeviceDescriptor */
jclass usbDeviceDescriptorClass = NULL;

/* javax.usb.UsbConfiguration */
jclass usbConfigurationClass = NULL;

/* javax.usb.UsbInterface */
jclass usbInterfaceClass = NULL;
jclass usbInterfaceArrayClass = NULL;

/* javax.usb.UsbInterfaceDescriptor */
jclass usbInterfaceDescriptorClass = NULL;

/* javax.usb.UsbEndpoint */
jclass usbEndpointClass = NULL;

/* javax.usb.UsbEndpointDescriptor */
jclass usbEndpointDescriptorClass = NULL;

/* javax.usb.UsbPlatformException */
jclass usbPlatformExceptionClass = NULL;
jmethodID usbPlatformExceptionConstructorMsgCode = NULL;

/* javax.usb.UsbStallException */
jclass usbStallExceptionClass = NULL;
jmethodID usbStallExceptionConstructorMsg = NULL;

/* javax.usb.UsbAbortException */
jclass usbAbortExceptionClass = NULL;
jmethodID usbAbortExceptionConstructorMsg = NULL;

/* javax.usb.UsbDisconnectedException */
jclass usbDisconnectedExceptionClass = NULL;
jmethodID usbDisconnectedExceptionConstructorMsg = NULL;

/* javax.usb.impl.DefaultUsbDeviceDescriptor */
jclass defaultUsbDeviceDescriptorClass = NULL;
jmethodID defaultUsbDeviceDescriptorConstructor = NULL;

/* javax.usb.impl.DefaultUsbConfigurationDescriptor */
jclass defaultUsbConfigurationDescriptorClass = NULL;
jmethodID defaultUsbConfigurationDescriptorConstructor = NULL;

/* javax.usb.impl.DefaultUsbInterfaceDescriptor */
jclass defaultUsbInterfaceDescriptorClass = NULL;
jmethodID defaultUsbInterfaceDescriptorConstructor = NULL;

/* javax.usb.impl.DefaultUsbEndpointDescriptor */
jclass defaultUsbEndpointDescriptorClass = NULL;
jmethodID defaultUsbEndpointDescriptorConstructor = NULL;

/*
TODO: Make a construct() method that looks up and add a reference to the class objects and a
destroy() method that releases them again. These should be called by the constructor so that the JVM
will basically all of the reference counting.

usb_init() should be called as a part of the static initializer.
*/

static void releaseReferences(JNIEnv *env);
static JavaVM *jvm; // needed to get a JNIEnv pointer in asyncCallback
static int debug = 0;

static void debug_printf(const char *format, ...) {
    char p[1024];

    if(!debug) {
        return;
    }

    va_list ap;
    va_start(ap, format);
    (void)vsnprintf(p, sizeof(p), format, ap);
    va_end(ap);

    /* Would a right-adjusted name look more readable? */
    fprintf(stderr, "javalibusb1: DEBUG %s", p);
    fflush(stderr);
}

static jclass findAndReferenceClass(JNIEnv *env, const char* name) {
    debug_printf("Loading class %s\n", name);
    jclass klass = (*env)->FindClass(env, name);

    if(klass == NULL) {
        (*env)->ExceptionClear(env);
        debug_printf("Error finding class %s\n", name);
        (*env)->FatalError(env, name);
        return NULL;
    }

    klass = (jclass) (*env)->NewGlobalRef(env, klass);

    if(klass == NULL) {
        (*env)->ExceptionClear(env);
        debug_printf("Error adding reference to class %s\n", name);
        (*env)->FatalError(env, name);
        return NULL;
    }

    return klass;
}

static void unreferenceClass(JNIEnv *env, jclass* klass) {
    if(klass == NULL) {
        return;
    }

    (*env)->DeleteGlobalRef(env, *klass);
    *klass = NULL;
}

static void throwPlatformException(JNIEnv *env, const char *message)
{
    (*env)->ThrowNew(env, usbPlatformExceptionClass, message);
}

static void throwUsbExceptionMsgCode(JNIEnv *env, int errorCode, const char *format, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    (void)vsnprintf(buf, 1024, format, ap);
    va_end(ap);

    jstring s = (*env)->NewStringUTF(env, buf);

    jobject e;
    switch(errorCode){
        case LIBUSB_TRANSFER_NO_DEVICE:
            e = (*env)->NewObject(env, usbDisconnectedExceptionClass, usbDisconnectedExceptionConstructorMsg, s);
            break;
        case LIBUSB_TRANSFER_CANCELLED:
            e = (*env)->NewObject(env, usbAbortExceptionClass, usbAbortExceptionConstructorMsg, s);
            break;
        case LIBUSB_TRANSFER_STALL:
            e = (*env)->NewObject(env, usbStallExceptionClass, usbStallExceptionConstructorMsg, s);
            break;
        case LIBUSB_TRANSFER_ERROR:
        case LIBUSB_TRANSFER_TIMED_OUT:
        case LIBUSB_TRANSFER_OVERFLOW:
        default:
            e = (*env)->NewObject(env, usbPlatformExceptionClass, usbPlatformExceptionConstructorMsgCode, s, (jint)errorCode);
            break;
    }

    if(e == NULL) {
        return; // failed to allocate an exception
    }

    (*env)->Throw(env, e);
}

static jobject config_descriptor2java(JNIEnv *env, const struct libusb_config_descriptor* config_descriptor) {
    return (*env)->NewObject(env, defaultUsbConfigurationDescriptorClass, defaultUsbConfigurationDescriptorConstructor,
        config_descriptor->bConfigurationValue,
        config_descriptor->bmAttributes,
        config_descriptor->MaxPower,
        config_descriptor->bNumInterfaces,
        config_descriptor->iConfiguration,
        config_descriptor->wTotalLength);
}

static jobject interface_descriptor2java(JNIEnv *env, const struct libusb_interface_descriptor *interface_descriptor) {
    return (*env)->NewObject(env, defaultUsbInterfaceDescriptorClass, defaultUsbInterfaceDescriptorConstructor,
        interface_descriptor->bAlternateSetting,
        interface_descriptor->bInterfaceClass,
        interface_descriptor->bInterfaceNumber,
        interface_descriptor->bInterfaceProtocol,
        interface_descriptor->bInterfaceSubClass,
        interface_descriptor->bNumEndpoints,
        interface_descriptor->iInterface);
}

static jobject endpoint_descriptor2java(JNIEnv *env, const struct libusb_endpoint_descriptor *endpoint_descriptor) {
    return (*env)->NewObject(env, defaultUsbEndpointDescriptorClass, defaultUsbEndpointDescriptorConstructor,
        endpoint_descriptor->bEndpointAddress,
        endpoint_descriptor->bInterval,
        endpoint_descriptor->bmAttributes,
        endpoint_descriptor->wMaxPacketSize);
}

static jobject config_descriptor2usbConfiguration(JNIEnv *env, jobject usbDevice, const struct libusb_config_descriptor* config_descriptor, jboolean known_active, int config_value) {
    const struct libusb_interface *interface = NULL;
    const struct libusb_interface_descriptor *interface_descriptor;
    const struct libusb_endpoint_descriptor *endpoint_descriptor;
    jobject usbConfiguration;
    jobject usbConfigurationDescriptor;
    jobjectArray interfacesArrayArray, interfacesArray;
    jobject usbInterface;
    jboolean interface_active;
    jobject usbInterfaceDescriptor;
    jobject usbEndpoint;
    jobject usbEndpointDescriptor;
    jobjectArray endpoints;
    int i, j, k;

    if((usbConfigurationDescriptor = config_descriptor2java(env, config_descriptor)) == NULL) {
        return NULL;
    }

    if((interfacesArrayArray = (*env)->NewObjectArray(env, config_descriptor->bNumInterfaces, usbInterfaceArrayClass, NULL)) == NULL) {
        return NULL;
    }

    // If the device is not known to be active, but the bConfigurationValue matches the currently
    // active configuration value, then it's active
    if(!known_active && config_value == config_descriptor->bConfigurationValue) {
        known_active = JNI_TRUE;
    }

    usbConfiguration = (*env)->NewObject(env, libusb1UsbConfigurationClass, libusb1UsbConfigurationConstructor,
        usbDevice, usbConfigurationDescriptor, interfacesArrayArray, known_active);
    if(usbConfiguration == NULL) {
        return NULL;
    }

    for(i = 0; i < config_descriptor->bNumInterfaces; i++) {
        interface = &config_descriptor->interface[i];

        if((interfacesArray = (*env)->NewObjectArray(env, interface->num_altsetting, usbInterfaceClass, NULL)) == NULL) {
            return NULL;
        }

        (*env)->SetObjectArrayElement(env, interfacesArrayArray, i, interfacesArray);
        if((*env)->ExceptionCheck(env)) {
            return NULL;
        }

        for(j = 0; j < interface->num_altsetting; j++) {
            interface_descriptor = &interface->altsetting[j];

            usbInterfaceDescriptor = interface_descriptor2java(env, interface_descriptor);
            if(usbInterfaceDescriptor == NULL) {
                return NULL;
            }

            if((endpoints = (*env)->NewObjectArray(env, interface_descriptor->bNumEndpoints, usbEndpointClass, NULL)) == NULL) {
                return NULL;
            }

            // I'm not sure if this is a correct assumption, but right now it just sets
            // the first interface to be the active one.
            interface_active = j == 0;

            usbInterface = (*env)->NewObject(env, libusb1UsbInterfaceClass, libusb1UsbInterfaceConstructor,
                usbConfiguration, usbInterfaceDescriptor, endpoints, interface_active);
            if(usbInterface == NULL) {
                return NULL;
            }

            for(k = 0; k < interface_descriptor->bNumEndpoints; k++) {
                endpoint_descriptor = &interface_descriptor->endpoint[k];
                usbEndpointDescriptor = endpoint_descriptor2java(env, endpoint_descriptor);
                if(usbEndpointDescriptor == NULL) {
                    return NULL;
                }

                usbEndpoint = (*env)->NewObject(env, libusb1UsbEndpointClass, libusb1UsbEndpointConstructor,
                    usbInterface, usbEndpointDescriptor);
                if(usbEndpoint == NULL) {
                    return NULL;
                }

                (*env)->SetObjectArrayElement(env, endpoints, k, usbEndpoint);
                if((*env)->ExceptionCheck(env)) {
                    return NULL;
                }
            }

            (*env)->SetObjectArrayElement(env, interfacesArray, j, usbInterface);
            if((*env)->ExceptionCheck(env)) {
                return NULL;
            }
        }
    }

    return usbConfiguration;
}

JNIEXPORT void JNICALL Java_javalibusb1_libusb1_set_1trace_1calls
  (JNIEnv *env, jclass klass, jboolean on)
{
    usbw_set_trace_calls(on);
}

/**
 * Initializes the library in three phases:
 *
 * I) Create a libusb context
 * II) Look up all the required Java classes, fields and methods
 * III) Create the libusb object and return it.
 *
 * TODO: To prevent possible bugs, consider passing all Class as a reference
 * to the classhas to be registered with the JVM and unregistered when closed.
 * An alternative is to put all references in a list (libusb has an
 * implementation) and just iterate that on close().
 */
JNIEXPORT jobject JNICALL Java_javalibusb1_libusb1_create
  (JNIEnv *env, jclass klass, jint debug_level)
{
    struct libusb_context *context;
    
    debug = debug_level; // debug is boolean

    /* Initalization, Phase I */
    if(usbw_init(&context)) {
        throwPlatformException(env, "Unable to initialize libusb.");
        goto fail;
    }

    usbw_set_debug(context, debug_level);

    /* Initalization, Phase II */
    // TODO: Create some macros to do the lookups
    /* Lookups */
    if((libusb1UsbDeviceClass = findAndReferenceClass(env, "javalibusb1/Libusb1UsbDevice")) == NULL) {
        goto fail;
    }
    if((libusb1UsbDeviceConstructor = (*env)->GetMethodID(env, libusb1UsbDeviceClass, "<init>", "(JBBILjavax/usb/UsbDeviceDescriptor;)V")) == NULL) {
        goto fail;
    }
    if((libusb1UsbDeviceSetConfiguration = (*env)->GetMethodID(env, libusb1UsbDeviceClass, "_setConfiguration", "(Ljavax/usb/UsbConfiguration;B)V")) == NULL) {
        goto fail;
    }
    if((libusb1UsbDeviceSetActiveConfiguration = (*env)->GetMethodID(env, libusb1UsbDeviceClass, "_setActiveConfiguration", "(B)V")) == NULL) {
        goto fail;
    }

    if((libusb1UsbConfigurationClass = findAndReferenceClass(env, "javalibusb1/Libusb1UsbConfiguration")) == NULL) {
        goto fail;
    }
    if((libusb1UsbConfigurationConstructor = (*env)->GetMethodID(env, libusb1UsbConfigurationClass, "<init>", "(Ljavalibusb1/Libusb1UsbDevice;Ljavax/usb/UsbConfigurationDescriptor;[[Ljavax/usb/UsbInterface;Z)V")) == NULL) {
        goto fail;
    }

    if((libusb1UsbInterfaceClass = findAndReferenceClass(env, "javalibusb1/Libusb1UsbInterface")) == NULL) {
        goto fail;
    }
    if((libusb1UsbInterfaceConstructor = (*env)->GetMethodID(env, libusb1UsbInterfaceClass, "<init>", "(Ljavalibusb1/Libusb1UsbConfiguration;Ljavax/usb/UsbInterfaceDescriptor;[Ljavax/usb/UsbEndpoint;Z)V")) == NULL) {
        goto fail;
    }

    if((libusb1UsbEndpointClass = findAndReferenceClass(env, "javalibusb1/Libusb1UsbEndpoint")) == NULL) {
        goto fail;
    }
    if((libusb1UsbEndpointConstructor = (*env)->GetMethodID(env, libusb1UsbEndpointClass, "<init>", "(Ljavalibusb1/Libusb1UsbInterface;Ljavax/usb/UsbEndpointDescriptor;)V")) == NULL) {
        goto fail;
    }

    if((libusb1UsbPipeClass = findAndReferenceClass(env, "javalibusb1/Libusb1UsbPipe")) == NULL) {
        goto fail;
    }
    if((libusb1UsbPipeAsyncCallback = (*env)->GetMethodID(env, libusb1UsbPipeClass, "asyncCallback", "(Ljavax/usb/UsbIrp;)V")) == NULL) {
        goto fail;
    }

    if((usbIrpClass = findAndReferenceClass(env, "javax/usb/UsbIrp")) == NULL) {
        goto fail;
    }
    if((usbIrpSetActualLength = (*env)->GetMethodID(env, usbIrpClass, "setActualLength", "(I)V")) == NULL) {
        goto fail;
    }
    if((usbIrpSetUsbException = (*env)->GetMethodID(env, usbIrpClass, "setUsbException", "(Ljavax/usb/UsbException;)V")) == NULL) {
        goto fail;
    }
    //if((usbIrpComplete = (*env)->GetMethodID(env, usbIrpClass, "complete", "()V")) == NULL) {
        //goto fail;
    //}
    //if((UsbIrpData = (*env)->GetFieldID(env, usbIrpClass, "data", "Ljava/lang/String;");) == NULL) {
        //goto fail;
    //}


    if((usbDeviceDescriptorClass = findAndReferenceClass(env, "javax/usb/UsbDeviceDescriptor")) == NULL) {
        goto fail;
    }

    if((usbConfigurationClass = findAndReferenceClass(env, "javax/usb/UsbConfiguration")) == NULL) {
        goto fail;
    }

    if((usbInterfaceClass = findAndReferenceClass(env, "javax/usb/UsbInterface")) == NULL) {
        goto fail;
    }
    if((usbInterfaceArrayClass = findAndReferenceClass(env, "[Ljavax/usb/UsbInterface;")) == NULL) {
        goto fail;
    }

    if((usbInterfaceDescriptorClass = findAndReferenceClass(env, "javax/usb/UsbInterfaceDescriptor")) == NULL) {
        goto fail;
    }

    if((usbEndpointClass = findAndReferenceClass(env, "javax/usb/UsbEndpoint")) == NULL) {
        goto fail;
    }

    if((usbEndpointDescriptorClass = findAndReferenceClass(env, "javax/usb/UsbEndpointDescriptor")) == NULL) {
        goto fail;
    }

    if((usbPlatformExceptionClass = findAndReferenceClass(env, "javax/usb/UsbPlatformException")) == NULL) {
        goto fail;
    }
    if((usbPlatformExceptionConstructorMsgCode = (*env)->GetMethodID(env, usbPlatformExceptionClass, "<init>", "(Ljava/lang/String;I)V")) == NULL) {
        goto fail;
    }

    if((usbDisconnectedExceptionClass = findAndReferenceClass(env, "javax/usb/UsbDisconnectedException")) == NULL) {
        goto fail;
    }
    if((usbDisconnectedExceptionConstructorMsg = (*env)->GetMethodID(env, usbDisconnectedExceptionClass, "<init>", "(Ljava/lang/String;)V")) == NULL) {
        goto fail;
    }

    if((usbAbortExceptionClass = findAndReferenceClass(env, "javax/usb/UsbAbortException")) == NULL) {
        goto fail;
    }
    if((usbAbortExceptionConstructorMsg = (*env)->GetMethodID(env, usbAbortExceptionClass, "<init>", "(Ljava/lang/String;)V")) == NULL) {
        goto fail;
    }

    if((usbStallExceptionClass = findAndReferenceClass(env, "javax/usb/UsbStallException")) == NULL) {
        goto fail;
    }
    if((usbStallExceptionConstructorMsg = (*env)->GetMethodID(env, usbStallExceptionClass, "<init>", "(Ljava/lang/String;)V")) == NULL) {
        goto fail;
    }

    if((defaultUsbDeviceDescriptorClass = findAndReferenceClass(env, "javax/usb/impl/DefaultUsbDeviceDescriptor")) == NULL) {
        goto fail;
    }
    if((defaultUsbDeviceDescriptorConstructor = (*env)->GetMethodID(env, defaultUsbDeviceDescriptorClass, "<init>", "(SBBBBSSSBBBB)V")) == NULL) {
        goto fail;
    }

    if((defaultUsbConfigurationDescriptorClass = findAndReferenceClass(env, "javax/usb/impl/DefaultUsbConfigurationDescriptor")) == NULL) {
        goto fail;
    }
    if((defaultUsbConfigurationDescriptorConstructor = (*env)->GetMethodID(env, defaultUsbConfigurationDescriptorClass, "<init>", "(BBBBBS)V")) == NULL) {
        goto fail;
    }

    if((defaultUsbInterfaceDescriptorClass = findAndReferenceClass(env, "javax/usb/impl/DefaultUsbInterfaceDescriptor")) == NULL) {
        goto fail;
    }
    if((defaultUsbInterfaceDescriptorConstructor = (*env)->GetMethodID(env, defaultUsbInterfaceDescriptorClass, "<init>", "(BBBBBBB)V")) == NULL) {
        goto fail;
    }

    if((defaultUsbEndpointDescriptorClass = findAndReferenceClass(env, "javax/usb/impl/DefaultUsbEndpointDescriptor")) == NULL) {
        goto fail;
    }
    if((defaultUsbEndpointDescriptorConstructor = (*env)->GetMethodID(env, defaultUsbEndpointDescriptorClass, "<init>", "(BBBS)V")) == NULL) {
        goto fail;
    }

    /* Initialization, phase III */
    jmethodID c = (*env)->GetMethodID(env, klass, "<init>", "(J)V");

    if((*env)->ExceptionCheck(env)) {
        goto fail;
    }
    
    if((*env)->GetJavaVM(env, &jvm)) {
        goto fail;
    }

    return (*env)->NewObject(env, klass, c, context);

fail:
    releaseReferences(env);
    return NULL;
}

JNIEXPORT void JNICALL Java_javalibusb1_libusb1_close
    (JNIEnv *env, jobject obj, jlong java_context)
{
    struct libusb_context *context = (struct libusb_context *)(POINTER_STORAGE_TYPE)java_context;

    releaseReferences(env);

    usbw_exit(context);
}

static void releaseReferences(JNIEnv *env) {
    // In the opposite order of referencing
    unreferenceClass(env, &defaultUsbEndpointDescriptorClass);
    unreferenceClass(env, &defaultUsbInterfaceDescriptorClass);
    unreferenceClass(env, &defaultUsbConfigurationDescriptorClass);
    unreferenceClass(env, &defaultUsbDeviceDescriptorClass);
    unreferenceClass(env, &usbStallExceptionClass);
    unreferenceClass(env, &usbAbortExceptionClass);
    unreferenceClass(env, &usbDisconnectedExceptionClass);
    unreferenceClass(env, &usbPlatformExceptionClass);
    unreferenceClass(env, &usbEndpointDescriptorClass);
    unreferenceClass(env, &usbEndpointClass);
    unreferenceClass(env, &usbInterfaceDescriptorClass);
    unreferenceClass(env, &usbInterfaceArrayClass);
    unreferenceClass(env, &usbInterfaceClass);
    unreferenceClass(env, &usbConfigurationClass);
    unreferenceClass(env, &usbDeviceDescriptorClass);
    unreferenceClass(env, &libusb1UsbPipeClass);
    unreferenceClass(env, &libusb1UsbEndpointClass);
    unreferenceClass(env, &libusb1UsbInterfaceClass);
    unreferenceClass(env, &libusb1UsbConfigurationClass);
    unreferenceClass(env, &libusb1UsbDeviceClass);
}

JNIEXPORT void JNICALL Java_javalibusb1_libusb1_set_1debug
    (JNIEnv *env, jobject obj, jlong java_context, jint level)
{
    struct libusb_context *context = (struct libusb_context *)(POINTER_STORAGE_TYPE)java_context;

    usbw_set_debug(context, level);
}

int load_configurations(JNIEnv *env, struct libusb_device *device, uint8_t bNumConfigurations, jobject usbDevice, struct libusb_device_descriptor descriptor) {
    int err;
    struct libusb_device_handle* handle;

    err = usbw_open(device, &handle);

    // On Darwin, the device has to be open while querying for the descriptor
    if(err) {
        // On Linux, you will get on some devices. Just ignore those devices.
        if(err == LIBUSB_ERROR_ACCESS) {
            return 1;
        }

        throwUsbExceptionMsgCode(env, err, "libusb_open(): %s", usbw_error_to_string(err));
        return 1;
    }

    int config;
    if((err = usbw_get_configuration(handle, &config))) {
        // This happens on OSX with Apple's IR Receiver which almost always is suspended
        // debug_printf("**** get_configuration: could not get descriptor with index %d of %d in total. Skipping device %04x:%04x, err=%s\n", index, bNumConfigurations, descriptor.idVendor, descriptor.idProduct, usbw_error_to_string(err));
        // throwUsbExceptionMsgCode(env, err, "libusb_get_configuration(): %s", usbw_error_to_string(err));
        usbw_close(handle);
        return 1;
    }

    (*env)->CallVoidMethod(env, usbDevice, libusb1UsbDeviceSetActiveConfiguration, config);
    if((*env)->ExceptionCheck(env)) {
        return 1;
    }

    struct libusb_config_descriptor *config_descriptor = NULL;
    int index;
    for(index = 0; index < bNumConfigurations; index++) {
        if((err = usbw_get_config_descriptor(device, index, &config_descriptor))) {
            throwUsbExceptionMsgCode(env, err, "libusb_get_config_descriptor(): %s", usbw_error_to_string(err));
            break;
        }

        jobject usbConfiguration = config_descriptor2usbConfiguration(env, usbDevice, config_descriptor, JNI_FALSE, config);
        usbw_free_config_descriptor(config_descriptor);
        if(usbConfiguration == NULL || (*env)->ExceptionCheck(env)) {
            break;
        }

        (*env)->CallVoidMethod(env, usbDevice, libusb1UsbDeviceSetConfiguration, usbConfiguration, index + 1);
        if((*env)->ExceptionCheck(env)) {
            break;
        }
    }

    usbw_close(handle);

    return 0;
}

JNIEXPORT jobjectArray JNICALL Java_javalibusb1_libusb1_get_1devices
    (JNIEnv *env, jobject obj, jlong libusb_context_ptr)
{
    struct libusb_context *context = (struct libusb_context *)(POINTER_STORAGE_TYPE)libusb_context_ptr;
    struct libusb_device **devices;
    struct libusb_device *device;
    struct libusb_device_descriptor descriptor;
    int i;
    ssize_t size;
    jobject usbDevice;
    uint8_t busNumber, deviceAddress;
    jint speed;
    jobject usbDeviceDescriptor;
    int failed = 0;

    size = usbw_get_device_list(context, &devices);
    if(size < 0) {
        throwUsbExceptionMsgCode(env, size, "libusb_get_device_list(): %s", usbw_error_to_string(size));
        return NULL;
    }

    jobjectArray usbDevices = (*env)->NewObjectArray(env, size, libusb1UsbDeviceClass, NULL);
    if(usbDevices == NULL) {
        return NULL;
    }
    for(i = 0; i < size; i++) {
        device = devices[i];

        busNumber = usbw_get_bus_number(device);
        deviceAddress = usbw_get_device_address(device);

        if(usbw_get_device_descriptor(device, &descriptor)) {
            throwPlatformException(env, "libusb_get_device_descriptor()");
            failed = 1;
            break;
        }

        usbDeviceDescriptor = (*env)->NewObject(env, defaultUsbDeviceDescriptorClass, defaultUsbDeviceDescriptorConstructor,
            (jshort) descriptor.bcdUSB,
            (jbyte) descriptor.bDeviceClass,
            (jbyte) descriptor.bDeviceSubClass,
            (jbyte) descriptor.bDeviceProtocol,
            (jbyte) descriptor.bMaxPacketSize0,
            (jshort) descriptor.idVendor,
            (jshort) descriptor.idProduct,
            (jshort) descriptor.bcdDevice,
            (jbyte) descriptor.iManufacturer,
            (jbyte) descriptor.iProduct,
            (jbyte) descriptor.iSerialNumber,
            (jbyte) descriptor.bNumConfigurations);
        if(usbDeviceDescriptor == NULL) {
            return NULL;
        }

        switch (usbw_get_speed(device)) {
            case LIBUSB_SPEED_LOW:
                speed = 1;
                break;
            case LIBUSB_SPEED_FULL:
                speed = 2;
                break;
            case LIBUSB_SPEED_HIGH:
                speed = 3;
                break;
            default:
                speed = 0;
        }

        jlong device_ptr = (jlong) (POINTER_STORAGE_TYPE) device;
        usbDevice = (*env)->NewObject(env, libusb1UsbDeviceClass,
            libusb1UsbDeviceConstructor,
            device_ptr,
            busNumber,
            deviceAddress,
            speed,
            usbDeviceDescriptor);
        if(usbDevice == NULL || (*env)->ExceptionCheck(env)) {
            return NULL;
        }

        int failed = load_configurations(env, device, descriptor.bNumConfigurations, usbDevice, descriptor);

        // Ignore devices that failed and did *not* throw an exception
        if((*env)->ExceptionCheck(env)) {
            failed = 1;
            break;
        }

        if(failed) {
            continue;
        }

        (*env)->SetObjectArrayElement(env, usbDevices, i, usbDevice);
        if((*env)->ExceptionCheck(env)) {
            failed = 1;
            break;
        }
    }

    if(failed) {
        usbw_free_device_list(devices, 1);
        usbDevices = NULL;
    }
    else {
        usbw_free_device_list(devices, 0);
    }

    return usbDevices;
}

JNIEXPORT jint JNICALL Java_javalibusb1_libusb1_control_1transfer
  (JNIEnv *env, jclass klass, jlong libusb_device_ptr, jbyte bmRequestType, jbyte bRequest, jshort wValue, jshort wIndex, jlong timeout, jbyteArray bytes, jint offset, jshort length)
{
    int err = -1;
    struct libusb_device* device;
    struct libusb_device_handle *handle = NULL;
    uint8_t* data = NULL;
    uint16_t wLength;

    device = (struct libusb_device*)(POINTER_STORAGE_TYPE)libusb_device_ptr;

    data = malloc(length);
    if(data == NULL) {
        throwUsbExceptionMsgCode(env, err, "Unable to allocate memory buffer");
        goto fail;
    }

    // If this is an OUT transfer, copy the bytes to data
    if(!(bmRequestType & LIBUSB_ENDPOINT_DIR_MASK)) {
        (*env)->GetByteArrayRegion(env, bytes, offset, length, (jbyte*)data);
        if((*env)->ExceptionCheck(env)) {
            goto fail;
        }
    }

    wLength = length;

    if((err = usbw_open(device, &handle))) {
        throwUsbExceptionMsgCode(env, err, "libusb_open(): %s", usbw_error_to_string(err));
        goto fail;
    }

    if((err = usbw_control_transfer(handle, bmRequestType, bRequest, wValue, wIndex, data, wLength, timeout)) < 0) {
        throwUsbExceptionMsgCode(env, err, "libusb_control_transfer(): %s", usbw_error_to_string(err));
        goto fail;
    }

    // If this is an IN transfer, copy the data to bytes
    if(bmRequestType & LIBUSB_ENDPOINT_DIR_MASK) {
        (*env)->SetByteArrayRegion(env, bytes, offset, length, (jbyte*)data);
        if((*env)->ExceptionCheck(env)) {
            goto fail;
        }
    }

fail:
    if(data) {
        free(data);
    }
    if(handle) {
        usbw_close(handle);
    }

    return err;
}

//enum transferType {
    //transferTypeBulk,
    //transferTypeInterrupt
//};

//static jint bulk_or_interrupt_transfer(enum transferType transferType, JNIEnv *env, jclass klass, jlong libusb_device_handle_ptr, jbyte bEndpointAddress, jbyteArray bytes, jint offset, jint length, jlong timeout)
//{
    //int err = -1;
    //struct libusb_device_handle *handle = (struct libusb_device_handle *)(POINTER_STORAGE_TYPE)libusb_device_handle_ptr;
    //uint8_t* data = NULL;
    //int transferred;

    //data = malloc(length);
    //if(data == NULL) {
        //throwUsbExceptionMsgCode(env, err, "Unable to allocate memory.");
        //goto fail;
    //}

     ////If this is an OUT transfer, copy the bytes to data
    //if(!(bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)) {
        //(*env)->GetByteArrayRegion(env, bytes, offset, length, (jbyte*)data);
        //if((*env)->ExceptionCheck(env)) {
            //goto fail;
        //}
    //}

    //switch(transferType) {
        //case transferTypeBulk:
            //if((err = usbw_bulk_transfer(handle, bEndpointAddress, (unsigned char *)data + offset, length, &transferred, timeout))) {
                //throwUsbExceptionMsgCode(env, err, "libusb_bulk_transfer(): %s", usbw_error_to_string(err));
                //goto fail;
            //}
            //break;
        //case transferTypeInterrupt:
            //if((err = usbw_interrupt_transfer(handle, bEndpointAddress, (unsigned char *)data + offset, length, &transferred, timeout))) {
                //throwUsbExceptionMsgCode(env, err, "libusb_interrupt_transfer(): %s", usbw_error_to_string(err));
                //goto fail;
            //}
            //break;
    //}

     ////If this is an IN transfer, copy the data to bytes
    //if(bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) {
      //(*env)->SetByteArrayRegion(env, bytes, offset, length, (jbyte*)data);
      //if((*env)->ExceptionCheck(env)) {
          //goto fail;
      //}
    //}

//fail:
    //if(data) {
        //free(data);
    //}

    //return transferred;
//}

//JNIEXPORT jint JNICALL Java_javalibusb1_libusb1_bulk_1transfer
  //(JNIEnv *env, jclass klass, jlong libusb_device_handle_ptr, jbyte bEndpointAddress, jbyteArray bytes, jint offset, jint length, jlong timeout)
//{
    //return bulk_or_interrupt_transfer(transferTypeBulk, env, klass, libusb_device_handle_ptr, bEndpointAddress, bytes, offset, length, timeout);
//}

//JNIEXPORT jint JNICALL Java_javalibusb1_libusb1_interrupt_1transfer
  //(JNIEnv *env, jclass klass, jlong libusb_device_handle_ptr, jbyte bEndpointAddress, jbyteArray bytes, jint offset, jint length, jlong timeout)
//{
    //return bulk_or_interrupt_transfer(transferTypeInterrupt, env, klass, libusb_device_handle_ptr, bEndpointAddress, bytes, offset, length, timeout);
//}

/*****************************************************************************
 * javalibusb1_Libusb1UsbDevice
 *****************************************************************************/

JNIEXPORT void JNICALL Java_javalibusb1_Libusb1UsbDevice_nativeClose
    (JNIEnv *env, jobject obj, jlong libusb_device_ptr)
{
    struct libusb_device *device;

    device = (struct libusb_device*)(POINTER_STORAGE_TYPE)libusb_device_ptr;

    if(device == 0) {
        return;
    }

    usbw_unref_device(device);
}

JNIEXPORT jstring JNICALL Java_javalibusb1_Libusb1UsbDevice_nativeGetString
  (JNIEnv *env, jobject obj, jlong libusb_device_ptr, jbyte index, jint length)
{
    struct libusb_device *device;
    struct libusb_device_handle *handle;
    unsigned char *data = NULL;
    jstring s = NULL;
    int err;

    device = (struct libusb_device*)(POINTER_STORAGE_TYPE)libusb_device_ptr;

    data = malloc(sizeof(unsigned char) * length);

    if(data == NULL) {
        throwPlatformException(env, "Unable to allocate buffer.");
        return s;
    }

    if((err = usbw_open(device, &handle))) {
        throwUsbExceptionMsgCode(env, err, "libusb_open(): %s", usbw_error_to_string(err));
        goto fail;
    }

    if((err = usbw_get_string_descriptor_ascii(handle, index, data, length)) <= 0) {
        throwUsbExceptionMsgCode(env, err, "libusb_get_string_descriptor_ascii(): d", err);
        goto fail;
    }

    usbw_close(handle);

    s = (*env)->NewStringUTF(env, (const char*)data);

fail:
    free(data);

    return s;
}


/*****************************************************************************
 * javalibusb1.Libusb1UsbInterface
 *****************************************************************************/

JNIEXPORT void JNICALL Java_javalibusb1_Libusb1UsbInterface_nativeSetConfiguration
  (JNIEnv *env, jobject obj, jlong libusb_device_ptr, jint configuration)
{
    struct libusb_device *device;
    struct libusb_device_handle *handle;
    int err;

    device = (struct libusb_device*)(POINTER_STORAGE_TYPE)libusb_device_ptr;

    if((err = usbw_open(device, &handle))) {
        throwUsbExceptionMsgCode(env, err, "libusb_open(): %s", usbw_error_to_string(err));
        return;
    }

    if((err = usbw_set_configuration(handle, configuration))) {
        throwUsbExceptionMsgCode(env, err, "libusb_set_configuration(): %s", usbw_error_to_string(err));
    };

    usbw_close(handle);
}

JNIEXPORT jlong JNICALL Java_javalibusb1_Libusb1UsbInterface_nativeClaimInterface
  (JNIEnv *env, jobject obj, jlong libusb_device_ptr, jint bInterfaceNumber)
{
    struct libusb_device *device;
    struct libusb_device_handle *handle;
    int err;

    device = (struct libusb_device*)(POINTER_STORAGE_TYPE)libusb_device_ptr;

    if((err = usbw_open(device, &handle))) {
        throwUsbExceptionMsgCode(env, err, "libusb_open(): %s", usbw_error_to_string(err));
        return 0;
    }

    if((err = usbw_claim_interface(handle, bInterfaceNumber))) {
        throwUsbExceptionMsgCode(env, err, "libusb_claim_interface(): %s", usbw_error_to_string(err));
        goto fail;
    };

    return (long)handle;

fail:
    usbw_close(handle);
    return 0;
}

JNIEXPORT void JNICALL Java_javalibusb1_Libusb1UsbInterface_nativeRelease
  (JNIEnv *env, jobject obj, jlong libusb_device_handle_ptr)
{
    usbw_close((struct libusb_device_handle*)((POINTER_STORAGE_TYPE)libusb_device_handle_ptr));
}

/*************************************************************************
 * Asynchronous device I/O
 */

static void asyncCallback(struct libusb_transfer *transfer);
static void freeTransfer(struct libusb_transfer *trans_ptr);
//void Java_javalibusb1_libusb1_free_1transfer(JNIEnv *env, jclass klass, jlong trans_ptr);
static void freeCallback(JNIEnv *env, struct libusb_transfer *transfer);

struct cb_struct {
	jobject irp_g;
	jobject pipe_g;
	jint offset_g;
	jbyteArray byteArray_g;
};

JNIEXPORT jint JNICALL Java_javalibusb1_libusb1_handle_1events_1timeout
  (JNIEnv *env, jclass klass, jlong libusb_context_ptr, jlong timeoutUS)
{
    struct libusb_context *context = (struct libusb_context *)(POINTER_STORAGE_TYPE)libusb_context_ptr;
    struct timeval tv = {0, timeoutUS};
    return usbw_handle_events_timeout(context, &tv);
}

JNIEXPORT jlong JNICALL Java_javalibusb1_libusb1_alloc_1transfer
  (JNIEnv *env, jclass klass, jint iso_packets)
{
	struct libusb_transfer *transfer = usbw_alloc_transfer(iso_packets);
    struct cb_struct *user_data = malloc(sizeof(struct cb_struct));

    if(transfer == NULL || user_data == NULL) {
        throwUsbExceptionMsgCode(env, -1, "Unable to allocate memory.");
        freeTransfer(transfer);
    } else {
        transfer->user_data = user_data;
    }
	return (long)transfer;
}

//JNIEXPORT void JNICALL Java_javalibusb1_libusb1_free_1transfer
  //(JNIEnv *env, jclass klass, jlong trans_ptr)
//{
    //struct libusb_transfer *trans = (struct libusb_transfer *)(POINTER_STORAGE_TYPE)trans_ptr;
	//if(trans != NULL)
		//free(trans->user_data);
    //usbw_free_transfer(trans);
//}

static void freeTransfer(struct libusb_transfer *trans)
{
	if(trans != NULL)
		free(trans->user_data);
    usbw_free_transfer(trans);
}

static void freeCallback(JNIEnv *env, struct libusb_transfer *transfer){
    if(transfer->user_data != NULL){
        struct cb_struct *user_data = transfer->user_data;
        (*env)->DeleteGlobalRef(env, user_data->irp_g);
        (*env)->DeleteGlobalRef(env, user_data->byteArray_g);
        (*env)->DeleteGlobalRef(env, user_data->pipe_g);
    }
    free(transfer->buffer);
}

JNIEXPORT jint JNICALL Java_javalibusb1_libusb1_fill_1and_1submit_1transfer
  (JNIEnv *env, jobject klass, jobject pipe_o, jlong trans_ptr, jbyte type, jlong libusb_device_handle_ptr,
	jbyte endpoint, jbyteArray bytes, jint offset, jint length,	jobject sourceIrp, jlong timeout)
{
    struct libusb_transfer *transfer = (struct libusb_transfer *)(POINTER_STORAGE_TYPE)trans_ptr;
    if(transfer == NULL) {
        throwUsbExceptionMsgCode(env, -1, "Non-NULL transfer required.");
        return LIBUSB_ERROR_INVALID_PARAM;
    }
    struct cb_struct *user_data = transfer->user_data;

	struct libusb_device_handle *dev_handle = (struct libusb_device_handle *)(POINTER_STORAGE_TYPE)libusb_device_handle_ptr;
    if(dev_handle == NULL) {
        throwUsbExceptionMsgCode(env, -1, "Non-NULL dev_handle required.");
        goto cleanup;
    }

    unsigned char *data = malloc(length);
    if(data == NULL) {
        throwUsbExceptionMsgCode(env, -1, "Unable to allocate memory.");
        goto cleanup;
    }

    jobject irp, pipe, byteArray;
    if((irp = (*env)->NewGlobalRef(env, sourceIrp)) == NULL || (pipe = (*env)->NewGlobalRef(env, pipe_o)) == NULL || (byteArray = (*env)->NewGlobalRef(env, bytes)) == NULL){;
        throwUsbExceptionMsgCode(env, -1, "Unable to allocate memory.");
        goto cleanup;
    }

    // If this is an OUT transfer, copy the bytes to data
    if(!(endpoint & LIBUSB_ENDPOINT_DIR_MASK)) {
        (*env)->GetByteArrayRegion(env, bytes, offset, length, (jbyte*)data);
        if((*env)->ExceptionCheck(env)) {
            goto cleanup;
        }
    }else
        user_data->offset_g = offset;


    transfer->dev_handle = dev_handle;
	transfer->endpoint = endpoint;
	transfer->type = type;
	transfer->timeout = timeout;
	transfer->buffer = data;
	transfer->length = length;
    //transfer->flags = 0; // already done by libusb_alloc_transfer
    user_data->irp_g = irp;
    user_data->pipe_g = pipe;
    user_data->byteArray_g = byteArray;
	transfer->callback = &asyncCallback;
    return usbw_submit_transfer(transfer);

cleanup:
    freeCallback(env, transfer);
    freeTransfer(transfer);
    return LIBUSB_ERROR_OTHER;
}


JNIEXPORT jint JNICALL Java_javalibusb1_libusb1_cancel_1transfer
  (JNIEnv *env, jclass klass, jlong trans_ptr)
{
    return usbw_cancel_transfer((struct libusb_transfer *)(POINTER_STORAGE_TYPE)trans_ptr);
}

static void asyncCallback(struct libusb_transfer *transfer)
{
	if(transfer == NULL)
        return;
    int errorCode;
    struct cb_struct *user_data = transfer->user_data;
	if(user_data == NULL){
		errorCode = -1;
        goto exception;
    }
    jobject pipe = user_data->pipe_g;
    jobject irp = user_data->irp_g;
    if(pipe == NULL || irp == NULL)
		goto cleanup; // without the pipe we can't call back; a valid UspIRP irp is also needed
    
    JNIEnv *env;
    if((*jvm)->AttachCurrentThread(jvm, (void **)&env, NULL) != 0){
		errorCode = -1;
        goto exception;
    }

    (*env)->CallVoidMethod(env, irp, usbIrpSetActualLength, transfer->actual_length);
    if((*env)->ExceptionCheck(env)){
		errorCode = -1;
        goto exception;
    }

    //If this is an IN transfer, copy the data to bytes
    if(transfer->endpoint & LIBUSB_ENDPOINT_DIR_MASK) {
		if(user_data->byteArray_g == NULL || transfer->buffer == NULL){
            errorCode = -1;
            goto exception;
        }
        (*env)->SetByteArrayRegion(env, user_data->byteArray_g, user_data->offset_g, transfer->actual_length, (jbyte*)transfer->buffer);
        if((*env)->ExceptionCheck(env)){
            errorCode = -1;
            goto exception;
        }
    }

    errorCode = transfer->status;
exception:
    if(errorCode != LIBUSB_TRANSFER_COMPLETED) {
        jobject e;
        switch(errorCode) {
            case -1:
                e = (*env)->NewObject(env, usbPlatformExceptionClass, usbPlatformExceptionConstructorMsgCode, "Something went wrong while examining the transfer delivered by libusb.", (jint)errorCode);
                break;
            case LIBUSB_TRANSFER_NO_DEVICE:
                e = (*env)->NewObject(env, usbDisconnectedExceptionClass, usbDisconnectedExceptionConstructorMsg, NULL);
                break;
            case LIBUSB_TRANSFER_CANCELLED:
                e = (*env)->NewObject(env, usbAbortExceptionClass, usbAbortExceptionConstructorMsg, NULL);
                break;
            case LIBUSB_TRANSFER_STALL:
                e = (*env)->NewObject(env, usbStallExceptionClass, usbStallExceptionConstructorMsg, NULL);
                break;
            case LIBUSB_TRANSFER_ERROR:
            case LIBUSB_TRANSFER_TIMED_OUT:
            case LIBUSB_TRANSFER_OVERFLOW:
            default:
                e = (*env)->NewObject(env, usbPlatformExceptionClass, usbPlatformExceptionConstructorMsgCode, NULL, (jint)errorCode);
                break;
        }
        (*env)->CallVoidMethod(env, irp, usbIrpSetUsbException, e);
    }

    (*env)->CallVoidMethod(env, pipe, libusb1UsbPipeAsyncCallback, irp);
cleanup:
    freeCallback(env, transfer);
    freeTransfer(transfer);
}
