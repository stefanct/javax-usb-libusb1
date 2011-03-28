package javax.usb;

import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.InvocationTargetException;
import java.util.Properties;

public final class UsbHostManager{

public static final String JAVAX_USB_SERVICES = "javax.usb.services";

public static final String JAVAX_USB_PROPERTIES_FILE = "javax.usb.properties";

private static UsbServices usbServices = null;

private final static Object lock = new Object();

private UsbHostManager(){
}

public static UsbServices getUsbServices() throws UsbException, SecurityException{
	synchronized(lock){
		if(usbServices == null){
			usbServices = initialize();
		}
		return usbServices;
	}
}

private static UsbServices initialize() throws UsbException, SecurityException{
	Properties properties = loadProperties();
	String services = getServicesName(properties);

	// TODO: Use the thread's current context class loader?
	try{
		return (UsbServices)UsbHostManager.class.getClassLoader().loadClass(services).getConstructor(Properties.class).newInstance(properties);
	} catch(ClassNotFoundException e){
		throw new UsbPlatformException("Unable to load UsbServices class '" + services + '\'', e);
	} catch(InstantiationException e){
		throw new UsbPlatformException("Unable to instantiate class '" + services + '\'', e);
	} catch(IllegalAccessException e){
		throw new UsbPlatformException("Unable to instantiate class '" + services + '\'', e);
	} catch(ClassCastException e){
		throw new UsbPlatformException("Class " + services + " is not an instance of javax.usb.UsbServices", e);
	} catch(NoSuchMethodException e){
		throw new UsbPlatformException("Class " + services + " does not have the needed constructor", e);
	} catch(InvocationTargetException e){
		throw new UsbPlatformException("Class " + services + " has thrown an exception while initializing", e);
	}
}

private static Properties loadProperties() throws UsbException{
	String fileName = System.getenv(getEnvFromProperty(JAVAX_USB_PROPERTIES_FILE));
	if(fileName == null){
		fileName = System.getProperty(JAVAX_USB_PROPERTIES_FILE);
		if(fileName == null){
			// default
			fileName = JAVAX_USB_PROPERTIES_FILE;
		}
	}

	// load the properties file
	InputStream stream = UsbHostManager.class.getClassLoader().getResourceAsStream(fileName);
	if(stream == null){
		return null;
	}

	try{
		Properties properties = new Properties();
		properties.load(stream);
		return properties;
	} catch(IOException e){
		throw new UsbPlatformException("Error while reading configuration file", e);
	} finally{
		try{
			stream.close();
		} catch(IOException e){
			// ignore
		}
	}
}

public static String getServicesName(Properties properties) throws UsbException, SecurityException{
	// first try to get the services name directly form the environment or system properties
	String servicesName = System.getenv(getEnvFromProperty(JAVAX_USB_SERVICES));
	if(servicesName != null){
		return servicesName;
	}

	servicesName = System.getProperty(JAVAX_USB_SERVICES);
	if(servicesName != null){
		return servicesName;
	}

	// else try to determine the name of the services class from properties
	servicesName = properties.getProperty(JAVAX_USB_SERVICES);
	if(servicesName != null){
		return servicesName;
	}
	throw new UsbException("Neither environment variable '"
	                       + getEnvFromProperty(JAVAX_USB_SERVICES)
	                       + "' nor property '"
	                       + JAVAX_USB_SERVICES
	                       + "' in the configuration file or in the system properties are set.");
}

private static String getEnvFromProperty(String propertyName){
	return propertyName.toUpperCase().replace('.', '_');
}
}
