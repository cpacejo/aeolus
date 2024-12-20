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
#include <stdlib.h>
#include <stdio.h>

#include "midimatrix.h"
#include "callbacks.h"
#include "messages.h"
#include "styles.h"



Midimatrix::Midimatrix (X_window *parent, X_callback *callb, int xp, int yp) :
    X_window (parent, xp, yp, UISCALE(100), UISCALE(100), Colors.midi_bg),
    _callb (callb),
    _mapped (false)
{
    x_add_events (ExposureMask | ButtonPressMask | StructureNotifyMask);
    x_set_bit_gravity (NorthWestGravity);
}


Midimatrix::~Midimatrix (void)
{
}


void Midimatrix::init (M_ifc_init *M)
{
    int i;

    _nkeybd = M->_nkeybd;
    _ndivis = 0;
    for (i = 0; i < M->_nkeybd; i++)
    {
        if (i == 16) break;
	_labels [i] = M->_keybdd [i]._label;
    }
    for (i = 0; i < M->_ndivis; i++)
    {
        if (i == 16) break;
        if (M->_divisd [i]._flags)
	{
            _ndivis++;
            _labels [_nkeybd + i] = M->_divisd [i]._label;
	}
    }
    for (i = 0; i < 16; i++) _chconf [i] = 0;
    _xs = UISCALE(XL + 16 * DX + XR);
    _ys = UISCALE(YT + (_nkeybd + _ndivis + 1) * DY + YB);
    x_resize (_xs, _ys);
    x_map ();
}


void Midimatrix::set_chconf (uint16_t *d)
{
    plot_allconn ();
    std::copy_n (d, 16, _chconf);
    plot_allconn ();
}


void Midimatrix::handle_event (XEvent *E)
{
    switch (E->type)
    {
    case MapNotify:
        _mapped = true;
        break;
 
    case UnmapNotify:
        _mapped = false;
        break;
 
    case Expose:
        expose ((XExposeEvent *) E);
        break;  

    case ButtonPress:
        bpress ((XButtonEvent *) E);
        break;
    }
}


void Midimatrix::expose (XExposeEvent *E)
{
    if (E->count) return;
    redraw ();
}


void Midimatrix::redraw (void)
{
    int     i, x, y, d;
    char    s [4];
    X_draw  D (dpy (), win (), dgc (), xft ());

    if (! _mapped) return;
    D.clearwin ();
    D.setfunc (GXcopy);
    D.setcolor (Colors.midi_gr1);
    for (i = 0, x = UISCALE(XL + DX); i < 16; i++, x += UISCALE(DX))
    {
	D.move (x, UISCALE(YT));
        D.draw (x, _ys - UISCALE(YT));
    }
    for (i = 0, y = UISCALE(YT); i <= _nkeybd + _ndivis + 1; i++, y += UISCALE(DY))
    {
	D.move (UISCALE(XR), y);
        D.rdraw (_xs - UISCALE(2 * XR), 0);
    }
    D.setcolor (XftColors.midi_fg);
    D.setfont (XftFonts.midimt);
    d = (UISCALE(DY) + D.textascent () - D.textdescent ()) / 2;
    for (i = 0, y = UISCALE(YT); i < _nkeybd + _ndivis; i++, y += UISCALE(DY))
    {
        D.move (UISCALE(XL - 40), y + d);
        D.drawstring (_labels [i], 0);
    }
    x = UISCALE(XL + DX / 2);
    y += UISCALE(DY);
    for (i = 1; i <= 16; i++)
    {
	sprintf (s, "%d", i);
        D.move (x, y + d);
        D.drawstring (s, 0);
        x += UISCALE(DX);
    } 
    D.setcolor (Colors.midi_gr2);
    D.move (UISCALE(XL), UISCALE(YT));
    D.rdraw (0, _ys - UISCALE(2 * YT));
    y = UISCALE(YT);
    D.move (UISCALE(XR), y);
    D.rdraw (_xs - UISCALE(2 * XR), 0);
    D.setcolor (XftColors.midi_fg);
    D.move (UISCALE(10), y + d);
    D.drawstring ("Keyboards", -1);
    y += _nkeybd * UISCALE(DY);
    D.setcolor (Colors.midi_gr2);
    D.move (UISCALE(XR), y);
    D.rdraw (_xs - UISCALE(2 * XR), 0);
    D.setcolor (XftColors.midi_fg);
    D.move (UISCALE(10), y + d);
    D.drawstring ("Divisions", -1);
    y += _ndivis * UISCALE(DY);
    D.setcolor (Colors.midi_gr2);
    D.move (UISCALE(XR), y);
    D.rdraw (_xs - UISCALE(2 * XR), 0);
    D.setcolor (XftColors.midi_fg);
    D.move (UISCALE(10), y + d);
    D.drawstring ("Control", -1);
    y += UISCALE(DY);
    D.setcolor (Colors.midi_gr2);
    D.move (UISCALE(XR), y);
    D.rdraw (_xs - UISCALE(2 * XR), 0);
    D.setcolor (Colors.midi_gr2);
    D.move (_xs - 1, 0);
    D.rdraw (0, _ys - 1);
    D.rdraw (1 - _xs, 0);
    plot_allconn ();
}


void Midimatrix::plot_allconn (void)
{
    int i, m;
       
    for (i = 0; i < 16; i++)
    {
	m = _chconf [i];
        if (m & 0x1000) plot_conn (i, m & 15);
        if (m & 0x2000) plot_conn (i, _nkeybd + ((m >> 4) & 15));
        if (m & 0x4000) plot_conn (i, _nkeybd + _ndivis);
    }       
}


void Midimatrix::plot_conn (int x, int y)
{
    X_draw D (dpy (), win (), dgc (), 0);

    if (y < _nkeybd)                 D.setcolor (Colors.midi_bg ^ Colors.midi_co1);
    else if (y < _nkeybd + _ndivis)  D.setcolor (Colors.midi_bg ^ Colors.midi_co2);
    else                             D.setcolor (Colors.midi_bg ^ Colors.midi_co3);
    D.setfunc (GXxor);
    x = UISCALE(XL + x * DX + 5);
    y = UISCALE(YT + y * DY + 5);
    D.fillrect (x, y, x + UISCALE(DX - 10) + 1, y + UISCALE(DY - 10) + 1);
}    


void Midimatrix::bpress (XButtonEvent *E)
{
    int i, j, k, m, x, y;

    x = E->x - UISCALE(XL);
    y = E->y - UISCALE(YT);
    if ((x < 0) || (y < 0)) return;
    i = x / UISCALE(DX);
    j = y / UISCALE(DY);
    m = _nkeybd + _ndivis;
    if ((i < 0) || (i > 16)) return;
    if ((j < 0) || (j >  m)) return;

    if (j < _nkeybd)
    {
        k = (_chconf [i] & 0x1000) ? (_chconf [i] & 15) : -1;
        _chconf [i] &= 0x6FF0; 
        if (k != j)
	{
            _chconf [i] |= 0x1000 | j;
            if (k >= 0) plot_conn (i, k);
	}
        plot_conn (i, j);
    }
    else if (j < _nkeybd + _ndivis)
    {
        j -= _nkeybd;
        k = (_chconf [i] & 0x2000) ? ((_chconf [i] >> 4) & 15) : -1;
        _chconf [i] &= 0x5F0F; 
        if (k != j)
	{
            _chconf [i] |= 0x2000 | (j << 4);
            if (k >= 0) plot_conn (i, k + _nkeybd);
	}
        plot_conn (i, j + _nkeybd);
    }
    else
    {
        _chconf [i] ^= 0x4000;
        plot_conn (i, _nkeybd + _ndivis);
    }

    if (_callb) _callb->handle_callb (CB_MIDI_MODCONF, this, 0);
}


