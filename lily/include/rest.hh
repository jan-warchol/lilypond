/*
  rest.hh -- declare Rest

  source file of the GNU LilyPond music typesetter

  (c)  1997--2000 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/


#ifndef REST_HH
#define REST_HH

#include "rhythmic-head.hh"

/**
   A pause.
   
   Properties

   style -- string specifying glyph style
 */
class  Rest : public Rhythmic_head
{
public:
  SCM member_after_line_breaking ();
  static SCM after_line_breaking (SCM);
  SCM member_brew_molecule () const;

   static SCM brew_molecule (SCM);
  
Rest (SCM s);
};
#endif // REST_HH
