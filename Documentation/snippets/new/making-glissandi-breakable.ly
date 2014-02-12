\version "2.17.30"

\header {
  lsrtags = "staff-notation, tweaks-and-overrides"

  texidoc = "
Setting the @code{breakable} property to @code{#t} in combination with
@code{after-line-breaking} allows a glissando to break if it occurs
at a line break:
"

  doctitle = "Making glissandi breakable"
}

glissandoSkipOn = {
  \override NoteColumn.glissando-skip = ##t
  \hide NoteHead
  \override NoteHead.no-ledgers = ##t
}

\relative c'' {
  \override Glissando.breakable = ##t
  \override Glissando.after-line-breaking = ##t
  f1\glissando |
  \break
  a4 r2. |
  f1\glissando
  \once \glissandoSkipOn
  \break
  a2 a4 r4 |
}
