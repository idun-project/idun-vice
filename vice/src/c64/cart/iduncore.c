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
#include "machine.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API

#include "iduncore.h"
#include "lib.h"
#include "monitor.h"
#include "log.h"
#include "alarm.h"
#include "maincpu.h"
#include "interrupt.h"

#include <string.h>
#include <ctype.h>
#include <assert.h>

#ifndef MSG_WAITALL
#  ifdef WINDOWS_COMPILE
#    define MSG_WAITALL 0x8
#  else
#    define MSG_WAITALL 0x100
#  endif
#endif

/* This module is currently used in the following emulated hardware:
   - C64/C128 Idun cartridge
*/

#define IDUN_DEBUG(_x) log_debug _x
//#define IDUN_DEBUG(_x)
//#define IDUN_VERBOSE_DEBUG(_x) log_debug _x
#define IDUN_VERBOSE_DEBUG(_x)

// This defines come from the `idunio` service
#define MAX_PIPE_MSG_BYTES 293
#define SYSTEM_BLOCK 255
#define PAGES_PER_BLOCK 64
#define CMD_LOAD_BLOCK 0xfc
#define CMD_UPDATE_PAGE 0xfd
#define CMD_FREEMAP 0xf7

// For nmi local unix domain datagram socket path (uses XDG_RUNTIME_DIR at runtime)
#define NMI_XDG_ENV "XDG_RUNTIME_DIR"
#define NMI_UNIX_DOMAIN_PATH_FMT "%s/cartmon"
// For nmi udp socket
#define NMI_UDP_PORT 64128

// We use vice alarms to poll for nmi messages
#define NMIMSG_POLL_INTERVAL 128    //128 microsecs

/* ---------------------------------------------------------------------------------------------------- */
static uint8_t recvBuf[MAX_PIPE_MSG_BYTES];
static uint8_t blockMem[16384];
static uint8_t boot_rom_bkup[496];
static io_iduncart_t iduncart = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, SYSTEM_BLOCK, 0, 0, blockMem};
static unsigned int nmi_int_num = 0;

static void nmimsg_alarm_handler(void *data);
#ifdef HAVE_UNIX_DOMAIN_SOCKETS
static void nmimsg_alarm_handler_unix_domain(CLOCK offset, void *data);
#endif
static void nmimsg_alarm_handler_udp(CLOCK offset, void *data);
struct alarm_s *nmimsg_alarm_unix_domain = NULL;
struct alarm_s *nmimsg_alarm_udp = NULL;

/* ---------------------------------------------------------------------------------------------------- */
void nmimsg_alarm_handler(void *data)
{
    if (data) {
        idun_socket_t *s = (idun_socket_t *)data;
        if (idun_socket_poll(s)) {
            uint8_t buffer[496];

            int numBytes = idun_socket_recvfrom(s, buffer, 2, 0);
            if (numBytes != 2) {
                log_error(LOG_DEFAULT, "NMI request sync.");
                return;
            }

            int16_t msgBytes = (int16_t)((uint16_t)buffer[1] | ((uint16_t)buffer[0] << 8));
            // check if the header is a negative value
            if (msgBytes < 0) {
                if (msgBytes==-1 || msgBytes==-2 || msgBytes==-1000) {
                    // That's a reboot message to the cartridge monitor
                    // For emulation, we'll treat it as a generic reset.
                    machine_trigger_reset(MACHINE_RESET_MODE_POWER_CYCLE);
                }
                return;
            }
            // Otherwise, this is an nmi request and the header gives
            // the size.
            assert(msgBytes <= sizeof buffer);
            
            numBytes = idun_socket_recvfrom(s, buffer, msgBytes, 0);
            if (numBytes != msgBytes) {
                log_error(LOG_DEFAULT, "NMI request size.");
            }

            IDUN_DEBUG((LOG_DEFAULT, "NMI request: %d bytes.", msgBytes));
            if (iduncart.rombase && nmi_int_num) {
                memcpy(iduncart.rombase, buffer, msgBytes);
                maincpu_set_nmi(nmi_int_num, IK_NMI);
            } else {
                log_error(LOG_DEFAULT, "NMI load fail. Is idun-cart attached?");
            }
        }
    }
}

/* ---------------------------------------------------------------------------------------------------- */
#ifdef HAVE_UNIX_DOMAIN_SOCKETS
void nmimsg_alarm_handler_unix_domain(CLOCK offset, void *data)
{
    alarm_unset(nmimsg_alarm_unix_domain);
    nmimsg_alarm_handler(data);
    alarm_set(nmimsg_alarm_unix_domain, maincpu_clk + NMIMSG_POLL_INTERVAL);
}
#endif

/* ---------------------------------------------------------------------------------------------------- */
void nmimsg_alarm_handler_udp(CLOCK offset, void *data)
{
    alarm_unset(nmimsg_alarm_udp);
    nmimsg_alarm_handler(data);
    alarm_set(nmimsg_alarm_udp, maincpu_clk + NMIMSG_POLL_INTERVAL);
}

/* ---------------------------------------------------------------------------------------------------- */
static void iduncart_eram_read()
{
    size_t offset = 0;
    uint8_t pages = 0;
    uint8_t talk[] = {0x40, 0x7f};
    uint8_t untalk[] = {0x5f};

    // TALK #0
    ssize_t n = vice_network_send(iduncart.socket, &talk, 2, 0);
    if (n < 0) { log_error(LOG_DEFAULT, "eram_read: TALK send failed: %d", vice_network_get_errorcode()); return; }

    // First byte is num pages
    while (vice_network_select_poll_one(iduncart.socket) == 0);
    n = vice_network_receive(iduncart.socket, &pages, 1, 0);
    if (n < 0) { log_error(LOG_DEFAULT, "eram_read: pages recv failed: %d", vice_network_get_errorcode()); return; }
    if (pages >= PAGES_PER_BLOCK) { log_error(LOG_DEFAULT, "eram_read: invalid page count %d", pages); return; }

    log_debug(LOG_DEFAULT, "Read %d pages for block %d", pages, iduncart.m_block);

    while (pages > 0) {
        n = vice_network_receive(iduncart.socket, &blockMem[offset], 256, MSG_WAITALL);
        if (n != 256) {
			log_error(LOG_DEFAULT, "eram_read: short page recv %zd", n);
			return;
		}

        uint8_t *a = blockMem;
        log_debug(LOG_DEFAULT, "page #%d", (int)offset/256);
        for (uint16_t i=0;i < 16; i++) {
            uint16_t b = offset + (16 * i);
            log_debug(LOG_DEFAULT, "%02x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                (unsigned int)i*16, a[b],a[b+1],a[b+2],a[b+3],a[b+4],a[b+5],a[b+6],a[b+7],a[b+8],a[b+9],a[b+10],a[b+11],a[b+12],a[b+13],a[b+14],a[b+15]);
        }

        offset += 256;

        // UNTALK
        n = vice_network_send(iduncart.socket, &untalk, 1, 0);
        if (n < 0) { log_error(LOG_DEFAULT, "eram_read: UNTALK send failed: %d", vice_network_get_errorcode()); return; }
        if (--pages == 0) return;

        // TALK #0
        n = vice_network_send(iduncart.socket, &talk, 2, 0);
        if (n < 0) { log_error(LOG_DEFAULT, "eram_read: TALK send failed: %d", vice_network_get_errorcode()); return; }
    }
    // UNTALK
    n = vice_network_send(iduncart.socket, &untalk, 1, 0);
    if (n < 0) { log_error(LOG_DEFAULT, "eram_read: final UNTALK send failed: %d", vice_network_get_errorcode()); }
}

static void iduncart_eram_loadblock()
{
    uint8_t cmd[] = {0x20, 0x7f, CMD_LOAD_BLOCK, iduncart.m_block};

    ssize_t n = vice_network_send(iduncart.socket, &cmd, 4, 0);
    if (n < 0) {
        log_error(LOG_DEFAULT, "eram_loadblock: send failed: %d", vice_network_get_errorcode());
    } else {
        iduncart_eram_read();
        log_debug(LOG_DEFAULT, "ERAM block %d loaded", iduncart.m_block);
    }
}

static void iduncart_eram_freemap()
{
    uint8_t cmd[] = {0x20, 0x7f, CMD_FREEMAP, iduncart.m_block};

    ssize_t n = vice_network_send(iduncart.socket, &cmd, 4, 0);
    if (n < 0) {
        log_error(LOG_DEFAULT, "eram_freemap: send failed: %d", vice_network_get_errorcode());
    } else {
        iduncart_eram_read();
        log_debug(LOG_DEFAULT, "ERAM system block re-loaded");
    }
}

static void iduncart_eram_writeback()
{
    uint8_t cmd[] = {0x20, 0x7f, CMD_UPDATE_PAGE, 0};
    int8_t c = 63;

    if ((iduncart.dirty0 | iduncart.dirty1)==0) return;
    log_debug(LOG_DEFAULT, "Update dirty pages: 0x%08x%08x", iduncart.dirty1, iduncart.dirty0);

    while (c >= 0) {
        uint32_t pg = (c < 32) ? 1<<c : 1<<(c-32);
        uint32_t cmp = (c < 32) ? iduncart.dirty0 : iduncart.dirty1;
        if (cmp & pg) {
            uint16_t offset = c * 256;
            cmd[3] = c;

            ssize_t n = vice_network_send(iduncart.socket, &cmd, 4, 0);
            if (n < 0) { log_error(LOG_DEFAULT, "eram_writeback: cmd send failed: %d", vice_network_get_errorcode()); return; }

            n = vice_network_send(iduncart.socket, &iduncart.block_data[offset], 256, 0);
            if (n < 0) { log_error(LOG_DEFAULT, "eram_writeback: data send failed: %d", vice_network_get_errorcode()); return; }

            uint8_t *a = iduncart.block_data;
            log_debug(LOG_DEFAULT, "UPDATE #%d", c);
            for (uint16_t i=0;i < 16; i++) {
                uint16_t b = offset + (16 * i);
                log_debug(LOG_DEFAULT, "%02x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                    (unsigned int)i*16, a[b],a[b+1],a[b+2],a[b+3],a[b+4],a[b+5],a[b+6],a[b+7],a[b+8],a[b+9],a[b+10],a[b+11],a[b+12],a[b+13],a[b+14],a[b+15]);
            }
        }
        c--;
    }
    iduncart.dirty0 = iduncart.dirty1 = 0;
}
/* ---------------------------------------------------------------------------------------------------- */
void iduncart_io_reset(io_iduncart_t *context)
{
    log_message(LOG_DEFAULT, "Idun cart reset");

    if (context) {
        iduncart_io_destroy(context);
        if (context->host) {
            iduncart_init(context->host);
        }
        if (iduncart.rombase && nmi_int_num) {
            memcpy(iduncart.rombase, boot_rom_bkup, sizeof boot_rom_bkup);
            maincpu_set_nmi(nmi_int_num, IK_NONE);
        }
    }
}

io_iduncart_t *iduncart_init(const char *host)
{
    log_message(LOG_DEFAULT, "Idun connect: %s", host);

    iduncart.host = host;
    iduncart.pfirst = iduncart.plast = recvBuf;
    nmi_int_num = interrupt_cpu_status_int_new(maincpu_int_status, "IdunCartridge");

    /* parse the address */
    vice_network_socket_address_t *ad = NULL;
    ad = vice_network_address_generate(host, 25232);
    if (!ad) {
        log_error(LOG_DEFAULT, "Bad idunhost. Should be ipaddr:port, but is '%s'.", host);
    }
    else {
        /* connect socket */
        iduncart.socket = vice_network_client(ad);
        if (!iduncart.socket) {
            log_error(LOG_DEFAULT, "Can't open connection.");
        }
        /* init the block cache by loading SYSTEM_BLOCK */
        iduncart.m_block = SYSTEM_BLOCK;
        iduncart_eram_loadblock();
        iduncart.m_page = 0x40;
    }

    if (ad) {
        vice_network_address_close(ad);
    }

    /* determine if idunhost is local (127.0.0.1 or localhost) */
    do {
        const char *colon = strchr(host, ':');
        size_t hostlen = colon ? (size_t)(colon - host) : strlen(host);
        if (!((hostlen == 9 && strncmp(host, "127.0.0.1", 9) == 0) ||
              (hostlen == 9 && strncmp(host, "localhost", 9) == 0))) {
            break; /* remote host — skip to UDP below */
        }

#ifdef HAVE_UNIX_DOMAIN_SOCKETS
        /* setup unix datagram socket for nmi messages - will work only when running vice on the same pi as idun */
        char nmi_path[256];
        const char *xdg = getenv(NMI_XDG_ENV);
        snprintf(nmi_path, sizeof(nmi_path), NMI_UNIX_DOMAIN_PATH_FMT, xdg ? xdg : "/tmp");
        iduncart.nmisock_unix_domain = idun_socket_open_unix(nmi_path);
        if (!iduncart.nmisock_unix_domain) {
            log_error(LOG_DEFAULT, "Can't open unix domain socket for nmi.");
            break;
        }

        /* alarm used to poll unix domain socket for nmi messages */
        nmimsg_alarm_unix_domain = alarm_new(maincpu_alarm_context, "NmiMessageAlarmUnixDomain",
                                             nmimsg_alarm_handler_unix_domain, iduncart.nmisock_unix_domain);
        alarm_set(nmimsg_alarm_unix_domain, maincpu_clk + NMIMSG_POLL_INTERVAL);

        log_message(LOG_DEFAULT, "Idun unix domain socket bound");
#else
        log_error(LOG_DEFAULT, "Unix domain sockets not available; cannot open NMI socket for localhost.");
#endif
        return &iduncart;
    } while(0);

    /* UDP for NMI messages (remote host) */
    do {
        if (iduncart.nmisock_unix_domain) {
            log_message(LOG_DEFAULT, "Not opening idun udp - unix domain socket open for local iduncart");
        }

        iduncart.nmisock_udp = idun_socket_open_udp(NMI_UDP_PORT);
        if (!iduncart.nmisock_udp) {
            log_error(LOG_DEFAULT, "Can't open udp socket for nmi.");
            break;
        }

        /* alarm used to poll udp socket for nmi messages */
        nmimsg_alarm_udp = alarm_new(maincpu_alarm_context, "NmiMessageAlarmUdp",
                                     nmimsg_alarm_handler_udp, iduncart.nmisock_udp);
        alarm_set(nmimsg_alarm_udp, maincpu_clk + NMIMSG_POLL_INTERVAL);

        log_message(LOG_DEFAULT, "Idun udp socket bound");
    } while(0);

    return &iduncart;
}

void iduncart_io_destroy(io_iduncart_t *context)
{
    log_message(LOG_DEFAULT, "Idun disconnect");

    if (nmimsg_alarm_unix_domain) alarm_destroy(nmimsg_alarm_unix_domain);
    if (nmimsg_alarm_udp) alarm_destroy(nmimsg_alarm_udp);

    if (!context) return;
    
    do {
        if (!context->socket) {
            log_error(LOG_DEFAULT, "Attempt to close non-open idunio");
            break;
        }
        vice_network_socket_close(context->socket);
        context->socket = NULL;
    } while (0);

#ifdef HAVE_UNIX_DOMAIN_SOCKETS
    if (context->nmisock_unix_domain) {
        idun_socket_close(context->nmisock_unix_domain);
        context->nmisock_unix_domain = NULL;
    }
#endif
    if (context->nmisock_udp) {
        idun_socket_close(context->nmisock_udp);
        context->nmisock_udp = NULL;
    }
}

/* ---------------------------------------------------------------------------------------------------- */

void iduncart_io_store_data(io_iduncart_t *context, uint8_t data)
{
    if (!context->socket) {
        log_error(LOG_DEFAULT, "Attempt to write to non-open socket");
        return;
    }

    IDUN_VERBOSE_DEBUG((LOG_DEFAULT, "Output 0x%02x '%c'.", data, isgraph(data) ? data : '.'));

    ssize_t n = vice_network_send(context->socket, &data, 1, 0);
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
        log_debug(LOG_DEFAULT, "Dirty pages=0x%08x%08x", iduncart.dirty1, iduncart.dirty0);
        iduncart_eram_writeback();
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
        if (byte & 0x80) {
            uint8_t sh = byte & 0x3f;
            if (sh < 32)
                context->dirty0 |= 1<<sh;
            else
                context->dirty1 |= 1<<(sh-32);
        }
        context->m_page = byte | 0x40;
    }
}

void iduncart_soft_switch(io_iduncart_t *context, uint16_t addr, uint8_t byte)
{
    assert(context!=NULL);

    maincpu_set_nmi(nmi_int_num, IK_NONE);
    if (addr == 0x7f) {
        IDUN_DEBUG((LOG_DEFAULT, "Soft-switch enable exrom"));
        if (machine_class & VICE_MACHINE_C64SC)
            cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ | CMODE_PHI2_RAM);
    } else if (addr == 0x7e) {
        IDUN_DEBUG((LOG_DEFAULT, "Soft-switch disable exrom"));
        if (machine_class & VICE_MACHINE_C64SC)
            cart_config_changed_slotmain(CMODE_RAM, CMODE_RAM, CMODE_READ | CMODE_PHI2_RAM);
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
        log_debug(LOG_DEFAULT, "eram peek(%d)", addr);
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

        IDUN_VERBOSE_DEBUG((LOG_DEFAULT, "Idun($de00)=%x", b));
        
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
            ssize_t nr = vice_network_receive(context->socket, recvBuf, MAX_PIPE_MSG_BYTES, 0);
            c = (nr > 0) ? (size_t)nr : 0;
            context->plast = context->pfirst + c;
        }
#pragma GCC diagnostic pop
        
        IDUN_VERBOSE_DEBUG((LOG_DEFAULT, "Idun($de01)=%x", (unsigned int)c));

        return (c < 256)? c : 255;
    }
}

int iduncart_io_dump()
{
    mon_out("4096K avail bytes\n");
    return 0;
}

void iduncart_set_rombase(uint8_t* base)
{
    iduncart.rombase = base;
    memcpy(boot_rom_bkup, base, sizeof boot_rom_bkup);
}
