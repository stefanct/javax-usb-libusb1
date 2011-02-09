package javax.usb;

public class UsbNotActiveException extends RuntimeException {
    public UsbNotActiveException() {
        super();
    }

    public UsbNotActiveException(String s) {
        super(s);
    }
}
