
/************************************************\
 * speud (27-06-2004)                           *
 *                                              *
 * 	- Replaced fprintf                      *
 * 	- Implemented compressed files support  *
 * 	- Implemented VMU support               *
 * 	- Rounded some floats                   *
 *                                              *
\************************************************/

#include <kos.h>
#include <zlib/zlib.h>

#include "vmu_misc.h"

#include "quakedef.h"

uint8_t icon_palette[32] = {
	0xFF, 0x0F, 0x70, 0xF7, 0xBB, 0xFB, 0x00, 0xF7, 0x77, 0xF7, 0x00, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t icon_bitmap[512] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x20, 0x00, 0x00, 0x00, 0x24, 0x20, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x05, 0x42, 0x00, 0x00, 0x00, 0x00, 0x02, 0x45, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x50, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x05, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x35, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x45, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x40, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x52, 0x00, 0x00,
	0x00, 0x00, 0x04, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x54, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x51, 0x00, 0x00,
	0x00, 0x00, 0x25, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x55, 0x20, 0x00,
	0x00, 0x00, 0x25, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x55, 0x20, 0x00,
	0x00, 0x00, 0x25, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x55, 0x20, 0x00,
	0x00, 0x00, 0x25, 0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x55, 0x20, 0x00,
	0x00, 0x00, 0x25, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x53, 0x20, 0x00,
	0x00, 0x00, 0x25, 0x51, 0x00, 0x00, 0x02, 0x20, 0x00, 0x22, 0x00, 0x00, 0x01, 0x55, 0x20, 0x00,
	0x00, 0x00, 0x05, 0x55, 0x20, 0x00, 0x02, 0x55, 0x55, 0x52, 0x00, 0x00, 0x45, 0x55, 0x00, 0x00,
	0x00, 0x00, 0x04, 0x55, 0x12, 0x00, 0x00, 0x45, 0x55, 0x40, 0x00, 0x02, 0x15, 0x54, 0x00, 0x00,
	0x00, 0x00, 0x02, 0x55, 0x51, 0x20, 0x00, 0x25, 0x55, 0x20, 0x00, 0x21, 0x55, 0x52, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x45, 0x55, 0x14, 0x00, 0x25, 0x55, 0x20, 0x04, 0x15, 0x55, 0x40, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x25, 0x55, 0x55, 0x51, 0x45, 0x55, 0x41, 0x55, 0x55, 0x55, 0x20, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x52, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x25, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x20, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x02, 0x45, 0x55, 0x55, 0x55, 0x55, 0x55, 0x42, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x45, 0x55, 0x55, 0x55, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x25, 0x55, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/*
===============================================================================

LOAD / SAVE GAME

===============================================================================
*/

#define SAVEGAME_VERSION 5

// -----------------------------------------
// BlackAura (08-12-2002) - Replacing fscanf
// -----------------------------------------

static char sbuf[32768];
static void DC_ScanString(FILE *file, char *string)
{
	char newchar;
	fread(&newchar, 1, 1, file);
	while (newchar != '\n')
	{
		*string++ = newchar;
		fread(&newchar, 1, 1, file);
	}
	*string++ = '\0';
}

static int DC_ScanInt(FILE *file)
{
	DC_ScanString(file, sbuf);
	return atoi(sbuf);
}

static float DC_ScanFloat(FILE *file)
{
	DC_ScanString(file, sbuf);
	return atof(sbuf);
}

/*
===============
Host_SavegameComment

Writes a SAVEGAME_COMMENT_LENGTH character comment describing the current
===============
*/
void Host_SavegameComment(char *text)
{
	int i;
	char kills[20];

	for (i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
		text[i] = ' ';
	memcpy(text, cl.levelname, strlen(cl.levelname));
	sprintf(kills, "kills:%3i/%3i", cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS]);
	memcpy(text + 22, kills, strlen(kills));
	// convert space to _ to make stdio happy
	for (i = 0; i < SAVEGAME_COMMENT_LENGTH; i++)
		if (text[i] == ' ')
			text[i] = '_';
	text[SAVEGAME_COMMENT_LENGTH] = '\0';
}

// ------------------------------------------------------------------------
// speud (27-06-2004) - Replacing Host_Savegame_f and Host_Loadgame_f begin
// ------------------------------------------------------------------------

/*
===============
Host_Savegame_f
===============
*/

/*
    Name: Ian micheal
    Date: 25/11/24 05:54
    Description: Fixed pointer signedness in VMU CRC calculation
*/
static char w_buf[40000];
void Host_Savegame_f(void)
{
	char name[256], comment[SAVEGAME_COMMENT_LENGTH + 1], *ptr, *file_buf;
	char buffer[8];
	uint16 crc;
	int i, filesize, total;
	gzFile f1;
	file_t f2;
	FILE *f;

	extern cvar_t vmu_port;
	extern cvar_t vmu_unit;

	if (cmd_source != src_command)
		return;

	if (!sv.active)
	{
		Con_Printf("Not playing a local game.\n");
		return;
	}

	if (cl.intermission)
	{
		Con_Printf("Can't save in intermission.\n");
		return;
	}

	if (svs.maxclients != 1)
	{
		Con_Printf("Can't save multiplayer games.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf("save <savename> : save a game\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_Printf("Relative pathnames are not allowed.\n");
		return;
	}

	for (i = 0; i < svs.maxclients; i++)
	{
		if (svs.clients[i].active && (svs.clients[i].edict->v.health <= 0))
		{
			Con_Printf("Can't savegame with a dead player\n");
			return;
		}
	}

	// Saving compressed game to RAM
	Con_Printf("Saving compressed game to %s...\n", "/ram/compressed.gz");
	f1 = gzopen("/ram/compressed.gz", "w");
	if (!f1)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return;
	}

	sprintf(w_buf, "%i\n", SAVEGAME_VERSION);
	gzwrite(f1, w_buf, strlen(w_buf));

	Host_SavegameComment(comment);

	/* Quick Save/Load menu changes */
	int saveslot = atoi(Cmd_Argv(1)+18);
	printf("Saveslot message: %d %s\n", saveslot, Cmd_Argv(1)+18);
	#define	MAX_SAVEGAMES		12
	extern char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
	extern int	loadable[MAX_SAVEGAMES];
	loadable[saveslot] = true;
	memcpy(m_filenames[saveslot], comment, SAVEGAME_COMMENT_LENGTH);
	m_filenames[saveslot][SAVEGAME_COMMENT_LENGTH] = '\0';
	for (i=0 ; i<SAVEGAME_COMMENT_LENGTH ; i++)
		if (m_filenames[saveslot][i] == '_')
			m_filenames[saveslot][i] = ' ';

	sprintf(w_buf, "%s\n", comment);
	gzwrite(f1, w_buf, strlen(w_buf));

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		sprintf(w_buf, "%d.000000\n", (int)svs.clients->spawn_parms[i]);
		gzwrite(f1, w_buf, strlen(w_buf));
	}

	sprintf(w_buf, "%d\n", current_skill);
	gzwrite(f1, w_buf, strlen(w_buf));

	sprintf(w_buf, "%s\n", sv.name);
	gzwrite(f1, w_buf, strlen(w_buf));

	sprintf(w_buf, "%d.000000\n", (int)sv.time);
	gzwrite(f1, w_buf, strlen(w_buf));

	// write the light styles

	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		if (sv.lightstyles[i])
		{
			sprintf(w_buf, "%s\n", sv.lightstyles[i]);
			gzwrite(f1, w_buf, strlen(w_buf));
		}
		else
		{
			sprintf(w_buf, "m\n");
			gzwrite(f1, w_buf, strlen(w_buf));
		}
	}


	ED_WriteGlobals(f1);

	for (i = 0; i < sv.num_edicts; i++)
		ED_Write(f1, EDICT_NUM(i));

	gzclose(f1);

	// Reading compressed game
	Con_Printf("Reading compressed game from %s...\n", "/ram/compressed.gz");
	f2 = fs_open("/ram/compressed.gz", O_RDONLY);
	if (!f2)
	{
		Con_Printf("ERROR: couldn't open.\n");
		fs_unlink("/ram/compressed.gz");
		return;
	}

	filesize = fs_total(f2);

	total = filesize + 640;
	while ((total % 512) != 0)
	{
		total++;
	}

	file_buf = (char *)Hunk_TempAlloc(total + 1);
	if (file_buf == NULL)
	{
		Con_Printf("ERROR: not enough memory.\n");
		fs_unlink("/ram/compressed.gz");
		fs_close(f2);
		return;
	}

	memset(file_buf, 0, total);

	fs_read(f2, file_buf + 640, filesize);
	fs_close(f2);

	// Filling VMS header
	Con_Printf("Filling VMS header...\n");

	ptr = file_buf;
	char desc[17];
	snprintf(desc,17,"\x12%s   GAME", VMU_NAME);
	for (i=1; i < 17; i++)
    {
        if (desc[i]=='_')
            desc[i] = ' ';
    }

	memcpy(ptr, desc, 16);
	ptr += 16; // VM desc

	for (i = 0; i < 22; i++)
		if (comment[i] == '_')
			comment[i] = ' ';

	memcpy(ptr, comment, 22);
	ptr += 22; // DC desc: mapname
	memset(ptr, 0, 6);
	ptr += 6; // DC desc: reserved

	sprintf(buffer, "%c%c%c%c",
			cl.stats[STAT_MONSTERS] % 256,
			cl.stats[STAT_MONSTERS] / 256,
			cl.stats[STAT_TOTALMONSTERS] % 256,
			cl.stats[STAT_TOTALMONSTERS] / 256);

	memcpy(ptr, buffer, 4);
	ptr += 4; // DC desc: kills
	memcpy(ptr, APP_NAME, 16);
	ptr += 16; // Application
	memcpy(ptr, "\x01\0", 2);
	ptr += 2; // Icons number
	memcpy(ptr, "\0\0", 2);
	ptr += 2; // Anim speed
	memset(ptr, 0, 2);
	ptr += 2; // Eyecatch type
	memset(ptr, 0, 2);
	ptr += 2; // CRC checksum

	sprintf(buffer, "%c%c%c%c",
			(filesize & 0xff) >> 0,
			(filesize & 0xff00) >> 8,
			(filesize & 0xff0000) >> 16,
			(filesize & 0xff000000) >> 24);

	memcpy(ptr, buffer, 4);
	ptr += 4; // Filesize
	memset(ptr, 0, 20);
	ptr += 20; // Reserved
	memcpy(ptr, icon_palette, 32);
	ptr += 32; // Icons palette
	memcpy(ptr, icon_bitmap, 512);
	ptr += 512; // Icons bitmap

    Con_Printf("Calculating CRC checksum...\n");

    // Fixed: Cast to const uint8_t* to match VMU_calcCRC signature
    crc = VMU_calcCRC((const uint8_t *)file_buf, filesize + 640);

    file_buf[0x46] = (crc & 0x00ff) >> 0;
    file_buf[0x47] = (crc & 0xff00) >> 8;
	// Saving game
	sprintf(name, "%s", Cmd_Argv(1));

	if (name[0] != '/')
	{
		memcpy(name, va("/vmu/%c%d/", (int)vmu_port.value + 'a', (int)vmu_unit.value), 8);
		memcpy(name + 8, Cmd_Argv(1), strlen(Cmd_Argv(1)));
		name[strlen(Cmd_Argv(1)) + 8] = 0;
	}

	if (!memcmp(name, "/vmu/", 5))
	{
		int port, slot;

		port = name[5];
		slot = name[6];

		if (port < 'a' || port > 'd' || slot < '1' || slot > '2')
		{
			Con_Printf("ERROR: \"%c-%c\" is not a valid VM port/slot.\n", port, slot);
			fs_unlink("/ram/compressed.gz");
			free(file_buf);
			return;
		}

		Con_Printf("Counting free blocks for VMU in %c-%c...\n", port, slot);

		vm_dev = maple_enum_dev(port - 'a', slot - '1' + 1);
		vmu_freeblocks = VMU_GetFreeblocks();

		if (vmu_freeblocks == -1)
		{
			Con_Printf("ERROR: couldn't read root block.\n");
			fs_unlink("/ram/compressed.gz");
			free(file_buf);
			return;
		}
		else if (vmu_freeblocks == -2)
		{
			Con_Printf("ERROR: couldn't read FAT.\n");
			fs_unlink("/ram/compressed.gz");
			free(file_buf);
			return;
		}

		f2 = fs_open(name, O_RDONLY);
		if (f2)
		{
			vmu_freeblocks += fs_total(f2) / 512;
			fs_close(f2);
		}

		if ((vmu_freeblocks * 512) < total)
		{
			Con_Printf("Not enough free blocks. Free:%d Needed:%d.\n", vmu_freeblocks, total / 512);
			fs_unlink("/ram/compressed.gz");
			free(file_buf);
			return;
		}
	}

	Con_Printf("Saving game to %s...\n", name);

	fs_unlink(name);

	f = fopen(name, "w");
	if (!f)
	{
		Con_Printf("ERROR: couldn't open.\n");
		fs_unlink("/ram/compressed.gz");
		free(file_buf);
		return;
	}

	fwrite(file_buf, total, 1, f);
	fclose(f);

	Con_Printf("done.\n");

	fs_unlink("/ram/compressed.gz");
	free(file_buf);
}

void Host_Reconnect_f(void);
extern size_t zlib_getlength(const char*filename);
/*
===============
Host_Loadgame_f
===============
*/
static char str[32768];
void Host_Loadgame_f(void)
{
	char name[MAX_OSPATH], mapname[MAX_QPATH],  *compressed_buf, *uncompressed_buf;
	const char *start;
	float time, tfloat, spawn_parms[NUM_SPAWN_PARMS];
	int entnum, version, i, r, uncompressed_size, offset = 0, head_len;
	uint16 icons_n, crc, eyec_t;
	uint32 compressed_size;
	edict_t *ent;
	file_t f1;
	gzFile f2;
	FILE *f;

	extern cvar_t vmu_port;
	extern cvar_t vmu_unit;

	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("load <savename> : load a game\n");
		return;
	}

	cls.demonum = -1; // stop demo loop in case this fails

	// we can't call SCR_BeginLoadingPlaque, because too much stack space has
	// been used.  The menu calls it before stuffing loadgame command
	//	SCR_BeginLoadingPlaque ();

	// Opening game save
	sprintf(name, "%s", Cmd_Argv(1));

	if (name[0] != '/')
	{
		memcpy(name, va("/vmu/%c%d/", (int)vmu_port.value + 'a', (int)vmu_unit.value), 8);
		memcpy(name + 8, Cmd_Argv(1), strlen(Cmd_Argv(1)));
		name[strlen(Cmd_Argv(1)) + 8] = 0;
	}

	Con_Printf("Opening compressed game from %s...\n", name);
	f1 = fs_open(name, O_RDONLY);
	if (!f1)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return;
	}

	// Skipping DC/VM desc
	fs_seek(f1, 48, SEEK_SET);

	// Checking application name
	fs_read(f1, str, 16);
	if (!!memcmp(str, APP_NAME, 16))
	{
		Con_Printf("ERROR: not a valid RADQuake game save. (%s)\n", str);
		fs_close(f1);
		return;
	}

	// Checking icons number
	fs_read(f1, &icons_n, 2);
	if (icons_n < 1 || icons_n > 3)
	{
		Con_Printf("ERROR: not a valid VMS icon. (%d)\n", icons_n);
		fs_close(f1);
		return;
	}

	// Skipping icons anim speed
	fs_seek(f1, 2, SEEK_CUR);

	// Checking eyecatch type
	fs_read(f1, &eyec_t, 2);
	if (eyec_t > 3)
	{
		Con_Printf("ERROR: not a valid VMS eyecatch. (%d)\n", eyec_t);
		fs_close(f1);
		return;
	}

	// getting data length
	fs_seek(f1, 70, SEEK_SET);
	fs_read(f1, &crc, 2);
	fs_read(f1, &compressed_size, 4);
	//	printf("compressed_size=%d;\n", compressed_size);
	head_len = 128 + icons_n * 512;
	if (eyec_t > 0)
	{
		switch (eyec_t)
		{
		case 1:
			head_len += 8064;
			break;
		case 2:
			head_len += 4544;
			break;
		case 3:
			head_len += 2048;
			break;
		}
	}
	//	Con_Printf ("VMS header's length: %d.\n", head_len);

	// Reading compressed game
	compressed_buf = (char *)Hunk_TempAlloc(compressed_size + 1);
	if (compressed_buf == NULL)
	{
		Con_Printf("ERROR: not enough memory.\n");
		fs_close(f1);
		return;
	}

	fs_seek(f1, head_len, SEEK_SET);
	fs_read(f1, compressed_buf, compressed_size);

	fs_close(f1);

	// Copying compressed game to RAM
	Con_Printf("Copying compressed game to %s...\n", "/ram/compressed.gz");
	f1 = fs_open("/ram/compressed.gz", O_WRONLY);
	if (!f1)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return;
	}

	fs_write(f1, compressed_buf, compressed_size);
	fs_close(f1);

	// Uncompressing game
	Con_Printf("Uncompressing game from %s...\n", "/ram/compressed.gz");
	f2 = gzopen("/ram/compressed.gz", "r");
	if (!f2)
	{
		Con_Printf("ERROR: couldn't open.\n");
		fs_unlink("/ram/compressed.gz");
		return;
	}

	uncompressed_size = zlib_getlength("/ram/compressed.gz");
	printf("uncompressed_size=%d;\n", uncompressed_size);

	uncompressed_buf = (char *)Hunk_TempAlloc(uncompressed_size + 1);
	if (uncompressed_buf == NULL)
	{
		Con_Printf("ERROR: not enough memory.\n");
		fs_unlink("/ram/compressed.gz");
		gzclose(f2);
		return;
	}

	gzread(f2, uncompressed_buf, uncompressed_size);
	gzclose(f2);

	// Copying uncompressed game to RAM
	Con_Printf("Copying uncompressed game to %s...\n", "/ram/uncompressed.sav");
	f1 = fs_open("/ram/uncompressed.sav", O_WRONLY);
	if (!f1)
	{
		Con_Printf("ERROR: couldn't open.\n");
		fs_unlink("/ram/compressed.gz");
		return;
	}

	fs_write(f1, uncompressed_buf, uncompressed_size);
	fs_close(f1);

	// Loading game
	Con_Printf("Loading game from %s...\n", "/ram/uncompressed.sav");
	f = fopen("/ram/uncompressed.sav", "r");
	if (!f)
	{
		Con_Printf("ERROR: couldn't open.\n");
		fs_unlink("/ram/compressed.gz");
		fs_unlink("/ram/uncompressed.sav");
		return;
	}

	version = DC_ScanInt(f);
	if (version != SAVEGAME_VERSION)
	{
		Con_Printf("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
		fs_unlink("/ram/compressed.gz");
		fs_unlink("/ram/uncompressed.sav");
		fclose(f);
		return;
	}
	//	Con_Printf ("version: %i;\n", version);

	DC_ScanString(f, str);

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		spawn_parms[i] = DC_ScanFloat(f);

	// this silliness is so we can load 1.06 save files, which have float skill values
	tfloat = DC_ScanFloat(f);

	current_skill = (int)(tfloat + 0.1);
	Cvar_SetValue("skill", (float)current_skill);
	//	Con_Printf ("skill: %f;\n", (float)current_skill);

	DC_ScanString(f, mapname);
	//	Con_Printf ("mapname: %s;\n", mapname);

	time = DC_ScanFloat(f);
	//	Con_Printf ("time: %f;\n", time);

	CL_Disconnect_f();

	SV_SpawnServer(mapname);

	if (!sv.active)
	{
		Con_Printf("Couldn't load map\n");
		fs_unlink("/ram/compressed.gz");
		fs_unlink("/ram/uncompressed.sav");
		fclose(f);
		return;
	}
	sv.paused = true; // pause until all clients connect
	sv.loadgame = true;

	// load the light styles
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		DC_ScanString(f, str);
		//sv.lightstyles[i] = Hunk_Alloc (strlen(str)+1);
		//strcpy (sv.lightstyles[i], str);
		sv.lightstyles[i] = (const char *)Hunk_Strdup (str, "lightstyles");
	}

	fclose(f);

	// load the edicts out of the savegame file
	while (offset < uncompressed_size && uncompressed_buf[offset + 1] != '{' && uncompressed_buf[offset] != 0)
		offset++;

	entnum = -1; // -1 is the globals
	while (offset < uncompressed_size)
	{
		for (i = 0; i < (int)sizeof(str) - 1; i++)
		{
			r = uncompressed_buf[offset];
			offset++;

			if (offset >= uncompressed_size || !r)
				break;
			str[i] = r;
			if (r == '}')
			{
				i++;
				break;
			}
		}
		if (i == sizeof(str) - 1)
			Sys_Error("Loadgame buffer overflow");
		str[i] = 0;
		start = COM_Parse(str);
		if (!com_token[0])
			break; // end of file
		if (strcmp((const char*)com_token, "{"))
			Sys_Error("First token isn't a brace");

		if (entnum == -1)
		{ // parse the global vars
			ED_ParseGlobals(start);
		}
		else
		{ // parse an edict

			ent = EDICT_NUM(entnum);
			memset(&ent->v, 0, progs->entityfields * 4);
			ent->free = false;
			ED_ParseEdict(start, ent);

			// link it into the bsp tree
			if (!ent->free)
				SV_LinkEdict(ent, false);
		}

		entnum++;
	}

	sv.num_edicts = entnum;
	sv.time = time;

	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		svs.clients->spawn_parms[i] = spawn_parms[i];

	if (cls.state != ca_dedicated)
	{
		CL_EstablishConnection("local");
		Host_Reconnect_f();
	}

	fs_unlink("/ram/compressed.gz");
	fs_unlink("/ram/uncompressed.sav");
}

// ----------------------------------------------------------------------
// speud (27-06-2004) - Replacing Host_Savegame_f and Host_Loadgame_f end
// ----------------------------------------------------------------------

#ifdef QUAKE2
void SaveGamestate()
{
	char name[256];
	FILE *f;
	int i;
	char comment[SAVEGAME_COMMENT_LENGTH + 1];
	edict_t *ent;

	sprintf(name, "%s/%s.gip", com_gamedir, sv.name);

	Con_Printf("Saving game to %s...\n", name);
	f = fopen(name, "w");
	if (!f)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return;
	}

	fprintf(f, "%i\n", SAVEGAME_VERSION);
	Host_SavegameComment(comment);
	fprintf(f, "%s\n", comment);
	//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
	//		fprintf (f, "%f\n", svs.clients->spawn_parms[i]);
	fprintf(f, "%f\n", skill.value);
	fprintf(f, "%s\n", sv.name);
	fprintf(f, "%f\n", sv.time);

	// write the light styles

	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		if (sv.lightstyles[i])
			fprintf(f, "%s\n", sv.lightstyles[i]);
		else
			fprintf(f, "m\n");
	}

	for (i = svs.maxclients + 1; i < sv.num_edicts; i++)
	{
		ent = EDICT_NUM(i);
		if ((int)ent->v.flags & FL_ARCHIVE_OVERRIDE)
			continue;
		fprintf(f, "%i\n", i);
		ED_Write(f, ent);
		fflush(f);
	}
	fclose(f);
	Con_Printf("done.\n");
}

int LoadGamestate(char *level, char *startspot)
{
	char name[MAX_OSPATH];
	FILE *f;
	char mapname[MAX_QPATH];
	float time, sk;
	char str[32768], *start;
	int i, r;
	edict_t *ent;
	int entnum;
	int version;
	//	float	spawn_parms[NUM_SPAWN_PARMS];

	sprintf(name, "%s/%s.gip", com_gamedir, level);

	Con_Printf("Loading game from %s...\n", name);
	f = fopen(name, "r");
	if (!f)
	{
		Con_Printf("ERROR: couldn't open.\n");
		return -1;
	}

	// BlackAura (08-12-2002) - Replacing fscanf
	// fscanf (f, "%i\n", &version);
	version = DC_ScanFloat(f);
	if (version != SAVEGAME_VERSION)
	{
		fclose(f);
		Con_Printf("Savegame is version %i, not %i\n", version, SAVEGAME_VERSION);
		return -1;
	}
	// BlackAura (08-12-2002) - Replacing fscanf
	// fscanf (f, "%s\n", str);
	DC_ScanString(f, str);
	//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
	//		fscanf (f, "%f\n", &spawn_parms[i]);

	// BlackAura (08-12-2002) - Replacing fscanf
	// fscanf (f, "%f\n", &sk);
	sk = DC_ScanFloat(f);
	Cvar_SetValue("skill", sk);

	// BlackAura (08-12-2002) - Replacing fscanf
	// fscanf (f, "%s\n",mapname);
	// fscanf (f, "%f\n",&time);
	DC_ScanString(f, mapname);
	time = DC_ScanFloat(time);

	SV_SpawnServer(mapname, startspot);

	if (!sv.active)
	{
		Con_Printf("Couldn't load map\n");
		return -1;
	}

	// load the light styles
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		// BlackAura (08-12-2002) - Replacing fscanf
		// fscanf (f, "%s\n", str);
		DC_ScanString(f, str);
		sv.lightstyles[i] = Hunk_Alloc(strlen(str) + 1);
		strcpy(sv.lightstyles[i], str);
	}

	// load the edicts out of the savegame file
	while (!feof(f))
	{
		// BlackAura (08-12-2002) - Replacing fscanf
		// fscanf (f, "%i\n",&entnum);
		entnum = DC_ScanInt(f);
		for (i = 0; i < sizeof(str) - 1; i++)
		{
			r = fgetc(f);
			if (r == EOF || !r)
				break;
			str[i] = r;
			if (r == '}')
			{
				i++;
				break;
			}
		}
		if (i == sizeof(str) - 1)
			Sys_Error("Loadgame buffer overflow");
		str[i] = 0;
		start = str;
		start = COM_Parse(str);
		if (!com_token[0])
			break; // end of file
		if (strcmp(com_token, "{"))
			Sys_Error("First token isn't a brace");

		// parse an edict

		ent = EDICT_NUM(entnum);
		memset(&ent->v, 0, progs->entityfields * 4);
		ent->free = false;
		ED_ParseEdict(start, ent);

		// link it into the bsp tree
		if (!ent->free)
			SV_LinkEdict(ent, false);
	}

	//	sv.num_edicts = entnum;
	sv.time = time;
	fclose(f);

	//	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
	//		svs.clients->spawn_parms[i] = spawn_parms[i];

	return 0;
}

// changing levels within a unit
void Host_Changelevel2_f(void)
{
	char level[MAX_QPATH];
	char _startspot[MAX_QPATH];
	char *startspot;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("changelevel2 <levelname> : continue game on a new level in the unit\n");
		return;
	}
	if (!sv.active || cls.demoplayback)
	{
		Con_Printf("Only the server may changelevel\n");
		return;
	}

	strcpy(level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
		startspot = NULL;
	else
	{
		strcpy(_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

	SV_SaveSpawnparms();

	// save the current level's state
	SaveGamestate();

	// try to restore the new level
	if (LoadGamestate(level, startspot))
		SV_SpawnServer(level, startspot);
}
#endif
