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


#ifndef __ASECTION_H
#define __ASECTION_H


#include <memory>
#include "global.h"


#define PERIOD 64
#define MIXLEN 64
#define NCHANN 4


class Diffuser
{
public:

    Diffuser () = default;
    Diffuser (int size, float c);

    int  size (void) { return _size; }
    float process (float x);
    void period_begin ();
    void period_end ();

private:

    std::unique_ptr <float []> _data;
    int        _size;
    int        _i;
    float      _c;
};


class Asection
{
public:

    Asection (float fsam);

    float *get_wptr (void) { return _base.get () + _offs0; }
    Fparm *get_apar (void) { return _apar; }

    void set_size (float size);
    void process (float vol, float *W, float *X, float *Y, float *R);
    
    static float _refl [16];

    enum { AZIMUTH, STWIDTH, DIRECT, REFLECT, REVERB };

private:

    int      _offs0;
    int      _offs [16];
    float    _fsam;
    std::unique_ptr <float []> _base;
    float    _sw;
    float    _sx;
    float    _sy;
    Diffuser _dif0;
    Diffuser _dif1;
    Diffuser _dif2;
    Diffuser _dif3;
    Fparm    _apar [5];
};


#endif

