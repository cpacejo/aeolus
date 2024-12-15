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
#include "imidi.h"

Imidi::Imidi (Lfq_u32 *qnote, Lfq_u8 *qmidi, uint16_t *midimap, const char *appname) :
    A_thread ("Imidi"),
    _appname (appname),
    _client(0),
    _ipport(0),
    _qnote(qnote),
    _qmidi(qmidi),
    _midimap (midimap)
{
}


Imidi::~Imidi (void)
{
}

void Imidi::open_midi (void)
{
    on_open_midi();

    M_midi_info *M = new M_midi_info ();
    M->_client = _client;
    M->_ipport = _ipport;
    std::copy_n (_midimap, 16, M->_chbits);
    send_event (TO_MODEL, M);
}


void Imidi::close_midi (void)
{
    on_close_midi();
}

void Imidi::terminate()
{
    on_terminate();
}

void Imidi::proc_midi_event(const MidiEvent &ev)
{
    int              c, f, k, t, n, v, p;

    c = ev.note.channel;
    k = _midimap [c] & 15;
    f = _midimap [c] >> 12;
    t = ev.type;

    switch (t)
	{ 
	case SND_SEQ_EVENT_NOTEON:
	case SND_SEQ_EVENT_NOTEOFF:
	    n = ev.note.note;
	    v = ev.note.velocity;
            if ((t == SND_SEQ_EVENT_NOTEON) && v)
	    {
                // Note on.
  	        if (n < 36)
	        {
                    if ((f & 4) && (n >= 24) && (n < 34))
		    {
			// Preset selection, sent to model thread
			// if on control-enabled channel.
 		        if (_qmidi->write_avail () >= 3)
		        {
		            _qmidi->write (0, 0x90);
		            _qmidi->write (1, n);
		            _qmidi->write (2, v);
		            _qmidi->write_commit (3);
			}
	            }
	        }
                else if (n <= 96)
	        {
                    if (f & 1)
		    {
	                if (_qnote->write_avail () > 0)
	                {
	                    _qnote->write (0, (static_cast<int>(command::key_on) << 24) | ((n - 36) << 16) | k);
                            _qnote->write_commit (1);
	                } 
		    }
		}
            }
            else
	    {
                // Note off.
  	        if (n < 36)
	        {
	        }
                else if (n <= 96)
	        {
                    if (f & 1)
		    {
	                if (_qnote->write_avail () > 0)
	                {
	                    _qnote->write (0, (static_cast<int>(command::key_off) << 24) | ((n - 36) << 16) | k);
                            _qnote->write_commit (1);
	                } 
		    }
		}
	    }
	    break;

	case SND_SEQ_EVENT_CONTROLLER:
	    p = ev.control.param;
	    v = ev.control.value;
	    switch (static_cast<midictl>(p))
	    {
	    case midictl::hold:
		// Hold pedal.
                if (f & 1)
                {
                    c = static_cast<int>((v > 63) ? command::hold_on : command::hold_off);
                    if (_qnote->write_avail () > 0)
                    {
                        _qnote->write (0, (c << 24) | k);
                        _qnote->write_commit (1);
                    }
		}
		break;

	    case midictl::asoff:
		// All sound off, accepted on control channels only.
		// Clears all keyboards, including held notes.
		if (f & 4)
		{
	            if (_qnote->write_avail () > 0)
	            {
	                _qnote->write (0, (static_cast<int>(command::midi_off) << 24) | NKEYBD);
                        _qnote->write_commit (1);
	            } 
		}
		break;

            case midictl::anoff:
		// All notes off, accepted on channels controlling
		// a keyboard. Does not clear held notes. 
		if (f & 1)
		{
	            if (_qnote->write_avail () > 0)
	            {
	                _qnote->write (0, (static_cast<int>(command::midi_off) << 24) | k);
                        _qnote->write_commit (1);
	            } 
		}
                break;

	    case midictl::bank:
	    case midictl::ifelm:
                // Program bank selection or stop control, sent
                // to model thread if on control-enabled channel.
		if (f & 4)
		{
		    if (_qmidi->write_avail () >= 3)
		    {
			_qmidi->write (0, 0xB0 | c);
			_qmidi->write (1, p);
			_qmidi->write (2, v);
			_qmidi->write_commit (3);
		    }
		}
		break;

	    case midictl::swell:
	    case midictl::tfreq:
	    case midictl::tmodd:
		// Per-division performance controls, sent to model
                // thread if on a channel that controls a division.
		if (f & 2)
		{
		    if (_qmidi->write_avail () >= 3)
		    {
			_qmidi->write (0, 0xB0 | c);
			_qmidi->write (1, p);
			_qmidi->write (2, v);
			_qmidi->write_commit (3);
		    }
		}
		break;

	    case midictl::cresc:
	    case midictl::volume:
	    case midictl::sfz:
		// Instrument-wide and per-asection performance controls,
		// accepted on control channels only.
		if (f & 4)
		{
		    if (_qmidi->write_avail () >= 3)
		    {
			_qmidi->write (0, 0xB0 | c);
			_qmidi->write (1, p);
			_qmidi->write (2, v);
			_qmidi->write_commit (3);
		    }
		}
		break;

	    default:
		break;
	    }
	    break;

	case SND_SEQ_EVENT_PGMCHANGE:
            // Program change sent to model thread
            // if on control-enabled channel.
	    if (f & 4)
	    {
   	        if (_qmidi->write_avail () >= 3)
	        {
		    _qmidi->write (0, 0xC0);
		    _qmidi->write (1, ev.control.value);
		    _qmidi->write (2, 0);
		    _qmidi->write_commit (3);
		}
            }
	    break;

    }
}

