dnl Process this file with autoconf to produce a configure script.
AC_INIT([libp5glove],[0.3])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_CONFIG_MACRO_DIR([m4])

dnl Checks for programs.
AC_PROG_CC
LT_INIT
AC_PROG_INSTALL

dnl Checks for libraries.

dnl Check for X11
AC_PATH_X
AC_PATH_XTRA

dnl Check for GL libs
AC_CHECK_LIB(GL, glAccum)
AC_CHECK_LIB(GLU, gluSphere)
AC_CHECK_LIB(glut, glutInit)

dnl Replace `main' with a function in -lusb:
AC_CHECK_LIB(usb, usb_open)

AC_CHECK_LIB(usb, usb_interrupt_read, 
	     [AC_DEFINE(HAVE_USB_INTERRUPT_READ,1,[libusb has usb_interrupt_read()])],
	     [AC_MSG_WARN([You should really use libusb 0.1.7 or higher for usb_interrupt_read() support.])])

dnl Checks for header files.

dnl Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_TIME

dnl Checks for library functions.
AC_HEADER_STDC
AC_CHECK_FUNCS(gettimeofday strerror)

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	demo/Makefile
])
AC_OUTPUT
