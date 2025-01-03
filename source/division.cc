// ----------------------------------------------------------------------------
//
//  Copyright (C) 2003-2022 Fons Adriaensen <fons@linuxaudio.org>
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


#include <algorithm>
#include <cmath>
#include <math.h>
#include <memory>
#include <numbers>
#include <utility>
#include "division.h"

namespace
{

float compute_lowpass_alpha(const float omega_c)
{
    const float y = 1 - cosf(omega_c);
    return -y + sqrtf(y*y + 2*y);
}

static constexpr int
    NMASK_SET = 1 << ((NKEYBD + 1) * NLINKS),  // Set if mask is modified.
    NMASK_ONE_LINK = (1 << (NKEYBD + 1)) - 1,
    NMASK_ALL = (1 << ((NKEYBD + 1) * NLINKS)) - 1;

// bit magic to create a multiplicand which replicates
// a mask once for each linkage (NLINKS)
static constexpr int NMASK_LINKREPL = NMASK_SET / ((1 << (NKEYBD + 1)) - 1);

}


Division::Division (Asection *asect, float fsam) :
    _asect (asect),
    _nrank (0),
    _dmask (0),
    _trem (0), _tmask (0),
    _fsam (fsam),
    _swel (1.0f), _swel_last (1.0f),
    _gain (0.1f),
    _w (0.0f),
    _c (1.0f),
    _s (0.0f),
    _m (0.0f),
    _swel_alpha (compute_lowpass_alpha ((160.0f / fsam) * (2.0f * std::numbers::pi_v<float>))),
    _swel_y1 { }
{
}


void Division::process (void) 
{
    int    i;
    float  d, g, t;
    float  *p, *q; 

    std::fill_n (_buff, NCHANN * PERIOD, 0);
    for (i = 0; i < _nrank; i++)
        if (_ranks [i])
            _ranks [i]->play (1);

    g = 1.0f;
    if (_trem)
    {
	_s += _w * _c;
	_c -= _w * _s;
        t = sqrtf (_c * _c + _s * _s);
        _c /= t;
        _s /= t;
        if ((_trem == 2) && (fabsf (_s) < 0.05f))
        {
	    _trem = 0;
            _c = 1;
            _s = 0;
	}
        g *= 1.0f + _m * _s;
    }

    t = 1.05f * _gain;
    if (g > t) g = t;
    t = 0.95f * _gain;
    if (g < t) g = t;

    d = (g - _gain) / PERIOD;    
    g = _gain;
    p = _buff;
    float swel = _swel_last;
    const float swel_d = (_swel - swel) / PERIOD;
    q = _asect->get_wptr ();

    float swel_y1 [NCHANN];
    std::copy_n (_swel_y1, NCHANN, swel_y1);

    for (i = 0; i < PERIOD; i++)
    {
        g += d;
        swel += swel_d;
        for (int j = 0; j < NCHANN; j++)
        {
            const float x0 = p [j * PERIOD] * g;
            const float swel_y0 = _swel_alpha * x0 + (1.0f - _swel_alpha) * swel_y1 [j];
            q [j * PERIOD * MIXLEN] += std::lerp (swel_y0, x0, swel);
            swel_y1 [j] = swel_y0;
        }
        p++;
        q++;
    }
    _gain = g;
    _swel_last = swel;
    std::copy_n (swel_y1, NCHANN, _swel_y1);
}


// Set or replace the Rankwave for a Rank.
//
void Division::set_rank (int ind, std::unique_ptr <Rankwave> W, int pan, int del)
{
    if (_ranks [ind])
    {
        W->_nmask = _ranks [ind]->_nmask | NMASK_SET;
    }
    else W->_nmask = NMASK_SET;
    _ranks [ind] = std::move (W);
    del = (int)(1e-3f * del * _fsam / PERIOD);
    if (del > 31) del = 31;
    _ranks [ind]->set_param (_buff, del, pan);
    if (_nrank < ++ind) _nrank = ind;
}


// Handle key up down events.
//
void Division::update (int note, int16_t mask)
{
    const int fullmask = mask * NMASK_LINKREPL;

    int             r;
    Rankwave       *W;

    for (r = 0; r < _nrank; r++)
    {
	W = _ranks [r].get ();
	if (!W) continue;

        if (W->_nmask & NMASK_ALL)
	{     
	    if (fullmask & W->_nmask) W->note_on (note + 36);
	    else W->note_off (note + 36);
	}
    }
}


void Division::update (uint16_t *keys)
{
    int       d, r, m, n, n0, n1;
    uint16_t  *k;
    Rankwave  *W;

    for (r = 0; r < _nrank; r++)
    {
	W = _ranks [r].get ();
	if (!W) continue;

        if (W->_nmask & NMASK_SET)
	{
            W->_nmask ^= NMASK_SET;
            m = W->_nmask & NMASK_ALL;
            if (m)
	    {            
		n0 = W->n0 ();
		n1 = W->n1 ();
                k = keys;
                d = n0 - 36;
                if (d > 0) k += d;
                for (n = n0; n <= n1; n++)
	        {
                    if ((*k++ * NMASK_LINKREPL) & m) W->note_on (n);
		    else W->note_off (n);
		}
	    }
            else W->all_off ();
	}
    }
}


void Division::set_div_mask (int bit, int linkage)
{
    int       r;
    Rankwave *W;

    _dmask |= 1 << (bit + linkage * (NKEYBD + 1));
    for (r = 0; r < _nrank; r++)
    {
	W = _ranks [r].get ();
	if (!W) continue;

        const int d = (W->_nmask >> NKEYBD) & NMASK_LINKREPL;
        if (d)
        {
            W->_nmask |= d << bit;
            W->_nmask |= NMASK_SET;
        }
    }
}


void Division::clr_div_mask (int bit, int linkage)
{
    int       r;
    Rankwave *W;

    _dmask &= ~(1 << (bit + linkage * (NKEYBD + 1)));
    if (((_dmask >> bit) & NMASK_LINKREPL) != 0) return;
    for (r = 0; r < _nrank; r++)
    {
	W = _ranks [r].get ();
	if (!W) continue;

        const int d = (W->_nmask >> NKEYBD) & NMASK_LINKREPL;
        if (d)
        {
            W->_nmask &= ~(d << bit);
            W->_nmask |= NMASK_SET;
        }
    } 
}


void Division::set_rank_mask (int ind, int bit, int linkage)
{
    int b = 1 << (bit + linkage * (NKEYBD + 1));
    Rankwave *W = _ranks [ind].get ();
    if (bit == NKEYBD) b |= merged_dmask () << (linkage * (NKEYBD + 1));
    W->_nmask |= b;
    W->_nmask |= NMASK_SET;
}


void Division::clr_rank_mask (int ind, int bit, int linkage)
{
    int b = 1 << (bit + linkage * (NKEYBD + 1));
    Rankwave *W = _ranks [ind].get ();
    if (bit == NKEYBD) b |= merged_dmask () << (linkage * (NKEYBD + 1));
    W->_nmask &= ~b;
    W->_nmask |= NMASK_SET;
}


int Division::merged_dmask () const
{
    int merged = _dmask;
    for (int i = NLINKS; i > 1; i = (i + 1) >> 1)
        merged |= merged >> (((i + 1) >> 1) * (NKEYBD + 1));
    return merged & NMASK_ONE_LINK;
}


void Division::trem_on (int linkage)
{
    _tmask |= 1 << linkage;
    _trem = 1;
}


void Division::trem_off (int linkage)
{
    _tmask &= ~(1 << linkage);
    if (_tmask == 0) _trem = 2;
}
