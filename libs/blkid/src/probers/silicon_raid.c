/*
 * Copyright (C) 2008 Karel Zak <kzak@redhat.com>
 * Copyright (C) 2005 Kay Sievers <kay.sievers@vrfy.org>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>

#include "blkidP.h"

struct silicon_meta {
	uint8_t		unknown0[0x2E];
	uint8_t		ascii_version[0x36 - 0x2E];
	uint8_t		diskname[0x56 - 0x36];
	uint8_t		unknown1[0x60 - 0x56];
	uint32_t	magic;
	uint32_t	unknown1a[0x6C - 0x64];
	uint32_t	array_sectors_low;
	uint32_t	array_sectors_high;
	uint8_t		unknown2[0x78 - 0x74];
	uint32_t	thisdisk_sectors;
	uint8_t		unknown3[0x100 - 0x7C];
	uint8_t		unknown4[0x104 - 0x100];
	uint16_t	product_id;
	uint16_t	vendor_id;
	uint16_t	minor_ver;
	uint16_t	major_ver;
};

#define SILICON_MAGIC		0x2F000000


static int probe_silraid(blkid_probe pr, const struct blkid_idmag *mag)
{
	uint64_t meta_off;
	struct silicon_meta *sil;

	if (pr->size < 0x10000)
		return -1;

	meta_off = ((pr->size / 0x200) - 1) * 0x200;

	sil = (struct silicon_meta *) blkid_probe_get_buffer(pr,
						meta_off, 0x200);
	if (!sil)
		return -1;

	if (le32_to_cpu(sil->magic) != SILICON_MAGIC)
		return -1;

	if (blkid_probe_sprintf_version(pr, "%u.%u",
				le16_to_cpu(sil->major_ver),
				le16_to_cpu(sil->minor_ver)) != 0)
		return -1;

	return 0;
}

const struct blkid_idinfo silraid_idinfo = {
	.name		= "silicon_medley_raid_member",
	.usage		= BLKID_USAGE_RAID,
	.probefunc	= probe_silraid,
	.magics		= BLKID_NONE_MAGIC
};


