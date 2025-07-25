/*
 * idunmm128.c - Idun cartridge emulation for ROM/ERAM
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

#include "cartridge.h"
#include "cartio.h"
#include "util.h"

#include "c64cart.h"
#include "export.h"
#include "c128cart.h"
#include "functionrom.h"

#include "crt.h"
#include "iduncore.h"
#include "idunmm128.h"

/*
    Idun Cartridge

    This cartridge is the interface for ROM and ERAM in Idun cart.

    The `idunio` interface provides the registers in the $DE00 IO1 area,
    while this interface is only for accessing the External Function ROM
    and the currently selected ERAM page in the IO2 area.
*/

/* Idun enabled ?? */
static int idunmm128_active = 0;
static int idunmm128_accessed = 0;

/* ---------------------------------------------------------------------*/

/* Some prototypes are needed */
static uint8_t idunmm128_read(uint16_t addr);
static void idunmm128_store(uint16_t addr, uint8_t byte);
static int idunmm128_dump(void);

static io_source_t idunmm128_device = {
    CARTRIDGE_C128_NAME_IDUN,   /* name of the device */
    IO_DETACH_RESOURCE,         /* use resource to detach the device when involved in a read-collision */
    "IDUNMM",                   /* resource to set to '0' */
    0xdf00, 0xdfff, 0xff,       /* range for the device, regs: $df00-$dfff */
    0,                          /* read validity is determined by the device upon a read */
    idunmm128_store,            /* store function */
    NULL,                       /* NO poke function */
    idunmm128_read,             /* read function */
    idunmm128_read,             /* peek function */
    idunmm128_dump,             /* device state information dump function */
    CARTRIDGE_C128_IDUN,        /* cartridge ID */
    IO_PRIO_NORMAL,             /* normal priority, device read needs to be checked for collisions */
    0,                          /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE              /* NO mirroring */
};

static io_source_list_t *idunmm128_list_item = NULL;

static export_resource_t export_res = {
    CARTRIDGE_C128_NAME_IDUN, 0, 0, &idunmm128_device, NULL, CARTRIDGE_C128_IDUN
};

/* ---------------------------------------------------------------------*/
static int idunmm128_common_attach(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }

    idunmm128_list_item = io_source_register(&idunmm128_device);
    idunmm128_active = 1;

    return 0;
}

int idunmm128_bin_attach(const char *filename, uint8_t *rawcart)
{
    if (util_file_load(filename, rawcart, 0x4000, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }
    memcpy(rawcart + 0x4000, rawcart, 0x4000);
    return idunmm128_common_attach();
}

int idunmm128_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;

    if (crt_read_chip_header(&chip, fd)) {
        return -1;
    }

    if (chip.start == 0x8000 && chip.size == 0x4000) {
        if (crt_read_chip(rawcart, 0, &chip, fd)) {
            return -1;
        }
        memcpy(rawcart + 0x4000, rawcart, 0x4000);
        return idunmm128_common_attach();
    }
    return -1;
}

void idunmm128_config_setup(uint8_t *rawcart)
{
    /* copy loaded cartridge data into actually used ROM array */
    memcpy(&ext_function_rom[0], rawcart, EXTERNAL_FUNCTION_ROM_SIZE);
    iduncart_set_rombase(&ext_function_rom[0]);
    idunmm128_active = 1;
}

void idunmm128_detach(void)
{
    if (idunmm128_list_item != NULL) {
        export_remove(&export_res);
        io_source_unregister(idunmm128_list_item);
        idunmm128_list_item = NULL;
    }
}

/* ---------------------------------------------------------------------*/
static int idunmm128_dump(void)
{
    return iduncart_io_dump();
}

static uint8_t idunmm128_read(uint16_t addr)
{
    idunmm128_accessed = 1;
    idunmm128_device.io_source_valid = 1;
    return iduncart_page_read(addr);
}

static void idunmm128_store(uint16_t addr, uint8_t byte)
{
    idunmm128_accessed = 1;
    iduncart_page_store(addr, byte);
}

/* ---------------------------------------------------------------------*/

int idunmm128_snapshot_write_module(snapshot_t *s)
{
    return -1;
}

int idunmm128_snapshot_read_module(snapshot_t *s)
{
    return -1;
}
