//=============================================================================
/* LOAD/SAVE MENU */

extern void M_DrawCheckbox (int x, int y, int on);
void Host_ReadConfiguration (void);

/* speud - heavy editing begin */
int load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		12
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
int	loadable[MAX_SAVEGAMES];
static qboolean saves_checked = false;

void M_ScanSaves (void)
{
	if(saves_checked)
		return;

	int	i;
	uint16	killed, total;
	char	name[MAX_OSPATH/2], appname[16], mapname[22], kills[20];
	file_t	f;

	vm_dev = maple_enum_dev((int)vmu_port.value, (int)vmu_unit.value);
	vmu_freeblocks = VMU_GetFreeblocks();

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;

		sprintf (name, "/vmu/%c%d/%s_G%02d", (char)vmu_port.value+'a', (char)vmu_unit.value, VMU_NAME, i);
		f = fs_open (name, O_RDONLY);	
		if(f == -1){
				fs_close(f);
				continue;
		}
		loadable[i] = true;
			
		fs_seek(f, 48, SEEK_SET);
		fs_read(f, appname, 16);

		fs_seek(f, 16, SEEK_SET);
		fs_read(f, mapname, 22);

		fs_seek(f, 44, SEEK_SET);
		fs_read(f, &killed, 2);
		fs_read(f, &total, 2);

		memset(m_filenames[i],' ',SAVEGAME_COMMENT_LENGTH);

		memcpy(m_filenames[i], mapname, 22);
		sprintf(kills, "kills:%3i/%3i", killed, total);
		memcpy(m_filenames[i]+22, kills, strlen(kills));

		m_filenames[i][SAVEGAME_COMMENT_LENGTH] = '\0';

		fs_close(f);
	}
	saves_checked = true;
}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	m_state = m_load;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;
	if (cl.intermission)
		return;
	if (svs.maxclients != 1)
		return;
	m_entersound = true;
	m_state = m_save;
	key_dest = key_menu;
	M_ScanSaves ();
}


void M_Load_Draw (void)
{
	int	i;
	char	buffer[32];

	M_DrawPlaque ("gfx/p_load.lmp", false); // Manoel Kasimier

	if (vm_dev == NULL) {
		M_Print (100, 40 + 8*(MAX_SAVEGAMES+2), va("No VMU in %c-%d", (int)vmu_port.value+'A', (int)vmu_unit.value));
		M_Print (52, 40 + 8*(MAX_SAVEGAMES+4), "B button or Escape to exit");
		return;
	}

	for (i=0 ; i< MAX_SAVEGAMES; i++)
		M_Print (16, 40 + 8*i, m_filenames[i]);

	sprintf(buffer, "free blocks in %c-%d: %3d/%3d", (int)vmu_port.value+'A', (int)vmu_unit.value, vmu_freeblocks, vmu_totalblocks);
	M_Print (60, 40 + 8*(MAX_SAVEGAMES+2), buffer);

	M_Print (52, 40 + 8*(MAX_SAVEGAMES+4), "A button or Enter to load");
	M_Print (52, 40 + 8*(MAX_SAVEGAMES+5), "B button or Escape to cancel");
	M_Print (52, 40 + 8*(MAX_SAVEGAMES+6), "X button or Space to delete");

// line cursor
	M_DrawCharacter (8, 40 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Save_Draw (void)
{
	int	i;
	char	buffer[32];

	M_DrawPlaque ("gfx/p_save.lmp", false); // Manoel Kasimier

	if (vm_dev == NULL) {
		M_Print (100, 40 + 8*(MAX_SAVEGAMES+2), va("No VMU in %c-%d", (int)vmu_port.value+'A', (int)vmu_unit.value));
		M_Print (52, 40 + 8*(MAX_SAVEGAMES+4), "B button or Escape to exit");
		return;
	}

	for (i=0 ; i<MAX_SAVEGAMES ; i++)
		M_Print (16, 40 + 8*i, m_filenames[i]);

	sprintf(buffer, "free blocks in %c-%d: %3d/%3d", (int)vmu_port.value+'A', (int)vmu_unit.value, vmu_freeblocks, vmu_totalblocks);
	M_Print (60, 40 + 8*(MAX_SAVEGAMES+2), buffer);

	M_Print (52, 40 + 8*(MAX_SAVEGAMES+4), "A button or Enter to save");
	M_Print (52, 40 + 8*(MAX_SAVEGAMES+5), "B button or Escape to cancel");
	M_Print (52, 40 + 8*(MAX_SAVEGAMES+6), "X button or Space to delete");

// line cursor
	M_DrawCharacter (8, 40 + load_cursor*8, 12+((int)(realtime*4)&1));
}

//=============================================================================
/* VMU MENU */
#define	VMU_ITEMS 6
int	vmu_cursor;

void M_Vmu_f (void)
{
	key_dest = key_menu;
	m_state = m_vmu;
	m_entersound = true;
}

void M_Vmu_Draw (void)
{
	int y=32;

	M_DrawPlaque ("gfx/p_option.lmp", true); // Manoel Kasimier

	y += 8; M_Print (16, y, "     Default port-unit"); M_Print (220, y, va("%c-%d", (int)vmu_port.value+'A', (int)vmu_unit.value));
	y += 8; M_Print (16, y, "    Auto-save settings"); M_DrawCheckbox (220, y, vmu_autosave.value);
	y += 8; M_Print (16, y, "         Save settings");
	y += 8; M_Print (16, y, "         Load settings");
	y += 8; M_Print (16, y, "       Quick save game");
	y += 8; M_Print (16, y, "       Quick load game");

// cursor
	M_DrawCharacter (200, 40 + vmu_cursor*8, 12+((int)(realtime*4)&1));
}

void M_Vmu_Change (int dir)
{
	S_LocalSound ("misc/menu3.wav");

	switch (vmu_cursor)
	{
	case 0:	// port/untit
		if (dir > 0) {
			if (vmu_unit.value == 2) {
				if (vmu_port.value < 3) {
					Cvar_SetValue ("vmu_port", vmu_port.value+1);
					Cvar_SetValue ("vmu_unit", 1);
				}
			} else {
				vmu_unit.value ++;
			}
		} else {
			if (vmu_unit.value == 1) {
				if (vmu_port.value > 0) {
					Cvar_SetValue ("vmu_port", vmu_port.value-1);
					Cvar_SetValue ("vmu_unit", 2);
				}
			} else {
				vmu_unit.value --;
			}
		}
		break;
	case 1:	// auto-save settings
		Cvar_SetValue ("vmu_autosave", !vmu_autosave.value);
		break;
	case 2: // save settings
		Host_WriteConfiguration();
		break;
	case 3: // load settings
		Host_ReadConfiguration();
		break;
	case 4: // quick save game
		Cbuf_AddText ("save quick\n");
		break;
	case 5: // quick load game
		Cbuf_AddText ("load quick\n");
		break;
	}
}

void M_Vmu_Key (int k)
{
	switch (k)
	{
		
	case K_ALT:
	case K_SPACE:
	case K_ESCAPE:
		M_Menu_Options_f ();
		break;

	case K_ENTER:
		m_entersound = true;
		M_Vmu_Change (1);
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		vmu_cursor--;
		if (vmu_cursor < 0)
			vmu_cursor = VMU_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		vmu_cursor++;
		if (vmu_cursor >= VMU_ITEMS)
			vmu_cursor = 0;
		break;

	case K_LEFTARROW:
		if (vmu_cursor==0 || vmu_cursor==1 || vmu_cursor==4)
			M_Vmu_Change (-1);
		break;

	case K_RIGHTARROW:
		if (vmu_cursor==0 || vmu_cursor==1 || vmu_cursor==4)
			M_Vmu_Change (1);
		break;
	}
}