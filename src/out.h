/* out.h
 * Header file for output stuff
 * Copyright (C) Olly Betts 2000,2001,2013
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define out_current_action(S) if(fQuiet)(void)0;else printf("\n%s...\n", (S))
#define out_current_action1(S,A) if(!fQuiet){putnl();printf(S,A);printf("...\n");}else(void)0
