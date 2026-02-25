/*
 * idunsocket.c - Idun cartridge socket abstraction for unix domain and UDP.
 *
 * Written by
 *  Brian Holdsworth <brian.holdsworth@gmail.com>
 *  Deniss Vorona <dwwretro@gmail.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#ifdef HAVE_NETWORK

#include <errno.h>
#include <string.h>

#include "socketdrv/socketimpl.h"

#ifdef WINDOWS_COMPILE
# undef INVALID_SOCKET
# define INVALID_SOCKET -1
#endif

#include "archdep.h"
#include "lib.h"
#include "log.h"
#include "signals.h"
#include "idunsocket.h"

/* ---- Idun socket types ---- */

union idun_socket_addresses_u {
    struct sockaddr generic;
#ifdef HAVE_UNIX_DOMAIN_SOCKETS
    struct sockaddr_un local;
#endif
    struct sockaddr_in ipv4;
};

/*! \internal \brief Simplified variant of vice_network_socket_s for idun datagram usage */
struct idun_socket_s {
    SOCKET sockfd;
};

/* ---- Open functions ---- */

/*! \internal \brief Generate a unix domain socket address

  Initialises a socket address with a unix domain socket address

  \param path
     Path of a special file which represents the Unix domain socket.

  \return
     NULL on error;
     else, a pointer to idun_socket_t on success.

  \remark
     On platforms which do not support unix domain sockets, this function
     returns NULL as error.
*/
idun_socket_t *idun_socket_open_unix(const char *path)
{
#ifdef HAVE_UNIX_DOMAIN_SOCKETS
    union idun_socket_addresses_u addr;
    int sockfd;
    int err;

    if (path == NULL || path[0] == 0 || strlen(path) >= sizeof(addr.local.sun_path)) {
        log_error(LOG_DEFAULT,
                  "Unix domain socket path invalid or too long: '%s'",
                  path ? path : "(null)");
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.local.sun_family = AF_UNIX;
    strcpy(addr.local.sun_path, path);

    sockfd = (int)socket(PF_UNIX, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        err = errno;
        log_error(LOG_DEFAULT,
                  "idun_socket_open_unix(): socket() failed: %s", strerror(err));
        return NULL;
    }

    archdep_remove(path);

    if (bind(sockfd, &addr.generic, sizeof(addr.local)) < 0) {
        err = errno;
        log_error(LOG_DEFAULT,
                  "idun_socket_open_unix(): bind() failed: %s", strerror(err));
        closesocket(sockfd);
        return NULL;
    }

    {
        idun_socket_t *sock = lib_calloc(1, sizeof(*sock));
        sock->sockfd = sockfd;
        return sock;
    }
#else
    log_message(LOG_DEFAULT,
                "Unix domain sockets are not supported in this installation of VICE!\n");
    return NULL;
#endif
}

/*! \brief Open a UDP socket bound to a local port for NMI datagram reception.

  Creates and binds a UDP (SOCK_DGRAM) socket to INADDR_ANY on the given port.
  Used as the NMI socket on platforms that lack Unix domain socket support
  (e.g. Windows), where the Idun cartridge host sends interrupt messages as
  UDP datagrams.  The socket is polled periodically via the VICE alarm system
  (see iduncore.c).

  \param port
     The local UDP port number to bind (host byte order). 

  \return
     Pointer to a newly allocated idun_socket_t on success, or NULL if
     socket() or bind() fails (error is logged via log_error).
*/
idun_socket_t *idun_socket_open_udp(unsigned short port)
{
    struct sockaddr_in addr;
    int sockfd;
    int err;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    sockfd = (int)socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        err = errno;
        log_error(LOG_DEFAULT,
                  "idun_socket_open_udp(): socket() failed: %s", strerror(err));
        return NULL;
    }

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        err = errno;
        log_error(LOG_DEFAULT,
                  "idun_socket_open_udp(): bind() failed: %s", strerror(err));
        closesocket(sockfd);
        return NULL;
    }

    {
        idun_socket_t *sock = lib_calloc(1, sizeof(*sock));
        sock->sockfd = sockfd;
        return sock;
    }
}

/* ---- Common socket operations ---- */

/*! \brief Close a socket - idun_socket_t version

  Copy of vice_socket_poll from socket.c using idun_socket_t instead.

  \param sockfd
     The socket to be closed

  \return
     0 on success, else an error occurred.
*/
int idun_socket_close(idun_socket_t *sockfd)
{
    int error = -1;
    if (sockfd) {
        error = closesocket(sockfd->sockfd);
        lib_free(sockfd);
    }
    return error;
}

/*! \brief Check socket has incoming data to receive - idun_socket_t version

  Copy of vice_socket_poll from socket.c using idun_socket_t instead.

  \param readsockfd
     The connected socket to test for data

  \return
     1 if the specified socket has data; 0 if it does not contain
     any data, and -1 in case of an error.
*/
int idun_socket_poll(idun_socket_t *sockfd)
{
    TIMEVAL timeout = { 0, 0 };
    fd_set fdsockset;

    FD_ZERO(&fdsockset);
    FD_SET(sockfd->sockfd, &fdsockset);

    return select(sockfd->sockfd + 1, &fdsockset, NULL, NULL, &timeout);
}

/*! \brief Receive data from a datagram socket

  This function receives incoming data from a datagram socket.

  \param sockfd
     The connected socket to receive from

  \param buffer
     Pointer to the buffer which will hold the received data

  \param buffer_length
     The length of the buffer pointed to by buffer. This
     indicates the maximum number of bytes to receive.

  \param flags
     Flags for the socket. These flags are architecture dependent.

  \return
     the number of bytes received. This can be less than
     buffer_length.

     In case of an error, -1 is returned.
*/
ssize_t idun_socket_recvfrom(idun_socket_t *sockfd, void *buffer, size_t buffer_length, int flags)
{
    ssize_t ret;

    signals_pipe_set();
    ret = recvfrom(sockfd->sockfd, buffer, buffer_length, flags, NULL, NULL);
    signals_pipe_unset();

    return ret;
}

#endif /* HAVE_NETWORK */
