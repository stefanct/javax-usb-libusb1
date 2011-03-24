package javax.usb.extra;

import javax.usb.*;
import java.io.Closeable;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import static javax.usb.util.UsbUtil.toHexString;

public class ExtraUsbUtil{

private ExtraUsbUtil(){
}

public static boolean isUsbDevice(UsbDeviceDescriptor descriptor, short idVendor, short idProduct){
	return descriptor.idVendor() == idVendor && descriptor.idProduct() == idProduct;
}

public static UsbDevice findUsbDevice(UsbHub usbHub, short idVendor, short idProduct){
	for(UsbDevice device : usbHub.getAttachedUsbDevices()){
		if(device.isUsbHub()){
			continue;
		}

		UsbDeviceDescriptor deviceDescriptor = device.getUsbDeviceDescriptor();

		if(isUsbDevice(deviceDescriptor, idVendor, idProduct)){
			return device;
		}
	}

	for(UsbDevice device : usbHub.getAttachedUsbDevices()){
		if(!device.isUsbHub()){
			continue;
		}

		UsbDevice foundDevice = findUsbDevice((UsbHub)device, idVendor, idProduct);

		if(foundDevice != null){
			return foundDevice;
		}
	}

	return null;
}

public static List<UsbDevice> findUsbDevices(UsbHub usbHub, short idVendor, short idProduct){
	List<UsbDevice> devices = new ArrayList<UsbDevice>();

	for(UsbDevice device : usbHub.getAttachedUsbDevices()){
		if(device.isUsbHub()){
			continue;
		}

		UsbDeviceDescriptor deviceDescriptor = device.getUsbDeviceDescriptor();

		if(deviceDescriptor.idVendor() == idVendor && deviceDescriptor.idProduct() == idProduct){
			devices.add(device);
		}
	}

	for(UsbDevice device : usbHub.getAttachedUsbDevices()){
		if(!device.isUsbHub()){
			continue;
		}

		List<UsbDevice> foundDevices = findUsbDevices((UsbHub)device, idVendor, idProduct);

		devices.addAll(foundDevices);
	}

	return devices;
}

public static List<UsbDevice> listUsbDevices(UsbHub usbHub){
	List<UsbDevice> devices = new ArrayList<UsbDevice>();

	for(UsbDevice device : usbHub.getAttachedUsbDevices()){
		if(device.isUsbHub()){
			continue;
		}

		devices.add(device);
	}

	for(UsbDevice device : usbHub.getAttachedUsbDevices()){
		if(!device.isUsbHub()){
			continue;
		}

		devices.addAll(listUsbDevices(usbHub));
	}

	return devices;
}

public static String deviceIdToString(UsbDeviceDescriptor descriptor){
	return toHexString(descriptor.idVendor()) + ':' + toHexString(descriptor.idProduct());
}

public static String deviceIdToString(short idVendor, short idProduct){
	return toHexString(idVendor) + ':' + toHexString(idProduct);
}

public static String twoDigitBdc(short bdc){
	return ((bdc&0xf000)>>12) + ((bdc&0x0f00)>>8) + "." + ((bdc&0x00f0)>>4) + (bdc&0x000f);
}

public static String decodeLibusbError(int code){
	switch(code){
		case 0:
			return "success";
		case -1:
			return "io";
		case -2:
			return "invalid parameter";
		case -3:
			return "access";
		case -4:
			return "no device";
		case -5:
			return "not found";
		case -6:
			return "busy";
		case -7:
			return "timeout";
		case -8:
			return "overflow";
		case -9:
			return "pipe";
		case -10:
			return "interrupted";
		case -11:
			return "no mem";
		case -12:
			return "not supported";
		case -99:
			return "other";
		default:
			return "unknown";
	}
}

public static void close(Object o){
	if(o instanceof Closeable){
		Closeable closeable = (Closeable)o;

		try{
			closeable.close();
		} catch(IOException e){
			// ignore
		}
	}
}
}
