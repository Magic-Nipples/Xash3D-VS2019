/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#define _NEWTON_USE_LIB

#define METERS_PER_INCH	0.0254f
#define METER2INCH( x )	(float)( x * ( 1.0f / METERS_PER_INCH ))
#define INCH2METER( x )	(float)( x * ( METERS_PER_INCH / 1.0f ))

#define RAD2DEG( x )	((float)(x) * (float)(180.f / M_PI))
#define DEG2RAD( x )	((float)(x) * (float)(M_PI / 180.f))

#include "newton.h"
#include "bspfile.h"

#include "vector.h"
#include "matrix.h"
#include "studio.h"

//=========================================================
// physic.h
//=========================================================
class CPhysic
{
private:
	FILE		*m_pHullFile;

	byte		*m_pHullBuffer;
	int		m_iLength;
	int		m_iOffset;
	char		m_szMapName[256];
	BOOL		m_fLoaded;	// collision tree is loaded and actual

	NewtonWorld	*m_pWorld;	// pointer to Newton world
	NewtonCollision	*m_pWorldTree;	// pointer to collision tree
	NewtonBody	*m_pWorldBody;

	byte		*mod_base;
	Vector		*m_pVerts;
	dedge_t		*m_pEdges;
	dface_t		*m_pFaces;
	dmodel_t		*m_pModels;
	int		*m_pTexInfos;
	int		*m_pFaceContents;	// keep contents for each texture
	int		*m_pSurfedges;

	int		m_iNumVerts;
	int		m_iNumEdges;
	int		m_iNumFaces;
	int		m_iNumModels;
	int		m_iNumTexinfo;
	int		m_iNumFaceContents;
	int		m_iNumSurfEdges;
public:
	BOOL		m_fOptimizeCollision;	// optimize collision produce simplifed collision file

	NewtonCollision	*bmodels[MAX_MAP_MODELS];
	int		numbmodels;

	void		InitPhysic( void );
	void		FreePhysic( void );
	void		Update( float flTime );
	void		RemoveBody( edict_t *pEdict );
	void		RemoveBody( const NewtonBody *pBody );

	NewtonBody* CreateBodyFromEntity(CBaseEntity* pEntity);
	NewtonBody* CreateBodyFromStaticEntity(CBaseEntity* pEntity);
	NewtonBody* CreateBodyForPlayer(CBaseEntity* pEntity);

	NewtonBody* RestoreBody(CBaseEntity* pEntity);
	NewtonBody* RestoreStaticBody(CBaseEntity* pEntity);

	void		SetOrigin(CBaseEntity* pEntity, const Vector& origin);
	void		SetVelocity(CBaseEntity* pEntity, const Vector& velocity);
	void		SetOmega(CBaseEntity* pEntity, const Vector& omega);
	void		Freeze(CBaseEntity* pEntity);

	int		FLoadTree(char* szMapName);
	int		FSaveTree(char* szMapName);
	int		CheckBINFile(char* szMapName);
	int		BuildCollisionTree(char* szMapName);
	int		FReadFromMemory(byte* handle, void* buffer, long size);
	void	SetupWorld(void);
	void	DebugDraw(void);		// FIXME: add project to client
	int		LoadBSPFile(const char* szFilename);
	void	FreeBSPFile(void);
	void	FreeAllBodies(void);

private:
	// physics bsp loader
	void	LoadVertexes( const lump_t *l );
	void	LoadEdges( const lump_t *l );
	void	LoadSurfedges( const lump_t *l );
	void	LoadTextures( const lump_t *l );
	void	LoadTexinfo( const lump_t *l );
	void	LoadFaces( const lump_t *l );
	void	LoadSubmodels( const lump_t *l );

	NewtonCollision	*CollisionFromEntity( CBaseEntity *pEntity );
	NewtonCollision	*CollisionFromBmodel( entvars_t *pev, int modelindex );
	NewtonCollision	*CollisionFromStudio( entvars_t *pev, int modelindex );

	void	StudioCalcBoneQuaterion( mstudiobone_t *pbone, mstudioanim_t *panim, Vector4D &q );
	void	StudioCalcBonePosition( mstudiobone_t *pbone, mstudioanim_t *panim, Vector &pos );

	// misc routines
	int	TextureContents( const char *name );
	void	GetMapVertex( int index, Vector &out, int bmodel );
	void	AddCollisionFace( int facenum );
};

extern CPhysic WorldPhysic;