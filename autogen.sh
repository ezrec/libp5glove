libtoolize --force
autoheader
automake --add-missing
aclocal -I macros
autoconf
