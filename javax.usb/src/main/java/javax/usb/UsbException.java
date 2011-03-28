package javax.usb;

public class UsbException extends Exception{

public UsbException(){
	super();
}

public UsbException(String message){
	super(message);
}

public UsbException(String message, Throwable cause){
	super(message, cause);
}

public UsbException(Throwable cause){
	super(cause);
}

public UsbException(String s, Exception cause){
	super(s, cause);
}
}
