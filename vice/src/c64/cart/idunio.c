/*
 * idunio.c - Idun cartridge emulation for I/O functions.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "iduncore.h"
#include "idunio.h"
#include "export.h"
#include "lib.h"
#include "machine.h"
//#include "maincpu.h"
#include "resources.h"
#include "sid.h"
#include "snapshot.h"
#include "uiapi.h"
#include "util.h"

/*
    Idun Cartridge

    This cartridge is an IO interface to an ARM coprocessor running Linux.

    When inserted into the cart port the cart uses 2 registers for IO at
    $DE00-$DE01. $DE00 is for sending/receiving data, and $DE01 is for 
    polling how many bytes are available to read.

    In addition, there are 2 registers at $DEFE-$DEFF for setting the ERAM
    page & block addresses for ERAM access. This makes selected ERAM visible
    one page at a time in $DF00-$DFFF, when used in combp with the `idunmm`
    interface @see idunmm.c
*/

/* Idun enabled ?? */
static int idunio_enabled = 0;

static int idunio_accessed = 0;

static char *idunio_host = NULL;

/* ---------------------------------------------------------------------*/

/* Some prototypes are needed */
static uint8_t idunio_read(uint16_t addr);
static void idunio_store(uint16_t addr, uint8_t byte);
static int idunio_dump(void);

static io_source_t idunio_device = {
    CARTRIDGE_NAME_IDUNIO,      /* name of the device */
    IO_DETACH_RESOURCE,         /* use resource to detach the device when involved in a read-collision */
    "IDUNIO",                   /* resource to set to '0' */
    0xde00, 0xdeff, 0xff,       /* range for the device, regs: $de00-$deff */
    0,                          /* read validity is determined by the device upon a read */
    idunio_store,               /* store function */
    NULL,                       /* NO poke function */
    idunio_read,                /* read function */
    idunio_read,                /* peek function */
    idunio_dump,                /* device state information dump function */
    CARTRIDGE_IDUNIO,           /* cartridge ID */
    IO_PRIO_NORMAL,             /* normal priority, device read needs to be checked for collisions */
    0                           /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *idunio_list_item = NULL;

static export_resource_t export_res = {
    CARTRIDGE_NAME_IDUNIO, 0, 0, &idunio_device, NULL, CARTRIDGE_IDUNIO
};

/* idunio context */
static io_iduncart_t *idunio_context = NULL;

/* ---------------------------------------------------------------------*/

int idunio_cart_enabled(void)
{
    return idunio_enabled;
}

static int set_idunio_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!idunio_enabled && val) {
        if (export_add(&export_res) < 0) {
            return -1;
        }
        idunio_list_item = io_source_register(&idunio_device);
        idunio_context = iduncart_init(idunio_host);
        idunio_enabled = 1;
    } else if (idunio_enabled && !val) {
        if (idunio_list_item != NULL) {
            export_remove(&export_res);
            io_source_unregister(idunio_list_item);
            idunio_list_item = NULL;
            if (idunio_context) {
                iduncart_io_destroy(idunio_context);
                idunio_context = NULL;
            }
        }
        idunio_enabled = 0;
    }
    return 0;
}

void idunio_reset(void)
{
    if (idunio_context) {
        iduncart_io_reset(idunio_context);
    }
}

int idunio_enable(void)
{
    return resources_set_int("IDUNIO", 1);
}

int idunio_disable(void)
{
    return resources_set_int("IDUNIO", 0);
}

void idunio_detach(void)
{
    resources_set_int("IDUNIO", 0);
}

static int set_idunio_host(const char *name, void *param)
{
    if (idunio_host != NULL && name != NULL && strcmp(name, idunio_host) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        util_string_set(&idunio_host, name);
        if (idunio_enabled) {
            idunio_reset();
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------*/
static int idunio_dump(void)
{
    return iduncart_io_dump();
}

static uint8_t idunio_read(uint16_t addr)
{
    idunio_accessed = 1;
    idunio_device.io_source_valid = 1;

    if (addr <= 0x02) {
        return iduncart_io_read(idunio_context, addr);
    } else if (addr == 0xfe) {
        return iduncart_reg_read(idunio_context, addr);
    } else {
        return 0xde;
    }
}

static void idunio_store(uint16_t addr, uint8_t byte)
{
    if (addr == 0x00) {
        iduncart_io_store_data(idunio_context, byte);
    } else if (addr == 0xfe || addr == 0xff) {
        iduncart_reg_write(idunio_context, addr, byte);
    }
    idunio_accessed = 1;
}

/* ---------------------------------------------------------------------*/

static resource_int_t resources_int[] = {
    { "IDUNIO", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &idunio_enabled, set_idunio_enabled, NULL },
    RESOURCE_INT_LIST_END
};
static const resource_string_t resources_string[] = {
    { "IDUNHOST", "localhost:25232", RES_EVENT_NO, NULL,
      &idunio_host, set_idunio_host, NULL },
    RESOURCE_STRING_LIST_END
};

int idunio_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

void idunio_resources_shutdown(void)
{
    if (idunio_context) {
        iduncart_io_destroy(idunio_context);
        idunio_context = NULL;
    }
}

/* ---------------------------------------------------------------------*/
static const cmdline_option_t cmdline_options[] =
{
    { "-idunio", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDUNIO", (resource_value_t)1,
      NULL, "Enable the Idun cartridge I/O" },
    { "+idunio", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDUNIO", (resource_value_t)0,
      NULL, "Disable the Idun cartridge I/O" },
    { "-idunhost", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDUNHOST", NULL,
      "<host:port>", "Set host/port of Idun cartridge to connect" },
    CMDLINE_LIST_END
};

int idunio_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }
    return 0;
}

/* ---------------------------------------------------------------------*/
int idunio_snapshot_write_module(snapshot_t *s)
{
    return -1;
}

int idunio_snapshot_read_module(snapshot_t *s)
{
    return -1;
}
