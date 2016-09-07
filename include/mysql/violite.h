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

/*
 * Vio Lite.
 * Purpose: include file for Vio that will work with C and C++
 */

#ifndef vio_violite_h_
#define	vio_violite_h_



#include "my_net.h"			/* needed because of struct in_addr */

#ifdef HAVE_VIO
#include <Vio.h>				/* Full VIO interface */
#else

#ifdef HAVE_OPENSSL
#include <openssl/ssl.h>
#endif

enum enum_vio_io_event
{
  VIO_IO_EVENT_READ,
  VIO_IO_EVENT_WRITE,
  VIO_IO_EVENT_CONNECT
};

/* Simple vio interface in C;  The functions are implemented in violite.c */

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef Vio_defined
#define Vio_defined
struct st_vio;					/* Only C */
typedef struct st_vio Vio;
#endif

#ifndef _WIN32
#define HANDLE void *
#endif

/* vio read-ahead cachine */
#define VIO_CACHE_SIZE 16384
#define VIO_CACHE_MIN_SIZE 2048

enum enum_vio_type { VIO_CLOSED, VIO_TYPE_TCPIP, VIO_TYPE_SOCKET,
		     VIO_TYPE_NAMEDPIPE, VIO_TYPE_SSL};

Vio*		vio_new(my_socket	sd,
			enum enum_vio_type type,
			my_bool		localhost);
#ifdef _WIN32
Vio*		vio_new_win32pipe(HANDLE hPipe);
#endif
void		vio_delete(Vio* vio);
void vio_reset(Vio* vio, enum enum_vio_type type,
               my_socket sd, HANDLE hPipe,
               my_bool localhost);

/*
 * vio_read and vio_write should have the same semantics
 * as read(2) and write(2).
 */
size_t vio_read(Vio* vio, gptr buf,	size_t size);
my_bool vio_read_peek(Vio *vio, size_t *bytes);
size_t vio_write(Vio* vio, const gptr buf, size_t size);
/*
 * Whenever the socket is set to blocking mode or not.
 */
int		vio_blocking(		Vio*		vio,
					my_bool    	onoff,
                                        my_bool         *prevmode);
my_bool		vio_is_blocking(	Vio*		vio);
/*
 * setsockopt TCP_NODELAY at IPPROTO_TCP level, when possible.
 */
  int		vio_fastsend(		Vio*		vio);
/*
 * setsockopt SO_KEEPALIVE at SOL_SOCKET level, when possible.
 */
int		vio_keepalive(		Vio*		vio,
					my_bool		onoff);
/*
 * Whenever we should retry the last read/write operation.
 */
my_bool		vio_should_retry(	Vio*		vio);
/*
 * When the workday is over...
 */
int		vio_close(		Vio*		vio);
/*
 * Short text description of the socket for those, who are curious..
 */
const char*	vio_description(	Vio*		vio);

/* Return the type of the connection */
 enum enum_vio_type vio_type(Vio* vio);

/* set timeout */
void vio_read_timeout(Vio *vio, uint seconds);
void vio_write_timeout(Vio *vio, uint seconds);

/* Return last error number */
int vio_errno(Vio *vio);

/* Get socket number */
my_socket vio_fd(Vio *vio);

/*
 * Remote peer's address and name in text form.
 */
my_bool vio_peer_addr(Vio * vio, char *buf);

/* Remotes in_addr */

void vio_in_addr(Vio *vio, struct in_addr *in);

  /* Return 1 if there is data to be read */
my_bool vio_poll_read(Vio *vio,uint timeout);
int vio_wait_or_timeout(Vio *vio, my_bool is_read, int timeout);


struct st_vio
{
  my_socket sd; /* my_socket - real or imaginary */
  HANDLE hPipe;
  my_bool localhost; /* Are we from localhost? */
  int fcntl_mode; /* Buffered fcntl(sd,F_GETFL) */
  struct sockaddr_in local; /* Local internet address */
  struct sockaddr_in remote; /* Remote internet address */
  struct mysql_async_context *async_context; /* For non-blocking API */
  int write_timeout;
  int read_timeout;
  enum enum_vio_type type; /* Type of connection */
  char desc[30]; /* String description */
#ifdef HAVE_OPENSSL
  SSL *ssl;
#endif
  uchar *cache;       /* read-ahead cache to reduce reads (see CONC-79) */
  uchar *cache_pos;   /* position of read-ahead cached data */
  size_t cache_size;  /* <= VIO_CACHE_SIZE */
};

#ifdef	__cplusplus
}
#endif
#endif /* HAVE_VIO */
#endif /* vio_violite_h_ */
