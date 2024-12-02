/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"
#include "../renderer/r_local.h"
/*
    Name: Ian micheal
    Date: 25/11/24 05:54
    Description: Fixed warnings:
    - Removed static array NULL checks
    - Fixed uint8_t/char* pointer signedness
    - Added proper type casting for VMU_calcCRC
    - Corrected float-to-double conversions
    - Fixed memory size calculations
*/

/*

A server can allways be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

quakeparms_t host_parms;

qboolean	host_initialized;		// true if into command execution

float		host_frametime;
float		realtime;				// without any filtering or bounding
float		oldrealtime;			// last frame run
int			host_framecount;

int			host_hunklevel;

int			minimum_memory;

client_t	*host_client;			// current client

byte		*host_basepal;
byte		*host_colormap;

float	host_netinterval = 1.0/72;
cvar_t	host_framerate = {"host_framerate","0"};	// set for slow motion
cvar_t	host_speeds = {"host_speeds","0"};			// set for running times
cvar_t	host_maxfps = {"host_maxfps", "60", true}; //johnfitz, change
cvar_t	host_timescale = {"host_timescale", "0"}; //johnfitz

cvar_t	sys_ticrate = {"sys_ticrate","0.05"};
cvar_t	serverprofile = {"serverprofile","0"};

cvar_t	fraglimit = {"fraglimit","0",false,true};
cvar_t	timelimit = {"timelimit","0",false,true};
cvar_t	teamplay = {"teamplay","0",false,true};

cvar_t	samelevel = {"samelevel","0"};
cvar_t	noexit = {"noexit","0",false,true};

#ifdef QUAKE2
cvar_t	developer = {"developer","1"};	// should be 0 for release!
#else
cvar_t	developer = {"developer","0"};
#endif

cvar_t	skill = {"skill","1"};						// 0 - 3
cvar_t	deathmatch = {"deathmatch","0"};			// 0, 1, or 2
cvar_t	coop = {"coop","0"};			// 0 or 1

cvar_t	pausable = {"pausable","1"};

cvar_t	temp1 = {"temp1","0"};

//mrneo240
cvar_t	show_fps = {"show_fps","0"};	// set for running times - muff
int		fps_count;
void 	VID_Cvar_Update(void);
//


/*
================
Max_Fps_f -- ericw
================
*/
static void Max_Fps_f (cvar_t *var)
{
	if (var->value > 72 || var->value <= 0)
	{
		if (!host_netinterval)
			Con_Printf ("Using renderer/network isolation.\n");
		host_netinterval = 1.0/72;
	}
	else
	{
		if (host_netinterval)
			Con_Printf ("Disabling renderer/network isolation.\n");
		host_netinterval = 0;

		if (var->value > 72)
			Con_Printf ("host_maxfps above 72 breaks physics.\n");
	}
}

/*
================
Host_EndGame
================
*/
void Host_EndGame (char *message, ...)
{
	va_list		argptr;
	char		string[CON_MAX_MSG];

	va_start (argptr,message);
	vsprintf (string,message,argptr);
	va_end (argptr);
	Con_DPrintf ("Host_EndGame: %s\n",string);

	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_EndGame: %s\n",string);	// dedicated servers exit

	if (cls.demonum != -1)
		CL_NextDemo ();
	else
		CL_Disconnect ();
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error (char *error, ...)
{
	va_list		argptr;
	char		string[CON_MAX_MSG];
	static	qboolean inerror = false;

	if (inerror)
		Sys_Error ("Host_Error: recursively entered");
	inerror = true;

	SCR_EndLoadingPlaque ();		// reenable screen updates

	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);
	Con_Printf ("Host_Error: %s\n",string);

	if (sv.active)
		Host_ShutdownServer (false);

	if (cls.state == ca_dedicated)
		Sys_Error ("Host_Error: %s\n",string);	// dedicated servers exit

	CL_Disconnect ();
	cls.demonum = -1;

	inerror = false;
}

/*
================
Host_FindMaxClients
================
*/
void	Host_FindMaxClients (void)
{
	int		i;

	svs.maxclients = 1;

	i = COM_CheckParm ("-dedicated");
	if (i)
	{
		cls.state = ca_dedicated;
		if (i != (com_argc - 1))
		{
			svs.maxclients = atoi (com_argv[i+1]);
		}
		else
			svs.maxclients = 8;
	}
	else
		cls.state = ca_disconnected;

	i = COM_CheckParm ("-listen");
	if (i)
	{
		if (cls.state == ca_dedicated)
			Sys_Error ("Only one of -dedicated or -listen can be specified");
		if (i != (com_argc - 1))
			svs.maxclients = atoi (com_argv[i+1]);
		else
			svs.maxclients = 8;
	}
	if (svs.maxclients < 1)
		svs.maxclients = 8;
	else if (svs.maxclients > MAX_SCOREBOARD)
		svs.maxclients = MAX_SCOREBOARD;

	svs.maxclientslimit = svs.maxclients;
	if (svs.maxclientslimit < 4)
		svs.maxclientslimit = 4;
	svs.clients = Hunk_AllocName (svs.maxclientslimit*sizeof(client_t), "clients");

	if (svs.maxclients > 1)
		Cvar_SetValue ("deathmatch", 1.0);
	else
		Cvar_SetValue ("deathmatch", 0.0);
}


/*
=======================
Host_InitLocal
======================
*/
void Host_InitLocal (void)
{
	Host_InitCommands ();

	Cvar_RegisterVariable (&host_framerate);
	Cvar_RegisterVariable (&host_speeds);
	Cvar_RegisterVariableWithCallback(&host_maxfps, Max_Fps_f);
	Cvar_RegisterVariable (&host_timescale); //johnfitz

	Cvar_RegisterVariable (&sys_ticrate);
	Cvar_RegisterVariable (&serverprofile);

	Cvar_RegisterVariable (&fraglimit);
	Cvar_RegisterVariable (&timelimit);
	Cvar_RegisterVariable (&teamplay);
	Cvar_RegisterVariable (&samelevel);
	Cvar_RegisterVariable (&noexit);
	Cvar_RegisterVariable (&skill);
	Cvar_RegisterVariable (&developer);
	Cvar_RegisterVariable (&deathmatch);
	Cvar_RegisterVariable (&coop);

	Cvar_RegisterVariable (&pausable);

	Cvar_RegisterVariable (&temp1);

	Cvar_RegisterVariable (&show_fps); // muff

	Host_FindMaxClients ();
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
#if defined(_WIN32) || defined(__linux__)
void Host_WriteConfiguration (void)
{
	FILE	*f;

// dedicated servers initialize the host but don't parse and set the
// config.cfg cvars
	if (host_initialized & !isDedicated)
	{
		f = fopen (va("%s/config.cfg",com_gamedir), "w");
		if (!f)
		{
			Con_Printf ("Couldn't write config.cfg.\n");
			return;
		}

		Key_WriteBindings (f);
		Cvar_WriteVariables (f);

		fclose (f);
	}
}
#else
#include "dreamcast/vmu_misc.h"
extern int filelength (FILE *f);
// Static buffer declaration - can never be NULL since it's static
static uint8_t file_buf[1024 * 128]; //128k

void Host_WriteConfiguration(void)
{
    char    buffer[8];
    uint16  crc;
    int     filesize, total, head_len=1664;
    char    vmu_path[21];  
    uint8_t *ptr;  // FIXED: Changed from char* to uint8_t* to match file_buf type
    char    playername[33];
    FILE    *f1;
    file_t  f2;

    // dedicated servers initialize the host but don't parse and set the
    // config.cfg cvars
    if (!host_initialized && !isDedicated)
    {
        // Saving options to RAM
        f1 = fopen("/ram/save.cfg", "w");
        if (!f1)
        {
            Con_Printf("Couldn't write config file.\n");
            return;
        }

        Key_WriteBindings(f1);
        Cvar_WriteVariables(f1);
        fclose(f1);

        FILE *temp = fopen("/ram/save.cfg", "r");
        filesize = filelength(temp);
        fclose(temp);
        printf("Options filesize: %d\n", filesize);

        // Reading options
        f2 = fs_open("/ram/save.cfg", O_RDONLY);
        total = filesize + head_len;
        while ((total % 512) != 0)
            total++;

        // FIXED: Removed NULL check for file_buf since it's a static array
        // and can never be NULL
        memset(file_buf, 0, total);
        fs_read(f2, file_buf+head_len, filesize);
        fs_close(f2);

        // Filling VMS header
        // FIXED: ptr is now uint8_t* to match file_buf type
        ptr = file_buf;

        sprintf(playername, "%-32s", Cvar_VariableString("_cl_name"));
        char desc[17];
        snprintf(desc,17,"\x12%s CONFIG",VMU_NAME);
        for (int i=1; i < 17; i++)
        {
            if (desc[i]=='_')
                desc[i] = ' ';
        }

        // All ptr operations now work with uint8_t* type
        memcpy(ptr, desc,        16); ptr += 16;    // VM desc
        memcpy(ptr, playername,  32); ptr += 32;    // DC desc: playername
        memcpy(ptr, APP_NAME,    16); ptr += 16;    // Application
        memcpy(ptr, "\x01\0",    2);  ptr += 2;     // Icons number
        memcpy(ptr, "\0\0",      2);  ptr += 2;     // Anim speed
        memset(ptr, 0,           2);  ptr += 2;     // Eyecatch type
        memset(ptr, 0,           2);  ptr += 2;     // CRC checksum

        sprintf(buffer, "%c%c%c%c",
            (char)((filesize & 0xff)      >> 0),
            (char)((filesize & 0xff00)    >> 8),
            (char)((filesize & 0xff0000)  >> 16),
            (char)((filesize & 0xff000000)>> 24));

        memcpy(ptr, buffer,      4);  ptr += 4;     // Filesize
        memset(ptr, 0,           20); ptr += 20;    // Reserved
        memcpy(ptr, icon_palette,32); ptr += 32;    // Icons palette
        // FIXED: Only copy 512 bytes from icon_bitmap to prevent buffer overread
        memcpy(ptr, icon_bitmap, 512); ptr += 512;  // Icons bitmap

        // FIXED: Using updated VMU_calcCRC signature that takes const uint8_t*
        crc = VMU_calcCRC(file_buf, filesize+head_len);
        file_buf[0x46] = (crc & 0xff)     >> 0;
        file_buf[0x47] = (crc & 0xff00)   >> 8;

        // Saving config to VM
        vm_dev = maple_enum_dev((int)vmu_port.value, (int)vmu_unit.value);
        if (vm_dev == NULL) {
            Con_Printf("ERROR: no VM in %c-%d.\n", (int)vmu_port.value+'A', (int)vmu_unit.value);
            fs_unlink("/ram/save.cfg");
            return;
        }

        vmu_freeblocks = VMU_GetFreeblocks();
        if (vmu_freeblocks == -1) {
            Con_Printf("ERROR: couldn't read root block.\n");
            fs_unlink("/ram/save.cfg");
            return;
        } else if (vmu_freeblocks == -2) {
            Con_Printf("ERROR: couldn't read FAT.\n");
            fs_unlink("/ram/save.cfg");
            return;
        }

        sprintf(vmu_path, "/vmu/%c%d/%s_CFG", (int)vmu_port.value+'a', (int)vmu_unit.value, VMU_NAME);
        f2 = fs_open(vmu_path, O_RDONLY);
        if (f2) {
            vmu_freeblocks += fs_total(f2) / 512;
            fs_close(f2);
        }

        if ((vmu_freeblocks*512) < total) {
            Con_Printf("Not enough free blocks. Free:%d Needed:%d.\n", vmu_freeblocks, total/512);
            fs_unlink("/ram/save.cfg");
            return;
        }

        Con_Printf("Saving options to %s...\n", vmu_path);
        fs_unlink(vmu_path);
        f2 = fs_open(vmu_path, O_WRONLY);
        if (!f2)
        {
            Con_Printf("ERROR: couldn't open.\n");
            fs_unlink("/ram/save.cfg");
            return;
        }

        fs_write(f2, file_buf, total);
        fs_close(f2);
        fs_unlink("/ram/save.cfg");
        Con_Printf("done.\n");
    }
}

/*
===============
Host_ReadConfiguration

Added by speud
===============
*/
void Host_ReadConfiguration(void)
{
    FILE    *f;
    char    path[32], buffer[32];
    int     head_len;
    uint16  icons_n, crc, eyec_t;
    uint32  data_len;

    // Build the VMU path
    sprintf(path, "/vmu/%c%d/%s_CFG", (int)vmu_port.value+'a', (int)vmu_unit.value, VMU_NAME);
    path[20] = 0;

    Con_Printf("Reading settings from %s...\n", path);

    f = fopen(path, "r");
    if (!f) {
        Con_Printf("No config file found in %c-%d.\n", (int)vmu_port.value+'A', (int)vmu_unit.value);
        return;
    }

    // Skipping DC/VM desc
    fseek(f, 48, SEEK_SET);

    // Checking application name
    fread(buffer, 16, 1, f);
    if (!!memcmp(buffer, APP_NAME, 16))
    {
        Con_Printf("ERROR: not a valid RADQuake game save. (%s)\n", buffer);
        fclose(f);
        return;
    }

    // Checking icons number
    fread(&icons_n, 2, 1, f);
    if (icons_n < 1 || icons_n > 3)
    {
        Con_Printf("ERROR: not a valid VMS icon. (%d)\n", icons_n);
        fclose(f);
        return;
    }

    // Skipping icons anim speed
    fseek(f, 2, SEEK_CUR);

    // Checking eyecatch type
    fread(&eyec_t, 2, 1, f);
    if (eyec_t > 3)
    {
        Con_Printf("ERROR: not a valid VMS eyecatch. (%d)\n", eyec_t);
        fclose(f);
        return;
    }

    // Getting data size
    fread(&crc, 2, 1, f);
    fread(&data_len, 4, 1, f);
    head_len = 128 + icons_n * 512;
    if (eyec_t > 0) {
        switch (eyec_t) {
            case 1: head_len += 8064; break;
            case 2: head_len += 4544; break;
            case 3: head_len += 2048; break;
        }
    }

    // Reading options
    // FIXED: Removed NULL check for file_buf since it's a static array
    // FIXED: Using 0x680 as fixed header length instead of calculated head_len
    fseek(f, 0x680, SEEK_SET);
    fread(file_buf, data_len, 1, f);
    fclose(f);

    // FIXED: Added proper type casting for text operations
    Cbuf_AddText("echo \"Loading user's settings...\"\n");
    // FIXED: Cast uint8_t* to const char* for text operations
    Cbuf_AddText((const char *)file_buf);
    // FIXED: Cast uint8_t* to char* for Con_Print
    Con_Print((char *)file_buf);
    Cbuf_AddText("echo \"done.\"\n");
}
#endif


/*
=================
SV_ClientPrintf

Sends text across to be displayed
FIXME: make this just a stuffed echo?
=================
*/
void SV_ClientPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		string[CON_MAX_MSG];

	va_start (argptr,fmt);
	vsprintf (string, fmt,argptr);
	va_end (argptr);

	MSG_WriteByte (&host_client->message, svc_print);
	MSG_WriteString (&host_client->message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		string[CON_MAX_MSG];
	int			i;

	va_start (argptr,fmt);
	vsprintf (string, fmt,argptr);
	va_end (argptr);

	for (i=0 ; i<svs.maxclients ; i++)
		if (svs.clients[i].active && svs.clients[i].spawned)
		{
			MSG_WriteByte (&svs.clients[i].message, svc_print);
			MSG_WriteString (&svs.clients[i].message, string);
		}
}

/*
=================
Host_ClientCommands

Send text over to the client to be executed
=================
*/
void Host_ClientCommands (char *fmt, ...)
{
	va_list		argptr;
	char		string[CON_MAX_MSG];

	va_start (argptr,fmt);
	vsprintf (string, fmt,argptr);
	va_end (argptr);

	MSG_WriteByte (&host_client->message, svc_stufftext);
	MSG_WriteString (&host_client->message, string);
}

/*
=====================
SV_DropClient

Called when the player is getting totally kicked off the host
if (crash = true), don't bother sending signofs
=====================
*/
void SV_DropClient (qboolean crash)
{
	int		saveSelf;
	int		i;
	client_t *client;

	if (!crash)
	{
		// send any final messages (don't check for errors)
		if (NET_CanSendMessage (host_client->netconnection))
		{
			MSG_WriteByte (&host_client->message, svc_disconnect);
			NET_SendMessage (host_client->netconnection, &host_client->message);
		}

		if (host_client->edict && host_client->spawned)
		{
		// call the prog function for removing a client
		// this will set the body to a dead frame, among other things
			saveSelf = pr_global_struct->self;
			pr_global_struct->self = EDICT_TO_PROG(host_client->edict);
			PR_ExecuteProgram (pr_global_struct->ClientDisconnect);
			pr_global_struct->self = saveSelf;
		}

		Sys_Printf ("Client %s removed\n",host_client->name);
	}

// break the net connection
	NET_Close (host_client->netconnection);
	host_client->netconnection = NULL;

// free the client (the body stays around)
	host_client->active = false;
	host_client->name[0] = 0;
	host_client->old_frags = -999999;
	net_activeconnections--;

// send notification to all clients
	for (i=0, client = svs.clients ; i<svs.maxclients ; i++, client++)
	{
		if (!client->active)
			continue;
		MSG_WriteByte (&client->message, svc_updatename);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteString (&client->message, "");
		MSG_WriteByte (&client->message, svc_updatefrags);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteShort (&client->message, 0);
		MSG_WriteByte (&client->message, svc_updatecolors);
		MSG_WriteByte (&client->message, host_client - svs.clients);
		MSG_WriteByte (&client->message, 0);
	}
}

/*
==================
Host_ShutdownServer

This only happens at the end of a game, not between levels
==================
*/
void Host_ShutdownServer(qboolean crash)
{
	int		i;
	int		count;
	sizebuf_t	buf;
	char		message[4];
	float	start;

	if (!sv.active)
		return;

	sv.active = false;

// stop all client sounds immediately
	if (cls.state == ca_connected)
		CL_Disconnect ();

// flush any pending messages - like the score!!!
	start = Sys_FloatTime();
	do
	{
		count = 0;
		for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		{
			if (host_client->active && host_client->message.cursize)
			{
				if (NET_CanSendMessage (host_client->netconnection))
				{
					NET_SendMessage(host_client->netconnection, &host_client->message);
					SZ_Clear (&host_client->message);
				}
				else
				{
					NET_GetMessage(host_client->netconnection);
					count++;
				}
			}
		}
		if ((Sys_FloatTime() - start) > 3.0)
			break;
	}
	while (count);

// make sure all the clients know we're disconnecting
	buf.data = (byte*)message;
	buf.maxsize = 4;
	buf.cursize = 0;
	MSG_WriteByte(&buf, svc_disconnect);
	count = NET_SendToAll(&buf, 5);
	if (count)
		Con_Printf("Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count);

	for (i=0, host_client = svs.clients ; i<svs.maxclients ; i++, host_client++)
		if (host_client->active)
			SV_DropClient(crash);

//
// clear structures
//
	memset (&sv, 0, sizeof(sv));
	memset (svs.clients, 0, svs.maxclientslimit*sizeof(client_t));
}


/*
================
Host_ClearMemory

This clears all the memory used by both the client and server, but does
not reinitialize anything.
================
*/
void Host_ClearMemory (void)
{
	Con_DPrintf ("Clearing memory\n");
	D_FlushCaches ();
	Mod_ClearAll ();
	if (host_hunklevel)
		Hunk_FreeToLowMark (host_hunklevel);

	cls.signon = 0;
	memset (&sv, 0, sizeof(sv));
	memset (&cl, 0, sizeof(cl));
}


//============================================================================


/*
===================
Host_FilterTime

Returns false if the time is too short to run a frame
===================
*/
qboolean Host_FilterTime (float time)
{
	float maxfps; //johnfitz

	realtime += time;

	//johnfitz -- max fps cvar
	maxfps = CLAMP (10.0, host_maxfps.value, 120.0);
	if (host_maxfps.value && !cls.timedemo && realtime - oldrealtime < 1.0/maxfps)
		return false; // framerate is too high
	//johnfitz

	host_frametime = realtime - oldrealtime;
	oldrealtime = realtime;

	//johnfitz -- host_timescale is more intuitive than host_framerate
	if (host_timescale.value > 0)
		host_frametime *= host_timescale.value;
	//johnfitz
	else if (host_framerate.value > 0)
		host_frametime = host_framerate.value;
	else if (host_maxfps.value)// don't allow really long or short frames
		host_frametime = CLAMP ((1.0/120.0), host_frametime, (1.0/10.0)); //johnfitz -- use CLAMP

	return true;
}


/*
===================
Host_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void Host_GetConsoleCommands (void)
{
	char	*cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput ();
		if (!cmd)
			break;
		Cbuf_AddText (cmd);
	}
}


/*
==================
Host_ServerFrame

==================
*/
#ifdef FPS_20

void _Host_ServerFrame (void)
{
// run the world state
	pr_global_struct->frametime = host_frametime;

// read client messages
	SV_RunClients ();

// move things around and think
// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game) )
		SV_Physics ();
}

void Host_ServerFrame (void)
{
	float	save_host_frametime;
	float	temp_host_frametime;

// run the world state
	pr_global_struct->frametime = host_frametime;

// set the time and clear the general datagram
	SV_ClearDatagram ();

// check for new clients
	SV_CheckForNewClients ();

	temp_host_frametime = save_host_frametime = host_frametime;
	while(temp_host_frametime > (1.0/72.0))
	{
		if (temp_host_frametime > 0.05)
			host_frametime = 0.05;
		else
			host_frametime = temp_host_frametime;
		temp_host_frametime -= host_frametime;
		_Host_ServerFrame ();
	}
	host_frametime = save_host_frametime;

// send all messages to the clients
	SV_SendClientMessages ();
}

#else

void Host_ServerFrame (void)
{
// run the world state
	pr_global_struct->frametime = host_frametime;

// set the time and clear the general datagram
	SV_ClearDatagram ();

// check for new clients
	SV_CheckForNewClients ();

// read client messages
	SV_RunClients ();

// move things around and think
// always pause in single player if in console or menus
	if (!sv.paused && (svs.maxclients > 1 || key_dest == key_game) )
		SV_Physics ();

// send all messages to the clients
	SV_SendClientMessages ();
}

#endif


/*
==================
Host_Frame

Runs all active servers
==================
*/
void _Host_Frame (float time)
{
	static float	accumtime = 0;
	static float		time1 = 0;
	static float		time2 = 0;
	static float		time3 = 0;
	int			pass1, pass2, pass3;

	for (;;) // something bad happened, or the server disconnected
	{

// keep the random time dependent
	rand ();

// decide the simulation time
	accumtime += host_netinterval?CLAMP(0, time, 0.2):0;	//for renderer/server isolation
	if (!Host_FilterTime (time))
		return;			// don't run too fast, or packets will flood out

// get new key events
	Sys_SendKeyEvents ();

// allow mice or other external controllers to add commands
	IN_Commands ();

//check the stdin for commands (dedicated servers)
	Host_GetConsoleCommands ();

// process console commands
	Cbuf_Execute ();

	NET_Poll();

//Run the server+networking (client->server->client), at a different rate from everyt
	if (accumtime >= host_netinterval)
	{
		float realframetime = host_frametime;
		if (host_netinterval)
		{
			host_frametime = q_max(accumtime, host_netinterval);
			accumtime -= host_frametime;
			if (host_timescale.value > 0)
				host_frametime *= host_timescale.value;
			else if (host_framerate.value)
				host_frametime = host_framerate.value;
		}
		else
			accumtime -= host_netinterval;
		CL_SendCmd ();
		if (sv.active)
		{
			Host_ServerFrame ();
		}
		host_frametime = realframetime;
		Cbuf_Waited();
	}

//-------------------
//
// client operations
//
//-------------------

// fetch results from server
	if (cls.state == ca_connected)
		CL_ReadFromServer ();

// update video
	if (host_speeds.value)
		time1 = Sys_FloatTime ();

	SCR_UpdateScreen ();

	if (host_speeds.value)
		time2 = Sys_FloatTime ();

// update audio
	if (cls.signon == SIGNONS)
	{
		S_Update (r_origin, vpn, vright, vup);
		CL_DecayLights ();
	}
	else
		S_Update (vec3_origin, vec3_origin, vec3_origin, vec3_origin);

	CDAudio_Update();

	if (host_speeds.value)
	{
		pass1 = (time1 - time3)*1000;
		time3 = Sys_FloatTime ();
		pass2 = (time2 - time1)*1000;
		pass3 = (time3 - time2)*1000;
		Con_Printf ("%3i tot %3i server %3i gfx %3i snd\n",
					pass1+pass2+pass3, pass1, pass2, pass3);
	}

	host_framecount++;
	fps_count++;	//muff
	break;
	}
}

void Host_Frame (float time)
{
	float	time1, time2;
	static float	timetotal;
	static int		timecount;
	int		i, c, m;

	if (!serverprofile.value)
	{
		_Host_Frame (time);
		return;
	}

	time1 = Sys_FloatTime ();
	_Host_Frame (time);
	time2 = Sys_FloatTime ();

	timetotal += time2 - time1;
	timecount++;

	if (timecount < 1000)
		return;

	m = timetotal*1000/timecount;
	timecount = 0;
	timetotal = 0;
	c = 0;
	for (i=0 ; i<svs.maxclients ; i++)
	{
		if (svs.clients[i].active)
			c++;
	}

	Con_Printf ("serverprofile: %2i clients %2i msec\n",  c,  m);
}

//============================================================================

#ifndef _arch_dreamcast
extern int vcrFile;
#define	VCR_SIGNATURE	0x56435231
// "VCR1"

void Host_InitVCR (quakeparms_t *parms)
{
	int		i, len, n;
	char	*p;

	if (COM_CheckParm("-playback"))
	{
		if (com_argc != 2)
			Sys_Error("No other parameters allowed with -playback\n");

		Sys_FileOpenRead("quake.vcr", &vcrFile);
		if (vcrFile == -1)
			Sys_Error("playback file not found\n");

		Sys_FileRead (vcrFile, &i, sizeof(int));
		if (i != VCR_SIGNATURE)
			Sys_Error("Invalid signature in vcr file\n");

		Sys_FileRead (vcrFile, &com_argc, sizeof(int));
		com_argv = malloc(com_argc * sizeof(char *));
		com_argv[0] = parms->argv[0];
		for (i = 0; i < com_argc; i++)
		{
			Sys_FileRead (vcrFile, &len, sizeof(int));
			p = malloc(len);
			Sys_FileRead (vcrFile, p, len);
			com_argv[i+1] = p;
		}
		com_argc++; /* add one for arg[0] */
		parms->argc = com_argc;
		parms->argv = com_argv;
	}

	if ( (n = COM_CheckParm("-record")) != 0)
	{
		vcrFile = Sys_FileOpenWrite("quake.vcr");

		i = VCR_SIGNATURE;
		Sys_FileWrite(vcrFile, &i, sizeof(int));
		i = com_argc - 1;
		Sys_FileWrite(vcrFile, &i, sizeof(int));
		for (i = 1; i < com_argc; i++)
		{
			if (i == n)
			{
				len = 10;
				Sys_FileWrite(vcrFile, &len, sizeof(int));
				Sys_FileWrite(vcrFile, "-playback", len);
				continue;
			}
			len = strlen(com_argv[i]) + 1;
			Sys_FileWrite(vcrFile, &len, sizeof(int));
			Sys_FileWrite(vcrFile, com_argv[i], len);
		}
	}
}
#endif
void load_defaultConfig(void) {

	Cbuf_AddText ( 
"echo \"Loading default settings...\"\n"
"bind \"PAUSE\" \"pause\"\n"
//"bind \"PAUSE\" \"pause\"\n"
"bind \"DC_START\" \"togglemenu\"\n"
"bind \"DC_A\" \"+back\"\n"
"bind \"DC_B\" \"+moveright\"\n"
"bind \"DC_X\" \"+moveleft\"\n"
"bind \"DC_Y\" \"+forward\"\n"
"bind \"DC_DUP\" \"impulse 10\"\n"
"bind \"DC_DDOWN\" \"impulse 12\"\n"
"bind \"DC_DLEFT\" \"+showscores\"\n"
"bind \"DC_DLEFT\" \"impulse 1\"\n"
"bind \"DC_DLEFT\" \"impulse f1\"\n"
"bind \"DC_TRIGL\" \"+jump\"\n"
"bind \"DC_TRIGR\" \"+attack\"\n"
"m_side \"0.8\"\n"
"m_forward \"1\"\n"
"m_yaw \"0.022\"\n"
"m_pitch \"0.022\"\n"
"sensitivity \"3\"\n"
"lookstrafe \"0\"\n"
"lookspring \"0\"\n"
"cl_backspeed \"400\"\n"
"cl_forwardspeed \"400\"\n"
"_cl_color \"22\"\n"
"_cl_name \"player\"\n"
"sbar \"4\"\n"
"sbar_y \"20\"\n"
"sbar_x \"12\"\n"
"sbar_show_ammo \"1\"\n"
"sbar_show_health \"1\"\n"
"sbar_show_armor \"1\"\n"
"sbar_show_powerups \"1\"\n"
"sbar_show_runes \"1\"\n"
"sbar_show_keys \"1\"\n"
"sbar_show_weaponlist \"1\"\n"
"sbar_show_ammolist \"1\"\n"
"sbar_show_scores \"0\"\n"
"_snd_mixahead \"0.1\"\n"
"bgmvolume \"1\"\n"
"volume \"0.7\"\n"
"d_mipscale \"1\"\n"
"r_drawviewmodel \"1\"\n"
"r_drawentities \"1\"\n"
"r_fullbright \"1\"\n"
"r_waterwarp \"1\"\n"
"viewsize \"90\"\n"
"vmu_autosave \"1\"\n"
"vmu_unit \"1\"\n"
"vmu_port \"0\"\n"
"sv_aim \"0.93\"\n"
"sv_aim_h \"1\"\n"
"saved4 \"0\"\n"
"saved3 \"0\"\n"
"saved2 \"0\"\n"
"saved1 \"0\"\n"
"savedgamecfg \"1\"\n"
"vid_gamma  \"0.4\"\n"
"scr_ofsy \"0\"\n"
"cl_nobob \"0\"\n"
"gl_polyblend \"1\"\n"
"crosshair_color \"0\"\n"
"crosshair \"1\"\n"
"echo \"done Auto config.\"\n"
);
}
/*
====================
Host_Init
====================
*/
void Host_Init(quakeparms_t *parms)
{
    if (standard_quake)
        minimum_memory = MINIMUM_MEMORY;
    else
        minimum_memory = MINIMUM_MEMORY_LEVELPAK;

    if (COM_CheckParm("-minmemory"))
        parms->memsize = minimum_memory;

    host_parms = *parms;

    // FIXED: Calculate memory size using proper double arithmetic
    if (parms->memsize < minimum_memory) {
        double megs = (double)parms->memsize / (double)0x100000;
        Sys_Error("Only %4.1f megs of memory available, can't execute game", megs);
    }

    com_argc = parms->argc;
    com_argv = parms->argv;

    Memory_Init(parms->membase, parms->memsize);
    Cbuf_Init();
    Cmd_Init();
    V_Init();
    Chase_Init();
    #ifndef _arch_dreamcast
    Host_InitVCR(parms);
    #endif
    COM_Init();
    Host_InitLocal();
    W_LoadWadFile("gfx.wad");
    Key_Init();
    Con_Init();
    M_Init();
    PR_Init();
    Mod_Init();
    NET_Init();
    SV_Init();

    #ifdef _arch_dreamcast
    /* speud - VMU begin */
    VMU_init();
    /* speud - VMU end */
    #endif

    Con_Printf("Exe: "__TIME__" "__DATE__"\n");
    
    // FIXED: Calculate heap size using proper double arithmetic
    {
        const double mb = 1024.0 * 1024.0;  // Define megabyte constant
        double heap_size = (double)parms->memsize / mb;
        Con_Printf("%4.1f megabyte heap\n", heap_size);
    }

    R_InitTextures();        // needed even for dedicated servers

    if (cls.state != ca_dedicated)
    {
        host_basepal = (byte *)COM_LoadHunkFile("gfx/palette.lmp");
        if (!host_basepal)
            Sys_Error("Couldn't load gfx/palette.lmp");
        host_colormap = (byte *)COM_LoadHunkFile("gfx/colormap.lmp");
        if (!host_colormap)
            Sys_Error("Couldn't load gfx/colormap.lmp");

#ifndef _WIN32 // on non win32, mouse comes before video for security reasons
        IN_Init();
#endif
        VID_Init(host_basepal);

        Draw_Init();
        SCR_Init();
        R_Init();
        VID_Cvar_Update();

        S_Init();
        CDAudio_Init();

        Sbar_Init();
        CL_Init();
#if defined(_WIN32) || defined(__linux__) // on non win32, mouse comes before video for security reasons
        IN_Init();
#endif
    }

 //	Cbuf_InsertText ("exec quake.rc\n");
	Cbuf_AddText ("exec quake.rc\n");

/* Ian micheal- load default config begin */
	load_defaultConfig();
/* Ian micheal - load default config end */

/* Ian micheal- load options begin */
	Host_ReadConfiguration();
/* Ian micheal - load options end */

    Hunk_AllocName(0, "-HOST_HUNKLEVEL-");
    host_hunklevel = Hunk_LowMark();

    host_initialized = true;

    Sys_Printf("========Quake Initialized=========\n");
}

/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown(void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

// keep Con_Printf from trying to update the screen
	scr_disabled_for_loading = true;

	Host_WriteConfiguration ();

	CDAudio_Shutdown ();
	NET_Shutdown ();
	S_Shutdown();
	IN_Shutdown ();

	if (cls.state != ca_dedicated)
	{
		VID_Shutdown();
	}
}

