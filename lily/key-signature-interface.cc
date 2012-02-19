/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 1996--2012 Han-Wen Nienhuys <hanwen@xs4all.nl>

  keyplacement by Mats Bengtsson

  LilyPond is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LilyPond is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "accidental-interface.hh"
#include "font-interface.hh"
#include "international.hh"
#include "item.hh"
#include "lookup.hh"
#include "output-def.hh"
#include "skyline-pair.hh"
#include "staff-symbol-referencer.hh"
#include "rational.hh"

struct Key_signature_interface
{
  DECLARE_SCHEME_CALLBACK (print, (SCM));
  DECLARE_SCHEME_CALLBACK (vertical_skylines, (SCM));
  DECLARE_GROB_INTERFACE ();

  static SCM internal_print (SCM, bool);
};

/*
  TODO
  - space the `natural' signs wider
*/
MAKE_SCHEME_CALLBACK (Key_signature_interface, print, 1);
SCM
Key_signature_interface::print (SCM smob)
{
  return internal_print (smob, true);
}

MAKE_SCHEME_CALLBACK (Key_signature_interface, vertical_skylines, 1);
SCM
Key_signature_interface::vertical_skylines (SCM smob)
{
  return internal_print (smob, false);
}

SCM
Key_signature_interface::internal_print (SCM smob, bool return_stencil)
{
  Item *me = dynamic_cast<Item *> (unsmob_grob (smob));

  Real inter = Staff_symbol_referencer::staff_space (me) / 2.0;

  Stencil mol;
  Skyline_pair vertical_skylines;

  SCM c0s = me->get_property ("c0-position");

  bool is_cancellation = me->internal_has_interface
                         (ly_symbol2scm ("key-cancellation-interface"));

  /*
    SCM lists are stacks, so we work from right to left, ending with
    the cancellation signature.
  */

  int last_pos = -1000;
  SCM last_glyph_name = SCM_BOOL_F;
  SCM padding_pairs = me->get_property ("padding-pairs");
  SCM make_vs_cache_name = ly_lily_module_constant ("make-vertical-skylines-cache-name");
  SCM vertical_skylines_cache = ly_lily_module_constant ("vertical-skylines-cache");

  Font_metric *fm = Font_interface::get_default_font (me);
  SCM alist = me->get_property ("glyph-name-alist");

  for (SCM s = me->get_property ("alteration-alist"); scm_is_pair (s); s = scm_cdr (s))
    {
      SCM alt = is_cancellation
                ? scm_from_int (0)
                : scm_cdar (s);

      SCM glyph_name_scm = ly_assoc_get (alt, alist, SCM_BOOL_F);
      if (!scm_is_string (glyph_name_scm))
        {
          me->warning (_f ("No glyph found for alteration: %s",
                           ly_scm2rational (alt).to_string ().c_str ()));
          continue;
        }

      string glyph_name = ly_scm2string (glyph_name_scm);

      Stencil acc (fm->find_by_name (glyph_name));

      if (acc.is_empty ())
        me->warning (_ ("alteration not found"));
      else
        {
          SCM what = scm_caar (s);

          SCM proc = ly_lily_module_constant ("key-signature-interface::alteration-position");

          int pos = scm_to_int (scm_call_3 (proc, what, scm_cdar (s), c0s));
          Real padding = robust_scm2double (me->get_property ("padding"),
                                            0.0);

          /*
            The natural sign (unlike flat & sharp)
            has vertical edges on both sides. A little padding is
            needed to prevent collisions.
          */
          SCM handle = scm_assoc (scm_cons (glyph_name_scm, last_glyph_name),
                                  padding_pairs);

          if (scm_is_pair (handle))
            padding = robust_scm2double (scm_cdr (handle), 0.0);
          else if (glyph_name == "accidentals.natural"
                   && last_pos < pos + 2
                   && last_pos > pos - 6)
            padding += 0.3;

          if (!return_stencil)
            {
              SCM key = scm_call_3 (make_vs_cache_name,
                                    acc.smobbed_copy (),
                                    fm->self_scm (),
                                    ly_string2scm (glyph_name));
              Skyline_pair ret;
              if (Skyline_pair *vsk =
                    Skyline_pair::unsmob
                      (ly_assoc_get (key,
                                     vertical_skylines_cache,
                                     SCM_BOOL_F)))
                {
                  // code dup from stencil-integral - we've already calculated this.  just need to shift it...
                  ret = Skyline_pair (*vsk);
                }
              else
                {
                  Skyline_pair *skyp = Skyline_pair::unsmob (Stencil::vertical_skylines_from_stencil (acc.smobbed_copy ()));
                  SCM write_to_skyline_cache = ly_lily_module_constant ("write-to-vertical-skylines-cache");
                  (void) scm_call_2 (write_to_skyline_cache, skyp->smobbed_copy (), key);
                  ret = Skyline_pair (*skyp);
                }
              vertical_skylines.shift (acc.extent (X_AXIS).length () + padding);
              ret.raise (pos * inter);
              vertical_skylines.merge (ret);
            }
          acc.translate_axis (pos * inter, Y_AXIS);

          mol.add_at_edge (X_AXIS, LEFT, acc, padding);

          last_pos = pos;
          last_glyph_name = glyph_name_scm;
        }
    }


  if (return_stencil)
    {
      mol.align_to (X_AXIS, LEFT);
      return mol.smobbed_copy ();
    }
  else
    {
      if (!vertical_skylines.is_empty ())
        {
          vertical_skylines.shift
            (-vertical_skylines.left ());
        }
      return vertical_skylines.smobbed_copy ();
    }
}

ADD_INTERFACE (Key_signature_interface,
               "A group of accidentals, to be printed as signature sign.",

               /* properties */
               "alteration-alist "
               "c0-position "
               "glyph-name-alist "
               "padding "
               "padding-pairs "
              );
