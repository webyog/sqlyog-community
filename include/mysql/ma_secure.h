/************************************************************************************
    Copyright (C) 2012 Monty Program AB
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc., 
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA

   Part of this code includes code from the PHP project which
   is freely available from http://www.php.net
*************************************************************************************/
#ifndef _ma_secure_h_
#define _ma_secure_h_

#ifdef HAVE_OPENSSL
#include <mysql.h>
#include <openssl/ssl.h> /* SSL and SSL_CTX */
#include <openssl/err.h> /* error reporting */
#include <openssl/conf.h>

struct MYSQL;

size_t my_ssl_read(Vio *vio, uchar* buf, size_t size);
int my_ssl_close(Vio *vio);
size_t my_ssl_write(Vio *vio, const uchar* buf, size_t size);
SSL *my_ssl_init(MYSQL *mysql);
int my_ssl_connect(SSL *ssl);
int my_ssl_verify_server_cert(SSL *ssl);
int ma_ssl_verify_fingerprint(SSL *ssl);

int my_ssl_start(MYSQL *mysql);
void my_ssl_end(void);

#endif /* HAVE_OPENSSL */
#endif /* _ma_secure_h_ */
