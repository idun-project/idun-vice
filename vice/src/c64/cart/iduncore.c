/*
 * iduncore.c - Idun cartridge emulation.
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

#include "vice.h"

#include "iduncore.h"
#include "lib.h"
#include "monitor.h"
#include "log.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>

/* This module is currently used in the following emulated hardware:
   - C64/C128 Idun cartridge
*/

//#define IDUN_VERBOSE_DEBUG(_x) log_debug _x
#define IDUN_VERBOSE_DEBUG(_x)

/* This define is from the `idunio` service and is the max. sized buffer
   that it will send.
*/
#define MAX_PIPE_MSG_BYTES 293
#define SYSTEM_BLOCK 255
#define CMD_LOAD_BLOCK 0xfc
#define CMD_UPDATE_PAGE 0xfd
#define CMD_FREEMAP 0xf7

/* ---------------------------------------------------------------------------------------------------- */
static uint8_t recvBuf[MAX_PIPE_MSG_BYTES];
static uint8_t blockMem[16384];
static io_iduncart_t iduncart = {NULL, NULL, NULL, NULL, 0, SYSTEM_BLOCK, blockMem};

static void iduncart_eram_read()
{
    size_t offset = 0;
    uint8_t pages = 0;
    uint8_t talk[] = {0x40, 0x7f};
    uint8_t untalk[] = {0x5f};

    // TALK #0
    int n = vice_network_send(iduncart.socket, &talk, 2, 0);
    assert(n==2);
    // First byte is num pages
    while (vice_network_select_poll_one(iduncart.socket) == 0);
    vice_network_receive(iduncart.socket, &pages, 1, 0);
    while (pages > 0) {
        size_t n = vice_network_receive(iduncart.socket, &blockMem[offset], 256, 0);
        assert(n == 256);
        offset += 256;
        // UNTALK
        vice_network_send(iduncart.socket, &untalk, 1, 0);
        if (--pages == 0) return;
        // TALK #0
        vice_network_send(iduncart.socket, &talk, 2, 0);
    }
    // UNTALK
    vice_network_send(iduncart.socket, &untalk, 1, 0);
}

static void iduncart_eram_loadblock()
{
    uint8_t cmd[] = {0x20, 0x7f, CMD_LOAD_BLOCK, iduncart.m_block};

    int n = vice_network_send(iduncart.socket, &cmd, 4, 0);
    if (n < 0) {
        log_error(LOG_DEFAULT, "Idun socket write failed: %d.", vice_network_get_errorcode());
    } else {
        iduncart_eram_read();
        log_message(LOG_DEFAULT, "ERAM block %d loaded", iduncart.m_block);
    }
}

static void iduncart_eram_freemap()
{
    uint8_t cmd[] = {0x20, 0x7f, CMD_FREEMAP, iduncart.m_block};

    int n = vice_network_send(iduncart.socket, &cmd, 4, 0);
    if (n < 0) {
        log_error(LOG_DEFAULT, "Idun socket write failed: %d.", vice_network_get_errorcode());
    } else {
        iduncart_eram_read();
        log_message(LOG_DEFAULT, "ERAM system block re-loaded");
    }
}

/* ---------------------------------------------------------------------------------------------------- */
void iduncart_io_reset(io_iduncart_t *context)
{
    log_message(LOG_DEFAULT, "Idun cart reset");

    iduncart_io_destroy(context);
    iduncart_init(context->host);
}

io_iduncart_t *iduncart_init(const char *host)
{
    log_message(LOG_DEFAULT, "Idun connect: %s", host);

    iduncart.host = host;
    iduncart.pfirst = iduncart.plast = recvBuf;

    /* parse the address */
    vice_network_socket_address_t *ad = NULL;
    ad = vice_network_address_generate(host, 0);
    if (!ad) {
        log_error(LOG_DEFAULT, "Bad idunhost. Should be ipaddr:port, but is '%s'.", host);
    }
    else {
        /* connect socket */
        iduncart.socket = vice_network_client(ad);
        if (!iduncart.socket) {
            log_error(LOG_DEFAULT, "Cant open connection.");
        }
        /* init the block cache by loading SYSTEM_BLOCK */
        iduncart.m_block = SYSTEM_BLOCK;
        iduncart_eram_loadblock();
        iduncart.m_page = 0x40;
    }

    if (ad) {
        vice_network_address_close(ad);
    }

    return &iduncart;
}

void iduncart_io_destroy(io_iduncart_t *context)
{
    log_message(LOG_DEFAULT, "Idun disconnect");

    do {
        if (!context->socket) {
            log_error(LOG_DEFAULT, "Attempt to close non-open socket");
            break;
        }    

        vice_network_socket_close(context->socket);
        context->socket = NULL;
    } while (0);
}

/* ---------------------------------------------------------------------------------------------------- */

void iduncart_io_store_data(io_iduncart_t *context, uint8_t data)
{
    if (!context->socket) {
        log_error(LOG_DEFAULT, "Attempt to write to non-open socket");
        return;
    }

    IDUN_VERBOSE_DEBUG(("Output 0x%02x '%c'.", data, isgraph(data) ? data : '.'));

    int n = vice_network_send(context->socket, &data, 1, 0);
    if (n < 0) {
        log_error(LOG_DEFAULT, "Error writing: %d.", vice_network_get_errorcode());
        vice_network_socket_close(context->socket);
        context->socket = NULL;
    }
}

uint8_t iduncart_reg_read(io_iduncart_t *context, uint16_t addr) {
    assert(context!=NULL);
    assert(addr==0xfe);

    return context->m_page;
}

void iduncart_reg_write(io_iduncart_t *context, uint16_t addr, uint8_t byte)
{
    assert(context!=NULL);

    if (addr == 0xff) {
        if (context->m_block != byte) {
            context->m_block = byte;
            context->m_page = 0;
            iduncart_eram_loadblock();
            context->m_page |= 0x40;
        } else if (byte == SYSTEM_BLOCK) {
            context->m_page = 0;
            iduncart_eram_freemap();
            context->m_page |= 0x40;
        }
    } else if (addr == 0xfe) {
        context->m_page = byte | 0x40;
    }
}

void iduncart_page_store(uint16_t addr, uint8_t byte)
{
    if (iduncart.m_page & 0x80) {
        addr = (iduncart.m_page & 0x3f)*256 + addr;
        iduncart.block_data[addr] = byte;
    }
}

uint8_t iduncart_page_read(uint16_t addr)
{
    if ((iduncart.m_page & 0x80) == 0) {
        addr = (iduncart.m_page & 0x3f)*256 + addr;
        return iduncart.block_data[addr];
    } else {
        return 0xde;
    }
}

uint8_t iduncart_io_read(io_iduncart_t *context, uint16_t ioaddr)
{
    assert(ioaddr <= 0x02); // $de00-$de02 only!

    if (ioaddr == 0x02) {
        // I am an Emulator and I am Ok.
        return 0x9b;        // ~0x64 ;)
    }
    else if (ioaddr == 0x00) {
        // read data byte from $de00
        uint8_t b = 0x42;   // no data; false read flag

        if (context->pfirst < context->plast)
            b = *(++context->pfirst);

        IDUN_VERBOSE_DEBUG(("Idun($de00)=%x", b));
        
        return b;
    }
    else {
        // read bytes available from $de01
        size_t c = context->plast - context->pfirst;

        // simple case
        if (c > 0) 
            return (c < 256)? c : 255;

        // no buffered data available; need to poll socket
#pragma GCC diagnostic ignored "-Warray-bounds"
        if (vice_network_select_poll_one(context->socket) > 0) {
            // socket ready; fetch data to buffer
            context->pfirst = recvBuf-1;
            c = vice_network_receive(context->socket, recvBuf, MAX_PIPE_MSG_BYTES, 0);
            context->plast = context->pfirst + c;
        }
#pragma GCC diagnostic pop
        
        IDUN_VERBOSE_DEBUG(("Idun($de01)=%x", (unsigned int)c));

        return (c < 256)? c : 255;
    }
}

int iduncart_io_dump()
{
    mon_out("4096K avail bytes\n");
    return 0;
}
