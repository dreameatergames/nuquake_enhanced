#include "quakedef.h"

//this file houses the builtins that are specific to the client. Be that menu or csprogs.
//At this time, we don't support menu.

#if defined(EXT_CSQC)

#include "pr_common.h"


void PF_cl_findkeysforcommand (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifdef TOFIX
	moo
#endif
}

void PF_cl_getkeybind (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifdef TOFIX
	moo
#endif
}

void PF_cl_stringtokeynum (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	char *s;

	s = QC_GetStringOfs(prinst, OFS_PARM0);
	i = Key_StringToKeynum(s);
	if (i < 0)
	{
		QC_FLOAT(OFS_RETURN) = -1;
		return;
	}
	i = CSQC_Key_CSQCToNative(i);
	QC_FLOAT(OFS_RETURN) = i;
}

void PF_cl_keynumtostring (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int code = QC_FLOAT(OFS_PARM0);

	code = CSQC_Key_NativeToCSQC (code);

	RETURN_TSTRING(Key_KeynumToString(code));
}

void PF_CL_drawsubpic (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifdef TOFIX
	moo
#endif
}

void PF_CL_stringwidth (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//returns length of the string as it would be printed.
	//basically removes ^4 type colours.
	//note that we don't support that anyway, so passing through to the strlen function works fine

	PFC_strlen(prinst, pr_globals);
}

void PF_CL_drawresetcliparea (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//csqc doesn't need to implement this
}

void PF_CL_drawsetcliparea (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//csqc doesn't need to implement this
}

void PF_CL_drawfillpal (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	float *size = QC_VECTOR(OFS_PARM1);
	float pal = QC_FLOAT(OFS_PARM2);

	Draw_Fill(pos[0], pos[1], size[0], size[1], pal);
}
void PF_CL_drawfillrgb (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	float *size = QC_VECTOR(OFS_PARM1);
	float *rgb = QC_VECTOR(OFS_PARM2);
	float alpha = QC_FLOAT(OFS_PARM3);

	Draw_FillRGBA(pos[0], pos[1], size[0], size[1], rgb[0], rgb[1], rgb[2], alpha);
}

void Draw_AlphaPic (int x, int y, qpic_t *pic, float alpha);
void PF_CL_drawpic (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qpic_t *pic;
	float *pos = QC_VECTOR(OFS_PARM0);
	char *picname = QC_GetStringOfs(prinst, OFS_PARM1);
	float *size = QC_VECTOR(OFS_PARM2);
	float *rgb = QC_VECTOR(OFS_PARM3);
	float alpha = QC_FLOAT(OFS_PARM4);

	pic = Draw_CachePic_CSQC(picname);
#ifdef GLQUAKE
	if (alpha < 1)
		Draw_AlphaPic(pos[0], pos[1], pic, alpha);
	else
#endif
		Draw_Pic(pos[0], pos[1], pic);
}

void PF_CL_drawstring (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	char *text = QC_GetStringOfs(prinst, OFS_PARM1);
	float *size = QC_VECTOR(OFS_PARM2);
	float *rgb = QC_VECTOR(OFS_PARM3);
	float alpha = QC_FLOAT(OFS_PARM4);

	Draw_String(pos[0], pos[1], text);
}

void PF_CL_drawcharacter (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *pos = QC_VECTOR(OFS_PARM0);
	int chara = QC_FLOAT(OFS_PARM1);
	float *size = QC_VECTOR(OFS_PARM2);
	float *rgb = QC_VECTOR(OFS_PARM3);
	float alpha = QC_FLOAT(OFS_PARM4);

	Draw_Character(pos[0], pos[1], chara);
}

void PF_CL_free_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*str = QC_GetStringOfs(prinst, OFS_PARM0);

	//not supported. doesn't matter really.
}

void PF_CL_drawgetimagesize (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qpic_t *pic;
	char *picname = QC_GetStringOfs(prinst, OFS_PARM0);
	pic = Draw_CachePic_CSQC(picname);
	if (pic)
	{
		QC_FLOAT(OFS_RETURN+0) = pic->width;
		QC_FLOAT(OFS_RETURN+1) = pic->height;
		QC_FLOAT(OFS_RETURN+2) = 0;
	}
	else
	{
		QC_FLOAT(OFS_RETURN+0) = 0;
		QC_FLOAT(OFS_RETURN+1) = 0;
		QC_FLOAT(OFS_RETURN+2) = 0;
	}
}

void PF_CL_precache_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*picname = QC_GetStringOfs(prinst, OFS_PARM0);
	Draw_CachePic_CSQC(picname);
}

void PF_CL_is_cached_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*picname = QC_GetStringOfs(prinst, OFS_PARM0);
	QC_FLOAT(OFS_RETURN) = Draw_CachePic_CSQC(picname)?1:0;
}

void PF_CL_drawline (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifdef TOFIX
	moo
#endif
}




int CSQC_Key_CSQCToNative(int code)
{
	switch(code)
	{
	case K_TAB:				return 9;
	case K_ENTER:			return 13;
	case K_ESCAPE:			return 27;
	case K_SPACE:			return 32;
	case K_BACKSPACE:		return 127;
	case K_UPARROW:			return 128;
	case K_DOWNARROW:		return 129;
	case K_LEFTARROW:		return 130;
	case K_RIGHTARROW:		return 131;
	case K_ALT:				return 132;
	case K_CTRL:			return 133;
	case K_SHIFT:			return 134;
	case K_F1:				return 135;
	case K_F2:				return 136;
	case K_F3:				return 137;
	case K_F4:				return 138;
	case K_F5:				return 139;
	case K_F6:				return 140;
	case K_F7:				return 141;
	case K_F8:				return 142;
	case K_F9:				return 143;
	case K_F10:				return 144;
	case K_F11:				return 145;
	case K_F12:				return 146;
	case K_INS:				return 147;
	case K_DEL:				return 148;
	case K_PGDN:			return 149;
	case K_PGUP:			return 150;
	case K_HOME:			return 151;
	case K_END:				return 152;
//	case K_KP_HOME:			return 160;
//	case K_KP_UPARROW:		return 161;
//	case K_KP_PGUP:			return 162;
//	case K_KP_LEFTARROW:	return 163;
//	case K_KP_5:			return 164;
//	case K_KP_RIGHTARROW:	return 165;
//	case K_KP_END:			return 166;
//	case K_KP_DOWNARROW:	return 167;
//	case K_KP_PGDN:			return 168;
//	case K_KP_ENTER:		return 169;
//	case K_KP_INS:			return 170;
//	case K_KP_DEL:			return 171;
//	case K_KP_SLASH:		return 172;
//	case K_KP_MINUS:		return 173;
//	case K_KP_PLUS:			return 174;
	case K_PAUSE:			return 255;
	case K_JOY1:			return 768;
	case K_JOY2:			return 769;
	case K_JOY3:			return 770;
	case K_JOY4:			return 771;
	case K_AUX1:			return 772;
	case K_AUX2:			return 773;
	case K_AUX3:			return 774;
	case K_AUX4:			return 775;
	case K_AUX5:			return 776;
	case K_AUX6:			return 777;
	case K_AUX7:			return 778;
	case K_AUX8:			return 779;
	case K_AUX9:			return 780;
	case K_AUX10:			return 781;
	case K_AUX11:			return 782;
	case K_AUX12:			return 783;
	case K_AUX13:			return 784;
	case K_AUX14:			return 785;
	case K_AUX15:			return 786;
	case K_AUX16:			return 787;
	case K_AUX17:			return 788;
	case K_AUX18:			return 789;
	case K_AUX19:			return 790;
	case K_AUX20:			return 791;
	case K_AUX21:			return 792;
	case K_AUX22:			return 793;
	case K_AUX23:			return 794;
	case K_AUX24:			return 795;
	case K_AUX25:			return 796;
	case K_AUX26:			return 797;
	case K_AUX27:			return 798;
	case K_AUX28:			return 799;
	case K_AUX29:			return 800;
	case K_AUX30:			return 801;
	case K_AUX31:			return 802;
	case K_AUX32:			return 803;
	case K_MOUSE1:			return 512;
	case K_MOUSE2:			return 513;
	case K_MOUSE3:			return 514;
//	case K_MOUSE4:			return 515;
//	case K_MOUSE5:			return 516;
//	case K_MOUSE6:			return 517;
//	case K_MOUSE7:			return 518;
//	case K_MOUSE8:			return 519;
//	case K_MOUSE9:			return 520;
//	case K_MOUSE10:			return 521;
	case K_MWHEELDOWN:		return 515;//K_MOUSE4;
	case K_MWHEELUP:		return 516;//K_MOUSE5;
	default:				return code;
	}
}

int CSQC_Key_NativeToCSQC(int code)
{
	switch(code)
	{
	case 9:			return K_TAB;
	case 13:		return K_ENTER;
	case 27:		return K_ESCAPE;
	case 32:		return K_SPACE;
	case 127:		return K_BACKSPACE;
	case 128:		return K_UPARROW;
	case 129:		return K_DOWNARROW;
	case 130:		return K_LEFTARROW;
	case 131:		return K_RIGHTARROW;
	case 132:		return K_ALT;
	case 133:		return K_CTRL;
	case 134:		return K_SHIFT;
	case 135:		return K_F1;
	case 136:		return K_F2;
	case 137:		return K_F3;
	case 138:		return K_F4;
	case 139:		return K_F5;
	case 140:		return K_F6;
	case 141:		return K_F7;
	case 142:		return K_F8;
	case 143:		return K_F9;
	case 144:		return K_F10;
	case 145:		return K_F11;
	case 146:		return K_F12;
	case 147:		return K_INS;
	case 148:		return K_DEL;
	case 149:		return K_PGDN;
	case 150:		return K_PGUP;
	case 151:		return K_HOME;
	case 152:		return K_END;
//	case 160:		return K_KP_HOME;
//	case 161:		return K_KP_UPARROW;
//	case 162:		return K_KP_PGUP;
//	case 163:		return K_KP_LEFTARROW;
//	case 164:		return K_KP_5;
//	case 165:		return K_KP_RIGHTARROW;
//	case 166:		return K_KP_END;
//	case 167:		return K_KP_DOWNARROW;
//	case 168:		return K_KP_PGDN;
//	case 169:		return K_KP_ENTER;
//	case 170:		return K_KP_INS;
//	case 171:		return K_KP_DEL;
//	case 172:		return K_KP_SLASH;
//	case 173:		return K_KP_MINUS;
//	case 174:		return K_KP_PLUS;
	case 255:		return K_PAUSE;

	case 768:		return K_JOY1;
	case 769:		return K_JOY2;
	case 770:		return K_JOY3;
	case 771:		return K_JOY4;
	case 772:		return K_AUX1;
	case 773:		return K_AUX2;
	case 774:		return K_AUX3;
	case 775:		return K_AUX4;
	case 776:		return K_AUX5;
	case 777:		return K_AUX6;
	case 778:		return K_AUX7;
	case 779:		return K_AUX8;
	case 780:		return K_AUX9;
	case 781:		return K_AUX10;
	case 782:		return K_AUX11;
	case 783:		return K_AUX12;
	case 784:		return K_AUX13;
	case 785:		return K_AUX14;
	case 786:		return K_AUX15;
	case 787:		return K_AUX16;
	case 788:		return K_AUX17;
	case 789:		return K_AUX18;
	case 790:		return K_AUX19;
	case 791:		return K_AUX20;
	case 792:		return K_AUX21;
	case 793:		return K_AUX22;
	case 794:		return K_AUX23;
	case 795:		return K_AUX24;
	case 796:		return K_AUX25;
	case 797:		return K_AUX26;
	case 798:		return K_AUX27;
	case 799:		return K_AUX28;
	case 800:		return K_AUX29;
	case 801:		return K_AUX30;
	case 802:		return K_AUX31;
	case 803:		return K_AUX32;
	case 512:		return K_MOUSE1;
	case 513:		return K_MOUSE2;
	case 514:		return K_MOUSE3;
//	case 515:		return K_MOUSE4;
//	case 516:		return K_MOUSE5;
//	case 517:		return K_MOUSE6;
//	case 518:		return K_MOUSE7;
//	case 519:		return K_MOUSE8;
//	case 520:		return K_MOUSE9;
//	case 521:		return K_MOUSE10;
	case 515:		return K_MWHEELDOWN;//K_MOUSE4;
	case 516:		return K_MWHEELUP;//K_MOUSE5;
	default:		return code;
	}
}

#endif
