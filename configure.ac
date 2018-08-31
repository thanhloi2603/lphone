AC_INIT([lphone], [0.1], [loi.dangthanh@gmail.com])
AM_INIT_AUTOMAKE
AC_PROG_CC

# AC_CONFIG_SRCDIR([call_func.h lutils.h])
# AC_CONFIG_HEADERS([call_func.h lutils.h])

# Checks for libraries.
# pjsua
AC_CHECK_LIB([pjsua], [pjsua_init, pjsua_call_make_call, pjsua_destroy], 
	[export LIBS="$LIBS `pkg-config --libs libpjproject`"], [AC_MSG_ERROR([Please install pjsua before trying to build this prog.])], 
	[-lpjsip -lpjmedia-codec -lpjmedia -lpjnath -lpjlib-util -lsrtp -lgsmcodec -lspeex -lilbccodec -lg7221codec])

# confuse
AC_CHECK_LIB([confuse], [cfg_init, cfg_parse], 
	[export LIBS="$LIBS `pkg-config --libs libconfuse`"], [AC_MSG_ERROR([Please install libconfuse before trying to build this prog.])])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h stdio.h])
AC_CHECK_HEADER([confuse.h])
AC_CHECK_HEADER([pjsua.h])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strcasecmp strcpy snprintf strsep])

# Checks for types.
AC_CHECK_TYPES([pjsua_acc_id, pjsua_call_id, pjsua_acc_config, pjsua_config, pjsua_logging_config, pjsua_transport_config],
	[], [AC_MSG_ERROR([we need pjsua's types])], [[#include <pjsua-lib/pjsua.h>]])

AC_CHECK_TYPES([cfg_opt_t, cfg_t], [], [AC_MSG_ERROR([we need confuse's types])], [[#include <confuse.h>]])

# Checks for required files.
AC_CHECK_FILE(lphone.conf, [], [AC_MSG_WARN([lphone.conf is missing])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
