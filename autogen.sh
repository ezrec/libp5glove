libtoolize --automake
aclocal -I macros
autoheader
automake --add-missing
autoconf
