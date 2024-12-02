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


#ifndef __DIVISION_H
#define __DIVISION_H


#include <memory>
#include "asection.h"
#include "rankwave.h"


class Division
{
public:

    Division (Asection *asect, float fsam);

    void set_rank (int ind, std::unique_ptr <Rankwave> W, int pan, int del);
    void set_swell (float stat) { _swel = stat; }
    void set_tfreq (float freq) { _w = 6.283184f * PERIOD * freq / _fsam; }
    void set_tmodd (float modd) { _m = modd; }
    void set_div_mask (int bits);
    void clr_div_mask (int bits);
    void set_rank_mask (int ind, int linkage, int bits);
    void clr_rank_mask (int ind, int linkage, int bits);
    void trem_on (void)  { _trem = 1; }
    void trem_off (void) { _trem = 2; }

    void process (void);
    void update (int note, int16_t mask);
    void update (uint16_t *keys);

private:
   
    Asection  *_asect;
    std::unique_ptr <Rankwave> _ranks [NRANKS];
    int        _nrank;
    int        _dmask;
    int        _trem;
    float      _fsam;
    float      _swel, _swel_last;
    float      _gain;
    float      _w;    
    float      _c;
    float      _s;
    float      _m;
    float      _swel_alpha;
    float      _swel_y1 [NCHANN];
    float      _buff [NCHANN * PERIOD];
};


#endif

