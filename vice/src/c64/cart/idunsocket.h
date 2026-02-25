/*
 * idunsocket.h - Idun cartridge socket abstraction for unix domain and UDP.
 *
 * Written by
 *  Brian Holdsworth <brian.holdsworth@gmail.com>
 *  Deniss Vorona <arimyr@gmail.com>
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

#ifndef IDUN_SOCKET_H
#define IDUN_SOCKET_H

#include "types.h"

typedef struct idun_socket_s idun_socket_t;

idun_socket_t *idun_socket_open_unix(const char *path);
idun_socket_t *idun_socket_open_udp(unsigned short port);
int idun_socket_close(idun_socket_t *sockfd);
int idun_socket_poll(idun_socket_t *sockfd);
ssize_t idun_socket_recvfrom(idun_socket_t *sockfd, void *buffer, size_t buffer_length, int flags);

#endif /* IDUN_SOCKET_H */