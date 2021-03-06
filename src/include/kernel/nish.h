/*
 *  Copyright (C) 2007,2008 Sven Köhler
 *
 *  This file is part of Nupkux.
 *
 *  Nupkux is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Nupkux is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Nupkux.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
	Nupkux intern shell
*/

#ifndef _NUPKUX_H
#define _NUPKUX_H

#include <kernel.h>

#define NISH_EXIT	0xE0
#define NISH_HALT	0xE1
#define NISH_REBOOT	0xE2

extern int nish(void);
extern void _echo_pc_speaker(UINT freq);
extern void _stop_pc_speaker(void);

#endif
