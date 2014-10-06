/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 1997--2014 Han-Wen Nienhuys <hanwen@xs4all.nl>

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

#include "translator.hh"

#include "context-def.hh"
#include "dispatcher.hh"
#include "global-context.hh"
#include "international.hh"
#include "translator-group.hh"
#include "warn.hh"

#include "translator.icc"

Translator::~Translator ()
{
}

void
Translator::init ()
{
  daddy_context_ = 0;
  smobify_self ();
}

void
Translator::process_music ()
{
}

void
Translator::process_acknowledged ()
{
}

Translator::Translator ()
{
  init ();
}

Translator::Translator (Translator const &src)
{
  (void) src;
  init ();
}

Moment
Translator::now_mom () const
{
  return daddy_context_->now_mom ();
}

Output_def *
Translator::get_output_def () const
{
  return daddy_context_->get_output_def ();
}

Translator_group *
Translator::get_daddy_translator () const
{
  return daddy_context_->implementation ();
}

void
Translator::protect_event (SCM ev)
{
  get_daddy_translator ()->protect_event (ev);
}

SCM
Translator::get_property (string sym) const
{
  return daddy_context_->internal_get_property (ly_symbol2scm (sym));
}

SCM
Translator::internal_get_property (SCM sym) const
{
  return daddy_context_->internal_get_property (sym);
}

void
Translator::stop_translation_timestep ()
{
}

/*
  this function is called once each moment, before any user
  information enters the translators.  (i.e. no \property or event has
  been processed yet.)
*/
void
Translator::start_translation_timestep ()
{
}

void
Translator::initialize ()
{
}

void
Translator::finalize ()
{
}

void
Translator::connect_to_context (Context *c)
{
  for (translator_listener_record *r = get_listener_list (); r; r = r->next_)
    c->events_below ()->add_listener (r->get_listener_ (this, r->event_class_),
                                      r->event_class_);
}

void
Translator::disconnect_from_context (Context *c)
{
  for (translator_listener_record *r = get_listener_list (); r; r = r->next_)
    c->events_below ()->remove_listener (r->get_listener_ (this, r->event_class_),
                                         r->event_class_);
}

/*
  internally called once, statically, for each translator
  listener. Connects the name of an event class with a procedure that
  fetches the corresponding listener.

  The method should only be called from the macro
  IMPLEMENT_TRANSLATOR_LISTENER.
 */
void
Translator::add_translator_listener (translator_listener_record **listener_list,
                                     translator_listener_record *r,
                                     Listener (*get_listener) (void *, SCM),
                                     const char *ev_class)
{
  /* ev_class is the C++ identifier name. Convert to scm symbol */
  string name = string (ev_class);
  name = replace_all (&name, '_', '-');
  name += "-event";

  // we make the symbol permanent in order not to have to bother about
  // the static translator_listener_record chains while garbage
  // collecting.

  SCM class_sym = scm_permanent_object (scm_from_locale_symbol (name.c_str ()));

  r->event_class_ = class_sym;
  r->get_listener_ = get_listener;
  r->next_ = *listener_list;
  *listener_list = r;
}

/*
 Helps the individual static_translator_description methods of translators.
*/
SCM
Translator::static_translator_description (const char *grobs,
                                           const char *desc,
                                           translator_listener_record *listener_list,
                                           const char *read,
                                           const char *write) const
{
  SCM static_properties = SCM_EOL;

  static_properties = scm_acons (ly_symbol2scm ("grobs-created"),
                                 parse_symbol_list (grobs), static_properties);

  static_properties = scm_acons (ly_symbol2scm ("description"),
                                 scm_from_locale_string (desc), static_properties);

  SCM list = SCM_EOL;
  for (; listener_list; listener_list = listener_list->next_)
    list = scm_cons (listener_list->event_class_, list);
  static_properties = scm_acons (ly_symbol2scm ("events-accepted"),
                                 list, static_properties);

  static_properties = scm_acons (ly_symbol2scm ("properties-read"),
                                 parse_symbol_list (read), static_properties);

  static_properties = scm_acons (ly_symbol2scm ("properties-written"),
                                 parse_symbol_list (write), static_properties);

  return static_properties;
}

/*
  SMOBS
*/
SCM
Translator::mark_smob ()
{
  derived_mark ();
  return SCM_EOL;
}

Global_context *
Translator::get_global_context () const
{
  return daddy_context_->get_global_context ();
}

Context *
Translator::get_score_context () const
{
  return daddy_context_->get_score_context ();
}

const char Translator::type_p_name_[] = "ly:translator?";

bool
Translator::must_be_last () const
{
  return false;
}

void
Translator::derived_mark () const
{
}

int
Translator::print_smob (SCM s, SCM port, scm_print_state *)
{
  Translator *me = (Translator *) SCM_CELL_WORD_1 (s);
  scm_puts ("#<Translator ", port);
  scm_puts (me->class_name (), port);
  scm_puts (" >", port);
  return 1;
}

void
add_acknowledger (Engraver_void_function_engraver_grob_info ptr,
                  char const *func_name,
                  vector<Acknowledge_information> *ack_array)
{
  Acknowledge_information inf;
  inf.function_ = ptr;

  string interface_name (func_name);

  interface_name = replace_all (&interface_name, '_', '-');
  interface_name += "-interface";

  /*
    this is only called during program init, so safe to use scm_gc_protect_object ()
  */
  inf.symbol_ = scm_gc_protect_object (ly_symbol2scm (interface_name.c_str ()));
  ack_array->push_back (inf);
}

Engraver_void_function_engraver_grob_info
generic_get_acknowledger (SCM sym, vector<Acknowledge_information> const *ack_array)
{
  for (vsize i = 0; i < ack_array->size (); i++)
    {
      if (ack_array->at (i).symbol_ == sym)
        return ack_array->at (i).function_;
    }
  return 0;
}

Moment
get_event_length (Stream_event *e)
{
  Moment *m = Moment::unsmob (e->get_property ("length"));
  if (m)
    return *m;
  else
    return Moment (0);
}

Moment
get_event_length (Stream_event *e, Moment now)
{
  Moment len = get_event_length (e);

  if (now.grace_part_)
    {
      len.grace_part_ = len.main_part_;
      len.main_part_ = Rational (0);
    }
  return len;
}

/*
  Helper, used through ASSIGN_EVENT_ONCE to throw warnings for
  simultaneous events. The helper is only useful in listen_* methods
  of translators.
*/
bool
internal_event_assignment (Stream_event **old_ev, Stream_event *new_ev, const char *function)
{
  if (*old_ev
      && !to_boolean (scm_equal_p ((*old_ev)->self_scm (),
                                   new_ev->self_scm ())))
    {
      /* extract event class from function name */
      string ev_class = function;

      /* This assertion fails if EVENT_ASSIGNMENT was called outside a
         translator listener. Don't do that. */
      const char *prefix = "listen_";
      assert (0 == ev_class.find (prefix));

      /* "listen_foo_bar" -> "foo-bar" */
      ev_class.erase (0, strlen (prefix));
      replace_all (&ev_class, '_', '-');

      new_ev->origin ()->warning (_f ("Two simultaneous %s events, junking this one", ev_class.c_str ()));
      (*old_ev)->origin ()->warning (_f ("Previous %s event here", ev_class.c_str ()));
      return false;
    }
  else
    {
      *old_ev = new_ev;
      return true;
    }
}

ADD_TRANSLATOR (Translator,
                /* doc */
                "Base class.  Not instantiated.",

                /* create */
                "",

                /* read */
                "",

                /* write */
                ""
               );
