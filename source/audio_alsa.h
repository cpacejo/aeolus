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


#ifndef __AUDIO_ALSA_H
#define __AUDIO_ALSA_H

#include <memory>
#include "audio.h"
#include <zita-alsa-pcmi.h>

class Audio_alsa : public Audio
{
public:

    Audio_alsa (const char *jname, Lfq_u32 *qnote, Lfq_u32 *qcomm, const char *device, int fsamp, int fsize, int nfrag, bool binaural);
    virtual ~Audio_alsa ();

private:

    void  init (const char *device, int fsamp, int fsize, int nfrag, bool binaural);
    void close (void);
    virtual void thr_main (void);

    std::unique_ptr <Alsa_pcmi> _alsa_handle;
};


#endif

