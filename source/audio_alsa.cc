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


#include <atomic>
#include <memory>
#include "audio_alsa.h"
#include "messages.h"

Audio_alsa::Audio_alsa (const char *name, Lfq_u32 *qnote, Lfq_u32 *qcomm, const char *device, int fsamp, int fsize, int nfrag) :
    Audio(name, qnote, qcomm)
{
    init(device, fsamp, fsize, nfrag);
}


Audio_alsa::~Audio_alsa ()
{
    if (_alsa_handle) close ();
}

void Audio_alsa::init (const char *device, int fsamp, int fsize, int nfrag)
{
    _alsa_handle = std::make_unique <Alsa_pcmi> (device, nullptr, nullptr, fsamp, fsize, nfrag);
    if (_alsa_handle->state () < 0)
    {
        fprintf (stderr, "Error: can't connect to ALSA.\n");
        exit (1);
    } 
    _nplay = _alsa_handle->nplay ();
    _fsize = fsize;
    _fsamp = fsamp;
    if (_nplay > 2) _nplay = 2;
    init_audio ();
    _outbuf_storage = std::make_unique <float []> (_nplay * fsize);
    for (int i = 0; i < _nplay; i++) _outbuf [i] = &_outbuf_storage [i * fsize];
    _running.store (true, std::memory_order_relaxed);
    if (thr_start (_policy = SCHED_FIFO, _relpri = -20, 0))
    {
        fprintf (stderr, "Warning: can't run ALSA thread in RT mode.\n");
        if (thr_start (_policy = SCHED_OTHER, _relpri = 0, 0))
        {
            fprintf (stderr, "Error: can't create ALSA thread.\n");
            exit (1);
	}
    }
}


void Audio_alsa::close ()
{
    _running.store (false, std::memory_order_relaxed);
    get_event (1 << EV_EXIT);
}


void Audio_alsa::thr_main (void)
{
    unsigned long k;

    _alsa_handle->pcm_start ();

    while (_running.load (std::memory_order_relaxed))
    {
	k = _alsa_handle->pcm_wait ();  
        proc_queue (_qnote);
        proc_queue (_qcomm);
        proc_keys1 ();
        proc_keys2 ();
        while (k >= _fsize)
       	{
            proc_synth (_fsize);
            _alsa_handle->play_init (_fsize);
            for (int i = 0; i < _nplay; i++) _alsa_handle->play_chan (i, _outbuf [i], _fsize);
            _alsa_handle->play_done (_fsize);
            k -= _fsize;
	}
        proc_mesg ();
    }

    _alsa_handle->pcm_stop ();
    put_event (EV_EXIT);
}
