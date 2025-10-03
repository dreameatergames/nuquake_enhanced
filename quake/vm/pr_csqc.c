#include "quakedef.h"

//#define TOFIX

// CSQC ENGINEDEVS is used to mark 'interesting' first-port places to modify that may not be obvious at first

#define FULLENGINENAME "CSQCQuake"

float	CL_LerpPoint (void);
#define CL_CalcClientTime CL_LerpPoint

#define cl_csqcdebug 0

/*

  EXT_CSQC is the 'root' extension
  EXT_CSQC_1 are a collection of additional features to cover omissions in the original spec


  note the CHEAT_PARANOID define disables certain EXT_CSQC_1 features,
  in an attempt to prevent the player from finding out where he/she is, thus preventing aimbots.
  This is specifically targetted towards deathmatch mods where each player is a single player.
  In reality, this paranoia provides nothing which could not be done with a cheat proxy.
  Seeing as the client ensures hashes match in the first place, this paranoia gives nothing in the long run.
  Unfortunatly EXT_CSQC was designed around this paranoia.
*/

#ifdef EXT_CSQC

#ifdef GLQUAKE
//#include "glquake.h"	//evil to include this
#endif

//#define CHEAT_PARANOID

#include "pr_common.h"


static progfuncs_t *csqcprogs;

typedef struct csqctreadstate_s {
	float resumetime;
	struct qcthread_s *thread;
	int self;
	int other;

	struct csqctreadstate_s *next;
} csqctreadstate_t;

struct {
	char filename[MAX_QPATH];
	qboolean permissive;
	unsigned int len;
	unsigned int hash;
} csauth;

void *csprogs_ptr;
int csprogs_len;

static csqctreadstate_t *csqcthreads;
qboolean csqc_resortfrags;
qboolean csqc_drawsbar;
qboolean csqc_addcrosshair;
static int num_csqc_edicts;
static int csqc_fakereadbyte;

#define CSQCPROGSGROUP "CSQC progs control"
cvar_t	pr_csmaxedicts = {"pr_csmaxedicts", "3072"};
cvar_t  cl_nocsqc = {"cl_nocsqc", "0"};
cvar_t  pr_csqc_coreonerror = {"pr_csqc_coreonerror", "1"};


#define MASK_DELTA 1
#define MASK_STDVIEWMODEL 2

#define	STEPSIZE	18	//quake really should have put this in a header


// standard effect cvars/sounds
extern cvar_t r_explosionlight;
extern sfx_t			*cl_sfx_wizhit;
extern sfx_t			*cl_sfx_knighthit;
extern sfx_t			*cl_sfx_tink1;
extern sfx_t			*cl_sfx_ric1;
extern sfx_t			*cl_sfx_ric2;
extern sfx_t			*cl_sfx_ric3;
extern sfx_t			*cl_sfx_r_exp3;


//shared constants
typedef enum
{
	VF_MIN = 1,
	VF_MIN_X = 2,
	VF_MIN_Y = 3,
	VF_SIZE = 4,
	VF_SIZE_X = 5,
	VF_SIZE_Y = 6,
	VF_VIEWPORT = 7,
	VF_FOV = 8,
	VF_FOVX = 9,
	VF_FOVY = 10,
	VF_ORIGIN = 11,
	VF_ORIGIN_X = 12,
	VF_ORIGIN_Y = 13,
	VF_ORIGIN_Z = 14,
	VF_ANGLES = 15,
	VF_ANGLES_X = 16,
	VF_ANGLES_Y = 17,
	VF_ANGLES_Z = 18,
	VF_DRAWWORLD = 19,
	VF_ENGINESBAR = 20,
	VF_DRAWCROSSHAIR = 21,
	VF_CARTESIAN_ANGLES = 22,

	//this is a DP-compatibility hack.
	VF_CL_VIEWANGLES_V = 33,
	VF_CL_VIEWANGLES_X = 34,
	VF_CL_VIEWANGLES_Y = 35,
	VF_CL_VIEWANGLES_Z = 36,

#pragma message("FIXME: add cshift")

	//33-36 used by DP...
	VF_PERSPECTIVE = 200,
	//201 used by DP... WTF? CLEARSCREEN
	VF_LPLAYER = 202,
	VF_AFOV = 203,	//aproximate fov (match what the engine would normally use for the fov cvar). p0=fov, p1=zoom
} viewflags;

#define CSQCRF_VIEWMODEL		1 //Not drawn in mirrors
#define CSQCRF_EXTERNALMODEL	2 //drawn ONLY in mirrors
#define CSQCRF_DEPTHHACK		4 //fun depthhack
#define CSQCRF_ADDITIVE			8 //add instead of blend
#define CSQCRF_USEAXIS			16 //use v_forward/v_right/v_up as an axis/matrix - predraw is needed to use this properly
#define CSQCRF_NOSHADOW			32 //don't cast shadows upon other entities (can still be self shadowing, if the engine wishes, and not additive)
#define CSQCRF_FRAMETIMESARESTARTTIMES 64 //EXT_CSQC_1: frame times should be read as (time-frametime).


//If I do it like this, I'll never forget to register something...
#define csqcglobals	\
	globalfunction(init_function,		"CSQC_Init");	\
	globalfunction(worldloaded,			"CSQC_WorldLoaded");	\
	globalfunction(shutdown_function,	"CSQC_Shutdown");	\
	globalfunction(draw_function,		"CSQC_UpdateView");	\
	globalfunction(parse_stuffcmd,		"CSQC_Parse_StuffCmd");	\
	globalfunction(parse_centerprint,	"CSQC_Parse_CenterPrint");	\
	globalfunction(input_event,			"CSQC_InputEvent");	\
	globalfunction(input_frame,			"CSQC_Input_Frame");/*EXT_CSQC_1*/	\
	globalfunction(console_command,		"CSQC_ConsoleCommand");	\
	\
	globalfunction(ent_update,			"CSQC_Ent_Update");	\
	globalfunction(ent_remove,			"CSQC_Ent_Remove");	\
	\
	globalfunction(event_sound,			"CSQC_Event_Sound");	\
	globalfunction(serversound,			"CSQC_ServerSound");/*obsolete, use event_sound*/	\
	globalfunction(loadresource,		"CSQC_LoadResource");/*EXT_CSQC_1*/	\
	globalfunction(parse_tempentity,	"CSQC_Parse_TempEntity");/*EXT_CSQC_ABSOLUTLY_VILE*/	\
	\
	/*These are pointers to the csqc's globals.*/	\
	globalfloat(svtime,					"time");				/*float		Written before entering most qc functions*/	\
	globalfloat(cltime,					"cltime");				/*float		Written before entering most qc functions*/	\
	globalentity(self,					"self");				/*entity	Written before entering most qc functions*/	\
	globalentity(other,					"other");				/*entity	Written before entering most qc functions*/	\
	\
	globalfloat(maxclients,				"maxclients");			/*float		max number of players allowed*/	\
	\
	globalvector(forward,				"v_forward");			/*vector	written by anglevectors*/	\
	globalvector(right,					"v_right");				/*vector	written by anglevectors*/	\
	globalvector(up,					"v_up");				/*vector	written by anglevectors*/	\
	\
	globalfloat(trace_allsolid,			"trace_allsolid");		/*bool		written by traceline*/	\
	globalfloat(trace_startsolid,		"trace_startsolid");	/*bool		written by traceline*/	\
	globalfloat(trace_fraction,			"trace_fraction");		/*float		written by traceline*/	\
	globalfloat(trace_inwater,			"trace_inwater");		/*bool		written by traceline*/	\
	globalfloat(trace_inopen,			"trace_inopen");		/*bool		written by traceline*/	\
	globalvector(trace_endpos,			"trace_endpos");		/*vector	written by traceline*/	\
	globalvector(trace_plane_normal,	"trace_plane_normal");	/*vector	written by traceline*/	\
	globalfloat(trace_plane_dist,		"trace_plane_dist");	/*float		written by traceline*/	\
	globalentity(trace_ent,				"trace_ent");			/*entity	written by traceline*/	\
	globalfloat(trace_surfaceflags,		"trace_surfaceflags");	/*float		written by traceline*/	\
	globalfloat(trace_endcontents,		"trace_endcontents");	/*float		written by traceline EXT_CSQC_1*/	\
	\
	globalfloat(clientcommandframe,		"clientcommandframe");	/*float		the next frame that will be sent*/ \
	globalfloat(servercommandframe,		"servercommandframe");	/*float		the most recent frame received from the server*/ \
	\
	globalfloat(player_localentnum,		"player_localentnum");	/*float		the entity number of the local player*/	\
	globalfloat(player_localnum,		"player_localnum");		/*float		the entity number of the local player*/	\
	globalfloat(intermission,			"intermission");		/*float		set when the client receives svc_intermission*/	\
	globalvector(view_angles,			"view_angles");			/*float		set to the view angles at the start of each new frame (EXT_CSQC_1)*/ \
	globalfloat(dpcompat_sbshowscores,	"sb_showscores");		/*float		ask darkplaces people, its not part of the csqc standard */	\
	\
	globalvector(pmove_org,				"pmove_org");			/*read/written by runplayerphysics*/ \
	globalvector(pmove_vel,				"pmove_vel");			/*read/written by runplayerphysics*/ \
	globalvector(pmove_mins,			"pmove_mins");			/*read/written by runplayerphysics*/ \
	globalvector(pmove_maxs,			"pmove_maxs");			/*read/written by runplayerphysics*/ \
	globalfloat(pmove_jump_held,		"pmove_jump_held");		/*read/written by runplayerphysics*/ \
	globalfloat(pmove_waterjumptime,	"pmove_waterjumptime");	/*read/written by runplayerphysics*/ \
	\
	globalfloat(input_timelength,		"input_timelength");	/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalvector(input_angles,			"input_angles");		/*vector	filled by getinputstate, read by runplayerphysics*/ \
	globalvector(input_movevalues,		"input_movevalues");	/*vector	filled by getinputstate, read by runplayerphysics*/ \
	globalfloat(input_buttons,			"input_buttons");		/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalfloat(input_impulse,			"input_impulse");		/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalfloat(input_lightlevel,		"input_lightlevel");	/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalfloat(input_weapon,			"input_weapon");		/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalfloat(input_servertime,		"input_servertime");	/*float		filled by getinputstate, read by runplayerphysics*/ \
	globalfloat(input_clienttime,		"input_clienttime");	/*float		filled by getinputstate, read by runplayerphysics*/ \
	\
	globalfloat(movevar_gravity,		"movevar_gravity");		/*float		obtained from the server*/ \
	globalfloat(movevar_stopspeed,		"movevar_stopspeed");	/*float		obtained from the server*/ \
	globalfloat(movevar_maxspeed,		"movevar_maxspeed");	/*float		obtained from the server*/ \
	globalfloat(movevar_spectatormaxspeed,"movevar_spectatormaxspeed");		/*float		obtained from the server*/ \
	globalfloat(movevar_accelerate,		"movevar_accelerate");	/*float		obtained from the server*/ \
	globalfloat(movevar_airaccelerate,	"movevar_airaccelerate");	/*float		obtained from the server*/ \
	globalfloat(movevar_wateraccelerate,"movevar_wateraccelerate");	/*float		obtained from the server*/ \
	globalfloat(movevar_friction,		"movevar_friction");	/*float		obtained from the server*/ \
	globalfloat(movevar_waterfriction,	"movevar_waterfriction");	/*float		obtained from the server*/ \
	globalfloat(movevar_entgravity,		"movevar_entgravity");	/*float		obtained from the server*/ \


typedef struct {
#define globalfloat(name,qcname) float *name
#define globalvector(name,qcname) float *name
#define globalentity(name,qcname) int *name
#define globalstring(name,qcname) string_t *name
#define globalfunction(name,qcname) func_t name
//These are the functions the engine will call to, found by name.

	csqcglobals

#undef globalfloat
#undef globalvector
#undef globalentity
#undef globalstring
#undef globalfunction
} csqcglobals_t;
static csqcglobals_t csqcg;

static void CSQC_FindGlobals(void)
{
#define globalfloat(name,qcname) csqcg.name = (float*)csqcprogs->FindGlobal(csqcprogs, qcname, 0);
#define globalvector(name,qcname) csqcg.name = (float*)csqcprogs->FindGlobal(csqcprogs, qcname, 0);
#define globalentity(name,qcname) csqcg.name = (int*)csqcprogs->FindGlobal(csqcprogs, qcname, 0);
#define globalstring(name,qcname) csqcg.name = (string_t*)csqcprogs->FindGlobal(csqcprogs, qcname, 0);
#define globalfunction(name,qcname) csqcg.name = csqcprogs->FindFunction(csqcprogs,qcname,PR_ANY);

	csqcglobals

#undef globalfloat
#undef globalvector
#undef globalentity
#undef globalstring
#undef globalfunction

	if (csqcg.svtime)
		*csqcg.svtime = cl.mtime[0];
	if (csqcg.cltime)
		*csqcg.cltime = cl.time;

	if (csqcg.maxclients)
		*csqcg.maxclients = cl.maxclients;
}



//this is the list for all the csqc fields.
//(the #define is so the list always matches the ones pulled out)
#define csqcfields	\
	fieldfloat(entnum);		\
	fieldfloat(modelindex);	\
	fieldvector(origin);	\
	fieldvector(angles);	\
	fieldvector(velocity);	\
	fieldfloat(pmove_flags);		/*transparency*/	\
	fieldfloat(alpha);		/*transparency*/	\
	fieldfloat(scale);		/*model scale*/		\
	fieldfloat(fatness);	/*expand models X units along their normals.*/	\
	fieldfloat(skin);		\
	fieldfloat(colormap);	\
	fieldfloat(effects);	\
	fieldfloat(flags);		\
	fieldfloat(frame);		\
	fieldfloat(frame2);		/*EXT_CSQC_1*/\
	fieldfloat(frame1time);	/*EXT_CSQC_1*/\
	fieldfloat(frame2time);	/*EXT_CSQC_1*/\
	fieldfloat(lerpfrac);	/*EXT_CSQC_1*/\
	fieldfloat(renderflags);\
	fieldfloat(forceshader);/*FTE_CSQC_SHADERS*/\
	fieldfloat(dimension_hit);	\
	fieldfloat(dimension_solid);	\
							\
	fieldfloat(baseframe);	/*FTE_CSQC_BASEFRAME*/\
	fieldfloat(baseframe2);	/*FTE_CSQC_BASEFRAME*/\
	fieldfloat(baseframe1time);	/*FTE_CSQC_BASEFRAME*/\
	fieldfloat(baseframe2time);	/*FTE_CSQC_BASEFRAME*/\
	fieldfloat(baselerpfrac);	/*FTE_CSQC_BASEFRAME*/\
	fieldfloat(basebone);	/*FTE_CSQC_BASEFRAME*/\
							\
  	fieldfloat(bonecontrol1);	/*FTE_CSQC_HALFLIFE_MODELS*/\
	fieldfloat(bonecontrol2);	/*FTE_CSQC_HALFLIFE_MODELS*/\
	fieldfloat(bonecontrol3);	/*FTE_CSQC_HALFLIFE_MODELS*/\
	fieldfloat(bonecontrol4);	/*FTE_CSQC_HALFLIFE_MODELS*/\
	fieldfloat(bonecontrol5);	/*FTE_CSQC_HALFLIFE_MODELS*/\
	fieldfloat(subblendfrac);	/*FTE_CSQC_HALFLIFE_MODELS*/\
	fieldfloat(basesubblendfrac);	/*FTE_CSQC_HALFLIFE_MODELS+FTE_CSQC_BASEFRAME*/\
							\
	fieldfloat(skeletonindex);		/*FTE_CSQC_SKELETONOBJECTS*/\
							\
	fieldfloat(drawmask);	/*So that the qc can specify all rockets at once or all bannanas at once*/	\
	fieldfunction(predraw);	/*If present, is called just before it's drawn.*/	\
							\
	fieldstring(model);		\
	fieldfloat(ideal_yaw);	\
	fieldfloat(ideal_pitch);\
	fieldfloat(yaw_speed);	\
	fieldfloat(pitch_speed);\
							\
	fieldentity(chain);		\
	fieldentity(enemy);		\
	fieldentity(groundentity);	\
	fieldentity(owner);		\
							\
	fieldfunction(touch);	\
							\
	fieldfloat(solid);		\
	fieldvector(mins);		\
	fieldvector(maxs);		\
	fieldvector(size);		\
	fieldvector(absmin);	\
	fieldvector(absmax);	\
	fieldfloat(hull);		/*(FTE_PEXT_HEXEN2)*/


//note: doesn't even have to match the clprogs.dat :)
typedef struct {
#define fieldfloat(name) float name
#define fieldvector(name) vec3_t name
#define fieldentity(name) int name
#define fieldstring(name) string_t name
#define fieldfunction(name) func_t name
csqcfields
#undef fieldfloat
#undef fieldvector
#undef fieldentity
#undef fieldstring
#undef fieldfunction
} csqcentvars_t;

typedef struct csqcedict_s
{
	qboolean	isfree;
	float		freetime; // sv.time when the object was freed
	int			entnum;
	qboolean	readonly;	//world
	csqcentvars_t	*v;

	//add whatever you wish here
	link_t	area;
} csqcedict_t;

static csqcedict_t *csqc_edicts;	//consider this 'world'


static void CSQC_InitFields(void)
{	//CHANGING THIS FUNCTION REQUIRES CHANGES TO csqcentvars_t
#define fieldfloat(name) csqcprogs->RegisterFieldVar(csqcprogs, ev_float, #name, (int)&((csqcentvars_t*)0)->name, -1)
#define fieldvector(name) csqcprogs->RegisterFieldVar(csqcprogs, ev_vector, #name, (int)&((csqcentvars_t*)0)->name, -1)
#define fieldentity(name) csqcprogs->RegisterFieldVar(csqcprogs, ev_entity, #name, (int)&((csqcentvars_t*)0)->name, -1)
#define fieldstring(name) csqcprogs->RegisterFieldVar(csqcprogs, ev_string, #name, (int)&((csqcentvars_t*)0)->name, -1)
#define fieldfunction(name) csqcprogs->RegisterFieldVar(csqcprogs, ev_function, #name, (int)&((csqcentvars_t*)0)->name, -1)
csqcfields	//any *64->int32 casts are erroneous, it's biased off NULL.
#undef fieldfloat
#undef fieldvector
#undef fieldentity
#undef fieldstring
#undef fieldfunction
}

static csqcedict_t **csqcent;
static int maxcsqcentities;

static int csqcentsize;

static char *csqcmapentitydata;
static qboolean csqcmapentitydataloaded;







static model_t *CSQC_GetModelForIndex(int index);
static void CS_LinkEdict(csqcedict_t *ent, qboolean touchtriggers);


typedef struct csareanode_s
{
	int		axis;		// -1 = leaf node
	float	dist;
	struct csareanode_s	*children[2];
	link_t	trigger_edicts;
	link_t	solid_edicts;
} csareanode_t;

#define	AREA_DEPTH	4
#define	AREA_NODES	32

csareanode_t	cs_areanodes[AREA_NODES];
int			cs_numareanodes;
csareanode_t *CS_CreateAreaNode (int depth, vec3_t mins, vec3_t maxs)
{
	csareanode_t	*anode;
	vec3_t		size;
	vec3_t		mins1, maxs1, mins2, maxs2;

	anode = &cs_areanodes[cs_numareanodes];
	cs_numareanodes++;

	ClearLink (&anode->trigger_edicts);
	ClearLink (&anode->solid_edicts);

	if (depth == AREA_DEPTH)
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}

	VectorSubtract (maxs, mins, size);
	if (size[0] > size[1])
		anode->axis = 0;
	else
		anode->axis = 1;

	anode->dist = 0.5 * (maxs[anode->axis] + mins[anode->axis]);
	VectorCopy (mins, mins1);
	VectorCopy (mins, mins2);
	VectorCopy (maxs, maxs1);
	VectorCopy (maxs, maxs2);

	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

	anode->children[0] = CS_CreateAreaNode (depth+1, mins2, maxs2);
	anode->children[1] = CS_CreateAreaNode (depth+1, mins1, maxs1);

	return anode;
}

void CS_ClearWorld (void)
{
	int i;

	memset (cs_areanodes, 0, sizeof(cs_areanodes));
	cs_numareanodes = 0;
	if (cl.worldmodel)
		CS_CreateAreaNode (0, cl.worldmodel->mins, cl.worldmodel->maxs);
	else
	{
		vec3_t mins, maxs;
		int i;
		for (i = 0; i < 3; i++)
		{
			mins[i] = -4096;
			maxs[i] = 4096;
		}
		CS_CreateAreaNode (0, mins, maxs);
	}

	for (i = 1; i < num_csqc_edicts; i++)
		CS_LinkEdict((csqcedict_t*)csqcprogs->EdictNum(csqcprogs, i), false);
}

#define MAX_NODELINKS	256	//all this means is that any more than this will not touch.
static csqcedict_t *csnodelinks[MAX_NODELINKS];
void CS_TouchLinks ( csqcedict_t *ent, csareanode_t *node )
{	//Spike: rewritten this function to cope with killtargets used on a few maps.
	link_t		*l, *next;
	csqcedict_t		*touch;
	int			old_self, old_other;

	int linkcount = 0, ln;

	//work out who they are first.
	for (l = node->trigger_edicts.next ; l != &node->trigger_edicts ; l = next)
	{
		if (linkcount == MAX_NODELINKS)
			break;
		next = l->next;
		touch = (csqcedict_t*)EDICT_FROM_AREA(l);
		if (touch == ent)
			continue;

		if (!touch->v->touch || touch->v->solid != SOLID_TRIGGER)
			continue;

		if (ent->v->absmin[0] > touch->v->absmax[0]
		|| ent->v->absmin[1] > touch->v->absmax[1]
		|| ent->v->absmin[2] > touch->v->absmax[2]
		|| ent->v->absmax[0] < touch->v->absmin[0]
		|| ent->v->absmax[1] < touch->v->absmin[1]
		|| ent->v->absmax[2] < touch->v->absmin[2] )
			continue;

//		if (!((int)ent->xv->dimension_solid & (int)touch->xv->dimension_hit))
//			continue;

		csnodelinks[linkcount++] = touch;
	}

	old_self = *csqcg.self;
	old_other = *csqcg.other;
	for (ln = 0; ln < linkcount; ln++)
	{
		touch = csnodelinks[ln];

		//make sure nothing moved it away
		if (touch->isfree)
			continue;
		if (!touch->v->touch || touch->v->solid != SOLID_TRIGGER)
			continue;
		if (ent->v->absmin[0] > touch->v->absmax[0]
		|| ent->v->absmin[1] > touch->v->absmax[1]
		|| ent->v->absmin[2] > touch->v->absmax[2]
		|| ent->v->absmax[0] < touch->v->absmin[0]
		|| ent->v->absmax[1] < touch->v->absmin[1]
		|| ent->v->absmax[2] < touch->v->absmin[2] )
			continue;

//		if (!((int)ent->xv->dimension_solid & (int)touch->xv->dimension_hit))	//didn't change did it?...
//			continue;

		*csqcg.self = csqcprogs->EdictToProgs(csqcprogs, (edict_t*)touch);
		*csqcg.other = csqcprogs->EdictToProgs(csqcprogs, (edict_t*)ent);

		csqcprogs->ExecuteProgram (csqcprogs, touch->v->touch);

		if (ent->isfree)
			break;
	}
	*csqcg.self = old_self;
	*csqcg.other = old_other;


// recurse down both sides
	if (node->axis == -1 || ent->isfree)
		return;
	
	if ( ent->v->absmax[node->axis] > node->dist )
		CS_TouchLinks ( ent, node->children[0] );
	if ( ent->v->absmin[node->axis] < node->dist )
		CS_TouchLinks ( ent, node->children[1] );
}

static void CS_UnlinkEdict (csqcedict_t *ent)
{
	if (!ent->area.prev)
		return;		// not linked in anywhere
	RemoveLink (&ent->area);
	ent->area.prev = ent->area.next = NULL;
}

static void CS_LinkEdict(csqcedict_t *ent, qboolean touchtriggers)
{
	csareanode_t *node;

	if (ent->area.prev)
		CS_UnlinkEdict (ent);	// unlink from old position

	if (ent == csqc_edicts)
		return;		// don't add the world

	//FIXME: use some sort of area grid ?
	VectorAdd(ent->v->origin, ent->v->mins, ent->v->absmin);
	VectorAdd(ent->v->origin, ent->v->maxs, ent->v->absmax);

	if ((int)ent->v->flags & FL_ITEM)
	{
		ent->v->absmin[0] -= 15;
		ent->v->absmin[1] -= 15;
		ent->v->absmax[0] += 15;
		ent->v->absmax[1] += 15;
	}
	else
	{	// because movement is clipped an epsilon away from an actual edge,
		// we must fully check even when bounding boxes don't quite touch
		ent->v->absmin[0] -= 1;
		ent->v->absmin[1] -= 1;
		ent->v->absmin[2] -= 1;
		ent->v->absmax[0] += 1;
		ent->v->absmax[1] += 1;
		ent->v->absmax[2] += 1;
	}

	if (!ent->v->solid)
		return;

	// find the first node that the ent's box crosses
	node = cs_areanodes;
	while (1)
	{
		if (node->axis == -1)
			break;
		if (ent->v->absmin[node->axis] > node->dist)
			node = node->children[0];
		else if (ent->v->absmax[node->axis] < node->dist)
			node = node->children[1];
		else
			break;		// crosses the node
	}

// link it in

	if (ent->v->solid == SOLID_TRIGGER)
		InsertLinkBefore(&ent->area, &node->trigger_edicts);
	else
		InsertLinkBefore(&ent->area, &node->solid_edicts);

	// if touch_triggers, touch all entities at this node and decend for more
	if (touchtriggers)
		CS_TouchLinks(ent, cs_areanodes);
}

typedef struct {
	int type;
	trace_t trace;
	vec3_t boxmins;	//mins/max of total move.
	vec3_t boxmaxs;
	vec3_t start;
	vec3_t end;
	vec3_t mins;	//mins/max of ent
	vec3_t maxs;
	csqcedict_t *passedict;
} csmoveclip_t;
#define	CSEDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,csqcedict_t,area)
static void CS_ClipToLinks ( csareanode_t *node, csmoveclip_t *clip )
{
	model_t		*model;
	trace_t		tr;
	link_t		*l, *next;
	csqcedict_t		*touch;

	//work out who they are first.
	for (l = node->solid_edicts.next ; l != &node->solid_edicts ; l = next)
	{
		next = l->next;
		touch = (csqcedict_t*)CSEDICT_FROM_AREA(l);
		if (touch->v->solid == SOLID_NOT)
			continue;
		if (touch == clip->passedict)
			continue;
		if (touch->v->solid == SOLID_TRIGGER)
			continue;

		if (clip->type & MOVE_NOMONSTERS && touch->v->solid != SOLID_BSP)
			continue;

		if (clip->passedict)
		{
#ifdef SOLID_CORPSE
			// don't clip corpse against character
			if (clip->passedict->v->solid == SOLID_CORPSE && (touch->v->solid == SOLID_SLIDEBOX || touch->v->solid == SOLID_CORPSE))
				continue;
			// don't clip character against corpse
			if (clip->passedict->v->solid == SOLID_SLIDEBOX && touch->v->solid == SOLID_CORPSE)
				continue;
#endif
		}

		if (clip->boxmins[0] > touch->v->absmax[0]
		|| clip->boxmins[1] > touch->v->absmax[1]
		|| clip->boxmins[2] > touch->v->absmax[2]
		|| clip->boxmaxs[0] < touch->v->absmin[0]
		|| clip->boxmaxs[1] < touch->v->absmin[1]
		|| clip->boxmaxs[2] < touch->v->absmin[2] )
			continue;

		if (clip->passedict && clip->passedict->v->size[0] && !touch->v->size[0])
			continue;	// points never interact

	// might intersect, so do an exact clip
		if (clip->trace.allsolid)
			return;
		if (clip->passedict)
		{
		 	if ((csqcedict_t*)csqcprogs->ProgsToEdict(csqcprogs, touch->v->owner) == clip->passedict)
				continue;	// don't clip against own missiles
			if ((csqcedict_t*)csqcprogs->ProgsToEdict(csqcprogs, clip->passedict->v->owner) == touch)
				continue;	// don't clip against owner
		}


		if (!((int)clip->passedict->v->dimension_solid & (int)touch->v->dimension_hit))
			continue;

		model = CSQC_GetModelForIndex(touch->v->modelindex);
		if (!model)
			continue;

#ifdef TOFIX
		model->funcs.Trace(model, 0, 0, clip->start, clip->end, clip->mins, clip->maxs, &tr);
		if (tr.fraction < clip->trace.fraction)
		{
			tr.ent = (void*)touch;
			clip->trace = tr;
		}
#endif
	}
}

hull_t *CS_HullForEntity (csqcedict_t *ent, vec3_t mins, vec3_t maxs, vec3_t offset)
{
	model_t		*model;
	vec3_t		size;
	vec3_t		hullmins, hullmaxs;
	hull_t		*hull;

// decide which clipping hull to use, based on the size
	if (ent->v->solid == SOLID_BSP)
	{	// explicit hulls in the BSP model

		model = CSQC_GetModelForIndex(ent->v->modelindex);

		if (!model || model->type != mod_brush)
			Sys_Error ("SOLID_BSP with a non bsp model");

		VectorSubtract (maxs, mins, size);
		if (size[0] < 3)
			hull = &model->hulls[0];
		else if (size[0] <= 32)
			hull = &model->hulls[1];
		else
			hull = &model->hulls[2];

// calculate an offset value to center the origin
		VectorSubtract (hull->clip_mins, mins, offset);
		VectorAdd (offset, ent->v->origin, offset);
	}
	else
	{	// create a temp hull from bounding box sizes

		VectorSubtract (ent->v->mins, maxs, hullmins);
		VectorSubtract (ent->v->maxs, mins, hullmaxs);
		hull = SV_HullForBox (hullmins, hullmaxs);
		
		VectorCopy (ent->v->origin, offset);
	}


	return hull;
}

static trace_t CS_Move(vec3_t v1, vec3_t mins, vec3_t maxs, vec3_t v2, float nomonsters, csqcedict_t *passedict)
{
	csmoveclip_t clip;

	if (cl.worldmodel)
	{
		vec3_t		offset;
		vec3_t		start_l, end_l;
		hull_t		*hull;

	// fill in a default trace
		memset (&clip.trace, 0, sizeof(trace_t));
		clip.trace.fraction = 1;
		clip.trace.allsolid = true;
		VectorCopy (v2, clip.trace.endpos);

	// get the clipping hull
		hull = CS_HullForEntity (csqc_edicts, mins, maxs, offset);

		VectorSubtract (v1, offset, start_l);
		VectorSubtract (v2, offset, end_l);

		SV_RecursiveHullCheck (hull, hull->firstclipnode, 0, 1, start_l, end_l, &clip.trace);

		if (clip.trace.fraction != 1)
			VectorAdd (clip.trace.endpos, offset, clip.trace.endpos);

		clip.trace.ent = (void*)csqc_edicts;
	}
	else
	{
		memset(&clip.trace, 0, sizeof(clip.trace));
		clip.trace.fraction = 1;
		VectorCopy(v2, clip.trace.endpos);
		clip.trace.ent = (void*)csqc_edicts;
	}

//why use trace.endpos instead?
//so that if we hit a wall early, we don't have a box covering the whole world because of a shotgun trace.
	clip.boxmins[0] = ((v1[0] < clip.trace.endpos[0])?v1[0]:clip.trace.endpos[0]) - mins[0]-1;
	clip.boxmins[1] = ((v1[1] < clip.trace.endpos[1])?v1[1]:clip.trace.endpos[1]) - mins[1]-1;
	clip.boxmins[2] = ((v1[2] < clip.trace.endpos[2])?v1[2]:clip.trace.endpos[2]) - mins[2]-1;
	clip.boxmaxs[0] = ((v1[0] > clip.trace.endpos[0])?v1[0]:clip.trace.endpos[0]) + maxs[0]+1;
	clip.boxmaxs[1] = ((v1[1] > clip.trace.endpos[1])?v1[1]:clip.trace.endpos[1]) + maxs[1]+1;
	clip.boxmaxs[2] = ((v1[2] > clip.trace.endpos[2])?v1[2]:clip.trace.endpos[2]) + maxs[2]+1;

	VectorCopy(mins, clip.mins);
	VectorCopy(maxs, clip.maxs);
	VectorCopy(v1, clip.start);
	VectorCopy(v2, clip.end);
	clip.passedict = passedict;

	CS_ClipToLinks(cs_areanodes, &clip);
	return clip.trace;
}

static void CS_CheckVelocity(csqcedict_t *ent)
{
}









static void PF_cs_remove (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ed;

	ed = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM0);

	if (ed->isfree)
	{
		Con_DPrintf("CSQC Tried removing free entity\n");
		return;
	}

	CS_UnlinkEdict(ed);
	csqcprogs->EntFree (prinst, (void*)ed);
}

//too specific to the prinst's builtins.
static void PF_Fixme (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_Printf("\n");

	prinst->RunError(prinst, "\nBuiltin %i not implemented.\nCSQC is not compatible.", prinst->lastcalledbuiltinnumber);
	PR_BIError (prinst, "bulitin not implemented");
}
static void PF_NoCSQC (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_Printf("\n");

	prinst->RunError(prinst, "\nBuiltin %i does not make sense in csqc.\nCSQC is not compatible.", prinst->lastcalledbuiltinnumber);
	PR_BIError (prinst, "bulitin not implemented");
}

static void PF_cl_cprint (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *str = prinst->VarString(prinst, 0);
	SCR_CenterPrint(str);
}

static void PF_cs_makevectors (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (!csqcg.forward || !csqcg.right || !csqcg.up)
		Host_EndGame("PF_makevectors: one of v_forward, v_right or v_up was not defined\n");
	AngleVectors (QC_VECTOR(OFS_PARM0), csqcg.forward, csqcg.right, csqcg.up);
}

/*
void QuaternainToAngleMatrix(float *quat, vec3_t *mat)
{
	float xx      = quat[0] * quat[0];
    float xy      = quat[0] * quat[1];
    float xz      = quat[0] * quat[2];
    float xw      = quat[0] * quat[3];
    float yy      = quat[1] * quat[1];
    float yz      = quat[1] * quat[2];
    float yw      = quat[1] * quat[3];
    float zz      = quat[2] * quat[2];
    float zw      = quat[2] * quat[3];
    mat[0][0]  = 1 - 2 * ( yy + zz );
    mat[0][1]  =     2 * ( xy - zw );
    mat[0][2]  =     2 * ( xz + yw );
    mat[1][0]  =     2 * ( xy + zw );
    mat[1][1]  = 1 - 2 * ( xx + zz );
    mat[1][2]  =     2 * ( yz - xw );
    mat[2][0]  =     2 * ( xz - yw );
    mat[2][1]  =     2 * ( yz + xw );
    mat[2][2] = 1 - 2 * ( xx + yy );
}

void quaternion_multiply(float *a, float *b, float *c)
{
#define x1 a[0]
#define y1 a[1]
#define z1 a[2]
#define w1 a[3]
#define x2 b[0]
#define y2 b[1]
#define z2 b[2]
#define w2 b[3]
	c[0] = w1*x2 + x1*w2 + y1*z2 - z1*y2;
	c[1] = w1*y2 + y1*w2 + z1*x2 - x1*z2;
	c[2] = w1*z2 + z1*w2 + x1*y2 - y1*x2;
	c[3] = w1*w2 - x1*x2 - y1*y2 - z1*z2;
}

void quaternion_rotation(float pitch, float roll, float yaw, float angle, float *quat)
{
	float sin_a, cos_a;

	sin_a = sin( angle / 360 );
    cos_a = cos( angle / 360 );
    quat[0]    = pitch	* sin_a;
    quat[1]    = yaw	* sin_a;
    quat[2]    = roll	* sin_a;
    quat[3]    = cos_a;
}

void EularToQuaternian(vec3_t angles, float *quat)
{
  float x[4] = {sin(angles[2]/360), 0, 0, cos(angles[2]/360)};
  float y[4] = {0, sin(angles[1]/360), 0, cos(angles[1]/360)};
  float z[4] = {0, 0, sin(angles[0]/360), cos(angles[0]/360)};
  float t[4];
  quaternion_multiply(x, y, t);
  quaternion_multiply(t, z, quat);
}
*/

static model_t *CSQC_GetModelForIndex(int index)
{
	if (index == 0)
		return NULL;
	else if (index > 0 && index < MAX_MODELS)
		return cl.model_precache[index];
	else if (index < 0 && index > -MAX_CSQCMODELS)
		return cl.model_csqcprecache[-index];
	else
		return NULL;
}

static qboolean CopyCSQCEdictToEntity(csqcedict_t *in, entity_t *out)
{
	int i;
	model_t *model;
	unsigned int rflags;

	//make sure that it'll actually be visible
	i = in->v->modelindex;
	model = CSQC_GetModelForIndex(in->v->modelindex);
	if (!model)
		return false; //there might be other ent types later as an extension that stop this.

	//they don't really care where it ends up, so presumably they wanted a visible ent.
	//so lets give them one
	if (!out)
	{
		//ENGINEDEVS: This function will quickly run out of available entities
		out = CL_NewTempEntity();
		if (!out)
			return false;
	}
	else
		memset(out, 0, sizeof(*out));
	out->model = model;

	if (in->v->renderflags)
	{
		rflags = in->v->renderflags;
		if (rflags & CSQCRF_VIEWMODEL)
			out->renderflags |= RF_DEPTHHACK|RF_WEAPONMODEL;
		if (rflags & CSQCRF_EXTERNALMODEL)
			out->renderflags |= RF_EXTERNALMODEL;
		if (rflags & CSQCRF_DEPTHHACK)
			out->renderflags |= RF_DEPTHHACK;
		if (rflags & CSQCRF_ADDITIVE)
			out->renderflags |= RF_ADDATIVE;
		//CSQCRF_USEAXIS is below
		if (rflags & CSQCRF_NOSHADOW)
			out->renderflags |= RF_NOSHADOW;
		//CSQCRF_FRAMETIMESARESTARTTIMES is below
	}
	else
		rflags = 0;

	VectorCopy(in->v->origin, out->origin);
#ifdef TOFIX
	if (rflags & CSQCRF_USEAXIS)
	{
		VectorCopy(csqcg.forward, out->axis[0]);
		VectorNegate(csqcg.right, out->axis[1]);
		VectorCopy(csqcg.up, out->axis[2]);
		out->scale = 1;
	}
	else
#endif
	{
		VectorCopy(in->v->angles, out->angles);
	}

	if (in->v->colormap > 0 && in->v->colormap <= cl.maxclients)
	{
		out->colormap = cl.scores[(int)in->v->colormap-1].translations;
	} // TODO: DP COLORMAP extension?


	out->frame = in->v->frame;
#ifdef TOFIX
	sequence start times
#endif
	//ENGINEDEVS: we don't support frame blending in this engine
	//so you'll need to add that here for your engine

	out->skinnum = in->v->skin;
	//ENGINEDEVS: copy out alpha and other entity rendering fields

	return true;
}

static void PF_cs_makestatic (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{	//still does a remove.
	csqcedict_t *in = (void*)QC_EDICT(prinst, OFS_PARM0);
	entity_t *ent;

	if (cl.num_statics >= MAX_STATIC_ENTITIES)
	{
		Con_Printf ("Too many static entities");

		PF_cs_remove(prinst, pr_globals);
		return;
	}

	ent = &cl_static_entities[cl.num_statics];
	if (CopyCSQCEdictToEntity(in, ent))
	{
		cl.num_statics++;
		R_AddEfrags(ent);
	}

	PF_cs_remove(prinst, pr_globals);
}

static void PF_R_AddEntity(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *in = (void*)QC_EDICT(prinst, OFS_PARM0);

	CopyCSQCEdictToEntity(in, NULL);
}

static void PF_R_AddDynamicLight(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	dlight_t *dl;
	float *org = QC_VECTOR(OFS_PARM0);
	float radius = QC_FLOAT(OFS_PARM1);
	float *rgb = QC_VECTOR(OFS_PARM2);

	dl = CL_AllocDlight (0);
	VectorCopy (org,  dl->origin);
	dl->radius = radius;
	dl->die = cl.time;
	//FIXME: your engine might support dlight colours, in which case copy out from rgb
}

static void PF_R_AddEntityMask(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int mask = QC_FLOAT(OFS_PARM0);
	csqcedict_t *ent;
	int e;

	int oldself = *csqcg.self;

	if (cl.worldmodel)
	{
		if (mask & MASK_DELTA)
		{
			CL_RelinkEntities ();
		}
	}

	for (e=1; e < *prinst->parms->sv_num_edicts; e++)
	{
		ent = (csqcedict_t*)prinst->EdictNum(prinst, e);
		if (ent->isfree)
			continue;

		if ((int)ent->v->drawmask & mask)
		{
			if (ent->v->predraw)
			{
				*csqcg.self = prinst->EdictToProgs(prinst, (void*)ent);
				prinst->ExecuteProgram(prinst, ent->v->predraw);

				if (ent->isfree)
					continue;	//bummer...
			}

			CopyCSQCEdictToEntity(ent, NULL);
		}
	}
	*csqcg.self = oldself;

	if (cl.worldmodel)
	{
		if (mask & MASK_STDVIEWMODEL)
		{
#ifdef TOFIX
			CL_LinkViewModel ();
#endif
		}
		CL_UpdateTEnts ();
	}
}

qboolean csqc_rebuildmatricies;
#ifdef TOFIX
float mvp[12];
float mvpi[12];
static void buildmatricies(void)
{
	float modelview[16];
	float proj[16];

	Matrix4_ModelViewMatrix(modelview, r_refdef.viewangles, r_refdef.vieworg);
	Matrix4_Projection2(proj, r_refdef.fov_x, r_refdef.fov_y, 4);
	Matrix4_Multiply(proj, modelview, mvp);
	Matrix4_Invert_Simple((matrix4x4_t*)mvpi, (matrix4x4_t*)mvp);	//not actually used in this function.

	csqc_rebuildmatricies = false;
}
static void PF_cs_project (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (csqc_rebuildmatricies)
		buildmatricies();


	{
		float *in = QC_VECTOR(OFS_PARM0);
		float *out = QC_VECTOR(OFS_RETURN);
		float v[4], tempv[4];

		v[0] = in[0];
		v[1] = in[1];
		v[2] = in[2];
		v[3] = 1;

		Matrix4_Transform4(mvp, v, tempv);

		tempv[0] /= tempv[3];
		tempv[1] /= tempv[3];
		tempv[2] /= tempv[3];

		out[0] = (1+tempv[0])/2;
		out[1] = (1+tempv[1])/2;
		out[2] = (1+tempv[2])/2;

		out[0] = out[0]*r_refdef.vrect.width + r_refdef.vrect.x;
		out[1] = out[1]*r_refdef.vrect.height + r_refdef.vrect.y;
	}
}
static void PF_cs_unproject (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (csqc_rebuildmatricies)
		buildmatricies();


	{
		float *in = QC_VECTOR(OFS_PARM0);
		float *out = QC_VECTOR(OFS_RETURN);

		float v[4], tempv[4];

		out[0] = (out[0]-r_refdef.vrect.x)/r_refdef.vrect.width;
		out[1] = (out[1]-r_refdef.vrect.y)/r_refdef.vrect.height;

		v[0] = in[0]*2-1;
		v[1] = in[1]*2-1;
		v[2] = in[2]*2-1;
		v[3] = 1;

		Matrix4_Transform4(mvpi, v, tempv);

		out[0] = tempv[0];
		out[1] = tempv[1];
		out[2] = tempv[2];
	}
}
#endif

//float CalcFov (float fov_x, float width, float height);
//clear scene, and set up the default stuff.
static void PF_R_ClearScene (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float CalcFov (float fov_x, float width, float height);
	extern cvar_t scr_fov;

	csqc_rebuildmatricies = true;

	CL_DecayLights ();

	CL_ClearDisplayList ();

	V_CalcRefdef();	//set up the defaults (for player 0)
	r_refdef.fov_x = scr_fov.value;
	r_refdef.fov_y = CalcFov (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);
	/*
	VectorCopy(cl.simangles[csqc_lplayernum], r_refdef.viewangles);
	VectorCopy(cl.simorg[csqc_lplayernum], r_refdef.vieworg);
	r_refdef.flags = 0;

	r_refdef.vrect.x = 0;
	r_refdef.vrect.y = 0;
	r_refdef.vrect.width = vid.width;
	r_refdef.vrect.height = vid.height;

	r_refdef.fov_x = scr_fov.value;
	r_refdef.fov_y = CalcFov (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);
	*/

	csqc_addcrosshair = false;
	csqc_drawsbar = false;
}

static void PF_R_SetViewFlag(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	viewflags parametertype = QC_FLOAT(OFS_PARM0);
	float *p = QC_VECTOR(OFS_PARM1);

	csqc_rebuildmatricies = true;

	QC_FLOAT(OFS_RETURN) = 1;
	switch(parametertype)
	{
	case VF_FOV:
		r_refdef.fov_x = p[0];
		r_refdef.fov_y = p[1];
		break;

	case VF_FOVX:
		r_refdef.fov_x = *p;
		break;

	case VF_FOVY:
		r_refdef.fov_y = *p;
		break;

	case VF_AFOV:
		{
			float frustumx, frustumy;
			frustumy = tan(p[0] * (M_PI/360)) * 0.75;
			if (*prinst->callargc > 2)
				frustumy *= QC_FLOAT(OFS_PARM2);
			frustumx = frustumy * vid.width / vid.height /* / vid.pixelheight*/;
			r_refdef.fov_x = atan2(frustumx, 1) * (360/M_PI);
			r_refdef.fov_y = atan2(frustumy, 1) * (360/M_PI);
		}
		break;

	case VF_ORIGIN:
		VectorCopy(p, r_refdef.vieworg);
		break;

	case VF_ORIGIN_Z:
	case VF_ORIGIN_X:
	case VF_ORIGIN_Y:
		r_refdef.vieworg[parametertype-VF_ORIGIN_X] = *p;
		break;

	case VF_ANGLES:
		VectorCopy(p, r_refdef.viewangles);
		break;
	case VF_ANGLES_X:
	case VF_ANGLES_Y:
	case VF_ANGLES_Z:
		r_refdef.viewangles[parametertype-VF_ANGLES_X] = *p;
		break;

	case VF_CL_VIEWANGLES_V:
		VectorCopy(p, cl.viewangles);
		break;
	case VF_CL_VIEWANGLES_X:
	case VF_CL_VIEWANGLES_Y:
	case VF_CL_VIEWANGLES_Z:
		cl.viewangles[parametertype-VF_CL_VIEWANGLES_X] = *p;
		break;

	case VF_CARTESIAN_ANGLES:
		Con_Printf("WARNING: CARTESIAN ANGLES ARE NOT YET SUPPORTED!\n");
		break;

	case VF_VIEWPORT:
		r_refdef.vrect.x = p[0];
		r_refdef.vrect.y = p[1];
		p+=3;
		r_refdef.vrect.width = p[0];
		r_refdef.vrect.height = p[1];
		break;

	case VF_SIZE_X:
		r_refdef.vrect.width = *p;
		break;
	case VF_SIZE_Y:
		r_refdef.vrect.height = *p;
		break;
	case VF_SIZE:
		r_refdef.vrect.width = p[0];
		r_refdef.vrect.height = p[1];
		break;

	case VF_MIN_X:
		r_refdef.vrect.x = *p;
		break;
	case VF_MIN_Y:
		r_refdef.vrect.y = *p;
		break;
	case VF_MIN:
		r_refdef.vrect.x = p[0];
		r_refdef.vrect.y = p[1];
		break;

	case VF_DRAWWORLD:
		r_refdef.flags = (r_refdef.flags&~RDF_NOWORLDMODEL) | (*p?0:RDF_NOWORLDMODEL);
		break;
	case VF_ENGINESBAR:
		csqc_drawsbar = *p;
		break;
	case VF_DRAWCROSSHAIR:
		csqc_addcrosshair = *p;
		break;

	case VF_PERSPECTIVE:
		//useperspective is an extension which we don't suppport
		break;

	default:
		Con_DPrintf("SetViewFlag: %i not recognised\n", parametertype);
		QC_FLOAT(OFS_RETURN) = 0;
		break;
	}
}

static void PF_R_GetViewFlag(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	viewflags parametertype = QC_FLOAT(OFS_PARM0);
	float *r = QC_VECTOR(OFS_RETURN);

	r[0] = 0;
	r[1] = 0;
	r[2] = 0;

	switch(parametertype)
	{
	case VF_FOV:
		r[0] = r_refdef.fov_x;
		r[1] = r_refdef.fov_y;
		break;

	case VF_FOVX:
		*r = r_refdef.fov_x;
		break;

	case VF_FOVY:
		*r = r_refdef.fov_y;
		break;

#pragma message("fixme: AFOV not retrievable")
	case VF_AFOV:
		*r = r_refdef.fov_x;
		break;

	case VF_ORIGIN:
#ifdef CHEAT_PARANOID
		r[0] = r[1] = r[2] = 0;
#else
		VectorCopy(r_refdef.vieworg, r);
#endif
		break;

	case VF_ORIGIN_Z:
	case VF_ORIGIN_X:
	case VF_ORIGIN_Y:
#ifdef CHEAT_PARANOID
		*r = 0;
#else
		*r = r_refdef.vieworg[parametertype-VF_ORIGIN_X];
#endif
		break;

	case VF_ANGLES:
		VectorCopy(r_refdef.viewangles, r);
		break;
	case VF_ANGLES_X:
	case VF_ANGLES_Y:
	case VF_ANGLES_Z:
		*r = r_refdef.viewangles[parametertype-VF_ANGLES_X];
		break;

	case VF_CL_VIEWANGLES_V:
		VectorCopy(cl.viewangles, r);
		break;
	case VF_CL_VIEWANGLES_X:
	case VF_CL_VIEWANGLES_Y:
	case VF_CL_VIEWANGLES_Z:
		*r = cl.viewangles[parametertype-VF_CL_VIEWANGLES_X];
		break;

	case VF_CARTESIAN_ANGLES:
		Con_Printf("WARNING: CARTESIAN ANGLES ARE NOT YET SUPPORTED!\n");
		break;

	case VF_VIEWPORT:
		r[0] = r_refdef.vrect.width;
		r[1] = r_refdef.vrect.height;
		break;

	case VF_SIZE_X:
		*r = r_refdef.vrect.width;
		break;
	case VF_SIZE_Y:
		*r = r_refdef.vrect.height;
		break;
	case VF_SIZE:
		r[0] = r_refdef.vrect.width;
		r[1] = r_refdef.vrect.height;
		break;

	case VF_MIN_X:
		*r = r_refdef.vrect.x;
		break;
	case VF_MIN_Y:
		*r = r_refdef.vrect.y;
		break;
	case VF_MIN:
		r[0] = r_refdef.vrect.x;
		r[1] = r_refdef.vrect.y;
		break;

	case VF_DRAWWORLD:
		*r = !(r_refdef.flags&RDF_NOWORLDMODEL);
		break;
	case VF_ENGINESBAR:
		*r = csqc_drawsbar;
		break;
	case VF_DRAWCROSSHAIR:
		*r = csqc_addcrosshair;
		break;

	case VF_PERSPECTIVE:
		//useperspective is an extension which we don't suppport
		break;

	default:
		Con_DPrintf("GetViewFlag: %i not recognised\n", parametertype);
		break;
	}
}

static void PF_R_RenderScene(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	extern qboolean r_viewchanged;
	if (cl.worldmodel)
		R_PushDlights ();

	VectorCopy (r_refdef.vieworg, cl.viewent.origin);
	CalcGunAngle();

#ifdef GLQUAKE
	R_RenderView();
	GL_Set2D();
#else
	R_ViewChanged(NULL, 0, 1);

//	VID_LockBuffer ();
//	D_DisableBackBufferAccess ();	// of all overlay stuff if drawing directly

	R_RenderView();

//	VID_UnlockBuffer ();
//	D_EnableBackBufferAccess ();	// of all overlay stuff if drawing directly
#endif

	vid.recalc_refdef = 1;

	if (csqc_drawsbar)
	{
		Sbar_Changed();
		Sbar_Draw ();
	}

#ifdef TOFIX
	if (csqc_addcrosshair)
		Draw_Crosshair();
#endif
}

static void PF_cs_getstatf(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int stnum = QC_FLOAT(OFS_PARM0);
	float val = cl.statsfl[stnum];	//copy float into the stat
	QC_FLOAT(OFS_RETURN) = val;
}
static void PF_cs_getstati(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{	//convert an int stat into a qc float.

	int stnum = QC_FLOAT(OFS_PARM0);
	int val = cl.stats[stnum];
	if (*prinst->callargc > 1)
	{
		int first, count;
		first = QC_FLOAT(OFS_PARM1);
		if (*prinst->callargc > 2)
			count = QC_FLOAT(OFS_PARM2);
		else
			count = 1;
		QC_FLOAT(OFS_RETURN) = (((unsigned int)val)&(((1<<count)-1)<<first))>>first;
	}
	else
		QC_FLOAT(OFS_RETURN) = val;
}
static void PF_cs_getstats(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int stnum = QC_FLOAT(OFS_PARM0);

	RETURN_TSTRING(cl.statsstr[stnum]);
}

static void PF_cs_SetOrigin(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (void*)QC_EDICT(prinst, OFS_PARM0);
	float *org = QC_VECTOR(OFS_PARM1);

	VectorCopy(org, ent->v->origin);

	CS_LinkEdict(ent, false);
}

static void PF_cs_SetSize(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (void*)QC_EDICT(prinst, OFS_PARM0);
	float *mins = QC_VECTOR(OFS_PARM1);
	float *maxs = QC_VECTOR(OFS_PARM2);

	VectorCopy(mins, ent->v->mins);
	VectorCopy(maxs, ent->v->maxs);

	CS_LinkEdict(ent, false);
}

static void cs_settracevars(trace_t *tr)
{
	*csqcg.trace_allsolid = tr->allsolid;
	*csqcg.trace_startsolid = tr->startsolid;
	*csqcg.trace_fraction = tr->fraction;
	*csqcg.trace_inwater = tr->inwater;
	*csqcg.trace_inopen = tr->inopen;
	VectorCopy (tr->endpos, csqcg.trace_endpos);
	VectorCopy (tr->plane.normal, csqcg.trace_plane_normal);
	*csqcg.trace_plane_dist =  tr->plane.dist;

	if (tr->ent)
		*csqcg.trace_ent = csqcprogs->EdictToProgs(csqcprogs, (void*)tr->ent);
	else
		*csqcg.trace_ent = csqcprogs->EdictToProgs(csqcprogs, (void*)csqc_edicts);
}

static void PF_cs_traceline(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*v1, *v2, *mins, *maxs;
	trace_t	trace;
	int		nomonsters;
	csqcedict_t	*ent;
	int savedhull;

	v1 = QC_VECTOR(OFS_PARM0);
	v2 = QC_VECTOR(OFS_PARM1);
	nomonsters = QC_FLOAT(OFS_PARM2);
	ent = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM3);

//	if (*prinst->callargc == 6)
//	{
//		mins = QC_VECTOR(OFS_PARM4);
//		maxs = QC_VECTOR(OFS_PARM5);
//	}
//	else
	{
		mins = vec3_origin;
		maxs = vec3_origin;
	}

	savedhull = ent->v->hull;
	ent->v->hull = 0;
	trace = CS_Move (v1, mins, maxs, v2, nomonsters, ent);
	ent->v->hull = savedhull;

	cs_settracevars(&trace);
}
static void PF_cs_tracebox(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*v1, *v2, *mins, *maxs;
	trace_t	trace;
	int		nomonsters;
	csqcedict_t	*ent;
	int savedhull;

	v1 = QC_VECTOR(OFS_PARM0);
	mins = QC_VECTOR(OFS_PARM1);
	maxs = QC_VECTOR(OFS_PARM2);
	v2 = QC_VECTOR(OFS_PARM3);
	nomonsters = QC_FLOAT(OFS_PARM4);
	ent = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM5);

	savedhull = ent->v->hull;
	ent->v->hull = 0;
	trace = CS_Move (v1, mins, maxs, v2, nomonsters, ent);
	ent->v->hull = savedhull;

	cs_settracevars(&trace);
}

static trace_t CS_Trace_Toss (csqcedict_t *tossent, csqcedict_t *ignore)
{
	int i;
	int savedhull;
	float gravity;
	vec3_t move, end;
	trace_t trace;
	cvar_t *v;
//	float maxvel = Cvar_Get("sv_maxvelocity", "2000", 0, "CSQC physics")->value;

	vec3_t origin, velocity;

	// this has to fetch the field from the original edict, since our copy is truncated
	gravity = 1;//tossent->v->gravity;
	if (!gravity)
		gravity = 1.0;
	v = Cvar_FindVar("sv_gravity");
	if (v)
		gravity *= v->value * 0.05;

	VectorCopy (tossent->v->origin, origin);
	VectorCopy (tossent->v->velocity, velocity);

	CS_CheckVelocity (tossent);

	savedhull = tossent->v->hull;
	tossent->v->hull = 0;
	for (i = 0;i < 200;i++) // LordHavoc: sanity check; never trace more than 10 seconds
	{
		velocity[2] -= gravity;
		VectorScale (velocity, 0.05, move);
		VectorAdd (origin, move, end);
		trace = CS_Move (origin, tossent->v->mins, tossent->v->maxs, end, MOVE_NORMAL, tossent);
		VectorCopy (trace.endpos, origin);

		CS_CheckVelocity (tossent);

		if (trace.fraction < 1 && trace.ent && (void*)trace.ent != ignore)
			break;
	}
	tossent->v->hull = savedhull;

	trace.fraction = 0; // not relevant
	return trace;
}
static void PF_cs_tracetoss (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	trace_t	trace;
	csqcedict_t	*ent;
	csqcedict_t	*ignore;

	ent = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM0);
	if (ent == csqc_edicts)
		Con_DPrintf("tracetoss: can not use world entity\n");
	ignore = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM1);

	trace = CS_Trace_Toss (ent, ignore);

	cs_settracevars(&trace);
}

static int CS_PointContents(vec3_t org)
{
	int cont;
	if (!cl.worldmodel)
		cont = CONTENTS_EMPTY;
	else
		cont = SV_HullPointContents (&cl.worldmodel->hulls[0], 0, org);
	if (cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN)
		cont = CONTENTS_WATER;

	return cont;
}
static void PF_cs_pointcontents(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*v;
	int cont;

	v = QC_VECTOR(OFS_PARM0);

	QC_FLOAT(OFS_RETURN) = CS_PointContents(v);
}

static int FindModel(char *name, int *free)
{
	int i;

	*free = 0;

	if (!name || !*name)
		return 0;

	for (i = 1; i < MAX_CSQCMODELS; i++)
	{
		if (!*cl.model_csqcname[i])
		{
			*free = -i;
			break;
		}
		if (!strcmp(cl.model_csqcname[i], name))
			return -i;
	}
	for (i = 1; i < MAX_MODELS; i++)
	{
		if (!cl.model_precache[i])
			break;

		if (!strcmp(cl.model_precache[i]->name, name))
			return i;
	}
	return 0;
}

static void csqc_setmodel(progfuncs_t *prinst, csqcedict_t *ent, int modelindex)
{
	model_t *model;

	ent->v->modelindex = modelindex;
	if (modelindex < 0)
	{
		if (modelindex <= -MAX_MODELS)
			return;
		ent->v->model = prinst->StringToProgs(prinst, cl.model_csqcname[-modelindex]);
		if (!cl.model_csqcprecache[-modelindex])
			cl.model_csqcprecache[-modelindex] = Mod_ForName(cl.model_csqcname[-modelindex], false);
		model = cl.model_csqcprecache[-modelindex];
	}
	else
	{
		if (modelindex >= MAX_MODELS)
			return;
		ent->v->model = prinst->StringToProgs(prinst, cl.model_precache[modelindex]->name);
		model = cl.model_precache[modelindex];
	}
	if (model)
	{
		VectorCopy(model->mins, ent->v->mins);
		VectorCopy(model->maxs, ent->v->maxs);
	}
	else
	{
		ent->v->mins[0] = ent->v->mins[1] = ent->v->mins[2] =
		ent->v->maxs[0] = ent->v->maxs[1] = ent->v->maxs[2] = 0;
	}

	CS_LinkEdict(ent, false);
}

static void PF_cs_SetModel(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (void*)QC_EDICT(prinst, OFS_PARM0);
	char *modelname = QC_GetStringOfs(prinst, OFS_PARM1);
	int freei;
	int modelindex = FindModel(modelname, &freei);

	if (!modelindex && modelname && *modelname)
	{
		if (!freei)
			Host_EndGame("CSQC ran out of model slots\n");
		Con_DPrintf("Late caching model \"%s\"\n", modelname);
		Q_strncpyz(cl.model_csqcname[-freei], modelname, sizeof(cl.model_csqcname[-freei]));	//allocate a slot now
		modelindex = freei;

		cl.model_csqcprecache[-freei] = NULL;
	}

	csqc_setmodel(prinst, ent, modelindex);
}
static void PF_cs_SetModelIndex(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (void*)QC_EDICT(prinst, OFS_PARM0);
	int modelindex = QC_FLOAT(OFS_PARM1);

	csqc_setmodel(prinst, ent, modelindex);
}
static void PF_cs_PrecacheModel(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int modelindex, freei;
	char *modelname = QC_GetStringOfs(prinst, OFS_PARM0);
	int i;

	for (i = 1; i < MAX_MODELS; i++)	//Make sure that the server specified model is loaded..
	{
		if (!cl.model_precache[i])
			break;
		if (!strcmp(cl.model_precache[i]->name, modelname))
		{
			break;
		}
	}

	modelindex = FindModel(modelname, &freei);	//now load it

	if (!modelindex)
	{
		if (!freei)
			Host_EndGame("CSQC ran out of model slots\n");
		Q_strncpyz(cl.model_csqcname[-freei], modelname, sizeof(cl.model_csqcname[-freei]));	//allocate a slot now
		modelindex = freei;

		cl.model_csqcprecache[-freei] = NULL;
	}

	QC_FLOAT(OFS_RETURN) = modelindex;
}
static void PF_cs_PrecacheSound(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *soundname = QC_GetStringOfs(prinst, OFS_PARM0);
	S_PrecacheSound(soundname);
}

static void PF_cs_ModelnameForIndex(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int modelindex = QC_FLOAT(OFS_PARM0);

	if (modelindex < 0)
		QC_INT(OFS_RETURN) = (int)prinst->StringToProgs(prinst, cl.model_csqcname[-modelindex]);
	else if (cl.model_precache[modelindex])
		QC_INT(OFS_RETURN) = (int)prinst->StringToProgs(prinst, cl.model_precache[modelindex]->name);
	else
		QC_INT(OFS_RETURN) = 0;
}

static void PF_ReadByte(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (csqc_fakereadbyte != -1)
	{
		QC_FLOAT(OFS_RETURN) = csqc_fakereadbyte;
		csqc_fakereadbyte = -1;
	}
	else
	{
		QC_FLOAT(OFS_RETURN) = MSG_ReadByte();
	}
}

static void PF_ReadChar(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = MSG_ReadChar();
}

static void PF_ReadShort(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = MSG_ReadShort();
}

static void PF_ReadEntityNum(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	unsigned short val;
	val = MSG_ReadShort();
	if (val & 0x8000)
	{	//our protocol only supports 15bits of revelent entity number (16th bit is used as 'remove').
		//so warn with badly coded mods.
		Con_Printf("ReadEntityNumber read bad entity number\n");
		QC_FLOAT(OFS_RETURN)	= 0;
	}
	else
		QC_FLOAT(OFS_RETURN) = val;
}

static void PF_ReadLong(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = MSG_ReadLong();
}

static void PF_ReadCoord(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = MSG_ReadCoord();
}

static void PF_ReadFloat(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = MSG_ReadFloat();
}

static void PF_ReadString(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *read = MSG_ReadString();

	RETURN_TSTRING(read);
}

static void PF_ReadAngle(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = MSG_ReadAngle();
}


static void PF_objerror (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*s;
	struct edict_s	*ed;

	s = prinst->VarString(prinst, 0);
/*	Con_Printf ("======OBJECT ERROR in %s:\n%s\n", PR_GetString(pr_xfunction->s_name),s);
*/	ed = prinst->ProgsToEdict(prinst, *csqcg.self);
/*	ED_Print (ed);
*/
	prinst->PrintEdict(prinst, ed);
	Con_Printf("%s", s);

	if (developer.value)
		(*prinst->pr_trace) = 2;
	else
	{
		prinst->EntFree (prinst, ed);

		prinst->AbortStack(prinst);

		PR_BIError (prinst, "Program error: %s", s);
	}
}

static void PF_cs_setsensativityscaler (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	cl.sensitivityscale = QC_FLOAT(OFS_PARM0);
}

static void PF_cs_pointparticles (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int effectnum = QC_FLOAT(OFS_PARM0)-1;
	float *pos = QC_VECTOR(OFS_PARM1);
	float *vel = QC_VECTOR(OFS_PARM2);
	float count = QC_FLOAT(OFS_PARM3);

	if (*prinst->callargc < 3)
		vel = vec3_origin;
	if (*prinst->callargc < 4)
		count = 1;

	if (!effectnum)
		return;

	switch(effectnum)
	{
	case TE_SPIKE:
		R_RunParticleEffect (pos, vec3_origin, 0, 10);
		break;
	case TE_SUPERSPIKE:
		R_RunParticleEffect (pos, vec3_origin, 0, 20);
		break;
	case TE_GUNSHOT:
		R_RunParticleEffect (pos, vec3_origin, 0, 20);
		break;
	case TE_EXPLOSION:
		R_ParticleExplosion (pos);
		break;
	case TE_TAREXPLOSION:
		R_BlobExplosion (pos);
		break;
	case TE_WIZSPIKE:
		R_RunParticleEffect (pos, vec3_origin, 20, 30);
		break;
	case TE_KNIGHTSPIKE:
		R_RunParticleEffect (pos, vec3_origin, 226, 20);
		break;
	case TE_LAVASPLASH:
		R_LavaSplash (pos);
		break;
	case TE_TELEPORT:
		R_TeleportSplash (pos);
		break;
	}
}

static void PF_cs_trailparticles (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int efnum = QC_FLOAT(OFS_PARM0)-1;
	csqcedict_t *ent = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM1);
	float *start = QC_VECTOR(OFS_PARM2);
	float *end = QC_VECTOR(OFS_PARM3);

	if (!efnum)
		return;

	R_RocketTrail(start, end, efnum-100);
}

struct {
	char *name;
	int value;
} particleeffectnames[] =
{
	//warning: mods should NEVER hardcode these values
	{"te_spike", TE_SPIKE},
	{"te_superspike", TE_SUPERSPIKE},
	{"te_gunshot", TE_GUNSHOT},
	{"te_explosion", TE_EXPLOSION},
	{"te_tarexplosion", TE_TAREXPLOSION},
//	{"", TE_LIGHTNING1},
//	{"", TE_LIGHTNING2},
	{"te_wizspike", TE_WIZSPIKE},
	{"te_knightspike", TE_KNIGHTSPIKE},
//	{"", TE_LIGHTNING3},
	{"te_lavasplash", TE_LAVASPLASH},
	{"te_teleport", TE_TELEPORT},

	{"tr_rocket", 100+0},
	{"tr_smoke", 100+1},
	{"tr_blood", 100+2},
	{"tr_tracer1", 100+3},
	{"tr_tracer2", 100+5},
	{"tr_slightblood", 100+4},
	{"tr_vore", 100+6},

	{NULL}
};
static void PF_cs_particleeffectnum (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	char *effectname = QC_GetStringOfs(prinst, OFS_PARM0);

	QC_FLOAT(OFS_RETURN) = 0;

	for (i = 0; particleeffectnames[i].name; i++)
	{
		if (!strcmp(effectname, particleeffectnames[i].name))
		{
			QC_FLOAT(OFS_RETURN) = particleeffectnames[i].value+1;
			break;
		}
	}
}

static void PF_cs_sendevent (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent;
	int i;
	char *eventname = QC_GetStringOfs(prinst, OFS_PARM0);
	char *argtypes = QC_GetStringOfs(prinst, OFS_PARM1);

	MSG_WriteByte(&cls.message, clc_qcrequest);
	for (i = 0; i < 6; i++)
	{
		if (argtypes[i] == 's')
		{
			MSG_WriteByte(&cls.message, ev_string);
			MSG_WriteString(&cls.message, QC_GetStringOfs(prinst, OFS_PARM2+i*3));
		}
		else if (argtypes[i] == 'f')
		{
			MSG_WriteByte(&cls.message, ev_float);
			MSG_WriteFloat(&cls.message, QC_FLOAT(OFS_PARM2+i*3));
		}
		else if (argtypes[i] == 'i')
		{
			MSG_WriteByte(&cls.message, /*ev_integer*/8);
			MSG_WriteFloat(&cls.message, QC_FLOAT(OFS_PARM2+i*3));
		}
		else if (argtypes[i] == 'v')
		{
			MSG_WriteByte(&cls.message, ev_vector);
			MSG_WriteFloat(&cls.message, QC_FLOAT(OFS_PARM2+i*3+0));
			MSG_WriteFloat(&cls.message, QC_FLOAT(OFS_PARM2+i*3+1));
			MSG_WriteFloat(&cls.message, QC_FLOAT(OFS_PARM2+i*3+2));
		}
		else if (argtypes[i] == 'e')
		{
			ent = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM2+i*3);
			MSG_WriteByte(&cls.message, ev_entity);
			MSG_WriteShort(&cls.message, ent->v->entnum);
		}
		else
			break;
	}
	MSG_WriteByte(&cls.message, 0);
	MSG_WriteString(&cls.message, eventname);
}

static void cs_set_input_state (usercmdextra_t *cmd)
{
	if (csqcg.input_timelength)
		*csqcg.input_timelength = cmd->msec/1000.0f;
	if (csqcg.input_angles)
	{
		csqcg.input_angles[0] = cmd->std->viewangles[0];
		csqcg.input_angles[1] = cmd->std->viewangles[1];
		csqcg.input_angles[2] = cmd->std->viewangles[2];
	}
	if (csqcg.input_movevalues)
	{
		csqcg.input_movevalues[0] = cmd->std->forwardmove;
		csqcg.input_movevalues[1] = cmd->std->sidemove;
		csqcg.input_movevalues[2] = cmd->std->upmove;
	}
	if (csqcg.input_buttons)
		*csqcg.input_buttons = cmd->buttons;

	if (csqcg.input_impulse)
		*csqcg.input_impulse = cmd->impulse;
#ifdef QUAKE2
	if (csqcg.input_lightlevel)
		*csqcg.input_lightlevel = cmd->lightlevel;
#endif
	if (csqcg.input_servertime)
		*csqcg.input_servertime = cmd->servertime/1000.0f;
	if (csqcg.input_clienttime)
		*csqcg.input_clienttime = cmd->fclienttime/1000.0f;
}

static void cs_get_input_state (usercmdextra_t *cmd)
{
	if (csqcg.input_timelength)
		cmd->msec = *csqcg.input_timelength*1000;
	if (csqcg.input_angles)
	{
		cmd->std->viewangles[0] = csqcg.input_angles[0];
		cmd->std->viewangles[1] = csqcg.input_angles[1];
		cmd->std->viewangles[2] = csqcg.input_angles[2];
	}
	if (csqcg.input_movevalues)
	{
		cmd->std->forwardmove = csqcg.input_movevalues[0];
		cmd->std->sidemove = csqcg.input_movevalues[1];
		cmd->std->upmove = csqcg.input_movevalues[2];
	}
	if (csqcg.input_buttons)
		cmd->buttons = *csqcg.input_buttons;

	if (csqcg.input_impulse)
		cmd->impulse = *csqcg.input_impulse;
#ifdef QUAKE2
	if (csqcg.input_lightlevel)
		cmd->lightlevel = *csqcg.input_lightlevel;
#endif
	if (csqcg.input_servertime)
		cmd->servertime = *csqcg.input_servertime*1000;
}

//get the input commands, and stuff them into some globals.
static void PF_cs_getinputstate (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int f;
	usercmdextra_t *cmd;

	f = QC_FLOAT(OFS_PARM0);
	if (f >= cls.inputlog_sequenceout)
	{
		QC_FLOAT(OFS_RETURN) = false;
		return;
	}
	if (f < cls.inputlog_sequenceout - INPUTLOG_MASK || f < 0)
	{
		QC_FLOAT(OFS_RETURN) = false;
		return;
	}

	// save this command off for prediction
	cmd = &cls.inputlog_cmd[f&INPUTLOG_MASK];

	cs_set_input_state(cmd);

	QC_FLOAT(OFS_RETURN) = true;
}

//read lots of globals, run the default player physics, write lots of globals.
//not intended to affect client state at all
static void PF_cs_runplayerphysics (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent;

	ent = (void*)QC_EDICT(prinst, OFS_PARM0);

	//there is not normally any default prediction in NQ
	//this builtin can thus do nothing
}

static void PF_cs_getentitytoken (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (!csqcmapentitydata)
	{
		//nothing more to parse
		QC_INT(OFS_RETURN) = 0;
	}
	else
	{
		csqcmapentitydata = COM_Parse(csqcmapentitydata);
		RETURN_TSTRING(com_token);
	}
}

static void PF_cs_serverkey (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *keyname = prinst->VarString(prinst, 0);
	char *ret;

	if (!strcmp(keyname, "ip"))
	{
		if (cls.netcon)
			ret = cls.netcon->address;
		else
			ret = "";
	}
	else if (!strcmp(keyname, "protocol"))
	{	//using this is pretty acedemic, really. Not particuarly portable.
		//a tokenizable string
		//first is the base game qw/nq
		//second is branch (custom engine name)
		//third is protocol version.
		ret = "NetQuake Std 15";
	}
#ifdef TOFIX
	else if (!strcmp(keyname, "coop"))
	else if (!strcmp(keyname, "deathmatch"))
#endif
	else
	{
		ret = "";//
		//Info_ValueForKey(cl.serverinfo, keyname);
	}

	if (*ret)
		RETURN_TSTRING(ret);
	else
		QC_INT(OFS_RETURN) = 0;
}

//string(float pnum, string keyname)
static void PF_cs_getplayerkey (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char buffer[64];
	char *ret;
	int pnum = QC_FLOAT(OFS_PARM0);
	char *keyname = QC_GetStringOfs(prinst, OFS_PARM1);
	if (pnum < 0)
	{
#ifdef TOFIX
		if (csqc_resortfrags)
		{
			Sbar_SortFrags(false);
			csqc_resortfrags = false;
		}
		if (pnum >= -scoreboardlines)
		{//sort by
			pnum = fragsort[-(pnum+1)];
		}
#endif
	}

	if (pnum < 0 || pnum >= cl.maxclients)
		ret = "";
	else if (!*cl.scores[pnum].name)
		ret = "";	//player isn't on the server.
	else if (!strcmp(keyname, "ping"))
	{
		ret = "";	//finding out other player's pings can be awkward.
		//FIXME
	}
	else if (!strcmp(keyname, "frags"))
	{
		ret = buffer;
		sprintf(ret, "%i", cl.scores[pnum].frags);
	}
	else if (!strcmp(keyname, "pl"))	//packet loss
	{
		ret = "";	//packetloss is not known in netquake
	}
	else if (!strcmp(keyname, "entertime"))	//packet loss
	{
		ret = buffer;
		sprintf(ret, "%i", (int)cl.scores[pnum].entertime);
	}
	else
	{
		ret = "";//Info_ValueForKey(cl.players[pnum].userinfo, keyname);
	}
	if (*ret)
		RETURN_TSTRING(ret);
	else
		QC_INT(OFS_RETURN) = 0;
}

static void PF_cs_sound(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char		*sample;
	int			channel;
	csqcedict_t		*entity;
	float volume;
	float attenuation;

	sfx_t *sfx;

	entity = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM0);
	channel = QC_FLOAT(OFS_PARM1);
	sample = QC_GetStringOfs(prinst, OFS_PARM2);
	volume = QC_FLOAT(OFS_PARM3);
	attenuation = QC_FLOAT(OFS_PARM4);

	sfx = S_PrecacheSound(sample);
	if (sfx)
		S_StartSound(-entity->entnum, channel, sfx, entity->v->origin, volume, attenuation);
};

void PF_cs_pointsound(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *sample;
	float *origin;
	float volume;
	float attenuation;

	sfx_t *sfx;

	origin = QC_VECTOR(OFS_PARM0);
	sample = QC_GetStringOfs(prinst, OFS_PARM1);
	volume = QC_FLOAT(OFS_PARM2);
	attenuation = QC_FLOAT(OFS_PARM3);

	sfx = S_PrecacheSound(sample);
	if (sfx)
		S_StartSound(0, 0, sfx, origin, volume, attenuation);
}

static void PF_cs_particle(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *org = QC_VECTOR(OFS_PARM0);
	float *dir = QC_VECTOR(OFS_PARM1);
	float colour = QC_FLOAT(OFS_PARM2);
	float count = QC_FLOAT(OFS_PARM2);

	R_RunParticleEffect(org, dir, colour, count);
}

void PF_cl_ambientsound(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char		*samp;
	float		*pos;
	float 		vol, attenuation;

	pos = QC_VECTOR (OFS_PARM0);
	samp = QC_GetStringOfs(prinst, OFS_PARM1);
	vol = QC_FLOAT(OFS_PARM2);
	attenuation = QC_FLOAT(OFS_PARM3);

	S_StaticSound (S_PrecacheSound (samp), pos, vol, attenuation);
}

/*
static void PF_cs_vectorvectors (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	VectorCopy(QC_VECTOR(OFS_PARM0), csqcg.forward);
	VectorNormalize(csqcg.forward);
	VectorVectors(csqcg.forward, csqcg.right, csqcg.up);
}
*/
#define PF_cs_vectorvectors PF_Fixme

static void PF_cs_lightstyle (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int stnum = QC_FLOAT(OFS_PARM0);
	char *str = QC_GetStringOfs(prinst, OFS_PARM1);

	if ((unsigned)stnum >= MAX_LIGHTSTYLES)
	{
		Con_Printf ("PF_cs_lightstyle: stnum > MAX_LIGHTSTYLES");
		return;
	}
	Q_strncpyz (cl_lightstyle[stnum].map,  str, sizeof(cl_lightstyle[stnum].map));
	cl_lightstyle[stnum].length = Q_strlen(cl_lightstyle[stnum].map);
}

static void PF_cs_changeyaw (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t		*ent;
	float		ideal, current, move, speed;

	ent = (void*)prinst->ProgsToEdict(prinst, *csqcg.self);
	current = anglemod( ent->v->angles[1] );
	ideal = ent->v->ideal_yaw;
	speed = ent->v->yaw_speed;

	if (current == ideal)
		return;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	ent->v->angles[1] = anglemod (current + move);
}
static void PF_cs_changepitch (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t		*ent;
	float		ideal, current, move, speed;

	ent = (void*)prinst->ProgsToEdict(prinst, *csqcg.self);
	current = anglemod( ent->v->angles[0] );
	ideal = ent->v->ideal_pitch;
	speed = ent->v->pitch_speed;

	if (current == ideal)
		return;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	ent->v->angles[0] = anglemod (current + move);
}

static void PF_cs_findradius (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t	*ent, *chain;
	float	rad;
	float	*org;
	vec3_t	eorg;
	int		i, j;

	chain = (csqcedict_t *)*prinst->parms->sv_edicts;

	org = QC_VECTOR(OFS_PARM0);
	rad = QC_FLOAT(OFS_PARM1);

	for (i=1 ; i<*prinst->parms->sv_num_edicts ; i++)
	{
		ent = (void*)prinst->EdictNum(prinst, i);
		if (ent->isfree)
			continue;
//		if (ent->v->solid == SOLID_NOT && !sv_gameplayfix_blowupfallenzombies.value)
//			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (ent->v->origin[j] + (ent->v->mins[j] + ent->v->maxs[j])*0.5);
		if (Length(eorg) > rad)
			continue;

		ent->v->chain = prinst->EdictToProgs(prinst, (void*)chain);
		chain = ent;
	}

	QCRETURN_EDICT(prinst, chain);
}

//entity(string field, float match) findchainflags = #450
//chained search for float, int, and entity reference fields
void PF_cs_findchainflags (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, f;
	int s;
	csqcedict_t	*ent, *chain;

	chain = (csqcedict_t *) *prinst->parms->sv_edicts;

	f = QC_INT(OFS_PARM0)+prinst->fieldadjust;
	s = QC_FLOAT(OFS_PARM1);

	for (i = 1; i < *prinst->parms->sv_num_edicts; i++)
	{
		ent = (csqcedict_t*)prinst->EdictNum(prinst, i);
		if (ent->isfree)
			continue;
		if (!((int)((float *)ent->v)[f] & s))
			continue;

		ent->v->chain = prinst->EdictToProgs(prinst, (edict_t*)chain);
		chain = ent;
	}

	QCRETURN_EDICT(prinst, chain);
}

//entity(string field, float match) findchainfloat = #403
void PF_cs_findchainfloat (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, f;
	float s;
	csqcedict_t	*ent, *chain;

	chain = (csqcedict_t *) *prinst->parms->sv_edicts;

	f = QC_INT(OFS_PARM0)+prinst->fieldadjust;
	s = QC_FLOAT(OFS_PARM1);

	for (i = 1; i < *prinst->parms->sv_num_edicts; i++)
	{
		ent = (csqcedict_t*)prinst->EdictNum(prinst, i);
		if (ent->isfree)
			continue;
		if (((float *)ent->v)[f] != s)
			continue;

		ent->v->chain = prinst->EdictToProgs(prinst, (edict_t*)chain);
		chain = ent;
	}

	QCRETURN_EDICT(prinst, chain);
}


//entity(string field, string match) findchain = #402
//chained search for strings in entity fields
void PF_cs_findchain (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i, f;
	char *s;
	string_t t;
	csqcedict_t *ent, *chain;

	chain = (csqcedict_t *) *prinst->parms->sv_edicts;

	f = QC_INT(OFS_PARM0)+prinst->fieldadjust;
	s = QC_GetStringOfs(prinst, OFS_PARM1);

	for (i = 1; i < *prinst->parms->sv_num_edicts; i++)
	{
		ent = (csqcedict_t*)prinst->EdictNum(prinst, i);
		if (ent->isfree)
			continue;
		t = *(string_t *)&((float*)ent->v)[f];
		if (!t)
			continue;
		if (strcmp(prinst->StringToNative(prinst, t), s))
			continue;

		ent->v->chain = prinst->EdictToProgs(prinst, (edict_t*)chain);
		chain = ent;
	}

	QCRETURN_EDICT(prinst, chain);
}

/*
static void PF_cl_te_gunshot (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	float scaler = 1;
	if (*prinst->callargc >= 2)	//fte is a quakeworld engine
		scaler = QC_FLOAT(OFS_PARM1);
	if (P_RunParticleEffectType(pos, NULL, scaler, pt_gunshot))
		P_RunParticleEffect (pos, vec3_origin, 0, 20*scaler);
}
static void PF_cl_te_bloodqw (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	float scaler = 1;
	if (*prinst->callargc >= 2)	//fte is a quakeworld engine
		scaler = QC_FLOAT(OFS_PARM1);
	if (P_RunParticleEffectType(pos, NULL, scaler, ptqw_blood))
		if (P_RunParticleEffectType(pos, NULL, scaler, ptdp_blood))
			P_RunParticleEffect (pos, vec3_origin, 73, 20*scaler);
}
static void PF_cl_te_blooddp (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	float *dir = QC_VECTOR(OFS_PARM1);
	float scaler = QC_FLOAT(OFS_PARM2);

	if (P_RunParticleEffectType(pos, dir, scaler, ptdp_blood))
		if (P_RunParticleEffectType(pos, dir, scaler, ptqw_blood))
			P_RunParticleEffect (pos, dir, 73, 20*scaler);
}
static void PF_cl_te_lightningblood (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	if (P_RunParticleEffectType(pos, NULL, 1, ptqw_lightningblood))
		P_RunParticleEffect (pos, vec3_origin, 225, 50);
}
static void PF_cl_te_spike (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	if (P_RunParticleEffectType(pos, NULL, 1, pt_spike))
		if (P_RunParticleEffectType(pos, NULL, 10, pt_gunshot))
			P_RunParticleEffect (pos, vec3_origin, 0, 10);
}
static void PF_cl_te_superspike (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	if (P_RunParticleEffectType(pos, NULL, 1, pt_superspike))
		if (P_RunParticleEffectType(pos, NULL, 2, pt_spike))
			if (P_RunParticleEffectType(pos, NULL, 20, pt_gunshot))
				P_RunParticleEffect (pos, vec3_origin, 0, 20);
}
static void PF_cl_te_explosion (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);

	// light
	if (r_explosionlight.value) {
		dlight_t *dl;

		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 150 + r_explosionlight.value*200;
		dl->die = cl.time + 1;
		dl->decay = 300;

		dl->color[0] = 0.2;
		dl->color[1] = 0.155;
		dl->color[2] = 0.05;
		dl->channelfade[0] = 0.196;
		dl->channelfade[1] = 0.23;
		dl->channelfade[2] = 0.12;
	}

	if (P_RunParticleEffectType(pos, NULL, 1, pt_explosion))
		P_RunParticleEffect(pos, NULL, 107, 1024); // should be 97-111

	R_AddStain(pos, -1, -1, -1, 100);

	S_StartSound (-2, 0, cl_sfx_r_exp3, pos, 1, 1);
}
static void PF_cl_te_tarexplosion (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	P_RunParticleEffectType(pos, NULL, 1, pt_tarexplosion);

	S_StartSound (-2, 0, cl_sfx_r_exp3, pos, 1, 1);
}
static void PF_cl_te_wizspike (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	if (P_RunParticleEffectType(pos, NULL, 1, pt_wizspike))
		P_RunParticleEffect (pos, vec3_origin, 20, 30);

	S_StartSound (-2, 0, cl_sfx_knighthit, pos, 1, 1);
}
static void PF_cl_te_knightspike (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	if (P_RunParticleEffectType(pos, NULL, 1, pt_knightspike))
		P_RunParticleEffect (pos, vec3_origin, 226, 20);

	S_StartSound (-2, 0, cl_sfx_knighthit, pos, 1, 1);
}
static void PF_cl_te_lavasplash (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	P_RunParticleEffectType(pos, NULL, 1, pt_lavasplash);
}
static void PF_cl_te_teleport (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	P_RunParticleEffectType(pos, NULL, 1, pt_teleportsplash);
}
static void PF_cl_te_gunshotquad (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	if (P_RunParticleEffectTypeString(pos, vec3_origin, 1, "te_gunshotquad"))
		if (P_RunParticleEffectType(pos, NULL, 1, pt_gunshot))
			P_RunParticleEffect (pos, vec3_origin, 0, 20);
}
static void PF_cl_te_spikequad (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	if (P_RunParticleEffectTypeString(pos, vec3_origin, 1, "te_spikequad"))
		if (P_RunParticleEffectType(pos, NULL, 1, pt_spike))
			if (P_RunParticleEffectType(pos, NULL, 10, pt_gunshot))
				P_RunParticleEffect (pos, vec3_origin, 0, 10);
}
static void PF_cl_te_superspikequad (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	if (P_RunParticleEffectTypeString(pos, vec3_origin, 1, "te_superspikequad"))
		if (P_RunParticleEffectType(pos, NULL, 1, pt_superspike))
			if (P_RunParticleEffectType(pos, NULL, 2, pt_spike))
				if (P_RunParticleEffectType(pos, NULL, 20, pt_gunshot))
					P_RunParticleEffect (pos, vec3_origin, 0, 20);
}
static void PF_cl_te_explosionquad (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	if (P_RunParticleEffectTypeString(pos, vec3_origin, 1, "te_explosionquad"))
		if (P_RunParticleEffectType(pos, NULL, 1, pt_explosion))
			P_RunParticleEffect(pos, NULL, 107, 1024); // should be 97-111

	R_AddStain(pos, -1, -1, -1, 100);

	// light
	if (r_explosionlight.value) {
		dlight_t *dl;

		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 150 + r_explosionlight.value*200;
		dl->die = cl.time + 1;
		dl->decay = 300;

		dl->color[0] = 0.2;
		dl->color[1] = 0.155;
		dl->color[2] = 0.05;
		dl->channelfade[0] = 0.196;
		dl->channelfade[1] = 0.23;
		dl->channelfade[2] = 0.12;
	}

	S_StartSound (-2, 0, cl_sfx_r_exp3, pos, 1, 1);
}

//void(vector org, float radius, float lifetime, vector color) te_customflash
static void PF_cl_te_customflash (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *org = QC_VECTOR(OFS_PARM0);
	float radius = QC_FLOAT(OFS_PARM1);
	float lifetime = QC_FLOAT(OFS_PARM2);
	float *colour = QC_VECTOR(OFS_PARM3);

	dlight_t *dl;
	// light
	dl = CL_AllocDlight (0);
	VectorCopy (org, dl->origin);
	dl->radius = radius;
	dl->die = cl.time + lifetime;
	dl->decay = dl->radius / lifetime;
	dl->color[0] = colour[0]*0.5f;
	dl->color[1] = colour[1]*0.5f;
	dl->color[2] = colour[2]*0.5f;
}

static void PF_cl_te_bloodshower (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
}
static void PF_cl_te_particlecube (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *minb = QC_VECTOR(OFS_PARM0);
	float *maxb = QC_VECTOR(OFS_PARM1);
	float *vel = QC_VECTOR(OFS_PARM2);
	float howmany = QC_FLOAT(OFS_PARM3);
	float color = QC_FLOAT(OFS_PARM4);
	float gravity = QC_FLOAT(OFS_PARM5);
	float jitter = QC_FLOAT(OFS_PARM6);

	P_RunParticleCube(minb, maxb, vel, howmany, color, gravity, jitter);
}
static void PF_cl_te_spark (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
}
static void PF_cl_te_smallflash (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
}
static void PF_cl_te_explosion2 (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
}
static void PF_cl_te_lightning1 (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM0);
	float *start = QC_VECTOR(OFS_PARM1);
	float *end = QC_VECTOR(OFS_PARM1);

	CL_AddBeam(0, ent->entnum+MAX_EDICTS, start, end);
}
static void PF_cl_te_lightning2 (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM0);
	float *start = QC_VECTOR(OFS_PARM1);
	float *end = QC_VECTOR(OFS_PARM1);

	CL_AddBeam(1, ent->entnum+MAX_EDICTS, start, end);
}
static void PF_cl_te_lightning3 (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM0);
	float *start = QC_VECTOR(OFS_PARM1);
	float *end = QC_VECTOR(OFS_PARM1);

	CL_AddBeam(2, ent->entnum+MAX_EDICTS, start, end);
}
static void PF_cl_te_beam (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM0);
	float *start = QC_VECTOR(OFS_PARM1);
	float *end = QC_VECTOR(OFS_PARM1);

	CL_AddBeam(5, ent->entnum+MAX_EDICTS, start, end);
}
static void PF_cl_te_plasmaburn (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
}
static void PF_cl_te_explosionrgb (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *org = QC_VECTOR(OFS_PARM0);
	float *colour = QC_VECTOR(OFS_PARM1);

	dlight_t *dl;

	if (P_RunParticleEffectType(org, NULL, 1, pt_explosion))
		P_RunParticleEffect(org, NULL, 107, 1024); // should be 97-111

	R_AddStain(org, -1, -1, -1, 100);

	// light
	if (r_explosionlight.value)
	{
		dl = CL_AllocDlight (0);
		VectorCopy (org, dl->origin);
		dl->radius = 150 + r_explosionlight.value*200;
		dl->die = cl.time + 0.5;
		dl->decay = 300;

		dl->color[0] = 0.4f*colour[0];
		dl->color[1] = 0.4f*colour[1];
		dl->color[2] = 0.4f*colour[2];
		dl->channelfade[0] = 0;
		dl->channelfade[1] = 0;
		dl->channelfade[2] = 0;
	}

	S_StartSound (-2, 0, cl_sfx_r_exp3, org, 1, 1);
}
static void PF_cl_te_particlerain (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *min = QC_VECTOR(OFS_PARM0);
	float *max = QC_VECTOR(OFS_PARM1);
	float *vel = QC_VECTOR(OFS_PARM2);
	float howmany = QC_FLOAT(OFS_PARM3);
	float colour = QC_FLOAT(OFS_PARM4);

	P_RunParticleWeather(min, max, vel, howmany, colour, "rain");
}
static void PF_cl_te_particlesnow (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *min = QC_VECTOR(OFS_PARM0);
	float *max = QC_VECTOR(OFS_PARM1);
	float *vel = QC_VECTOR(OFS_PARM2);
	float howmany = QC_FLOAT(OFS_PARM3);
	float colour = QC_FLOAT(OFS_PARM4);

	P_RunParticleWeather(min, max, vel, howmany, colour, "snow");
}
*/

void CSQC_RunThreads(void)
{
	csqctreadstate_t *state = csqcthreads, *next;
	float ctime = Sys_FloatTime();
	csqcthreads = NULL;
	while(state)
	{
		next = state->next;

		if (state->resumetime > ctime)
		{	//not time yet, reform original list.
			state->next = csqcthreads;
			csqcthreads = state;
		}
		else
		{	//call it and forget it ever happened. The Sleep biltin will recreate if needed.


			*csqcg.self = csqcprogs->EdictToProgs(csqcprogs, csqcprogs->EdictNum(csqcprogs, state->self));
			*csqcg.other = csqcprogs->EdictToProgs(csqcprogs, csqcprogs->EdictNum(csqcprogs, state->other));

			csqcprogs->RunThread(csqcprogs, state->thread);
			csqcprogs->parms->memfree(state->thread);
			csqcprogs->parms->memfree(state);
		}

		state = next;
	}
}

static void PF_cs_addprogs (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *s = QC_GetStringOfs(prinst, OFS_PARM0);
	if (!s || !*s)
		QC_FLOAT(OFS_RETURN) = -1;
	else
		QC_FLOAT(OFS_RETURN) = prinst->LoadProgs(prinst, s, 0, NULL, 0);
}

static void PF_cs_OpenPortal (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifdef Q2BSPS
	if (cl.worldmodel->fromgame == fg_quake2)
		CMQ2_SetAreaPortalState(QC_FLOAT(OFS_PARM0), QC_FLOAT(OFS_PARM1));
#endif
}

static void PF_cs_droptofloor (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t		*ent;
	vec3_t		end;
	vec3_t		start;
	trace_t		trace;

	ent = (csqcedict_t*)prinst->ProgsToEdict(prinst, *csqcg.self);

	VectorCopy (ent->v->origin, end);
	end[2] -= 512;

	VectorCopy (ent->v->origin, start);
	trace = CS_Move (start, ent->v->mins, ent->v->maxs, end, MOVE_NORMAL, ent);

	if (trace.fraction == 1 || trace.allsolid)
		QC_FLOAT(OFS_RETURN) = 0;
	else
	{
		VectorCopy (trace.endpos, ent->v->origin);
		CS_LinkEdict (ent, false);
		ent->v->flags = (int)ent->v->flags | FL_ONGROUND;
		ent->v->groundentity = prinst->EdictToProgs(prinst, trace.ent);
		QC_FLOAT(OFS_RETURN) = 1;
	}
}

static void PF_cs_copyentity (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *in, *out;

	in = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM0);
	out = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM1);

	memcpy(out->v, in->v, csqcentsize);

	CS_LinkEdict (out, false);
}

static void PF_cl_playingdemo (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	QC_FLOAT(OFS_RETURN) = !!cls.demoplayback;
}

static void PF_cl_runningserver (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifdef CLIENTONLY
	QC_FLOAT(OFS_RETURN) = false;
#else
	QC_FLOAT(OFS_RETURN) = !!sv.active;
#endif
}

static void PF_cl_getlight (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *ret = QC_VECTOR(OFS_RETURN);
	int light;
	light = R_LightPoint(QC_VECTOR(OFS_PARM0))/255.0f;
	ret[0] = light;
	ret[1] = light;
	ret[2] = light;
}

/*
static void PF_Stub (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_Printf("Obsolete csqc builtin (%i) executed\n", prinst->lastcalledbuiltinnumber);
}
*/

static void EdictToTransform(csqcedict_t *ed, float *trans)
{
	AngleVectors(ed->v->angles, trans+0, trans+4, trans+8);
	VectorInverse(trans+4);

	trans[3] = ed->v->origin[0];
	trans[7] = ed->v->origin[1];
	trans[11] = ed->v->origin[2];
}

static void PF_rotatevectorsbyangles (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *ang = QC_VECTOR(OFS_PARM0);
	vec3_t src[3], trans[3], res[3];
	ang[0]*=-1;
	AngleVectors(ang, trans[0], trans[1], trans[2]);
	ang[0]*=-1;
	VectorInverse(trans[1]);

	VectorCopy(csqcg.forward, src[0]);
	VectorNegate(csqcg.right, src[1]);
	VectorCopy(csqcg.up, src[2]);

	R_ConcatRotations(trans, src, res);

	VectorCopy(res[0], csqcg.forward);
	VectorNegate(res[1], csqcg.right);
	VectorCopy(res[2], csqcg.up);
}
static void PF_rotatevectorsbymatrix (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	vec3_t src[3], trans[3], res[3];

	VectorCopy(QC_VECTOR(OFS_PARM0), src[0]);
	VectorNegate(QC_VECTOR(OFS_PARM1), src[1]);
	VectorCopy(QC_VECTOR(OFS_PARM2), src[2]);

	VectorCopy(csqcg.forward, src[0]);
	VectorNegate(csqcg.right, src[1]);
	VectorCopy(csqcg.up, src[2]);

	R_ConcatRotations(trans, src, res);

	VectorCopy(res[0], csqcg.forward);
	VectorNegate(res[1], csqcg.right);
	VectorCopy(res[2], csqcg.up);
}




static qboolean CS_CheckBottom (csqcedict_t *ent)
{
	int savedhull;
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;

	if (!cl.worldmodel)
		return false;

	VectorAdd (ent->v->origin, ent->v->mins, mins);
	VectorAdd (ent->v->origin, ent->v->maxs, maxs);

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (!(CS_PointContents (start) == CONTENTS_SOLID))
				goto realcheck;
		}

//	c_yes++;
	return true;		// we got out easy

realcheck:
//	c_no++;
//
// check it for real...
//
	start[2] = mins[2];

// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - 2*STEPSIZE;
	trace = CS_Move (start, vec3_origin, vec3_origin, stop, true, ent);

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];

// the corners must be within 16 of the midpoint
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];

			savedhull = ent->v->hull;
			ent->v->hull = 0;
			trace = CS_Move (start, vec3_origin, vec3_origin, stop, true, ent);
			ent->v->hull = savedhull;

			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > STEPSIZE)
				return false;
		}

//	c_yes++;
	return true;
}
static void PF_cs_checkbottom (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t	*ent;

	ent = (csqcedict_t*)QC_EDICT(prinst, OFS_PARM0);

	QC_FLOAT(OFS_RETURN) = CS_CheckBottom (ent);
}

static void PF_cs_break (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_Printf ("break statement\n");
#ifdef TEXTEDITOR
	(*prinst->pr_trace)++;
#endif
}

static qboolean CS_movestep (csqcedict_t *ent, vec3_t move, qboolean relink, qboolean noenemy, qboolean set_trace)
{
	float		dz;
	vec3_t		oldorg, neworg, end;
	trace_t		trace;
	int			i;
	csqcedict_t		*enemy = csqc_edicts;

// try the move
	VectorCopy (ent->v->origin, oldorg);
	VectorAdd (ent->v->origin, move, neworg);

// flying monsters don't step up
	if ( (int)ent->v->flags & (FL_SWIM | FL_FLY) )
	{
	// try one move with vertical motion, then one without
		for (i=0 ; i<2 ; i++)
		{
			VectorAdd (ent->v->origin, move, neworg);
			if (!noenemy)
			{
				enemy = (csqcedict_t*)csqcprogs->ProgsToEdict(csqcprogs, ent->v->enemy);
				if (i == 0 && enemy != csqc_edicts)
				{
					dz = ent->v->origin[2] - ((csqcedict_t*)csqcprogs->ProgsToEdict(csqcprogs, ent->v->enemy))->v->origin[2];
					if (dz > 40)
						neworg[2] -= 8;
					if (dz < 30)
						neworg[2] += 8;
				}
			}
			trace = CS_Move (ent->v->origin, ent->v->mins, ent->v->maxs, neworg, false, ent);
			if (set_trace)
				cs_settracevars(&trace);

			if (trace.fraction == 1)
			{
				if ( ((int)ent->v->flags & FL_SWIM) && CS_PointContents(trace.endpos) == CONTENTS_EMPTY)
					return false;	// swim monster left water

				VectorCopy (trace.endpos, ent->v->origin);
				if (relink)
					CS_LinkEdict (ent, true);
				return true;
			}

			if (noenemy || enemy == csqc_edicts)
				break;
		}

		return false;
	}

// push down from a step height above the wished position
	neworg[2] += STEPSIZE;
	VectorCopy (neworg, end);
	end[2] -= STEPSIZE*2;

	trace = CS_Move (neworg, ent->v->mins, ent->v->maxs, end, false, ent);
	if (set_trace)
		cs_settracevars(&trace);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		neworg[2] -= STEPSIZE;
		trace = CS_Move (neworg, ent->v->mins, ent->v->maxs, end, false, ent);
		if (set_trace)
			cs_settracevars(&trace);
		if (trace.allsolid || trace.startsolid)
			return false;
	}
	if (trace.fraction == 1)
	{
	// if monster had the ground pulled out, go ahead and fall
		if ( (int)ent->v->flags & FL_PARTIALGROUND )
		{
			VectorAdd (ent->v->origin, move, ent->v->origin);
			if (relink)
				CS_LinkEdict (ent, true);
			ent->v->flags = (int)ent->v->flags & ~FL_ONGROUND;
//	Con_Printf ("fall down\n");
			return true;
		}

		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	VectorCopy (trace.endpos, ent->v->origin);

	if (!CS_CheckBottom (ent))
	{
		if ( (int)ent->v->flags & FL_PARTIALGROUND )
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
				CS_LinkEdict (ent, true);
			return true;
		}
		VectorCopy (oldorg, ent->v->origin);
		return false;
	}

	if ( (int)ent->v->flags & FL_PARTIALGROUND )
	{
//		Con_Printf ("back on ground\n");
		ent->v->flags = (int)ent->v->flags & ~FL_PARTIALGROUND;
	}
	ent->v->groundentity = csqcprogs->EdictToProgs(csqcprogs, trace.ent);

// the move is ok
	if (relink)
		CS_LinkEdict (ent, true);
	return true;
}

static void PF_cs_walkmove (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t	*ent;
	float	yaw, dist;
	vec3_t	move;
//	dfunction_t	*oldf;
	int 	oldself;
	qboolean settrace;

	ent = (csqcedict_t*)prinst->ProgsToEdict(prinst, *csqcg.self);
	yaw = QC_FLOAT(OFS_PARM0);
	dist = QC_FLOAT(OFS_PARM1);
	if (*prinst->callargc >= 3 && QC_FLOAT(OFS_PARM2))
		settrace = true;
	else
		settrace = false;

	if ( !( (int)ent->v->flags & (FL_ONGROUND|FL_FLY|FL_SWIM) ) )
	{
		QC_FLOAT(OFS_RETURN) = 0;
		return;
	}

	yaw = yaw*M_PI*2 / 360;

	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

// save program state, because CS_movestep may call other progs
	oldself = *csqcg.self;

	QC_FLOAT(OFS_RETURN) = CS_movestep(ent, move, true, false, settrace);

// restore program state
	*csqcg.self = oldself;
}

static void CS_ConsoleCommand_f(void)
{	//FIXME: unregister them.
	char cmd[2048];
	Q_snprintf(cmd, sizeof(cmd), "%s %s", Cmd_Argv(0), Cmd_Args());
	CSQC_ConsoleCommand(cmd);
}
static void PF_cs_registercommand (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *str = prinst->VarString(prinst, 0);
	if (!strcmp(str, "+showscores") || !strcmp(str, "-showscores"))
		return;
	Cmd_AddDynCommand(str, CS_ConsoleCommand_f);
}

static qboolean csqc_usinglistener;
qboolean CSQC_SettingListener(void)
{	//stops the engine from setting the listener positions.
	if (csqc_usinglistener)
	{
		csqc_usinglistener = false;
		return true;
	}
	return false;
}
static void PF_cs_setlistener (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *origin = QC_VECTOR(OFS_PARM0);
	float *forward = QC_VECTOR(OFS_PARM1);
	float *right = QC_VECTOR(OFS_PARM2);
	float *up = QC_VECTOR(OFS_PARM3);
	csqc_usinglistener = true;
//	S_UpdateListener(origin, forward, right, up, false);
}

#define RSES_NOLERP 1
#define RSES_NOROTATE 2
#define RSES_NOTRAILS 4
#define RSES_NOLIGHTS 8

void CSQC_EntStateToCSQC(unsigned int flags, float lerptime, unsigned int srcnumber, entity_t *src, csqcedict_t *ent)
{
	model_t *model;
	vec3_t neworg;
	vec3_t newang;
	float ad, f;
	int i;

	if (src->forcelink || flags == RSES_NOLERP)
	{
		VectorCopy(src->msg_origins[0], neworg);
		VectorCopy(src->msg_angles[0], newang);
	}
	else
	{
		f = cl.mlerp;
		for (i = 0; i < 3; i++)
		{
			neworg[i] = src->msg_origins[0][i]-src->msg_origins[1][i];
			if (neworg[i] < -100 || neworg[i] > 100)
				f = 1;	//assume teleport
		}

		for (i = 0; i < 3; i++)
		{
			neworg[i] = src->msg_origins[1][i] + f*neworg[i];
			ad = src->msg_angles[0][i]-src->msg_angles[1][i];
			if (ad < -180)
				ad += 360;
			else if (ad > 180)
				ad -= 360;
			newang[i] = src->msg_angles[1][i] + f*ad;
		}
	}

	//frames needs special handling to lerp them
	ent->v->frame = src->frame;
	//ENGINEDEVS: you'll need to add default interpolation here
	ent->v->frame2 = src->frame;
	ent->v->lerpfrac = 0;	//this is how much of frame2 to be set

	model = cl.model_precache[src->modelindex];
#ifdef TOFIX
	if (!(flags & RSES_NOTRAILS))
	{
		//use entnum as a test to see if its new (if the old origin isn't usable)
		if (ent->v->entnum && model->particletrail >= 0)
		{
			if (pe->ParticleTrail (ent->v->origin, src->origin, model->particletrail, &(le->trailstate)))
				pe->ParticleTrailIndex(ent->v->origin, src->origin, model->traildefaultindex, 0, &(le->trailstate));
		}
	}
#endif

	ent->v->entnum = srcnumber;
	ent->v->modelindex = src->modelindex;
	ent->v->effects = src->effects;
	ent->v->origin[0] = neworg[0];
	ent->v->origin[1] = neworg[1];
	ent->v->origin[2] = neworg[2];
	ent->v->angles[0] = newang[0];
	ent->v->angles[1] = newang[1];
	ent->v->angles[2] = newang[2];

	ent->v->colormap = src->colormapnum;
	ent->v->skin = src->skinnum;

	if (model)
	{
		if (!(flags & RSES_NOROTATE) && (model->flags & EF_ROTATE))
		{
			ent->v->angles[0] = 0;
			ent->v->angles[1] = 100*lerptime;
			ent->v->angles[2] = 0;
		}
	}
}

unsigned int deltaflags[MAX_MODELS];
func_t deltafunction[MAX_MODELS];

typedef struct
{
	unsigned int readpos;	//pos
	unsigned int numents;	//present
	unsigned int maxents;	//buffer size
	struct
	{
		unsigned short n;	//don't rely on the ent->v->entnum
		csqcedict_t *e;	//the csqc ent
	} *e;
} csqcdelta_pack_t;
static csqcdelta_pack_t csqcdelta_pack_new;
static csqcdelta_pack_t csqcdelta_pack_old;
float csqcdelta_time;

void CSQC_DeltaStart(float time)
{
	csqcdelta_pack_t tmp;
	csqcdelta_time = time;

	tmp = csqcdelta_pack_new;
	csqcdelta_pack_new = csqcdelta_pack_old;
	csqcdelta_pack_old = tmp;

	csqcdelta_pack_new.numents = 0;

	csqcdelta_pack_new.readpos = 0;
	csqcdelta_pack_old.readpos = 0;
}
qboolean CSQC_DeltaUpdate(int srcnumber, entity_t *src)
{
	//FTE ensures that this function is called with increasing ent numbers each time
	func_t func;
	func = deltafunction[src->modelindex];
	if (func)
	{
		void *pr_globals;
		csqcedict_t *ent, *oldent;




		if (csqcdelta_pack_old.readpos == csqcdelta_pack_old.numents)
		{	//reached the end of the old frame's ents
			oldent = NULL;
		}
		else
		{
			while (csqcdelta_pack_old.readpos < csqcdelta_pack_old.numents && csqcdelta_pack_old.e[csqcdelta_pack_old.readpos].n < srcnumber)
			{
				//this entity is stale, remove it.
				oldent = csqcdelta_pack_old.e[csqcdelta_pack_old.readpos].e;
				*csqcg.self = csqcprogs->EdictToProgs(csqcprogs, (void*)oldent);
				csqcprogs->ExecuteProgram(csqcprogs, csqcg.ent_remove);
				csqcdelta_pack_old.readpos++;
			}

			if (srcnumber < csqcdelta_pack_old.e[csqcdelta_pack_old.readpos].n)
				oldent = NULL;
			else
			{
				oldent = csqcdelta_pack_old.e[csqcdelta_pack_old.readpos].e;
				csqcdelta_pack_old.readpos++;
			}
		}

		if (srcnumber < maxcsqcentities && csqcent[srcnumber])
		{
			//in the csqc list (don't permit in the delta list too)
			if (oldent)
			{
				*csqcg.self = csqcprogs->EdictToProgs(csqcprogs, (void*)oldent);
				csqcprogs->ExecuteProgram(csqcprogs, csqcg.ent_remove);
			}
			return false;
		}




		if (oldent)
			ent = oldent;
		else
			ent = (csqcedict_t *)csqcprogs->EntAlloc(csqcprogs);

		CSQC_EntStateToCSQC(deltaflags[src->modelindex], csqcdelta_time, srcnumber, src, ent);
		ent->v->drawmask = MASK_DELTA;

	
		*csqcg.self = csqcprogs->EdictToProgs(csqcprogs, (void*)ent);
		pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);
		QC_FLOAT(OFS_PARM0) = !oldent;
		csqcprogs->ExecuteProgram(csqcprogs, func);


		if (csqcdelta_pack_new.maxents <= csqcdelta_pack_new.numents)
		{
			csqcdelta_pack_new.maxents = csqcdelta_pack_new.numents + 64;
			csqcdelta_pack_new.e = realloc(csqcdelta_pack_new.e, sizeof(*csqcdelta_pack_new.e)*csqcdelta_pack_new.maxents);
		}
		csqcdelta_pack_new.e[csqcdelta_pack_new.numents].e = ent;
		csqcdelta_pack_new.e[csqcdelta_pack_new.numents].n = srcnumber;
		csqcdelta_pack_new.numents++;

		return QC_FLOAT(OFS_RETURN);
	}
	return false;
}

void CSQC_DeltaEnd(void)
{
	//remove any unreferenced ents stuck on the end
	while (csqcdelta_pack_old.readpos < csqcdelta_pack_old.numents)
	{
		*csqcg.self = csqcprogs->EdictToProgs(csqcprogs, (void*)csqcdelta_pack_old.e[csqcdelta_pack_old.readpos].e);
		csqcprogs->ExecuteProgram(csqcprogs, csqcg.ent_remove);
		csqcdelta_pack_old.readpos++;
	}
}

void PF_DeltaListen(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	char *mname = QC_GetStringOfs(prinst, OFS_PARM0);
	func_t func = QC_INT(OFS_PARM1);
	unsigned int flags = QC_FLOAT(OFS_PARM2);

	if (prinst->GetFuncArgCount(prinst, func) < 0)
	{
		Con_Printf("PF_DeltaListen: Bad function index\n");
		return;
	}

	if (!strcmp(mname, "*"))
	{
		//yes, even things that are not allocated yet
		for (i = 0; i < MAX_MODELS; i++)
		{
			deltafunction[i] = func;
			deltaflags[i] = flags;
		}
	}
	else
	{
		for (i = 1; i < MAX_MODELS; i++)
		{
			if (!cl.model_precache[i])
				break;
			if (!strcmp(cl.model_precache[i]->name, mname))
			{
				deltafunction[i] = func;
				deltaflags[i] = flags;
				break;
			}
		}
	}
}



#if 1
void PF_ReadServerEntityState(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
}
#else

packet_entities_t *CL_ProcessPacketEntities(float *servertime, qboolean nolerp);
void PF_ReadServerEntityState(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//read the arguments the csqc gave us
	unsigned int flags = QC_FLOAT(OFS_PARM0);
	float servertime = QC_FLOAT(OFS_PARM1);

	//locals
	packet_entities_t *pack;
	csqcedict_t *ent;
	entity_state_t *src;
	unsigned int i;
	lerpents_t		*le;
	csqcedict_t *oldent;
	oldcsqcpack_t *oldlist, *newlist;
	int oldidx = 0, newidx = 0;
	model_t *model;
	player_state_t *srcp;

	//setup
	servertime += cl.servertime;
	pack = CL_ProcessPacketEntities(&servertime, (flags & RSES_NOLERP)); 
	if (!pack)
		return;	//we're lagging. can't do anything, just don't update

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		srcp = &cl.frames[cl.validsequence&UPDATE_MASK].playerstate[i];
		ent = deltaedplayerents[i];
		if (srcp->messagenum == cl.validsequence && (i+1 >= maxcsqcentities || !csqcent[i+1]))
		{
			if (!ent)
			{
				ent = (csqcedict_t *)ED_Alloc(prinst);
				deltaedplayerents[i] = ent;
				QC_FLOAT(OFS_PARM0) = true;
			}
			else
			{
				QC_FLOAT(OFS_PARM0) = false;
			}

			CSQC_PlayerStateToCSQC(i, srcp, ent);

			if (csqcg.delta_update)
			{
				*csqcg.self = EDICT_TO_PROG(prinst, (void*)ent);
				PR_ExecuteProgram(prinst, csqcg.delta_update);
			}
		}
		else if (ent)
		{
			*csqcg.self = EDICT_TO_PROG(prinst, (void*)ent);
			PR_ExecuteProgram(prinst, csqcg.delta_remove);
			deltaedplayerents[i] = NULL;
		}
	}

	oldlist = &loadedcsqcpack[loadedcsqcpacknum];
	loadedcsqcpacknum ^= 1;
	newlist = &loadedcsqcpack[loadedcsqcpacknum];
	newlist->numents = 0;

	for (i = 0; i < pack->num_entities; i++)
	{
		src = &pack->entities[i];
// CL_LinkPacketEntities

#ifndef _MSC_VER
#warning what to do here?
#endif
//		if (csqcent[src->number])
//			continue;	//don't add the entity if we have one sent specially via csqc protocols.

		if (oldidx == oldlist->numents)
		{	//reached the end of the old frame's ents
			oldent = NULL;
		}
		else
		{
			while (oldidx < oldlist->numents && oldlist->entnum[oldidx] < src->number)
			{
				//this entity is stale, remove it.
				oldent = oldlist->entptr[oldidx];
				*csqcg.self = EDICT_TO_PROG(prinst, (void*)oldent);
				PR_ExecuteProgram(prinst, csqcg.delta_remove);
				oldidx++;
			}

			if (src->number < oldlist->entnum[oldidx])
				oldent = NULL;
			else
			{
				oldent = oldlist->entptr[oldidx];
				oldidx++;
			}
		}

		if (src->number < maxcsqcentities && csqcent[src->number])
		{
			//in the csqc list
			if (oldent)
			{
				*csqcg.self = EDICT_TO_PROG(prinst, (void*)oldent);
				PR_ExecuteProgram(prinst, csqcg.delta_remove);
			}
			continue;
		}

		//note: we don't delta the state here. we just replace the old.
		//its already lerped

		if (oldent)
			ent = oldent;
		else
			ent = (csqcedict_t *)ED_Alloc(prinst);

		CSQC_EntStateToCSQC(flags, servertime, src, ent);
		
		if (csqcg.delta_update)
		{
			*csqcg.self = EDICT_TO_PROG(prinst, (void*)ent);
			QC_FLOAT(OFS_PARM0) = !oldent;
			PR_ExecuteProgram(prinst, csqcg.delta_update);
		}

		if (newlist->maxents <= newidx)
		{
			newlist->maxents = newidx + 64;
			newlist->entptr = BZ_Realloc(newlist->entptr, sizeof(*newlist->entptr)*newlist->maxents);
			newlist->entnum = BZ_Realloc(newlist->entnum, sizeof(*newlist->entnum)*newlist->maxents);
		}
		newlist->entptr[newidx] = ent;
		newlist->entnum[newidx] = src->number;
		newidx++;

	}

	//remove any unreferenced ents stuck on the end
	while (oldidx < oldlist->numents)
	{
		oldent = oldlist->entptr[oldidx];
		*csqcg.self = EDICT_TO_PROG(prinst, (void*)oldent);
		PR_ExecuteProgram(prinst, csqcg.delta_remove);
		oldidx++;
	}

	newlist->numents = newidx;
}
#endif

#define PF_FixTen PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme

//prefixes:
//PF_ - common, works on any vm
//PF_cs_ - works in csqc only (dependant upon globals or fields)
//PF_cl_ - works in csqc and menu (if needed...)

//these are the builtins that still need to be added.
#define PS_cs_setattachment		PF_Fixme

#define PF_R_PolygonBegin		PF_Fixme			// #306 void(string texturename) R_BeginPolygon (EXT_CSQC_???)
#define PF_R_PolygonVertex		PF_Fixme			// #307 void(vector org, vector texcoords, vector rgb, float alpha) R_PolygonVertex (EXT_CSQC_???)
#define PF_R_PolygonEnd			PF_Fixme			// #308 void() R_EndPolygon (EXT_CSQC_???)

//warning: functions that depend on globals are bad, mkay?
static struct {
	char *name;
	qcbuiltin_t bifunc;
	int ebfsnum;
}  BuiltinList[] = {
//0
	{"makevectors",	PF_cs_makevectors, 1},		// #1 void() makevectors (QUAKE)
	{"setorigin",	PF_cs_SetOrigin, 2},		// #2 void(entity e, vector org) setorigin (QUAKE)
	{"setmodel",	PF_cs_SetModel, 3},			// #3 void(entity e, string modl) setmodel (QUAKE)
	{"setsize",	PF_cs_SetSize, 4},			// #4 void(entity e, vector mins, vector maxs) setsize (QUAKE)
//5
	{"debugbreak",	PF_cs_break, 6},			// #6 void() debugbreak (QUAKE)
	{"random",	PFC_random,	7},				// #7 float() random (QUAKE)
	{"sound",	PF_cs_sound,	8},			// #8 void(entity e, float chan, string samp, float vol, float atten) sound (QUAKE)
	{"normalize",	PFC_normalize,	9},			// #9 vector(vector in) normalize (QUAKE)
//10
	{"error",	PFC_error,	10},				// #10 void(string errortext) error (QUAKE)
	{"objerror",	PF_objerror,	11},			// #11 void(string errortext) onjerror (QUAKE)
	{"vlen",	PFC_vlen,	12},				// #12 float(vector v) vlen (QUAKE)
	{"vectoyaw",	PFC_vectoyaw,	13},			// #13 float(vector v) vectoyaw (QUAKE)
	{"spawn",	PFC_Spawn,	14},				// #14 entity() spawn (QUAKE)
	{"remove",	PF_cs_remove,	15},			// #15 void(entity e) remove (QUAKE)
	{"traceline",	PF_cs_traceline,	16},		// #16 void(vector v1, vector v2, float nomonst, entity forent) traceline (QUAKE)
	{"checkclient",	PF_NoCSQC,	17},				// #17 entity() checkclient (QUAKE) (don't support)
	{"findstring",	PFC_FindString,	18},			// #18 entity(entity start, .string fld, string match) findstring (QUAKE)
	{"precache_sound",	PF_cs_PrecacheSound,	19},	// #19 void(string str) precache_sound (QUAKE)
//20
	{"precache_model",	PF_cs_PrecacheModel,	20},	// #20 void(string str) precache_model (QUAKE)
	{"stuffcmd",	PF_NoCSQC,	21},		// #21 void(entity client, string s) stuffcmd (QUAKE) (don't support)
	{"findradius",	PF_cs_findradius,	22},		// #22 entity(vector org, float rad) findradius (QUAKE)
	{"bprint",	PF_NoCSQC,	23},				// #23 void(string s, ...) bprint (QUAKE) (don't support)
	{"sprint",	PF_NoCSQC,	24},				// #24 void(entity e, string s, ...) sprint (QUAKE) (don't support)
	{"dprint",	PFC_dprint,	25},				// #25 void(string s, ...) dprint (QUAKE)
	{"ftos",	PFC_ftos,	26},				// #26 string(float f) ftos (QUAKE)
	{"vtos",	PFC_vtos,	27},				// #27 string(vector f) vtos (QUAKE)
	{"coredump",	PFC_coredump,	28},			// #28 void(void) coredump (QUAKE)
	{"traceon",	PFC_traceon,	29},				// #29 void() traceon (QUAKE)
//30
	{"traceoff",	PFC_traceoff,	30},			// #30 void() traceoff (QUAKE)
	{"eprint",	PFC_eprint,	31},				// #31 void(entity e) eprint (QUAKE)
	{"walkmove",	PF_cs_walkmove,	32},			// #32 float(float yaw, float dist) walkmove (QUAKE)
	{"?",	PF_Fixme,	33},				// #33
	{"droptofloor",	PF_cs_droptofloor,	34},		// #34
	{"lightstyle",	PF_cs_lightstyle,	35},		// #35 void(float lightstyle, string stylestring) lightstyle (QUAKE)
	{"rint",	PFC_rint,	36},				// #36 float(float f) rint (QUAKE)
	{"floor",	PFC_floor,	37},				// #37 float(float f) floor (QUAKE)
	{"ceil",	PFC_ceil,	38},				// #38 float(float f) ceil (QUAKE)
//	{"?",	PF_Fixme,	39},				// #39
//40
	{"checkbottom",	PF_cs_checkbottom,	40},	// #40 float(entity e) checkbottom (QUAKE)
	{"pointcontents",	PF_cs_pointcontents,	41},	// #41 float(vector org) pointcontents (QUAKE)
//	{"?",	PF_Fixme,	42},				// #42
	{"fabs",	PFC_fabs,	43},				// #43 float(float f) fabs (QUAKE)
	{"aim",	PF_NoCSQC,	44},				// #44 vector(entity e, float speed) aim (QUAKE) (don't support)
	{"cvar",	PFC_cvar,	45},				// #45 float(string cvarname) cvar (QUAKE)
	{"localcmd",	PFC_localcmd,	46},			// #46 void(string str) localcmd (QUAKE)
	{"nextent",	PFC_nextent,	47},				// #47 entity(entity e) nextent (QUAKE)
	{"particle",	PF_cs_particle,	48},		// #48 void(vector org, vector dir, float colour, float count) particle (QUAKE)
	{"changeyaw",	PF_cs_changeyaw,	49},		// #49 void() changeyaw (QUAKE)
//50
//	{"?",	PF_Fixme,	50},				// #50
	{"vectoangles",	PFC_vectoangles,	51},			// #51 vector(vector v) vectoangles (QUAKE)
//	{"WriteByte",	PF_Fixme,	52},				// #52 void(float to, float f) WriteByte (QUAKE)
//	{"WriteChar",	PF_Fixme,	53},				// #53 void(float to, float f) WriteChar (QUAKE)
//	{"WriteShort",	PF_Fixme,	54},				// #54 void(float to, float f) WriteShort (QUAKE)

//	{"WriteLong",	PF_Fixme,	55},				// #55 void(float to, float f) WriteLong (QUAKE)
//	{"WriteCoord",	PF_Fixme,	56},				// #56 void(float to, float f) WriteCoord (QUAKE)
//	{"WriteAngle",	PF_Fixme,	57},				// #57 void(float to, float f) WriteAngle (QUAKE)
//	{"WriteString",	PF_Fixme,	58},				// #58 void(float to, float f) WriteString (QUAKE)
//	{"WriteEntity",	PF_Fixme,	59},				// #59 void(float to, float f) WriteEntity (QUAKE)

//60
	{"sin",	PFC_Sin,	60},					// #60 float(float angle) sin (DP_QC_SINCOSSQRTPOW)
	{"cos",	PFC_Cos,	61},					// #61 float(float angle) cos (DP_QC_SINCOSSQRTPOW)
	{"sqrt",	PFC_Sqrt,	62},				// #62 float(float value) sqrt (DP_QC_SINCOSSQRTPOW)
	{"changepitch",	PF_cs_changepitch,	63},		// #63 void(entity ent) changepitch (DP_QC_CHANGEPITCH)
	{"tracetoss",	PF_cs_tracetoss,	64},		// #64 void(entity ent, entity ignore) tracetoss (DP_QC_TRACETOSS)

	{"etos",	PFC_etos,	65},				// #65 string(entity ent) etos (DP_QC_ETOS)
	{"?",	PF_Fixme,	66},				// #66
//	{"movetogoal",	PF_Fixme,	67},				// #67 void(float step) movetogoal (QUAKE)
	{"precache_file",	PF_NoCSQC,	68},				// #68 void(string s) precache_file (QUAKE) (don't support)
	{"makestatic",	PF_cs_makestatic,	69},		// #69 void(entity e) makestatic (QUAKE)
//70
	{"changelevel",	PF_NoCSQC,	70},				// #70 void(string mapname) changelevel (QUAKE) (don't support)
//	{"?",	PF_Fixme,	71},				// #71
	{"cvar_set",	PFC_cvar_set,	72},			// #72 void(string cvarname, string valuetoset) cvar_set (QUAKE)
	{"centerprint",	PF_NoCSQC,	73},				// #73 void(entity ent, string text) centerprint (QUAKE) (don't support - cprint is supported instead)
	{"ambientsound",	PF_cl_ambientsound,	74},		// #74 void (vector pos, string samp, float vol, float atten) ambientsound (QUAKE)

	{"precache_model2",	PF_cs_PrecacheModel,	80},	// #75 void(string str) precache_model2 (QUAKE)
	{"precache_sound2",	PF_cs_PrecacheSound,	76},	// #76 void(string str) precache_sound2 (QUAKE)
	{"precache_file2",	PF_NoCSQC,	77},				// #77 void(string str) precache_file2 (QUAKE)
	{"setspawnparms",	PF_NoCSQC,	78},				// #78 void() setspawnparms (QUAKE) (don't support)
	{"logfrag",	PF_NoCSQC,	79},				// #79 void(entity killer, entity killee) logfrag (QW_ENGINE) (don't support)

//80
	{"infokey",	PF_NoCSQC,	80},				// #80 string(entity e, string keyname) infokey (QW_ENGINE) (don't support)
	{"stof",	PFC_stof,	81},				// #81 float(string s) stof (FRIK_FILE or QW_ENGINE)
	{"multicast",	PF_NoCSQC,	82},				// #82 void(vector where, float set) multicast (QW_ENGINE) (don't support)


//90
	{"tracebox",	PF_cs_tracebox,	90},
	{"randomvec",	PFC_randomvector,	91},		// #91 vector() randomvec (DP_QC_RANDOMVEC)
	{"getlight",	PF_cl_getlight,	92},			// #92 vector(vector org) getlight (DP_QC_GETLIGHT)
	{"registercvar",	PFC_registercvar,	93},		// #93 void(string cvarname, string defaultvalue) registercvar (DP_QC_REGISTERCVAR)
	{"min",	PFC_min,	94},				// #94 float(float a, floats) min (DP_QC_MINMAXBOUND)

	{"max",	PFC_max,	95},					// #95 float(float a, floats) max (DP_QC_MINMAXBOUND)
	{"bound",	PFC_bound,	96},				// #96 float(float minimum, float val, float maximum) bound (DP_QC_MINMAXBOUND)
	{"pow",	PFC_pow,	97},					// #97 float(float value) pow (DP_QC_SINCOSSQRTPOW)
	{"findfloat",	PFC_FindFloat,	98},			// #98 entity(entity start, .float fld, float match) findfloat (DP_QC_FINDFLOAT)
	{"checkextension",	PFC_checkextension,	99},		// #99 float(string extname) checkextension (EXT_CSQC)

//110
	{"fopen",	PFC_fopen,	110},				// #110 float(string strname, float accessmode) fopen (FRIK_FILE)
	{"fclose",	PFC_fclose,	111},				// #111 void(float fnum) fclose (FRIK_FILE)
	{"fgets",	PFC_fgets,	112},				// #112 string(float fnum) fgets (FRIK_FILE)
	{"fputs",	PFC_fputs,	113},				// #113 void(float fnum, string str) fputs (FRIK_FILE)
	{"strlen",	PFC_strlen,	114},				// #114 float(string str) strlen (FRIK_FILE)

	{"strcat",	PFC_strcat,	115},				// #115 string(string str1, string str2, ...) strcat (FRIK_FILE)
	{"substring",	PFC_substring,	116},			// #116 string(string str, float start, float length) substring (FRIK_FILE)
	{"stov",	PFC_stov,	117},				// #117 vector(string str) stov (FRIK_FILE)
	{"strzone",	PFC_dupstring,	118},			// #118 string(string str) dupstring (FRIK_FILE)
	{"strunzone",	PFC_forgetstring,	119},		// #119 void(string str) freestring (FRIK_FILE)

//200
	{"precachemodel",	PF_cs_PrecacheModel,	200},
	{"eterncall",	PFC_externcall,	201},
	{"addprogs",	PF_cs_addprogs,	202},
	{"externvalue",	PFC_externvalue,	203},
	{"externset",	PFC_externset,	204},

	{"externrefcall",	PFC_externrefcall,	205},
	{"instr",	PFC_instr,	206},
	{"openportal",	PF_cs_OpenPortal,	207},	//q2bsps
	{"registertempent",	PF_NoCSQC,	208},//{"RegisterTempEnt", PF_RegisterTEnt,	0,		0,		0,		208},
	{"customtempent",	PF_NoCSQC,	209},//{"CustomTempEnt",	PF_CustomTEnt,		0,		0,		0,		209},
//210
//	{"fork",	PF_Fixme,	210},//{"fork",			PF_Fork,			0,		0,		0,		210},
	{"abort",	PFC_Abort,	211}, //#211 void() abort (FTE_MULTITHREADED)
//	{"sleep",	PF_Fixme,	212},//{"sleep",			PF_Sleep,			0,		0,		0,		212},
	{"forceinfokey",	PF_NoCSQC,	213},//{"forceinfokey",	PF_ForceInfoKey,	0,		0,		0,		213},
	{"chat",	PF_NoCSQC,	214},//{"chat",			PF_chat,			0,		0,		0,		214},// #214 void(string filename, float starttag, entity edict) SV_Chat (FTE_NPCCHAT)

//220
	{"strstrofs",	PFC_strstrofs,	221},	// #221 float(string s1, string sub) strstrofs (FTE_STRINGS)
	{"str2chr",	PFC_str2chr,	222},		// #222 float(string str, float index) str2chr (FTE_STRINGS)
	{"chr2str",	PFC_chr2str,	223},		// #223 string(float chr, ...) chr2str (FTE_STRINGS)
	{"strconv",	PFC_strconv,	224},		// #224 string(float ccase, float redalpha, float redchars, string str, ...) strconv (FTE_STRINGS)

	{"strpad",	PFC_strpad,	225},		// #225 string strpad(float pad, string str1, ...) strpad (FTE_STRINGS)
	{"infoadd",	PFC_infoadd,	226},		// #226 string(string old, string key, string value) infoadd
	{"infoget",	PFC_infoget,	227},		// #227 string(string info, string key) infoget
	{"strncmp",	PFC_strncmp,	228},		// #228 float(string s1, string s2, float len) strncmp (FTE_STRINGS)
	{"strcasecmp",	PFC_strcasecmp,	229},	// #229 float(string s1, string s2) strcasecmp (FTE_STRINGS)

//230
	{"strncasecmp",	PFC_strncasecmp,	230},	// #230 float(string s1, string s2, float len) strncasecmp (FTE_STRINGS)
	{"clientstat",	PF_NoCSQC,	231},		// #231 clientstat
	{"runclientphys",	PF_NoCSQC,	232},		// #232 runclientphys
	{"isbackbuffered",	PF_NoCSQC,	233},		// #233 float(entity ent) isbackbuffered
	{"rotatevectorsbytag",	PF_rotatevectorsbytag,	234},	// #234

	{"rotatevectorsbyangle",	PF_rotatevectorsbyangles,	235}, // #235
	{"rotatevectorsbymatrix",	PF_rotatevectorsbymatrix,	236}, // #236
	{"skinforname",	PFC_skinforname,	237},		// #237

	{"stoi",			PFC_stoi,			259},
	{"itos",			PFC_itos,			260},
	{"stoh",			PFC_stoh,			261},
	{"htos",			PFC_htos,			262},

//300
	{"clearscene",	PF_R_ClearScene,	300},				// #300 void() clearscene (EXT_CSQC)
	{"addentities",	PF_R_AddEntityMask,	301},				// #301 void(float mask) addentities (EXT_CSQC)
	{"addentity",	PF_R_AddEntity,	302},					// #302 void(entity ent) addentity (EXT_CSQC)
	{"setproperty",	PF_R_SetViewFlag,	303},				// #303 float(float property, ...) setproperty (EXT_CSQC)
	{"renderscene",	PF_R_RenderScene,	304},				// #304 void() renderscene (EXT_CSQC)

	{"adddynamiclight",	PF_R_AddDynamicLight,	305},			// #305 void(vector org, float radius, vector lightcolours) adddynamiclight (EXT_CSQC)

	{"R_BeginPolygon",	PF_R_PolygonBegin,	306},				// #306 void(string texturename) R_BeginPolygon (EXT_CSQC_???)
	{"R_PolygonVertex",	PF_R_PolygonVertex,	307},				// #307 void(vector org, vector texcoords, vector rgb, float alpha) R_PolygonVertex (EXT_CSQC_???)
	{"R_EndPolygon",	PF_R_PolygonEnd,	308},				// #308 void() R_EndPolygon (EXT_CSQC_???)

	{"getproperty",	PF_R_GetViewFlag,	309},				// #309 vector/float(float property) getproperty (EXT_CSQC_1)

//310
//maths stuff that uses the current view settings.
	{"unproject",	PF_cs_unproject,	310},				// #310 vector (vector v) unproject (EXT_CSQC)
	{"project",	PF_cs_project,		311},				// #311 vector (vector v) project (EXT_CSQC)

//	{"?",	PF_Fixme,			312},				// #312
//	{"?",	PF_Fixme,		313},					// #313
	{"drawfillpal",	PF_CL_drawfillpal,			314},				// #314

//2d (immediate) operations
	{"drawline",	PF_CL_drawline,			315},			// #315 void(float width, vector pos1, vector pos2) drawline (EXT_CSQC)
	{"iscachedpic",	PF_CL_is_cached_pic,		316},		// #316 float(string name) iscachedpic (EXT_CSQC)
	{"precache_pic",	PF_CL_precache_pic,			317},		// #317 string(string name, float trywad) precache_pic (EXT_CSQC)
	{"draw_getimagesize",	PF_CL_drawgetimagesize,		318},		// #318 vector(string picname) draw_getimagesize (EXT_CSQC)
	{"freepic",	PF_CL_free_pic,				319},		// #319 void(string name) freepic (EXT_CSQC)
//320
	{"drawcharacter",	PF_CL_drawcharacter,		320},		// #320 float(vector position, float character, vector scale, vector rgb, float alpha [, float flag]) drawcharacter (EXT_CSQC, [EXT_CSQC_???])
	{"drawstring",	PF_CL_drawstring,				321},	// #321 float(vector position, string text, vector scale, vector rgb, float alpha [, float flag]) drawstring (EXT_CSQC, [EXT_CSQC_???])
	{"drawpic",	PF_CL_drawpic,				322},		// #322 float(vector position, string pic, vector size, vector rgb, float alpha [, float flag]) drawpic (EXT_CSQC, [EXT_CSQC_???])
	{"drawfillrgb",	PF_CL_drawfillrgb,				323},		// #323 float(vector position, vector size, vector rgb, float alpha [, float flag]) drawfill (EXT_CSQC, [EXT_CSQC_???])
	{"drawsetcliparea",	PF_CL_drawsetcliparea,			324},	// #324 void(float x, float y, float width, float height) drawsetcliparea (EXT_CSQC_???)
	{"drawresetcliparea",	PF_CL_drawresetcliparea,	325},		// #325 void(void) drawresetcliparea (EXT_CSQC_???)

	{"drawcolorcodedstring",	PF_CL_drawstring,						326},	// #326
	{"stringwidth",	PF_CL_stringwidth,					327},	// #327 EXT_CSQC_'DARKPLACES'
	{"drawsubpic",	PF_CL_drawsubpic,						328},	// #328 EXT_CSQC_'DARKPLACES'
//	{"?",	PF_Fixme,						329},	// #329 EXT_CSQC_'DARKPLACES'

//330
	{"getstatf",	PF_cs_getstatf,					330},	// #330 float(float stnum) getstatf (EXT_CSQC)
	{"getstati",	PF_cs_getstati,					331},	// #331 float(float stnum) getstati (EXT_CSQC)
	{"getstats",	PF_cs_getstats,					332},	// #332 string(float firststnum) getstats (EXT_CSQC)
	{"setmodelindex",	PF_cs_SetModelIndex,			333},	// #333 void(entity e, float mdlindex) setmodelindex (EXT_CSQC)
	{"modelnameforindex",	PF_cs_ModelnameForIndex,		334},	// #334 string(float mdlindex) modelnameforindex (EXT_CSQC)

	{"particleeffectnum",	PF_cs_particleeffectnum,			335},	// #335 float(string effectname) particleeffectnum (EXT_CSQC)
	{"trailparticles",	PF_cs_trailparticles,			336},	// #336 void(entity ent, float effectnum, vector start, vector end) trailparticles (EXT_CSQC),
	{"pointparticles",	PF_cs_pointparticles,			337},	// #337 void(float effectnum, vector origin [, vector dir, float count]) pointparticles (EXT_CSQC)

	{"cprint",	PF_cl_cprint,					338},	// #338 void(string s) cprint (EXT_CSQC)
	{"print",	PFC_print,						339},	// #339 void(string s) print (EXT_CSQC)

//340
	{"keynumtostring",	PF_cl_keynumtostring,			340},	// #340 string(float keynum) keynumtostring (EXT_CSQC)
	{"stringtokeynum",	PF_cl_stringtokeynum,			341},	// #341 float(string keyname) stringtokeynum (EXT_CSQC)
	{"getkeybind",	PF_cl_getkeybind,				342},	// #342 string(float keynum) getkeybind (EXT_CSQC)

//	{"?",	PF_Fixme,						343},	// #343
//	{"?",	PF_Fixme,						344},	// #344

	{"getinputstate",	PF_cs_getinputstate,			345},	// #345 float(float framenum) getinputstate (EXT_CSQC)
	{"setsensitivityscaler",	PF_cs_setsensativityscaler, 	346},	// #346 void(float sens) setsensitivityscaler (EXT_CSQC)

	{"runstandardplayerphysics",	PF_cs_runplayerphysics,			347},	// #347 void() runstandardplayerphysics (EXT_CSQC)

	{"getplayerkeyvalue",	PF_cs_getplayerkey,				348},	// #348 string(float playernum, string keyname) getplayerkeyvalue (EXT_CSQC)

	{"isdemo",	PF_cl_playingdemo,				349},	// #349 float() isdemo (EXT_CSQC)
//350
	{"isserver",	PF_cl_runningserver,			350},	// #350 float() isserver (EXT_CSQC)

	{"SetListener",	PF_cs_setlistener, 				351},	// #351 void(vector origin, vector forward, vector right, vector up) SetListener (EXT_CSQC)
	{"registercommand",	PF_cs_registercommand,			352},	// #352 void(string cmdname) registercommand (EXT_CSQC)
	{"wasfreed",	PFC_WasFreed,					353},	// #353 float(entity ent) wasfreed (EXT_CSQC) (should be availabe on server too)

	{"serverkey",	PF_cs_serverkey,				354},	// #354 string(string key) serverkey;
	{"getentitytoken",	PF_cs_getentitytoken,			355},	// #355 string() getentitytoken;
//	{"?",	PF_Fixme,						356},	// #356
//	{"?",	PF_Fixme,						357},	// #357
//	{"?",	PF_Fixme,						358},	// #358
	{"sendevent",	PF_cs_sendevent,				359},	// #359	void(string evname, string evargs, ...) (EXT_CSQC_1)

//360
//note that 'ReadEntity' is pretty hard to implement reliably. Modders should use a combination of ReadShort, and findfloat, and remember that it might not be known clientside (pvs culled or other reason)
	{"readbyte",	PF_ReadByte,					360},	// #360 float() readbyte (EXT_CSQC)
	{"readchar",	PF_ReadChar,					361},	// #361 float() readchar (EXT_CSQC)
	{"readshort",	PF_ReadShort,					362},	// #362 float() readshort (EXT_CSQC)
	{"readlong",	PF_ReadLong,					363},	// #363 float() readlong (EXT_CSQC)
	{"readcoord",	PF_ReadCoord,					364},	// #364 float() readcoord (EXT_CSQC)

	{"readangle",	PF_ReadAngle,					365},	// #365 float() readangle (EXT_CSQC)
	{"readstring",	PF_ReadString,					366},	// #366 string() readstring (EXT_CSQC)
	{"readfloat",	PF_ReadFloat,					367},	// #367 string() readfloat (EXT_CSQC)
	{"readentitynum",	PF_ReadEntityNum,				368},	// #368 float() readentitynum (EXT_CSQC)

	{"readserverentitystate",	PF_ReadServerEntityState,		369},	// #369 void(float flags, float simtime) readserverentitystate (EXT_CSQC_1)
//	{"readsingleentitystate",	PF_ReadSingleEntityState,		370},
	{"deltalisten",	PF_DeltaListen,					371},		// #371 float(string modelname, float flags) deltalisten  (EXT_CSQC_1)

//400
	{"copyentity",	PF_cs_copyentity,		400},	// #400 void(entity from, entity to) copyentity (DP_QC_COPYENTITY)
	{"setcolors",	PF_NoCSQC,				401},	// #401 void(entity cl, float colours) setcolors (DP_SV_SETCOLOR) (don't implement)
	{"findchain",	PF_cs_findchain,			402},	// #402 entity(string field, string match) findchain (DP_QC_FINDCHAIN)
	{"findchainfloat",	PF_cs_findchainfloat,		403},	// #403 entity(float fld, float match) findchainfloat (DP_QC_FINDCHAINFLOAT)
	{"effect",	PF_cl_effect,		404},		// #404 void(vector org, string modelname, float startframe, float endframe, float framerate) effect (DP_SV_EFFECT)

	{"te_blood",	PF_cl_te_blooddp,		405},	// #405 void(vector org, vector velocity, float howmany) te_blood (DP_TE_BLOOD)
	{"te_bloodshower",	PF_cl_te_bloodshower,406},		// #406 void(vector mincorner, vector maxcorner, float explosionspeed, float howmany) te_bloodshower (DP_TE_BLOODSHOWER)
	{"te_explosionrgb",	PF_cl_te_explosionrgb,	407},	// #407 void(vector org, vector color) te_explosionrgb (DP_TE_EXPLOSIONRGB)
	{"te_particlecube",	PF_cl_te_particlecube,408},		// #408 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color, float gravityflag, float randomveljitter) te_particlecube (DP_TE_PARTICLECUBE)
	{"te_particlerain",	PF_cl_te_particlerain,	409},	// #409 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlerain (DP_TE_PARTICLERAIN)

	{"te_particlesnow",	PF_cl_te_particlesnow,410},		// #410 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color) te_particlesnow (DP_TE_PARTICLESNOW)
	{"te_spark",	PF_cl_te_spark,		411},		// #411 void(vector org, vector vel, float howmany) te_spark (DP_TE_SPARK)
	{"te_gunshotquad",	PF_cl_te_gunshotquad,	412},	// #412 void(vector org) te_gunshotquad (DP_TE_QUADEFFECTS1)
	{"te_spikequad",	PF_cl_te_spikequad,	413},		// #413 void(vector org) te_spikequad (DP_TE_QUADEFFECTS1)
	{"te_superspikequad",	PF_cl_te_superspikequad,414},	// #414 void(vector org) te_superspikequad (DP_TE_QUADEFFECTS1)

	{"te_explosionquad",	PF_cl_te_explosionquad,	415},	// #415 void(vector org) te_explosionquad (DP_TE_QUADEFFECTS1)
	{"te_smallflash",	PF_cl_te_smallflash,	416},	// #416 void(vector org) te_smallflash (DP_TE_SMALLFLASH)
	{"te_customflash",	PF_cl_te_customflash,	417},	// #417 void(vector org, float radius, float lifetime, vector color) te_customflash (DP_TE_CUSTOMFLASH)
	{"te_gunshot",	PF_cl_te_gunshot,	418},		// #418 void(vector org) te_gunshot (DP_TE_STANDARDEFFECTBUILTINS)
	{"te_spike",	PF_cl_te_spike,		419},		// #419 void(vector org) te_spike (DP_TE_STANDARDEFFECTBUILTINS)

	{"te_superspike",	PF_cl_te_superspike,420},		// #420 void(vector org) te_superspike (DP_TE_STANDARDEFFECTBUILTINS)
	{"te_explosion",	PF_cl_te_explosion,	421},		// #421 void(vector org) te_explosion (DP_TE_STANDARDEFFECTBUILTINS)
	{"te_tarexplosion",	PF_cl_te_tarexplosion,422},		// #422 void(vector org) te_tarexplosion (DP_TE_STANDARDEFFECTBUILTINS)
	{"te_wizspike",	PF_cl_te_wizspike,	423},		// #423 void(vector org) te_wizspike (DP_TE_STANDARDEFFECTBUILTINS)
	{"te_knightspike",	PF_cl_te_knightspike,424},		// #424 void(vector org) te_knightspike (DP_TE_STANDARDEFFECTBUILTINS)

	{"te_lavasplash",	PF_cl_te_lavasplash,425},		// #425 void(vector org) te_lavasplash  (DP_TE_STANDARDEFFECTBUILTINS)
	{"te_teleport",	PF_cl_te_teleport,	426},		// #426 void(vector org) te_teleport (DP_TE_STANDARDEFFECTBUILTINS)
	{"te_explosion2",	PF_cl_te_explosion2,427},		// #427 void(vector org, float color, float colorlength) te_explosion2 (DP_TE_STANDARDEFFECTBUILTINS)
	{"te_lightning1",	PF_cl_te_lightning1,	428},	// #428 void(entity own, vector start, vector end) te_lightning1 (DP_TE_STANDARDEFFECTBUILTINS)
	{"te_lightning2",	PF_cl_te_lightning2,429},		// #429 void(entity own, vector start, vector end) te_lightning2 (DP_TE_STANDARDEFFECTBUILTINS)

	{"te_lightning3",	PF_cl_te_lightning3,430},		// #430 void(entity own, vector start, vector end) te_lightning3 (DP_TE_STANDARDEFFECTBUILTINS)
	{"te_beam",	PF_cl_te_beam,		431},		// #431 void(entity own, vector start, vector end) te_beam (DP_TE_STANDARDEFFECTBUILTINS)
	{"vectorvectors",	PF_cs_vectorvectors,432},		// #432 void(vector dir) vectorvectors (DP_QC_VECTORVECTORS)
	{"te_plasmaburn",	PF_cl_te_plasmaburn,433},		// #433 void(vector org) te_plasmaburn (DP_TE_PLASMABURN)
//	{"getsurfacenumpoints",	PF_Fixme,					434},		// #434 float(entity e, float s) getsurfacenumpoints (DP_QC_GETSURFACE)

//	{"getsurfacepoint",	PF_Fixme,					435},		// #435 vector(entity e, float s, float n) getsurfacepoint (DP_QC_GETSURFACE)
//	{"getsurfacenormal",	PF_Fixme,					436},		// #436 vector(entity e, float s) getsurfacenormal (DP_QC_GETSURFACE)
//	{"getsurfacetexture",	PF_Fixme,				437},			// #437 string(entity e, float s) getsurfacetexture (DP_QC_GETSURFACE)
//	{"getsurfacenearpoint",	PF_Fixme,					438},		// #438 float(entity e, vector p) getsurfacenearpoint (DP_QC_GETSURFACE)
//	{"getsurfaceclippedpoint",	PF_Fixme,				439},			// #439 vector(entity e, float s, vector p) getsurfaceclippedpoint (DP_QC_GETSURFACE)

	{"clientcommand",	PF_NoCSQC,			440},		// #440 void(entity e, string s) clientcommand (KRIMZON_SV_PARSECLIENTCOMMAND) (don't implement)
	{"tokenize",	PFC_Tokenize,		441},		// #441 float(string s) tokenize (KRIMZON_SV_PARSECLIENTCOMMAND)
	{"argv",	PFC_ArgV,			442},		// #442 string(float n) argv (KRIMZON_SV_PARSECLIENTCOMMAND)
	{"setattachment",	PS_cs_setattachment,443},		// #443 void(entity e, entity tagentity, string tagname) setattachment (DP_GFX_QUAKE3MODELTAGS)
	{"search_begin",	PFC_search_begin,	444},		// #444 float	search_begin(string pattern, float caseinsensitive, float quiet) (DP_QC_FS_SEARCH)

	{"search_end",	PFC_search_end,			445},	// #445 void	search_end(float handle) (DP_QC_FS_SEARCH)
	{"search_getsize",	PFC_search_getsize,	446},		// #446 float	search_getsize(float handle) (DP_QC_FS_SEARCH)
	{"search_getfilename",	PFC_search_getfilename,447},		// #447 string	search_getfilename(float handle, float num) (DP_QC_FS_SEARCH)
	{"dp_cvar_string",	PFC_cvar_string,		448},		// #448 string(float n) cvar_string (DP_QC_CVAR_STRING)
	{"findflags",	PFC_FindFlags,		449},		// #449 entity(entity start, .entity fld, float match) findflags (DP_QC_FINDFLAGS)

	{"findchainflags",	PF_cs_findchainflags,	450},		// #450 entity(.float fld, float match) findchainflags (DP_QC_FINDCHAINFLAGS)
	{"gettagindex",	PF_cs_gettagindex,	451},		// #451 float(entity ent, string tagname) gettagindex (DP_MD3_TAGSINFO)
	{"gettaginfo",	PF_cs_gettaginfo,	452},		// #452 vector(entity ent, float tagindex) gettaginfo (DP_MD3_TAGSINFO)
	{"dropclient",	PF_NoCSQC,			453},		// #453 void(entity player) dropclient (DP_SV_BOTCLIENT) (don't implement)
	{"spawnclient",	PF_NoCSQC,			454},		// #454	entity() spawnclient (DP_SV_BOTCLIENT) (don't implement)

	{"clienttype",	PF_NoCSQC,			455},		// #455 float(entity client) clienttype (DP_SV_BOTCLIENT) (don't implement)

	
//	{"WriteUnterminatedString",PF_WriteString2,		456},	//writestring but without the null terminator. makes things a little nicer.

//DP_TE_FLAMEJET
//	{"te_flamejet",		PF_te_flamejet,			457},	// #457 void(vector org, vector vel, float howmany) te_flamejet

	//no 458 documented.

//DP_QC_EDICT_NUM
	{"edict_num",		PFC_edict_for_num,		459},	// #459 entity(float entnum) edict_num

//DP_QC_STRINGBUFFERS
	{"buf_create",		PFC_buf_create,		460},	// #460 float() buf_create
	{"buf_del",			PFC_buf_del,				461},	// #461 void(float bufhandle) buf_del
	{"buf_getsize",		PFC_buf_getsize,		462},	// #462 float(float bufhandle) buf_getsize
	{"buf_copy",		PFC_buf_copy,		463},	// #463 void(float bufhandle_from, float bufhandle_to) buf_copy
	{"buf_sort",		PFC_buf_sort,		464},	// #464 void(float bufhandle, float sortpower, float backward) buf_sort
	{"buf_implode",		PFC_buf_implode,		465},	// #465 string(float bufhandle, string glue) buf_implode
	{"bufstr_get",		PFC_bufstr_get,		466},	// #466 string(float bufhandle, float string_index) bufstr_get
	{"bufstr_set",		PFC_bufstr_set,		467},	// #467 void(float bufhandle, float string_index, string str) bufstr_set
	{"bufstr_add",		PFC_bufstr_add,		468},	// #468 float(float bufhandle, string str, float order) bufstr_add
	{"bufstr_free",		PFC_bufstr_free,			469},	// #469 void(float bufhandle, float string_index) bufstr_free

	//no 470 documented

//DP_QC_ASINACOSATANATAN2TAN
	{"asin",			PFC_asin,			471},	// #471 float(float s) asin
	{"acos",			PFC_acos,			472},	// #472 float(float c) acos
	{"atan",			PFC_atan,			473},	// #473 float(float t) atan
	{"atan2",			PFC_atan2,			474},	// #474 float(float c, float s) atan2
	{"tan",				PFC_tan,				475},	// #475 float(float a) tan


////DP_QC_STRINGCOLORFUNCTIONS
	{"strlennocol",		PFC_strlennocol,		476},	// #476 float(string s) strlennocol
	{"strdecolorize",	PFC_strdecolorize,	477},	// #477 string(string s) strdecolorize

//DP_QC_STRFTIME
	{"strftime",		PFC_strftime,		478},	// #478 string(float uselocaltime, string format, ...) strftime

//DP_QC_TOKENIZEBYSEPARATOR
	{"tokenizebyseparator",PFC_tokenizebyseparator,	479},	// #479 float(string s, string separator1, ...) tokenizebyseparator

//DP_QC_STRING_CASE_FUNCTIONS
	{"strtolower",		PFC_strtolower,		480},	// #476 string(string s) strtolower
	{"strtoupper",		PFC_strtoupper,		481},	// #476 string(string s) strlennocol

//DP_QC_CVAR_DEFSTRING
	{"cvar_defstring",	PFC_cvar_defstring,	482},	// #482 string(string s) cvar_defstring

//DP_SV_POINTSOUND
	{"pointsound",		PF_cs_pointsound,		483},	// #483 void(vector origin, string sample, float volume, float attenuation) pointsound

//DP_QC_STRREPLACE
	{"strreplace",		PFC_strreplace,		484},	// #484 string(string search, string replace, string subject) strreplace
	{"strireplace",		PFC_strireplace,		485},	// #485 string(string search, string replace, string subject) strireplace


//DP_QC_GETSURFACEPOINTATTRIBUTE
	{"getsurfacepointattribute",PF_getsurfacepointattribute,	486},	// #486vector(entity e, float s, float n, float a) getsurfacepointattribute

#ifndef NOMEDIA
//DP_GECKO_SUPPORT
	{"gecko_create",	PF_cs_gecko_create,		487},	// #487 float(string name) gecko_create( string name )
	{"gecko_destroy",	PF_cs_gecko_destroy,	488},	// #488 void(string name) gecko_destroy( string name )
	{"gecko_navigate",	PF_cs_gecko_navigate,	489},	// #489 void(string name) gecko_navigate( string name, string URI )
	{"gecko_keyevent",	PF_cs_gecko_keyevent,	490},	// #490 float(string name) gecko_keyevent( string name, float key, float eventtype )
	{"gecko_mousemove",	PF_cs_gecko_mousemove,	491},	// #491 void gecko_mousemove( string name, float x, float y )
	{"gecko_resize",	PF_cs_gecko_resize,	492},	// #492 void gecko_resize( string name, float w, float h )
	{"gecko_get_texture_extent",PF_cs_gecko_get_texture_extent,	493},	// #493 vector gecko_get_texture_extent( string name )
#endif

//DP_QC_CRC16
	{"crc16",			PFC_crc16,				494},	// #494 float(float caseinsensitive, string s, ...) crc16

//DP_QC_CVAR_TYPE
	{"cvar_type",		PFC_cvar_type,		495},	// #495 float(string name) cvar_type

//DP_QC_ENTITYDATA
	{"numentityfields",	PFC_numentityfields,			496},	// #496 float() numentityfields
	{"entityfieldname",	PFC_entityfieldname,			497},	// #497 string(float fieldnum) entityfieldname
	{"entityfieldtype",	PFC_entityfieldtype,		498},	// #498 float(float fieldnum) entityfieldtype
	{"getentityfieldstring",PFC_getentityfieldstring,		499},	// #499 string(float fieldnum, entity ent) getentityfieldstring
	{"putentityfieldstring",PFC_putentityfieldstring,	500},	// #500 float(float fieldnum, entity ent, string s) putentityfieldstring

//DP_SV_WRITEPICTURE
	{"WritePicture",	PF_cl_ReadPicture,		501},	// #501 void(float to, string s, float sz) WritePicture

	//no 502 documented

//DP_QC_WHICHPACK
	{"whichpack",		PFC_whichpack,			503},	// #503 string(string filename) whichpack

//DP_QC_URI_ESCAPE
	{"uri_escape",		PFC_uri_escape,				510},	// #510 string(string in) uri_escape
	{"uri_unescape",	PFC_uri_unescape,	511},	// #511 string(string in) uri_unescape = #511;

//DP_QC_NUM_FOR_EDICT
	{"num_for_edict",	PFC_num_for_edict,		512},	// #512 float(entity ent) num_for_edict

//DP_QC_URI_GET
	{"uri_get",			PFC_uri_get,			513},	// #513 float(string uril, float id) uri_get

	{"keynumtostring",			PF_cl_keynumtostring,			520},	// #520
	{"findkeysforcommand",			PF_cl_findkeysforcommand,			521},	// #521

	{NULL}
};

static qcbuiltin_t csqc_builtins[550];




static jmp_buf csqc_abort;
static progparms_t csqcprogparms;


//Any menu builtin error or anything like that will come here.
void VARGS CSQC_Abort (char *format, ...)	//an error occured.
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr, format);
	Q_vsnprintf (string,sizeof(string)-1, format,argptr);
	va_end (argptr);

	Con_Printf("CSQC_Abort: %s\nShutting down csqc\n", string);

	if (pr_csqc_coreonerror.value)
	{
		int size = 1024*1024*8;
		char *buffer = malloc(size);
		csqcprogs->save_ents(csqcprogs, buffer, &size, 3);
		COM_WriteFile("csqccore.txt", buffer, size);
		free(buffer);
	}

	Host_EndGame("csqc error");
}

void CSQC_ForgetThreads(void)
{
	csqctreadstate_t *state = csqcthreads, *next;
	csqcthreads = NULL;
	while(state)
	{
		next = state->next;

		csqcprogs->parms->memfree(state->thread);
		csqcprogs->parms->memfree(state);

		state = next;
	}
}

void CSQC_ClearState(void)
{
	if (csprogs_ptr)
	{
		COM_FreeMallocFile(csprogs_ptr);
		csprogs_len = -1;
		csprogs_ptr = NULL;
	}
	if (csqcprogs)
	{
		CSQC_ForgetThreads();
		CloseProgs(csqcprogs);
		csqcprogs = NULL;
		Con_Printf("Closed csqc\n");
	}

	if (csqcdelta_pack_new.e)
		free(csqcdelta_pack_new.e);
	memset(&csqcdelta_pack_new, 0, sizeof(csqcdelta_pack_new));
	if (csqcdelta_pack_old.e)
		free(csqcdelta_pack_old.e);
	memset(&csqcdelta_pack_old, 0, sizeof(csqcdelta_pack_old));

	memset(&deltafunction, 0, sizeof(deltafunction));

	csqcmapentitydata = NULL;
	csqcmapentitydataloaded = false;

	cl.sensitivityscale = 1;
	num_csqc_edicts = 0;

	csqc_usinglistener = false;

	memset(&csqcg, 0, sizeof(csqcg));
}

void CSQC_Shutdown(void)
{
	memset(&csauth, 0, sizeof(csauth));
	CSQC_ClearState();
}

unsigned int CRC_Block(unsigned char *file, unsigned int filelength)
{
	unsigned short rcrc;
	CRC_Init(&rcrc);
	while (filelength-->0)
		CRC_ProcessByte(&rcrc, *file++);
	return CRC_Value(rcrc);
}

qboolean CSQC_LocateCSProgs(void)
{
	char newname[MAX_QPATH];

	if (!csauth.hash)
	{
		if (csauth.permissive)
		{
			csprogs_ptr = COM_LoadMallocFile(csauth.filename);
			csprogs_len = com_filesize;
		}
		else
		{
			csprogs_ptr = NULL;
			csprogs_len = -1;
		}
		return csprogs_ptr!=NULL;
	}

	Q_snprintf(newname, MAX_QPATH, "csprogsvers/%x.dat", csauth.hash);

	//read from the cache if available
	csprogs_ptr = COM_LoadMallocFile(newname);
	csprogs_len = com_filesize;
	if (csprogs_ptr)
	{
		//make sure the version is as expected
		if (csprogs_len == csauth.len)
		//and they didn't try and hack the cache
		if (CRC_Block(csprogs_ptr, csprogs_len) == csauth.hash)
		{
			return true;
		}
		COM_FreeMallocFile(csprogs_ptr);
	}

	//okay, its not available in our cache
	//maybe we can get a real csprogs.dat?

	csprogs_ptr = COM_LoadMallocFile(csauth.filename);
	csprogs_len = com_filesize;
	if (csprogs_ptr && !cls.demoplayback)	//allow them to use csprogs.dat if playing a demo, and don't care about the checksum
	{
		//make sure the version is as required
		if (csprogs_len == csauth.len)
		//and that its the right version
		if (CRC_Block(csprogs_ptr, csprogs_len) == csauth.hash)
		{
			//back it up into the cache
			COM_WriteFile(newname, csprogs_ptr, csprogs_len);
			return true;
		}

		//it is a valid progs, not the one we intended, but it is valid
		if (csauth.permissive)
			return true;

		COM_FreeMallocFile(csprogs_ptr);
	}

	csprogs_ptr = NULL;
	csprogs_len = -1;

	return false;
}

//when the qclib needs a file, it calls out to this function.
byte *CSQC_PRLoadFile (char *path, void *buffer, int bufsize)
{
	byte *file;

	if (!strcmp(path, "csprogs.dat"))
	{
		if (csprogs_len >= 0)
			memcpy(buffer, csprogs_ptr, csprogs_len);
		return buffer;
	}

	return COM_LoadStackFile(path, buffer, bufsize);
}

int CSQC_PRFileSize (char *path)
{
	byte *file;

	if (!strcmp(path, "csprogs.dat"))
	{
		return csprogs_len;
	}

	return -1;
}

qboolean CSQC_IsLoaded(void)
{
	if (csqcprogs)
		return true;
	return false;
}

double  csqctime;
qboolean CSQC_Init (void)
{
	int i;
	string_t *str;
	csqcedict_t *worldent;

	csqc_usinglistener = false;

	//its already running...
	if (csqcprogs)
		return false;

	cl.sensitivityscale = 1;

	if (cl_nocsqc.value)
		return false;

	if (!*csauth.filename)
		strcpy(csauth.filename, "csprogs.dat");
	if (cls.demoplayback)
	{
		//allow csprogs to be loaded even if not specified
		csauth.permissive = true;
	}

	if (!CSQC_LocateCSProgs())
		return false;

	for (i = 0; i < sizeof(csqc_builtins)/sizeof(csqc_builtins[0]); i++)
		csqc_builtins[i] = PF_Fixme;
	for (i = 0; BuiltinList[i].bifunc; i++)
	{
		if (BuiltinList[i].ebfsnum)
			csqc_builtins[BuiltinList[i].ebfsnum] = BuiltinList[i].bifunc;
	}

	memset(cl.model_csqcname, 0, sizeof(cl.model_csqcname));
	memset(cl.model_csqcprecache, 0, sizeof(cl.model_csqcprecache));

	csqcprogparms.progsversion = PROGSTRUCT_VERSION;
	csqcprogparms.ReadFile = CSQC_PRLoadFile;//char *(*ReadFile) (char *fname, void *buffer, int *len);
	csqcprogparms.FileSize = CSQC_PRFileSize;//int (*FileSize) (char *fname);	//-1 if file does not exist
	csqcprogparms.WriteFile = QC_WriteFile;//bool (*WriteFile) (char *name, void *data, int len);
	csqcprogparms.printf = (void *)Con_Printf;//Con_Printf;//void (*printf) (char *, ...);
	csqcprogparms.Sys_Error = (void *)Sys_Error;
	csqcprogparms.Abort = CSQC_Abort;
	csqcprogparms.edictsize = sizeof(csqcedict_t);

	csqcprogparms.entspawn = NULL;//void (*entspawn) (struct edict_s *ent);	//ent has been spawned, but may not have all the extra variables (that may need to be set) set
	csqcprogparms.entcanfree = NULL;//bool (*entcanfree) (struct edict_s *ent);	//return true to stop ent from being freed
	csqcprogparms.stateop = NULL;//StateOp;//void (*stateop) (float var, func_t func);
	csqcprogparms.cstateop = NULL;//CStateOp;
	csqcprogparms.cwstateop = NULL;//CWStateOp;
	csqcprogparms.thinktimeop = NULL;//ThinkTimeOp;

	//used when loading a game
	csqcprogparms.builtinsfor = NULL;//builtin_t *(*builtinsfor) (int num);	//must return a pointer to the builtins that were used before the state was saved.
	csqcprogparms.loadcompleate = NULL;//void (*loadcompleate) (int edictsize);	//notification to reset any pointers.

	csqcprogparms.memalloc = PR_CB_Malloc;//void *(*memalloc) (int size);	//small string allocation	malloced and freed randomly
	csqcprogparms.memfree = PR_CB_Free;//void (*memfree) (void * mem);


	csqcprogparms.globalbuiltins = csqc_builtins;//builtin_t *globalbuiltins;	//these are available to all progs
	csqcprogparms.numglobalbuiltins = sizeof(csqc_builtins)/sizeof(csqc_builtins[0]);

	csqcprogparms.autocompile = PR_COMPILEIGNORE;//enum {PR_NOCOMPILE, PR_COMPILENEXIST, PR_COMPILECHANGED, PR_COMPILEALWAYS} autocompile;

	csqcprogparms.gametime = &csqctime;

	csqcprogparms.sv_edicts = (struct edict_s **)&csqc_edicts;
	csqcprogparms.sv_num_edicts = &num_csqc_edicts;

	csqcprogparms.useeditor = NULL;//void (*useeditor) (char *filename, int line, int nump, char **parms);

	csqctime = Sys_FloatTime();
	if (!csqcprogs)
	{
		cl.sensitivityscale = 1;
		csqcmapentitydataloaded = true;
		csqcprogs = InitProgs(&csqcprogparms);
		csqcprogs->Configure(csqcprogs, -1, 16);

		CS_ClearWorld();
		SV_InitBoxHull();
		CSQC_InitFields();	//let the qclib know the field order that the engine needs.

		if (csqcprogs->LoadProgs(csqcprogs, "csprogs.dat", 17105, NULL, 0) < 0) //standard.
		{
			CSQC_Shutdown();
			//failed to load or something
			return false;
		}

		if (csprogs_ptr)
		{
			COM_FreeMallocFile(csprogs_ptr);
			csprogs_len = -1;
			csprogs_ptr = NULL;
		}

		if (setjmp(csqc_abort))
		{
			CSQC_Shutdown();
			return false;
		}

		CSQC_FindGlobals();

		csqc_fakereadbyte = -1;
		memset(csqcent, 0, sizeof(*csqcent)*maxcsqcentities);	//clear the server->csqc entity translations.

		csqcentsize = csqcprogs->InitEnts(csqcprogs, pr_csmaxedicts.value);

		csqcprogs->EntAlloc(csqcprogs);	//we need a word entity.
		//world edict becomes readonly
		worldent = (csqcedict_t *)csqcprogs->EdictNum(csqcprogs, 0);

		worldent->readonly = true;
		worldent->isfree = false;
		worldent->v->model = csqcprogs->StringToProgs(csqcprogs, cl.model_precache[1]->name);
		worldent->v->modelindex = 1;
		worldent->v->solid = SOLID_BSP;

		str = (string_t*)csqcprogs->GetEdictFieldValue(csqcprogs, (edict_t*)worldent, "message", NULL);
		if (str)
			*str = csqcprogs->StringToProgs(csqcprogs, cl.levelname);
		
		str = (string_t*)csqcprogs->FindGlobal(csqcprogs, "mapname", 0);
		if (str)
		{
			char mapname[MAX_QPATH];
			if (!cl.model_precache[1])
				*mapname = *"";
			else
			{
				COM_StripExtension(COM_SkipPath(cl.model_precache[1]->name), mapname);
			}
			if (!*mapname)
				strcpy(mapname, "unknown");
			*str = QC_NewString(csqcprogs, mapname, strlen(mapname)+1);
		}

		if (csqcg.init_function)
		{
			void *pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);
			QC_FLOAT(OFS_PARM0) = 1.0;	//api version
			(((string_t *)pr_globals)[OFS_PARM1] = csqcprogs->TempString(csqcprogs, FULLENGINENAME));
			QC_FLOAT(OFS_PARM2) = VERSION;
			csqcprogs->ExecuteProgram(csqcprogs, csqcg.init_function);
		}

		Con_Printf("Loaded csqc\n");
		csqcmapentitydataloaded = false;
	}

	return true; //success!
}

void CSQC_WorldLoaded(void)
{
	if (!csqcprogs)
		return;
	if (csqcmapentitydataloaded)
		return;
	csqcmapentitydataloaded = true;
	csqcmapentitydata = cl.worldmodel->entities;
	if (csqcg.worldloaded)
		csqcprogs->ExecuteProgram(csqcprogs, csqcg.worldloaded);
	csqcmapentitydata = NULL;
}

void CSQC_CoreDump(void)
{
	if (!csqcprogs)
	{
		Con_Printf("Can't core dump, you need to be running the CSQC progs first.");
		return;
	}

	{
		int size = 1024*1024*8;
		char *buffer = malloc(size);
		csqcprogs->save_ents(csqcprogs, buffer, &size, 3);
		COM_WriteFile("csqccore.txt", buffer, size);
		free(buffer);
	}

}

void PR_CSExtensionList_f(void)
{
	int i;
	int ebi;
	int bi;
	lh_extension_t *extlist;

#define SHOW_ACTIVEEXT 1
#define SHOW_ACTIVEBI 2
#define SHOW_NOTSUPPORTEDEXT 4
#define SHOW_NOTACTIVEEXT 8
#define SHOW_NOTACTIVEBI 16

	int showflags = atoi(Cmd_Argv(1));
	if (!showflags)
		showflags = SHOW_ACTIVEEXT|SHOW_NOTACTIVEEXT;

	//make sure the info is valid
	if (!csqc_builtins[0])
	{
		for (i = 0; i < sizeof(csqc_builtins)/sizeof(csqc_builtins[0]); i++)
			csqc_builtins[i] = PF_Fixme;
		for (i = 0; BuiltinList[i].bifunc; i++)
		{
			if (BuiltinList[i].ebfsnum)
				csqc_builtins[BuiltinList[i].ebfsnum] = BuiltinList[i].bifunc;
		}
	}


	if (showflags & (SHOW_ACTIVEBI|SHOW_NOTACTIVEBI))
	for (i = 0; BuiltinList[i].name; i++)
	{
		if (!BuiltinList[i].ebfsnum)
			continue;	//a reserved builtin.
		if (BuiltinList[i].bifunc == PF_Fixme)
			Con_Printf("^1%s:%i needs to be added\n", BuiltinList[i].name, BuiltinList[i].ebfsnum);
		else if (csqc_builtins[BuiltinList[i].ebfsnum] == BuiltinList[i].bifunc)
		{
			if (showflags & SHOW_ACTIVEBI)
				Con_Printf("%s is active on %i\n", BuiltinList[i].name, BuiltinList[i].ebfsnum);
		}
		else
		{
			if (showflags & SHOW_NOTACTIVEBI)
				Con_Printf("^4%s is NOT active (%i)\n", BuiltinList[i].name, BuiltinList[i].ebfsnum);
		}
	}

	if (showflags & (SHOW_NOTSUPPORTEDEXT|SHOW_NOTACTIVEEXT|SHOW_ACTIVEEXT))
	{
		extlist = QSG_Extensions;

		for (i = 0; i < QSG_Extensions_count; i++)
		{
			if (!extlist[i].name)
				continue;

			for (ebi = 0; ebi < extlist[i].numbuiltins; ebi++)
			{
				for (bi = 0; BuiltinList[bi].name; bi++)
				{
					if (!strcmp(BuiltinList[bi].name, extlist[i].builtinnames[ebi]))
						break;
				}

				if (!BuiltinList[bi].name)
				{
					if (showflags & SHOW_NOTSUPPORTEDEXT)
						Con_Printf("^4%s is not supported\n", extlist[i].name);
					break;
				}
				if (csqc_builtins[BuiltinList[bi].ebfsnum] != BuiltinList[bi].bifunc)
				{
					if (csqc_builtins[BuiltinList[bi].ebfsnum] == PF_Fixme)
					{
						if (showflags & SHOW_NOTACTIVEEXT)
							Con_Printf("^4%s is not currently active (builtin: %s#%i)\n", extlist[i].name, BuiltinList[bi].name, BuiltinList[bi].ebfsnum);
					}
					else
					{
						if (showflags & SHOW_NOTACTIVEEXT)
							Con_Printf("^4%s was overridden (builtin: %s#%i)\n", extlist[i].name, BuiltinList[bi].name, BuiltinList[bi].ebfsnum);
					}
					break;
				}
			}
			if (ebi == extlist[i].numbuiltins)
			{
				if (showflags & SHOW_ACTIVEEXT)
				{
					if (!extlist[i].numbuiltins)
						Con_Printf("%s is supported\n", extlist[i].name);
					else
						Con_Printf("%s is currently active\n", extlist[i].name);
				}
			}
		}
	}
}

void CSQC_RegisterCvarsAndThings(void)
{
	Cmd_AddCommand("coredump_csqc", CSQC_CoreDump);
	Cmd_AddCommand ("extensionlist_csqc", PR_CSExtensionList_f);


	Cvar_RegisterVariable(&pr_csmaxedicts);
	Cvar_RegisterVariable(&cl_nocsqc);
	Cvar_RegisterVariable(&pr_csqc_coreonerror);
}

qboolean CSQC_DrawView(void)
{
	if (!csqcg.draw_function || !csqcprogs || !cl.worldmodel)
		return false;

	CL_CalcClientTime();

	csqc_resortfrags = true;

	if (csqcg.player_localentnum)
		*csqcg.player_localentnum = cl.viewentity;

	if (csqcg.view_angles)
	{
		csqcg.view_angles[0] = cl.viewangles[0];
		csqcg.view_angles[1] = cl.viewangles[1];
		csqcg.view_angles[2] = cl.viewangles[2];
	}


	if (csqcg.clientcommandframe)
		*csqcg.clientcommandframe = cls.inputlog_sequenceout;
	if (csqcg.servercommandframe)
		*csqcg.servercommandframe = cls.inputlog_sequencein;
	if (csqcg.intermission)
		*csqcg.intermission = cl.intermission;


/*	if (csqcg.dpcompat_sbshowscores)
	{
		extern qboolean sb_showscores;
		*csqcg.dpcompat_sbshowscores = sb_showscores;
	}
*/
	if (csqcg.cltime)
		*csqcg.cltime = cl.time;
	if (csqcg.svtime)
		*csqcg.svtime = cl.mtime[0];

	CSQC_RunThreads();	//wake up any qc threads

	//EXT_CSQC_1
	{
		void *pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);
		QC_FLOAT(OFS_PARM0) = vid.width;
		QC_FLOAT(OFS_PARM1) = vid.height;
		QC_FLOAT(OFS_PARM2) = !(key_dest == key_menu);
	}
	//end EXT_CSQC_1
	csqcprogs->ExecuteProgram(csqcprogs, csqcg.draw_function);

	return true;
}

qboolean CSQC_KeyPress(int key, qboolean down)
{
	void *pr_globals;

	if (!csqcprogs || !csqcg.input_event)
		return false;

	pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);
	QC_FLOAT(OFS_PARM0) = !down;
	QC_FLOAT(OFS_PARM1) = CSQC_Key_NativeToCSQC(key);
	QC_FLOAT(OFS_PARM2) = 0;

	csqcprogs->ExecuteProgram (csqcprogs, csqcg.input_event);

	return QC_FLOAT(OFS_RETURN);
}
qboolean CSQC_MouseMove(float xdelta, float ydelta)
{
	void *pr_globals;

	if (!csqcprogs || !csqcg.input_event)
		return false;

	pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);
	QC_FLOAT(OFS_PARM0) = 2;
	QC_FLOAT(OFS_PARM1) = xdelta;
	QC_FLOAT(OFS_PARM2) = ydelta;

	csqcprogs->ExecuteProgram (csqcprogs, csqcg.input_event);

	return QC_FLOAT(OFS_RETURN);
}

qboolean CSQC_ConsoleCommand(char *cmd)
{
	void *pr_globals;
	if (!csqcprogs || !csqcg.console_command)
		return false;

	pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);
	(((string_t *)pr_globals)[OFS_PARM0] = csqcprogs->TempString(csqcprogs, cmd));

	csqcprogs->ExecuteProgram (csqcprogs, csqcg.console_command);
	return QC_FLOAT(OFS_RETURN);
}

#pragma message("do we really need the firstbyte parameter here?")
qboolean CSQC_ParseTempEntity(unsigned char firstbyte)
{
	void *pr_globals;
	if (!csqcprogs || !csqcg.parse_tempentity)
		return false;

	csqc_fakereadbyte = firstbyte;
	pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);
	csqcprogs->ExecuteProgram (csqcprogs, csqcg.parse_tempentity);
	csqc_fakereadbyte = -1;
	return !!QC_FLOAT(OFS_RETURN);
}

qboolean CSQC_ParseGamePacket(void)
{
	return false;
}

qboolean CSQC_LoadResource(char *resname, char *restype)
{
	void *pr_globals;
	if (!csqcprogs || !csqcg.loadresource)
		return true;

	pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);
	(((string_t *)pr_globals)[OFS_PARM0] = csqcprogs->TempString(csqcprogs, resname));
	(((string_t *)pr_globals)[OFS_PARM0] = csqcprogs->TempString(csqcprogs, restype));

	csqcprogs->ExecuteProgram (csqcprogs, csqcg.loadresource);

	return !!QC_FLOAT(OFS_RETURN);
}

static qboolean CSQC_Version_StuffCmd(char *cmd)
{
	Cmd_TokenizeString(cmd);
	cmd = Cmd_Argv(0);

	if (!strcmp(cmd, "name"))
	{
		strncpy(csauth.filename, Cmd_Argv(1), sizeof(csauth.filename)-1);
		return true;
	}
	if (!strcmp(cmd, "size"))
	{
		csauth.len = strtoul(Cmd_Argv(1), NULL, 0);
		return true;
	}
	if (!strcmp(cmd, "crc"))
	{
		csauth.hash = strtoul(Cmd_Argv(1), NULL, 0);
		return true;
	}
	return false;
}

qboolean CSQC_StuffCmd(char *cmd)
{
	void *pr_globals;
	if (!strncmp(cmd, "//csqc_prog", 11))
	{
		if (CSQC_Version_StuffCmd(cmd+11))
			return true;
	}
	if (!strncmp(cmd, "csqc_prog", 9))
	{
		if (CSQC_Version_StuffCmd(cmd+9))
			return true;
	}

	if (!csqcprogs || !csqcg.parse_stuffcmd)
		return false;

	pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);
	(((string_t *)pr_globals)[OFS_PARM0] = csqcprogs->TempString(csqcprogs, cmd));

	csqcprogs->ExecuteProgram (csqcprogs, csqcg.parse_stuffcmd);
	return true;
}
qboolean CSQC_CenterPrint(char *cmd)
{
	void *pr_globals;
	if (!csqcprogs || !csqcg.parse_centerprint)
		return false;

	pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);
	(((string_t *)pr_globals)[OFS_PARM0] = csqcprogs->TempString(csqcprogs, cmd));

	csqcprogs->ExecuteProgram (csqcprogs, csqcg.parse_centerprint);
	return QC_FLOAT(OFS_RETURN);
}

void CSQC_Input_Frame(usercmdextra_t *cmd)
{
	if (!csqcprogs || !csqcg.input_frame)
		return;

	CL_CalcClientTime();
	if (csqcg.svtime)
		*csqcg.svtime = cl.mtime[0];
	if (csqcg.cltime)
		*csqcg.cltime = cl.time;

	if (csqcg.clientcommandframe)
		*csqcg.clientcommandframe = cls.inputlog_sequenceout;

	cs_set_input_state(cmd);
	csqcprogs->ExecuteProgram (csqcprogs, csqcg.input_frame);
	cs_get_input_state(cmd);
}

static void CSQC_EntityCheck(int entnum)
{
	int newmax;

	if (entnum >= maxcsqcentities)
	{
		newmax = entnum+64;
		csqcent = realloc(csqcent, sizeof(*csqcent)*newmax);
		memset(csqcent + maxcsqcentities, 0, (newmax - maxcsqcentities)*sizeof(csqcent));
		maxcsqcentities = newmax;
	}
}

int CSQC_StartSound(int entnum, int channel, char *soundname, vec3_t pos, float vol, float attenuation)
{
	void *pr_globals;
	csqcedict_t *ent;

	if (!csqcprogs)
		return false;
	if (csqcg.event_sound)
	{
		pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);

		CSQC_EntityCheck(entnum);
		ent = csqcent[entnum];
		if (!ent)
			ent = csqc_edicts;
		*csqcg.self = csqcprogs->EdictToProgs(csqcprogs, (void*)ent);

		QC_FLOAT(OFS_PARM0) = entnum;
		QC_FLOAT(OFS_PARM1) = channel;
		QC_INT(OFS_PARM2) = csqcprogs->TempString(csqcprogs, soundname);
		QC_FLOAT(OFS_PARM3) = vol;
		QC_FLOAT(OFS_PARM4) = attenuation;
		VectorCopy(pos, QC_VECTOR(OFS_PARM5));

		csqcprogs->ExecuteProgram(csqcprogs, csqcg.event_sound);

		return QC_FLOAT(OFS_RETURN);
	}
	else if (csqcg.serversound)
	{
		CSQC_EntityCheck(entnum);
		ent = csqcent[entnum];
		if (!ent)
			ent = csqc_edicts;

		pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);

		*csqcg.self = csqcprogs->EdictToProgs(csqcprogs, (void*)ent);
		QC_FLOAT(OFS_PARM0) = channel;
		QC_INT(OFS_PARM1) = csqcprogs->TempString(csqcprogs, soundname);
		VectorCopy(pos, QC_VECTOR(OFS_PARM2));
		QC_FLOAT(OFS_PARM3) = vol;
		QC_FLOAT(OFS_PARM4) = attenuation;
		QC_FLOAT(OFS_PARM5) = entnum;

		csqcprogs->ExecuteProgram(csqcprogs, csqcg.serversound);

		return QC_FLOAT(OFS_RETURN);
	}
	return false;
}

//this protocol allows up to 32767 edicts.
void CSQC_ParseEntities(void)
{
	csqcedict_t *ent;
	unsigned short entnum;
	void *pr_globals;
	int packetsize;
	int packetstart;

	if (!csqcprogs)
		Host_EndGame("CSQC needs to be initialized for this server.\n");

	if (!csqcg.ent_update || !csqcg.self)
		Host_EndGame("CSQC is unable to parse entities\n");

	pr_globals = csqcprogs->globals(csqcprogs, PR_CURRENT);

	CL_CalcClientTime();
	if (csqcg.svtime)		//estimated server time
		*csqcg.svtime = cl.mtime[0];
	if (csqcg.cltime)	//smooth client time.
		*csqcg.cltime = cl.time;

	if (csqcg.clientcommandframe)
		*csqcg.clientcommandframe = cls.inputlog_sequenceout;
	if (csqcg.servercommandframe)
		*csqcg.servercommandframe = cls.inputlog_sequencein;

	for(;;)
	{
		entnum = MSG_ReadShort();
		if (!entnum || msg_badread)
			break;
		if (entnum & 0x8000)
		{	//remove
			entnum &= ~0x8000;

			if (!entnum)
				Host_EndGame("CSQC cannot remove world!\n");

			CSQC_EntityCheck(entnum);

			ent = csqcent[entnum];
			csqcent[entnum] = NULL;

			if (!ent)	//hrm.
				continue;

			*csqcg.self = csqcprogs->EdictToProgs(csqcprogs, (void*)ent);
			csqcprogs->ExecuteProgram(csqcprogs, csqcg.ent_remove);
			//the csqc is expected to call the remove builtin.
		}
		else
		{
			CSQC_EntityCheck(entnum);

			if (cl_csqcdebug)
			{
				packetsize = MSG_ReadShort();
				packetstart = msg_readcount;
			}
			else
			{
				packetsize = 0;
				packetstart = 0;
			}

			ent = csqcent[entnum];
			if (!ent)
			{
				ent = (csqcedict_t*)csqcprogs->EntAlloc(csqcprogs);
				csqcent[entnum] = ent;
				ent->v->entnum = entnum;
				QC_FLOAT(OFS_PARM0) = true;
			}
			else
			{
				QC_FLOAT(OFS_PARM0) = false;
			}

			*csqcg.self = csqcprogs->EdictToProgs(csqcprogs, (void*)ent);
			csqcprogs->ExecuteProgram(csqcprogs, csqcg.ent_update);

			if (cl_csqcdebug)
			{
				if (msg_readcount != packetstart+packetsize)
				{
					if (msg_readcount > packetstart+packetsize)
						Con_Printf("CSQC overread entity %i. Size %i, read %i\n", entnum, packetsize, msg_readcount - packetsize);
					else
						Con_Printf("CSQC underread entity %i. Size %i, read %i\n", entnum, packetsize, msg_readcount - packetsize);
					Con_Printf("First byte is %i\n", net_message.data[msg_readcount]);
				}
				msg_readcount = packetstart+packetsize;	//leetism.
			}
		}
	}
}

#endif
