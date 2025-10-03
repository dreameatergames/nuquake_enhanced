
#define NOSHORTCUTS
#include "qclib/progslib.h"

//builtin funcs (which operate on globals)
//To use these outside of builtins, you will likly have to use the 'globals' method.
#define	QC_FLOAT(o) (((float *)pr_globals)[o])
#define	QC_FLOAT2(o) (((float *)pr_globals)[OFS_PARM0 + o*3])
#define	QC_INT(o) (((int *)pr_globals)[o])
#define	QC_EDICT(pf, o) pf->ProgsToEdict(pf, QC_INT(o)) //((edict_t *)((char *) sv.edicts+ *(int *)&((float *)pr_globals)[o]))
#define QC_EDICTNUM(pf, o) pf->NumForEdict(pf, QC_EDICT(pf, o))
#define	QC_VECTOR(o) (&((float *)pr_globals)[o])
#define	QC_FUNCTION(o) (*(func_t *)&((float *)pr_globals)[o])
#define	QC_PROG QC_FLOAT

//builtin funcs (which operate on globals)
//To use these outside of builtins, you will likly have to use the 'globals' method.
#undef	G_FLOAT
#undef	G_FLOAT2
#undef	G_INT
#undef	G_EDICT
#undef	G_EDICTNUM
#undef	G_VECTOR
#undef	G_FUNCTION

#define QC_GetStringOfs(pf,o)								(*pf->StringToNative)		(pf, QC_INT(o))
#define QC_NewString(p, s, l) p->StringToProgs(p, p->AddString(p, s, l))

#define	QCRETURN_EDICT(pf,e) (((int *)pr_globals)[OFS_RETURN] = pf->EdictToProgs(pf,(struct edict_s *)e))
#define	RETURN_SSTRING(s) (((int *)pr_globals)[OFS_RETURN] = prinst->StringToProgs(prinst, s))	//static - exe will not change it.
#define	RETURN_TSTRING(s) (((int *)pr_globals)[OFS_RETURN] = prinst->TempString(prinst, s))	//temp (static but cycle buffers)
#define	RETURN_CSTRING(s) (((int *)pr_globals)[OFS_RETURN] = prinst->StringToProgs(prinst, s))	//semi-permanant. (hash tables?)
#define	RETURN_PSTRING(s) (((int *)pr_globals)[OFS_RETURN] = prinst->NewString(prinst, s, 0))	//permanant



pbool QC_WriteFile(char *name, void *data, int len);
void *VARGS PR_CB_Malloc(int size);
void VARGS PR_CB_Free(void *mem);


void PFC_fclose_progs (progfuncs_t *prinst);
void PFC_Common_RegisterCvars(void);
#define VectorNegate(a,b)		((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2])


typedef struct {
	char *name;
	int numbuiltins;
	char *builtinnames[16];
} lh_extension_t;
extern lh_extension_t QSG_Extensions[];
extern unsigned int QSG_Extensions_count;




//builtin naming

//PFC_ common to all, and has no field/global expectations
//PF_ specific to this module
//PF_cl_ specific to client side (menu or csprogs)
//PF_cs_ specific to csprogs



#define PFC_cin_open PF_Fixme
#define PFC_cin_close PF_Fixme
#define PFC_cin_setstate PF_Fixme
#define PFC_cin_getstate PF_Fixme
#define PFC_cin_restart PF_Fixme
#define PFC_drawline PF_Fixme
#define PFC_drawcolorcodedstring PF_Fixme
#define PFC_uri_get PF_Fixme
#define PFC_strreplace PF_Fixme
#define PFC_strireplace PF_Fixme
#define PFC_gecko_create PF_Fixme
#define PFC_gecko_destroy PF_Fixme
#define PFC_gecko_navigate PF_Fixme
#define PFC_gecko_keyevent PF_Fixme
#define PFC_gecko_movemouse PF_Fixme
#define PFC_gecko_resize PF_Fixme
#define PFC_gecko_get_texture_extent PF_Fixme
#define PFC_uri_get PF_Fixme

#define PFC_pointsound PF_Fixme
#define PF_getsurfacepointattribute PF_Fixme
#define PFC_gecko_mousemove PF_Fixme
#define PFC_numentityfields PF_Fixme
#define PFC_entityfieldname PF_Fixme
#define PFC_entityfieldtype PF_Fixme
#define PFC_getentityfieldstring PF_Fixme
#define PFC_putentityfieldstring PF_Fixme
#define PFC_WritePicture PF_Fixme
#define PF_cl_ReadPicture PF_Fixme
#define PF_rotatevectorsbytag	PF_Fixme
#define PFC_skinforname	PF_Fixme
#define PF_cs_unproject	PF_Fixme
#define PF_cs_project	PF_Fixme
#define PF_cl_effect	PF_Fixme
#define PF_cl_te_blooddp	PF_Fixme
#define PF_cl_te_bloodshower	PF_Fixme
#define PF_cl_te_explosionrgb	PF_Fixme
#define PF_cl_te_particlecube	PF_Fixme
#define PF_cl_te_spark	PF_Fixme
#define PF_cl_te_particlerain	PF_Fixme
#define PF_cl_te_particlesnow	PF_Fixme
#define PF_cl_te_gunshotquad	PF_Fixme
#define PF_cl_te_spikequad	PF_Fixme
#define PF_cl_te_superspikequad	PF_Fixme
#define PF_cl_te_explosionquad	PF_Fixme
#define PF_cl_te_customflash	PF_Fixme
#define PF_cl_te_smallflash	PF_Fixme
#define PF_cl_te_gunshot	PF_Fixme
#define PF_cl_te_spike	PF_Fixme
#define PF_cl_te_superspike	PF_Fixme
#define PF_cl_te_explosion	PF_Fixme
#define PF_cl_te_wizspike	PF_Fixme
#define PF_cl_te_tarexplosion	PF_Fixme
#define PF_cl_te_lavasplash	PF_Fixme
#define PF_cl_te_teleport	PF_Fixme
#define PF_cl_te_explosion2	PF_Fixme
#define PF_cl_te_lightning1	PF_Fixme
#define PF_cl_te_lightning2	PF_Fixme
#define PF_cl_te_lightning3	PF_Fixme
#define PF_cl_te_knightspike	PF_Fixme
#define PF_cl_te_beam	PF_Fixme
#define PF_cl_te_plasmaburn	PF_Fixme
#define PF_cs_gettagindex	PF_Fixme
#define PF_cs_gettaginfo	PF_Fixme
#define PF_cs_gecko_destroy	PF_Fixme
#define PF_cs_gecko_create	PF_Fixme
#define PF_cs_gecko_navigate	PF_Fixme
#define PF_cs_gecko_keyevent	PF_Fixme
#define PF_cs_gecko_mousemove	PF_Fixme
#define PF_cs_gecko_resize	PF_Fixme
#define PF_cs_gecko_get_texture_extent	PF_Fixme


#define PFC_uri_unescape	PF_Fixme
#define PFC_uri_escape	PF_Fixme
#define PFC_whichpack	PF_Fixme
#define PFC_crc16	PF_Fixme
#define PFC_cvar_defstring	PF_Fixme
#define PFC_tokenizebyseparator	PF_Fixme
#define PFC_strftime	PF_Fixme
#define PFC_strdecolorize	PF_Fixme
#define PFC_strlennocol	PF_Fixme
#define PFC_bufstr_free	PF_Fixme
#define PFC_bufstr_add	PF_Fixme
#define PFC_buf_implode	PF_Fixme
#define PFC_buf_sort	PF_Fixme
#define PFC_buf_copy	PF_Fixme
#define PFC_search_getfilename	PF_Fixme
#define PFC_search_getsize	PF_Fixme
#define PFC_search_end	PF_Fixme
#define PFC_search_begin	PF_Fixme
#define PFC_infoget	PF_Fixme
#define PFC_infoadd	PF_Fixme



//pr_cmds.c builtins that need to be moved to a common.
void VARGS PR_BIError(progfuncs_t *progfuncs, char *format, ...);
void PFC_cvar (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_cvar_string (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_cvar_set (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_cvar_setf (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_print (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_dprint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_error (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_rint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_floor (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_ceil (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Tokenize  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_ArgV  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_ArgC (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_FindString (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_FindFloat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_nextent (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_randomvec (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Sin (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Cos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Sqrt (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_bound (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strlen(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strcat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_ftos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fabs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vtos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_etos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_stof (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_mod (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_substring (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_stov (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_dupstring(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_forgetstring(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Spawn (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_min (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_max (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_registercvar (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_pow (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_chr2str (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_localcmd (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_random (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_randomvector (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fopen (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fclose (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fputs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fgets (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_normalize (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vlen (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vectoyaw (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vectoangles (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_FindFlags (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_findchain (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_findchainfloat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_findchainflags (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_coredump (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_traceon (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_traceoff (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_eprint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_bitshift(progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_registercvar (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Abort(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_externcall (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_externrefcall (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_externvalue (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_externset (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_instr (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_strlennocol (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strdecolorize (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strtolower (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strtoupper (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strftime (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_strstrofs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_str2chr (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_chr2str (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strconv (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_infoadd (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_infoget (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strncmp (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strcasecmp (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strncasecmp (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strpad (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_edict_for_num (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_num_for_edict (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_cvar_defstring (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_itos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_stoi (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_stoh (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_htos (progfuncs_t *prinst, struct globalvars_s *pr_globals);










int CSQC_Key_CSQCToNative(int code);
int CSQC_Key_NativeToCSQC(int code);

//pr_cmds.c builtins that need to be moved to a common.
void VARGS PR_BIError(progfuncs_t *progfuncs, char *format, ...);
void PFC_checkextension (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_print (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_dprint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_error (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_rint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_floor (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_ceil (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Tokenize  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_tokenizebyseparator  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_ArgV  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_FindString (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_FindFloat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_nextent (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_randomvector (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Sin (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Cos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Sqrt (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_bound (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strlen(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strcat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_ftos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fabs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vtos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_etos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_stof (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_mod (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_substring (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_stov (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_dupstring(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_forgetstring(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Spawn (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_min (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_max (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_registercvar (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_pow (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_asin (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_acos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_atan (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_atan2 (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_tan (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_chr2str (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_localcmd (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_random (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fopen (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fclose (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fputs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fgets (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_normalize (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vlen (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vectoyaw (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vectoangles (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_findchain (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_findchainfloat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_coredump (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_traceon (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_traceoff (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_eprint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_search_begin (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_search_end (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_search_getsize (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_search_getfilename (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_WasFreed (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_break (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_crc16 (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_cvar_type (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_uri_escape  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_uri_unescape  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_itos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_stoi (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_stoh (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_htos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PR_fclose_progs (progfuncs_t *prinst);
char *PFC_VarString (progfuncs_t *prinst, int	first, struct globalvars_s *pr_globals);








//pr_cmds.c builtins that need to be moved to a common.
void VARGS PR_BIError(progfuncs_t *progfuncs, char *format, ...);
void PFC_cvar_string (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_cvar_set (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_cvar_setf (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_print (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_dprint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_error (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_rint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_floor (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_ceil (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Tokenize  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_ArgV  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_ArgC (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_FindString (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_FindFloat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_nextent (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_randomvec (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Sin (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Cos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Sqrt (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_bound (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strlen(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strcat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_ftos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fabs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vtos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_etos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_stof (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_mod (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_substring (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_stov (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_dupstring(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_forgetstring(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Spawn (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_min (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_max (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_registercvar (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_pow (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_chr2str (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_localcmd (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_random (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_randomvector (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fopen (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fclose (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fputs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_fgets (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_normalize (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vlen (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vectoyaw (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_vectoangles (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_FindFlags (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_findchain (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_findchainfloat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_findchainflags (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_coredump (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_traceon (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_traceoff (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_eprint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_bitshift(progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_registercvar (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_Abort(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_externcall (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_externrefcall (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_externvalue (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_externset (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_instr (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_strlennocol (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strdecolorize (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strtolower (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strtoupper (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strftime (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_strstrofs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_str2chr (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_chr2str (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strconv (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_infoadd (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_infoget (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strncmp (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strcasecmp (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strncasecmp (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_strpad (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_edict_for_num (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_num_for_edict (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_cvar_defstring (progfuncs_t *prinst, struct globalvars_s *pr_globals);

//these functions are from pr_menu.dat
void PF_CL_is_cached_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_precache_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_free_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawcharacter (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawstring (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawpic (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawline (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawfillpal (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawfillrgb (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawsetcliparea (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawresetcliparea (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawgetimagesize (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_stringwidth (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawsubpic (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PF_cl_keynumtostring (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_cl_findkeysforcommand (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_cl_stringtokeynum(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_cl_getkeybind (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_search_begin (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_search_end (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_search_getsize (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_search_getfilename (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_buf_create  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_buf_del  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_buf_getsize  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_buf_copy  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_buf_sort  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_buf_implode  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_bufstr_get  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_bufstr_set  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_bufstr_add  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PFC_bufstr_free  (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PFC_whichpack (progfuncs_t *prinst, struct globalvars_s *pr_globals);
