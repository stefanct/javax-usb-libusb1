package javalibusb1;

import java.io.*;
import java.util.Properties;

public class Libusb1Utils{

public static final String JAVAX_USB_PROPERTIES_FILE = "javax.usb.properties";
private static Properties javaxUsbProperties;

static{
	javaxUsbProperties = loadProperties();
}

private static Properties loadProperties(){
	String fileName = System.getenv(getEnvFromProperty(JAVAX_USB_PROPERTIES_FILE));
	if(fileName == null){
		fileName = System.getProperty(JAVAX_USB_PROPERTIES_FILE);
		if(fileName == null){
			// default
			fileName = JAVAX_USB_PROPERTIES_FILE;
		}
	}

	// load the properties file
	InputStream stream = Libusb1Utils.class.getClassLoader().getResourceAsStream(fileName);
	if(stream == null){
		return null;
	}

	try{
		Properties properties = new Properties();
		properties.load(stream);
		if(properties.isEmpty())
			return null;
		else
			return properties;
	} catch(IOException e){
		throw new RuntimeException("Error while reading configuration file", e);
	} finally{
		try{
			stream.close();
		} catch(IOException e){
			// ignore
		}
	}
}

private static String getEnvFromProperty(String propertyName){
	return propertyName.toUpperCase().replace('.', '_');
}

synchronized public static void addProperties(Properties props){
	if(props == null || props.isEmpty())
		return;
	if(javaxUsbProperties == null)
		javaxUsbProperties = new Properties();
	javaxUsbProperties.putAll(props);
}

/**
 Returns the property value corresponding to the key given.
 <p/>
 The method tries to retrieve the value from different sources, namely:
 <lu>
 <li>Environment variables (after replacing '.' in the key with '_' and converting it to upper case,</li>
 <li>System properties,</li>
 <li>Properties loaded by the static initializer.</li>
 </lu>
 */
public static String getProperty(String propertyKey){
	String s = null;

	try{
		s = System.getenv(getEnvFromProperty(propertyKey));
	} catch(SecurityException e){
		// ignore
	}
	if(s != null){
		return s;
	}

	try{
		s = System.getProperty(propertyKey);
	} catch(SecurityException e){
		// ignore
	}
	if(s != null){
		return s;
	}

	if(javaxUsbProperties != null){
		s = javaxUsbProperties.getProperty(propertyKey);
	}
	return s;
}

public static void closeSilently(Closeable closeable){
	if(closeable == null){
		return;
	}
	try{
		closeable.close();
	} catch(IOException e){
		// ignore
	}
}
}
