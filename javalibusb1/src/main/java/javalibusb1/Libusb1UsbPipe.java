package javalibusb1;

import javax.usb.*;
import javax.usb.event.UsbPipeDataEvent;
import javax.usb.event.UsbPipeErrorEvent;
import javax.usb.event.UsbPipeListener;
import javax.usb.util.DefaultUsbControlIrp;
import javax.usb.util.DefaultUsbIrp;
import java.util.ArrayList;
import java.util.List;

import static javalibusb1.Libusb1UsbControlIrp.createControlIrp;
import static javalibusb1.Libusb1UsbDevice.internalSyncSubmitControl;
import static javax.usb.UsbConst.*;

public class Libusb1UsbPipe implements UsbPipe {

    private final Libusb1UsbEndpoint endpoint;
    private boolean open;

    // oh my; the RI is bloat.
    // but firing the events in another thread (todo) and making it threadsafe (done) may be useful,
    // although it is not specified anywhere? - stefan
    private final List<UsbPipeListener> listeners;

    public Libusb1UsbPipe(Libusb1UsbEndpoint endpoint) {
        this.endpoint = endpoint;
        listeners = new ArrayList<UsbPipeListener>(0);
    }

    // -----------------------------------------------------------------------
    // UsbPipe Implementation
    // -----------------------------------------------------------------------

    public void abortAllSubmissions() {
        throw new RuntimeException("Not implemented");
    }

    public void addUsbPipeListener(UsbPipeListener listener) {
        synchronized(listeners){
            if (!listeners.contains(listener))
                listeners.add(listener);
        }
    }

    public void removeUsbPipeListener(UsbPipeListener listener) {
        synchronized(listeners){
            listeners.remove(listener);
        }
    }

    public UsbIrp asyncSubmit(byte[] data) throws UsbException{
	    UsbIrp irp = new DefaultUsbIrp();
	    irp.setData(data);
	    asyncSubmit(irp);
	    return irp;
    }

    public void asyncSubmit(List list) throws UsbException{
	    // generics anyone?! :/
	    for (Object o : list) {
		    if (o != null && UsbIrp.class == o.getClass())
	            asyncSubmit((UsbIrp)o);
		    else
			    throw new IllegalArgumentException("The List contains a non-UsbIrp object."); // like RI
	    }
    }

    public void asyncSubmit(UsbIrp irp) throws UsbException{
	    // TODO: isochronous and control transfers
	    UsbException ex;
	    try{
			if (!isOpen()) {
				throw new UsbNotOpenException();
			}

			if (!isActive()) {
				throw new UsbNotActiveException();
			}

			long trans_ptr = libusb1.alloc_transfer(0);
			int err = libusb1.fill_and_submit_transfer(this, trans_ptr,
													   endpoint.getType(),
													   endpoint.usbInterface.libusb_device_handle_ptr,
													   endpoint.descriptor.bEndpointAddress(),
													   irp.getData(),
													   irp.getOffset(),
													   irp.getLength(), irp,
													   0);
			if(0 == err){
				return;
			}
			ex = new UsbPlatformException("Submitting failed", err);
		} catch(UsbException e){
			ex = e;
		}
	    fireUsbPipeErrorEvent(new UsbPipeErrorEvent(this, irp));
	    throw ex;
    }

	public void asyncCallback(UsbIrp irp) {
		irp.complete();
		if(irp.isUsbException())
			fireUsbPipeErrorEvent(new UsbPipeErrorEvent(this, irp));
		else
			fireUsbPipeDataEvent(new UsbPipeDataEvent(this, irp));

		synchronized(irp){
			irp.notifyAll();
		}
	}

    public void close() {
        open = false;
    }

    public UsbControlIrp createUsbControlIrp(byte bmRequestType, byte bRequest, short wValue, short wIndex) {
        return new DefaultUsbControlIrp(bmRequestType, bRequest, wValue, wIndex);
    }

    public UsbIrp createUsbIrp() {
        return new DefaultUsbIrp();
    }

    public UsbEndpoint getUsbEndpoint() {
        return endpoint;
    }

    public boolean isActive() {
        return endpoint.getUsbInterface().isActive() && endpoint.getUsbInterface().getUsbConfiguration().isActive();
    }

    public boolean isOpen() {
        return open;
    }

    public void open() throws UsbException {
        try{
            if (!isActive()) {
                throw new UsbNotActiveException();
            }

            if (!endpoint.getUsbInterface().isClaimed()) {
                throw new UsbNotClaimedException();
            }

            if (open) {
                throw new UsbException("Already open");
            }
            open = true;
        } catch(UsbException e){
            fireUsbPipeErrorEvent(new UsbPipeErrorEvent(this, e));
            throw e;
        }
    }

    public int syncSubmit(byte[] data) throws UsbException, UsbNotActiveException, UsbNotOpenException, java.lang.IllegalArgumentException {
        // This is not correct after what I understand from the reference implementation
        UsbIrp irp = new DefaultUsbIrp();
        irp.setData(data);
	    syncSubmit(irp);
	    return irp.getActualLength();
    }

    public void syncSubmit(List<UsbIrp> list) throws UsbException, UsbNotActiveException, UsbNotOpenException, java.lang.IllegalArgumentException {
        for (UsbIrp usbIrp : list) {
            syncSubmit(usbIrp);
        }
    }

    @SuppressWarnings({"ResultOfMethodCallIgnored"})
    synchronized public void syncSubmit(UsbIrp irp) throws UsbException, UsbNotActiveException, UsbNotOpenException, java.lang.IllegalArgumentException {
	    // From what I can tell from the API you don't have to open a device to send control packets.
	    try{
	        if (irp instanceof UsbControlIrp) {
	            if (endpoint.getType() != ENDPOINT_TYPE_CONTROL) {
	                throw new IllegalArgumentException("This is not a control endpoint.");
	            }

	            internalSyncSubmitControl(endpoint.usbInterface.libusb_device_handle_ptr, createControlIrp((UsbControlIrp)irp));
		        irp.getActualLength();
		        return;
	        }
	    } catch(UsbException e){
	        irp.setUsbException(e);
	        fireUsbPipeErrorEvent(new UsbPipeErrorEvent(this, irp));
	        throw e;
	    }
	    try{
			synchronized(irp){
				asyncSubmit(irp);
				while(!irp.isComplete()){
					irp.wait();
				}
			}
		    if(irp.isUsbException()){
			    throw irp.getUsbException();
		    }
	    }catch(InterruptedException cause){
		    UsbPlatformException e = new UsbPlatformException("Interrupted.", cause);
		    irp.setUsbException(e);
		    throw e;
	    }

    }

    // -----------------------------------------------------------------------
    //
    // -----------------------------------------------------------------------

    private void fireUsbPipeDataEvent(UsbPipeDataEvent e){
        synchronized(listeners){
            try{
                for (UsbPipeListener l : listeners)
                    l.dataEventOccurred(e);
            } catch(Exception ignored){}
        }
    }

    private void fireUsbPipeErrorEvent(UsbPipeErrorEvent e){
        synchronized(listeners){
            try{
                for (UsbPipeListener l : listeners)
                    l.errorEventOccurred(e);
            } catch (Exception ignored){}
        }
    }

}

