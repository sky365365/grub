/*
 *  SSTD  -- Simple StdLib for UEFI
 *  Copyright (C) 2019  a1ive
 *
 *  SSTD is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SSTD is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with SSTD.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/term.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>

#include <sstdlib.h>

grub_efi_boolean_t
guidcmp (const grub_packed_guid_t *g1, const grub_packed_guid_t *g2)
{
  grub_efi_uint64_t g1_low, g2_low;
  grub_efi_uint64_t g1_high, g2_high;
  g1_low = grub_get_unaligned64 ((const grub_efi_uint64_t *)g1);
  g2_low = grub_get_unaligned64 ((const grub_efi_uint64_t *)g2);
  g1_high = grub_get_unaligned64 ((const grub_efi_uint64_t *)g1 + 1);
  g2_high = grub_get_unaligned64 ((const grub_efi_uint64_t *)g2 + 1);
  return (grub_efi_boolean_t) (g1_low == g2_low && g1_high == g2_high);
}

grub_packed_guid_t *
guidcpy (grub_packed_guid_t *dst, const grub_packed_guid_t *src)
{
  grub_set_unaligned64 ((grub_efi_uint64_t *)dst,
                        grub_get_unaligned64 ((const grub_efi_uint64_t *)src));
  grub_set_unaligned64 ((grub_efi_uint64_t *)dst + 1,
                        grub_get_unaligned64 ((const grub_efi_uint64_t*)src + 1));
  return dst;
}

void
pause (void)
{
  grub_printf ("Press any key to continue booting...");
  grub_getkey ();
  grub_printf ("\n");
}
