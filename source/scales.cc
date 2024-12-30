// ----------------------------------------------------------------------------
//
//  Copyright (C) 2003-2013 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#include "scales.h"


// Pythagorean

float scale_pythagorean [12] =
{
    1.00000000,
    1.06787109,
    1.12500000,
    1.18518519,
    1.26562500,
    1.33333333,
    1.42382812,
    1.50000000,
    1.60180664,
    1.68750000,
    1.77777778,
    1.89843750,
};


// 1/4 comma meantone (Pietro Aaron, 1523)

float scale_meanquart [12] = 
{
    1.00000000,
    1.04490673,
    1.11803399,
    1.19627902,
    1.25000000,
    1.33748061,
    1.39754249,
    1.49534878,
    1.56250000,
    1.67185076,
    1.78885438,
    1.86918598
};


// Andreas Werckmeister III, 1681

float scale_werckm3 [12] = 
{
    1.00000000,
    1.05349794,
    1.11740331,
    1.18518519,
    1.25282725,
    1.33333333,
    1.40466392,
    1.49492696,
    1.58024691,
    1.67043633,
    1.77777778,
    1.87924088
};


// Kirnberger III

float scale_kirnberg3 [12] = 
{
    1.00000000,
    1.05349794,
    1.11848107,
    1.18518519,
    1.25000021,
    1.33333333,
    1.40625000,
    1.49542183,
    1.58024691,
    1.67176840,
    1.77777778,
    1.87500000
};


// Well-tempered (Jacob Breetvelt)

float scale_welltemp [12] =
{
    1.00000000,
    1.05468828,
    1.12246205,
    1.18652432,
    1.25282725,
    1.33483985,
    1.40606829,
    1.49830708,
    1.58203242,
    1.67705161,
    1.77978647,
    1.87711994
};


// Equally Tempered

float scale_equaltemp [12] =
{
    1.00000000,
    1.05946309,
    1.12246205,
    1.18920712,
    1.25992105,
    1.33483985,
    1.41421356,
    1.49830708,
    1.58740105,
    1.68179283,
    1.78179744,
    1.88774863,
};


// The following five were contributed by Hanno Hoffstadt.
// The Lehman temperament was also provided by Adam Sampson.

// Vogel/Ahrend

float scale_ahrend [12] =
{
    1.00000000,
    1.05064661,
    1.11891853,
    1.18518519,
    1.25197868,
    1.33695184,
    1.40086215,
    1.49594019,
    1.57596992,
    1.67383521,
    1.78260246,
    1.87288523,
};


// Vallotti-Barca

float scale_vallotti [12] =
{
    1.00000000,
    1.05647631,
    1.12035146,
    1.18808855,
    1.25518740,
    1.33609659,
    1.40890022,
    1.49689777,
    1.58441623,
    1.67705160,
    1.78179744,
    1.87888722,
};


// Kellner

float scale_kellner [12] =
{
    1.00000000,
    1.05349794,
    1.11891853,
    1.18518519,
    1.25197868,
    1.33333333,
    1.40466392,
    1.49594019,
    1.58024691,
    1.67383521,
    1.77777778,
    1.87796802,
};


// Lehman

float scale_lehman [12] =
{
    1.00000000,
    1.05826737,
    1.11992982,
    1.18786496,
    1.25424281,
    1.33634808,
    1.41102316,
    1.49661606,
    1.58560949,
    1.67610496,
    1.77978647,
    1.88136421,
};


// Pythagorean

float scale_pure_cfg [12] =
{
    1.00000000,
    1.04166667,
    1.12500000,
    1.1892,
    1.25000000,
    1.33333333,
    1.40625000,
    1.50000000,
    1.5874,
    1.66666667,
    1.77777778,
    1.87500000,
};


// The following three were contributed by Chris Pacejo.

// 1/3 comma meantone (Francisco de Salinas, 1577)

float scale_meanthird [12] =
{
    1.00000000,
    1.03736221,
    1.11572158,
    1.20000000,
    1.24483465,
    1.33886590,
    1.38888889,
    1.49380158,
    1.54961331,
    1.66666667,
    1.79256190,
    1.85953597
};


// Neidhardt "Kleine Stadt", 1732

float scale_kleine_stadt [12] =
{
    1.00000000,
    1.05587996,
    1.11992982,
    1.18652431,
    1.25424281,
    1.33333333,
    1.40783995,
    1.49661606,
    1.58381994,
    1.67610496,
    1.77777778,
    1.87924087
};


// Neidhardt "Große Stadt", 1724

float scale_grosse_stadt [12] =
{
    1.00000000,
    1.05707299,
    1.11992982,
    1.18786496,
    1.25565996,
    1.33333333,
    1.41102316,
    1.49661606,
    1.58381994,
    1.67610496,
    1.77978647,
    1.88348995
};


struct temper scales [NSCALES] =
{
    { "Pythagorean", "pyt", scale_pythagorean },
    { "Meantone 1/4", "mtq", scale_meanquart },
    { "Meantone 1/3", "mtt", scale_meanthird },
    { "Werckmeister III", "we3", scale_werckm3 },
    { "Neidhardt Große Stadt", "ngs", scale_grosse_stadt },
    { "Neidhardt Kleine Stadt", "nks", scale_kleine_stadt },
    { "Kirnberger III", "ki3", scale_kirnberg3 },
    { "Well Tempered", "wt",  scale_welltemp },
    { "Equally Tempered", "et", scale_equaltemp },
    { "Vogel/Ahrend", "ahr", scale_ahrend },
    { "Vallotti-Barca", "val", scale_vallotti },
    { "Kellner", "kel", scale_kellner },
    { "Lehman", "leh", scale_lehman },
    { "Pure C/F/G", "cfg", scale_pure_cfg },
};
