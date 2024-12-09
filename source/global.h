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


#ifndef __GLOBAL_H
#define __GLOBAL_H

#include <cstdint>
#include <endian.h>
#ifdef __BYTE_ORDER
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define WR2(p,v) { (p)[0] = v; (p)[1] = v >> 8; }
#define WR4(p,v) { (p)[0] = v; (p)[1] = v >> 8;  (p)[2] = v >> 16;  (p)[3] = v >> 24; }
#define RD2(p) ((p)[0] + ((p)[1] << 8));
#define RD4(p) ((p)[0] + ((p)[1] << 8) + ((p)[2] << 16) + ((p)[3] << 24));
#elif (__BYTE_ORDER == __BIG_ENDIAN)
#define WR2(p,v) { (p)[1] = v; (p)[0] = v >> 8; }
#define WR4(p,v) { (p)[3] = v; (p)[2] = v >> 8;  (p)[1] = v >> 16;  (p)[0] = v >> 24; }
#define RD2(p) ((p)[1] + ((p)[0] << 8));
#define RD4(p) ((p)[3] + ((p)[2] << 8) + ((p)[1] << 16) + ((p)[0] << 24));
#else
#error Byte order is not supported !
#endif
#else
#error Byte order is undefined !
#endif

#include "lfqueue.h"


// GLOBAL LIMITS
static constexpr int
    NASECT = 4,
    NDIVIS = 8,
    NKEYBD = 8,
    NGROUP = 8,
    NRANKS = 32,
    NNOTES = 61,
    NBANK  = 32,
    NPRES  = 32,
    NLINKS = 3;  // maximum number of linkages to a rank (e.g. drawstop + Sfz + GC)


enum class midictl: std::uint8_t
{
    cresc = 4,  // grand crescendo / foot controller
    volume = 7,  // instrument volume / channel volume
    swell = 11,  // swell / expression controller
    bank  = 32,  // bank LSB
    hold  = 64,  // damper pedal (sustain)
    ifelm = 80,  // stop control / general purpose controller 5
    tfreq = 76,  // tremulant frequency / sound controller 7 / vibrato rate
    tmodd = 92,  // tremulant amplitude / effects 2 depth / tremolo depth
    asoff = 120, // all sound off
    anoff = 123  // all notes off
};

enum class command: std::uint8_t
{
    key_off = 0,  // single key off
    key_on,       // single key on
    midi_off,     // all notes off
    unused3,
    clr_div_mask,   // clear bit in division mask
    set_div_mask,   // set bit in division mask
    clr_rank_mask,  // clear bit in rank mask
    set_rank_mask,  // set bit in rank mask
    hold_off,  // hold off
    hold_on,   // hold on

    set_tremul = 16,  // tremulant on/off
    set_dipar  // per-division performance controllers
};

enum class dipar
{
    swell = 0,  // swell
    tfreq,      // tremulant frequency
    tmodd       // tremulant amplitude
};

static constexpr int
    NDIPAR = 3,
    NAUPAR = 5;

#define SWELL_MIN 0.0f
#define SWELL_MAX 1.0f
#define SWELL_DEF 1.0f

#define TFREQ_MIN 2.0f
#define TFREQ_MAX 8.0f
#define TFREQ_DEF 4.0f

#define TMODD_MIN 0.0f
#define TMODD_MAX 0.6f
#define TMODD_DEF 0.3f

#define VOLUME_MIN 0.0f
#define VOLUME_MAX 1.0f
#define VOLUME_DEF ((100.0f / 127.0f) * (100.0f / 127.0f))  // per MIDI spec


static constexpr int CRESC_BANK = NBANK - 1;
static constexpr int CRESC_LINKAGE = 1;


static constexpr int
    KMAP_SET = 1 << NKEYBD,  // Set if keymap entry is modified.
    KMAP_ALL = (1 << NKEYBD) - 1;


class Fparm
{
public:

    float  _val;
    float  _min;
    float  _max;
};


#endif

