/* hex_dump.h -- simple hex dump routine
 *
 * Copyright (C) 2002 convergence GmbH
 * Johannes Stezenbach <js@convergence.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>

#include "hex_dump.h"


void hex_dump(uint8_t data[], int bytes)
{
	int i, j;
	uint8_t c;

	for (i = 0; i < bytes; i++) {
		if (!(i % 8) && i)
			printf(" ");
		if (!(i % 16) && i) {
			printf("  ");
			for (j = 0; j < 16; j++) {
				c = data[i+j-16];
				if ((c < 0x20) || (c >= 0x7f))
					c = '.';
				printf("%c", c);
			}
			printf("\n");
		}
		printf("%.2x ", data[i]);
	}
	j = (bytes % 16);
	j = (j != 0 ? j : 16);
	for (i = j; i < 16; i++) {
		if (!(i % 8) && i)
			printf(" ");
		printf("   ");
	}
	printf("   ");
	for (i = bytes - j; i < bytes; i++) {
		c = data[i];
		if ((c < 0x20) || (c >= 0x7f))
			c = '.';
		printf("%c", c);
	}
	printf("\n");
}
