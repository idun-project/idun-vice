/*
 * idunmm.c - Idun cartridge emulation for ERAM functions.
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
#include "idunmm.h"
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

    This cartridge is the interface for ERAM in Idun cart.

    The `idunio` interface provides the registers in the $DE00 IO1 area,
    while this interface is only for accessing the currently selected
    ERAM page, which appears in the $DF00 IO2 area.
*/

/* Idun enabled ?? */
static int idunmm_enabled = 0;
static int idunmm_accessed = 0;

/* ---------------------------------------------------------------------*/

/* Some prototypes are needed */
static uint8_t idunmm_read(uint16_t addr);
static void idunmm_store(uint16_t addr, uint8_t byte);
static int idunmm_dump(void);

static io_source_t idunmm_device = {
    CARTRIDGE_NAME_IDUNMM,      /* name of the device */
    IO_DETACH_RESOURCE,         /* use resource to detach the device when involved in a read-collision */
    "IDUNMM",                   /* resource to set to '0' */
    0xdf00, 0xdfff, 0xff,       /* range for the device, regs: $df00-$dfff */
    0,                          /* read validity is determined by the device upon a read */
    idunmm_store,               /* store function */
    NULL,                       /* NO poke function */
    idunmm_read,                /* read function */
    idunmm_read,                /* peek function */
    idunmm_dump,                /* device state information dump function */
    CARTRIDGE_IDUNMM,           /* cartridge ID */
    IO_PRIO_NORMAL,             /* normal priority, device read needs to be checked for collisions */
    0                           /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *idunmm_list_item = NULL;

static export_resource_t export_res = {
    CARTRIDGE_NAME_IDUNMM, 0, 0, &idunmm_device, NULL, CARTRIDGE_IDUNMM
};

/* ---------------------------------------------------------------------*/
int idunmm_cart_enabled(void)
{
    return idunmm_enabled;
}

static int set_idunmm_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!idunmm_enabled && val) {
        if (export_add(&export_res) < 0) {
            return -1;
        }
        idunmm_list_item = io_source_register(&idunmm_device);
        idunmm_enabled = 1;
    } else if (idunmm_enabled && !val) {
        if (idunmm_list_item != NULL) {
            export_remove(&export_res);
            io_source_unregister(idunmm_list_item);
            idunmm_list_item = NULL;
        }
        idunmm_enabled = 0;
    }
    return 0;
}

void idunmm_reset(void)
{
}

int idunmm_enable(void)
{
    return resources_set_int("IDUNMM", 1);
}

int idunmm_disable(void)
{
    return resources_set_int("IDUNMM", 0);
}

void idunmm_detach(void)
{
    resources_set_int("IDUNMM", 0);
}

/* ---------------------------------------------------------------------*/
static int idunmm_dump(void)
{
    return iduncart_io_dump();
}

static uint8_t idunmm_read(uint16_t addr)
{
    idunmm_accessed = 1;
    idunmm_device.io_source_valid = 1;
    return iduncart_page_read(addr);
}

static void idunmm_store(uint16_t addr, uint8_t byte)
{
    idunmm_accessed = 1;
    iduncart_page_store(addr, byte);
}

/* ---------------------------------------------------------------------*/

static resource_int_t resources_int[] = {
    { "IDUNMM", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &idunmm_enabled, set_idunmm_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int idunmm_resources_init(void)
{
    return resources_register_int(resources_int);
}

void idunmm_resources_shutdown(void)
{
}

/* ---------------------------------------------------------------------*/

static const cmdline_option_t cmdline_options[] =
{
    { "-idunmm", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDUNMM", (resource_value_t)1,
      NULL, "Enable the Idun ERAM" },
    { "+idunmm", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDUNMM", (resource_value_t)0,
      NULL, "Disable the Idun ERAM" },
    CMDLINE_LIST_END
};

int idunmm_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }
    return 0;
}

/* ---------------------------------------------------------------------*/

int idunmm_snapshot_write_module(snapshot_t *s)
{
    return -1;
}

int idunmm_snapshot_read_module(snapshot_t *s)
{
    return -1;
}
