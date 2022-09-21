// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 2022 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2022 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  spb.c
/// \brief Self Propelled Bomb item code.

#include "../doomdef.h"
#include "../doomstat.h"
#include "../info.h"
#include "../k_kart.h"
#include "../k_objects.h"
#include "../m_random.h"
#include "../p_local.h"
#include "../r_main.h"
#include "../s_sound.h"
#include "../g_game.h"
#include "../z_zone.h"
#include "../k_waypoint.h"
#include "../k_respawn.h"

//#define SPB_SEEKTEST

#define SPB_SLIPTIDEDELTA (ANG1 * 3)
#define SPB_STEERDELTA (ANGLE_90 - ANG10)
#define SPB_DEFAULTSPEED (FixedMul(mapobjectscale, K_GetKartSpeedFromStat(5) * 2))

enum
{
	SPB_MODE_SEEK,
	SPB_MODE_CHASE,
	SPB_MODE_WAIT,
};

#define spb_mode(o) ((o)->extravalue1)
#define spb_modetimer(o) ((o)->extravalue2)

#define spb_nothink(o) ((o)->threshold)
#define spb_lastplayer(o) ((o)->lastlook)
#define spb_speed(o) ((o)->movefactor)
#define spb_pitch(o) ((o)->movedir)

#define spb_curwaypoint(o) ((o)->cusval)

#define spb_owner(o) ((o)->target)
#define spb_chase(o) ((o)->tracer)

static void SpawnSPBTrailRings(mobj_t *spb)
{
	if (leveltime % (spb_mode(spb) != SPB_MODE_SEEK ? 6 : 3) == 0)
	{
		mobj_t *ring = P_SpawnMobjFromMobj(spb,
			-FixedDiv(spb->momx, spb->scale),
			-FixedDiv(spb->momy, spb->scale),
			-FixedDiv(spb->momz, spb->scale) + (24*FRACUNIT),
			MT_RING
		);

		ring->threshold = 10;
		ring->fuse = 35*TICRATE;

		ring->colorized = true;
		ring->color = SKINCOLOR_RED;
	}
}

static void SpawnSPBDust(mobj_t *spb)
{
	// The easiest way to spawn a V shaped cone of dust from the SPB is simply to spawn 2 particles, and to both move them to the sides in opposite direction.
	mobj_t *dust;
	fixed_t sx;
	fixed_t sy;
	fixed_t sz = spb->floorz;
	angle_t sa = spb->angle - ANG1*60;
	INT32 i;

	if (spb->eflags & MFE_VERTICALFLIP)
	{
		sz = spb->ceilingz;
	}

	if ((leveltime & 1) && abs(spb->z - sz) < FRACUNIT*64) // Only every other frame. Also don't spawn it if we're way above the ground.
	{
		// Determine spawning position next to the SPB:
		for (i = 0; i < 2; i++)
		{
			sx = 96 * FINECOSINE(sa >> ANGLETOFINESHIFT);
			sy = 96 * FINESINE(sa >> ANGLETOFINESHIFT);

			dust = P_SpawnMobjFromMobj(spb, sx, sy, 0, MT_SPBDUST);
			dust->z = sz;

			dust->momx = spb->momx/2;
			dust->momy = spb->momy/2;
			dust->momz = spb->momz/2; // Give some of the momentum to the dust

			P_SetScale(dust, spb->scale * 2);

			dust->color = SKINCOLOR_RED;
			dust->colorized = true;

			dust->angle = spb->angle - FixedAngle(FRACUNIT*90 - FRACUNIT*180*i); // The first one will spawn to the right of the spb, the second one to the left.
			P_Thrust(dust, dust->angle, 6*dust->scale);

			K_MatchGenericExtraFlags(dust, spb);

			sa += ANG1*120;	// Add 120 degrees to get to mo->angle + ANG1*60
		}
	}
}

// Spawns SPB slip tide. To be used when the SPB is turning.
// Modified version of K_SpawnAIZDust. Maybe we could merge those to be cleaner?

// dir should be either 1 or -1 to determine where to spawn the dust.

static void SpawnSPBSliptide(mobj_t *spb, INT32 dir)
{
	fixed_t newx;
	fixed_t newy;
	mobj_t *spark;
	angle_t travelangle;
	fixed_t sz = spb->floorz;

	if (spb->eflags & MFE_VERTICALFLIP)
	{
		sz = spb->ceilingz;
	}

	travelangle = K_MomentumAngle(spb);

	if ((leveltime & 1) && abs(spb->z - sz) < FRACUNIT*64)
	{
		newx = P_ReturnThrustX(spb, travelangle - (dir*ANGLE_45), 24*FRACUNIT);
		newy = P_ReturnThrustY(spb, travelangle - (dir*ANGLE_45), 24*FRACUNIT);

		spark = P_SpawnMobjFromMobj(spb, newx, newy, 0, MT_SPBDUST);
		spark->z = sz;

		spark->colorized = true;
		spark->color = SKINCOLOR_RED;

		spark->flags = MF_NOGRAVITY|MF_PAIN;
		P_SetTarget(&spark->target, spb);

		spark->angle = travelangle + (dir * ANGLE_90);
		P_SetScale(spark, (spark->destscale = spb->scale*3/2));

		spark->momx = (6*spb->momx)/5;
		spark->momy = (6*spb->momy)/5;

		K_MatchGenericExtraFlags(spark, spb);
	}
}

// Used for seeking and when SPB is trailing its target from way too close!
static void SpawnSPBSpeedLines(mobj_t *spb)
{
	mobj_t *fast = P_SpawnMobjFromMobj(spb,
		P_RandomRange(-24, 24) * FRACUNIT,
		P_RandomRange(-24, 24) * FRACUNIT,
		(spb->info->height / 2) + (P_RandomRange(-24, 24) * FRACUNIT),
		MT_FASTLINE
	);

	P_SetTarget(&fast->target, spb);
	fast->angle = K_MomentumAngle(spb);

	fast->color = SKINCOLOR_RED;
	fast->colorized = true;

	K_MatchGenericExtraFlags(fast, spb);
}

static fixed_t SPBDist(mobj_t *a, mobj_t *b)
{
	return P_AproxDistance(P_AproxDistance(
		a->x - b->x,
		a->y - b->y),
		a->z - b->z
	);
}

static void SPBTurn(
	fixed_t destSpeed, angle_t destAngle,
	fixed_t *editSpeed, angle_t *editAngle,
	fixed_t lerp, SINT8 *returnSliptide)
{
	INT32 delta = destAngle - *editAngle;
	fixed_t dampen = FRACUNIT;

	// Slow down when turning; it looks better and makes U-turns not unfair
	dampen = FixedDiv((180 * FRACUNIT) - AngleFixed(abs(delta)), 180 * FRACUNIT);

	*editSpeed = FixedMul(destSpeed, dampen);

	delta = FixedMul(delta, lerp);

	// Calculate sliptide effect during seeking.
	if (returnSliptide != NULL)
	{
		INT32 sliptide = (abs(delta) > SPB_SLIPTIDEDELTA);

		if (delta < 0)
		{
			sliptide = -sliptide;
		}

		*returnSliptide = sliptide;
	}

	*editAngle += delta;
}

static void SetSPBSpeed(mobj_t *spb, fixed_t xySpeed, fixed_t zSpeed)
{
	spb->momx = FixedMul(FixedMul(
		xySpeed,
		FINECOSINE(spb->angle >> ANGLETOFINESHIFT)),
		FINECOSINE(spb_pitch(spb) >> ANGLETOFINESHIFT)
	);

	spb->momy = FixedMul(FixedMul(
		xySpeed,
		FINESINE(spb->angle >> ANGLETOFINESHIFT)),
		FINECOSINE(spb_pitch(spb) >> ANGLETOFINESHIFT)
	);

	spb->momz = FixedMul(
		zSpeed,
		FINESINE(spb_pitch(spb) >> ANGLETOFINESHIFT)
	);
}

static void SPBSeek(mobj_t *spb, player_t *bestPlayer)
{
	const fixed_t desiredSpeed = SPB_DEFAULTSPEED;

	waypoint_t *curWaypoint = NULL;
	waypoint_t *destWaypoint = NULL;

	fixed_t dist = INT32_MAX;

	fixed_t destX = spb->x;
	fixed_t destY = spb->y;
	fixed_t destZ = spb->z;
	angle_t destAngle = spb->angle;
	angle_t destPitch = 0U;

	fixed_t xySpeed = desiredSpeed;
	fixed_t zSpeed = desiredSpeed;
	SINT8 sliptide = 0;

	fixed_t steerDist = INT32_MAX;
	mobj_t *steerMobj = NULL;

	size_t i;

	spb_lastplayer(spb) = -1; // Just make sure this is reset

	if (bestPlayer == NULL
		|| bestPlayer->mo == NULL
		|| P_MobjWasRemoved(bestPlayer->mo) == true
		|| bestPlayer->mo->health <= 0
		|| (bestPlayer->respawn.state != RESPAWNST_NONE))
	{
		// No one there? Completely STOP.
		spb->momx = spb->momy = spb->momz = 0;

		if (bestPlayer == NULL)
		{
			spbplace = -1;
		}

		return;
	}

	// Found someone, now get close enough to initiate the slaughter...
	P_SetTarget(&spb_chase(spb), bestPlayer->mo);
	spbplace = bestPlayer->position;

	dist = SPBDist(spb, spb_chase(spb));

#ifdef SPB_SEEKTEST // Easy debug switch
	(void)dist;
#else
	if (dist <= (1024 * spb_chase(spb)->scale))
	{
		S_StartSound(spb, spb->info->attacksound);
		spb_mode(spb) = SPB_MODE_CHASE; // TARGET ACQUIRED
		spb_modetimer(spb) = 7*TICRATE;
		spb_speed(spb) = desiredSpeed;
		return;
	}
#endif

	// Move along the waypoints until you get close enough
	if (spb_curwaypoint(spb) == -1)
	{
		// Determine first waypoint.
		curWaypoint = K_GetBestWaypointForMobj(spb);
		spb_curwaypoint(spb) = (INT32)K_GetWaypointHeapIndex(curWaypoint);
	}
	else
	{
		curWaypoint = K_GetWaypointFromIndex( (size_t)spb_curwaypoint(spb) );
	}

	destWaypoint = bestPlayer->nextwaypoint;

	if (curWaypoint != NULL)
	{
		fixed_t waypointDist = INT32_MAX;
		fixed_t waypointRad = INT32_MAX;

		CONS_Printf("Moving towards waypoint... (%d)\n", K_GetWaypointID(curWaypoint));
		destX = curWaypoint->mobj->x;
		destY = curWaypoint->mobj->y;
		destZ = curWaypoint->mobj->z;

		waypointDist = R_PointToDist2(spb->x, spb->y, destX, destY) / mapobjectscale;
		waypointRad = max(curWaypoint->mobj->radius / mapobjectscale, DEFAULT_WAYPOINT_RADIUS);

		if (waypointDist <= waypointRad)
		{
			boolean pathfindsuccess = false;

			if (destWaypoint != NULL)
			{
				// Go to next waypoint.
				const boolean huntbackwards = false;
				const boolean useshortcuts  = K_GetWaypointIsShortcut(destWaypoint); // If the player is on a shortcut, use shortcuts. No escape.

				path_t pathtoplayer = {0};

				pathfindsuccess = K_PathfindToWaypoint(
					curWaypoint, destWaypoint,
					&pathtoplayer,
					useshortcuts, huntbackwards
				);

				if (pathfindsuccess == true)
				{
					if (pathtoplayer.numnodes > 1)
					{
						curWaypoint = (waypoint_t *)pathtoplayer.array[1].nodedata;
						CONS_Printf("NEW: Proper next waypoint (%d)\n", K_GetWaypointID(curWaypoint));
					}
					else if (destWaypoint->numnextwaypoints > 0)
					{
						curWaypoint = destWaypoint->nextwaypoints[0];
						CONS_Printf("NEW: Forcing next waypoint (%d)\n", K_GetWaypointID(curWaypoint));
					}
					else
					{
						curWaypoint = destWaypoint;
						CONS_Printf("NEW: Forcing destination (%d)\n", K_GetWaypointID(curWaypoint));
					}

					Z_Free(pathtoplayer.array);
				}
			}

			if (pathfindsuccess == true && curWaypoint != NULL)
			{
				// Update again
				spb_curwaypoint(spb) = (INT32)K_GetWaypointHeapIndex(curWaypoint);
				destX = curWaypoint->mobj->x;
				destY = curWaypoint->mobj->y;
				destZ = curWaypoint->mobj->z;
			}
			else
			{
				CONS_Printf("FAILURE, no waypoint (pathfind unsuccessful)\n");
				spb_curwaypoint(spb) = -1;
				destX = spb_chase(spb)->x;
				destY = spb_chase(spb)->y;
				destZ = spb_chase(spb)->z;
			}
		}
	}
	else
	{
		CONS_Printf("FAILURE, no waypoint (no initial waypoint)\n");
		spb_curwaypoint(spb) = -1;
		destX = spb_chase(spb)->x;
		destY = spb_chase(spb)->y;
		destZ = spb_chase(spb)->z;
	}

	destAngle = R_PointToAngle2(spb->x, spb->y, destX, destY);
	destPitch = R_PointToAngle2(0, spb->z, P_AproxDistance(spb->x - destX, spb->y - destY), destZ);

	SPBTurn(desiredSpeed, destAngle, &xySpeed, &spb->angle, FRACUNIT/8, &sliptide);
	SPBTurn(desiredSpeed, destPitch, &zSpeed, &spb_pitch(spb), FRACUNIT/8, NULL);

	SetSPBSpeed(spb, xySpeed, zSpeed);

	// see if a player is near us, if they are, try to hit them by slightly thrusting towards them, otherwise, bleh!
	steerDist = 1536 * mapobjectscale;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		fixed_t ourDist = INT32_MAX;
		INT32 ourDelta = INT32_MAX;

		if (playeringame[i] == false || players[i].spectator == true)
		{
			// Not in-game
			continue; 
		}

		if (players[i].mo == NULL || P_MobjWasRemoved(players[i].mo) == true)
		{
			// Invalid mobj
			continue;
		}

		ourDelta = AngleDelta(spb->angle, R_PointToAngle2(spb->x, spb->y, players[i].mo->x, players[i].mo->y));
		if (ourDelta > SPB_STEERDELTA)
		{
			// Check if the angle wouldn't make us LOSE speed.
			continue;
		}

		ourDist = R_PointToDist2(spb->x, spb->y, players[i].mo->x, players[i].mo->y);
		if (ourDist < steerDist)
		{
			steerDist = ourDist;
			steerMobj = players[i].mo; // it doesn't matter if we override this guy now.
		}
	}

	// different player from our main target, try and ram into em~!
	if (steerMobj != NULL && steerMobj != spb_chase(spb))
	{
		P_Thrust(spb, R_PointToAngle2(spb->x, spb->y, steerMobj->x, steerMobj->y), spb_speed(spb) / 4);
	}

	if (sliptide != 0)
	{
		// 1 if turning left, -1 if turning right.
		// Angles work counterclockwise, remember!
		SpawnSPBSliptide(spb, sliptide);
	}
	else
	{
		// if we're mostly going straight, then spawn the V dust cone!
		SpawnSPBDust(spb);
	}

	// Always spawn speed lines while seeking
	SpawnSPBSpeedLines(spb);

	// Spawn a trail of rings behind the SPB!
	SpawnSPBTrailRings(spb);
}

static void SPBChase(mobj_t *spb, player_t *bestPlayer)
{
	fixed_t baseSpeed = 0;
	fixed_t maxSpeed = 0;
	fixed_t desiredSpeed = 0;

	fixed_t range = INT32_MAX;
	fixed_t cx = 0, cy = 0;

	fixed_t dist = INT32_MAX;
	angle_t destAngle = spb->angle;
	angle_t destPitch = 0U;
	fixed_t xySpeed = 0;
	fixed_t zSpeed = 0;

	mobj_t *chase = NULL;
	player_t *chasePlayer = NULL;

	spb_curwaypoint(spb) = -1; // Reset waypoint

	chase = spb_chase(spb);

	if (chase == NULL || P_MobjWasRemoved(chase) == true || chase->health <= 0)
	{
		P_SetTarget(&spb_chase(spb), NULL);
		spb_mode(spb) = SPB_MODE_WAIT;
		spb_modetimer(spb) = 55; // Slightly over the respawn timer length
		return;
	}

	if (chase->hitlag)
	{
		// If the player is frozen, the SPB should be too.
		spb->hitlag = max(spb->hitlag, chase->hitlag);
		return;
	}

	baseSpeed = SPB_DEFAULTSPEED;
	range = (160 * chase->scale);

	// Play the intimidating gurgle
	if (S_SoundPlaying(spb, spb->info->activesound) == false)
	{
		S_StartSound(spb, spb->info->activesound);
	}

	// Maybe we want SPB to target an object later? IDK lol
	chasePlayer = chase->player;
	if (chasePlayer != NULL)
	{
		UINT8 fracmax = 32;
		UINT8 spark = ((10 - chasePlayer->kartspeed) + chasePlayer->kartweight) / 2;
		fixed_t easiness = ((chasePlayer->kartspeed + (10 - spark)) << FRACBITS) / 2;

		spb_lastplayer(spb) = chasePlayer - players; // Save the player num for death scumming...
		spbplace = chasePlayer->position;

		chasePlayer->pflags |= PF_RINGLOCK; // set ring lock

		if (P_IsObjectOnGround(chase) == false)
		{
			// In the air you have no control; basically don't hit unless you make a near complete stop
			baseSpeed = (7 * chasePlayer->speed) / 8;
		}
		else
		{
			// 7/8ths max speed for Knuckles, 3/4ths max speed for min accel, exactly max speed for max accel
			baseSpeed = FixedMul(
				((fracmax+1) << FRACBITS) - easiness,
				K_GetKartSpeed(chasePlayer, false, false)
			) / fracmax;
		}

		if (chasePlayer->carry == CR_SLIDING)
		{
			baseSpeed = chasePlayer->speed/2;
		}

		// Be fairer on conveyors
		cx = chasePlayer->cmomx;
		cy = chasePlayer->cmomy;

		// Switch targets if you're no longer 1st for long enough
		if (bestPlayer != NULL && chasePlayer->position <= bestPlayer->position)
		{
			spb_modetimer(spb) = 7*TICRATE;
		}
		else
		{
			if (spb_modetimer(spb) > 0)
			{
				spb_modetimer(spb)--;
			}

			if (spb_modetimer(spb) <= 0)
			{
				spb_mode(spb) = SPB_MODE_SEEK; // back to SEEKING
			}
		}
	}

	dist = P_AproxDistance(P_AproxDistance(spb->x - chase->x, spb->y - chase->y), spb->z - chase->z);

	desiredSpeed = FixedMul(baseSpeed, FRACUNIT + FixedDiv(dist - range, range));

	if (desiredSpeed < baseSpeed)
	{
		desiredSpeed = baseSpeed;
	}

	maxSpeed = (baseSpeed * 3) / 2;
	if (desiredSpeed > maxSpeed)
	{
		desiredSpeed = maxSpeed;
	}

	if (desiredSpeed < 20 * chase->scale)
	{
		desiredSpeed = 20 * chase->scale;
	}

	if (chasePlayer != NULL && chasePlayer->carry == CR_SLIDING)
	{
		// Hack for current sections to make them fair.
		desiredSpeed = min(desiredSpeed, chasePlayer->speed / 2);
	}

	destAngle = R_PointToAngle2(spb->x, spb->y, chase->x, chase->y);
	destPitch = R_PointToAngle2(0, spb->z, P_AproxDistance(spb->x - chase->x, spb->y - chase->y), chase->z);

	// Modify stored speed
	if (desiredSpeed > spb_speed(spb))
	{
		spb_speed(spb) += (desiredSpeed - spb_speed(spb)) / TICRATE;
	}
	else
	{
		spb_speed(spb) = desiredSpeed;
	}

	SPBTurn(spb_speed(spb), destAngle, &xySpeed, &spb->angle, FRACUNIT, NULL);
	SPBTurn(spb_speed(spb), destPitch, &zSpeed, &spb_pitch(spb), FRACUNIT, NULL);

	SetSPBSpeed(spb, xySpeed, zSpeed);
	spb->momx += cx;
	spb->momy += cy;

	// Spawn a trail of rings behind the SPB!
	SpawnSPBTrailRings(spb);

	// Red speed lines for when it's gaining on its target. A tell for when you're starting to lose too much speed!
	if (R_PointToDist2(0, 0, spb->momx, spb->momy) > (16 * R_PointToDist2(0, 0, chase->momx, chase->momy)) / 15 // Going faster than the target
		&& xySpeed > 20 * mapobjectscale) // Don't display speedup lines at pitifully low speeds
	{
		SpawnSPBSpeedLines(spb);
	}
}

static void SPBWait(mobj_t *spb)
{
	player_t *oldPlayer = NULL;

	spb->momx = spb->momy = spb->momz = 0; // Stoooop
	spb_curwaypoint(spb) = -1; // Reset waypoint

	if (spb_lastplayer(spb) != -1
		&& playeringame[spb_lastplayer(spb)] == true)
	{
		oldPlayer = &players[spb_lastplayer(spb)];
	}

	if (oldPlayer != NULL
		&& oldPlayer->spectator == false
		&& oldPlayer->exiting > 0)
	{
		spbplace = oldPlayer->position;
		oldPlayer->pflags |= PF_RINGLOCK;
	}

	if (spb_modetimer(spb) > 0)
	{
		spb_modetimer(spb)--;
	}

	if (spb_modetimer(spb) <= 0)
	{
		if (oldPlayer != NULL)
		{
			if (oldPlayer->mo != NULL && P_MobjWasRemoved(oldPlayer->mo) == false)
			{
				P_SetTarget(&spb_chase(spb), oldPlayer->mo);
				spb_mode(spb) = SPB_MODE_CHASE;
				spb_modetimer(spb) = 7*TICRATE;
				spb_speed(spb) = SPB_DEFAULTSPEED;
			}
		}
		else
		{
			spb_mode(spb) = SPB_MODE_SEEK;
			spb_modetimer(spb) = 0;
			spbplace = -1;
		}
	}
}

void Obj_SPBThink(mobj_t *spb)
{
	mobj_t *ghost = NULL;
	player_t *bestPlayer = NULL;
	UINT8 bestRank = UINT8_MAX;
	size_t i;

	if (spb->health <= 0)
	{
		return;
	}

	indirectitemcooldown = 20*TICRATE;

	ghost = P_SpawnGhostMobj(spb);
	ghost->fuse = 3;

	if (spb_owner(spb) != NULL && P_MobjWasRemoved(spb_owner(spb)) == false && spb_owner(spb)->player != NULL)
	{
		ghost->color = spb_owner(spb)->player->skincolor;
		ghost->colorized = true;
	}

	if (spb_nothink(spb) > 0)
	{
		// Doesn't think yet, when it initially spawns.
		spb_lastplayer(spb) = -1;
		spb_curwaypoint(spb) = -1;
		spbplace = -1;

		P_InstaThrust(spb, spb->angle, SPB_DEFAULTSPEED);

		spb_nothink(spb)--;
	}
	else
	{
		// Find the player with the best rank
		for (i = 0; i < MAXPLAYERS; i++)
		{
			player_t *player = NULL;

			if (playeringame[i] == false)
			{
				// Not valid
				continue;
			}

			player = &players[i];

			if (player->spectator == true || player->exiting > 0)
			{
				// Not playing
				continue;
			}

			/*
			if (player->mo == NULL || P_MobjWasRemoved(player->mo) == true)
			{
				// No mobj
				continue;
			}

			if (player->mo <= 0)
			{
				// Dead
				continue;
			}

			if (player->respawn.state != RESPAWNST_NONE)
			{
				// Respawning
				continue;
			}
			*/

			if (player->position < bestRank)
			{
				bestRank = player->position;
				bestPlayer = player;
			}
		}

		switch (spb_mode(spb))
		{
			case SPB_MODE_SEEK:
			default:
				SPBSeek(spb, bestPlayer);
				break;

			case SPB_MODE_CHASE:
				SPBChase(spb, bestPlayer);
				break;

			case SPB_MODE_WAIT:
				SPBWait(spb);
				break;
		}
	}

	// Clamp within level boundaries.
	if (spb->z < spb->floorz)
	{
		spb->z = spb->floorz;
	}
	else if (spb->z > spb->ceilingz - spb->height)
	{
		spb->z = spb->ceilingz - spb->height;
	}
}
