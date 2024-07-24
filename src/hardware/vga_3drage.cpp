/*
 *  Copyright (C) 2002-2021  The DOSBox Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <assert.h>

#include "dosbox.h"
#include "logging.h"
#include "setup.h"
#include "video.h"
#include "pic.h"
#include "vga.h"
#include "inout.h"
#include "programs.h"
#include "support.h"
#include "setup.h"
#include "timer.h"
#include "mem.h"
#include "util_units.h"
#include "control.h"
#include "pc98_cg.h"
#include "pc98_dac.h"
#include "pc98_gdc.h"
#include "pc98_gdc_const.h"
#include "mixer.h"
#include "menu.h"
#include "mem.h"
#include "render.h"
#include "jfont.h"
#include "bitop.h"
#include "sdlmain.h"

#include <string.h>
#include <stdlib.h>
#include <string>
#include <stdio.h>

struct ATIRageState {
	uint8_t				index = 0;
	uint32_t			crtc_h_total_disp = 0; /* index 0x00 */
	uint32_t			crtc_v_total_disp = 0; /* index 0x08 */
	uint32_t			crtc_off_pitch = 0; /* index 0x14 */
	uint32_t			crtc_gen_cntl = 0; /* index 0x1C */
	uint32_t			clock_cntl = 0; /* index 0x90 */
	uint32_t			config_chip_id = 0; /* index 0xE0 */
	uint32_t			input_status_register = 0; /* index 0xBB */
	uint32_t			color_mode = 0;
};

ATIRageState atirage_state;

enum {
    BPP_1  = 0,
    BPP_4  = 1,
    BPP_8  = 2,
    BPP_15 = 3,
    BPP_16 = 4,
    BPP_24 = 5,
    BPP_32 = 6
};

Bitu ATIRageExtIndex_Read(Bitu /*port*/, Bitu /*len*/) {
	return atirage_state.index;
}

void ATIRageExtIndex_Write(Bitu /*port*/, Bitu val, Bitu /*len*/) {
	atirage_state.index = (uint8_t)val;
}

void ATIRage_UpdateColorMode(void) {
	switch ((atirage_state.crtc_gen_cntl >> 8) & 7) {
		case BPP_32: atirage_state.color_mode = M_LIN32; break;
		case BPP_24: atirage_state.color_mode = M_LIN24; break;
		case BPP_16: atirage_state.color_mode = M_LIN16; break;
		case BPP_15: atirage_state.color_mode = M_LIN15; break;
		case BPP_8:  atirage_state.color_mode = M_LIN8;  break;
		case BPP_4:  atirage_state.color_mode = M_LIN4;  break;
	}
}

Bitu ATIRageExtData_Read(Bitu port, Bitu /*len*/) {
	switch (atirage_state.index) {
		case 0x00:
			return atirage_state.crtc_h_total_disp = vga.crtc.horizontal_total;
		case 0x08:
			return atirage_state.crtc_v_total_disp = vga.crtc.vertical_total;
		case 0x1C:
			return atirage_state.crtc_gen_cntl;
		case 0xE0:
			return atirage_state.config_chip_id;
		default:
			break;
	};

	LOG(LOG_MISC,LOG_DEBUG)("Unhandled ATI Rage extended read port=%x index=%x",(unsigned int)port,atirage_state.index);
	return 0;
}

void ATIRageExtData_Write(Bitu port, Bitu val, Bitu /*len*/) {
	switch (atirage_state.index) {
		case 0x00:
			atirage_state.crtc_h_total_disp = (uint8_t)val;
			break;
		case 0x08:
			atirage_state.crtc_v_total_disp = (uint8_t)val;
			break;
		case 0x1C:
			atirage_state.crtc_gen_cntl = (uint8_t)val;
			ATIRage_UpdateColorMode();
			break;
		case 0xE0:
			atirage_state.config_chip_id = (uint8_t)val;
			break;
		default:
			break;
	};

	LOG(LOG_MISC,LOG_DEBUG)("Unhandled ATI Rage extended write port=%x index=%x val=%x",(unsigned int)port,atirage_state.index,(unsigned int)val);
}

void SVGA_Setup_ATIRage(void) {
	if (vga.mem.memsize == 0)
		vga.mem.memsize = 2*1024*1024;

	ati_state.config_chip_id = 0x3a004755;

	IO_RegisterWriteHandler(0x1ce,&ATIExtIndex_Write,IO_MB);
	IO_RegisterReadHandler(0x1ce,&ATIExtIndex_Read,IO_MB);
	IO_RegisterWriteHandler(0x1cf,&ATIExtData_Write,IO_MB);
	IO_RegisterReadHandler(0x1cf,&ATIExtData_Read,IO_MB);
}

