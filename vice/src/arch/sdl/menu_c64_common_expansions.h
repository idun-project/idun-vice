/*
 * menu_c64_common_expansions.h - C64/C128 expansions menu for SDL UI.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#ifndef VICE_MENU_C64_COMMON_EXPANSIONS_H
#define VICE_MENU_C64_COMMON_EXPANSIONS_H

#include "vice.h"
#include "types.h"
#include "uimenu.h"

extern const ui_menu_entry_t digimax_menu[];
extern const ui_menu_entry_t ds12c887rtc_c64_menu[];
extern const ui_menu_entry_t ds12c887rtc_c128_menu[];
extern const ui_menu_entry_t idunio_c64_menu[];
extern const ui_menu_entry_t idunio_c128_menu[];
extern const ui_menu_entry_t idunmm_c64_menu[];
extern const ui_menu_entry_t idunmm_c128_menu[];
extern const ui_menu_entry_t ide64_menu[];

void uiclockport_ide64_menu_create(void);

#endif
