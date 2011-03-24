package javax.usb.util;

import javax.usb.*;
import javax.usb.event.UsbDeviceListener;
import javax.usb.event.UsbPipeListener;
import java.io.UnsupportedEncodingException;
import java.util.List;

public class UsbUtil{

public static class SynchronizedUsbPipe implements UsbPipe{

	protected java.lang.Object openLock;
	protected java.lang.Object submitLock;
	public UsbPipe usbPipe;

	public SynchronizedUsbPipe(UsbPipe usbPipe){
		this.usbPipe = usbPipe;
	}

	public void abortAllSubmissions() throws UsbNotActiveException, UsbNotOpenException, UsbDisconnectedException{
	}

	public void addUsbPipeListener(UsbPipeListener listener){
	}

	public UsbIrp asyncSubmit(byte[] data)
			throws UsbException, UsbNotActiveException, UsbNotOpenException, IllegalArgumentException, UsbDisconnectedException{
		return null;
	}

	public void asyncSubmit(List list)
			throws UsbException, UsbNotActiveException, UsbNotOpenException, IllegalArgumentException, UsbDisconnectedException{
	}

	public void asyncSubmit(UsbIrp irp)
			throws UsbException, UsbNotActiveException, UsbNotOpenException, IllegalArgumentException, UsbDisconnectedException{
	}

	public void close() throws UsbException, UsbNotActiveException, UsbNotOpenException, UsbDisconnectedException{
	}

	public UsbControlIrp createUsbControlIrp(byte bmRequestType, byte bRequest, short wValue, short wIndex){
		return null;
	}

	public UsbIrp createUsbIrp(){
		return null;
	}

	public UsbEndpoint getUsbEndpoint(){
		return null;
	}

	public boolean isActive(){
		return false;
	}

	public boolean isOpen(){
		return false;
	}

	public void open() throws UsbException, UsbNotActiveException, UsbNotClaimedException, UsbDisconnectedException{
	}

	public void removeUsbPipeListener(UsbPipeListener listener){
	}

	public int syncSubmit(byte[] data)
			throws UsbException, UsbNotActiveException, UsbNotOpenException, IllegalArgumentException, UsbDisconnectedException{
		return 0;
	}

	public void syncSubmit(List<UsbIrp> list)
			throws UsbException, UsbNotActiveException, UsbNotOpenException, IllegalArgumentException, UsbDisconnectedException{
	}

	public void syncSubmit(UsbIrp irp)
			throws UsbException, UsbNotActiveException, UsbNotOpenException, IllegalArgumentException, UsbDisconnectedException{
	}
}

public static class SynchronizedUsbDevice implements UsbDevice{

	protected java.lang.Object listenerLock;
	protected java.lang.Object submitLock;
	public UsbDevice usbDevice;

	public SynchronizedUsbDevice(UsbDevice usbDevice){
		this.usbDevice = usbDevice;
		throw new RuntimeException("Not implemented");
	}

	public String getManufacturerString() throws UsbException, UnsupportedEncodingException, UsbDisconnectedException{
		return null;
	}

	public String getSerialNumberString() throws UsbException, UnsupportedEncodingException, UsbDisconnectedException{
		return null;
	}

	public String getProductString() throws UsbException, UnsupportedEncodingException, UsbDisconnectedException{
		return null;
	}

	public UsbStringDescriptor getUsbStringDescriptor(byte index){
		return null;
	}

	public String getString(byte index) throws UsbException, UsbDisconnectedException{
		return null;
	}

	public UsbDeviceDescriptor getUsbDeviceDescriptor(){
		return null;
	}

	public Object getSpeed(){
		return null;
	}

	public boolean isConfigured(){
		return false;
	}

	public boolean isUsbHub(){
		return false;
	}

	public void addUsbDeviceListener(UsbDeviceListener listener){
	}

	public void removeUsbDeviceListener(UsbDeviceListener listener){
	}

	public UsbControlIrp createUsbControlIrp(byte bmRequestType, byte bRequest, short wValue, short wIndex){
		return null;
	}

	public void asyncSubmit(List<UsbControlIrp> list) throws UsbException, IllegalArgumentException, UsbDisconnectedException{
	}

	public void asyncSubmit(UsbControlIrp irp) throws UsbException, IllegalArgumentException, UsbDisconnectedException{
	}

	public void syncSubmit(List<UsbControlIrp> list) throws UsbException, IllegalArgumentException, UsbDisconnectedException{
	}

	public void syncSubmit(UsbControlIrp irp) throws UsbException, IllegalArgumentException, UsbDisconnectedException{
	}

	public UsbPort getParentUsbPort() throws UsbDisconnectedException{
		return null;
	}

	public byte getActiveUsbConfigurationNumber(){
		return 0;
	}

	public UsbConfiguration getActiveUsbConfiguration(){
		return null;
	}

	public boolean containsUsbConfiguration(byte number){
		return false;
	}

	public UsbConfiguration getUsbConfiguration(byte number){
		return null;
	}

	public List<UsbConfiguration> getUsbConfigurations(){
		return null;
	}
}

/** API violation with a warm cup of "fuck you". */
private UsbUtil(){
}

public static String getSpeedString(Object object){
	if(object == null){
		return "null";
	}

	if(object == UsbConst.DEVICE_SPEED_LOW){
		return "Low";
	}

	if(object == UsbConst.DEVICE_SPEED_FULL){
		return "Full";
	}

	if(object == UsbConst.DEVICE_SPEED_HIGH){
		return "High";
	}

	return "Invalid";
}

static UsbDevice synchronizedUsbDevice(UsbDevice usbDevice){
	return new SynchronizedUsbDevice(usbDevice);
}

static UsbPipe synchronizedUsbPipe(UsbPipe usbPipe){
	return new SynchronizedUsbPipe(usbPipe);
}

private static final char[] hexDigits = new char[] {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

public static String toHexString(byte b){
	int i = unsignedInt(b);
	return new String(new char[] {hexDigits[i>>4], hexDigits[i&0x0f],
	});
}

public static String toHexString(int i){
	return new String(new char[] {hexDigits[(i>>28)&0x0f],
	                              hexDigits[(i>>24)&0x0f],
	                              hexDigits[(i>>20)&0x0f],
	                              hexDigits[(i>>16)&0x0f],
	                              hexDigits[(i>>12)&0x0f],
	                              hexDigits[(i>>8)&0x0f],
	                              hexDigits[(i>>4)&0x0f],
	                              hexDigits[i&0x0f],
	});
}

public static String toHexString(long l){
	throw new RuntimeException("Not implemented");

}

public static String toHexString(long l, char c, int min, int max){
	throw new RuntimeException("Not implemented");

}

public static String toHexString(short i){
	return new String(new char[] {hexDigits[(i>>12)&0x0f], hexDigits[(i>>8)&0x0f], hexDigits[(i>>4)&0x0f], hexDigits[i&0x0f],
	});
}

public static String toHexString(java.lang.String delimiter, byte[] array){
	throw new RuntimeException("Not implemented");

}

public static String toHexString(java.lang.String delimiter, byte[] array, int length){
	throw new RuntimeException("Not implemented");

}

public static String toHexString(java.lang.String delimiter, int[] array){
	throw new RuntimeException("Not implemented");

}

public static String toHexString(java.lang.String delimiter, int[] array, int length){
	throw new RuntimeException("Not implemented");

}

public static String toHexString(java.lang.String delimiter, long[] array){
	throw new RuntimeException("Not implemented");

}

public static String toHexString(java.lang.String delimiter, long[] array, int length){
	throw new RuntimeException("Not implemented");

}

public static String toHexString(java.lang.String delimiter, short[] array){
	throw new RuntimeException("Not implemented");

}

public static String toHexString(java.lang.String delimiter, short[] array, int length){
	throw new RuntimeException("Not implemented");

}

/**
 @param a Least significant byte
 @param b Most significant byte
 */
public static short toShort(byte a, byte b){
	return (short)(b<<8|a);
}

/**
 @param a Least significant byte
 @param d Most significant byte
 */
public static int toInt(byte a, byte b, byte c, byte d){
	return d<<24|c<<16|b<<8|a;
}

public static int toInt(short mss, short lss){
	throw new RuntimeException("Not implemented");

}

public static long toLong(byte byte7, byte byte6, byte byte5, byte byte4, byte byte3, byte byte2, byte byte1, byte byte0){
	throw new RuntimeException("Not implemented");

}

public static long toLong(int msi, int lsi){
	throw new RuntimeException("Not implemented");

}

public static long toLong(short short3, short short2, short short1, short short0){
	throw new RuntimeException("Not implemented");

}

public static int unsignedInt(byte b){
	return 0x000000ff&b;
}

public static int unsignedInt(short s){
	return 0x0000ffff&s;
}

public static long unsignedLong(byte b){
	return 0x00000000000000ff&b;
}

public static long unsignedLong(int s){
	return 0x00000000ffffffff&s;
}

public static long unsignedLong(short s){
	return 0x000000000000ffff&s;
}

public static short unsignedShort(byte b){
	return (short)(0x00ff&b);
}
}
