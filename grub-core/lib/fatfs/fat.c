/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/dl.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/command.h>
#include <grub/i18n.h>
#include <grub/disk.h>
#include <grub/partition.h>

#include "ff.h"
#include "diskio.h"

GRUB_MOD_LICENSE ("GPLv3+");

static grub_err_t
grub_cmd_mount (grub_command_t cmd __attribute__ ((unused)),
                int argc, char **args)

{
  unsigned int num = 0;
  int namelen;
  grub_disk_t disk = 0;
  if (argc == 1 && grub_strcmp (args[0], "status") == 0)
  {
    int i;
    for (i = 0; i < 10; i++)
    {
      if (!fat_stat[i].disk)
        continue;
      if (!fat_stat[i].disk->partition)
        grub_printf ("%s -> %s:\n", fat_stat[i].disk->name, fat_stat[i].name);
      else
        grub_printf ("%s,%d -> %s:\n", fat_stat[i].disk->name,
                     fat_stat[i].disk->partition->number + 1,
                     fat_stat[i].name);
    }
    return GRUB_ERR_NONE;
  }
  if (argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad argument");
  num = grub_strtoul (args[1], NULL, 10);
  if (num > 9)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid number");
  namelen = grub_strlen (args[0]);
  if ((args[0][0] == '(') && (args[0][namelen - 1] == ')'))
  {
    args[0][namelen - 1] = 0;
    disk = grub_disk_open (&args[0][1]);
  }
  else
    disk = grub_disk_open (args[0]);
  if (!disk)
    return grub_errno;

  if (fat_stat[num].disk)
  {
    grub_disk_close (disk);
    return grub_error (GRUB_ERR_BAD_DEVICE, "disk number in use");
  }
  fat_stat[num].present = 1;
  grub_snprintf (fat_stat[num].name, 2, "%u", num);
  fat_stat[num].disk = disk;
  fat_stat[num].total_sectors = disk->total_sectors;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_umount (grub_command_t cmd __attribute__ ((unused)),
                int argc, char **args)

{
  unsigned int num = 0;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad argument");
  num = grub_strtoul (args[0], NULL, 10);
  if (num > 9)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "invalid number");

  if (fat_stat[num].disk)
    grub_disk_close (fat_stat[num].disk);
  fat_stat[num].disk = 0;
  fat_stat[num].present = 0;
  fat_stat[num].total_sectors = 0;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_mkdir (grub_command_t cmd __attribute__ ((unused)),
                int argc, char **args)

{
  char dev[3] = "0:";
  FATFS fs;
  FRESULT res;
  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad argument");
  if (grub_isdigit (args[0][0]))
    dev[0] = args[0][0];

  f_mount (&fs, dev, 0);
  res = f_mkdir (args[0]);
  if (res)
    return grub_error (GRUB_ERR_WRITE_ERROR, "mkdir failed %d", res);
  f_mount(0, dev, 0);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_cp (grub_command_t cmd __attribute__ ((unused)),
             int argc, char **args)

{
  char in_dev[3] = "0:";
  char out_dev[3] = "0:";
  FIL in, out;
  FATFS in_fs, out_fs;
  FRESULT res;
  BYTE buffer[4096];
  UINT br, bw;
  if (argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad argument");

  if (grub_isdigit (args[0][0]))
    in_dev[0] = args[0][0];
  if (grub_isdigit (args[1][0]))
    out_dev[0] = args[1][0];

  f_mount (&in_fs, in_dev, 0);
  f_mount (&out_fs, out_dev, 0);
  res = f_open (&in, args[0], FA_READ);
  if (res)
    return grub_error (GRUB_ERR_BAD_FILENAME, "src open failed %d", res);
  res = f_open (&out, args[1], FA_WRITE | FA_CREATE_ALWAYS);
  if (res)
    return grub_error (GRUB_ERR_BAD_FILENAME, "dst open failed %d", res);

  for (;;)
  {
    res = f_read (&in, buffer, sizeof (buffer), &br);
    if (res || br == 0)
      break; /* error or eof */
    res = f_write (&out, buffer, br, &bw);
    if (res || bw < br)
      break; /* error or disk full */
  }
  f_close(&in);
  f_close(&out);
  /* Unregister work area prior to discard it */
  f_mount(0, in_dev, 0);
  f_mount(0, out_dev, 0);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_rename (grub_command_t cmd __attribute__ ((unused)),
                 int argc, char **args)

{
  char dev[3] = "0:";
  FATFS fs;
  FRESULT res;
  if (argc != 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad argument");
  if (grub_isdigit (args[0][0]))
    dev[0] = args[0][0];
  if (grub_isdigit (args[1][0]) && args[1][0] != dev[0])
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "dst drive error");

  f_mount (&fs, dev, 0);
  res = f_rename (args[0], args[1]);
  if (res)
    return grub_error (GRUB_ERR_WRITE_ERROR, "rename failed %d", res);
  f_mount(0, dev, 0);
  return GRUB_ERR_NONE;
}

static grub_command_t cmd_mount, cmd_umount, cmd_mkdir, cmd_cp, cmd_rename;

GRUB_MOD_INIT(fatfs)
{
  cmd_mount = grub_register_command ("mount", grub_cmd_mount,
                                      N_("status | DISK NUM"),
                                      N_("Mount FAT partition."));
  cmd_umount = grub_register_command ("umount", grub_cmd_umount,
                                      N_("NUM"),
                                      N_("Unmount FAT partition."));
  cmd_mkdir = grub_register_command ("mkdir", grub_cmd_mkdir,
                                      N_("PATH"),
                                      N_("Create new directory."));
  cmd_cp = grub_register_command ("cp", grub_cmd_cp,
                                      N_("FILE1 FILE2"),
                                      N_("Copy file."));
  cmd_rename = grub_register_command ("rename", grub_cmd_rename,
                                      N_("FILE FILE_NAME"),
                                      N_("Renames a file or sub-directory and can also move it to other directory in the same volume."));
}

GRUB_MOD_FINI(fatfs)
{
  grub_unregister_command (cmd_mount);
  grub_unregister_command (cmd_umount);
  grub_unregister_command (cmd_mkdir);
  grub_unregister_command (cmd_cp);
  grub_unregister_command (cmd_rename);
}
