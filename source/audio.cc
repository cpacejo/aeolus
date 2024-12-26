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
#include <math.h>
#include <memory>
#include <numbers>
#include <stop_token>
#include <utility>
#include "audio.h"
#include "global.h"
#include "messages.h"
#if LIBSPATIALAUDIO_VERSION
#include <spatialaudio/BFormat.h>
#endif



Audio::Audio (const char *name, Lfq_u32 *qnote, Lfq_u32 *qcomm) :
    A_thread("Audio"),
    _appname (name),
    _qnote (qnote),
    _qcomm (qcomm),
    _running (std::nostopstate),
    _abspri (0),
    _relpri (0),
    _nplay (0),
    _fsamp (0),
    _fsize (0),
    _bform (0),
    _binaural (0),
    _nasect (0),
    _ndivis (0)
{
}


Audio::~Audio ()
{
}


void Audio::init_audio (bool binaural)
{
    int i;

#if LIBSPATIALAUDIO_VERSION
    _binaural = binaural;
#endif
    
    _audiopar [VOLUME]._val = VOLUME_DEF;
    _audiopar [VOLUME]._min = VOLUME_MIN;
    _audiopar [VOLUME]._max = VOLUME_MAX;
    _audiopar [REVSIZE]._val = _revsize = 0.075f;
    _audiopar [REVSIZE]._min =  0.025f;
    _audiopar [REVSIZE]._max =  0.150f;
    _audiopar [REVTIME]._val = _revtime = 4.0f;
    _audiopar [REVTIME]._min =  2.0f;
    _audiopar [REVTIME]._max =  7.0f;
    // -1 -> mono (no Y)
    // -1 .. 0 -> amount to reduce Y by
    // 0 -> stereo (Y = W)
    // 0 .. 1 -> superstereo
    // magnitude is sine squared of half angle to compress or expand sound stage by
    // (so 0.75 expands sound stage by +120°, thus reproducing 180° sound stage on 60° separated speakers)
    _audiopar [STPOSIT]._val =  0.0f;
    _audiopar [STPOSIT]._min = -1.0f;
    _audiopar [STPOSIT]._max =  1.0f;

    _reverb = Reverb (_fsamp);
    _reverb.set_t60mf (_revtime);
    _reverb.set_t60lo (_revtime * 1.50f, 250.0f);
    _reverb.set_t60hi (_revtime * 0.50f, 3e3f);

    _nasect = NASECT;
    for (i = 0; i < NASECT; i++)
    {
        _asectp [i] = std::make_unique <Asection> ((float) _fsamp);
        _asectp [i]->set_size (_revsize);
    }
    _hold = KMAP_ALL;

#if LIBSPATIALAUDIO_VERSION
    if (_binaural)
    {
        if (!_binauralizer_src.Configure (1, true, _fsize))
        {
            fprintf (stderr, "Unable to configure binauralizer buffer; disabling.\n");
            _binaural = false;
        }
        else
        {
            unsigned tailLength;
            if (!_binauralizer.Configure (1, true, _fsamp, _fsize, tailLength, ""))
            {
                fprintf (stderr, "Unable to configure binauralizer; disabling.\n");
                _binaural = false;
            }
        #if LIBSPATIALAUDIO_VERSION < 0x0301
            else
            {
                // starting with libspatialaudio 0.3.1 this is no longer a limitation
                if (tailLength > _fsize)
                {
                    fprintf (stderr, "Audio period size too small for binauralizer (have %d, need >= %d); disabling.\n",
                        _fsize, tailLength);
                    _binaural = false;
                }
            }
        #endif
        }
    }
#endif
}


void Audio::start (void)
{
    M_audio_info  *M;
    int           i;

    M = new M_audio_info ();
    M->_nasect = _nasect;
    M->_fsamp  = _fsamp;
    M->_fsize  = _fsize;
    M->_instrpar = _audiopar;
    for (i = 0; i < _nasect; i++) M->_asectpar [i] = _asectp [i]->get_apar ();
    send_event (TO_MODEL, M);
}

void Audio::proc_queue (Lfq_u32 *Q)
{
    int       c, i, j, k, n;
    uint32_t  q;
    uint16_t  m;
    union     { uint32_t i; float f; } u;

    // Execute commands from the model thread (qcomm),
    // or from the midi thread (qnote).

    n = Q->read_avail ();
    while (n > 0)
    {
	q = Q->read (0);
        c = (q >> 24) & 255;  // command    
        i = (q >> 16) & 255;  // key, rank or parameter index
        j = (q >>  8) & 255;  // division index
        k = q & 255;          // keyboard index

        switch (static_cast<command>(c))
	{
	case command::key_off:
	    // Single key off.
            key_off (i, 1 << k);
	    Q->read_commit (1);
	    break;

	case command::key_on:
	    // Single key on.
            key_on (i, 1 << k);
	    Q->read_commit (1);
	    break;

	case command::midi_off:
	    // All notes off.
            m = (k == NKEYBD) ? KMAP_ALL : (1 << k);
	    cond_key_off (m, m);
	    Q->read_commit (1);
	    break;

	case command::unused3:
	    Q->read_commit (1);
	    break;

        case command::clr_div_mask:
	    // Clear bit in division mask.
            _divisp [j]->clr_div_mask (k & 0xf, k >> 4);
	    Q->read_commit (1);
            break;

        case command::set_div_mask:
	    // Set bit in division mask.
            _divisp [j]->set_div_mask (k & 0xf, k >> 4);
	    Q->read_commit (1);
            break;

        case command::clr_rank_mask:
	    // Clear bit in rank mask.
            _divisp [j]->clr_rank_mask (i, k & 0xf, k >> 4);
	    Q->read_commit (1);
            break;

        case command::set_rank_mask:
	    // Set bit in rank mask.
            _divisp [j]->set_rank_mask (i, k & 0xf, k >> 4);
	    Q->read_commit (1);
            break;

        case command::hold_off:
	    // Hold off.
            printf ("HOLD OFF %d", k);
//            _hold = KMAP_ALL;
//             cond_key_off (KMAP_HLD, KMAP_HLD);
            Q->read_commit (1);
	    break;

        case command::hold_on:
	    // Hold on.
            printf ("HOLD ON  %d", k);
//            _hold = KMAP_ALL | KMAP_HLD;
//            cond_key_on (, KMAP_HLD);
            Q->read_commit (1);
	    break;

        case command::set_tremul:
	    // Tremulant on/off.
            if ((k & 0xf) != 0) _divisp [j]->trem_on (k >> 4);
            else   _divisp [j]->trem_off (k >> 4);
	    Q->read_commit (1);
            break;

        case command::set_dipar:
	    // Per-division performance controllers.
	    if (n < 2) return;
            u.i = Q->read (1);
            Q->read_commit (2);        
            switch (static_cast<dipar>(i))
 	    {
            case dipar::swell: _divisp [j]->set_swell (u.f); break;
            case dipar::tfreq: _divisp [j]->set_tfreq (u.f); break;
            case dipar::tmodd: _divisp [j]->set_tmodd (u.f); break;
	    }
            break;
            
        default:
            Q->read_commit (1);
	}
        n = Q->read_avail ();
    }
}


void Audio::proc_keys1 (void)
{    
    int       d, n;
    uint16_t  m;

    for (n = 0; n < NNOTES; n++)
    {
	m = _keymap [n];
	if (m & KMAP_SET)
	{
            m ^= KMAP_SET;
   	    _keymap [n] = m;
            for (d = 0; d < _ndivis; d++)
            {
                _divisp [d]->update (n, m & KMAP_ALL);
            }
	}
    }
}


void Audio::proc_keys2 (void)
{    
    int d;

    for (d = 0; d < _ndivis; d++)
    {
        _divisp [d]->update (_keymap);
    }
}


void Audio::proc_synth (int nframes) 
{
    int           j, k;
    float         W [PERIOD];
    float         X [PERIOD];
    float         Y [PERIOD];
    float         Z [PERIOD];
    float         R [PERIOD];
    float        *out [8];

    if (fabsf (_revsize - _audiopar [REVSIZE]._val) > 0.001f)
    {
        _revsize = _audiopar [REVSIZE]._val;
	_reverb.set_delay (_revsize);
        for (j = 0; j < _nasect; j++) _asectp[j]->set_size (_revsize);
    }
    if (fabsf (_revtime - _audiopar [REVTIME]._val) > 0.1f)
    {
        _revtime = _audiopar [REVTIME]._val;
 	_reverb.set_t60mf (_revtime);
 	_reverb.set_t60lo (_revtime * 1.50f, 250.0f);
 	_reverb.set_t60hi (_revtime * 0.50f, 3e3f);
    }

#if LIBSPATIALAUDIO_VERSION < 0x0301
    // spatialaudio < 0.3.1 does not support variable-size frames;
    // permanently disable binauralization if the frame size ever differs
    if (_binaural && static_cast<unsigned>(nframes) != _fsize) _binaural = false;
#endif

    // mid+side virtual microphone balance
    float WXk = 0.0f, Yk = 0.0f;  // only used in stereo mode; configured below

    if (_bform)
    {
        // no initialization required
    }
#if LIBSPATIALAUDIO_VERSION
    if (_binaural)
    {
        for (j = 0; j < 4; j++) out [j] = _binauralizer_src.RefStream (j);
    }
    else
#endif
    {
        // stereo; mid+side micing

        const float s2 = _audiopar [STPOSIT]._val;

        if (s2 <= 0.0f)
        {
            // "compression"
            const float s = sqrtf (-s2);
            const float c = sqrtf (1.0f + s2);
            WXk = 0.5f * (1.0f + s);
            Yk = 0.5f * c;
        }
        else
        {
            // "expansion"
            const float s = sqrtf (s2);
            const float c = sqrtf (1.0f - s2);
            // compute mid+side coefficients for an idempotent projection
            // for the given sound stage expansion (given as sine of angle with X)
            // (note that WXk is half the mid gain, since it combines W and X as a cardioid)
            WXk = 0.5f / (1.0f + s);
            Yk = std::min (4.0f, 0.5f / c);  // limit to maximum gain of +12 dB (~166 deg expansion)
        }

        for (j = 0; j < _nplay; j++) out [j] = _outbuf [j];
    }

    for (k = 0; k < nframes; k += PERIOD)
    {
        on_synth_period(k);

        std::fill_n (W, PERIOD, 0);
        std::fill_n (X, PERIOD, 0);
        std::fill_n (Y, PERIOD, 0);
        std::fill_n (Z, PERIOD, 0);
        std::fill_n (R, PERIOD, 0);

        for (j = 0; j < _ndivis; j++) _divisp [j]->process ();
        for (j = 0; j < _nasect; j++) _asectp [j]->process (_audiopar [VOLUME]._val, W, X, Y, R);
        _reverb.process (PERIOD, _audiopar [VOLUME]._val, R, W, X, Y, Z);

        // Note that W does *not* have a -3 dB adjustment applied to it.
        // (Most literature assumes it does.)

        if (_bform)
	{
            for (j = 0; j < PERIOD; j++)
            {
                // by default, we output FuMa (i.e. WXYZ) order and maxN* normalization
                // (i.e. W is adjusted by -3 dB)
	        out [0][j] = (0.5f * std::numbers::sqrt2_v<float>) * W [j];
	        out [1][j] = X [j];
	        out [2][j] = Y [j];
	        out [3][j] = Z [j];
   	    }
            for (j = 0; j < 4; j++) out [j] += PERIOD;
	}
#if LIBSPATIALAUDIO_VERSION
        else if (_binaural)
	{
            // libspatialaudio uses ACN (i.e. WYZX) order and SN3D normalization
            // (i.e. W is not adjusted)
            std::copy_n (W, PERIOD, out [0]);
            std::copy_n (Y, PERIOD, out [1]);
            std::copy_n (Z, PERIOD, out [2]);
            std::copy_n (X, PERIOD, out [3]);
            for (j = 0; j < 4; j++) out [j] += PERIOD;
        }
#endif
        else
        {
            for (j = 0; j < PERIOD; j++)
            {
                const float mid = WXk * (W [j] + X [j]);
                const float side = Yk * Y [j];
                out [0][j] = mid + side;
                out [1][j] = mid - side;
   	    }
            for (j = 0; j < 2; j++) out [j] += PERIOD;
        }
    }

#if LIBSPATIALAUDIO_VERSION
#if LIBSPATIALAUDIO_VERSION >= 0x0301
    if (_binaural) _binauralizer.Process (&_binauralizer_src, _outbuf, nframes);
#else
    if (_binaural) _binauralizer.Process (&_binauralizer_src, _outbuf);
#endif
#endif
}


void Audio::proc_mesg (void) 
{
    ITC_mesg *M;

    while (get_event_nowait () != EV_TIME)
    {
	M = get_message ();
        if (! M) continue; 

        switch (M->type ())
	{
	    case MT_NEW_DIVIS:
	    {
	        M_new_divis  *X = (M_new_divis *) M;
                std::unique_ptr <Division> D = std::make_unique <Division> (_asectp [X->_asect].get (), (float) _fsamp);
                D->set_div_mask (X->_keybd);
                D->set_swell (X->_swell);
                D->set_tfreq (X->_tfreq);
                D->set_tmodd (X->_tmodd);
                _divisp [_ndivis] = std::move (D);
                _ndivis++;
                break; 
	    }
	    case MT_CALC_RANK:
	    case MT_LOAD_RANK:
	    {
	        M_def_rank *X = (M_def_rank *) M;
                _divisp [X->_divis]->set_rank (X->_rank, std::unique_ptr <Rankwave> (X->_rwave), X->_synth->_pan, X->_synth->_del);
                send_event (TO_MODEL, M);
                M = 0;
	        break;
	    }
	    case MT_AUDIO_SYNC:
                send_event (TO_MODEL, M);
                M = 0;
		break;
	} 
        if (M) M->recover ();
    }
}
