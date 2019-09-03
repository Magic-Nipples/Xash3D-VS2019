//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

//LRC - the fogging fog
#include <windows.h>
#include <gl/gl.h>

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"

//LRC - the fogging fog
#include "r_studioint.h"
extern float g_fFogColor[4];
extern float g_fStartDist;
extern float g_fFinalValue;
extern vec3_t FogColor;

//magic nipples - rain
#include "rain.h"
#include "com_model.h"
#include "studio_util.h"
#include "tri_rope.h" //magic nipples - ropes

// Buz Start
#include "windows.h"
#include "gl/gl.h"
#include "gl/glext.h"

#include "r_studioint.h"
extern engine_studio_api_t IEngineStudio;

#include "com_model.h"
#include "studio.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
extern CGameStudioModelRenderer g_StudioRenderer;

#define BIT( n )		(1<<( n ))
//#define	SURF_PLANEBACK		2
//#define	SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
//#define SURF_DRAWTURB		0x10
//#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40
//#define SURF_UNDERWATER		0x80
#define SURF_DONTWARP		0x100
#define BACKFACE_EPSILON	0.01

// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2
// Buz End

extern engine_studio_api_t IEngineStudio;

#define DLLEXPORT __declspec( dllexport )

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
};


void SetPoint(float x, float y, float z, float(*matrix)[4]) //magic nipples - rain
{
	vec3_t point, result;
	point[0] = x;
	point[1] = y;
	point[2] = z;

	VectorTransform(point, matrix, result);

	gEngfuncs.pTriAPI->Vertex3f(result[0], result[1], result[2]);
}

//#define TEST_IT
#if defined( TEST_IT )

/*
=================
Draw_Triangles

Example routine.  Draws a sprite offset from the player origin.
=================
*/
void Draw_Triangles( void )
{
	cl_entity_t *player;
	vec3_t org;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if ( !player )
		return;

	org = player->origin;

	org.x += 50;
	org.y += 50;

	if (gHUD.m_hsprCursor == 0)
	{
		char sz[256];
		sprintf( sz, "sprites/cursor.spr" );
		gHUD.m_hsprCursor = SPR_Load( sz );
	}

	if ( !gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( gHUD.m_hsprCursor ), 0 ))
	{
		return;
	}
	
	// Create a triangle, sigh
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	// Overload p->color with index into tracer palette, p->packedColor with brightness
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
	// UNDONE: This gouraud shading causes tracers to disappear on some cards (permedia2)
	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y, org.z );

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

#endif


//LRC - the fogging fog
void RenderFog ( void )
{
	float g_fFogColor[4] = { FogColor.x, FogColor.y, FogColor.z, 1.0 };
	bool bFog = g_fStartDist > 0 && g_fFinalValue > 0;
	if (bFog)
	{
		glEnable(GL_FOG);
		glFogi(GL_FOG_MODE, GL_LINEAR);
		glFogfv(GL_FOG_COLOR, g_fFogColor);
		glFogf(GL_FOG_START, g_fStartDist);
		glFogf(GL_FOG_END, g_fFinalValue);
		glHint(GL_FOG_HINT, GL_NICEST);
	}
}

/*
=================================
DrawRain
draw raindrips and snowflakes
=================================
*/
extern cl_drip FirstChainDrip;
extern rain_properties Rain;

void DrawRain(void) //magic nipples - rain
{
	if (FirstChainDrip.p_Next == NULL)
		return; // no drips to draw

	HSPRITE_HL hsprTexture;
	const model_s* pTexture;
	float visibleHeight = Rain.globalHeight - SNOWFADEDIST;

	if (Rain.weatherMode == 0)
		hsprTexture = LoadSprite("gfx/fx/rain.spr"); // load rain sprite
	else
		hsprTexture = LoadSprite("gfx/fx/snow.spr"); // load snow sprite

	if (!hsprTexture) return;

	// usual triapi stuff
	pTexture = gEngfuncs.GetSpritePointer(hsprTexture);
	gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)pTexture, 0);
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);

	// go through drips list
	cl_drip* Drip = FirstChainDrip.p_Next;
	cl_entity_t* player = gEngfuncs.GetLocalPlayer();

	if (Rain.weatherMode == 0) // draw rain
	{
		while (Drip != NULL)
		{
			cl_drip* nextdDrip = Drip->p_Next;

			Vector2D toPlayer;
			toPlayer.x = player->origin[0] - Drip->origin[0];
			toPlayer.y = player->origin[1] - Drip->origin[1];
			toPlayer = toPlayer.Normalize();

			toPlayer.x *= DRIP_SPRITE_HALFWIDTH;
			toPlayer.y *= DRIP_SPRITE_HALFWIDTH;

			float shiftX = (Drip->xDelta / DRIPSPEED) * DRIP_SPRITE_HALFHEIGHT;
			float shiftY = (Drip->yDelta / DRIPSPEED) * DRIP_SPRITE_HALFHEIGHT;

			// --- draw triangle --------------------------
			gEngfuncs.pTriAPI->Color4f(Drip->alpha, Drip->alpha, Drip->alpha, 1.0); //gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, Drip->alpha);
			gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);

			gEngfuncs.pTriAPI->TexCoord2f(0, 0);
			gEngfuncs.pTriAPI->Vertex3f(Drip->origin[0] - toPlayer.y - shiftX, Drip->origin[1] + toPlayer.x - shiftY, Drip->origin[2] + DRIP_SPRITE_HALFHEIGHT);

			gEngfuncs.pTriAPI->TexCoord2f(0.5, 1);
			gEngfuncs.pTriAPI->Vertex3f(Drip->origin[0] + shiftX, Drip->origin[1] + shiftY, Drip->origin[2] - DRIP_SPRITE_HALFHEIGHT);

			gEngfuncs.pTriAPI->TexCoord2f(1, 0);
			gEngfuncs.pTriAPI->Vertex3f(Drip->origin[0] + toPlayer.y - shiftX, Drip->origin[1] - toPlayer.x - shiftY, Drip->origin[2] + DRIP_SPRITE_HALFHEIGHT);

			gEngfuncs.pTriAPI->End();
			// --- draw triangle end ----------------------

			Drip = nextdDrip;
		}
	}
	else	// draw snow
	{
		vec3_t normal;
		gEngfuncs.GetViewAngles((float*)normal);

		float matrix[3][4];
		AngleMatrix(normal, matrix);	// calc view matrix

		while (Drip != NULL)
		{
			cl_drip* nextdDrip = Drip->p_Next;

			matrix[0][3] = Drip->origin[0]; // write origin to matrix
			matrix[1][3] = Drip->origin[1];
			matrix[2][3] = Drip->origin[2];

			// apply start fading effect
			float alpha = (Drip->origin[2] <= visibleHeight) ? Drip->alpha : ((Rain.globalHeight - Drip->origin[2]) / (float)SNOWFADEDIST) * Drip->alpha;

			// --- draw quad --------------------------
			gEngfuncs.pTriAPI->Color4f(alpha, alpha, alpha, 1.0); //gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, alpha);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);

			gEngfuncs.pTriAPI->TexCoord2f(0, 0);
			SetPoint(0, SNOW_SPRITE_HALFSIZE, SNOW_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->TexCoord2f(0, 1);
			SetPoint(0, SNOW_SPRITE_HALFSIZE, -SNOW_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->TexCoord2f(1, 1);
			SetPoint(0, -SNOW_SPRITE_HALFSIZE, -SNOW_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->TexCoord2f(1, 0);
			SetPoint(0, -SNOW_SPRITE_HALFSIZE, SNOW_SPRITE_HALFSIZE, matrix);

			gEngfuncs.pTriAPI->End();
			// --- draw quad end ----------------------

			Drip = nextdDrip;
		}
	}
}

// Buz Start
void ClearBuffer(void);
extern bool g_bShadows;

mleaf_t* Mod_PointInLeaf(vec3_t p, model_t* model) // quake's func
{
	mnode_t* node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t*)node;
		mplane_t* plane = node->plane;
		float d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}

model_t* g_pworld;
int		g_visframe;
int		g_framecount;
myvec3_t	g_lightvec;

void RecursiveDrawWorld(mnode_t* node)
{
	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->visframe != g_visframe)
		return;

	if (node->contents < 0)
		return;		// faces already marked by engine

	// recurse down the children, Order doesn't matter
	RecursiveDrawWorld(node->children[0]);
	RecursiveDrawWorld(node->children[1]);

	// draw stuff
	int c = node->numsurfaces;
	if (c)
	{
		msurface_t* surf = g_pworld->surfaces + node->firstsurface;

		for (; c; c--, surf++)
		{
			if (surf->visframe != g_framecount)
				continue;

			if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB | SURF_UNDERWATER))
				continue;

			// cull from light vector

			float dot;
			mplane_t* plane = surf->plane;

			switch (plane->type)
			{
			case PLANE_X:
				dot = g_lightvec[0];	break;
			case PLANE_Y:
				dot = g_lightvec[1];	break;
			case PLANE_Z:
				dot = g_lightvec[2];	break;
			default:
				dot = DotProduct(g_lightvec, plane->normal); break;
			}

			if ((dot > 0) && (surf->flags & SURF_PLANEBACK))
				continue;

			if ((dot < 0) && !(surf->flags & SURF_PLANEBACK))
				continue;

			glpoly_t* p = surf->polys;
			float* v = p->verts[0];

			glBegin(GL_POLYGON);
			for (int i = 0; i < p->numverts; i++, v += VERTEXSIZE)
			{
				glTexCoord2f(v[3], v[4]);
				glVertex3fv(v);
			}
			glEnd();
		}
	}
} // Buz End

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;	// Buz

void DLLEXPORT HUD_DrawNormalTriangles( void )
{
	ClearBuffer();	// Buz

	gHUD.m_Spectator.DrawOverview();

	// Buz Start
	if (g_bShadows && IEngineStudio.IsHardware())
	{
		if (NULL == glActiveTextureARB)
			glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");

		glPushAttrib(GL_ALL_ATTRIB_BITS);

		// BUzer: workaround half-life's bug, when multitexturing left enabled after
		// rendering brush entities
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE0_ARB);

		glDepthMask(GL_FALSE);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_DST_COLOR, GL_ZERO);
		glColor4f(0.5, 0.5, 0.5, 1);

		glStencilFunc(GL_NOTEQUAL, 0, ~0);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glEnable(GL_STENCIL_TEST);

		// get current visframe number
		g_pworld = gEngfuncs.GetEntityByIndex(0)->model;
		mleaf_t* leaf = Mod_PointInLeaf(g_StudioRenderer.m_vRenderOrigin, g_pworld);
		g_visframe = leaf->visframe;

		// get current frame number
		g_framecount = g_StudioRenderer.m_nFrameCount;

		// get light vector
		g_StudioRenderer.GetShadowVector(g_lightvec);

		// draw world
		RecursiveDrawWorld(g_pworld->nodes);

		glPopAttrib();
	}

	g_bShadows = false;
	// Buz End
	
#if defined( TEST_IT )
//	Draw_Triangles();
#endif

}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
	ProcessRain();
	DrawRain();

	if (CVAR_GET_FLOAT("cl_drawropes")) //magic nipples - ropes
		gRopeRender.DrawRopes(1);

#if defined( TEST_IT )
//	Draw_Triangles();
#endif
}