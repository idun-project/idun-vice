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

#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
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
#include "crt.h"

/*
    Idun Cartridge

    This cartridge is the interface for ERAM in Idun cart.

    The `idunio` interface provides the registers in the $DE00 IO1 area,
    while this interface is only for accessing the currently selected
    ERAM page, which appears in the $DF00 IO2 area.
*/

/* Idun enabled ?? */
static int idunmm_active = 0;
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
    0,                          /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE              /* NO mirroring */
};

static io_source_list_t *idunmm_list_item = NULL;

static export_resource_t export_res = {
    CARTRIDGE_NAME_IDUNMM, 0, 0, &idunmm_device, NULL, CARTRIDGE_IDUNMM
};

/* ---------------------------------------------------------------------*/
static int idunmm_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    idunmm_list_item = io_source_register(&idunmm_device);
    idunmm_active = 1;

    return 0;
}

int idunmm_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x2000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    return idunmm_common_attach();
}

int idunmm_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.size != 0x2000) {
        return -1;
    }

    if (crt_read_chip(rawcart, 0, &chip, fd)) {
        return -1;
    }

    return idunmm_common_attach();
}

void idunmm_config_init(void)
{
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    idunmm_active = 1;
}

void idunmm_config_setup(uint8_t *rawcart)
{
    memcpy(roml_banks, rawcart, 0x2000);
    cart_config_changed_slotmain(CMODE_8KGAME, CMODE_8KGAME, CMODE_READ);
    iduncart_set_rombase(roml_banks);
    idunmm_active = 1;
}

void idunmm_detach(void)
{
    if (idunmm_list_item != NULL) {
        export_remove(&export_res);
        io_source_unregister(idunmm_list_item);
        idunmm_list_item = NULL;
    }
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

int idunmm_snapshot_write_module(snapshot_t *s)
{
    return -1;
}

int idunmm_snapshot_read_module(snapshot_t *s)
{
    return -1;
}
