/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "rain.h" //magic nipples - rain

#define MAX_CLIENTS 32

extern rain_properties Rain; //magic nipples - rain

//LRC - the fogging fog
vec3_t FogColor;
int g_fFadeDuration; //negative = fading out
float g_fStartDist;
float g_fFinalValue;
float g_ftargetValue;
float g_iStartValue;

extern BEAM *pBeam;
extern BEAM *pBeam2;

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud :: MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf )
{
	ASSERT( iSize == 0 );

	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while ( pList )
	{
		if ( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	//LRC - the fogging fog
	g_fStartDist = 0;
	g_fFinalValue = 0;
	g_iStartValue = 30000;

	return 1;
}

void CAM_ToFirstPerson(void);

void CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
}

void CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;

	m_Lensflare.SunEnabled = FALSE; //magic nipples - lensflare
}


int CHud :: MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	return 1;
}


int CHud :: MsgFunc_Damage(const char *pszName, int iSize, void *pbuf )
{
	int		armor, blood;
	Vector	from;
	int		i;
	float	count;
	
	BEGIN_READ( pbuf, iSize );
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i=0 ; i<3 ; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return 1;
}

int CHud :: MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iConcussionEffect = READ_BYTE();
	if (m_iConcussionEffect)
		this->m_StatusIcons.EnableIcon("dmg_concuss",255,160,0);
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}

void CHud::MsgFunc_SetFog(const char* pszName, int iSize, void* pbuf) //LRC - the fogging fog
{
	BEGIN_READ(pbuf, iSize);
	FogColor.x = TransformColor(READ_BYTE());
	FogColor.y = TransformColor(READ_BYTE());
	FogColor.z = TransformColor(READ_BYTE());
	g_fFadeDuration = READ_BYTE();
	g_fStartDist = READ_SHORT();
	if (g_fFadeDuration > 0)
	{
		g_ftargetValue = READ_SHORT();
	}
	else //if (g_fFadeDuration <= 0)
	{
		g_fFinalValue = g_iStartValue = READ_SHORT();
	}
}

int CHud::MsgFunc_RainData(const char* pszName, int iSize, void* pbuf) //magic nipples - rain
{
	BEGIN_READ(pbuf, iSize);
	Rain.dripsPerSecond = READ_SHORT();
	Rain.distFromPlayer = READ_COORD();
	Rain.windX = READ_COORD();
	Rain.windY = READ_COORD();
	Rain.randX = READ_COORD();
	Rain.randY = READ_COORD();
	Rain.weatherMode = READ_SHORT();
	Rain.globalHeight = READ_COORD();

	return 1;
}

int CHud::MsgFunc_AddELight(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	dlight_t* dl = gEngfuncs.pEfxAPI->CL_AllocElight(READ_SHORT());

	int bELightActive = READ_BYTE();
	if (bELightActive <= 0)
	{
		dl->die = gEngfuncs.GetClientTime();
	}
	else
	{
		dl->die = gEngfuncs.GetClientTime() + 1E6;

		dl->origin[0] = READ_COORD();
		dl->origin[1] = READ_COORD();
		dl->origin[2] = READ_COORD();
		dl->radius = READ_COORD();
		dl->color.r = READ_BYTE();
		dl->color.g = READ_BYTE();
		dl->color.b = READ_BYTE();
	}
	return 1;
}