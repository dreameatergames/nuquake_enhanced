void EmitWaterPolys (msurface_t *fa);
void EmitWaterPolys_Temp (msurface_t *fa);

void R_DrawSkyChain (msurface_t *s);
qboolean R_CullBox (const vec3_t mins, const vec3_t maxs);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);
int R_LightPoint (vec3_t p);

void R_RotateForEntity (entity_t *e);
void R_AnimateLight (void);
void R_DrawWorld (void);
void R_DrawBrushModel (entity_t *ent);
void R_RenderDlights (void);
void R_DrawWaterSurfaces (void);
int R_LightPoint (vec3_t p);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);

void V_CalcBlend (void);