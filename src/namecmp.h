/* namecmp.h */
/* Ordering function for station names */
/* Copyright (C) 2001,2002,2008 Olly Betts
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef __cplusplus
#include "wx.h"

extern "C" {
#endif

extern int name_cmp(const char *a, const char *b, int separator);

#ifdef __cplusplus
};

inline int name_cmp(const wxString &a, const wxString &b, int separator) {
    return name_cmp(a.mb_str(), b.mb_str(), separator);
}
#endif
