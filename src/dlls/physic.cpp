//=========================================================
// physic.cpp - physic engine simulatenous
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"

#include <fcntl.h>
#include <direct.h>
#include <sys/stat.h>
#include <io.h>
#include <time.h>

#include <gl/gl.h>

CPhysic WorldPhysic;

// to avoid _ftol2 bug
extern "C" long _ftol( double ); 
//extern "C" long _ftol2( double x ) { return _ftol( x ); }

static int bodycount;	// just for debug

// Newton callbacks
static void pfnReadTree( void *inbuf, void *outbuf, unsigned int size )
{
	WorldPhysic.FReadFromMemory( (byte *)inbuf, outbuf, (long)size );
}

static void pfnSaveTree( void *f, const void* buffer, unsigned int size )
{
	fwrite( buffer, size, 1, (FILE *)f ); 
}

// debug shower
static void pfnDebugShowGeometryCollision( const NewtonBody *body, int vertexCount, const float *points, int id ) 
{    
	int	i = vertexCount - 1;
	Vector	p0, p1;

	p0.x = METER2INCH( points[i * 3 + 0] );
	p0.z = METER2INCH( points[i * 3 + 1] );
	p0.y = METER2INCH( points[i * 3 + 2] );

	for( i = 0; i < vertexCount; i++ )
	{
		p1.x = METER2INCH( points[i * 3 + 0] );
		p1.z = METER2INCH( points[i * 3 + 1] );
		p1.y = METER2INCH( points[i * 3 + 2] );
//#ifdef CLIENT_DLL 
		glVertex3fv( p0 );
		glVertex3fv( p1 );
//#endif 
 		p0 = p1;
 	}
} 

static void pfnDebugShowBodyCollision( const NewtonBody* body ) 
{ 
	NewtonBodyForEachPolygonDo( body, pfnDebugShowGeometryCollision ); 
}

static void pfnPrintStats( const NewtonBody* body ) 
{
	bodycount++;
}

static void pfnRemoveBody( const NewtonBody* m_pBody ) 
{
	WorldPhysic.RemoveBody( m_pBody );
}

static void pfnApplyForce( const NewtonBody *m_pBody ) 
{ 
	float	mass; 
	Vector	size; // unused, just stub 

	NewtonBodyGetMassMatrix( m_pBody, &mass, &size.x, &size.y, &size.z ); 

	Vector	torque( 0.0f, 0.0f, 0.0f );
	Vector	force( 0.0f, -9.8f * mass, 0.0f );

	NewtonBodyAddForce( m_pBody, force ); 
	NewtonBodySetTorque( m_pBody, torque );
}

static void pfnApplyTransform( const NewtonBody *m_pBody, const float* src )
{
	edict_t	*pEdict = (edict_t *)NewtonBodyGetUserData( m_pBody );

	if( !pEdict ) return;
	CBaseEntity *pObject = CBaseEntity::Instance( &pEdict->v );
	if( !pObject ) return;

	matrix4x4	m( (float *)src );
	m = m.NewtonToQuake();

	Vector origin = m.GetOrigin();
	Vector angles = m.GetAngles();

	// FIXME: detect studiomodels with reliable methods!
	if( *STRING( pObject->pev->model ) != '*' )
	{
		// stupid quake bug!!!
		angles[PITCH] = -angles[PITCH];
	}

	UTIL_SetOrigin( pObject->pev, origin );
	pObject->pev->angles = angles;

	NewtonBodyGetCentreOfMass( m_pBody, pObject->m_center );

	Vector	avel, vel;

	// keep forces an actual
	NewtonBodyGetOmega( m_pBody, avel );
	NewtonBodyGetVelocity( m_pBody, vel );
	NewtonBodyGetForce( m_pBody, pObject->m_vecForce );
	NewtonBodyGetTorque( m_pBody, pObject->m_vecTorque );

	pObject->pev->avelocity.x = METER2INCH( avel.x );
	pObject->pev->avelocity.y = METER2INCH( avel.z );
	pObject->pev->avelocity.z = METER2INCH( avel.y );
	pObject->pev->velocity.x = METER2INCH( vel.x );
	pObject->pev->velocity.y = METER2INCH( vel.z );
	pObject->pev->velocity.z = METER2INCH( vel.y );
}

static void pfnApplyTransformPlayer(const NewtonBody* m_pBody, const float* src)
{
	edict_t* pEdict = (edict_t*)NewtonBodyGetUserData(m_pBody);

	if (!pEdict) return;
	CBaseEntity* pObject = CBaseEntity::Instance(&pEdict->v);
	if (!pObject) return;

	matrix4x4	m((float*)src);
	m = m.NewtonToQuake();

	Vector origin = m.GetOrigin();
	Vector angles = m.GetAngles();

	// FIXME: detect studiomodels with reliable methods!
	if (*STRING(pObject->pev->model) != '*')
	{
		// stupid quake bug!!!
		angles[PITCH] = -angles[PITCH];
	}

	UTIL_SetOrigin(pObject->pev, origin);
	pObject->pev->angles = angles;

	NewtonBodyGetCentreOfMass(m_pBody, pObject->m_center);

	Vector	avel, vel;

	// keep forces an actual
	NewtonBodyGetOmega(m_pBody, avel);
	NewtonBodyGetVelocity(m_pBody, vel);
	NewtonBodyGetForce(m_pBody, pObject->m_vecForce);
	NewtonBodyGetTorque(m_pBody, pObject->m_vecTorque);

	pObject->pev->avelocity.x = METER2INCH(avel.x);
	pObject->pev->avelocity.y = METER2INCH(avel.z);
	pObject->pev->avelocity.z = METER2INCH(avel.y);
	pObject->pev->velocity.x = METER2INCH(vel.x);
	pObject->pev->velocity.y = METER2INCH(vel.z);
	pObject->pev->velocity.z = METER2INCH(vel.y);
}

/*
====================
AngleQuaternion
====================
*/
static void AngleQuaternion( const Vector &angles, Vector4D &quat )
{
	float sr, sp, sy, cr, cp, cy;

	SinCos( angles.z * 0.5f, &sy, &cy );
	SinCos( angles.y * 0.5f, &sp, &cp );
	SinCos( angles.x * 0.5f, &sr, &cr );

	float srXcp = sr * cp, crXsp = cr * sp;
	quat.x = srXcp * cy - crXsp * sy; // X
	quat.y = crXsp * cy + srXcp * sy; // Y

	float crXcp = cr * cp, srXsp = sr * sp;
	quat.z = crXcp * sy - srXsp * cy; // Z
	quat.w = crXcp * cy + srXsp * sy; // W (real component)
}

void CPhysic :: InitPhysic( void )
{
	if( m_pWorld )
	{
		ALERT( at_error, "InitPhysic: world already exist\n" );
		return;
	}

	// initialize NewtonWorld
	m_pWorld = NewtonCreate( NULL, NULL );
}

void CPhysic :: FreePhysic( void )
{
	if( !m_pWorld ) return;

	FreeAllBodies();
	NewtonDestroyBody( m_pWorld, m_pWorldBody );

	NewtonDestroy( m_pWorld );
	m_pWorldBody = NULL;
	m_pWorld = NULL;
}

void CPhysic :: FreeAllBodies( void )
{
	if( !m_pWorld ) return;

	NewtonWorldForEachBodyDo( m_pWorld, pfnRemoveBody );
}

void CPhysic :: Update( float flTime )
{
	if( !m_pWorld ) return;

	bodycount = 0;

	static float flAccumTime = 0.0f;
	static float flStepTime = (1.0f / 120.0f); //60.0f;

	if( flTime < 0.01f )
	{
		flAccumTime += flTime;

		// clamp up very high FPS
		if( flAccumTime > flStepTime )
		{
			NewtonUpdate( m_pWorld, flStepTime );
			flAccumTime -= flStepTime;
		}
	}
	else
	{
		NewtonUpdate( m_pWorld, flTime );
	}
	//DebugDraw();
#ifdef _DEBUG
	NewtonWorldForEachBodyDo( m_pWorld, pfnPrintStats );
	ALERT( at_console, "Total Active Bodies %i\n", bodycount );
#endif
}

NewtonCollision *CPhysic :: CollisionFromBmodel( entvars_t *pev, int modelindex )
{
	if( modelindex < 1 || modelindex > numbmodels )
	{
		ALERT( at_console, "CollisionFromEntity: entity %i has invalid brush model\n", ENTINDEX( ENT( pev ))); 
		return NULL;
	}
	
	return bmodels[modelindex];
}

void CPhysic :: StudioCalcBoneQuaterion( mstudiobone_t *pbone, mstudioanim_t *panim, Vector4D &q )
{
	mstudioanimvalue_t *panimvalue;
	Vector angle;

	for( int j = 0; j < 3; j++ )
	{
		if( panim->offset[j+3] == 0 )
		{
			angle[j] = pbone->value[j+3]; // default;
		}
		else
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j+3]);
			angle[j] = panimvalue[1].value;
			angle[j] = pbone->value[j+3] + angle[j] * pbone->scale[j+3];
		}
	}

	AngleQuaternion( angle, q );
}

void CPhysic :: StudioCalcBonePosition( mstudiobone_t *pbone, mstudioanim_t *panim, Vector &pos )
{
	mstudioanimvalue_t *panimvalue;

	for( int j = 0; j < 3; j++ )
	{
		pos[j] = pbone->value[j]; // default;

		if( panim->offset[j] != 0 )
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);
			pos[j] += panimvalue[1].value * pbone->scale[j];
		}
	}
}

NewtonCollision *CPhysic :: CollisionFromStudio( entvars_t *pev, int modelindex )
{
	studiohdr_t *phdr = (studiohdr_t *)GET_MODEL_PTR( ENT( pev ));

	if( !phdr || phdr->numbones < 1 )
	{
		ALERT( at_error, "CollisionFromStudio: invalid studiomodel\n" );
		return NULL;
	}

	// compute default pose for building mesh from
	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)phdr + phdr->seqindex);
	mstudioseqgroup_t *pseqgroup = (mstudioseqgroup_t *)((byte *)phdr + phdr->seqgroupindex) + pseqdesc->seqgroup;

	if( pseqdesc->seqgroup != 0 )
	{
		ALERT( at_error, "CollisionFromStudio: bad sequence group (must be 0)\n" );
		return NULL;
	}

	// compute the model bbox because Newton relies on this for mass compute
	UTIL_SetSize( pev, pseqdesc[0].bbmin, pseqdesc[0].bbmax );

	mstudioanim_t *panim = (mstudioanim_t *)((byte *)phdr + pseqgroup->data + pseqdesc->animindex);
	mstudiobone_t *pbone = (mstudiobone_t *)((byte *)phdr + phdr->boneindex);
	static Vector pos[MAXSTUDIOBONES];
	static Vector4D q[MAXSTUDIOBONES];

	for( int i = 0; i < phdr->numbones; i++, pbone++, panim++ ) 
	{
		StudioCalcBoneQuaterion( pbone, panim, q[i] );
		StudioCalcBonePosition( pbone, panim, pos[i] );
	}

	pbone = (mstudiobone_t *)((byte *)phdr + phdr->boneindex);

	matrix4x4	transform, bonematrix, bonetransform[MAXSTUDIOBONES];

	// NOTE: rotate only in this order for right output!
	transform.Identity();
	transform.ConcatRotate( -180, 1, 0, 0 );
	transform.ConcatRotate( 180, 0, 0, 1 );

	// compute bones for default anim
	for( int i = 0; i < phdr->numbones; i++ ) 
	{
		// initialize bonematrix
		bonematrix = matrix3x4( pos[i], q[i] );

		if( pbone[i].parent == -1 ) 
			bonetransform[i] = transform.ConcatTransforms( bonematrix );
		else bonetransform[i] = bonetransform[pbone[i].parent].ConcatTransforms( bonematrix );
	}

	mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((byte *)phdr + phdr->bodypartindex);
	mstudiomodel_t *psubmodel = (mstudiomodel_t *)((byte *)phdr + pbodypart->modelindex) + pev->body; //magic nipples - added pev->body to support body groups
	Vector *pstudioverts = (Vector *)((byte *)phdr + psubmodel->vertindex);
	Vector *verts = new Vector[psubmodel->numverts];	// allocate temporary vertices array
	byte *pvertbone = ((byte *)phdr + psubmodel->vertinfoindex);
	Vector tmp;

	// setup all the vertices
	for( int i = 0; i < psubmodel->numverts; i++ )
	{
		tmp = bonetransform[pvertbone[i]].VectorTransform( pstudioverts[i] );

		// scale vertices
		verts[i].x = -INCH2METER( tmp.x );
		verts[i].y = INCH2METER( tmp.y );
		verts[i].z = INCH2METER( tmp.z );
	}

	NewtonCollision *pCol;

	// NOTE: newton has the internal vertex sorting, so we don't need make it here

	ALERT( at_console, "Create hull for model %s, allocate %i vertices\n", phdr->name, psubmodel->numverts );
	pCol = NewtonCreateConvexHull( m_pWorld, psubmodel->numverts, (float *)&verts[0][0], 12, NULL );

	delete [] verts;

	return pCol;
}

NewtonCollision *CPhysic :: CollisionFromEntity( CBaseEntity *pObject )
{
	if( !pObject ) return NULL;

	// check for bspmodel
	const char *model = STRING( pObject->pev->model );

	// no model
	if( !model || !*model )
		return NULL;

	//ALERT(at_console, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!FUCKME!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

	if( *model == '*' )
	{
		return CollisionFromBmodel( pObject->pev, atoi( model + 1 ));
	}
	else if( GET_MODEL_PTR( ENT( pObject->pev )))
	{
		return CollisionFromStudio( pObject->pev, pObject->pev->modelindex );
	}

	ALERT( at_error, "CollisionFromEntity: entity %i has invalid/unsupported model %s\n", pObject->entindex(), model ); 

	return NULL;
}

NewtonBody *CPhysic :: CreateBodyFromEntity( CBaseEntity *pObject )
{
	NewtonCollision *pCollision = CollisionFromEntity( pObject );
	if( !pCollision ) return NULL;

	matrix4x4	m( pObject->pev->origin, pObject->pev->angles );
	m = m.QuakeToNewton();

	// convert size to physic;
	Vector	size;
	
	size.x = INCH2METER( pObject->pev->size.x );
	size.y = INCH2METER( pObject->pev->size.y );
	size.z = INCH2METER( pObject->pev->size.z );

	NewtonBody *m_pBody = NewtonCreateBody( m_pWorld, pCollision );
	NewtonBodySetUserData( m_pBody, pObject->edict() );

	// mass based on brushsize
	NewtonBodySetMassMatrix( m_pBody, DotProduct( size, size ) * 9.8f, size.x, size.y, size.z ); // 10 kg

	// setup transform callbacks
	NewtonBodySetTransformCallback( m_pBody, pfnApplyTransform );
	NewtonBodySetForceAndTorqueCallback( m_pBody, pfnApplyForce );

	// move body into final position		
	NewtonBodySetMatrix( m_pBody, m );

	return m_pBody;
}

NewtonBody* CPhysic::CreateBodyFromStaticEntity(CBaseEntity* pObject)
{
	NewtonCollision* pCollision = CollisionFromEntity(pObject);
	if (!pCollision) return NULL;

	matrix4x4	m(pObject->pev->origin, pObject->pev->angles);
	m = m.QuakeToNewton();

	NewtonBody* m_pBody = NewtonCreateBody(m_pWorld, pCollision);
	NewtonBodySetUserData(m_pBody, pObject->edict());

	// move body into final position		
	NewtonBodySetMatrix(m_pBody, m);

	return m_pBody;
}

NewtonBody* CPhysic::CreateBodyForPlayer(CBaseEntity* pObject)
{
	NewtonCollision* pCollision = CollisionFromEntity(pObject);
	if (!pCollision) return NULL;

	matrix4x4	m(pObject->pev->origin, pObject->pev->angles);
	m = m.QuakeToNewton();

	// convert size to physic;
	Vector	size;

	size.x = INCH2METER(pObject->pev->size.x);
	size.y = INCH2METER(pObject->pev->size.y);
	size.z = INCH2METER(pObject->pev->size.z);

	NewtonBody* m_pBody = NewtonCreateBody(m_pWorld, pCollision);
	NewtonBodySetUserData(m_pBody, pObject->edict());

	// mass based on brushsize
	NewtonBodySetMassMatrix(m_pBody, DotProduct(size, size) * 9.8f, size.x, size.y, size.z); // 10 kg

	// setup transform callbacks
	NewtonBodySetTransformCallback(m_pBody, pfnApplyTransformPlayer);
	NewtonBodySetForceAndTorqueCallback(m_pBody, pfnApplyForce);

	// move body into final position		
	NewtonBodySetMatrix(m_pBody, m);

	return m_pBody;
}

NewtonBody *CPhysic :: RestoreBody( CBaseEntity *pObject )
{
	NewtonCollision *pCollision = CollisionFromEntity( pObject );
	if( !pCollision ) return NULL;

	// convert size to physic;
	Vector	size;
	
	size.x = INCH2METER( pObject->pev->size.x );
	size.y = INCH2METER( pObject->pev->size.y );
	size.z = INCH2METER( pObject->pev->size.z );

	NewtonBody *m_pBody = NewtonCreateBody( m_pWorld, pCollision );
	NewtonBodySetUserData( m_pBody, pObject->edict() );

	// mass based on brushsize
	NewtonBodySetMassMatrix( m_pBody, DotProduct( size, size ) * 9.8f, size.x, size.y, size.z ); // 10 kg

	// setup transform callbacks
	NewtonBodySetTransformCallback( m_pBody, pfnApplyTransform );
	NewtonBodySetForceAndTorqueCallback( m_pBody, pfnApplyForce );

	Vector angles = pObject->pev->angles;

	// FIXME: detect studiomodels with reliable methods!
	if( *STRING( pObject->pev->model ) != '*' )
          {
		// stupid quake bug!!!
		angles[PITCH] = -angles[PITCH];
          }

	matrix4x4	m( pObject->pev->origin, angles );
	m = m.QuakeToNewton();

	// move body into final position		
	NewtonBodySetMatrix( m_pBody, m );

	Vector	vel, avel;

	avel.x = INCH2METER( pObject->pev->avelocity.x );
	avel.z = INCH2METER( pObject->pev->avelocity.y );
	avel.y = INCH2METER( pObject->pev->avelocity.z );
	vel.x = INCH2METER( pObject->pev->velocity.x );
	vel.z = INCH2METER( pObject->pev->velocity.y );
	vel.y = INCH2METER( pObject->pev->velocity.z );

	// restore all forces
	NewtonBodySetOmega( m_pBody, avel );
	NewtonBodySetVelocity( m_pBody, vel );
	NewtonBodySetForce( m_pBody, pObject->m_vecForce );
	NewtonBodySetTorque( m_pBody, pObject->m_vecTorque );
	NewtonBodySetCentreOfMass( m_pBody, pObject->m_center );

	return m_pBody;
}

NewtonBody* CPhysic::RestoreStaticBody(CBaseEntity* pObject)
{
	NewtonCollision* pCollision = CollisionFromEntity(pObject);
	if (!pCollision) return NULL;

	// convert size to physic;
	Vector	size;

	size.x = INCH2METER(pObject->pev->size.x);
	size.y = INCH2METER(pObject->pev->size.y);
	size.z = INCH2METER(pObject->pev->size.z);

	NewtonBody* m_pBody = NewtonCreateBody(m_pWorld, pCollision);
	NewtonBodySetUserData(m_pBody, pObject->edict());

	Vector angles = pObject->pev->angles;

	// FIXME: detect studiomodels with reliable methods!
	if (*STRING(pObject->pev->model) != '*')
	{
		// stupid quake bug!!!
		angles[PITCH] = -angles[PITCH];
	}

	matrix4x4	m(pObject->pev->origin, angles);
	m = m.QuakeToNewton();

	// move body into final position		
	NewtonBodySetMatrix(m_pBody, m);

	return m_pBody;
}
	
void CPhysic :: RemoveBody( edict_t *pEdict )
{
	if( !pEdict || pEdict->free ) return;
	CBaseEntity *pOther = (CBaseEntity *)CBaseEntity::Instance( &pEdict->v );

	if( !m_pWorld || !pOther || !pOther->m_pBody )
		return;

	NewtonDestroyBody( m_pWorld, pOther->m_pBody );
	pOther->m_pBody = NULL;
}

void CPhysic :: RemoveBody( const NewtonBody *pBody )
{
	// don't delete world
	if( !m_pWorld || !pBody || pBody == m_pWorldBody )
		return;

	NewtonDestroyBody( m_pWorld, pBody );
}

void CPhysic :: SetOrigin( CBaseEntity *pEntity, const Vector &origin )
{
	if( !pEntity || !pEntity->m_pBody )
		return;

	matrix4x4	m;

	NewtonBodyGetMatrix( pEntity->m_pBody, m );
	m = m.NewtonToQuake();
	m.SetOrigin( origin );
	m = m.QuakeToNewton();
	NewtonBodySetMatrix( pEntity->m_pBody, m );
}

void CPhysic :: SetVelocity( CBaseEntity *pEntity, const Vector &velocity )
{
	if( !pEntity || !pEntity->m_pBody )
		return;

	NewtonWorldUnfreezeBody(m_pWorld, pEntity->m_pBody); //magic nipples - unfreeze any objects when adjusting velocity

	Vector	vel;

	vel.x = INCH2METER( velocity.x );
	vel.y = INCH2METER( velocity.z );
	vel.z = INCH2METER( velocity.y );

	NewtonBodySetVelocity( pEntity->m_pBody, vel );
}

void CPhysic::SetOmega(CBaseEntity* pEntity, const Vector& omega)
{
	if (!pEntity || !pEntity->m_pBody)
		return;

	NewtonBodySetOmega(pEntity->m_pBody, omega);
}

void CPhysic::Freeze(CBaseEntity* pEntity)
{
	if (!pEntity || !pEntity->m_pBody)
		return;

	NewtonWorldFreezeBody(m_pWorld, pEntity->m_pBody);
}

int CPhysic :: CheckBINFile ( char *szMapName )
{
	if( !szMapName || !*szMapName )
		return 0;

	BOOL 	retValue;

	char	szBspFilename[MAX_PATH];
	char	szHullFilename[MAX_PATH];

	strcpy ( szBspFilename, "maps/" );
	strcat ( szBspFilename, szMapName );
	strcat ( szBspFilename, ".bsp" );

	strcpy ( szHullFilename, "maps/hulls/" );
	strcat ( szHullFilename, szMapName );
	strcat ( szHullFilename, ".bin" );
	
	retValue = TRUE;

	int iCompare;
	if ( COMPARE_FILE_TIME( szBspFilename, szHullFilename, &iCompare ))
	{
		if( iCompare > 0 )
		{
			// BSP file is newer.
			ALERT ( at_console, ".BIN file will be updated\n\n" );
			retValue = FALSE;
		}
	}
	else
	{
		retValue = FALSE;
	}

	return retValue;
}

int CPhysic :: FReadFromMemory( byte *inbuf, void *outbuf, long size )
{
	if ( size == 0 ) return 1;
	if ( !inbuf ) return 0;

	size_t	read_size = 0;

	// check for enough room
	if( m_iOffset >= m_iLength )
	{
		return 0; // hit EOF
	}
	if( m_iOffset + size <= m_iLength )
	{
		memcpy( outbuf, inbuf + m_iOffset, size );
		m_iOffset += size;
		read_size = size;
	}
	else
	{
		size_t reduced_size = m_iLength - m_iOffset;
		memcpy( outbuf, inbuf + m_iOffset, reduced_size );
		m_iOffset += reduced_size;
		read_size = reduced_size;
		ALERT( at_warning, "ReadFromMemory: buffer is out\n" );
	}

	return read_size;
}

int CPhysic :: FLoadTree( char *szMapName )
{
	if( !szMapName || !*szMapName )
		return 0;

	if( m_fLoaded )
	{
		if( !stricmp( szMapName, m_szMapName ))
		{
			// stay on same map - no reload
			return 1;
		}

		if( m_pWorldBody )
			NewtonDestroyBody( m_pWorld, m_pWorldBody );
		m_pWorldBody = NULL;

		m_fLoaded = FALSE; // trying to load new collision tree 
	}

	// make sure the directories have been made
	char	szDirName[MAX_PATH];
	char	szFilename[MAX_PATH];

	// save off mapname
	strcpy ( m_szMapName, szMapName );
	
	GET_GAME_DIR( szDirName );
	strcat( szDirName, "/maps" );
	CreateDirectory( szDirName, NULL );
	strcat( szDirName, "/hulls" );
	CreateDirectory( szDirName, NULL );

	strcpy( szFilename, "maps/hulls/" );
	strcat( szFilename, szMapName );
	strcat( szFilename, ".bin" );

	m_pHullBuffer = LOAD_FILE_FOR_ME( szFilename, &m_iLength );
	if( !m_pHullBuffer ) return 0;
	m_iOffset = 0; // reset file offset

	ALERT( at_console, "Load collision tree from %s\n", szFilename );
	m_pWorldTree = NewtonCreateTreeCollisionFromSerialization( m_pWorld, NULL, pfnReadTree, m_pHullBuffer );
	FREE_FILE( m_pHullBuffer );
	m_pHullBuffer = NULL;

	return 1;
}

int CPhysic :: FSaveTree( char *szMapName )
{
	if( !szMapName || !*szMapName || !m_pWorldTree )
		return 0;

	// make sure the directories have been made
	char	szFilename[MAX_PATH];

	// make sure directories have been made
	GET_GAME_DIR( szFilename );
	strcat( szFilename, "/maps" );
	CreateDirectory( szFilename, NULL );
	strcat( szFilename, "/hulls" );
	CreateDirectory( szFilename, NULL );

	strcat( szFilename, "/" );
	strcat( szFilename, szMapName );
	strcat( szFilename, ".bin" );

	m_pHullFile = fopen( szFilename, "wb" );

	if( !m_pHullFile )
	{
		ALERT( at_error, "*failed to save collision tree for map %s\n", szMapName );
		return 0;
	}

	NewtonTreeCollisionSerialize( m_pWorldTree, pfnSaveTree, m_pHullFile );
	fclose( m_pHullFile );
	m_pHullFile = NULL;

	return 1;
}

void CPhysic :: SetupWorld( void )
{
	if( m_pWorldBody )
		return; // already loaded

	if( !m_pWorldTree )
	{
		ALERT( at_error, "*collision tree not ready!\n" );
		return;
	}

	Vector	boxP0, boxP1;
	Vector	extra( 10.0f, 10.0f, 10.0f ); 
	matrix4x4	gWorldMatrix;

	m_pWorldBody = NewtonCreateBody( m_pWorld, m_pWorldTree );
	NewtonBodyGetMatrix( m_pWorldBody, gWorldMatrix );	// set the global position of this body 
	NewtonCollisionCalculateAABB( m_pWorldTree, gWorldMatrix, boxP0, boxP1 ); 
	NewtonReleaseCollision( m_pWorld, m_pWorldTree );
	m_pWorldTree = NULL;

	// add some paddings to world size
	boxP0 = boxP0 - extra;
	boxP1 = boxP1 + extra;

	NewtonSetWorldSize( m_pWorld, boxP0, boxP1 ); 
	NewtonSetSolverModel( m_pWorld, 0 );
	NewtonSetFrictionModel( m_pWorld, 0 );
	m_fLoaded = true;

	// release BSP file when no longer used
	FreeBSPFile();
}

void CPhysic :: LoadVertexes( const lump_t *l )
{
	dvertex_t	*in;
	Vector	*out;

	in = (dvertex_t *)(mod_base + l->fileofs);
	m_iNumVerts = l->filelen / sizeof( *in );
	m_pVerts = out = new Vector[m_iNumVerts];	

	for( int i = 0; i < m_iNumVerts; i++, in++, out++ )
	{
		out->x = in->point[0];
		out->y = in->point[1];
		out->z = in->point[2];
	}
}

void CPhysic :: LoadEdges( const lump_t *l )
{
	dedge_t *in;

	in = (dedge_t *)(mod_base + l->fileofs);
	m_iNumEdges = l->filelen / sizeof( *in );
	m_pEdges = new dedge_t[m_iNumEdges];

	memcpy( m_pEdges, in, l->filelen );
}

void CPhysic :: LoadSurfedges( const lump_t *l )
{
	int	*in;

	in = (int *)(mod_base + l->fileofs);
	m_iNumSurfEdges = l->filelen / sizeof( *in );
	m_pSurfedges = new int[m_iNumSurfEdges];

	memcpy( m_pSurfedges, in, l->filelen );
}

int CPhysic :: TextureContents( const char *name )
{
	if( !strnicmp( name, "sky", 3 ))
		return CONTENTS_SKY;
	if( !strnicmp( name+1,"!lava", 5 ))
		return CONTENTS_LAVA;
	if( !strnicmp( name+1, "!slime", 6 ))
		return CONTENTS_SLIME;
	if( !strnicmp( name, "!cur_90", 7 ))
		return CONTENTS_CURRENT_90;
	if( !strnicmp( name, "!cur_0", 6 ))
		return CONTENTS_CURRENT_0;
	if( !strnicmp( name, "!cur_270", 8 ))
		return CONTENTS_CURRENT_270;
	if( !strnicmp( name, "!cur_180", 8 ))
		return CONTENTS_CURRENT_180;
	if( !strnicmp( name, "!cur_up", 7 ))
		return CONTENTS_CURRENT_UP;
	if( !strnicmp( name, "!cur_dwn", 8 ))
		return CONTENTS_CURRENT_DOWN;
	if( name[0] == '!' )
		return CONTENTS_WATER;
	if( !strnicmp( name, "origin", 6 ))
		return CONTENTS_ORIGIN;	// ignore these brushes
	if( !strnicmp( name, "clip", 4 ))
		return CONTENTS_CLIP;
	if( !strnicmp( name, "translucent", 11 ))
		return CONTENTS_TRANSLUCENT;
	if( name[0] == '@' )
		return CONTENTS_TRANSLUCENT;
	return CONTENTS_SOLID;
}

void CPhysic :: LoadTextures( const lump_t *l )
{
	miptex_t		*mt;
	dmiptexlump_t	*m;

	if( !l->filelen )
	{
		ALERT( at_warning, "BuildCollision: %s failed to load face contents\n", m_szMapName );
		return;
	}

	m = (dmiptexlump_t *)(mod_base + l->fileofs);
	m_iNumFaceContents = m->nummiptex;
	m_pFaceContents = new int[m_iNumFaceContents];

	for( int i = 0; i < m->nummiptex; i++ )
	{
		if( m->dataofs[i] == -1 ) continue; // bad texture or somewhat

		mt = (miptex_t *)((byte *)m + m->dataofs[i]);

		// determine contents by name
		m_pFaceContents[i] = TextureContents( mt->name );
	}
}

void CPhysic :: LoadTexinfo( const lump_t *l )
{
	texinfo_t	*in;
	int	*out;

	in = (texinfo_t *)(mod_base + l->fileofs );
	m_iNumTexinfo = l->filelen / sizeof( *in );
	m_pTexInfos = out = new int[m_iNumTexinfo];	

	for( int i = 0; i < m_iNumTexinfo; i++, in++, out++ )
	{
		if( in->miptex >= 0 && in->miptex < m_iNumFaceContents )
			*out = in->miptex; // keep texture number only
		else *out = 0;
	}
}

void CPhysic :: LoadFaces( const lump_t *l )
{
	dface_t	*in;

	in = (dface_t *)(mod_base + l->fileofs );
	m_iNumFaces = l->filelen / sizeof( *in );
	m_pFaces = new dface_t[m_iNumFaces];	

	memcpy( m_pFaces, in, l->filelen );
}

void CPhysic :: LoadSubmodels( const lump_t *l )
{
	dmodel_t	*in;

	in = (dmodel_t *)(mod_base + l->fileofs );
	m_iNumModels = l->filelen / sizeof( *in );
	m_pModels = new dmodel_t[m_iNumModels];	

	memcpy( m_pModels, in, l->filelen );
}

void CPhysic :: GetMapVertex( int index, Vector &out, int bmodel )
{
	if( index < 0 || index > m_iNumSurfEdges )
	{
		ALERT( at_error, "invalid surfedge number %d\n", index );
		out = g_vecZero;	// assume error
		return;
	}

	int edge_index = m_pSurfedges[index];
	int vert_index;

	if( edge_index > 0 )
		vert_index = m_pEdges[edge_index].v[0];
	else vert_index = m_pEdges[-edge_index].v[1];
	Vector in = m_pVerts[vert_index];

	out.x = INCH2METER( in.x );

	// rotate and scale vertex
	if( bmodel )
	{
		out.y = INCH2METER( in.y );
		out.z = INCH2METER( in.z );
	}
	else
	{
		out.y = INCH2METER( in.z );
		out.z = INCH2METER( in.y );
	}
}

void CPhysic :: AddCollisionFace( int facenum )
{
	if( facenum < 0 || facenum >= m_pModels->numfaces )
	{
		ALERT( at_error, "invalid face number %d\n", facenum );
		return;
	}
          
	dface_t *psurf = m_pFaces + facenum;

	if( psurf->numedges < 3 )
	{
		ALERT( at_error, "AddCollisionFace: degenerate face number %d (%i verts)\n", facenum, psurf->numedges );
		return;
	}

	int iContents = m_pFaceContents[m_pTexInfos[psurf->texinfo]];

	// NOTE: tune uncollidable contents by taste
	switch( iContents )
	{
	case CONTENTS_LAVA:
	case CONTENTS_SLIME:
	case CONTENTS_WATER:
		return;	// will be replaced with byouancy plane 
	default: break;
	}

	// create a temp vertices array
	Vector *verts = new Vector[psurf->numedges];

	for( int j = 0; j < psurf->numedges; j++ )
	{
		int e = m_pSurfedges[psurf->firstedge+j];
		int v = (e > 0) ? m_pEdges[e].v[0] : m_pEdges[-e].v[1];
		verts[j].x = INCH2METER( m_pVerts[v].x );
		verts[j].y = INCH2METER( m_pVerts[v].z );
		verts[j].z = INCH2METER( m_pVerts[v].y );
	}

	// add a polygon into collision tree
	NewtonTreeCollisionAddFace( m_pWorldTree, psurf->numedges, (float *)&verts[0][0], 12, 1 );

	// delete a temp vertices array
	delete [] verts;
}

int CPhysic :: LoadBSPFile( const char *szMapName )
{
	char	szFilename[MAX_PATH];

	strcpy ( szFilename, "maps/" );
	strcat ( szFilename, szMapName );
	strcat( szFilename, ".bsp" );

	int iLength;
	byte *buffer = LOAD_FILE_FOR_ME( (char *)szFilename, &iLength );
	if( !buffer )
	{
		ALERT( at_error, "BuildCollision: can't loading map %s\n", szFilename );
		return 0;
	}

	dheader_t	*header;

	header = (dheader_t *)buffer;
	if( header->version != 30 )
	{
		ALERT( at_error, "BuildCollision: %s it's not a Half-Life map\n", m_szMapName );
		FREE_FILE( buffer );
		return 0;
	}

	// setup mod_base
	mod_base = (byte *)header;

	// load into heap
	LoadVertexes( &header->lumps[LUMP_VERTEXES] );
	LoadEdges( &header->lumps[LUMP_EDGES] );
	LoadSurfedges( &header->lumps[LUMP_SURFEDGES] );
	LoadTextures( &header->lumps[LUMP_TEXTURES] );
	LoadTexinfo( &header->lumps[LUMP_TEXINFO] );
	LoadFaces( &header->lumps[LUMP_FACES] );
	LoadSubmodels( &header->lumps[LUMP_MODELS] );

	return 1;
}

void CPhysic :: FreeBSPFile( void )
{
	if( !mod_base ) return;

	numbmodels = 1;

	// create bmodels collisions before release level
	for( int modelNum = 1; modelNum < m_iNumModels; modelNum++ )
	{
		dmodel_t *bmodel = &m_pModels[modelNum];

		int numVerts = 0, totalVerts = 0;
		dface_t *psurf;
		Vector *verts;
		int i, j, k;

		// compute vertexes count
		psurf = &m_pFaces[bmodel->firstface];
		for( i = 0; i < bmodel->numfaces; i++, psurf++ )
			totalVerts += psurf->numedges;

		// create a temp vertices array
		verts = new Vector[totalVerts];

		psurf = &m_pFaces[bmodel->firstface];
		for( i = 0; i < bmodel->numfaces; i++, psurf++ )
		{
			for( j = 0; j < psurf->numedges; j++ )
			{
				int e = m_pSurfedges[psurf->firstedge+j];
				int v = (e > 0) ? m_pEdges[e].v[0] : m_pEdges[-e].v[1];
				Vector pos = m_pVerts[v];

				// scale into newton coords
				pos.x = INCH2METER( pos.x );
				pos.y = INCH2METER( pos.y );
				pos.z = -INCH2METER( pos.z );

				// check for duplicate vertexes
				for( k = 0; k < numVerts; k++ )
				{
					if( verts[k] == pos )
						break; // already exist
				}

				// add unique vertex into array
				if( k == numVerts ) verts[numVerts++] = pos;
			}
		}

		if( bmodels[modelNum] )
		{
			// release old collision
			NewtonReleaseCollision( m_pWorld, bmodels[modelNum] );
			bmodels[modelNum] = NULL;
		}

		// NOTE: for some reasons physic engine produce degenerate convex hulls for many duplicated vertices
		// so we need filter them manually
		ALERT( at_console, "Create hull for model *%i, allocate %i/%i vertices\n", modelNum, numVerts, totalVerts );

		if(numVerts != 0) //magic nipples - clip, null, aaatrigger cause a crash because the faces are deleted. This is a partial fix till I can add in clipnodes.
			bmodels[modelNum] = NewtonCreateConvexHull( m_pWorld, numVerts, (float *)&verts[0][0], 12, NULL );

		delete [] verts;

		numbmodels++;
	}

	ALERT( at_console, "Total %i models stored\n", numbmodels );

	delete [] m_pVerts;
	delete [] m_pEdges;
	delete [] m_pFaces;
	delete [] m_pModels;
	delete [] m_pTexInfos;
	delete [] m_pSurfedges;
	delete [] m_pFaceContents;

	FREE_FILE( mod_base ); // no reason to keep this data
	mod_base = NULL;
}
	
int CPhysic :: BuildCollisionTree( char *szMapName )
{
	if( !szMapName || !*szMapName )
	{
		ALERT( at_error, "BuildCollision: invalid map %s\n", szMapName );
		return 0;
	}

	if( !LoadBSPFile( szMapName ))
		return 0;

	if( m_pWorldTree )
	{
		ALERT( at_error, "m_pWorldTree != NULL\n" );
		NewtonReleaseCollision( m_pWorld, m_pWorldTree );
	}

	// invalidate worldbody
	if( m_pWorldBody )
		NewtonDestroyBody( m_pWorld, m_pWorldBody );
	m_pWorldBody = NULL;

	// save off mapname
	strcpy ( m_szMapName, szMapName );

	// begin building collision tree
	ALERT( at_console, "Tree Collision out of Date. Rebuilding...\n" );
	m_pWorldTree = NewtonCreateTreeCollision( m_pWorld, NULL );
	NewtonTreeCollisionBeginBuild( m_pWorldTree );

	// collision options
	m_fOptimizeCollision = TRUE;

	for( int i = 0; i < m_pModels->numfaces; i++ )
		AddCollisionFace( m_pModels->firstface + i );

	NewtonTreeCollisionEndBuild( m_pWorldTree, m_fOptimizeCollision );

	// store collision tree onto the disk to avoid rebuild it again
	FSaveTree( szMapName );

	return 1;
}

void CPhysic :: DebugDraw( void  ) 
{ 
//#ifdef CLIENT_DLL
	glDisable( GL_TEXTURE_2D ); 
	glColor3f( 1, 0.7f, 0 );
	glBegin( GL_LINES );

	NewtonWorldForEachBodyDo( m_pWorld, pfnDebugShowBodyCollision ); 

	glEnd(); 
	glEnable( GL_TEXTURE_2D );
//#endif
}

////////////// SIMPLE PHYSIC ENTITY //////////////

class CPhysEntity : public CBaseEntity
{
public:
	void Precache( void )
	{
		PRECACHE_MODEL( (char *)STRING( pev->model ));
	}
	void Spawn( void )
	{
		Precache();

		pev->movetype = MOVETYPE_NONE;	// can'be apply velocity but ignore collisions
		pev->solid = SOLID_CUSTOM;		// not solidity

		SET_MODEL( ENT( pev ), STRING( pev->model ));
		UTIL_SetOrigin( pev, pev->origin );

		// motor!
		m_pBody = WorldPhysic.CreateBodyFromEntity( this );
	}
	virtual int IsRigidBody( void )
	{
		return TRUE;
	} 
};
LINK_ENTITY_TO_CLASS( prop_physics, CPhysEntity );


class CPhysBmodel : public CBaseEntity
{
public:
	void Spawn(void)
	{
		pev->movetype = MOVETYPE_NONE;	// can'be apply velocity but ignore collisions
		pev->solid = SOLID_CUSTOM;		// not solidity

		SET_MODEL(ENT(pev), STRING(pev->model));
		UTIL_SetOrigin(pev, pev->origin);

		// motor!
		m_pBody = WorldPhysic.CreateBodyFromEntity(this);
	}
	virtual int IsRigidBody(void)
	{
		return TRUE;
	}
};
LINK_ENTITY_TO_CLASS(func_physbox, CPhysBmodel);