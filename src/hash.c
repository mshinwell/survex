/* hash.c */
/* String hashing function */
/* Copyright (C) 1995-2001 Olly Betts
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

#include <ctype.h>
#include <debug.h>

#include "hash.h"

/* some (preferably prime) number for the hashing function */
#define HASH_PRIME 29363

int
hash_string(const char *p)
{
   int hash;
   ASSERT(p);
   for (hash = 0; *p; p++)
      hash = (hash * HASH_PRIME + *(const unsigned char*)p) & 0x7fff;
   return hash;
}

int
hash_lc_string(const char *p)
{
   int hash;
   ASSERT(p);
   for (hash = 0; *p; p++)
      hash = (hash * HASH_PRIME + tolower(*(const unsigned char*)p)) & 0x7fff;
   return hash;
}
