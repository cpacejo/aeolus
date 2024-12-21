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

#include <math.h>
#include "asection.h"
#include "audio.h"
#include "audiowin.h"
#include "callbacks.h"
#include "styles.h"


Audiowin::Audiowin (X_window *parent, X_callback *callb, int xp, int yp, X_resman *xresm) :
    X_window (parent, xp, yp, UISCALE(200), UISCALE(100), Colors.main_bg),
    _callb (callb),
    _xresm (xresm),
    _xp (xp),
    _yp (yp)
{
    _atom = XInternAtom (dpy (), "WM_DELETE_WINDOW", True);
    XSetWMProtocols (dpy (), win (), &_atom, 1);
    _atom = XInternAtom (dpy (), "WM_PROTOCOLS", True);
}


Audiowin::~Audiowin (void)
{
}


void Audiowin::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case ClientMessage:
        handle_xmesg ((XClientMessageEvent *) E);
        break;
    }
}


void Audiowin::handle_xmesg (XClientMessageEvent *E)
{
    if (E->message_type == _atom) x_unmap ();
}


void Audiowin::handle_callb (int k, X_window *W, XEvent *E)
{
    int c;

    switch (k)
    {
        case SLIDER | X_slider::MOVE: 
        case SLIDER | X_slider::STOP: 
        {
            X_slider *X = (X_slider *) W;
            c = X->cbid ();
            _asect = (c >> ASECT_BIT0) - 1;
            _parid = c & ASECT_MASK;
            _value = X->get_val (); 
            _final = k == (X_callback::SLIDER | X_slider::STOP);
            _callb->handle_callb (CB_AUDIO_ACT, this, E);
            break; 
	}
    }
}


void Audiowin::setup (M_ifc_init *M)
{
    int      i, j, k, x;
    char     s [256];
    Asect    *S; 
    X_hints  H;
    
    but1.size.x = UISCALE(20);
    but1.size.y = UISCALE(20);
    _nasect = M->_nasect;
    for (i = 0; i < _nasect; i++)
    {
	S = _asectd + i;
        x = UISCALE(XOFFS + XSTEP * i);
        k = ASECT_STEP * (i + 1);

        (S->_slid [Asection::AZIMUTH] = new X_hslider (this, this, &sli1, &sca_azim, x, UISCALE( 40), UISCALE(20), k + Asection::AZIMUTH))->x_map ();
        (S->_slid [Asection::STWIDTH] = new X_hslider (this, this, &sli1, &sca_difg, x, UISCALE( 75), UISCALE(20), k + Asection::STWIDTH))->x_map ();
        (S->_slid [Asection::DIRECT]  = new X_hslider (this, this, &sli1, &sca_dBsh, x, UISCALE(110), UISCALE(20), k + Asection::DIRECT))->x_map ();
        (S->_slid [Asection::REFLECT] = new X_hslider (this, this, &sli1, &sca_dBsh, x, UISCALE(145), UISCALE(20), k + Asection::REFLECT))->x_map ();
        (S->_slid [Asection::REVERB]  = new X_hslider (this, this, &sli1, &sca_dBsh, x, UISCALE(180), UISCALE(20), k + Asection::REVERB))->x_map ();
        (new X_hscale (this, &sca_azim, x, UISCALE( 30), UISCALE(10)))->x_map ();
        (new X_hscale (this, &sca_difg, x, UISCALE( 65), UISCALE(10)))->x_map ();
        (new X_hscale (this, &sca_dBsh, x, UISCALE(133), UISCALE(10)))->x_map ();
        (new X_hscale (this, &sca_dBsh, x, UISCALE(168), UISCALE(10)))->x_map ();
        S->_label [0] = 0;
        for (j = 0; j <  M->_ndivis; j++)
	{
	    if (M->_divisd [j]._asect == i)
	    {
		if (S->_label [0]) strcat (S->_label, " + ");
                strcat (S->_label, M->_divisd [j]._label);
                add_text (x, UISCALE(5), UISCALE(200), UISCALE(20), S->_label, &text0);
	    } 
	}
    }
    add_text (UISCALE( 10), UISCALE( 40), UISCALE(60), UISCALE(20), "Azimuth", &text0);
    add_text (UISCALE( 10), UISCALE( 75), UISCALE(60), UISCALE(20), "Width",   &text0);
    add_text (UISCALE( 10), UISCALE(110), UISCALE(60), UISCALE(20), "Direct ", &text0);
    add_text (UISCALE( 10), UISCALE(145), UISCALE(60), UISCALE(20), "Reflect", &text0);
    add_text (UISCALE( 10), UISCALE(180), UISCALE(60), UISCALE(20), "Reverb",  &text0);

    (_slid [Audio::VOLUME]  = new X_hslider (this, this, &sli1, &sca_dBsh, UISCALE(520), UISCALE(275), UISCALE(20), Audio::VOLUME))->x_map ();
    (_slid [Audio::REVSIZE] = new X_hslider (this, this, &sli1, &sca_size, UISCALE( 70), UISCALE(240), UISCALE(20), Audio::REVSIZE))->x_map ();
    (_slid [Audio::REVTIME] = new X_hslider (this, this, &sli1, &sca_trev, UISCALE( 70), UISCALE(275), UISCALE(20), Audio::REVTIME))->x_map ();
    (_slid [Audio::STPOSIT] = new X_hslider (this, this, &sli1, &sca_spos, UISCALE(305), UISCALE(275), UISCALE(20), Audio::STPOSIT))->x_map ();
    (new X_hscale (this, &sca_size, UISCALE( 70), UISCALE(230), UISCALE(10)))->x_map ();
    (new X_hscale (this, &sca_trev, UISCALE( 70), UISCALE(265), UISCALE(10)))->x_map ();
    (new X_hscale (this, &sca_spos, UISCALE(305), UISCALE(265), UISCALE(10)))->x_map ();
    (new X_hscale (this, &sca_dBsh, UISCALE(520), UISCALE(265), UISCALE(10)))->x_map ();
    add_text (UISCALE( 10), UISCALE(240), UISCALE(50), UISCALE(20), "Delay",    &text0);
    add_text (UISCALE( 10), UISCALE(275), UISCALE(50), UISCALE(20), "Time",     &text0);
    add_text (UISCALE(135), UISCALE(305), UISCALE(60), UISCALE(20), "Reverb",   &text0);
    add_text (UISCALE(355), UISCALE(305), UISCALE(80), UISCALE(20), "Position", &text0);
    add_text (UISCALE(570), UISCALE(305), UISCALE(60), UISCALE(20), "Volume",   &text0);

    sprintf (s, "%s   Aeolus-%s   Audio settings", M->_appid, VERSION);
    x_set_title (s);

    H.position (_xp, _yp);
    H.minsize (UISCALE(200), UISCALE(100));
    H.maxsize (UISCALE(XOFFS + _nasect * XSTEP), UISCALE(YSIZE));
    H.rname (_xresm->rname ());
    H.rclas (_xresm->rclas ());
    x_apply (&H); 
    x_resize (UISCALE(XOFFS + _nasect * XSTEP), UISCALE(YSIZE));
}


void Audiowin::set_aupar (M_ifc_aupar *M)
{
    if (M->_asect < 0)
    {
        if ((M->_parid >= 0) && (M->_parid < NAUPAR))
	{
	    _slid [M->_parid]->set_val (M->_value);
	}
    }
    else if (M->_asect < _nasect)
    {
        if ((M->_parid >= 0) && (M->_parid < 5))
	{
	    _asectd [M->_asect]._slid [M->_parid]->set_val (M->_value);
	}
    }
}


void Audiowin::add_text (int xp, int yp, int xs, int ys, const char *text, X_textln_style *style)
{
    (new X_textln (this, style, xp, yp, xs, ys, text, -1))->x_map ();
}
