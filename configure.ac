AC_INIT([multi2sim], [4.0.1])
AC_CONFIG_SRCDIR([src/m2s.c])
AM_INIT_AUTOMAKE([nostdinc])
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h float.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h utime.h])
AC_CHECK_HEADER([elf.h], [AC_DEFINE([HAVE_ELF_H])], [AC_MSG_ERROR([cannot find header elf.h])])



#
# Check for ZLib
#

have_zlib=yes
AC_CHECK_HEADER([zlib.h], [AC_DEFINE([HAVE_ZLIB_H])], [have_zlib=no])
AC_CHECK_LIB([z], [inflate], [], [have_zlib=no])
if test x$have_zlib == xno; then
AC_MSG_ERROR([
	
	The 'zlib' library development package is missing in your system.
	Please install the package and then retry. On Debian-based Linux
	distributions, you can run the following command:

		sudo apt-get install zlib1g-dev
])
fi


#
# Check for GLUT
#

# Check if the user explicitly disabled GLUT
AC_ARG_ENABLE(glut,
	[  --enable-glut={yes|no}	Enable support for GLUT (default=yes)],
	[], [ enable_glut=yes ])

# Check if GLUT is present
if test "$enable_glut" == "no"
then

	have_glut=no
	AC_MSG_WARN([

	The 'freeglut' library development explicitly disabled. Some features in
	Multi2Sim will not be accessible.
])

else

	have_glut=yes
	AC_CHECK_HEADER([GL/glut.h], [], [have_glut=no])
	AC_CHECK_HEADER([GL/gl.h], [], [have_glut=no])
	AC_CHECK_LIB([glut], [glutInit], [], [have_glut=no])
	AC_CHECK_LIB([GL], [glBegin], [], [have_glut=no])
	AC_CHECK_LIB([GLU], [gluOrtho2D], [], [have_glut=no])

	if test x$have_glut == xno; then
		AC_MSG_WARN([
	
	The 'freeglut' library development package is missing in your system.
	Please install the package and then retry. On Debian-based Linux
	distributions, you can run the following command:

		sudo apt-get install freeglut3-dev
])

	fi
fi

# Define macro for Automake and gcc
AM_CONDITIONAL(HAVE_GLUT_H, test x$have_glut == xyes -a x$enable_glut == xyes)
if test x$have_glut = xyes -a x$enable_glut = xyes; then
	AC_DEFINE(HAVE_GLUT, [1])
fi



# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])


# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_ERROR_AT_LINE
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([alarm bzero ftruncate getcwd gettimeofday memset mkdir munmap select socket strcasecmp strchr strdup strncasecmp strtol utime])

AC_CHECK_LIB([m], [sin])
AC_CHECK_LIB([pthread], [pthread_create])




#
# Check for GTK
#

have_gtk=no
have_gtk_test=yes

# Check if the user explicitly disabled GTK
AC_ARG_ENABLE(gtk,
	[  --enable-gtk={yes|no}	Enable support for GTK (default=yes)],
	[], [ enable_gtk=yes ])

# Check if test for GTK is available
ifdef([AM_PATH_GTK_3_0], [AM_PATH_GTK_3_0([3.0.0], have_gtk=yes, )], [have_gtk_test=no] )
if test x$have_gtk_test = xno; then
AC_MSG_WARN([
	
	The 'autoconf' tool could not interpret the macro checking for the
	presence of the GTK development library (libgtk-3-dev) in your system.
	Multi2Sim will be permanently compiled without support for GTK.
	Before releasing a version with this configuration (running 'make dist'),
	please install this package and rerun 'autoconf'. This will guarantee
	that the conditional inclusion of GTK will depend on the user configuration.
])
fi

# Check if GTK is available
if test x$have_gtk = xno; then
AC_MSG_WARN([

	Multi2Sim will be compiled without support for GTK graphical applications.
	Some features are disabled under this configuration, such as the GPU pipeline
	debugger. To enable it, please install the development packages for GTK on
	your system (e.g., package 'libgtk-3-dev' in Debian-based distributions),
	and run './configure' again.
])
fi

# Check if GTK is enabled
if test x$enable_gtk = xno; then
AC_MSG_WARN([

	GTK is present in your system, but you have explicitly disabled support
	for it. The GTK-based subset of Multi2Sim will not be available in the
	generated executable.
])
fi

# Define macro for Automake and gcc
AM_CONDITIONAL(HAVE_GTK, test x$have_gtk = xyes -a x$enable_gtk = xyes)
if test x$have_gtk = xyes -a x$enable_gtk = xyes; then
	AC_DEFINE(HAVE_GTK, [1])
fi



#
# Data directory
#

DATADIR=`eval echo $datadir`
AC_SUBST([DATADIR])
PKGDATADIR="$datadir/multi2sim"
AC_SUBST([PKGDATADIR])




# ---------------------------------------------------------------------
# Debug and profile
# ---------------------------------------------------------------------
debug=false
CFLAGS="-msse2 -O3 -std=gnu99 -DNDEBUG"
AC_ARG_ENABLE(debug,
[  --enable-debug    Turn on debugging],
[
	case "${enableval}" in
	yes)
		debug=true
		
		# Add flag '-Wno-error=deprecated-declarations' only if it is supported.
		# This flag prevents warnings caused by newer GTK versions that would cause with -Werror
		saved_flags="-msse2 -Wall -Werror -DMHANDLE -g -O0 -std=gnu99"
		CFLAGS="-Wno-error=deprecated-declarations"
		AC_MSG_CHECKING([whether CC supports -Wno-error=deprecated-declarations])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
				[ AC_MSG_RESULT([yes]) ]
				[ saved_flags="$saved_flags -Wno-error=deprecated-declarations" ],
				[ AC_MSG_RESULT([no]) ]
			)
		CFLAGS="$saved_flags"
		;;
	no)
		;;
	*)
		AC_MSG_ERROR(bad value ${enableval} for --enable-debug)
		;;
	esac
],
[])
AC_ARG_ENABLE(profile,
[  --enable-profile    Turn on profiling],
[
	case "${enableval}" in
	yes)
		CFLAGS+=" -pg"
		;;
	no)
		;;
	*)
		AC_MSG_ERROR(bad value ${enableval} for --enable-profile)
		;;
	esac
],
[])
AC_ARG_ENABLE(no-strict-aliasing,
[  --enable-no-strict-aliasing    Allow for no strict aliasing],
[
	case "${enableval}" in
	yes)
		CFLAGS+=" -fno-strict-aliasing"
		;;
	no)
		;;
	*)
		AC_MSG_ERROR(bad value ${enableval} for --enable-no-strict-aliasing)
		;;
	esac
],
[])
AC_SUBST([CFLAGS])


#
# Includes
#

M2S_INCLUDES=" \
\
-I\$(top_srcdir)/src \
\
"

AC_SUBST(M2S_INCLUDES)



#
# Makefiles
#

AC_CONFIG_FILES([Makefile
                 src/Makefile

		 src/arch/Makefile
		 src/driver/Makefile
		 src/lib/Makefile
		 src/mem-system/Makefile
		 src/network/Makefile
		 src/visual/Makefile

		 src/arch/common/Makefile

		 src/arch/arm/Makefile
		 src/arch/arm/asm/Makefile
		 src/arch/arm/emu/Makefile
		 src/arch/arm/timing/Makefile

		 src/arch/evergreen/Makefile
		 src/arch/evergreen/asm/Makefile
		 src/arch/evergreen/emu/Makefile
		 src/arch/evergreen/timing/Makefile

		 src/arch/fermi/Makefile
		 src/arch/fermi/asm/Makefile
		 src/arch/fermi/emu/Makefile
		 src/arch/fermi/timing/Makefile

		 src/arch/southern-islands/Makefile
		 src/arch/southern-islands/asm/Makefile
		 src/arch/southern-islands/emu/Makefile
		 src/arch/southern-islands/timing/Makefile

		 src/arch/x86/Makefile
		 src/arch/x86/asm/Makefile
		 src/arch/x86/emu/Makefile
		 src/arch/x86/timing/Makefile

		 src/driver/x86/Makefile
		 src/driver/x86/glut/Makefile

		 src/visual/common/Makefile
		 src/visual/evergreen/Makefile
		 src/visual/memory/Makefile
		 src/visual/southern-islands/Makefile
		 src/visual/x86/Makefile

		 src/lib/esim/Makefile
                 src/lib/mhandle/Makefile
                 src/lib/util/Makefile
		 ])
AC_OUTPUT

