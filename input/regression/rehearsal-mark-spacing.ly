\version "2.19.3"

\header {
  texidoc = "There should be no extra space inserted between notes when
a RehearsalMark is added inside a measure.  By default, the center of the
RehearsalMark should be aligned with the left edge of the note it is
attached to, regardless of spacing."
}

\relative f'' { e4 e e \mark I e }

\score{
  \relative f'' { e4 e e \mark I e }
  \layout{
    line-width = 100\mm
    ragged-right=##f
  }
}
