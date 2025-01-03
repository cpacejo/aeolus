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


#include <algorithm>
#include <cassert>
#include <math.h>
#include <memory>
#include <numbers>
#include "asection.h"


extern float exp2ap (float);


#define N (MIXLEN * PERIOD)


Diffuser::Diffuser (int size, float c)
{
    assert(size >= PERIOD);
    _size = size;
    _data = std::make_unique <float []> (size + PERIOD - 1);
    _i = 0;
    _c = c;
}


float Diffuser::process (float x)
{
    float w;

    w = x - _c * _data [_i];
    x = _data [_i] + _c * w;
    _data [_i] = w;
    ++_i;  // wrapping is handled by period_end ()
    return x;
}


void Diffuser::period_begin ()
{
    if (_i + PERIOD >= _size)
    {
        std::copy_n (_data.get(), _i + PERIOD - _size, _data.get() + _size);
    }
}

void Diffuser::period_end ()
{
    if (_i >= _size)
    {
        std::copy_n (_data.get() + _size, _i - _size, _data.get());
        _i -= _size;
    }
}


float Asection::_refl [16] =
{
    0.250f, 0.440f, 0.615f, 0.940f,
    0.200f, 0.370f, 0.560f, 0.900f,
    0.230f, 0.400f, 0.500f, 0.920f,
    0.280f, 0.460f, 0.690f, 0.960f
};


Asection::Asection (float fsam) : _fsam (fsam)
{
    _base = std::make_unique <float []> (NCHANN * N);

    _offs0 = 0;
    _sw = _sx = _sy = 0.0f;
    _dif0 = Diffuser ((int)(fsam * 0.017f), 0.5f);
    _dif1 = Diffuser ((int)(fsam * 0.029f), 0.5f);
    _dif2 = Diffuser ((int)(fsam * 0.023f), 0.5f);
    _dif3 = Diffuser ((int)(fsam * 0.013f), 0.5f);

    _apar [AZIMUTH]._val =  0.0f;
    _apar [AZIMUTH]._min = -0.5f;
    _apar [AZIMUTH]._max =  0.5f;
    _apar [STWIDTH]._val = 0.8f;
    _apar [STWIDTH]._min = 0.0f;
    _apar [STWIDTH]._max = 1.0f;
    _apar [DIRECT]._val = 0.56f;
    _apar [DIRECT]._min = 0.00f;
    _apar [DIRECT]._max = 1.00f;
    _apar [REFLECT]._val = 0.25f;
    _apar [REFLECT]._min = 0.00f;
    _apar [REFLECT]._max = 1.00f;
    _apar [REVERB]._val = 0.32f;
    _apar [REVERB]._min = 0.00f;
    _apar [REVERB]._max = 1.00f;
}


void Asection::set_size (float time) 
{
    int   i, d;
    float r;

    r = (time * _fsam);
    if (r > N - PERIOD) r = N - PERIOD;
    for (i = 0; i < 16; i++)
    {
	d = (int)(r * _refl [i]);
        d = (_offs0 - d * PERIOD) & (N - 1);
	_offs [i] = d + (i >> 2) * N;
    }
}


void Asection::process (float vol, float *W, float *X, float *Y, float *R) 
{
    int     i;
    float   s, d, g, gw, gr, gx1, gy1, gx2, gy2;
    float   *p, t0, t1, t2, t3;
    float   ta0 [PERIOD], ta1 [PERIOD], ta2 [PERIOD], ta3 [PERIOD];
    float   x [PERIOD];
    float   y [PERIOD];

    gw = vol * _apar [DIRECT]._val;   
    g = 0.45f * _apar [STWIDTH]._val;
    s = 0.5f + g * (1 - g);
    d = g - 0.5f;
    gx1 = gw * (s - d);
    gy1 = gw * (s + d);
    g = 0.25f * _apar [STWIDTH]._val;
    s = 0.5f + g * (1 - g);
    d = g - 0.5f;
    gx2 = gw * (s - d);
    gy2 = gw * (s + d);
    p = _base.get () + _offs0;
    gr = 0.5f * _apar [REVERB]._val;
    for (i = 0; i < PERIOD; i++)
    {
	t0 = p [0 * N];
	t1 = p [1 * N];
	t2 = p [2 * N];
	t3 = p [3 * N];
        p++;
        s = t0 + t1 + t2 + t3; 
        R [i] += gr * s; 
        W [i] += gw * s;
        x [i] = gx1 * (t3 + t0) + gx2 * (t2 + t1);
        y [i] = gy1 * (t3 - t0) + gy2 * (t2 - t1);
    }

    _dif0.period_begin ();
    p = _base.get ();
    for (i = 0; i < PERIOD; i++, p++)
        ta0 [i] = _dif0.process (p [_offs [1]] + p [_offs  [5]] + p [_offs [11]] + p [_offs [15]] + 1e-20f);
    _dif0.period_end ();

    _dif1.period_begin ();
    p = _base.get ();
    for (i = 0; i < PERIOD; i++, p++)
        ta1 [i] = _dif1.process (p [_offs [0]] + p [_offs  [4]] + p [_offs [10]] + p [_offs [14]] + 1e-20f);
    _dif1.period_end ();

    _dif2.period_begin ();
    p = _base.get ();
    for (i = 0; i < PERIOD; i++, p++)
        ta2 [i] = _dif2.process (p [_offs [2]] + p [_offs  [6]] + p [_offs  [8]] + p [_offs [12]] + 2e-20f);
    _dif2.period_end ();

    _dif3.period_begin ();
    p = _base.get ();
    for (i = 0; i < PERIOD; i++, p++)
        ta3 [i] = _dif3.process (p [_offs [3]] + p [_offs  [7]] + p [_offs  [9]] + p [_offs [13]] + 2e-20f);
    _dif3.period_end ();

    gr = vol * _apar [REFLECT]._val;
    for (i = 0; i < PERIOD; i++)
    {
        t0 = ta0 [i];
        t1 = ta1 [i];
        t2 = ta2 [i];
        t3 = ta3 [i];
        s = t0 + t1 + t2 + t3;
        _sw += 0.5f * (s - _sw);
        _sx += 0.5f * (0.4f * (t0 + t3) + 0.6f * (t2 + t1) - _sx);
        _sy += 0.5f * (0.9f * (t0 - t3) + 0.8f * (t2 - t1) - _sy);
        W [i] += gr * _sw;
        x [i] += gr * _sx;
        y [i] += gr * _sy;
    }

    g = (2.0f * std::numbers::pi_v<float>) * _apar [AZIMUTH]._val;
    gx1 = cosf (g);
    gy1 = sinf (g);
    for (i = 0; i < PERIOD; i++)
    {
	X [i] += gx1 * x [i] + gy1 * y [i];
	Y [i] += gx1 * y [i] - gy1 * x [i];
    }

    _offs0 = (_offs0 + PERIOD) & (N - 1);
    for (i = 0; i < 16; i++) _offs [i] = ((_offs [i] + PERIOD) & (N - 1)) + (i >> 2) * N;
    p = _base.get () + _offs0;
    std::fill_n (p + 0 * N, PERIOD, 0);
    std::fill_n (p + 1 * N, PERIOD, 0);
    std::fill_n (p + 2 * N, PERIOD, 0);
    std::fill_n (p + 3 * N, PERIOD, 0);
}

