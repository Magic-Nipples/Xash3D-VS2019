#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"
#include "vgui_TeamFortressViewport.h"
#include "triangleapi.h"

#include "pm_defs.h"

DECLARE_MESSAGE( m_Lensflare, Lensflare )

int CHudLensflare::Init(void)
{
	HOOK_MESSAGE( Lensflare );

    m_iFlags |= HUD_ACTIVE;
    gHUD.AddHudElem(this);

    srand( (unsigned)time( NULL ) );

    return 1;
}

int CHudLensflare::VidInit(void)
{
	flFinalBlend, flFinalBlend2, flFinalBlend3, flFinalBlend4, flFinalBlend5, flFinalBlend6 = 0.0;
	flStartBlend, flStartBlend2, flStartBlend3, flStartBlend4, flStartBlend5, flStartBlend6 = 0.0;
	flPlayerBlend, flPlayerBlend2, flPlayerBlend3, flPlayerBlend4, flPlayerBlend5, flPlayerBlend6 = 0.0;

    return 1;
}

float CHudLensflare::SmoothValues(float startValue, float endValue, float finalValue, float speed)
{
	float absd, d;

	d = endValue - startValue;
	absd = fabs(d);

	if (absd > 0.01f)
	{
		if (d > 0)
			finalValue = startValue + (absd * speed);
		else
			finalValue = startValue - (absd * speed);
	}
	else
	{
		finalValue = endValue;
	}
	startValue = finalValue;

	return startValue;

	//gEngfuncs.Con_Printf("%f\n", startValue);
}

int CHudLensflare::MsgFunc_Lensflare(const char *pszName,  int iSize, void *pbuf)
{
    BEGIN_READ( pbuf, iSize );

	Sunanglex = READ_COORD();
	Sunangley = READ_COORD();
	SunEnabled = READ_BYTE();

	m_iFlags |= HUD_ACTIVE;
    return 1;
}

extern vec3_t v_angles, v_origin;

int CHudLensflare::Draw(float flTime)
{  
	vec3_t sunangles, sundir, suntarget;
	vec3_t v_forward, v_right, v_up, angles; 
	vec3_t forward, right, up, screen;  
	pmtrace_t tr;
	float speed = CVAR_GET_FLOAT("cl_sunflarespeed");

	if (SunEnabled == TRUE)
	{
		//Sun position
		if (Sunanglex != NULL && Sunangley != NULL)
		{
			sunangles.x = Sunanglex;
			sunangles.y = Sunangley;
		}
		else
		{
			sunangles.x = -45;
			sunangles.y = 0;
		}

		text[0] = SPR_Load("sprites/lens/lens1.spr");
		red[0] = green[0] = blue[0] = 1.0;
		scale[0] = 45;
		multi[0] = -0.45;

		text[1] = SPR_Load("sprites/lens/lens2.spr");
		red[1] = green[0] = blue[0] = 1.0;
		scale[1] = 25;
		multi[1] = 0.2;

		text[2] = SPR_Load("sprites/lens/glow1.spr");
		red[2] = 132 / 255;
		green[2] = 1.0;
		blue[2] = 153 / 255;
		scale[2] = 35;
		multi[2] = 0.3;

		text[3] = SPR_Load("sprites/lens/glow2.spr");
		red[3] = 1.0;
		green[3] = 164 / 255;
		blue[3] = 164 / 255;
		scale[3] = 40;
		multi[3] = 0.46;

		text[4] = SPR_Load("sprites/lens/lens3.spr");
		red[4] = 1.0;
		green[4] = 164 / 255;
		blue[4] = 164 / 255;
		scale[4] = 52;
		multi[4] = 0.5;

		text[5] = SPR_Load("sprites/lens/lens2.spr");
		red[5] = green[5] = blue[5] = 1.0;
		scale[5] = 31;
		multi[5] = 0.54;

		text[6] = SPR_Load("sprites/lens/lens2.spr");
		red[6] = 0.6;
		green[6] = 1.0;
		blue[6] = 0.6;
		scale[6] = 26;
		multi[6] = 0.64;

		text[7] = SPR_Load("sprites/lens/glow1.spr");
		red[7] = 0.5;
		green[7] = 1.0;
		blue[7] = 0.5;
		scale[7] = 20;
		multi[7] = 0.77;

		text[8] = SPR_Load("sprites/lens/lens2.spr");

		text[9] = SPR_Load("sprites/lens/lens1.spr");

		//flPlayerBlend = 0.0;
		//flPlayerBlend2 = 0.0;

		AngleVectors(v_angles, forward, null, null);

		AngleVectors(sunangles, sundir, null, null);

		suntarget = v_origin + sundir * 16384;

		tr = *(gEngfuncs.PM_TraceLine(v_origin, suntarget, PM_STUDIO_IGNORE, 2, -1));

		if (gEngfuncs.PM_PointContents(tr.endpos, null) == CONTENTS_SKY)
		{
			flPlayerBlend = max(DotProduct(forward, sundir) - 0.75, 0.0) * 3.8; //0.85, 0.0 ) * 6.8;
			if (flPlayerBlend > 1.0)
				flPlayerBlend = 1.0;

			flPlayerBlend4 = max(DotProduct(forward, sundir) - 0.80, 0.0) * 3.6; //0.90, 0.0 ) * 6.6;
			if (flPlayerBlend4 > 1.0)
				flPlayerBlend4 = 1.0;

			flPlayerBlend6 = max(DotProduct(forward, sundir) - 0.70, 0.0) * 3.7; //0.80, 0.0 ) * 6.7;
			if (flPlayerBlend6 > 1.0)
				flPlayerBlend6 = 1.0;

			flPlayerBlend2 = flPlayerBlend6 * 140.0;
			flPlayerBlend3 = flPlayerBlend * 190.0;
			flPlayerBlend5 = flPlayerBlend4 * 222.0; //giant glow that isn't the sun


			flFinalBlend = SmoothValues(flStartBlend, flPlayerBlend, flFinalBlend, gHUD.m_flTimeDelta * speed);
			flStartBlend = flFinalBlend;

			flFinalBlend2 = SmoothValues(flStartBlend2, flPlayerBlend2, flFinalBlend2, gHUD.m_flTimeDelta * speed);
			flStartBlend2 = flFinalBlend2;

			flFinalBlend3 = SmoothValues(flStartBlend3, flPlayerBlend3, flFinalBlend3, gHUD.m_flTimeDelta * speed);
			flStartBlend3 = flFinalBlend3;

			flFinalBlend4 = SmoothValues(flStartBlend4, flPlayerBlend4, flFinalBlend4, gHUD.m_flTimeDelta * speed);
			flStartBlend4 = flFinalBlend4;

			flFinalBlend5 = SmoothValues(flStartBlend5, flPlayerBlend5, flFinalBlend5, gHUD.m_flTimeDelta * speed);
			flStartBlend5 = flFinalBlend5;

			flFinalBlend6 = SmoothValues(flStartBlend6, flPlayerBlend6, flFinalBlend6, gHUD.m_flTimeDelta * speed);
			flStartBlend6 = flFinalBlend6;

			//gEngfuncs.Con_Printf("%f %f %f\n", flStartBlend, flPlayerBlend, flFinalBlend);
		}
		else
		{
			flFinalBlend = SmoothValues(flStartBlend, 0, flFinalBlend, gHUD.m_flTimeDelta * speed);
			flStartBlend = flFinalBlend;

			flFinalBlend2 = SmoothValues(flStartBlend2, 0, flFinalBlend2, gHUD.m_flTimeDelta * speed);
			flStartBlend2 = flFinalBlend2;

			flFinalBlend3 = SmoothValues(flStartBlend3, 0, flFinalBlend3, gHUD.m_flTimeDelta * speed);
			flStartBlend3 = flFinalBlend3;

			flFinalBlend4 = SmoothValues(flStartBlend4, 0, flFinalBlend4, gHUD.m_flTimeDelta * speed);
			flStartBlend4 = flFinalBlend4;

			flFinalBlend5 = SmoothValues(flStartBlend5, 0, flFinalBlend5, gHUD.m_flTimeDelta * speed);
			flStartBlend5 = flFinalBlend5;

			flFinalBlend6 = SmoothValues(flStartBlend6, 0, flFinalBlend6, gHUD.m_flTimeDelta * speed);
			flStartBlend6 = flFinalBlend6;
		}

		//if( gEngfuncs.PM_PointContents( tr.endpos, null ) == CONTENTS_SKY )
		{
			vec3_t normal,point,origin;

			gEngfuncs.GetViewAngles((float*)normal);
			AngleVectors(normal,forward,right,up);

			VectorCopy( tr.endpos, origin );

			gEngfuncs.pTriAPI->WorldToScreen( tr.endpos, screen );

			Suncoordx = XPROJECT( screen[ 0 ] );
			Suncoordy = YPROJECT( screen[ 1 ] ); 

			if (Suncoordx < XRES( -10 ) || Suncoordx > XRES( 650 ) || Suncoordy < YRES( -10 ) || Suncoordy > YRES( 490 )) 
				return 1;

			Screenmx = ScreenWidth/2;
			Screenmy = ScreenHeight/2;

			Sundistx = Screenmx - Suncoordx;
			Sundisty = Screenmy - Suncoordy;

			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(SPR_Load("sprites/lens/lensflare2.spr")) , 0);//use hotglow, or any other sprite for the texture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(1.0, 1.0 , 155/255.0, flFinalBlend2/355.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend2/355.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx + 190, Suncoordy + 190, 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx + 190, Suncoordy - 190, 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx - 190, Suncoordy - 190, 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx - 190, Suncoordy + 190, 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);


			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(SPR_Load("sprites/lens/glow2.spr")) , 0);//use hotglow, or any other sprite for the texture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(1.0, 1.0 , 1.0, flFinalBlend3/255.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend3/255.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx + 160, Suncoordy + 160, 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx + 160, Suncoordy - 160, 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx - 160, Suncoordy - 160, 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Suncoordx - 160, Suncoordy + 160, 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);


			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(SPR_Load("sprites/lens/glow3.spr")) , 0);//use hotglow, or any other sprite for the texture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(1.0, 1.0 , 1.0, flFinalBlend5/255.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend5/255.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(0, 0, 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(0, ScreenHeight, 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(ScreenWidth, ScreenHeight, 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(ScreenWidth, 0, 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

			int i = 1;
			Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
			Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0); //hotglow, or any other sprite for the texture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

			i++;
			Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
			Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0); //hotglow, or any other sprite for the texture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

			i++;
			Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
			Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0); //hotglow, or any other sprite for the texture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

			i++;
			Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
			Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0); //hotglow, or any other sprite for the texture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

			i++;
			Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
			Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0); //hotglow, or any other sprite for the texture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

			i++;
			Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
			Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0); //hotglow, or any other sprite for the texture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

			i++;
			Lensx[i] = (Suncoordx + (Sundistx * multi[i]));
			Lensy[i] = (Suncoordy + (Sundisty * multi[i]));
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0); //hotglow, or any other sprite for the texture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(red[i], green[i] , green[i], flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] + scale[i], 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] + scale[i], Lensy[i] - scale[i], 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] - scale[i], 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx[i] - scale[i], Lensy[i] + scale[i], 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

			i++;
			int scale1 = 32;
			int Lensx1,Lensy1 = 0;
			Lensx1 = (Suncoordx + (Sundistx * 0.88));
			Lensy1 = (Suncoordy + (Sundisty * 0.88));
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0); //hotglow, or any other sprite for the texture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(0.9, 0.9 , 0.9, flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 + scale1, Lensy1 + scale1, 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 + scale1, Lensy1 - scale1, 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 - scale1, Lensy1 - scale1, 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 - scale1, Lensy1 + scale1, 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);

			i++;
			scale1 = 130; //140
			Lensx1 = (Suncoordx + (Sundistx * 1.6)); //1.1 //scale of position relative to player angle against sun angle. higher is farther away from crosshair.
			Lensy1 = (Suncoordy + (Sundisty * 1.6)); //1.1
			gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *) gEngfuncs.GetSpritePointer(text[i]) , 0); //lens1.spr the one nearest the crosshair
			gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
			gEngfuncs.pTriAPI->Color4f(0.9, 0.9 , 0.9, flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Brightness(flFinalBlend2/255.0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 + scale1, Lensy1 + scale1, 0); //top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 + scale1, Lensy1 - scale1, 0); //bottom left
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 - scale1, Lensy1 - scale1, 0); //bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);gEngfuncs.pTriAPI->Vertex3f(Lensx1 - scale1, Lensy1 + scale1, 0); //top right
			gEngfuncs.pTriAPI->End(); //end our list of vertexes
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
		}
	}
	return 1;
}