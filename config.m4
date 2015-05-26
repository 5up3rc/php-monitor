dnl $Id$
dnl config.m4 for extension monitor

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(monitor, for monitor support,
dnl Make sure that the comment is aligned:
dnl [  --with-monitor             Include monitor support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(monitor, whether to enable monitor support,
Make sure that the comment is aligned:
[  --enable-monitor           Enable monitor support])

if test "$PHP_MONITOR" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-monitor -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/monitor.h"  # you most likely want to change this
  dnl if test -r $PHP_MONITOR/$SEARCH_FOR; then # path given as parameter
  dnl   MONITOR_DIR=$PHP_MONITOR
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for monitor files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       MONITOR_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$MONITOR_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the monitor distribution])
  dnl fi

  dnl # --with-monitor -> add include path
  dnl PHP_ADD_INCLUDE($MONITOR_DIR/include)

  dnl # --with-monitor -> check for lib and symbol presence
  dnl LIBNAME=monitor # you may want to change this
  dnl LIBSYMBOL=monitor # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $MONITOR_DIR/$PHP_LIBDIR, MONITOR_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_MONITORLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong monitor lib version or lib not found])
  dnl ],[
  dnl   -L$MONITOR_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(MONITOR_SHARED_LIBADD)

  PHP_NEW_EXTENSION(monitor, monitor.c, $ext_shared)
fi
if test -z "$PHP_DEBUG"; then 
	AC_ARG_ENABLE(debug,
	[	--enable-debug			compile with debugging symbols],[
		PHP_DEBUG=$enableval
	],[	PHP_DEBUG=no
	])
fi