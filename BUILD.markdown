Prerequisites
=============

The build process uses several marvin plugins. Most of them are available via the usual plugin repositories.

Last time i tried, i had to build two plugins myself: *cpptasks-parallel* and *maven-nar-plugin*

	mkdir workingdir
	cd workingdir
	git clone http://github.com/duns/cpptasks-parallel.git
	cd cpptasks-parallel
	mvn install
	cd ..
	git clone http://github.com/duns/maven-nar-plugin.git
	cd maven-nar-plugin
	mvn install
	cd ..

javax-usb-libusb1
=================
To fetch this javax.usb implementation create a directory where it should reside, cd into it and clone it:

	git clone http://github.com/trygvis/javax-usb-libusb1.git

Inside the repo is a referenced external repo, which houses Trygve's branch of libusb.
To fetch, build and install it (notice that you have to provide an absolute path to libusb.home):

	git submodule init
	git submodule update
	cd libusb
	mvn install -Dlibusb.home='absolute-workingdir/javax-usb-libusb1/libusb-git'

After that a simple `mvn install` in the root directory should work.