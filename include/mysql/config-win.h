/* Copyright (C) 2000 MySQL AB & MySQL Finland AB & TCX DataKonsult AB
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA */

/* Defines for Win32 to make it compatible for MySQL */

#ifndef _config_win_h_
#define _config_win_h_

#include <sys/locking.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <math.h>			/* Because of rint() */
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#include <sys/stat.h>
#include <process.h>

#ifndef THREAD
  #define THREAD
#endif

#ifdef _WIN64
#define MACHINE_TYPE	"ia64"		/* Define to machine type name */
#else
#define MACHINE_TYPE	"i32"		/* Define to machine type name */
#endif /* _WIN64 */

/* File and lock constants */
#define O_SHARE         0x1000 		/* Open file in sharing mode */
#ifdef __BORLANDC__
#define	F_RDLCK		LK_NBLCK	/* read lock */
#define	F_WRLCK		LK_NBRLCK	/* write lock */
#define	F_UNLCK		LK_UNLCK	/* remove lock(s) */
#else
#define	F_RDLCK		_LK_NBLCK	/* read lock */
#define	F_WRLCK		_LK_NBRLCK	/* write lock */
#define	F_UNLCK		_LK_UNLCK	/* remove lock(s) */
#endif

#define F_EXCLUSIVE     1		/* We have only exclusive locking */
#define F_TO_EOF        (INT_MAX32/2)   /* size for lock of all file */
#define F_OK		0		/* parameter to access() */

#define S_IROTH		S_IREAD		/* for my_lib */

#ifdef __BORLANDC__
#define FILE_BINARY	O_BINARY	/* my_fopen in binary mode */
#define O_TEMPORARY	0
#define O_SHORT_LIVED	0
#define SH_DENYNO	_SH_DENYNO
#else
#define O_BINARY        _O_BINARY       /* compability with MSDOS */
#define FILE_BINARY     _O_BINARY       /* my_fopen in binary mode */
#define O_TEMPORARY     _O_TEMPORARY
#define O_SHORT_LIVED   _O_SHORT_LIVED
#define SH_DENYNO       _SH_DENYNO
#endif
#define NO_OPEN_3			/* For my_create() */

#define	SIGQUIT		SIGTERM		/* No SIGQUIT */

#undef _REENTRANT			/* Crashes something for win32 */
#undef SAFE_MUTEX			/* Can't be used on windows */

#define LONGLONG_MIN	((__int64) 0x8000000000000000)
#define LONGLONG_MAX	((__int64) 0x7FFFFFFFFFFFFFFF)
#define LL(A)		((__int64) A)

/* Type information */

typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned __int64 ulonglong;	/* Microsofts 64 bit types */
typedef __int64	longlong;
typedef int sigset_t;
#define longlong_defined
/* off_t should not be __int64 because of conflicts in header files;
   Use my_off_t or os_off_t instead */
typedef	long off_t;
typedef __int64 os_off_t;
#ifdef _WIN64
typedef UINT_PTR rf_SetTimer;
#else
typedef uint rf_SetTimer;
#endif

#define Socket_defined
#define my_socket SOCKET
#define bool BOOL
#define SIGPIPE	SIGINT
#define RETQSORTTYPE void
#define QSORT_TYPE_IS_VOID
#define RETSIGTYPE void
#define SOCKET_SIZE_TYPE int
#define my_socket_defined
#define bool_defined
#define byte_defined
#define HUGE_PTR
#define STDCALL __stdcall           /* Used by libmariadb.dll */

#define VOID_SIGHANDLER
#define SIZEOF_CHAR		1
#define SIZEOF_LONG		4
#define SIZEOF_LONG_LONG	8
#define SIZEOF_OFF_T		8
#define HAVE_BROKEN_NETINET_INCLUDES
#ifdef __NT__
#define HAVE_NAMED_PIPE			/* We can only create pipes on NT */
#endif

/* Use all character sets in MySQL */
#define USE_MB 1
#define USE_MB_IDENT 1
#define USE_STRCOLL 1
 
/* Convert some simple functions to Posix */

#define sigset(A,B) signal((A),(B))
#define finite(A) _finite(A)
#define sleep(A)  Sleep((A)*1000)

#ifndef __BORLANDC__
#define access(A,B) _access(A,B)
#endif

#if defined(__cplusplus)

inline double rint(double nr)
{
  double f = floor(nr);
  double c = ceil(nr);
  return (((c-nr) >= (nr-f)) ? f :c);
}

#ifdef _WIN64
#define ulonglong2double(A) ((double) (A))
#define my_off_t2double(A)  ((double) (A))

#else
inline double ulonglong2double(ulonglong value)
{
  longlong nr=(longlong) value;
  if (nr >= 0)
    return (double) nr;
  return (18446744073709551616.0 + (double) nr);
}
#define my_off_t2double(A) ulonglong2double(A)
#endif /* _WIN64 */
#else
#define inline __inline
#endif /* __cplusplus */

#define __attribute(A)

#if SIZEOF_OFF_T > 4
#define lseek(A,B,C) _lseeki64((A),(longlong) (B),(C))
#define tell(A) _telli64(A)
#endif

#define STACK_DIRECTION -1

/* redefine deprecated functions 
#define sprintf sprintf_s
#define strcpy strcpy_s
#define strcat strcat_s
#define fopen fopen_r
#define freopen freopen_r
#define getenv _dupenv_s
*/
#ifdef _WIN32
#include <stdio.h>
#endif

#define HAVE_PERROR
#define HAVE_VFPRINT
#define HAVE_CHSIZE		/* System has chsize() function */
#define HAVE_RENAME		/* Have rename() as function */
#define HAVE_BINARY_STREAMS	/* Have "b" flag in streams */
#define HAVE_LONG_JMP		/* Have long jump function */
#define HAVE_LOCKING		/* have locking() call */
#define HAVE_ERRNO_AS_DEFINE	/* errno is a define */
#define HAVE_STDLIB		/* everything is include in this file */
#define HAVE_MEMCPY
#define HAVE_MEMMOVE
#define HAVE_GETCWD
#define HAVE_TELL
#define HAVE_TZNAME
#define HAVE_PUTENV
#define HAVE_SELECT
#define HAVE_SETLOCALE
#define HAVE_SOCKET		/* Giangi */
#define HAVE_FLOAT_H
#define HAVE_LIMITS_H
#define HAVE_STDDEF_H
#define HAVE_RINT               /* defined in this file */
#define NO_FCNTL_NONBLOCK       /* No FCNTL */
#define HAVE_ALLOCA
#define HAVE_STRPBRK
#define HAVE_STRSTR

#ifdef WIN32
#define HAVE_SNPRINTF		/* Gave link error */
#define snprintf _snprintf
#endif

#ifdef _MSC_VER
#define HAVE_LDIV		/* The optimizer breaks in zortech for ldiv */
#define HAVE_ANSI_INCLUDE
#define HAVE_SYS_UTIME_H
#define HAVE_STRTOUL
#endif
#define my_reinterpret_cast(A) reinterpret_cast <A>
#define my_const_cast(A) const_cast<A>

/* MYSQL OPTIONS */

#ifdef _CUSTOMCONFIG_
#include <custom_conf.h>
#else
#define	DEFAULT_MYSQL_HOME	"c:\\mysql"
#define PACKAGE		 	"mysql"
#define DEFAULT_BASEDIR		"C:\\"
#define SHAREDIR		"share"
#define DEFAULT_CHARSET_HOME	"C:/mysql/"
#endif

/* File name handling */

#define FN_LIBCHAR	'\\'
#define FN_ROOTDIR	"\\"
#define FN_NETWORK_DRIVES	/* Uses \\ to indicate network drives */
#define FN_NO_CASE_SENCE	/* Files are not case-sensitive */
#define MY_NFILE	1024

#define DO_NOT_REMOVE_THREAD_WRAPPERS
#define thread_safe_increment(V,L) InterlockedIncrement((long*) &(V))
/* The following is only used for statistics, so it should be good enough */
#ifdef __NT__  /* This should also work on Win98 but .. */
#define thread_safe_add(V,C,L) InterlockedExchangeAdd((long*) &(V),(C))
#define thread_safe_sub(V,C,L) InterlockedExchangeAdd((long*) &(V),-(long) (C))
#define statistic_add(V,C,L) thread_safe_add((V),(C),(L))
#else
#define thread_safe_add(V,C,L) \
	pthread_mutex_lock((L)); (V)+=(C); pthread_mutex_unlock((L));
#define thread_safe_sub(V,C,L) \
	pthread_mutex_lock((L)); (V)-=(C); pthread_mutex_unlock((L));
#define statistic_add(V,C,L)     (V)+=(C)
#endif
#define statistic_increment(V,L) thread_safe_increment((V),(L))
#define strcasecmp(A,B) _stricmp((A),(B))

#define close(A) _close((A))
#define fdopen(A,B) _fdopen((A),(B))
#define sopen(A,B,C,D) _sopen((A),(B),(C),(D))


#endif
