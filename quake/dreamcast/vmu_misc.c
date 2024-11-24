#include <kos.h>
#include "vmu_misc.h"

#include "quakedef.h"

int vmu_freeblocks, vmu_totalblocks;
maple_device_t *vm_dev;

/************************************************\
 * speud (27-06-2004)                           *
 *                                              *
 * 	Misc. VMU functions                     *
 *                                              *
\************************************************/

cvar_t vmu_port = {"vmu_port", "0", true};
cvar_t vmu_unit = {"vmu_unit", "1", true};
cvar_t vmu_autosave = {"vmu_autosave", "1", true};

void VMU_init()
{
	Cvar_RegisterVariable(&vmu_port);
	Cvar_RegisterVariable(&vmu_unit);
	Cvar_RegisterVariable(&vmu_autosave);
}

#define VMU_BAD_ROOT -1
#define VMU_BAD_FAT -2
#define VMU_BAD_DEV -3

int VMU_GetFreeblocks()
{
	int i;
	uint8 buf[512];

	vmu_freeblocks = 0;
	vmu_totalblocks = 0;

	if (vm_dev == NULL)
		return VMU_BAD_DEV;

	if (vmu_block_read(vm_dev, 255, buf))
		return VMU_BAD_ROOT;

	vmu_totalblocks = buf[0x50];

	if (vmu_block_read(vm_dev, 254, buf))
		return VMU_BAD_FAT;

	for (i = vmu_totalblocks - 1; i >= 0; i--)
		if (buf[i * 2] == 0xfc && buf[i * 2 + 1] == 0xff)
			vmu_freeblocks++;

	return vmu_freeblocks;
}

/* calcCRC() by Marcus Comstedt */
uint16 VMU_calcCRC(char *file_buf, int data_len)
{
	uint16 crc = 0;
	int i, j;

	for (i = 0; i < data_len; i++)
	{
		crc ^= (file_buf[i] << 8);

		for (j = 0; j < 8; j++)
		{
			if (crc & 0x8000)
				crc = (crc << 1) ^ 4129;
			else
				crc = (crc << 1);
		}
	}

	return crc;
}
