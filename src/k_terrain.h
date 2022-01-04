// DR. ROBOTNIK'S RING RACERS
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2021 by ZDoom + GZDoom teams, and contributors
// Copyright (C) 2021 by Sally "TehRealSalt" Cochenour
// Copyright (C) 2021 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_terrain.h
/// \brief Implementation of a TERRAIN-style lump for DRRR, ala GZDoom's codebase.

#ifndef __K_TERRAIN_H__
#define __K_TERRAIN_H__

#include "doomdata.h"
#include "doomdef.h"
#include "doomtype.h"
#include "m_fixed.h"
#include "p_mobj.h"

#define TERRAIN_NAME_LEN 32

typedef struct t_splash_s
{
	// Splash definition.
	// These are particles spawned when hitting the floor.

	char name[TERRAIN_NAME_LEN];	// Lookup name.

	UINT16 mobjType;		// Thing type. MT_NULL to not spawn anything.
	UINT16 sfx;				// Sound to play.
	fixed_t scale;			// Thing scale multiplier.
	UINT16 color;			// Colorize effect. SKINCOLOR_NONE has no colorize.

	fixed_t pushH;			// Push-out horizontal multiplier.
	fixed_t pushV;			// Push-out vertical multiplier.
	fixed_t spread;			// Randomized spread distance.
	angle_t cone;			// Randomized angle of the push-out.

	UINT8 numParticles;		// Number of particles to spawn.
} t_splash_t;

typedef struct t_footstep_s
{
	// Footstep definition.
	// These are particles spawned when moving fast enough on a floor.

	char name[TERRAIN_NAME_LEN];	// Lookup name.

	UINT16 mobjType;		// Thing type. MT_NULL to not spawn anything.
	UINT16 sfx;				// Sound to play.
	fixed_t scale;			// Thing scale multiplier.
	UINT16 color;			// Colorize effect. SKINCOLOR_NONE has no colorize.

	fixed_t pushH;			// Push-out horizontal multiplier.
	fixed_t pushV;			// Push-out vertical multiplier.
	fixed_t spread;			// Randomized spread distance.
	angle_t cone;			// Randomized angle of the push-out.

	tic_t sfxFreq;			// How frequently to play the sound.
	tic_t frequency;		// How frequently to spawn the particles.
	fixed_t requiredSpeed;	// Speed percentage you need to be at to trigger the particles.
} t_footstep_t;

typedef enum
{
	// Terrain flag values.
	TRF_LIQUID = 1, // Texture water properties (wavy, slippery, etc)
	TRF_SNEAKERPANEL = 1<<1, // Texture is a booster
	TRF_STAIRJANK = 1<<2, // Texture is bumpy road
	TRF_TRIPWIRE = 1<<3 // Texture is a tripwire when used as a midtexture
} terrain_flags_t;

typedef struct terrain_s
{
	// Terrain definition.
	// These are all of the properties that the floor gets.

	char name[TERRAIN_NAME_LEN];	// Lookup name.

	size_t splashID;		// Splash defintion ID.
	size_t footstepID;		// Footstep defintion ID.

	fixed_t friction;		// The default friction of this texture.
	UINT8 offroad;			// The default offroad level of this texture.
	INT16 damageType;		// The default damage type of this texture. (Negative means no damage).
	UINT8 trickPanel;		// Trick panel strength
	UINT32 flags;			// Flag values (see: terrain_flags_t)
} terrain_t;

typedef struct t_floor_s
{
	// Terrain floor definition.
	// Ties texture names to a .

	// (Could be optimized by using texture IDs instead of names,
	// but was concerned because I recall sooomething about those not being netsafe?
	// Someone confirm if I just hallucinated that. :V)

	char textureName[9];	// Floor texture name.
	size_t terrainID;		// Terrain definition ID.
} t_floor_t;

/*--------------------------------------------------
	size_t K_GetSplashHeapIndex(t_splash_t *splash);

		Returns a splash defintion's index in the
		splash definition heap.

	Input Arguments:-
		splash - The splash definition to return the index of.

	Return:-
		The splash heap index, SIZE_MAX if the splash was invalid.
--------------------------------------------------*/

size_t K_GetSplashHeapIndex(t_splash_t *splash);


/*--------------------------------------------------
	size_t K_GetNumSplashDefs(void);

		Returns the number of splash definitions.

	Input Arguments:-
		None

	Return:-
		Length of splashDefs.
--------------------------------------------------*/

size_t K_GetNumSplashDefs(void);


/*--------------------------------------------------
	t_splash_t *K_GetSplashByIndex(size_t checkIndex);

		Retrieves a splash definition by its heap index.

	Input Arguments:-
		checkIndex - The heap index to retrieve.

	Return:-
		The splash definition, NULL if it didn't exist.
--------------------------------------------------*/

t_splash_t *K_GetSplashByIndex(size_t checkIndex);


/*--------------------------------------------------
	t_splash_t *K_GetSplashByName(const char *checkName);

		Retrieves a splash definition by its lookup name.

	Input Arguments:-
		checkName - The lookup name to retrieve.

	Return:-
		The splash definition, NULL if it didn't exist.
--------------------------------------------------*/

t_splash_t *K_GetSplashByName(const char *checkName);


/*--------------------------------------------------
	size_t K_GetFootstepHeapIndex(t_footstep_t *footstep);

		Returns a footstep defintion's index in the
		footstep definition heap.

	Input Arguments:-
		footstep - The footstep definition to return the index of.

	Return:-
		The footstep heap index, SIZE_MAX if the footstep was invalid.
--------------------------------------------------*/

size_t K_GetFootstepHeapIndex(t_footstep_t *footstep);


/*--------------------------------------------------
	size_t K_GetNumFootstepDefs(void);

		Returns the number of footstep definitions.

	Input Arguments:-
		None

	Return:-
		Length of footstepDefs.
--------------------------------------------------*/

size_t K_GetNumFootstepDefs(void);


/*--------------------------------------------------
	t_footstep_t *K_GetFootstepByIndex(size_t checkIndex);

		Retrieves a footstep definition by its heap index.

	Input Arguments:-
		checkIndex - The heap index to retrieve.

	Return:-
		The footstep definition, NULL if it didn't exist.
--------------------------------------------------*/

t_footstep_t *K_GetFootstepByIndex(size_t checkIndex);


/*--------------------------------------------------
	t_footstep_t *K_GetFootstepByName(const char *checkName);

		Retrieves a footstep definition by its lookup name.

	Input Arguments:-
		checkName - The lookup name to retrieve.

	Return:-
		The footstep definition, NULL if it didn't exist.
--------------------------------------------------*/

t_footstep_t *K_GetFootstepByName(const char *checkName);


/*--------------------------------------------------
	size_t K_GetTerrainHeapIndex(terrain_t *terrain);

		Returns a terrain defintion's index in the
		terrain definition heap.

	Input Arguments:-
		terrain - The terrain definition to return the index of.

	Return:-
		The terrain heap index, SIZE_MAX if the terrain was invalid.
--------------------------------------------------*/

size_t K_GetTerrainHeapIndex(terrain_t *terrain);


/*--------------------------------------------------
	size_t K_GetNumTerrainDefs(void);

		Returns the number of terrain definitions.

	Input Arguments:-
		None

	Return:-
		Length of terrainDefs.
--------------------------------------------------*/

size_t K_GetNumTerrainDefs(void);


/*--------------------------------------------------
	terrain_t *K_GetTerrainByIndex(size_t checkIndex);

		Retrieves a terrain definition by its heap index.

	Input Arguments:-
		checkIndex - The heap index to retrieve.

	Return:-
		The terrain definition, NULL if it didn't exist.
--------------------------------------------------*/

terrain_t *K_GetTerrainByIndex(size_t checkIndex);


/*--------------------------------------------------
	terrain_t *K_GetTerrainByName(const char *checkName);

		Retrieves a terrain definition by its lookup name.

	Input Arguments:-
		checkName - The lookup name to retrieve.

	Return:-
		The terrain definition, NULL if it didn't exist.
--------------------------------------------------*/

terrain_t *K_GetTerrainByName(const char *checkName);

/*--------------------------------------------------
	terrain_t *K_GetDefaultTerrain(void);

		Returns the default terrain definition, used
		in cases where terrain is not set for a texture.

	Input Arguments:-
		None

	Return:-
		The default terrain definition, NULL if it didn't exist.
--------------------------------------------------*/

terrain_t *K_GetDefaultTerrain(void);


/*--------------------------------------------------
	terrain_t *K_GetTerrainForTextureName(const char *checkName);

		Returns the terrain definition applied to
		the texture name inputted.

	Input Arguments:-
		checkName - The texture's name.

	Return:-
		The texture's terrain definition if it exists,
		otherwise the default terrain if it exists,
		otherwise NULL.
--------------------------------------------------*/

terrain_t *K_GetTerrainForTextureName(const char *checkName);


/*--------------------------------------------------
	terrain_t *K_GetTerrainForTextureNum(INT32 textureNum);

		Returns the terrain definition applied to
		the texture ID inputted.

	Input Arguments:-
		textureNum - The texture's ID.

	Return:-
		The texture's terrain definition if it exists,
		otherwise the default terrain if it exists,
		otherwise NULL.
--------------------------------------------------*/

terrain_t *K_GetTerrainForTextureNum(INT32 textureNum);


/*--------------------------------------------------
	terrain_t *K_GetTerrainForFlatNum(INT32 flatID);

		Returns the terrain definition applied to
		the level flat ID.

	Input Arguments:-
		flatID - The level flat's ID.

	Return:-
		The level flat's terrain definition if it exists,
		otherwise the default terrain if it exists,
		otherwise NULL.
--------------------------------------------------*/

terrain_t *K_GetTerrainForFlatNum(INT32 flatID);


/*--------------------------------------------------
	void K_UpdateMobjTerrain(mobj_t *mo, INT32 flatID);

		Updates an object's terrain pointer, based on
		the level flat ID supplied. Intended to be called
		when the object moves to new floors.

	Input Arguments:-
		mo - The object to update.
		flatID - The level flat ID the object is standing on.

	Return:-
		None
--------------------------------------------------*/

void K_UpdateMobjTerrain(mobj_t *mo, INT32 flatID);


/*--------------------------------------------------
	void K_ProcessTerrainEffect(mobj_t *mo);

		Handles applying terrain effects to the object,
		intended to be called in a thinker.

		Currently only intended for players, but
		could be modified to be inclusive of all
		object types.

	Input Arguments:-
		mo - The object to apply effects to.

	Return:-
		None
--------------------------------------------------*/

void K_ProcessTerrainEffect(mobj_t *mo);


/*--------------------------------------------------
	void K_SetDefaultFriction(mobj_t *mo);

		Resets an object to their default friction values.
		If they are on terrain with different friction,
		they will update to that value.

	Input Arguments:-
		mo - The object to reset the friction values of.

	Return:-
		None
--------------------------------------------------*/

void K_SetDefaultFriction(mobj_t *mo);


/*--------------------------------------------------
	void K_SpawnSplashForMobj(mobj_t *mo, fixed_t impact);

		Spawns the splash particles for an object's
		terrain type. Intended to be called when hitting a floor.

	Input Arguments:-
		mo - The object to spawn a splash for.

	Return:-
		None
--------------------------------------------------*/
void K_SpawnSplashForMobj(mobj_t *mo, fixed_t impact);


/*--------------------------------------------------
	void K_HandleFootstepParticles(mobj_t *mo);

		Spawns the footstep particles for an object's
		terrain type. Intended to be called every tic.

	Input Arguments:-
		mo - The object to spawn footsteps for.

	Return:-
		None
--------------------------------------------------*/

void K_HandleFootstepParticles(mobj_t *mo);


/*--------------------------------------------------
	void K_InitTerrain(UINT16 wadNum);

		Finds the TERRAIN lumps in a WAD/PK3, and
		processes all of them.

	Input Arguments:-
		wadNum - WAD file ID to process.

	Return:-
		None
--------------------------------------------------*/

void K_InitTerrain(UINT16 wadNum);

#endif // __K_TERRAIN_H__