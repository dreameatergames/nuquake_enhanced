#include <kos.h>

void Host_SavegameComment (char *text);
void Host_Savegame_f (void);
void Host_Loadgame_f (void);

#ifdef QUAKE2
void SaveGamestate();
int LoadGamestate(char *level, char *startspot);
void Host_Changelevel2_f (void);
#endif
