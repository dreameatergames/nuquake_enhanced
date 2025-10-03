#include "quakedef.h"


//added these for the ext_csqc stuff
void Q_strncpyz (char *dest, char *src, int count)
{
	if (count < 1)
		return;
	count--;
	while (*src && count--)
	{
		*dest++ = *src++;
	}
	*dest++ = 0;
}
void Q_vsnprintf(char *outb, size_t outl, const char *fmt, va_list vl)
{
#ifdef _WIN32
	outl--;
	_vsnprintf(outb, outl, fmt, vl);
	outb[outl] = 0;
#else
	vsnprintf(outb, outl, fmt, vl);
#endif
}
void Q_snprintf(char *outb, size_t outl, const char *fmt, ...)
{
	va_list vl;
	va_start(vl, fmt);
	Q_vsnprintf(outb, outl, fmt, vl);
	va_end(vl);
}




#if defined(EXT_CSQC)

#include "pr_common.h"

#include <ctype.h>

//fixme
#define Z_QC_TAG 2
#define PRSTR	0xa6ffb3d7

static char *cvargroup_progs = "Progs variables";

static char *strtoupper(char *s)
{
	char *p;

	p = s;
	while(*p)
	{
		*p = toupper(*p);
		p++;
	}

	return s;
}

static char *strtolower(char *s)
{
	char *p;

	p = s;
	while(*p)
	{
		*p = tolower(*p);
		p++;
	}

	return s;
}

void VARGS PR_BIError(progfuncs_t *progfuncs, char *format, ...)
{
	va_list		argptr;
	static char		string[2048];

	va_start (argptr, format);
	Q_vsnprintf (string,sizeof(string)-1, format,argptr);
	va_end (argptr);

	if (developer.value)
	{
		struct globalvars_s *pr_globals = progfuncs->globals(progfuncs, PR_CURRENT);
		Con_Printf("%s\n", string);
		*progfuncs->pr_trace = 1;
		QC_INT(OFS_RETURN)=0;	//just in case it was a float and should be an ent...
		QC_INT(OFS_RETURN+1)=0;
		QC_INT(OFS_RETURN+2)=0;
	}
	else
	{
		progfuncs->StackTrace(progfuncs);
		progfuncs->AbortStack(progfuncs);
		progfuncs->parms->Abort ("%s", string);
	}
}


pbool QC_WriteFile(char *name, void *data, int len)
{
	char buffer[256];
	Q_snprintf(buffer, sizeof(buffer), "%s", name);
	COM_WriteFile(buffer, data, len);
	return true;
}

//a little loop so we can keep track of used mem
void *VARGS PR_CB_Malloc(int size)
{
	return malloc(size);//Z_TagMalloc (size, 100);
}
void VARGS PR_CB_Free(void *mem)
{
	free(mem);
}

////////////////////////////////////////////////////
//Finding

/*
//entity(string field, float match) findchainflags = #450
//chained search for float, int, and entity reference fields
void PFC_findchainflags (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, f;
	int s;
	edictrun_t	*ent, *chain;

	chain = (edictrun_t *) *prinst->parms->sv_edicts;

	f = QC_INT(OFS_PARM0)+prinst->fieldadjust;
	s = QC_FLOAT(OFS_PARM1);

	for (i = 1; i < *prinst->parms->sv_num_edicts; i++)
	{
		ent = prinst->EdictNum(prinst, i);
		if (ent->isfree)
			continue;
		if (!((int)((float *)ent->v)[f] & s))
			continue;

		ent->v->chain = EDICT_TO_PROG(prinst, chain);
		chain = ent;
	}

	QCRETURN_EDICT(prinst, chain);
}
*/

/*
//entity(string field, float match) findchainfloat = #403
void PFC_findchainfloat (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, f;
	float s;
	edictrun_t	*ent, *chain;

	chain = (edictrun_t *) *prinst->parms->sv_edicts;

	f = QC_INT(OFS_PARM0)+prinst->fieldadjust;
	s = QC_FLOAT(OFS_PARM1);

	for (i = 1; i < *prinst->parms->sv_num_edicts; i++)
	{
		ent = prinst->EdictNum(prinst, i);
		if (ent->isfree)
			continue;
		if (((float *)ent->v)[f] != s)
			continue;

		ent->v->chain = EDICT_TO_PROG(prinst, chain);
		chain = ent;
	}

	QCRETURN_EDICT(prinst, chain);
}
*/

/*
//entity(string field, string match) findchain = #402
//chained search for strings in entity fields
void PFC_findchain (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, f;
	char *s;
	string_t t;
	edictrun_t *ent, *chain;

	chain = (edictrun_t *) *prinst->parms->sv_edicts;

	f = QC_INT(OFS_PARM0)+prinst->fieldadjust;
	s = QC_GetStringOfs(prinst, OFS_PARM1);

	for (i = 1; i < *prinst->parms->sv_num_edicts; i++)
	{
		ent = prinst->EdictNum(prinst, i);
		if (ent->isfree)
			continue;
		t = *(string_t *)&((float*)ent->v)[f];
		if (!t)
			continue;
		if (strcmp(PR_GetString(prinst, t), s))
			continue;

		ent->v->chain = EDICT_TO_PROG(prinst, chain);
		chain = ent;
	}

	QCRETURN_EDICT(prinst, chain);
}
*/

//EXTENSION: DP_QC_FINDFLAGS
//entity(entity start, float fld, float match) findflags = #449
void PFC_FindFlags (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int e, f;
	int s;
	edictrun_t *ed;

	e = QC_EDICTNUM(prinst, OFS_PARM0);
	f = QC_INT(OFS_PARM1)+prinst->fieldadjust;
	s = QC_FLOAT(OFS_PARM2);

	for (e++; e < *prinst->parms->sv_num_edicts; e++)
	{
		ed = (edictrun_t*)prinst->EdictNum(prinst, e);
		if (ed->isfree)
			continue;
		if ((int)((float *)ed->fields)[f] & s)
		{
			QCRETURN_EDICT(prinst, ed);
			return;
		}
	}

	QCRETURN_EDICT(prinst, *prinst->parms->sv_edicts);
}

//entity(entity start, float fld, float match) findfloat = #98
void PFC_FindFloat (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int e, f;
	int s;
	edictrun_t *ed;

	e = QC_EDICTNUM(prinst, OFS_PARM0);
	f = QC_INT(OFS_PARM1)+prinst->fieldadjust;
	s = QC_INT(OFS_PARM2);

	for (e++; e < *prinst->parms->sv_num_edicts; e++)
	{
		ed = (edictrun_t*)prinst->EdictNum(prinst, e);
		if (ed->isfree)
			continue;
		if (((int *)ed->fields)[f] == s)
		{
			QCRETURN_EDICT(prinst, ed);
			return;
		}
	}

	QCRETURN_EDICT(prinst, *prinst->parms->sv_edicts);
}

// entity (entity start, .string field, string match) find = #5;
void PFC_FindString (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int		e;
	int		f;
	char	*s;
	string_t t;
	edictrun_t	*ed;

	e = QC_EDICTNUM(prinst, OFS_PARM0);
	f = QC_INT(OFS_PARM1)+prinst->fieldadjust;
	s = QC_GetStringOfs(prinst, OFS_PARM2);
	if (!s)
	{
		PR_BIError (prinst, "PFC_FindString: bad search string");
		return;
	}

	for (e++ ; e < *prinst->parms->sv_num_edicts ; e++)
	{
		ed = (edictrun_t*)prinst->EdictNum(prinst, e);
		if (ed->isfree)
			continue;
		t = ((string_t *)ed->fields)[f];
		if (!t)
			continue;
		if (!strcmp(prinst->StringToNative(prinst, t),s))
		{
			QCRETURN_EDICT(prinst, ed);
			return;
		}
	}

	QCRETURN_EDICT(prinst, *prinst->parms->sv_edicts);
}

//Finding
////////////////////////////////////////////////////
//Cvars

//float(string cvarname) cvar
void PFC_cvar (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	cvar_t	*var;
	char	*str;

	str = QC_GetStringOfs(prinst, OFS_PARM0);
	{
		var = Cvar_FindVar(str);
		if (var)
			QC_FLOAT(OFS_RETURN) = var->value;
		else
			QC_FLOAT(OFS_RETURN) = 0;
	}
}

//string(string cvarname) cvar_string
void PFC_cvar_string (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*str = QC_GetStringOfs(prinst, OFS_PARM0);
	cvar_t *cv = Cvar_FindVar(str);
	if (cv)
		RETURN_CSTRING(cv->string);
	else
		QC_INT(OFS_RETURN) = 0;
}

//float(string name) cvar_type
void PFC_cvar_type (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*str = QC_GetStringOfs(prinst, OFS_PARM0);
	int ret = 0;
	cvar_t *v;

	v = Cvar_FindVar(str);
	if (v)
	{
		ret |= 1; // CVAR_EXISTS

		if(v->archive)
			ret |= 2; // CVAR_TYPE_SAVED

		//private cvars
		//	ret |= 4; // CVAR_TYPE_PRIVATE
		
		//not user created
			ret |= 8; // CVAR_TYPE_ENGINE
		
		//fte cvars don't support this
		//	ret |= 16; // CVAR_TYPE_HASDESCRIPTION
	}
	QC_FLOAT(OFS_RETURN) = ret;
}

//void(string cvarname, string newvalue) cvar
void PFC_cvar_set (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*var_name, *val;
	cvar_t *var;

	var_name = QC_GetStringOfs(prinst, OFS_PARM0);
	val = QC_GetStringOfs(prinst, OFS_PARM1);

	var = Cvar_FindVar(var_name);
	Cvar_Set (var_name, val);
}

void PFC_cvar_setf (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*var_name;
	float	val;
	cvar_t *var;

	var_name = QC_GetStringOfs(prinst, OFS_PARM0);
	val = QC_FLOAT(OFS_PARM1);

	var = Cvar_FindVar(var_name);
	Cvar_SetValue (var_name, val);
}

//float(string name, string value) registercvar
void PFC_registercvar (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *name, *value;
	cvar_t *var;
	value = QC_GetStringOfs(prinst, OFS_PARM0);

	if (Cvar_FindVar(value))
		QC_FLOAT(OFS_RETURN) = 0;
	else
	{
		name = malloc(strlen(value)+1);
		strcpy(name, value);
		if (*prinst->callargc > 1)
			value = QC_GetStringOfs(prinst, OFS_PARM1);
		else
			value = "";

		var = malloc(sizeof(*var));
		var->name = name;
		var->string = value;
		Cvar_RegisterVariable(var);
	// archive?
		if (Cvar_FindVar(value))
			QC_FLOAT(OFS_RETURN) = 1;
		else
			QC_FLOAT(OFS_RETURN) = 0;
	}
}

//Cvars
////////////////////////////////////////////////////
//File access

#define MAX_QC_FILES 256

#define FIRST_QC_FILE_INDEX 1000

typedef struct {
	char name[256];
	char *data;
	int bufferlen;
	int len;
	int ofs;
	int accessmode;
	progfuncs_t *prinst;
} pf_fopen_files_t;
pf_fopen_files_t pf_fopen_files[MAX_QC_FILES];

void PFC_fopen (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *name = QC_GetStringOfs(prinst, OFS_PARM0);
	int fmode = QC_FLOAT(OFS_PARM1);
	int i;

	for (i = 0; i < MAX_QC_FILES; i++)
		if (!pf_fopen_files[i].data)
			break;

	if (i == MAX_QC_FILES)	//too many already open
	{
		Con_Printf("qcfopen: too many files open (trying %s)\n", name);
		QC_FLOAT(OFS_RETURN) = -1;
		return;
	}

	if (name[1] == ':' ||	//dos filename absolute path specified - reject.
		strchr(name, '\\') || *name == '/' ||	//absolute path was given - reject
		strstr(name, ".."))	//someone tried to be cleaver.
	{
		QC_FLOAT(OFS_RETURN) = -1;
		return;
	}

	Q_strncpyz(pf_fopen_files[i].name, va("data/%s", name), sizeof(pf_fopen_files[i].name));

	pf_fopen_files[i].accessmode = fmode;
	switch (fmode)
	{
	case 0:	//read
		pf_fopen_files[i].data = COM_LoadMallocFile(pf_fopen_files[i].name);
		if (!pf_fopen_files[i].data)
		{
			Q_strncpyz(pf_fopen_files[i].name, name, sizeof(pf_fopen_files[i].name));
			pf_fopen_files[i].data = COM_LoadMallocFile(pf_fopen_files[i].name);
		}

		if (pf_fopen_files[i].data)
		{
			QC_FLOAT(OFS_RETURN) = i + FIRST_QC_FILE_INDEX;
			pf_fopen_files[i].prinst = prinst;
		}
		else
			QC_FLOAT(OFS_RETURN) = -1;

		pf_fopen_files[i].bufferlen = pf_fopen_files[i].len = com_filesize;
		pf_fopen_files[i].ofs = 0;
		break;
	case 1:	//append
		pf_fopen_files[i].data = COM_LoadMallocFile(pf_fopen_files[i].name);
		pf_fopen_files[i].ofs = pf_fopen_files[i].bufferlen = pf_fopen_files[i].len = com_filesize;
		if (pf_fopen_files[i].data)
		{
			QC_FLOAT(OFS_RETURN) = i + FIRST_QC_FILE_INDEX;
			pf_fopen_files[i].prinst = prinst;
			break;
		}
		//file didn't exist - fall through
	case 2:	//write
		pf_fopen_files[i].bufferlen = 8192;
		pf_fopen_files[i].data = Z_Malloc(pf_fopen_files[i].bufferlen);
		pf_fopen_files[i].len = 0;
		pf_fopen_files[i].ofs = 0;
		QC_FLOAT(OFS_RETURN) = i + FIRST_QC_FILE_INDEX;
		pf_fopen_files[i].prinst = prinst;
		break;
	case 3:
		pf_fopen_files[i].bufferlen = 0;
		pf_fopen_files[i].data = "";
		pf_fopen_files[i].len = 0;
		pf_fopen_files[i].ofs = 0;
		QC_FLOAT(OFS_RETURN) = i + FIRST_QC_FILE_INDEX;
		pf_fopen_files[i].prinst = prinst;
		break;
	default: //bad
		QC_FLOAT(OFS_RETURN) = -1;
		break;
	}
}

void PFC_fclose_i (int fnum)
{
	if (fnum < 0 || fnum >= MAX_QC_FILES)
	{
		Con_Printf("PF_fclose: File out of range\n");
		return;	//out of range
	}

	if (!pf_fopen_files[fnum].data)
	{
		Con_Printf("PF_fclose: File is not open\n");
		return;	//not open
	}

	switch(pf_fopen_files[fnum].accessmode)
	{
	case 0:
		COM_FreeMallocFile(pf_fopen_files[fnum].data);
		break;
	case 1:
	case 2:
		COM_WriteFile(pf_fopen_files[fnum].name, pf_fopen_files[fnum].data, pf_fopen_files[fnum].len);
		Z_Free(pf_fopen_files[fnum].data);
		break;
	case 3:
		break;
	}
	pf_fopen_files[fnum].data = NULL;
	pf_fopen_files[fnum].prinst = NULL;
}

void PFC_fclose (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int fnum = QC_FLOAT(OFS_PARM0)-FIRST_QC_FILE_INDEX;

	if (fnum < 0 || fnum >= MAX_QC_FILES)
	{
		Con_Printf("PF_fclose: File out of range\n");
		return;	//out of range
	}

	if (pf_fopen_files[fnum].prinst != prinst)
	{
		Con_Printf("PF_fclose: File is from wrong instance\n");
		return;	//this just isn't ours.
	}

	PFC_fclose_i(fnum);
}

void PFC_fgets (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char c, *s, *o, *max, *eof;
	int fnum = QC_FLOAT(OFS_PARM0) - FIRST_QC_FILE_INDEX;
	char pr_string_temp[4096];

	*pr_string_temp = '\0';
	QC_INT(OFS_RETURN) = 0;	//EOF
	if (fnum < 0 || fnum >= MAX_QC_FILES)
	{
		PR_BIError(prinst, "PF_fgets: File out of range\n");
		return;	//out of range
	}

	if (!pf_fopen_files[fnum].data)
	{
		PR_BIError(prinst, "PF_fgets: File is not open\n");
		return;	//not open
	}

	if (pf_fopen_files[fnum].prinst != prinst)
	{
		PR_BIError(prinst, "PF_fgets: File is from wrong instance\n");
		return;	//this just isn't ours.
	}

	//read up to the next \n, ignoring any \rs.
	o = pr_string_temp;
	max = o + sizeof(pr_string_temp)-1;
	s = pf_fopen_files[fnum].data+pf_fopen_files[fnum].ofs;
	eof = pf_fopen_files[fnum].data+pf_fopen_files[fnum].len;
	while(s < eof)
	{
		c = *s++;
		if (c == '\n')
			break;
		if (c == '\r')
			continue;

		if (o == max)
			break;
		*o++ = c;
	}
	*o = '\0';

	pf_fopen_files[fnum].ofs = s - pf_fopen_files[fnum].data;

	if (!pr_string_temp[0] && s == eof)
		QC_INT(OFS_RETURN) = 0;	//EOF
	else
		RETURN_TSTRING(pr_string_temp);
}

void PFC_fputs (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int fnum = QC_FLOAT(OFS_PARM0) - FIRST_QC_FILE_INDEX;
	char *msg = prinst->VarString(prinst, 1);
	int len = strlen(msg);
	if (fnum < 0 || fnum >= MAX_QC_FILES)
	{
		Con_Printf("PF_fgets: File out of range\n");
		return;	//out of range
	}

	if (!pf_fopen_files[fnum].data)
	{
		Con_Printf("PF_fgets: File is not open\n");
		return;	//not open
	}

	if (pf_fopen_files[fnum].prinst != prinst)
	{
		Con_Printf("PF_fgets: File is from wrong instance\n");
		return;	//this just isn't ours.
	}

	switch(pf_fopen_files[fnum].accessmode)
	{
	default:
		break;
	case 1:
	case 2:
		if (pf_fopen_files[fnum].bufferlen < pf_fopen_files[fnum].ofs + len)
		{
			char *newbuf;
			pf_fopen_files[fnum].bufferlen = pf_fopen_files[fnum].bufferlen*2 + len;
			newbuf = Z_Malloc(pf_fopen_files[fnum].bufferlen);
			memcpy(newbuf, pf_fopen_files[fnum].data, pf_fopen_files[fnum].len);
			Z_Free(pf_fopen_files[fnum].data);
			pf_fopen_files[fnum].data = newbuf;
		}

		memcpy(pf_fopen_files[fnum].data + pf_fopen_files[fnum].ofs, msg, len);
		if (pf_fopen_files[fnum].len < pf_fopen_files[fnum].ofs + len)
			pf_fopen_files[fnum].len = pf_fopen_files[fnum].ofs + len;
		pf_fopen_files[fnum].ofs+=len;
		break;
	}
}

void PFC_fcloseall (progfuncs_t *prinst)
{
	int i;
	for (i = 0; i < MAX_QC_FILES; i++)
	{
		if (pf_fopen_files[i].prinst != prinst)
			continue;
		Con_Printf("qc file %s was still open\n", pf_fopen_files[i].name);
		PFC_fclose_i(i);
	}
}

//closes filesystem type stuff for when a progs has stopped needing it.
void PRC_fclose_progs (progfuncs_t *prinst)
{
	PFC_fcloseall(prinst);
}

//File access
////////////////////////////////////////////////////
//Entities

void PFC_WasFreed (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	edictrun_t	*ent;
	ent = (edictrun_t*)QC_EDICT(prinst, OFS_PARM0);
	QC_FLOAT(OFS_RETURN) = ent->isfree;
}

void PFC_num_for_edict (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	edictrun_t	*ent;
	ent = (edictrun_t*)QC_EDICT(prinst, OFS_PARM0);
	QC_FLOAT(OFS_RETURN) = ent->entnum;
}

void PFC_edict_for_num(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	edictrun_t	*ent;
	ent = (edictrun_t*)prinst->EdictNum(prinst, QC_FLOAT(OFS_PARM0));

	QCRETURN_EDICT(prinst, ent);
}

//entity nextent(entity)
void PFC_nextent (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int		i;
	edictrun_t	*ent;

	i = QC_EDICTNUM(prinst, OFS_PARM0);
	while (1)
	{
		i++;
		if (i == *prinst->parms->sv_num_edicts)
		{
			QCRETURN_EDICT(prinst, *prinst->parms->sv_edicts);
			return;
		}
		ent = (edictrun_t*)prinst->EdictNum(prinst, i);
		if (!ent->isfree)
		{
			QCRETURN_EDICT(prinst, ent);
			return;
		}
	}
}

//entity() spawn
void PFC_Spawn (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	struct edict_s	*ed;
	ed = prinst->EntAlloc(prinst);
	QCRETURN_EDICT(prinst, ed);
}

//Entities
////////////////////////////////////////////////////
//String functions

//PF_dprint
void PFC_dprint (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_DPrintf ("%s",prinst->VarString(prinst, 0));
}

//PF_print
void PFC_print (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_Printf ("%s",prinst->VarString(prinst, 0));
}

//FTE_STRINGS
//C style strncasecmp (compare first n characters - case insensative)
void PFC_strncasecmp (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *a = QC_GetStringOfs(prinst, OFS_PARM0);
	char *b = QC_GetStringOfs(prinst, OFS_PARM1);
	float len = QC_FLOAT(OFS_PARM2);

	QC_FLOAT(OFS_RETURN) = strnicmp(a, b, len);
}

//FTE_STRINGS
//C style strcasecmp (case insensative string compare)
void PFC_strcasecmp (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *a = QC_GetStringOfs(prinst, OFS_PARM0);
	char *b = QC_GetStringOfs(prinst, OFS_PARM1);

	QC_FLOAT(OFS_RETURN) = stricmp(a, b);
}

//FTE_STRINGS
//C style strncmp (compare first n characters - case sensative. Note that there is no strcmp provided)
void PFC_strncmp (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *a = QC_GetStringOfs(prinst, OFS_PARM0);
	char *b = QC_GetStringOfs(prinst, OFS_PARM1);
	float len = QC_FLOAT(OFS_PARM2);

	QC_FLOAT(OFS_RETURN) = strncmp(a, b, len);
}

/*
//uses qw style \key\value strings
void PFC_infoget (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *info = QC_GetStringOfs(prinst, OFS_PARM0);
	char *key = QC_GetStringOfs(prinst, OFS_PARM1);

	key = Info_ValueForKey(info, key);

	RETURN_TSTRING(key);
}

//uses qw style \key\value strings
void PFC_infoadd (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *info = QC_GetStringOfs(prinst, OFS_PARM0);
	char *key = QC_GetStringOfs(prinst, OFS_PARM1);
	char *value = prinst->VarString(prinst, 2);
	char temp[8192];

	Q_strncpyz(temp, info, MAXTEMPBUFFERLEN);

	Info_SetValueForStarKey(temp, key, value, MAXTEMPBUFFERLEN);

	RETURN_TSTRING(temp);
}
*/

//string(float pad, string str1, ...) strpad
void PFC_strpad (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char destbuf[4096];
	char *dest = destbuf;
	int pad = QC_FLOAT(OFS_PARM0);
	char *src = prinst->VarString(prinst, 1);

	if (pad < 0)
	{	//pad left
		pad = -pad - strlen(src);
		if (pad>=sizeof(destbuf))
			pad = sizeof(destbuf)-1;
		if (pad < 0)
			pad = 0;

		Q_strncpyz(dest+pad, src, sizeof(destbuf)-pad);
		while(pad--)
		{
			pad--;
			dest[pad] = ' ';
		}
	}
	else
	{	//pad right
		if (pad>=sizeof(destbuf))
			pad = sizeof(destbuf)-1;
		pad -= strlen(src);
		if (pad < 0)
			pad = 0;

		Q_strncpyz(dest, src, sizeof(destbuf));
		dest+=strlen(dest);

		while(pad-->0)
			*dest++ = ' ';
		*dest = '\0';
	}

	RETURN_TSTRING(destbuf);
}

//part of PF_strconv
static int chrconv_number(int i, int base, int conv)
{
	i -= base;
	switch (conv)
	{
	default:
	case 5:
	case 6:
	case 0:
		break;
	case 1:
		base = '0';
		break;
	case 2:
		base = '0'+128;
		break;
	case 3:
		base = '0'-30;
		break;
	case 4:
		base = '0'+128-30;
		break;
	}
	return i + base;
}
//part of PF_strconv
static int chrconv_punct(int i, int base, int conv)
{
	i -= base;
	switch (conv)
	{
	default:
	case 0:
		break;
	case 1:
		base = 0;
		break;
	case 2:
		base = 128;
		break;
	}
	return i + base;
}
//part of PF_strconv
static int chrchar_alpha(int i, int basec, int baset, int convc, int convt, int charnum)
{
	//convert case and colour seperatly...

	i -= baset + basec;
	switch (convt)
	{
	default:
	case 0:
		break;
	case 1:
		baset = 0;
		break;
	case 2:
		baset = 128;
		break;

	case 5:
	case 6:
		baset = 128*((charnum&1) == (convt-5));
		break;
	}

	switch (convc)
	{
	default:
	case 0:
		break;
	case 1:
		basec = 'a';
		break;
	case 2:
		basec = 'A';
		break;
	}
	return i + basec + baset;
}
//FTE_STRINGS
//bulk convert a string. change case or colouring.
void PFC_strconv (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int ccase = QC_FLOAT(OFS_PARM0);		//0 same, 1 lower, 2 upper
	int redalpha = QC_FLOAT(OFS_PARM1);	//0 same, 1 white, 2 red,  5 alternate, 6 alternate-alternate
	int rednum = QC_FLOAT(OFS_PARM2);	//0 same, 1 white, 2 red, 3 redspecial, 4 whitespecial, 5 alternate, 6 alternate-alternate
	unsigned char *string = prinst->VarString(prinst, 3);
	int len = strlen(string);
	int i;
	unsigned char resbuf[8192];
	unsigned char *result = resbuf;

	if (len >= sizeof(resbuf))
		len = sizeof(resbuf)-1;

	for (i = 0; i < len; i++, string++, result++)	//should this be done backwards?
	{
		if (*string >= '0' && *string <= '9')	//normal numbers...
			*result = chrconv_number(*string, '0', rednum);
		else if (*string >= '0'+128 && *string <= '9'+128)
			*result = chrconv_number(*string, '0'+128, rednum);
		else if (*string >= '0'+128-30 && *string <= '9'+128-30)
			*result = chrconv_number(*string, '0'+128-30, rednum);
		else if (*string >= '0'-30 && *string <= '9'-30)
			*result = chrconv_number(*string, '0'-30, rednum);

		else if (*string >= 'a' && *string <= 'z')	//normal numbers...
			*result = chrchar_alpha(*string, 'a', 0, ccase, redalpha, i);
		else if (*string >= 'A' && *string <= 'Z')	//normal numbers...
			*result = chrchar_alpha(*string, 'A', 0, ccase, redalpha, i);
		else if (*string >= 'a'+128 && *string <= 'z'+128)	//normal numbers...
			*result = chrchar_alpha(*string, 'a', 128, ccase, redalpha, i);
		else if (*string >= 'A'+128 && *string <= 'Z'+128)	//normal numbers...
			*result = chrchar_alpha(*string, 'A', 128, ccase, redalpha, i);

		else if ((*string & 127) < 16 || !redalpha)	//special chars..
			*result = *string;
		else if (*string < 128)
			*result = chrconv_punct(*string, 0, redalpha);
		else
			*result = chrconv_punct(*string, 128, redalpha);
	}
	*result = '\0';

	RETURN_TSTRING(((char*)resbuf));
}

//FTE_STRINGS
//returns a string containing one character per parameter (up to the qc max params of 8).
void PFC_chr2str (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;

	char string[16];
	for (i = 0; i < *prinst->callargc; i++)
		string[i] = QC_FLOAT(OFS_PARM0 + i*3);
	string[i] = '\0';
	RETURN_TSTRING(string);
}

//FTE_STRINGS
//returns character at position X
void PFC_str2chr (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *instr = QC_GetStringOfs(prinst, OFS_PARM0);
	int ofs = (*prinst->callargc>1)?QC_FLOAT(OFS_PARM1):0;

	if (ofs < 0)
		ofs = strlen(instr)+ofs;

	if (ofs && (ofs < 0 || ofs > strlen(instr)))
		QC_FLOAT(OFS_RETURN) = '\0';
	else
		QC_FLOAT(OFS_RETURN) = instr[ofs];
}

//FTE_STRINGS
//strstr, without generating a new string. Use in conjunction with FRIK_FILE's substring for more similar strstr.
void PFC_strstrofs (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *instr = QC_GetStringOfs(prinst, OFS_PARM0);
	char *match = QC_GetStringOfs(prinst, OFS_PARM1);

	int firstofs = (*prinst->callargc>2)?QC_FLOAT(OFS_PARM2):0;

	if (firstofs && (firstofs < 0 || firstofs > strlen(instr)))
	{
		QC_FLOAT(OFS_RETURN) = -1;
		return;
	}

	match = strstr(instr+firstofs, match);
	if (!match)
		QC_FLOAT(OFS_RETURN) = -1;
	else
		QC_FLOAT(OFS_RETURN) = match - instr;
}

//float(string input) stof
void PFC_stof (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*s;

	s = QC_GetStringOfs(prinst, OFS_PARM0);

	QC_FLOAT(OFS_RETURN) = atof(s);
}

//tstring(float input) ftos
void PFC_ftos (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	v;
	char pr_string_temp[64];
	v = QC_FLOAT(OFS_PARM0);

	if (v == (int)v)
		sprintf (pr_string_temp, "%d",(int)v);
	else
		sprintf (pr_string_temp, "%5.1f",v);
	RETURN_TSTRING(pr_string_temp);
}

//tstring(integer input) itos
void PFC_itos (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int	v;
	char pr_string_temp[64];
	v = QC_INT(OFS_PARM0);

	sprintf (pr_string_temp, "%d",v);
	RETURN_TSTRING(pr_string_temp);
}

//int(string input) stoi
void PFC_stoi (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *input = QC_GetStringOfs(prinst, OFS_PARM0);

	QC_INT(OFS_RETURN) = atoi(input);
}

//tstring(integer input) htos
void PFC_htos (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned int	v;
	char pr_string_temp[64];
	v = QC_INT(OFS_PARM0);

	sprintf (pr_string_temp, "%08x",v);
	RETURN_TSTRING(pr_string_temp);
}

//int(string input) stoh
void PFC_stoh (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *input = QC_GetStringOfs(prinst, OFS_PARM0);

	QC_INT(OFS_RETURN) = strtoul(input, NULL, 16);
}

//vector(string s) stov = #117
//returns vector value from a string
void PFC_stov (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	char *s;
	float *out;

	s = prinst->VarString(prinst, 0);
	out = QC_VECTOR(OFS_RETURN);
	out[0] = out[1] = out[2] = 0;

	if (*s == '\'')
		s++;

	for (i = 0; i < 3; i++)
	{
		while (*s == ' ' || *s == '\t')
			s++;
		out[i] = atof (s);
		if (!out[i] && *s != '-' && *s != '+' && (*s < '0' || *s > '9'))
			break; // not a number
		while (*s && *s != ' ' && *s !='\t' && *s != '\'')
			s++;
		if (*s == '\'')
			break;
	}
}

//tstring(vector input) vtos
void PFC_vtos (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char pr_string_temp[64];
	//sprintf (pr_string_temp, "'%5.1f %5.1f %5.1f'", QC_VECTOR(OFS_PARM0)[0], QC_VECTOR(OFS_PARM0)[1], QC_VECTOR(OFS_PARM0)[2]);
	sprintf (pr_string_temp, "'%f %f %f'", QC_VECTOR(OFS_PARM0)[0], QC_VECTOR(OFS_PARM0)[1], QC_VECTOR(OFS_PARM0)[2]);
	RETURN_TSTRING(pr_string_temp);
}

void PFC_forgetstring(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *s=QC_RemoveProgsString(prinst, QC_INT(OFS_PARM0));
	if (!s)
	{
		Con_Printf("string was not strzoned\n");
		(*prinst->pr_trace) = 1;
		return;
	}
//	char *s=QC_GetStringOfs(prinst, OFS_PARM0);
	s-=8;
	if (((int *)s)[0] != PRSTR)
	{
		Con_Printf("QC tried to free a non dynamic string: ");
		Con_Printf("%s\n", s+8);	//two prints, so that logged prints ensure the first is written.
		(*prinst->pr_trace) = 1;
		prinst->StackTrace(prinst);
		return;
	}
	((int *)s)[0] = 0xabcd1234;
	Z_Free(s);

}

void PFC_dupstring(progfuncs_t *prinst, struct globalvars_s *pr_globals)	//frik_file
{
	char *s, *in;
	int len;
	in = prinst->VarString(prinst, 0);
	len = strlen(in)+1;
	s = Z_TagMalloc(len+8, Z_QC_TAG);
	if (!s)
	{
		QC_INT(OFS_RETURN) = 0;
		return;
	}
	((int *)s)[0] = PRSTR;
	((int *)s)[1] = len;
	strcpy(s+8, in);
	RETURN_SSTRING(s+8);
}

//returns a section of a string as a tempstring
void PFC_substring (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, start, length;
	char *s;
	char string[4096];

	s = QC_GetStringOfs(prinst, OFS_PARM0);
	start = QC_FLOAT(OFS_PARM1);
	length = QC_FLOAT(OFS_PARM2);

	if (start < 0)
		start = strlen(s)-start;
	if (length < 0)
		length = strlen(s)-start+(length+1);
	if (start < 0)
	{
	//	length += start;
		start = 0;
	}

	if (start >= strlen(s) || length<=0 || !*s)
	{
		RETURN_TSTRING("");
		return;
	}

	if (length >= sizeof(string))
		length = sizeof(string)-1;

	for (i = 0; i < start && *s; i++, s++)
		;

	for (i = 0; *s && i < length; i++, s++)
		string[i] = *s;
	string[i] = 0;

	RETURN_TSTRING(string);
}

//string(string str1, string str2) strcat
void PFC_strcat (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char dest[4096];
	char *src = prinst->VarString(prinst, 0);
	Q_strncpyz(dest, src, sizeof(dest));
	RETURN_TSTRING(dest);
}

void PFC_strlen(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = strlen(QC_GetStringOfs(prinst, OFS_PARM0));
}

//float(string input, string token) instr
void PFC_instr (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *sub;
	char *s1;
	char *s2;

	s1 = QC_GetStringOfs(prinst, OFS_PARM0);
	s2 = prinst->VarString(prinst, 1);

	if (!s1 || !s2)
	{
		PR_BIError(prinst, "Null string in \"instr\"\n");
		return;
	}

	sub = strstr(s1, s2);

	if (sub == NULL)
		QC_INT(OFS_RETURN) = 0;
	else
		RETURN_SSTRING(sub);	//last as long as the original string
}

//string(entity ent) etos = #65
void PFC_etos (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char s[64];
	Q_snprintf (s, sizeof(s), "entity %i", QC_EDICTNUM(prinst, OFS_PARM0));
	RETURN_TSTRING(s);
}

/*
//DP_QC_STRINGCOLORFUNCTIONS
// #476 float(string s) strlennocol - returns how many characters are in a string, minus color codes
void PFC_strlennocol (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *in = QC_GetStringOfs(prinst, OFS_PARM0);
	QC_FLOAT(OFS_RETURN) = COM_FunStringLength(in);
}

//DP_QC_STRINGCOLORFUNCTIONS
// string (string s) strdecolorize - returns the passed in string with color codes stripped
void PFC_strdecolorize (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *in = QC_GetStringOfs(prinst, OFS_PARM0);
	char result[8192];
	unsigned long flagged[8192];
	COM_ParseFunString(in, flagged, sizeof(flagged)/sizeof(flagged[0]));
	COM_DeFunString(flagged, result, sizeof(result), true);

	RETURN_TSTRING(result);
}
*/

//DP_QC_STRING_CASE_FUNCTIONS
void PFC_strtolower (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *in = QC_GetStringOfs(prinst, OFS_PARM0);
	char result[8192];

	Q_strncpyz(result, in, sizeof(result));
	strtolower(result);

	RETURN_TSTRING(result);
}

//DP_QC_STRING_CASE_FUNCTIONS
void PFC_strtoupper (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *in = QC_GetStringOfs(prinst, OFS_PARM0);
	char result[8192];

	Q_strncpyz(result, in, sizeof(result));
	strtoupper(result);

	RETURN_TSTRING(result);
}

/*
//DP_QC_STRFTIME
void PFC_strftime (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *in = prinst->VarString(prinst, 1);
	char result[8192];

	time_t ctime;
	struct tm *tm;

	ctime = time(NULL);

	if (QC_FLOAT(OFS_PARM0))
		tm = localtime(&ctime);
	else
		tm = gmtime(&ctime);
	strftime(result, sizeof(result), in, tm);
	Q_strncpyz(result, in, sizeof(result));
	strtoupper(result);

	RETURN_TSTRING(result);
}
*/

//String functions
////////////////////////////////////////////////////
//515's String functions

struct strbuf {
	progfuncs_t *prinst;
	char **strings;
	int used;
	int allocated;
};

#define NUMSTRINGBUFS 16
struct strbuf strbuflist[NUMSTRINGBUFS];

// #440 float() buf_create (DP_QC_STRINGBUFFERS)
void PFC_buf_create  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	
	int i;

	for (i = 0; i < NUMSTRINGBUFS; i++)
	{
		if (!strbuflist[i].prinst)
		{
			strbuflist[i].prinst = prinst;
			strbuflist[i].used = 0;
			strbuflist[i].allocated = 0;
			strbuflist[i].strings = NULL;
			QC_FLOAT(OFS_RETURN) = i+1;
			return;
		}
	}
	QC_FLOAT(OFS_RETURN) = 0;
}
// #441 void(float bufhandle) buf_del (DP_QC_STRINGBUFFERS)
void PFC_buf_del  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	int bufno = QC_FLOAT(OFS_PARM0)-1;

	if ((unsigned int)bufno >= NUMSTRINGBUFS)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	for (i = 0; i < strbuflist[bufno].used; i++)
		free(strbuflist[bufno].strings[i]);
	free(strbuflist[bufno].strings);

	strbuflist[bufno].strings = NULL;
	strbuflist[bufno].used = 0;
	strbuflist[bufno].allocated = 0;

	strbuflist[bufno].prinst = NULL;
}
// #442 float(float bufhandle) buf_getsize (DP_QC_STRINGBUFFERS)
void PFC_buf_getsize  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int bufno = QC_FLOAT(OFS_PARM0)-1;

	if ((unsigned int)bufno >= NUMSTRINGBUFS)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	QC_FLOAT(OFS_RETURN) = strbuflist[bufno].used;
}
/*
// #443 void(float bufhandle_from, float bufhandle_to) buf_copy (DP_QC_STRINGBUFFERS)
void PFC_buf_copy  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int buffrom = QC_FLOAT(OFS_PARM0)-1;
	int bufto = QC_FLOAT(OFS_PARM1)-1;

	if ((unsigned int)buffrom >= NUMSTRINGBUFS)
		return;
	if (strbuflist[buffrom].prinst != prinst)
		return;

	if ((unsigned int)bufto >= NUMSTRINGBUFS)
		return;
	if (strbuflist[bufto].prinst != prinst)
		return;

	//codeme
}
// #444 void(float bufhandle, float sortpower, float backward) buf_sort (DP_QC_STRINGBUFFERS)
void PFC_buf_sort  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int bufno = QC_FLOAT(OFS_PARM0)-1;
	int sortpower = QC_FLOAT(OFS_PARM1);
	int backwards = QC_FLOAT(OFS_PARM2);

	if ((unsigned int)bufno >= NUMSTRINGBUFS)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	//codeme
}
// #445 string(float bufhandle, string glue) buf_implode (DP_QC_STRINGBUFFERS)
void PFC_buf_implode  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int bufno = QC_FLOAT(OFS_PARM0)-1;
	char *glue = QC_GetStringOfs(prinst, OFS_PARM1);

	if ((unsigned int)bufno >= NUMSTRINGBUFS)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	//codeme

	RETURN_TSTRING("");
}
*/
// #446 string(float bufhandle, float string_index) bufstr_get (DP_QC_STRINGBUFFERS)
void PFC_bufstr_get  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int bufno = QC_FLOAT(OFS_PARM0)-1;
	int index = QC_FLOAT(OFS_PARM1);

	if ((unsigned int)bufno >= NUMSTRINGBUFS)
	{
		RETURN_CSTRING("");
		return;
	}
	if (strbuflist[bufno].prinst != prinst)
	{
		RETURN_CSTRING("");
		return;
	}

	if (index >= strbuflist[bufno].used)
	{
		RETURN_CSTRING("");
		return;
	}

	RETURN_TSTRING(strbuflist[bufno].strings[index]);
}
// #447 void(float bufhandle, float string_index, string str) bufstr_set (DP_QC_STRINGBUFFERS)
void PFC_bufstr_set  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int bufno = QC_FLOAT(OFS_PARM0)-1;
	int index = QC_FLOAT(OFS_PARM1);
	char *string = QC_GetStringOfs(prinst, OFS_PARM2);
	int oldcount;

	if ((unsigned int)bufno >= NUMSTRINGBUFS)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	if (index >= strbuflist[bufno].allocated)
	{
		oldcount = strbuflist[bufno].allocated;
		strbuflist[bufno].allocated = (index + 256);
		strbuflist[bufno].strings = realloc(strbuflist[bufno].strings, strbuflist[bufno].allocated*sizeof(char*));
		memset(strbuflist[bufno].strings+oldcount, 0, (strbuflist[bufno].allocated - oldcount) * sizeof(char*));
	}
	strbuflist[bufno].strings[index] = malloc(strlen(string)+1);
	strcpy(strbuflist[bufno].strings[index], string);

	if (index >= strbuflist[bufno].used)
		strbuflist[bufno].used = index+1;
}
/*
// #448 float(float bufhandle, string str, float order) bufstr_add (DP_QC_STRINGBUFFERS)
void PFC_bufstr_add  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int bufno = QC_FLOAT(OFS_PARM0)-1;
	char *string = QC_GetStringOfs(prinst, OFS_PARM1);
	int order = QC_FLOAT(OFS_PARM2);

	if ((unsigned int)bufno >= NUMSTRINGBUFS)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	//codeme

	QC_FLOAT(OFS_RETURN) = 0;
}
// #449 void(float bufhandle, float string_index) bufstr_free (DP_QC_STRINGBUFFERS)
void PFC_bufstr_free  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int bufno = QC_FLOAT(OFS_PARM0)-1;
	int index = QC_FLOAT(OFS_PARM1);

	if ((unsigned int)bufno >= NUMSTRINGBUFS)
		return;
	if (strbuflist[bufno].prinst != prinst)
		return;

	//codeme
}
*/

//515's String functions
////////////////////////////////////////////////////

/*
//float(float caseinsensitive, string s, ...) crc16 = #494 (DP_QC_CRC16)
void PFC_crc16 (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int insens = QC_FLOAT(OFS_PARM0);
	char *str = prinst->VarString(prinst, 1);
	int len = strlen(str);

	if (insens)
		QC_FLOAT(OFS_RETURN) = QCRC_Block_AsLower(str, len);	
	else
		QC_FLOAT(OFS_RETURN) = QCRC_Block(str, len);
}
*/

/*
// #510 string(string in) uri_escape = #510;
void PFC_uri_escape  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *s = QC_GetStringOfs(prinst, OFS_PARM0);
	RETURN_TSTRING(s);
}

// #511 string(string in) uri_unescape = #511;
void PFC_uri_unescape  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *s = QC_GetStringOfs(prinst, OFS_PARM0);
	RETURN_TSTRING(s);
}
*/

////////////////////////////////////////////////////
//Console functions

void PFC_ArgC  (progfuncs_t *prinst, struct globalvars_s *pr_globals)				//85			//float() argc;
{
	QC_FLOAT(OFS_RETURN) = Cmd_Argc();
}

/*
void PFC_tokenizebyseparator  (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Cmd_TokenizePunctation(QC_GetStringOfs(prinst, OFS_PARM0), QC_GetStringOfs(prinst, OFS_PARM1));
	QC_FLOAT(OFS_RETURN) = Cmd_Argc();
}*/

//KRIMZON_SV_PARSECLIENTCOMMAND added these two.
void PFC_Tokenize  (progfuncs_t *prinst, struct globalvars_s *pr_globals)			//84			//void(string str) tokanize;
{
	Cmd_TokenizeString(QC_GetStringOfs(prinst, OFS_PARM0));
	QC_FLOAT(OFS_RETURN) = Cmd_Argc();
}

void PFC_ArgV  (progfuncs_t *prinst, struct globalvars_s *pr_globals)				//86			//string(float num) argv;
{
	int i = QC_FLOAT(OFS_PARM0);
	if (i < 0)
	{
		PR_BIError(prinst, "pr_argv with i < 0");
		QC_INT(OFS_RETURN) = 0;
		return;
	}
	RETURN_TSTRING(Cmd_Argv(i));
}

//Console functions
////////////////////////////////////////////////////
//Maths functions

void PFC_random (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float		num;

	num = (rand ()&0x7fff) / ((float)0x7fff);

	QC_FLOAT(OFS_RETURN) = num;
}

//float(float number, float quantity) bitshift = #218;
void PFC_bitshift(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int bitmask;
	int shift;

	bitmask = QC_FLOAT(OFS_PARM0);
	shift = QC_FLOAT(OFS_PARM1);

	if (shift < 0)
		bitmask >>= shift;
	else
		bitmask <<= shift;

	QC_FLOAT(OFS_RETURN) = bitmask;
}

//float(float a, floats) min = #94
void PFC_min (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	float f;

	if (*prinst->callargc == 2)
	{
		QC_FLOAT(OFS_RETURN) = min(QC_FLOAT(OFS_PARM0), QC_FLOAT(OFS_PARM1));
	}
	else if (*prinst->callargc >= 3)
	{
		f = QC_FLOAT(OFS_PARM0);
		for (i = 1; i < *prinst->callargc; i++)
		{
			if (QC_FLOAT((OFS_PARM0 + i * 3)) < f)
				f = QC_FLOAT((OFS_PARM0 + i * 3));
		}
		QC_FLOAT(OFS_RETURN) = f;
	}
	else
		PR_BIError(prinst, "PFC_min: must supply at least 2 floats\n");
}

//float(float a, floats) max = #95
void PFC_max (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	float f;

	if (*prinst->callargc == 2)
	{
		QC_FLOAT(OFS_RETURN) = max(QC_FLOAT(OFS_PARM0), QC_FLOAT(OFS_PARM1));
	}
	else if (*prinst->callargc >= 3)
	{
		f = QC_FLOAT(OFS_PARM0);
		for (i = 1; i < *prinst->callargc; i++) {
			if (QC_FLOAT((OFS_PARM0 + i * 3)) > f)
				f = QC_FLOAT((OFS_PARM0 + i * 3));
		}
		QC_FLOAT(OFS_RETURN) = f;
	}
	else
	{
		PR_BIError(prinst, "PFC_min: must supply at least 2 floats\n");
	}
}

//float(float minimum, float val, float maximum) bound = #96
void PFC_bound (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (QC_FLOAT(OFS_PARM1) > QC_FLOAT(OFS_PARM2))
		QC_FLOAT(OFS_RETURN) = QC_FLOAT(OFS_PARM2);
	else if (QC_FLOAT(OFS_PARM1) < QC_FLOAT(OFS_PARM0))
		QC_FLOAT(OFS_RETURN) = QC_FLOAT(OFS_PARM0);
	else
		QC_FLOAT(OFS_RETURN) = QC_FLOAT(OFS_PARM1);
}

void PFC_Sin (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = sin(QC_FLOAT(OFS_PARM0));
}
void PFC_Cos (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = cos(QC_FLOAT(OFS_PARM0));
}
void PFC_Sqrt (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = sqrt(QC_FLOAT(OFS_PARM0));
}
void PFC_pow (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = pow(QC_FLOAT(OFS_PARM0), QC_FLOAT(OFS_PARM1));
}
void PFC_asin (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = asin(QC_FLOAT(OFS_PARM0));
}
void PFC_acos (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = acos(QC_FLOAT(OFS_PARM0));
}
void PFC_atan (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = atan(QC_FLOAT(OFS_PARM0));
}
void PFC_atan2 (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = atan2(QC_FLOAT(OFS_PARM0), QC_FLOAT(OFS_PARM1));
}
void PFC_tan (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = tan(QC_FLOAT(OFS_PARM0));
}

void PFC_fabs (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	v;
	v = QC_FLOAT(OFS_PARM0);
	QC_FLOAT(OFS_RETURN) = fabs(v);
}

void PFC_rint (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	f;
	f = QC_FLOAT(OFS_PARM0);
	if (f > 0)
		QC_FLOAT(OFS_RETURN) = (int)(f + 0.5);
	else
		QC_FLOAT(OFS_RETURN) = (int)(f - 0.5);
}

void PFC_floor (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = floor(QC_FLOAT(OFS_PARM0));
}

void PFC_ceil (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = ceil(QC_FLOAT(OFS_PARM0));
}

//Maths functions
////////////////////////////////////////////////////
//Vector functions

//vector() randomvec = #91
void PFC_randomvector (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	vec3_t temp;
	do
	{
		temp[0] = (rand() & 32767) * (2.0 / 32767.0) - 1.0;
		temp[1] = (rand() & 32767) * (2.0 / 32767.0) - 1.0;
		temp[2] = (rand() & 32767) * (2.0 / 32767.0) - 1.0;
	} while (DotProduct(temp, temp) >= 1);
	VectorCopy (temp, QC_VECTOR(OFS_RETURN));
}

//float vectoyaw(vector)
void PFC_vectoyaw (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*value1;
	float	yaw;

	value1 = QC_VECTOR(OFS_PARM0);

	if (value1[1] == 0 && value1[0] == 0)
		yaw = 0;
	else
	{
		yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	QC_FLOAT(OFS_RETURN) = yaw;
}

//float(vector) vlen
void PFC_vlen (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*value1;
	float	newv;

	value1 = QC_VECTOR(OFS_PARM0);

	newv = value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2];
	newv = sqrt(newv);

	QC_FLOAT(OFS_RETURN) = newv;
}

//vector vectoangles(vector)
void PFC_vectoangles (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*value1, *up;
	float	yaw, pitch, roll;

	value1 = QC_VECTOR(OFS_PARM0);
	if (*prinst->callargc >= 2)
		up = QC_VECTOR(OFS_PARM1);
	else
		up = NULL;

	

	if (value1[1] == 0 && value1[0] == 0)
	{
		if (value1[2] > 0)
		{
			pitch = -M_PI*0.5;
			yaw = up ? atan2(-up[1], -up[0]) : 0;
		}
		else
		{
			pitch = M_PI*0.5;
			yaw = up ? atan2(up[1], up[0]) : 0;
		}
		roll = 0;
	}
	else
	{
		yaw = atan2(value1[1], value1[0]);
		pitch = -atan2(value1[2], sqrt (value1[0]*value1[0] + value1[1]*value1[1]));

		if (up)
		{
			vec_t cp = cos(pitch), sp = sin(pitch);
			vec_t cy = cos(yaw), sy = sin(yaw);
			vec3_t tleft, tup;
			tleft[0] = -sy;
			tleft[1] = cy;
			tleft[2] = 0;
			tup[0] = sp*cy;
			tup[1] = sp*sy;
			tup[2] = cp;
			roll = -atan2(DotProduct(up, tleft), DotProduct(up, tup));
		}
		else
			roll = 0;
	}

	pitch *= -180 / M_PI;
	yaw *= 180 / M_PI;
	roll *= 180 / M_PI;
	if (pitch < 0)
		pitch += 360;
	if (yaw < 0)
		yaw += 360;
	if (roll < 0)
		roll += 360;
	QC_FLOAT(OFS_RETURN+0) = pitch;
	QC_FLOAT(OFS_RETURN+1) = yaw;
	QC_FLOAT(OFS_RETURN+2) = roll;
}

//vector normalize(vector)
void PFC_normalize (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*value1;
	vec3_t	newvalue;
	float	newf;

	value1 = QC_VECTOR(OFS_PARM0);

	newf = value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2];
	newf = sqrt(newf);

	if (newf == 0)
		newvalue[0] = newvalue[1] = newvalue[2] = 0;
	else
	{
		newf = 1/newf;
		newvalue[0] = value1[0] * newf;
		newvalue[1] = value1[1] * newf;
		newvalue[2] = value1[2] * newf;
	}

	VectorCopy (newvalue, QC_VECTOR(OFS_RETURN));
}

//Vector functions
////////////////////////////////////////////////////
//Progs internals

void PFC_Abort(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	prinst->AbortStack(prinst);
}

//this func calls a function in annother progs
//it works in the same way as the above func, except that it calls by reference to a function, as opposed to by it's name
//used for entity function variables - not actually needed anymore
void PFC_externrefcall (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int progsnum;
	func_t f;
	progsnum = QC_PROG(OFS_PARM0);
	f = QC_INT(OFS_PARM1);

	(*prinst->pr_trace)++;	//continue debugging.
	prinst->ExecuteProgram(prinst, f);
}

void PFC_externset (progfuncs_t *prinst, struct globalvars_s *pr_globals)	//set a value in annother progs
{
	int n = QC_PROG(OFS_PARM0);
	int v = QC_INT(OFS_PARM1);
	char *varname = prinst->VarString(prinst, 2);
	qceval_t *var;

	var = prinst->FindGlobal(prinst, varname, n);

	if (var)
		var->_int = v;
}

void PFC_externvalue (progfuncs_t *prinst, struct globalvars_s *pr_globals)	//return a value in annother progs
{
	int n = QC_PROG(OFS_PARM0);
	char *varname = prinst->VarString(prinst, 1);
	qceval_t *var;

	var = prinst->FindGlobal(prinst, varname, n);

	if (var)
	{
		QC_INT(OFS_RETURN+0) = ((int*)&var->_int)[0];
		QC_INT(OFS_RETURN+1) = ((int*)&var->_int)[1];
		QC_INT(OFS_RETURN+2) = ((int*)&var->_int)[2];
	}
	else
		QC_INT(OFS_RETURN) = 0;
}

void PFC_externcall (progfuncs_t *prinst, struct globalvars_s *pr_globals)	//this func calls a function in annother progs (by name)
{
	int progsnum;
	char *funcname;
	int i;
	string_t failedst = QC_INT(OFS_PARM1);
	func_t f;

	progsnum = QC_PROG(OFS_PARM0);
	funcname = QC_GetStringOfs(prinst, OFS_PARM1);

	f = prinst->FindFunction(prinst, funcname, progsnum);
	if (f)
	{
		for (i = OFS_PARM0; i < OFS_PARM5; i+=3)
			VectorCopy(QC_VECTOR(i+(2*3)), QC_VECTOR(i));

		(*prinst->pr_trace)++;	//continue debugging
		prinst->ExecuteProgram(prinst, f);
	}
	else if (!f)
	{
		f = prinst->FindFunction(prinst, "MissingFunc", progsnum);
		if (!f)
		{
			PR_BIError(prinst, "Couldn't find function %s", funcname);
			return;
		}

		for (i = OFS_PARM0; i < OFS_PARM6; i+=3)
			VectorCopy(QC_VECTOR(i+(1*3)), QC_VECTOR(i));
		QC_INT(OFS_PARM0) = failedst;

		(*prinst->pr_trace)++;	//continue debugging
		prinst->ExecuteProgram(prinst, f);
	}
}

/*
pr_csqc.obj : error LNK2001: unresolved external symbol _ PR_BIError
*/
void PFC_traceon (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	(*prinst->pr_trace) = true;
}

void PFC_traceoff (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	(*prinst->pr_trace) = false;
}
void PFC_coredump (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int size = 1024*1024*8;
	char *buffer = malloc(size);
	prinst->save_ents(prinst, buffer, &size, 3);
	COM_WriteFile("core.txt", buffer, size);
	free(buffer);
}
void PFC_eprint (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int size = 1024*1024;
	char *buffer = malloc(size);
	char *buf;
	buf = prinst->saveent(prinst, buffer, &size, QC_EDICT(prinst, OFS_PARM0));
	Con_Printf("Entity %i:\n%s\n", QC_EDICTNUM(prinst, OFS_PARM0), buf);
	free(buffer);
}

void PFC_break (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifdef SERVERONLY	//new break code
	char *s;

	//I would like some sort of network activity here,
	//but I don't want to mess up the sequence and stuff
	//It should be possible, but would mean that I would
	//need to alter the client, or rewrite a bit of the server..

	if (pr_globals)
		Con_TPrintf(STL_BREAKSTATEMENT);
	else if (developer.value!=2)
		return;	//non developers cann't step.
	for(;;)
	{
		s=Sys_ConsoleInput();
		if (s)
		{
			if (!*s)
				break;
			else
				Con_Printf("%s\n", svprogfuncs->EvaluateDebugString(svprogfuncs, s));
		}
	}
#elif defined(TEXTEDITOR)
	(*prinst->pr_trace)++;
#else	//old break code
Con_Printf ("break statement\n");
*(int *)-4 = 0;	// dump to debugger
//	PR_RunError ("break statement");
#endif
}

void PFC_error (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*s;

	s = prinst->VarString(prinst, 0);
/*	Con_Printf ("======SERVER ERROR in %s:\n%s\n", PR_GetString(pr_xfunction->s_name) ,s);
	ed = PROG_TO_EDICT(pr_global_struct->self);
	ED_Print (ed);
*/

	prinst->StackTrace(prinst);

	Con_Printf("%s\n", s);

	if (developer.value)
	{
//		SV_Error ("Program error: %s", s);
		PFC_break(prinst, pr_globals);
		(*prinst->pr_trace) = 2;
	}
	else
	{
		prinst->AbortStack(prinst);
		PR_BIError (prinst, "Program error: %s", s);
	}
}

//Progs internals
////////////////////////////////////////////////////
//System

//Sends text over to the client's execution buffer
void PFC_localcmd (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*str;

	str = prinst->VarString(prinst, 0);
	Cbuf_AddText (str);
}





void PFC_checkextension (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *extname = QC_GetStringOfs(prinst, OFS_PARM0);
	int i;
	for (i = 0; i < QSG_Extensions_count; i++)
	{
		if (!QSG_Extensions[i].name)
			continue;

		if (!strcmp(QSG_Extensions[i].name, extname))
		{
			QC_FLOAT(OFS_RETURN) = true;
			return;
		}
	}
	QC_FLOAT(OFS_RETURN) = false;
}

lh_extension_t QSG_Extensions[] = {

//as a special hack, the first 32 entries are PEXT features.
//some of these are overkill yes, but they are all derived from the fteextensions flags and document the underlaying protocol available.
//(which is why there are two lists of extensions here)
//note: not all of these are actually supported. This list mearly reflects the values of the PEXT_ constants.
//Check protocol.h to make sure that the related PEXT is enabled. The engine will only accept if they are actually supported.

	{"EXT_CSQC_SHARED"},				//this is a separate extension because it requires protocol modifications. note: this is also the extension that extends the allowed stats.

	{"DP_QC_ASINACOSATANATAN2TAN",		5,	{"asin", "acos", "atan", "atan2", "tan"}},
	{"DP_QC_CHANGEPITCH",				1,	{"changepitch"}},
	{"DP_QC_COPYENTITY",				1,	{"copyentity"}},
	{"DP_QC_CRC16",						1,	{"crc16"}},
	{"DP_QC_CVAR_DEFSTRING",			1,	{"cvar_defstring"}},
	{"DP_QC_CVAR_STRING",				1,	{"dp_cvar_string"}},	//448 builtin.
	{"DP_QC_CVAR_TYPE",					1,	{"cvar_type"}},
	{"DP_QC_EDICT_NUM",					1,	{"edict_num"}},
	{"DP_QC_ENTITYDATA",				5,	{"numentityfields", "entityfieldname", "entityfieldtype", "getentityfieldstring", "putentityfieldstring"}},
	{"DP_QC_ETOS",						1,	{"etos"}},
	{"DP_QC_FINDCHAIN",					1,	{"findchain"}},
	{"DP_QC_FINDCHAINFLOAT",			1,	{"findchainfloat"}},
	{"DP_QC_FINDFLAGS",					1,	{"findflags"}},
	{"DP_QC_FINDCHAINFLAGS",			1,	{"findchainflags"}},
	{"DP_QC_FINDFLOAT",					1,	{"findfloat"}},
	{"DP_QC_FS_SEARCH",					4,	{"search_begin", "search_end", "search_getsize", "search_getfilename"}},
	{"DP_QC_GETSURFACEPOINTATTRIBUTE",	1,	{"getsurfacepointattribute"}},
	{"DP_QC_MINMAXBOUND",				3,	{"min", "max", "bound"}},
	{"DP_QC_MULTIPLETEMPSTRINGS"},
	{"DP_QC_RANDOMVEC",					1,	{"randomvec"}},
	{"DP_QC_SINCOSSQRTPOW",				4,	{"sin", "cos", "sqrt", "pow"}},
	{"DP_QC_STRFTIME",					1,	{"strftime"}},
	{"DP_QC_STRING_CASE_FUNCTIONS",		2,	{"strtolower", "strtoupper"}},
	{"DP_QC_STRINGBUFFERS",				10,	{"buf_create", "buf_del", "buf_getsize", "buf_copy", "buf_sort", "buf_implode", "bufstr_get", "bufstr_set", "bufstr_add", "bufstr_free"}},
	{"DP_QC_STRINGCOLORFUNCTIONS",		2,	{"strlennocol", "strdecolorize"}},
	{"DP_QC_STRREPLACE",				2,	{"strreplace", "strireplace"}},
	{"DP_QC_TOKENIZEBYSEPARATOR",		1,	{"tokenizebyseparator"}},
	{"DP_QC_TRACEBOX",					1,	{"tracebox"}},
	{"DP_QC_UNLIMITEDTEMPSTRINGS"},
	{"DP_QC_URI_ESCAPE",				2,	{"uri_escape", "uri_unescape"}},
	{"DP_QC_URI_GET",					1,	{"uri_get"}},
	{"DP_QC_VECTOANGLES_WITH_ROLL"},
	{"DP_QC_VECTORVECTORS",				1,	{"vectorvectors"}},
	{"DP_QC_WHICHPACK",					1,	{"whichpack"}},
	{"DP_REGISTERCVAR",					1,	{"registercvar"}},
	{"DP_TE_PLASMABURN",				1,	{"te_plasmaburn"}},
	{"DP_TE_QUADEFFECTS1"},
	{"DP_TE_SMALLFLASH",				1,	{"te_smallflash"}},
	{"DP_TE_SPARK",						1,	{"te_spark"}},
	{"DP_TE_STANDARDEFFECTBUILTINS",	14,	{"te_gunshot", "te_spike", "te_superspike", "te_explosion", "te_tarexplosion", "te_wizspike", "te_knightspike", "te_lavasplash", "te_teleport", "te_explosion2", "te_lightning1", "te_lightning2", "te_lightning3", "te_beam"}},
	{"FRIK_FILE",						11, {"stof", "fopen","fclose","fgets","fputs","strlen","strcat","substring","stov","strzone","strunzone"}},
	{"FTE_CALLTIMEOFDAY",				1,	{"calltimeofday"}},
	{"FTE_FORCEINFOKEY",				1,	{"forceinfokey"}},
	{"FTE_ISBACKBUFFERED",				1,	{"isbackbuffered"}},
	{"FTE_MULTIPROGS"},	//multiprogs functions are available.
	{"FTE_MULTITHREADED",				3,	{"sleep", "fork", "abort"}},
	{"FTE_MVD_PLAYBACK"},
	{"FTE_QC_CHECKPVS",					1,	{"checkpvs"}},
	{"FTE_QC_MATCHCLIENTNAME",			1,	{"matchclientname"}},
	{"FTE_QC_SENDPACKET",				1,	{"sendpacket"}},

#ifdef SQL
	// serverside SQL functions for managing an SQL database connection
	{"FTE_SQL",							9, {"sqlconnect","sqldisconnect","sqlopenquery","sqlclosequery","sqlreadfield","sqlerror","sqlescape","sqlversion",
												  "sqlreadfloat"}},
#endif
	//eperimental advanced strings functions.
	//reuses the FRIK_FILE builtins (with substring extension)
	{"FTE_STRINGS",						16, {"stof", "strlen","strcat","substring","stov","strzone","strunzone",
												   "strstrofs", "str2chr", "chr2str", "strconv", "infoadd", "infoget", "strncmp", "strcasecmp", "strncasecmp"}},
	{"FTE_SV_REENTER"},
	{"FTE_TE_STANDARDEFFECTBUILTINS",	14,	{"te_gunshot", "te_spike", "te_superspike", "te_explosion", "te_tarexplosion", "te_wizspike", "te_knightspike", "te_lavasplash",
												   "te_teleport", "te_lightning1", "te_lightning2", "te_lightning3", "te_lightningblood", "te_bloodqw"}},

	{"KRIMZON_SV_PARSECLIENTCOMMAND",	3,	{"clientcommand", "tokenize", "argv"}},	//very very similar to the mvdsv system.

	{"ZQ_QC_STRINGS",					7, {"stof", "strlen","strcat","substring","stov","strzone","strunzone"}}	//a trimmed down FRIK_FILE.
};
unsigned int QSG_Extensions_count = sizeof(QSG_Extensions)/sizeof(QSG_Extensions[0]);
#endif
