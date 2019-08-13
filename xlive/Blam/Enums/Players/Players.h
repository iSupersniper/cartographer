#pragma once
#include "stdafx.h"

namespace Blam
{
	namespace Enums
	{
		namespace Player
		{
			enum class Color : BYTE // TODO: move this somewhere else
			{
				White,
				Steel,
				Red,
				Orange,
				Gold,
				Olive,
				Green,
				Sage,
				Cyan,
				Teal,
				Colbat,
				Blue,
				Violet,
				Purple,
				Pink,
				Crimson,
				Brown,
				Tan
			};
			enum class Biped : BYTE
			{
				MasterChief = 0,
				Dervish = 1,
				Spartan = 2,
				Elite = 3
			};
			enum class EmblemForeground : BYTE
			{
				SeventhColumn = 0,
				Bullseye = 1,
				Vortex = 2,
				Halt = 3,
				Spartan = 4,
				DaBomb = 5,
				Trinity = 6,
				Delta = 7,
				Rampancy = 8,
				Sergeant = 9,
				Phenoix = 10,
				Champion = 11,
				JollyRoger = 12,
				Marathon = 13,
				Cube = 14,
				Radioactive = 15,
				Smiley = 16,
				Frowney = 17,
				Spearhead = 18,
				Sol = 19,
				Waypoint = 20,
				YingYang = 21,
				Helmet = 22,
				Triad = 23,
				GruntSymbol = 24,
				Cleave = 25,
				Thor = 26,
				SkullKing = 27,
				Triplicate = 28,
				Subnova = 29,
				FlamingNinja = 30,
				DoubleCresent = 31,
				Spades = 32,
				Clubs = 33,
				Diamonds = 34,
				Hearts = 35,
				Wasp = 36,
				MarkOfShame = 37,
				Snake = 38,
				Hawk = 39,
				Lips = 40,
				Capsule = 41,
				Cancel = 42,
				GasMask = 43,
				Grenade = 44,
				Tsanta = 45,
				Race = 46,
				Valkyire = 47,
				Drone = 48,
				Grunt = 49,
				GruntHead = 50,
				BruteHead = 51,
				Runes = 52,
				Trident = 53,
				Number0 = 54,
				Number1 = 55,
				Number2 = 56,
				Number3 = 57,
				Number4 = 58,
				Number5 = 59,
				Number6 = 60,
				Number7 = 61,
				Number8 = 62,
				Number9 = 63
			};
			enum class EmblemBackground : BYTE
			{
				Solid = 0,
				VerticalSplit = 1,
				HorizontalSplit1 = 2,
				HorizontalSplit2 = 3,
				VerticalGradient = 4,
				HorizontalGradient = 5,
				TripleColumn = 6,
				TripleRow = 7,
				Quadrants1 = 8,
				Quadrants2 = 9,
				DiagonalSlice = 10,
				Cleft = 11,
				X1 = 12,
				X2 = 13,
				Circle = 14,
				Diamond = 15,
				Cross = 16,
				Square = 17,
				DualHalfCircle = 18,
				Triangle = 19,
				DiagonalQuadrant = 20,
				ThreeQuaters = 21,
				Quarter = 22,
				FourRows1 = 23,
				FourRows2 = 24,
				SplitCircle = 25,
				OneThird = 26,
				TwoThirds = 27,
				UpperField = 28,
				TopandBottom = 29,
				CenterStripe = 30,
				LeftandRight = 31
			};
			enum class EmblemToggle : BYTE
			{
				Off = 0,
				On = 1
			};
			enum Team : BYTE
			{
				// MP
				Red = 0,
				Blue = 1,
				Yellow = 2,
				Green = 3,
				Purple = 4,
				Orange = 5,
				Bronwn = 6,
				Pink = 7,
				// SP
				Default = 0,
				Player = 1,
				Human = 2,
				Covenant = 3,
				Flood = 4,
				Sentinel = 5,
				Heretic = 6,
				Prophet = 7,

				// unasigned team ids
				Unused8 = 8,
				Unused9 = 9,
				Unused10 = 10,
				Unused11 = 11,
				Unused12 = 12,
				Unused13 = 13,
				Unused14 = 14,
				Unused15 = 15,

				// Shared
				None = 255
			};
			enum class Handicap : BYTE
			{
				None = 0,
				Minor = 1,
				Moderate = 2,
				Severe = 3
			};
		}
	}
}
