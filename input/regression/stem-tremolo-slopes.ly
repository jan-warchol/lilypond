\version "2.15.14"

\header {
  texidoc = "First is default.  Three notes at the beginning should have
  beam-like slashes; slash of the third note should be more sloped.
  Slashes on beamed notes should be rectangular and parallel to the beams.
  In the second example all slashes should have the same slope except for
  downstem flagged notes.  Different style is also used."
}

music = {
  a''4:32 a':
  e''8: \noBeam e':
  a'': [ a': ]
  f': [ g':]
  d': [ d': ]
}

\new Staff {
  \music
}

\new Staff {
  \override StemTremolo #'slope = #ly:stem-tremolo::calc-constant-slope
  \override StemTremolo #'style = #'beam-like
  \music
}
