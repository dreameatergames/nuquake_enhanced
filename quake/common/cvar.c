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
// cvar.c -- dynamic variable tracking

#include "quakedef.h"

cvar_t	*cvar_vars;
char	*cvar_null_string = "";

/*
============
Cvar_FindVar
============
*/
cvar_t *Cvar_FindVar (const char *var_name)
{
	cvar_t	*var;
	
	for (var=cvar_vars ; var ; var=var->next)
		if (!strcmp (var_name, var->name))
			return var;

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float	Cvar_VariableValue (const char *var_name)
{
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return atof (var->string);
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (const char *var_name)
{
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return cvar_null_string;
	return var->string;
}


/*
============
Cvar_CompleteVariable
============
*/
char *Cvar_CompleteVariable (const char *partial)
{
	cvar_t		*cvar;
	int			len;
	
	len = strlen(partial);
	
	if (!len)
		return NULL;
		
// check functions
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!strncmp (partial,cvar->name, len))
			return cvar->name;

	return NULL;
}


/*
============
Cvar_Set
============
*/
void Cvar_Set (const char *var_name, const char *value)
{
	cvar_t	*var;
	qboolean changed;
	
	var = Cvar_FindVar (var_name);
	if (!var)
	{	// there is an error in C code if this happens
		Con_Printf ("Cvar_Set: variable %s not found\n", var_name);
		return;
	}

	changed = strcmp(var->string, value);
	
	Z_Free (var->string);	// free the old value string
	
	var->string = Z_Malloc (strlen(value)+1);
	strcpy (var->string, value);
	var->value = atof (var->string);
	if (var->server && changed)
	{
		if (sv.active)
			SV_BroadcastPrintf ("\"%s\" changed to \"%s\"\n", var->name, var->string);
	}
	//Heffo - Cvar Callback Function
	if(var->Cvar_Changed)
		var->Cvar_Changed(var);
	//Heffo - Cvar Callback Function
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (const char *var_name, float value)
{
	char	val[32];
	ftoa(value, val, 5, 4);
	//sprintf (val, "%f",value);
	Cvar_Set (var_name, val);
}


/*
============
Cvar_RegisterVariable

Adds a freestanding variable to the variable list.
============
*/
void Cvar_RegisterVariable (cvar_t *variable)
{
	char	*oldstr;
	
// first check to see if it has already been defined
	if (Cvar_FindVar (variable->name))
	{
		Con_Printf ("Can't register variable %s, already defined\n", variable->name);
		return;
	}
	
// check for overlap with a command
	if (Cmd_Exists (variable->name))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command\n", variable->name);
		return;
	}
		
// copy the value off, because future sets will Z_Free it
	oldstr = variable->string;
	variable->string = Z_Malloc (strlen(variable->string)+1);	
	strcpy (variable->string, oldstr);
	variable->value = atof (variable->string);
	
// link the variable in
	variable->next = cvar_vars;
	cvar_vars = variable;
}

//mankrip & Heffo - Cvar Callback Function - begin
void Cvar_RegisterVariableWithCallback (cvar_t *variable, void (*function) (cvar_t*))
{
// first check to see if it has already been defined
	if (Cvar_FindVar (variable->name))
	{
		Con_Printf ("Can't register variable %s, already defined\n", variable->name);
		return;
	}

// check for overlap with a command
	if (Cmd_Exists (variable->name))
	{
		Con_Printf ("Cvar_RegisterVariable: %s is a command\n", variable->name);
		return;
	}
	Cvar_RegisterVariable (variable);
	variable->Cvar_Changed = function;
}
//mankrip & Heffo - Cvar Callback Function - end

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean	Cvar_Command (void)
{
	cvar_t			*v;

// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;
		
// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Con_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
		return true;
	}

	Cvar_Set (v->name, Cmd_Argv(1));
	return true;
}


/*
============
Cvar_WriteVariables

Writes lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (FILE *f)
{
	cvar_t	*var;
	char buf[256];

	for (var = cvar_vars ; var ; var = var->next)
		if (var->archive)
			//fprintf (f, "%s \"%s\"\n", var->name, var->string);
			{
				const int size = snprintf (buf, 256, "%s \"%s\"\n", var->name, var->string);
				fwrite(buf, size, 1, f);
			}
}

