/*
  chord-name.hh -- declare Chord_name

  source file of the GNU LilyPond music typesetter

  (c) 1999--2000 Jan Nieuwenhuizen <janneke@gnu.org>
*/

#ifndef CHORD_NAME_HH
#define CHORD_NAME_HH

#include "chord.hh"
#include "item.hh"
#include "molecule.hh"

/**
   elt_properties:
   pitches: list of musical-pitch
   inversion(optional): musical-pitch
   bass(optional): musical-pitch
 */
class Chord_name : public Item
{
public:
  static SCM brew_molecule (SCM);
  
  VIRTUAL_COPY_CONS (Score_element);
  Molecule ly_word2molecule (SCM scm, Real* x) const;
  Molecule ly_text2molecule (SCM scm) const;
  Chord_name(SCM s);
public:
  SCM member_brew_molecule () const;

};

#endif // CHORD_NAME_HH
