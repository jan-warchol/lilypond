/*
  phrasing-slur-engraver.cc -- implement Phrasing_slur_engraver

  (c) 1997--2004 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/

#include "event.hh"
#include "new-slur.hh"
#include "warn.hh"
#include "note-column.hh"
#include "context.hh"

#include "engraver.hh"
#include "spanner.hh"

/*
  TODO:
  
  ALGRGRRGRG

  Derive this from Slur_engraver. This code is completely duplicate.
*/
class Phrasing_slur_engraver : public Engraver
{
  Link_array<Music> eventses_;
  Link_array<Music> new_phrasing_slur_evs_;
  Link_array<Grob> phrasing_slurs_;
  Link_array<Grob> end_phrasing_slurs_;
  Moment last_start_;

protected:
  virtual bool try_music (Music*);
  virtual void acknowledge_grob (Grob_info);
  virtual void stop_translation_timestep ();
  virtual void start_translation_timestep ();
  virtual void finalize ();
  virtual void process_acknowledged_grobs ();

public:
  TRANSLATOR_DECLARATIONS (Phrasing_slur_engraver);
  
};

Phrasing_slur_engraver::Phrasing_slur_engraver ()
{
  last_start_ = Moment (-1);
}

bool
Phrasing_slur_engraver::try_music (Music *ev)
{
 if (ev->is_mus_type ("phrasing-slur-event"))
    {
      /*
	Let's not start more than one phrasing slur per moment.
      */
      
      Direction d = to_dir (ev->get_property ("span-direction"));
 	  
      if (d == START)
	{
	  if (now_mom () > last_start_)
	    {
	      new_phrasing_slur_evs_.push (ev);
	      last_start_ = now_mom ();
	      return true;
	    }
	}
      else
	{
	  new_phrasing_slur_evs_.push (ev);
	  return true;
	}
    }
  return false;
}

void
Phrasing_slur_engraver::acknowledge_grob (Grob_info info)
{
  if (Note_column::has_interface (info.grob_))
    {
      Grob *e =info.grob_;
      for (int i = 0; i < phrasing_slurs_.size (); i++)
	New_slur::add_column (phrasing_slurs_[i], e);
      for (int i = 0; i < end_phrasing_slurs_.size (); i++)
	New_slur::add_column (end_phrasing_slurs_[i], e);
    }
}

void
Phrasing_slur_engraver::finalize ()
{
  for (int i = 0; i < phrasing_slurs_.size (); i++)
    {
      /*
	Let's not typeset unterminated stuff
      */
      phrasing_slurs_[i]->suicide ();
    }
  phrasing_slurs_.clear ();

  for (int i=0; i < eventses_.size (); i++)
    {
      eventses_[i]->origin ()->warning (_ ("unterminated phrasing slur"));
    }
}

void
Phrasing_slur_engraver::process_acknowledged_grobs ()
{
  Link_array<Grob> start_phrasing_slurs;
  for (int i=0; i< new_phrasing_slur_evs_.size (); i++)
    {
      Music* phrasing_slur_ev = new_phrasing_slur_evs_[i];
      // end phrasing slur: move the phrasing slur to other array

      Direction d = to_dir (phrasing_slur_ev->get_property ("span-direction"));
      
      if (d == STOP)
	{
	  if (phrasing_slurs_.is_empty ())
	    phrasing_slur_ev->origin ()->warning (_f ("can't find start of phrasing slur"));
	  else
	    {
	      Grob* phrasing_slur = phrasing_slurs_.pop ();
	      end_phrasing_slurs_.push (phrasing_slur);
	      eventses_.pop ();
	    }
	}
      else if (d == START)
	{
	  // push a new phrasing_slur onto stack.
	  // (use temp. array to wait for all phrasing_slur STOPs)
	  Grob* phrasing_slur = make_spanner ("PhrasingSlur", phrasing_slur_ev->self_scm ());

	  if (Direction updown = to_dir (phrasing_slur_ev->get_property ("direction")))
	    {
	      phrasing_slur->set_property ("direction", scm_int2num (updown));
	    }

	  start_phrasing_slurs.push (phrasing_slur);
	  eventses_.push (phrasing_slur_ev);
	}
    }
  for (int i=0; i < start_phrasing_slurs.size (); i++)
    phrasing_slurs_.push (start_phrasing_slurs[i]);
  new_phrasing_slur_evs_.clear ();
}

void
Phrasing_slur_engraver::stop_translation_timestep ()
{
  end_phrasing_slurs_.clear ();
}

void
Phrasing_slur_engraver::start_translation_timestep ()
{
  new_phrasing_slur_evs_.clear ();
}



ENTER_DESCRIPTION (Phrasing_slur_engraver,
/* descr */       "Print phrasing slurs. Similar to @ref{Slur_engraver}",
/* creats*/       "PhrasingSlur",
/* accepts */     "phrasing-slur-event",
/* acks  */       "note-column-interface",
/* reads */       "",
/* write */       "");
