dnl --------------------------------------------------------------
dnl Configure.ac for libdvdnav
dnl --------------------------------------------------------------

AC_INIT
AC_CONFIG_SRCDIR([src/dvdnav.c])

dnl --------------------------------------------------------------
dnl Require autoconf version 2.53
dnl --------------------------------------------------------------
AC_PREREQ(2.53)

dnl --------------------------------------------------------------
dnl Making releases: DVDNAV_SUB += 1; change LT_* accordingly
dnl --------------------------------------------------------------
DVDNAV_MAJOR=0
DVDNAV_MINOR=1
DVDNAV_SUB=10
DVDNAV_PRE=""

AC_SUBST(DVDNAV_MAJOR)
AC_SUBST(DVDNAV_MINOR)
AC_SUBST(DVDNAV_SUB)

dnl The libtool version numbers (LT_*); Don't even think about faking this!
dnl 
dnl immediately before every release do:
dnl ===================================
dnl if (the interface is totally unchanged from previous release)
dnl    LT_REVISION ++;
dnl else { /* interfaces have been added, removed or changed */
dnl    LT_REVISION = 0;
dnl    LT_CURRENT ++;
dnl /*
dnl The LT_AGE number is how many consecutive versions of the API the
dnl current implementation supports. Thus, if the CURRENT library API is
dnl the sixth published version of the interface and it is also binary
dnl compatible with the fourth and fifth versions (i.e., the last two),
dnl the CURRENT:REVISION:AGE might be 6:0:2. When you break binary
dnl compatibility, for example by removing a public function call, you
dnl need to set AGE to 0 and, of course, increment CURRENT.
dnl In linux, the final .so number is CURRENT-AGE.AGE.REVISION
dnl As libdvdnav is changing so much, the advice will be to keep LT_AGE
dnl always = 0
dnl */
dnl    if (any interfaces have been _added_ since last release)
dnl       AGE ++;
dnl    if (any interfaces have been _removed_ or incompatibly changed)
dnl       AGE = 0;
dnl }
LT_CURRENT=4
LT_AGE=0
LT_REVISION=0

AC_SUBST(LT_CURRENT)
AC_SUBST(LT_AGE)
AC_SUBST(LT_REVISION)

PACKAGE="libdvdnav"
RELEASE="1"
TAR_NAME=$PACKAGE-$DVDNAV_MAJOR.$DVDNAV_MINOR.$DVDNAV_SUB$DVDNAV_PRE
SPEC_VERSION=$DVDNAV_MAJOR.$DVDNAV_MINOR.$DVDNAV_SUB$DVDNAV_PRE

AC_SUBST(TAR_NAME)
AC_SUBST(SPEC_VERSION)
AC_SUBST(PACKAGE)
AC_SUBST(RELEASE)

LIBNAME=libdvdnav$DVDNAV_MAJOR
AC_SUBST(LIBNAME)

AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE("libdvdnav", $DVDNAV_MAJOR.$DVDNAV_MINOR.$DVDNAV_SUB$DVDNAV_PRE)

dnl --------------------------------------------------------------
dnl Make possible to build for another arch.
dnl --------------------------------------------------------------
if test x$DVDNAV_BUILD != "x"; then
  AC_MSG_RESULT(*** build forced to $DVDNAV_BUILD ***)
  build=$DVDNAV_BUILD
  host=$DVDNAV_BUILD
else
  check_athlon=yes
fi

dnl create a config.h file (Automake will add -DHAVE_CONFIG_H)
AM_CONFIG_HEADER(config.h)

dnl --------------------------------------------------------------
dnl Checks for programs.
dnl --------------------------------------------------------------
dnl Save CFLAGS, AC_ISC_POSIX set some unwanted default CFLAGS
saved_CFLAGS="$CFLAGS"
AC_ISC_POSIX
CFLAGS="$saved_CFLAGS"
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LN_S

dnl --------------------------------------------------------------
dnl Libtool
dnl --------------------------------------------------------------
AC_LIBTOOL_DLOPEN
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
if ${CONFIG_SHELL} ./libtool --features | grep "enable static" >/dev/null; then
  STATIC="-static"
else
  STATIC=
fi
AC_SUBST(STATIC)

dnl --------------------------------------------------------------
dnl Checks for header files.
dnl --------------------------------------------------------------
AC_HEADER_STDC
AC_CHECK_HEADER(unistd.h)
AC_CHECK_HEADER(string.h)

dnl --------------------------------------------------------------
dnl Checks for typedefs, structures, and compiler characteristics.
dnl --------------------------------------------------------------
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
dnl AC_CHECK_TYPES([ptrdiff_t])
AC_C_BIGENDIAN

dnl ---------------------------------------------
dnl threads
dnl ---------------------------------------------
case $host in
  *-*-freebsd*)
    THREAD_LIBS="-L/usr/local/lib -pthread"
    THREAD_CFLAGS="-I/usr/local/include -D_THREAD_SAFE"
    CFLAGS="$THREAD_CFLAGS $CFLAGS"
    ;;
  *mingw32* | *cygwin*)
    ;;
  *)
    AC_CHECK_LIB(pthread, pthread_create,
	[THREAD_LIBS="-lpthread"],
	AC_MSG_ERROR(pthread needed))
    ;;
esac
AC_SUBST(THREAD_LIBS)
AC_SUBST(THREAD_CFLAGS)

dnl ---------------------------------------------
dnl dynamic linker
dnl ---------------------------------------------
case $host in
  *mingw32*)
    CFLAGS="-idirafter \$(top_srcdir)/msvc/include $CFLAGS"
    LDFLAGS="-no-undefined $LDFLAGS"
    ;;
  *cygwin*)
    LDFLAGS="-no-undefined $LDFLAGS"
    ;;
  *)
    AC_CHECK_LIB(c, dlopen,
	         DYNAMIC_LD_LIBS="",
	         AC_CHECK_LIB(dl, dlopen,
             	              DYNAMIC_LD_LIBS="-ldl",
	                      AC_MSG_ERROR(dynamic linker needed)))
    AC_SUBST(DYNAMIC_LD_LIBS)
    ;;
esac

dnl ---------------------------------------------
dnl cflags
dnl ---------------------------------------------
dnl Common cflags for all platforms
CFLAGS="-O3 -Wall -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE $CFLAGS"
DEBUG_CFLAGS="-g -DDEBUG $CFLAGS"

AC_SUBST(DEBUG_CFLAGS)

dnl ---------------------------------------------
dnl Get where .m4 should be installed.
dnl ---------------------------------------------
case "`id`" in
  uid=0\(* )
    AC_MSG_CHECKING(for aclocal directory)
    if (aclocal --version) < /dev/null > /dev/null 2>&1; then
      ACLOCAL_DIR="`eval $ACLOCAL --print-ac-dir`"
      AC_MSG_RESULT($ACLOCAL_DIR)
    else
      ACLOCAL_DIR="/usr/local/share/aclocal"
      AC_MSG_RESULT(none - will be installed in $ACLOCAL_DIR)
    fi
    escapedprefix="`echo $prefix | sed -e 's/\\//\\\\\//g'`"
    ACLOCAL_DIR="`echo $ACLOCAL_DIR|sed -e 's/^'$escapedprefix/'\${prefix}'/`"
    AC_SUBST(ACLOCAL_DIR)
    ;;
esac
AM_CONDITIONAL(INSTALL_M4, test x"$ACLOCAL_DIR" != "x")

dnl ---------------------------------------------
dnl Check for doxygen (dynamic documentation generator)
dnl ---------------------------------------------
AC_CHECK_PROG(DOXYGEN, doxygen, doxygen, no)

dnl ---------------------------------------------
dnl Some include paths ( !!! DO NOT REMOVE !!! )
dnl ---------------------------------------------
INCLUDES='-I$(top_srcdir) $(DVDNAV_CFLAGS)'
AC_SUBST(INCLUDES)

dnl ---------------------------------------------
dnl It seems automake 1.5 don't take care about this script
dnl ---------------------------------------------
if test ! -z $am_depcomp; then
	DEPCOMP="depcomp"
fi
AC_SUBST(DEPCOMP)

dnl ---------------------------------------------
dnl Output configuration files
dnl ---------------------------------------------
AC_OUTPUT([
Makefile 
src/Makefile
src/dvdread/Makefile
src/vm/Makefile
misc/Makefile
misc/dvdnav-config
misc/libdvdnav.spec
misc/relchk.sh
m4/Makefile
doc/Makefile
examples/Makefile
])
