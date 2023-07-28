// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2004-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  y_inter.h
/// \brief Tally screens, or "Intermissions" as they were formally called in Doom

#ifndef __Y_INTER_H__
#define __Y_INTER_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	boolean rankingsmode; // rankings mode
	boolean gotthrough; // show "got through"
	boolean showrank; // show rank-restricted queue entry at the end, if it exists
	boolean encore; // encore mode
	boolean isduel; // duel mode
	UINT8 roundnum; // round number

	char headerstring[64]; // holds levelnames up to 64 characters

	UINT8 numplayers; // Number of players being displayed

	SINT8 num[MAXPLAYERS]; // Player #
	UINT8 pos[MAXPLAYERS]; // player positions. used for ties

	UINT8 character[MAXPLAYERS]; // Character #
	UINT16 color[MAXPLAYERS]; // Color #

	UINT32 val[MAXPLAYERS]; // Gametype-specific value
	char strval[MAXPLAYERS][MAXPLAYERNAME+1];

	INT16 increase[MAXPLAYERS]; // how much did the score increase by?
	UINT8 jitter[MAXPLAYERS]; // wiggle

	UINT8 mainplayer; // Most successful local player
	INT32 linemeter; // For GP only
} y_data_t;

void Y_IntermissionDrawer(void);
void Y_Ticker(void);

// Specific sub-drawers
void Y_PlayerStandingsDrawer(y_data_t *standings, INT32 xoffset);
void Y_RoundQueueDrawer(y_data_t *standings, INT32 offset, boolean doanimations, boolean widescreen);

void Y_StartIntermission(void);
void Y_EndIntermission(void);

void Y_DetermineIntermissionType(void);

typedef enum
{
	int_none,
	int_time,				// Always time
	int_score,				// Always score
	int_scoreortimeattack,	// Score unless 1P
} intertype_t;

extern intertype_t intertype;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __Y_INTER_H__
