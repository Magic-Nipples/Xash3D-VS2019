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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "triangleapi.h"

DECLARE_MESSAGE(m_Battery, Battery)

int CHudBattery::Init(void)
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;

	HOOK_MESSAGE(Battery);

	gHUD.AddHudElem(this);

	return 1;
};


int CHudBattery::VidInit(void)
{
	int HUD_suit_empty = gHUD.GetSpriteIndex( "suit_empty" );
	int HUD_suit_full = gHUD.GetSpriteIndex( "suit_full" );

	m_hSprite1 = m_hSprite2 = 0;  // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect( HUD_suit_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_suit_full );
	m_iHeight = m_prc2->bottom - m_prc1->top;
	m_fFade = 0;
	return 1;
};

int CHudBattery:: MsgFunc_Battery(const char *pszName,  int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	
	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();

	if (x != m_iBat)
	{
		m_fFade = 128;//FADE_TIME;
		m_iBat = x;
	}

	return 1;
}


int CHudBattery::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;

	int r, g, b, x, y, a;
	wrect_t rc;

	rc = *m_prc2;
	rc.top  += m_iHeight * ((float)(100-(min(100,m_iBat))) * 0.01);	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1

	UnpackRGB(r,g,b, RGB_YELLOWISH);

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	Vector2D offsetpoint;
	offsetpoint.x = 160;
	offsetpoint.y = ScreenHeight - 91;

	gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture);
	gEngfuncs.pTriAPI->SpriteTexture((struct model_s*) gEngfuncs.GetSpritePointer(SPR_Load("sprites/health_back.spr")), 0);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE); //no culling
	gEngfuncs.pTriAPI->Color4f(0.0, 0.0, 0.0, 1.0);
	gEngfuncs.pTriAPI->Brightness(1.0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f); gEngfuncs.pTriAPI->Vertex3f(offsetpoint.x, offsetpoint.y, 0); //top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f); gEngfuncs.pTriAPI->Vertex3f(offsetpoint.x, offsetpoint.y + 96, 0); //bottom left
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f); gEngfuncs.pTriAPI->Vertex3f(offsetpoint.x + 256, offsetpoint.y + 96, 0); //bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f); gEngfuncs.pTriAPI->Vertex3f(offsetpoint.x + 256, offsetpoint.y, 0); //top right
	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

	// Has health changed? Flash the health #
	if (m_fFade)
	{
		m_fFade -= (gHUD.m_flTimeDelta * 60); //20 speed of fade
		if (m_fFade <= 0)
		{
			a = MIN_ALPHA;
			m_fFade = 0;
		}

		// Fade the health number back to dim
		a = MIN_ALPHA +  (m_fFade/FADE_TIME) * 128;

	}
	else
		a = MIN_ALPHA;

	ScaleColors(r, g, b, a );

	x = 270;
	y = ScreenHeight - 68;
	x = gHUD.DrawHl2HudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b);
	
	/*
	int iOffset = (m_prc1->bottom - m_prc1->top)/6;

	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	x = ScreenWidth/5;

	// make sure we have the right sprite handles
	if ( !m_hSprite1 )
		m_hSprite1 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_empty" ) );
	if ( !m_hSprite2 )
		m_hSprite2 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_full" ) );

	SPR_Set(m_hSprite1, r, g, b );
	SPR_DrawAdditive( 0,  x, y - iOffset, m_prc1);

	if (rc.bottom > rc.top)
	{
		SPR_Set(m_hSprite2, r, g, b );
		SPR_DrawAdditive( 0, x, y - iOffset + (rc.top - m_prc2->top), &rc);
	}

	x += (m_prc1->right - m_prc1->left);
	x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b);
	*/

	return 1;
}