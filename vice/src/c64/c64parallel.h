/*
 * c64parallel.h - Parallel cable handling for the C64.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_C64PARALLEL_H
#define VICE_C64PARALLEL_H

#include "types.h"

void parallel_cable_cpu_execute(int type);
void parallel_cable_cpu_write(int type, uint8_t data);
void parallel_cable_cpu_pulse(int type);
uint8_t parallel_cable_cpu_read(int type, uint8_t data);
void parallel_cable_cpu_undump(int type, uint8_t data);

int parallel_cable_cpu_resources_init(void);

#endif
