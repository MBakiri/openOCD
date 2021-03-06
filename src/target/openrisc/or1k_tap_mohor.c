/***************************************************************************
 *   Copyright (C) 2013 by Franck Jullien                                  *
 *   elec4fun@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "or1k_tap.h"
#include "or1k.h"

#include <jtag/jtag.h>

#define OR1K_TAP_INST_DEBUG	0x8

static int or1k_tap_mohor_init(struct or1k_jtag *jtag_info)
{
	LOG_DEBUG("Initialising OpenCores JTAG TAP");

	/* Put TAP into state where it can talk to the debug interface
	   by shifting in correct value to IR. */
	struct jtag_tap *tap;

	tap = jtag_info->tap;
	if (tap == NULL)
		return ERROR_FAIL;


	struct scan_field field;
	uint8_t t[4];
	uint8_t ret[4];

	field.num_bits = tap->ir_length;
	field.out_value = t;
	/* OpenCores Mohor JTAG TAP-specific */
	buf_set_u32(t, 0, field.num_bits, OR1K_TAP_INST_DEBUG);
	field.in_value = ret;

	/* Ensure TAP is reset - maybe not necessary*/
	jtag_add_tlr();

	jtag_add_ir_scan(tap, &field, TAP_IDLE);
	if (jtag_execute_queue() != ERROR_OK) {
		LOG_ERROR("Setting TAP's IR to DEBUG failed");
		return ERROR_FAIL;
	}

	return ERROR_OK;

}

static struct or1k_tap_ip mohor_tap = {
	.name = "mohor",
	.init = or1k_tap_mohor_init,
};

int or1k_tap_mohor_register(void)
{
	list_add_tail(&mohor_tap.list, &tap_list);
	return 0;
}