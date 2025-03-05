/*
 * iduncore.h - Idun cartridge emulation.
 *
 * Written by
 *  Brian Holdsworth <brian.holdsworth@gmail.com>
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

#ifndef VICE_IDUNCORE_H
#define VICE_IDUNCORE_H

#include <stdlib.h>
#include "snapshot.h"
#include "types.h"
#include "vicesocket.h"

typedef struct io_iduncart_s {
    const char *host;
    vice_network_socket_t *socket;
    uint8_t *pfirst, *plast;
} io_iduncart_t;

extern void iduncart_io_reset(io_iduncart_t *context);
extern io_iduncart_t *iduncart_init(const char *device);
extern void iduncart_io_destroy(io_iduncart_t *context);

extern void iduncart_io_store_data(io_iduncart_t *context, uint8_t data);
extern uint8_t iduncart_io_read(io_iduncart_t *context, uint16_t addr);

extern int iduncart_io_dump(io_iduncart_t *context);

#endif
