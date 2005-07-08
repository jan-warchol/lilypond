/*
  font-interface.cc -- implement Font_interface

  source file of the GNU LilyPond music typesetter

  (c) 2000--2005 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/

#include "font-interface.hh"

#include "all-font-metrics.hh"
#include "output-def.hh"
#include "warn.hh"
#include "grob.hh"

/* todo: split up this func, reuse in text_item?  */
Font_metric *
Font_interface::get_default_font (Grob *me)
{
  Font_metric *fm = unsmob_metrics (me->get_property ("font"));
  if (!fm)
    {
      SCM chain = music_font_alist_chain (me);

      fm = select_font (me->get_layout (), chain);
      me->set_property ("font", fm->self_scm ());
    }

  return fm;
}

SCM
Font_interface::music_font_alist_chain (Grob *g)
{
  SCM defaults
    = g->get_layout ()->lookup_variable (ly_symbol2scm ("font-defaults"));
  return g->get_property_alist_chain (defaults);
}

SCM
Font_interface::text_font_alist_chain (Grob *g)
{
  SCM defaults
    = g->get_layout ()->lookup_variable (ly_symbol2scm ("text-font-defaults"));
  return g->get_property_alist_chain (defaults);
}

ADD_INTERFACE (Font_interface, "font-interface",
	       "Any symbol that is typeset through fixed sets of glyphs, "
	       " (ie. fonts)",
	       "font-magnification font font-series font-shape "
	       "font-family font-encoding font-name font-size");
