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


#ifndef __XIFACE_H
#define __XIFACE_H


#include <clxclient.h>
#include <memory>
#include "iface.h"
#include "mainwin.h"
#include "editwin.h"
#include "audiowin.h"
#include "instrwin.h"
#include "midiwin.h"


class Xiface : public Iface,  public X_callback
{
public:

    Xiface (int ac, char *av []);
    virtual ~Xiface ();
    virtual void stop (void);

private:

           
    virtual void thr_main (void);
    virtual void handle_callb (int, X_window*, _XEvent*);

    void handle_mesg (ITC_mesg *);
    void handle_time (void);
    void xcmesg (XClientMessageEvent *);
    void expose (XExposeEvent *);
    void add_text (X_window *win, int xp, int yp, int xs, int ys, const char *text, X_textln_style *style);

    X_resman       _xresm;
    std::unique_ptr <X_display> _disp;
    std::unique_ptr <X_rootwin> _root;
    std::unique_ptr <X_handler> _xhan;
    bool           _stop;
    bool           _ready;
    int            _xs;
    int            _ys;
    std::unique_ptr <Mainwin> _mainwin;
    std::unique_ptr <Editwin> _editwin;
    std::unique_ptr <Midiwin> _midiwin;
    std::unique_ptr <Audiowin> _audiowin;
    std::unique_ptr <Instrwin> _instrwin;

    M_ifc_aupar   *_aupar;
    M_ifc_dipar   *_dipar;
    M_ifc_edit    *_editp;
};


#endif
