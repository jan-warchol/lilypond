/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 1999--2014 Glen Prideaux <glenprideaux@iname.com>,
  Han-Wen Nienhuys <hanwen@xs4all.nl>,
  Jan Nieuwenhuizen <janneke@gnu.org>

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

#include "context.hh"
#include "engraver.hh"
#include "international.hh"
#include "item.hh"
#include "lyric-extender.hh"
#include "note-head.hh"
#include "pointer-group-interface.hh"
#include "stream-event.hh"
#include "warn.hh"
#include "spanner.hh"
#include "translator.icc"

void completize_extender (Spanner *sp);

class Extender_engraver : public Engraver
{
  Stream_event *ev_;
  Spanner *extender_;
  Spanner *pending_extender_;

public:
  TRANSLATOR_DECLARATIONS (Extender_engraver);

protected:
  DECLARE_TRANSLATOR_LISTENER (extender);
  DECLARE_TRANSLATOR_LISTENER (completize_extender);
  DECLARE_ACKNOWLEDGER (lyric_syllable);

  virtual void finalize ();

  void stop_translation_timestep ();
  void process_music ();
};

Extender_engraver::Extender_engraver ()
{
  extender_ = 0;
  pending_extender_ = 0;
  ev_ = 0;
}

IMPLEMENT_TRANSLATOR_LISTENER (Extender_engraver, extender);
void
Extender_engraver::listen_extender (Stream_event *ev)
{
  ASSIGN_EVENT_ONCE (ev_, ev);
}

/*
  A CompletizeExtenderEvent is sent at the end of each lyrics block
  to ensure any pending extender can be correctly terminated if the lyrics
  end before the associated voice (this prevents the right bound being extended
  to the next note-column if no lyric follows the extender)
*/
IMPLEMENT_TRANSLATOR_LISTENER (Extender_engraver, completize_extender);
void
Extender_engraver::listen_completize_extender (Stream_event * /* ev */)
{
  if (pending_extender_)
    {
      completize_extender (pending_extender_);
      pending_extender_ = 0;
    }
}

void
Extender_engraver::process_music ()
{
  if (ev_)
    extender_ = make_spanner ("LyricExtender", ev_->self_scm ());
}

void
Extender_engraver::acknowledge_lyric_syllable (Grob_info i)
{
  Item *item = i.item ();
  if (extender_)
    extender_->set_bound (LEFT, item);

  if (pending_extender_)
    {
      pending_extender_->set_object ("next", item->self_scm ());
      completize_extender (pending_extender_);
      pending_extender_ = 0;
    }
}

void
Extender_engraver::stop_translation_timestep ()
{
  if (extender_ || pending_extender_)
    {
      Context *voice = get_voice_to_lyrics (context ());
      Grob *h = voice ? get_current_note_head (voice, to_boolean (get_property ("includeGraceNotes"))) : 0;

      if (h)
        {
          if (extender_)
            {
              Pointer_group_interface::add_grob (extender_,
                                                 ly_symbol2scm ("heads"), h);
            }

          if (pending_extender_)
            {
              Pointer_group_interface::add_grob (pending_extender_,
                                                 ly_symbol2scm ("heads"), h);
            }
        }
      else
        {
          if (pending_extender_
              && !get_property ("extendersOverRests"))
            {
              completize_extender (pending_extender_);
              pending_extender_ = 0;
            }
        }
      if (extender_)
        {
          pending_extender_ = extender_;
          extender_ = 0;
        }
    }

  ev_ = 0;
}

void
completize_extender (Spanner *sp)
{
  if (!sp->get_bound (RIGHT))
    {
      extract_item_set (sp, "heads", heads);
      if (heads.size ())
        sp->set_bound (RIGHT, heads.back ());
    }
}

void
Extender_engraver::finalize ()
{
  if (extender_)
    {
      completize_extender (extender_);

      if (!extender_->get_bound (RIGHT))
        extender_->warning (_ ("unterminated extender"));
      extender_ = 0;
    }

  if (pending_extender_)
    {
      completize_extender (pending_extender_);

      if (!pending_extender_->get_bound (RIGHT))
        pending_extender_->warning (_ ("unterminated extender"));
      pending_extender_ = 0;
    }
}

ADD_ACKNOWLEDGER (Extender_engraver, lyric_syllable);
ADD_TRANSLATOR (Extender_engraver,
                /* doc */
                "Create lyric extenders.",

                /* create */
                "LyricExtender ",

                /* read */
                "extendersOverRests "
                "includeGraceNotes ",

                /* write */
                ""
               );
