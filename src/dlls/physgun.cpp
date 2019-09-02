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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

enum fishgun_e {
	PHYS_DRAW = 0,
	PHYS_IDLE,
	PHYS_HOLSTER,
	PHYS_SPIN,
	PHYS_HOLD,
};

class CPHISGUN : public CBasePlayerWeapon
{
	//DECLARE_CLASS( CPHISGUN, CBasePlayerWeapon );
	public:
		void Spawn( void );
		void Precache( void );

		int iItemSlot( void ) { return 1; }
		int GetItemInfo(ItemInfo *p);

		BOOL Deploy( void );
		//void Holster( int skiplocal );

		void PrimaryAttack( void );
		void SecondaryAttack( void );

		void CPHISGUN::WeaponIdle( void );

		void Grab( void );

		void ItemPostFrame( void );

		virtual BOOL UseDecrement( void )
		{ 
			return FALSE;
		}

		int inAttack1;
		float m_flNextPrimaryAttack1;
		float m_flNextSecondaryAttack1;
		float m_flTimeWeaponIdle1;

		virtual int		Save( CSave &save );
		virtual int		Restore( CRestore &restore );
		static	TYPEDESCRIPTION m_SaveData[];

	private:
		CBaseEntity *pCapture;
};

LINK_ENTITY_TO_CLASS( weapon_physgun, CPHISGUN );

TYPEDESCRIPTION	CPHISGUN::m_SaveData[] = 
{
	DEFINE_FIELD( CPHISGUN, m_flNextPrimaryAttack1, FIELD_TIME ),
	DEFINE_FIELD( CPHISGUN, m_flNextSecondaryAttack1, FIELD_TIME ),
	DEFINE_FIELD( CPHISGUN, m_flTimeWeaponIdle1, FIELD_TIME ),
	DEFINE_FIELD( CPHISGUN, inAttack1, FIELD_INTEGER ),
	DEFINE_FIELD( CPHISGUN, pCapture, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CPHISGUN, CBasePlayerWeapon );

void CPHISGUN::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_physgun"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_PHYSGUN;
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CPHISGUN::Precache( void )
{
	PRECACHE_MODEL("models/v_physgun.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");
}

int CPHISGUN::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_PHYSGUN;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}

BOOL CPHISGUN::Deploy( )
{
	m_flNextPrimaryAttack1 = 0;
	m_flNextSecondaryAttack1 = 0;
	m_flTimeWeaponIdle1 = 0;
	inAttack1 = 0;

	return DefaultDeploy( "models/v_physgun.mdl", "models/p_9mmhandgun.mdl", PHYS_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CPHISGUN::SecondaryAttack( void )
{
	//GlockFire( 0.1, 0.2, FALSE );
}

void CPHISGUN::PrimaryAttack( void )
{
	if(!(m_pPlayer->m_afButtonPressed & IN_ATTACK))
		return;

	if ( inAttack1 == 0 )
		inAttack1 = 1;

	if ( inAttack1 == 2 )
		inAttack1 = 0;

	SendWeaponAnim( PHYS_IDLE );
	m_flNextPrimaryAttack1 = gpGlobals->time + 0.2;
}

void CPHISGUN::Grab( void )
{
	TraceResult tr;
}

void CPHISGUN::WeaponIdle( void )
{
	//ALERT( at_console, "curtime: %f ptime: %f stime: %f itime: %f\n", gpGlobals->time, m_flNextPrimaryAttack1, m_flNextSecondaryAttack1, m_flTimeWeaponIdle1 );
	TraceResult tr;

	if ( inAttack1 == 1 )
	{
		edict_t *player = m_pPlayer->edict();

		UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
		Vector vecAiming = gpGlobals->v_forward;
		Vector vecSrc	 = m_pPlayer->GetGunPosition( );

		Vector vecDest = vecSrc + vecAiming * 2048;

		UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, player, &tr );

		if (tr.flFraction < 1.0)
		{
			pCapture = CBaseEntity::Instance( tr.pHit );

			if (pCapture->IsBSPModel())
			{
				inAttack1 = 0; return;
			}

			if( pCapture->pev->solid == SOLID_BSP)
			{
				inAttack1 = 0; return;
			}


			inAttack1 = 2;
			ALERT( at_console, "hit %s\n", STRING( pCapture->pev->classname ) );
			SendWeaponAnim( PHYS_SPIN );
		}
		else
		{
			inAttack1 = 0;
		}
	}
	if (inAttack1 == 2)
	{
		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
		Vector vecAiming = gpGlobals->v_forward;
		Vector vecSrc = m_pPlayer->GetGunPosition();

		Vector vecDest1 = vecSrc + vecAiming * 90;

		Vector velocityFinal = (vecDest1 - pCapture->pev->origin).Normalize() * ((vecDest1 - pCapture->pev->origin).Length() * 10);

		if ((vecDest1 - pCapture->pev->origin).Length() > 85)
			velocityFinal = (vecDest1 - pCapture->pev->origin).Normalize() * 550;


		WorldPhysic.SetVelocity(pCapture, velocityFinal);

		//UTIL_SetOrigin(pCapture->pev, vecDest1);
		//WorldPhysic.SetOrigin(pCapture, vecDest1);
	}
}

BOOL CanAttack1 ( float attack_time, float curtime, BOOL isPredicted )
{
#if defined( CLIENT_WEAPONS )
	if ( !isPredicted )
#else
	if ( 1 )
#endif
	{
		return ( attack_time <= curtime ) ? TRUE : FALSE;
	}
	else
	{
		return ( attack_time <= 0.0 ) ? TRUE : FALSE;
	}
}

void CPHISGUN::ItemPostFrame( void )
{
	if ((m_fInReload) && ( m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase() ) )
	{
		// complete the reload. 
		int j = min( iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);	

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;

		m_pPlayer->TabulateAmmo();

		m_fInReload = FALSE;
	}

	if ((m_pPlayer->pev->button & IN_ATTACK2) && CanAttack1( m_flNextSecondaryAttack1, gpGlobals->time, UseDecrement() ) )
	{
		if ( pszAmmo2() && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] )
		{
			m_fFireOnEmpty = TRUE;
		}

		m_pPlayer->TabulateAmmo();
		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) && CanAttack1( m_flNextPrimaryAttack1, gpGlobals->time, UseDecrement() ) )
	{
		if ( (m_iClip == 0 && pszAmmo1()) || (iMaxClip() == -1 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] ) )
		{
			m_fFireOnEmpty = TRUE;
		}

		m_pPlayer->TabulateAmmo();
		PrimaryAttack();
	}
	else if ( m_pPlayer->pev->button & IN_RELOAD && iMaxClip() != WEAPON_NOCLIP && !m_fInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}

	m_fFireOnEmpty = FALSE;

	if ( !IsUseable() && m_flNextPrimaryAttack1 < ( UseDecrement() ? 0.0 : gpGlobals->time ) ) 
	{
		// weapon isn't useable, switch.
		if ( !(iFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && g_pGameRules->GetNextBestWeapon( m_pPlayer, this ) )
		{
			m_flNextPrimaryAttack1 = ( UseDecrement() ? 0.0 : gpGlobals->time ) + 0.3;
			return;
		}
	}
	else
	{
		// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
		if ( m_iClip == 0 && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack1 < ( UseDecrement() ? 0.0 : gpGlobals->time ) )
		{
			Reload();
			return;
		}
	}

	WeaponIdle( );
	return;
	
	if ( ShouldWeaponIdle() )
	{
		WeaponIdle();
	}
}