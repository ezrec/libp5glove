libp5glove by Jason McMullan <jmcmullan@linuxcare.com>
Win32 port by Ross Bencina <rossb@audiomulch.com

Last Updated: February 26, 2004.

This is a port of libp5glove to Win32. It communicates with the glove using
userspace routines supplied in the added files win32_usb_hid.h/c

This version also includes code to transform glove LED coordinates into
orthogonal linear 3-space.

This code has been successfully compiled with gcc and bcc, however gcc's
libhid.a (the import library for windows/system32/hid.dll may be missing
the symbols beginning with HidD_ . I hacked together a minimally working
.a file with impdef and dlltool.

make-win32.bat contains command lines for building p5dump.exe and p5hand.exe with
mingw gcc.

the glut32.dll is a mingw version, supplied so you can run p5hand.exe easily.


History:


26th February 2004:

added second version of matrix code to p5glove.c


25th February 2004:

added initial version of matrix code to p5glove.c


20th February 2004:

added files src/win32_usb_hid.h and src/win32_usb_hid.c

added hacked_libhid/libhid.a for compiling with Mingw. (just use implib.exe with bcc, or for msvc get hold of the hid.lib from the DDK or here: http://www.alanmacek.com/usb/hostcode.html )

changed src/p5glove.c:

	- #ifndefed out the reframing code (doesn't seem to be needed, and seems to
		be buggy anyway since there is no guarantee the bytes wouldn't be 1
		naturally).
	- added conditional win32 code to call win32_usb_hid.h i/o functions
	- added clarifying () at lines 90 and 91 (79 and 80 in original file
	- changed "if (axis == 15)" to "if (axis > 7 )" at line 111 (100 in original file)
		this was causing problems for Tim Kreger on the Mac.
	- note that we don't use 30Hz speed limiting like the Linux code because
		it doesn't seem to be needed

changed demo/p5common.c:

	- #defined M_PI when it isn't available

changed demo/p5dump.c

	- #included stdlib.h for exit() and string.h for memset()
	- replaced '.' with ',' to separate fingers in printf
	- return 0 from main()

changed demo/p5hand.c
	
	- #defined M_PI when it isn't available
	- disabled GL_DEPTH_TEST and gluSphere bounding sphere because 
		the sphere seemed to be occluding the display at times.
	- return 0 from main()

