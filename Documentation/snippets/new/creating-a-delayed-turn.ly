\version "2.17.97"

\header {
  lsrtags = "editorial-annotations, expressive-marks, tweaks-and-overrides"

  texidoc = "
Creating a delayed turn, where the lower note of the turn uses the
accidental, requires several overrides.  The
@code{outside-staff-priority} property must be set to @code{#f}, as
otherwise this would take precedence over the @code{avoid-slur
property}.  Changing the fractions @code{2/3} and @code{1/3} adjusts the
horizontal position.
"
  doctitle = "Creating a delayed turn"
}


\relative c'' {
  c2*2/3 ( s2*1/3\turn d4) r
  <<
    { c4.( d8) }
    { s4 s\turn }
  >>
  \transpose c d \relative c'' <<
    { c4.( d8) }
    {
      s4
      \once \set suggestAccidentals = ##t
      \once \override AccidentalSuggestion #'outside-staff-priority = ##f
      \once \override AccidentalSuggestion #'avoid-slur = #'inside
      \once \override AccidentalSuggestion #'font-size = #-3
      \once \override AccidentalSuggestion #'script-priority = #-1
      \single \hideNotes
      b8-\turn \noBeam
      s8
    }
  >>
}
