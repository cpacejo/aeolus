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
#include <forward_list>
#include <memory>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <utility>
#include "rankwave.h"

#ifndef REPETITION_POINTS // sp
# define REPETITION_POINTS 1
#endif

extern float exp2ap (float);

Rngen   Pipewave::_rgen;
std::unique_ptr <float []> Pipewave::_arg;
std::unique_ptr <float []> Pipewave::_att;


void Pipewave::initstatic (float fsamp)
{
    int k;

    if (_arg) return;
    k = (int)(fsamp);
    _arg = std::make_unique <float []> (k);
    k = (int)(0.5f * fsamp);
    _att = std::make_unique <float []> (k);
}   


void Pipewave::play (void)
{
    int     i, k;
    float   g, dg, y, dy;
    float   *p, *q, *r;

    p = _p_p;
    r = _p_r;

    if (_sdel & 1)
    {
	if (! p) 
	{
	    p = _p0.get();
            _y_p = 0.0f;
            _z_p = 0.0f;
        }
    }
    else
    {
        if (! r)
	{
  	    r = p;
            p = 0;
            _g_r = 1.0f;
            _y_r = _y_p;
            _i_r = _k_r;     
	}
    }

    if (r)
    {
        k = PERIOD;
        q = _out; 
	g = _g_r;
        i = _i_r - 1;
        dg = g / PERIOD;  
        if (i) dg *= _m_r ;
 
        if (r < _p1)
        {
            while (k--)
            {
	        *q++ += g * *r++;
                g -= dg;
            }
        }
        else 
	{
            y = _y_r;  
            dy = _d_r;
            while (k--)
	    {            
		y += dy;
		if (y > 1.0f)
		{
		    y -= 1.0f;
		    r += 1;
		}
		else if (y < 0.0f)
		{
		    y += 1.0f;
		    r -= 1;
		}
                *q++ += g * (r [0] + y * (r [1] - r [0]));
		g -= dg;
                r += _k_s;
                if (r >= _p2) r -= _l1;
	    }
            _y_r = y;
	}           

        if (i) 
	{
	    _g_r = g;
            _i_r = i;
	}
        else r = 0;
    }	

    if (p) 
    { 
        k = PERIOD;             
        q = _out;
        if (p < _p1)
        {
            while (k--)
            {
		*q++ += *p++;
	    }
        }
        else 
	{
            y = _y_p;
            _z_p += _d_w * (_d_a * (_rgen.urandf () - 0.5f) - _z_p);
            dy = _z_p * _k_s;
            while (k--)
	    {            
		y += dy;
		if (y > 1.0f)
		{
		    y -= 1.0f;
		    p += 1;
		}
		else if (y < 0.0f)
		{
		    y += 1.0f;
		    p -= 1;
		}
                *q++ += p [0] + y * (p [1] - p [0]);
                p += _k_s;
                if (p >= _p2) p -= _l1;
	    }
            _y_p = y;
	}
    }

    _p_p = p;
    _p_r = r;
}               


void Pipewave::genwave (Addsynth *D, int n, float fsamp, float fpipe)
{
    int    h, i, k, nc;
    float  f0, f1, f, m, t, v, v0;

    m = D->_n_att.vi (n); // m is maximum attack duration in seconds
    for (h = 0; h < N_HARM; h++)
    {
	t = D->_h_att.vi (h, n);
        if (t > m) m = t;
    }
    _l0 = (int)(fsamp * m + 0.5); // _l0 is maximum attack duration in samples
    _l0 = (_l0 + PERIOD - 1) & ~(PERIOD - 1); // rounded up to an integer number of PERIODs (if PERIOD is a power of 2)

    f1 = (fpipe + D->_n_off.vi (n) + D->_n_ran.vi (n) * (2 * _rgen.urand () - 1)) / fsamp; // f1 is effective pipe frequency in terms of sampling rate
    f0 = f1 * exp2ap (D->_n_atd.vi (n) / 1200.0f); // f0 is detuned pipe frequency during attack

    for (h = N_HARM - 1; h >= 0; h--)
    {
        f = (h + 1) * f1;
	if ((f < 0.45f) && (D->_h_lev.vi (h, n) >= -40.0f)) break;
    }
    // f is frequency of highest relevant harmonics in terms of sampling rate
    if      (f > 0.250f) _k_s = 3; // choose sample step according to required
    else if (f > 0.125f) _k_s = 2; // temporal resolution
    else                 _k_s = 1;
    // _l1 is loop length in samples, computed by an intractable procedure
    // inside looplen()
    // nc appears to be the number of cycles in the loop (see below)
    looplen (f1 * fsamp, _k_s * fsamp, (int)(fsamp / 6.0f), &_l1, &nc);
    // round up _l1 to the nearest multiple of (_k_s * PERIOD)
    if (_l1 < _k_s * PERIOD)
    {
        k = (_k_s * PERIOD - 1) / _l1 + 1;
        _l1 *= k;
        nc *= k;
    }

    // k is the number of samples to allocate
    k = _l0 + _l1 + _k_s * (PERIOD + 4);       

    _p0 = std::make_unique <float []> (k);
    _p1 = _p0.get () + _l0; // begin of loop
    _p2 = _p1 + _l1; // end of loop
    std::fill_n (_p0.get(), k, 0);

    // _k_r is release duration in PERIODs
    _k_r = (int)(ceilf (D->_n_dct.vi (n) * fsamp / PERIOD) + 1);
    // _m_r is multiplier to apply for each PERIOD
    _m_r = 1.0f - powf (0.1, 1.0 / _k_r);
    // _d_r is release detune scaled to _k_s
    _d_r = _k_s * (exp2ap (D->_n_dcd.vi (n) / 1200.0f) - 1.0f);

    v = D->_n_ins.vi (n);
    _d_a = v * fsamp / 960e3;
    _d_w = 24 * v / fsamp;
    
    // use _arg as a buffer for time progress
    // _arg contains time in cycles
    t = 0.0f;
    // during attack, interpolate between detuned and nominal
    // frequency such that nominal frequency is reached at the
    // pipe's attack duration
    k = (int)(fsamp * D->_n_att.vi (n) + 0.5);
    for (i = 0; i <= _l0; i++)
    {
        _arg [i] = t - floorf (t + 0.5);
	t += (i < k) ? (((k - i) * f0 + i * f1) / k) : f1;
    }         
    // during loop, just fill _arg with the progressing
    // cycle number
    for (i = 1; i < _l1; i++)
    {
	t = _arg [_l0]+ (float) i * nc / _l1;
        _arg [i + _l0] = t - floorf (t + 0.5);
    }         
    // exp2ap(x) is a fast approximation of 2^x
    // 0.1661 is the factor to convert from dB to powers of 2
    // so v0 is the gain factor corresponding to the volume dB value of the pipe.
    v0 = exp2ap (0.1661 * D->_n_vol.vi (n));
    for (h = 0; h < N_HARM; h++)
    {
        // abort when harmonic frequency approaches Nyquist frequency
        if ((h + 1) * f1 > 0.45) break;
        // here, v is the harmonic's level in dB
        v = D->_h_lev.vi (h, n);          
        if (v < -80.0) continue;
        // here, v is the harmonic's final amplitude after applying random variation
        v = v0 * exp2ap (0.1661 * (v + D->_h_ran.vi (h, n) * (2 * _rgen.urand () - 1)));
        // k is the harmonic's attack duration in samples
        k = (int)(fsamp * D->_h_att.vi (h, n) + 0.5);
        // attgain() computes the harmonic's attack gain over
        // the attack period and stores it in the _att array
        attgain (k, D->_h_atp.vi (h, n));            
        // compute the harmonic's contribution to attack and loop samples
        for (i = 0; i < _l0 + _l1; i++) 
        {
	    t = _arg [i] * (h + 1);   
            t -= floorf (t);
            m = v * sinf (2 * M_PI * t);
            if (i < k) m *= _att [i]; // apply attack gain
            _p0 [i] += m;
        }
    }
    // fill remaining samples at the end with data from the loop
    for (i = 0; i < _k_s * (PERIOD + 4); i++) _p0 [i + _l0 + _l1] = _p0 [i + _l0];
}


void Pipewave::looplen (float f, float fsamp, int lmax, int *aa, int *bb)
{
    int     i, j, a, b, t;
    int     z [8];
    double  g, d;
    
    g = fsamp / f;
    for (i = 0; i < 8; i++)
    {
	a = z [i] = (int)(floor (g + 0.5));
        g -= a;
        b = 1;
        j = i;
        while (j > 0)
	{
            t = a;
  	    a = z [--j] * a + b;
	    b = t;
	}
        if (a < 0)
	{
	    a = -a;
            b = -b;
	}
        if (a <= lmax)
	{
	    d = fsamp * b / a - f; 
	    if ((fabs (d) < 0.1) && (fabs (d) < 3e-4 * f)) break;
	    g = (fabs (g) < 1e-6) ? 1e6 : 1.0 / g;
	}
        else 
	{
	    b = (int)(lmax * f / fsamp);
            a = (int)(b * fsamp / f + 0.5);
            d = fsamp * b / a - f; 
            break; 
	}
    }
    *aa = a;
    *bb = b;
}


void Pipewave::attgain (int n, float p)
{
    int    i, j, k;
    float  d, m, w, x, y, z;

    w = 0.05;
    x = 0.0;
    y = 0.6;
    if (p > 0) y += 0.11 * p;
    z = 0.0;
    j = 0;
    for (i = 1; i <= 24; i++)
    {
        k = n * i / 24;
        x =  1.0 - z - 1.5 * y;
        y += w * x;
        d = w * y * p / (k - j);
        while (j < k)
	{
            m = (double) j / n;
            _att [j++] = (1.0 - m) * z + m;
            z += d;
	}
    }
}


void Pipewave::save (FILE *F)
{
    int  k;
    union
    {
        int16_t i16 [16];
        int32_t i32 [8];
	float   flt [8];
    } d;

    d.i32 [0] = _l0;
    d.i32 [1] = _l1;
    d.i16 [4] = _k_s;
    d.i16 [5] = _k_r;
    d.flt [3] = _m_r;
    d.flt [4] = _d_r;
    d.flt [5] = _d_a;
    d.flt [6] = _d_w;
    d.i32 [7] = 0;
    fwrite (&d, 1, 32, F);
    k = _l0 +_l1 + _k_s * (PERIOD + 4);
    if (k > 0)
      fwrite (_p0.get(), k, sizeof (float), F);
}


void Pipewave::load (FILE *F)
{
    int  k;
    union
    {
        int16_t i16 [16];
        int32_t i32 [8];
	float   flt [8];
    } d;

    fread (&d, 1, 32, F);
    _l0  = d.i32 [0];
    _l1  = d.i32 [1];
    _k_s = d.i16 [4];
    _k_r = d.i16 [5];
    _m_r = d.flt [3];
    _d_r = d.flt [4];
    _d_a = d.flt [5];
    _d_w = d.flt [6];
    k = _l0 +_l1 + _k_s * (PERIOD + 4);
    _p0.reset();
    if (k > 0)
    {
      _p0 = std::make_unique <float []> (k);
      _p1 = _p0.get() + _l0;
      _p2 = _p1 + _l1;
      fread (_p0.get(), k, sizeof (float), F);
    }
}




Rankwave::Rankwave (int n0, int n1) : _n0 (n0), _n1 (n1), _list (0), _modif (false)
{
    _pipes = std::make_unique <Pipewave []> (n1 - n0 + 1);
}


#if REPETITION_POINTS
namespace
{

struct RepetitionPoint
{
    RepetitionPoint(int note, int num, int den)
    : note(note), num(num), den(den) {}
    int note, num, den;
};
      
std::forward_list <RepetitionPoint> ParseRepetitions(const char* s)
{
    char tag = '$';
    std::forward_list <RepetitionPoint> points;
    auto cur = points.before_begin();
    while (*s && *s != tag)
      ++s;
    if (*s)
      ++s;
    int note = 0, n = 0;
    char buf[64];
    while (*s && *s != tag && sscanf(s, "%d:%s%n", &note, buf, &n) > 0)
    {
      s += n;
      int wholes = 0, num = 0, den = 0;
      if (sscanf(buf, "%d+%d/%d", &wholes, &num, &den) != 3)
      {
        wholes = 0;
        if (sscanf(buf, "%d/%d", &num, &den) != 2)
        {
          den = 1;
          if (sscanf(buf, "%d", &num) != 1)
          {
            num = 0;
            fprintf (stderr, "Expected pitch written as 'a+b/c', got '%s'\n", buf);
          }
        }
      }
      cur = points.emplace_after (cur, note, num + wholes * den, den);
    }
    return points;
}

} // namespace
    
#endif // REPETITION_POINTS
    


void Rankwave::gen_waves (Addsynth *D, float fsamp, float fbase, float *scale)
{
    Pipewave::initstatic (fsamp);

#if REPETITION_POINTS
    float fn = D->_fn, fd = D->_fd,
          fbase_adj = fbase * D->_fn / (D->_fd * scale[9]);
    std::forward_list <RepetitionPoint> points = ParseRepetitions( D->_comments );
    auto p = points.begin();
    for (int i = _n0; i <= _n1; i++)
    {
        if( p != points.end() && i == p->note )
        {
          fbase_adj = 0;
          D->_fn = p->den * 8;
          D->_fd = p->num;
          if( D->_fn > 0 && D->_fd > 0 )
            fbase_adj = fbase * D->_fn / (D->_fd * scale[9]);
          ++p;
        }
        if( fbase_adj > 0 )
          _pipes [i - _n0].genwave (D, i - _n0, fsamp, ldexpf (fbase_adj * scale [i % 12], i / 12 - 5));
    }
    D->_fn = fn;
    D->_fd = fd;
#else
    fbase *=  D->_fn / (D->_fd * scale [9]);
    for (int i = _n0; i <= _n1; i++)
    {
	_pipes [i - _n0].genwave (D, i - _n0, fsamp, ldexpf (fbase * scale [i % 12], i / 12 - 5));
    }
#endif // REPETITION_POINTS
    _modif = true;
}


void Rankwave::set_param (float *out, int del, int pan)
{
    int         n, a, b;
    Pipewave   *P;

    _sbit = 1 << del;
    switch (pan)
    {
    case 'L': a = 2, b = 0; break;
    case 'C': a = 2, b = 1; break;
    case 'R': a = 2, b = 2; break;
    default:  a = 4, b = 0;
    }
    for (n = _n0, P = _pipes.get(); n <= _n1; n++, P++) P->_out = out + ((n % a) + b) * PERIOD;
}


void Rankwave::play (int shift)
{
    Pipewave *P, *Q;

    for (P = 0, Q = _list; Q; Q = Q->_link)
    {
	Q->play ();
        if (shift) Q->_sdel = (Q->_sdel >> 1) | Q->_sbit;
        if (Q->_sdel || Q->_p_p || Q->_p_r) P = Q;
        else
	{
  	    if (P) P->_link = Q->_link;
            else      _list = Q->_link;
	}
    }
}


int Rankwave::save (const char *path, Addsynth *D, float fsamp, float fbase, float *scale)
{
    FILE      *F;
    Pipewave  *P;
    int        i;
    char       name [1024];
    char       data [64];
    char      *p;

    sprintf (name, "%s/%s", path, D->_filename);
    if ((p = strrchr (name, '.'))) strcpy (p, ".ae1"); 
    else strcat (name, ".ae1");

    F = fopen (name, "wb");
    if (F == NULL) 
    {
	fprintf (stderr, "Can't open waveform file '%s' for writing\n", name);
        return 1;
    }
   
    std::fill_n (data, 16, 0);
    strcpy (data, "ae1");
    data [4] = 2;
    fwrite (data, 1, 16, F);

    std::fill_n (data, 64, 0);
    data [0] = 0;
    data [1] = 0;
    data [2] = 0;
    data [3] = 0;
    data [4] = _n0;
    data [5] = _n1;
    data [6] = 0;
    data [7] = 0;
    *((float *)(data +  8)) = fsamp;
    *((float *)(data + 12)) = fbase;
    std::copy_n (scale, 12, reinterpret_cast<float *>(data + 16));
    fwrite (data, 1, 64, F);

    for (i = _n0, P = _pipes.get(); i <= _n1; i++, P++) P->save (F);

    fclose (F);

    _modif = false;
    return 0;
}


int Rankwave::load (const char *path, Addsynth *D, float fsamp, float fbase, float *scale)
{
    FILE      *F;
    Pipewave  *P;
    int        i;
    char       name [1024];
    char       data [64];
    char      *p;
    float      f;

    sprintf (name, "%s/%s", path, D->_filename);
    if ((p = strrchr (name, '.'))) strcpy (p, ".ae1"); 
    else strcat (name, ".ae1");

    F = fopen (name, "rb");
    if (F == NULL) 
    {
#ifdef DEBUG
	fprintf (stderr, "Can't open waveform file '%s' for reading\n", name);
#endif
        return 1;
    }

    fread (data, 1, 16, F);
    if (strcmp (data, "ae1"))
    {
#ifdef DEBUG
	fprintf (stderr, "File '%s' is not an Aeolus waveform file\n", name);
#endif
        fclose (F);
        return 1;
    }

    if (data [4] != 2)
    {
#ifdef DEBUG
	fprintf (stderr, "File '%s' has an incompatible version tag (%d)\n", name, data [4]);
#endif
        fclose (F);
        return 1;
    }

    fread (data, 1, 64, F);
    if (_n0 != data [4] || _n1 != data [5])
    {
#ifdef DEBUG
	fprintf (stderr, "File '%s' has an incompatible note range (%d %d), (%d %d)\n", name, _n0, _n1, data [4], data [5]);
#endif
        fclose (F);
        return 1;
    }

    f = *((float *)(data + 8));
    if (fabsf (f - fsamp) > 0.1f)
    {
#ifdef DEBUG
	fprintf (stderr, "File '%s' has a different sample frequency (%3.1lf)\n", name, f);
#endif
        fclose (F);
        return 1;
    }

    f = *((float *)(data + 12));
    if (fabsf (f - fbase) > 0.1f)
    {
#ifdef DEBUG
	fprintf (stderr, "File '%s' has a different tuning (%3.1lf)\n", name, f);
#endif
        fclose (F);
        return 1;
    }

    for (i = 0; i < 12; i++)
    {
        f = *((float *)(data + 16 + 4 * i));
        if (fabsf (f /  scale [i] - 1.0f) > 6e-5f)
        {
#ifdef DEBUG
	    fprintf (stderr, "File '%s' has a different temperament\n", name);
#endif
            fclose (F);
            return 1;
        }
    }

    for (i = _n0, P = _pipes.get(); i <= _n1; i++, P++) P->load (F);
  
    fclose (F);

    _modif = false;
    return 0;
}
