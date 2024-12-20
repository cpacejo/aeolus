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


#ifndef __AUDIO_H
#define __AUDIO_H

#include <atomic>
#include <memory>
#include <stop_token>
#include "asection.h"
#include "division.h"
#include "lfqueue.h"
#include "reverb.h"
#include "global.h"
#include <clthreads.h>
#if LIBSPATIALAUDIO_VERSION
#include <spatialaudio/AmbisonicBinauralizer.h>
#include <spatialaudio/BFormat.h>
#endif


#if LIBSPATIALAUDIO_VERSION
class CBFormatEnh : public CBFormat
{
public:
    float* RefStream (const unsigned nChannel)
    {
        return m_ppfChannels [nChannel];
    }
};
#endif


class Audio : public A_thread
{
public:

    Audio (const char *jname, Lfq_u32 *qnote, Lfq_u32 *qcomm);
    virtual ~Audio ();
    void  start (void);

    const char  *appname (void) const { return _appname; }
    uint16_t    *midimap (void) const { return (uint16_t *) _midimap; }
    int  policy (void) const { return _policy; }
    int  abspri (void) const { return _abspri; }
    int  relpri (void) const { return _relpri; }
    
    enum { VOLUME, REVSIZE, REVTIME, STPOSIT };

protected:

    void init_audio (bool binaural);

    void proc_queue (Lfq_u32 *);
    void proc_synth (int);
    void proc_keys1 (void);
    void proc_keys2 (void);
    void proc_mesg (void);
    
    virtual void on_synth_period(int) {}

    void key_off (int i, int b)
    {
        _keymap [i] &= ~b;
        _keymap [i] |= KMAP_SET;
    }

    void key_on (int i, int b)
    {
        _keymap [i] |= b | KMAP_SET;
    }

    void cond_key_off (int m, int b)
    {
	int       i;
	uint16_t  *p;

	for (i = 0, p = _keymap; i < NNOTES; i++, p++)
	{
            if (*p & m)
	    {
                *p &= ~b;
		*p |= KMAP_SET;
	    }
	}
    }

    void cond_key_on (int m, int b)
    {
	int       i;
	uint16_t  *p;

	for (i = 0, p = _keymap; i < NNOTES; i++, p++)
	{
            if (*p & m)
	    {
                *p |= b | KMAP_SET;
	    }
	}
    }

    const char     *_appname;
    uint16_t        _midimap [16];
    Lfq_u32        *_qnote; 
    Lfq_u32        *_qcomm; 
    std::stop_source _running;

    int             _policy;
    int             _abspri;
    int             _relpri;
    int             _hold;
    int             _nplay;
    unsigned int    _fsamp;
    unsigned int    _fsize;
    bool            _bform;
    bool            _binaural;
    int             _nasect;
    int             _ndivis;
    std::unique_ptr <Asection> _asectp [NASECT];
    std::unique_ptr <Division> _divisp [NDIVIS];
    Reverb          _reverb;
    float          *_outbuf [8];
    std::unique_ptr <float[]> _outbuf_storage;
    uint16_t        _keymap [NNOTES];
    Fparm           _audiopar [NAUPAR];
    float           _revsize;
    float           _revtime;
#if LIBSPATIALAUDIO_VERSION
    CBFormatEnh _binauralizer_src;
    CAmbisonicBinauralizer _binauralizer;
#endif
};


#endif

