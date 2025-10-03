#define PROGSUSED
#include "progsint.h"
#include <stdlib.h>

typedef struct prmemb_s {
	struct prmemb_s *prev;
	int level;
} prmemb_t;
void *QCHunkAlloc(progfuncs_t *progfuncs, int ammount)
{
	prmemb_t *mem;
	ammount = sizeof(prmemb_t)+((ammount + 3)&~3);
	mem = memalloc(ammount); 
	memset(mem, 0, ammount);
	mem->prev = memb;
	if (!memb)
		mem->level = 1;
	else
		mem->level = ((prmemb_t *)memb)->level+1;
	memb = mem;

	return ((char *)mem)+sizeof(prmemb_t);
}

int QCHunkMark(progfuncs_t *progfuncs)
{
	return ((prmemb_t *)memb)->level;
}
void QCHunkFree(progfuncs_t *progfuncs, int mark)
{
	prmemb_t *omem;
	while(memb)
	{
		if (memb->level <= mark)
			return;

		omem = memb;
		memb = memb->prev;
		memfree(omem);
	}
	return;
}

//for 64bit systems. :)
//addressable memory is memory available to the vm itself for writing.
//once allocated, it cannot be freed for the lifetime of the VM.
void *QCAddressableAlloc(progfuncs_t *progfuncs, int ammount)
{
	ammount = (ammount + 4)&~3;	//round up to 4
	if (addressableused + ammount > addressablesize)
		Sys_Error("Not enough addressable memory for progs VM");

	addressableused += ammount;

#ifdef _WIN32
	if (!VirtualAlloc (addressablehunk, addressableused, MEM_COMMIT, PAGE_READWRITE))
		Sys_Error("VirtualAlloc failed. Blame windows.");
#endif

	return &addressablehunk[addressableused-ammount];
}

void QCAddressableFlush(progfuncs_t *progfuncs, int totalammount)
{
	addressableused = 0;
	if (totalammount < 0)	//flush
	{
		totalammount = addressablesize;
//		return;
	}

	if (addressablehunk)
#ifdef _WIN32
	VirtualFree(addressablehunk, 0, MEM_RELEASE);	//doesn't this look complicated? :p
	addressablehunk = VirtualAlloc (NULL, totalammount, MEM_RESERVE, PAGE_NOACCESS);
#else
	free(addressablehunk);
	addressablehunk = malloc(totalammount);	//linux will allocate-on-use anyway, which is handy.
//	memset(addressablehunk, 0xff, totalammount);
#endif
	if (!addressablehunk)
		Sys_Error("Out of memory\n");
	addressablesize = totalammount;
}

int QC_InitEnts(progfuncs_t *progfuncs, int max_ents)
{
	maxedicts = max_ents;

	sv_num_edicts = 0;

	max_fields_size = fields_size;

	prinst->edicttable = QCHunkAlloc(progfuncs, maxedicts*sizeof(struct edicts_s *));
	sv_edicts = QCHunkAlloc(progfuncs, externs->edictsize);
	prinst->edicttable[0] = sv_edicts;
	((edictrun_t*)prinst->edicttable[0])->fields = QCAddressableAlloc(progfuncs, max_fields_size);
	QC_ClearEdict(progfuncs, (edictrun_t *)sv_edicts);
	sv_num_edicts = 1;

	return max_fields_size;
}
edictrun_t qctempedict;	//used as a safty buffer
static float tempedictfields[2048];

static void QC_Configure (progfuncs_t *progfuncs, int addressable_size, int max_progs)	//can be used to wipe all memory
{
	unsigned int i;
	edictrun_t *e;

//	int a;
	max_fields_size=0;
	fields_size = 0;
	progfuncs->stringtable = 0;
	QC_StartShares(progfuncs);
	QC_InitShares(progfuncs);

	for ( i=1 ; i<maxedicts; i++)
	{
		e = (edictrun_t *)(prinst->edicttable[i]);
		prinst->edicttable[i] = NULL;
//		e->entnum = i;
		if (e)
			memfree(e);
	}

	QCHunkFree(progfuncs, 0);	//clear mem - our hunk may not be a real hunk.
	if (addressable_size<0)
		addressable_size = 8*1024*1024;
	QCAddressableFlush(progfuncs, addressable_size);

	pr_progstate = QCHunkAlloc(progfuncs, sizeof(progstate_t) * max_progs);

/*		for(a = 0; a < max_progs; a++)
		{
			pr_progstate[a].progs = NULL;
		}		
*/
		
	maxprogs = max_progs;
	pr_typecurrent=-1;

	prinst->reorganisefields = false;

	maxedicts = 1;
	prinst->edicttable = &sv_edicts;
	sv_num_edicts = 1;	//set up a safty buffer so things won't go horribly wrong too often
	sv_edicts=(struct edict_s *)&qctempedict;
	qctempedict.readonly = true;
	qctempedict.fields = tempedictfields;
	qctempedict.isfree = false;
}



static struct globalvars_s *QC_globals (progfuncs_t *progfuncs, progsnum_t pnum)
{
	if (pnum < 0)
	{
		if (!current_progstate)
			return NULL;	//err.. you've not loaded one yet.
		return (struct globalvars_s *)current_progstate->globals;
	}
	return (struct globalvars_s *)pr_progstate[pnum].globals;
}

static struct entvars_s *QC_entvars (progfuncs_t *progfuncs, struct edict_s *ed)
{
	if (((edictrun_t *)ed)->isfree)
		return NULL;

	return (struct entvars_s *)edvars(ed);
}

int QC_GetFuncArgCount(progfuncs_t *progfuncs, func_t func)
{
	unsigned int pnum;
	unsigned int fnum;
	dfunction_t *f;

	pnum = (func & 0xff000000)>>24;
	fnum = (func & 0x00ffffff);

	if (pnum >= (unsigned)maxprogs || !pr_progstate[pnum].functions)
		return -1;
	else if (fnum >= pr_progstate[pnum].progs->numfunctions)
		return -1;
	else
	{
		f = pr_progstate[pnum].functions + fnum;
		return f->numparms;
	}
}

func_t QC_FindFunc(progfuncs_t *progfuncs, char *funcname, progsnum_t pnum)
{
	dfunction_t *f=NULL;
	if (pnum == PR_ANY)
	{
		for (pnum = 0; (unsigned)pnum < maxprogs; pnum++)
		{
			if (!pr_progstate[pnum].progs)
				continue;
			f = QC_FindFunction(progfuncs, funcname, &pnum, pnum);
			if (f)
				break;
		}
	}
	else if (pnum == PR_ANYBACK)	//run backwards
	{
		for (pnum = maxprogs-1; pnum >= 0; pnum--)
		{
			if (!pr_progstate[pnum].progs)
				continue;
			f = QC_FindFunction(progfuncs, funcname, &pnum, pnum);
			if (f)
				break;
		}
	}
	else
		f = QC_FindFunction(progfuncs, funcname, &pnum, pnum);
	if (!f)
		return 0;

	{
	ddef16_t *var16;
	ddef32_t *var32;
	switch(pr_progstate[pnum].intsize)
	{
	case 24:
	case 16:
		var16 = QC_FindTypeGlobalFromProgs16(progfuncs, funcname, pnum, qct_function);	//we must make sure we actually have a function def - 'light' is defined as a field before it is defined as a function.
		if (!var16)
			return (f - pr_progstate[pnum].functions) | (pnum << 24);
		return *(int *)&pr_progstate[pnum].globals[var16->ofs];	
	case 32:
		var32 = QC_FindTypeGlobalFromProgs32(progfuncs, funcname, pnum, qct_function);	//we must make sure we actually have a function def - 'light' is defined as a field before it is defined as a function.
		if (!var32)
			return (f - pr_progstate[pnum].functions) | (pnum << 24);
		return *(int *)&pr_progstate[pnum].globals[var32->ofs];	
	}
	Sys_Error("Error with def size (PR_FindFunc)");	
	}
	return 0;
}

qceval_t *QC_FindGlobal(progfuncs_t *progfuncs, char *globname, progsnum_t pnum)
{
	unsigned int i;
	ddef16_t *var16;
	ddef32_t *var32;
	if (pnum == PR_CURRENT)
		pnum = pr_typecurrent;
	if (pnum == PR_ANY)
	{
		qceval_t *ev;
		for (i = 0; i < maxprogs; i++)
		{
			if (!pr_progstate[i].progs)
				continue;
			ev = QC_FindGlobal(progfuncs, globname, i);
			if (ev)
				return ev;
		}
		return NULL;
	}
	if (pnum < 0 || (unsigned)pnum >= maxprogs || !pr_progstate[pnum].progs)
		return NULL;
	switch(pr_progstate[pnum].intsize)
	{
	case 16:
	case 24:
		if (!(var16 = QC_FindGlobalFromProgs16(progfuncs, globname, pnum)))
			return NULL;

		return (qceval_t *)&pr_progstate[pnum].globals[var16->ofs];
	case 32:
		if (!(var32 = QC_FindGlobalFromProgs32(progfuncs, globname, pnum)))
			return NULL;

		return (qceval_t *)&pr_progstate[pnum].globals[var32->ofs];
	}
	Sys_Error("Error with def size (PR_FindGlobal)");
	return NULL;
}

static void QC_SetGlobalEdict(progfuncs_t *progfuncs, struct edict_s *ed, int ofs)
{
	((int*)pr_globals)[ofs] = EDICT_TO_PROG(progfuncs, ed);
}

static char *QC_VarString (progfuncs_t *progfuncs, int	first)
{
	int		i;
	static char out[1024];
	char *s;
	int written = 0;
	int slen;
	
	for (i=first ; i<pr_argc ; i++)
	{
		s = progfuncs->StringToNative(progfuncs, G_INT(OFS_PARM0+i*3));
		if (s)
		{
			slen = strlen(s);
			if (slen > sizeof(out) - written - 1)
				slen = sizeof(out) - written - 1;
			memcpy(out+written, s, slen);
			written += slen;
		}
	}
	out[written] = 0;
	return out;
}

static int QC_QueryField (progfuncs_t *progfuncs, unsigned int fieldoffset, etype_t *type, char **name, qcevalc_t *fieldcache)
{
	fdef_t *var;
	var = QC_FieldAtOfs(progfuncs, fieldoffset);
	if (!var)
		return false;

	if (type)
		*type = var->type & ~(DEF_SAVEGLOBAL|DEF_SHARED);
	if (name)
		*name = var->name;
	if (fieldcache)
	{
		fieldcache->ofs32 = var;
		fieldcache->varname = var->name;
	}
		
	return true;
}

qceval_t *QC_GetEdictFieldValue(progfuncs_t *progfuncs, struct edict_s *ed, char *name, qcevalc_t *cache)
{
	fdef_t *var;
	if (!cache)
	{
		var = QC_FindField(progfuncs, name);
		if (!var)
			return NULL;
		return (qceval_t *) &(((int*)(((edictrun_t*)ed)->fields))[var->ofs]);
	}
	if (!cache->varname)
	{
		cache->varname = name;
		var = QC_FindField(progfuncs, name);		
		if (!var)
		{
			cache->ofs32 = NULL;
			return NULL;
		}
		cache->ofs32 = var;
		cache->varname = var->name;
		if (!ed)
			return (void*)~0;	//something not null
		return (qceval_t *) &(((int*)(((edictrun_t*)ed)->fields))[var->ofs]);
	}
	if (cache->ofs32 == NULL)
		return NULL;
	return (qceval_t *) &(((int*)(((edictrun_t*)ed)->fields))[cache->ofs32->ofs]);
}

static struct edict_s *QC_ProgsToEdict (progfuncs_t *progfuncs, int progs)
{
	if ((unsigned)progs >= (unsigned)maxedicts)
	{
		printf("Bad entity index %i\n", progs);
		progs = 0;
	}
	return (struct edict_s *)PROG_TO_EDICT(progfuncs, progs);
}
static int QC_EdictToProgs (progfuncs_t *progfuncs, struct edict_s *ed)
{
	return EDICT_TO_PROG(progfuncs, ed);
}

string_t QC_StringToProgs			(progfuncs_t *progfuncs, char *str)
{
	char **ntable;
	int i, free=-1;

	if (!str)
		return 0;

//	if (str-progfuncs->stringtable < progfuncs->stringtablesize)
//		return str - progfuncs->stringtable;

	for (i = prinst->numallocedstrings-1; i >= 0; i--)
	{
		if (prinst->allocedstrings[i] == str)
			return (string_t)((unsigned int)i | 0x80000000);
		if (!prinst->allocedstrings[i])
			free = i;
	}

	if (free != -1)
	{
		i = free;
		prinst->allocedstrings[i] = str;
		return (string_t)((unsigned int)i | 0x80000000);
	}

	prinst->maxallocedstrings += 1024;
	ntable = memalloc(sizeof(char*) * prinst->maxallocedstrings); 
	memcpy(ntable, prinst->allocedstrings, sizeof(char*) * prinst->numallocedstrings);
	memset(ntable + prinst->numallocedstrings, 0, sizeof(char*) * (prinst->maxallocedstrings - prinst->numallocedstrings));
	prinst->numallocedstrings = prinst->maxallocedstrings;
	if (prinst->allocedstrings)
		memfree(prinst->allocedstrings);
	prinst->allocedstrings = ntable;

	for (i = prinst->numallocedstrings-1; i >= 0; i--)
	{
		if (!prinst->allocedstrings[i])
		{
			prinst->allocedstrings[i] = str;
			return (string_t)((unsigned int)i | 0x80000000);
		}
	}

	return 0;
}

char *QC_RemoveProgsString				(progfuncs_t *progfuncs, string_t str)
{
	char *ret;

	//input string is expected to be an allocated string
	//if its a temp, or a constant, just return NULL.
	if ((unsigned int)str & 0xc0000000)
	{
		if ((unsigned int)str & 0x80000000)
		{
			int i = str & ~0x80000000;
			if (i >= prinst->numallocedstrings)
			{
				pr_trace = 1;
				return NULL;
			}
			if (prinst->allocedstrings[i])
			{
				ret = prinst->allocedstrings[i];
				prinst->allocedstrings[i] = NULL;	//remove it
				return ret;
			}
			else
			{
				pr_trace = 1;
				return NULL;	//urm, was freed...
			}
		}
	}
	pr_trace = 1;
	return NULL;
}

char *QC_StringToNative				(progfuncs_t *progfuncs, string_t str)
{
	if ((unsigned int)str & 0xc0000000)
	{
		if ((unsigned int)str & 0x80000000)
		{
			int i = str & ~0x80000000;
			if (i >= prinst->numallocedstrings)
			{
				pr_trace = 1;
				return "";
			}
			if (prinst->allocedstrings[i])
				return prinst->allocedstrings[i];
			else
			{
				pr_trace = 1;
				return "";	//urm, was freed...
			}
		}
		if ((unsigned int)str & 0x40000000)
		{
			int i = str & ~0x40000000;
			if (i >= prinst->numtempstrings)
			{
				pr_trace = 1;
				return "";
			}
			return prinst->tempstrings[i];
		}
	}

	if (str >= progfuncs->stringtablesize)
	{
		pr_trace = 1;
		return "";
	}
	return progfuncs->stringtable + str;
}


string_t QC_AllocTempString			(progfuncs_t *progfuncs, char *str)
{
	char **ntable;
	int newmax;
	int i;

	if (!str)
		return 0;

	if (prinst->numtempstrings == prinst->maxtempstrings)
	{
		newmax = prinst->maxtempstrings += 1024;
		prinst->maxtempstrings += 1024;
		ntable = memalloc(sizeof(char*) * newmax);
		memcpy(ntable, prinst->tempstrings, sizeof(char*) * prinst->numtempstrings);
		prinst->maxtempstrings = newmax;
		if (prinst->tempstrings)
			memfree(prinst->tempstrings);
		prinst->tempstrings = ntable;
	}

	i = prinst->numtempstrings;
	if (i == 0x10000000)
		return 0;

	prinst->numtempstrings++;

	prinst->tempstrings[i] = memalloc(strlen(str)+1);
	strcpy(prinst->tempstrings[i], str);

	return (string_t)((unsigned int)i | 0x40000000);
}

void QC_FreeTemps			(progfuncs_t *progfuncs, int depth)
{
	int i;
	if (depth > prinst->numtempstrings)
	{
		Sys_Error("QC Temp stack inverted\n");
		return;
	}
	for (i = depth; i < prinst->numtempstrings; i++)
	{
		memfree(prinst->tempstrings[i]);
	}

	prinst->numtempstrings = depth;
}


struct qcthread_s *QC_ForkStack	(progfuncs_t *progfuncs);
void QC_ResumeThread			(progfuncs_t *progfuncs, struct qcthread_s *thread);
void	QC_AbortStack			(progfuncs_t *progfuncs);


void RegisterBuiltin(progfuncs_t *progfncs, char *name, qcbuiltin_t func);

progfuncs_t deffuncs = {
	PROGSTRUCT_VERSION,
	QC_Configure,
	QC_LoadProgs,
	QC_InitEnts,
	QC_ExecuteProgram,
	QC_SwitchProgs,
	QC_globals,
	QC_entvars,
	QC_RunError,
	QC_Edict_Print,
	QC_Edict_Alloc,
	QC_Edict_Free,

	QC_EdictNum,
	QC_NumForEdict,


	QC_SetGlobalEdict,

	QC_VarString,

	NULL,
	QC_FindFunc,
#ifdef MINIMAL
	NULL,
	NULL,
#else
	Comp_Begin,
	Comp_Continue,
#endif

	filefromprogs,
	filefromnewprogs,

	QC_SaveEnts,
	QC_LoadEnts,

	QC_SaveEnt,
	QC_RestoreEnt,

	QC_FindGlobal,
	QC_NewString,
	(void*)QCHunkAlloc,

	QC_GetEdictFieldValue,
	QC_ProgsToEdict,
	QC_EdictToProgs,

	QC_EvaluateDebugString,

	NULL,
	QC_StackTrace,

	QC_ToggleBreakpoint,
	0,
	NULL,
#ifdef MINIMAL
	NULL,
#else
	Decompile,
#endif
	NULL,
	NULL,
	RegisterBuiltin,

	0,
	0,

	QC_ForkStack,
	QC_ResumeThread,
	QC_AbortStack,

	0,

	QC_RegisterFieldVar,

	0,
	0,

	QC_AllocTempString,

	QC_StringToProgs,
	QC_StringToNative,
	0,
	QC_QueryField,
	QC_GetFuncArgCount
};
#undef printf

//defs incase following structure is not passed.
struct edict_s *safesv_edicts;
int safesv_num_edicts;
double safetime=0;

progexterns_t defexterns = {
	PROGSTRUCT_VERSION,		

	NULL, //char *(*ReadFile) (char *fname, void *buffer, int len);
	NULL, //int (*FileSize) (char *fname);	//-1 if file does not exist
	NULL, //bool (*WriteFile) (char *name, void *data, int len);
	printf, //void (*printf) (char *, ...);
	(void*)exit, //void (*Sys_Error) (char *, ...);
	NULL, //void (*Abort) (char *, ...);
	sizeof(edictrun_t), //int edictsize;	//size of edict_t

	NULL, //void (*entspawn) (struct edict_s *ent);	//ent has been spawned, but may not have all the extra variables (that may need to be set) set
	NULL, //bool (*entcanfree) (struct edict_s *ent);	//return true to stop ent from being freed
	NULL, //void (*stateop) (float var, func_t func);
	NULL,
	NULL,
	NULL,

	//used when loading a game
	NULL, //builtin_t *(*builtinsfor) (int num);	//must return a pointer to the builtins that were used before the state was saved.
	NULL, //void (*loadcompleate) (int edictsize);	//notification to reset any pointers.

	(void*)malloc, //void *(*memalloc) (int size);	//small string allocation	malloced and freed randomly by the executor. (use memalloc if you want)
	free, //void (*memfree) (void * mem);


	NULL, //builtin_t *globalbuiltins;	//these are available to all progs
	0, //int numglobalbuiltins;

	PR_NOCOMPILE,

	&safetime, //double *gametime;

	&safesv_edicts, //struct edict_s **sv_edicts;
	&safesv_num_edicts, //int *sv_num_edicts;

	NULL, //int (*useeditor) (char *filename, int line, int nump, char **parms);
};

//progfuncs_t *progfuncs = NULL;
#undef memfree
#undef prinst
#undef extensionbuiltin
#undef field
#undef shares
#undef sv_num_edicts


#ifdef QCLIBDLL_EXPORTS
__declspec(dllexport)
#endif 
void CloseProgs(progfuncs_t *inst)
{
//	extensionbuiltin_t *eb;
	void (VARGS *f) (void *);

	unsigned int i;
	edictrun_t *e;

	f = inst->parms->memfree;

	for ( i=1 ; i<inst->maxedicts; i++)
	{
		e = (edictrun_t *)(inst->prinst->edicttable[i]);
		inst->prinst->edicttable[i] = NULL;
		if (e)
		{
//			e->entnum = i;
			f(e);
		}
	}

	QCHunkFree(inst, 0);

#ifdef _WIN32
	VirtualFree(inst->addressablehunk, 0, MEM_RELEASE);	//doesn't this look complicated? :p
#else
	free(inst->addressablehunk);
#endif

/*
	while(inst->prinst->extensionbuiltin)
	{
		eb = inst->prinst->extensionbuiltin->prev;
		f(inst->prinst->extensionbuiltin);
		inst->prinst->extensionbuiltin = eb;
	}
*/
	if (inst->prinst->field)
		f(inst->prinst->field);
	if (inst->prinst->shares)
		f(inst->prinst->shares);	//free memory
	f(inst->prinst);
	f(inst);
}

void RegisterBuiltin(progfuncs_t *progfuncs, char *name, qcbuiltin_t func)
{
/*
	extensionbuiltin_t *eb;
	eb = memalloc(sizeof(extensionbuiltin_t));
	eb->prev = progfuncs->prinst->extensionbuiltin;
	progfuncs->prinst->extensionbuiltin = eb;
	eb->name = name;
	eb->func = func;
*/
}

#ifndef WIN32
#define QCLIBINT	//don't use dllspecifications
#endif

#if defined(QCLIBDLL_EXPORTS)
__declspec(dllexport)
#endif
progfuncs_t * InitProgs(progexterns_t *ext)
{	
	progfuncs_t *funcs;

	if (!ext)
		ext = &defexterns;
	else
	{
		int i;
		if (ext->progsversion > PROGSTRUCT_VERSION)
			return NULL;

		for (i=0;i<sizeof(progexterns_t); i+=4)	//make sure there are no items left out.
			if (!*(int *)((char *)ext+i))
				*(int *)((char *)ext+i) = *(int *)((char *)&defexterns+i);		
	}	
#undef memalloc
#undef pr_trace
	funcs = ext->memalloc(sizeof(progfuncs_t));	
	memcpy(funcs, &deffuncs, sizeof(progfuncs_t));

	funcs->prinst = ext->memalloc(sizeof(prinst_t));
	memset(funcs->prinst,0, sizeof(prinst_t));

	funcs->pr_trace = &funcs->prinst->pr_trace;
	funcs->progstate = &funcs->pr_progstate;
	funcs->callargc = &funcs->pr_argc;

	funcs->parms = ext;

	SetEndian();
	
	return funcs;
}
















#ifdef QCC
void main (int argc, char **argv)
{
	progexterns_t ext;

	progfuncs_t *funcs;
	funcs = InitProgs(&ext);
	if (funcs->PR_StartCompile(argc, argv))
		while(funcs->PR_ContinueCompile());
}
#endif
