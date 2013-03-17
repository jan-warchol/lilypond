\version "2.17.15"
#(set-global-staff-size 30)

\header {
  texidoc = "Unassociated lyrics (i.e. lyrics without an associatedVoice)
  should be properly aligned and behave just like associated lyrics."
}

\markup "default-aligned (centered):"
<<
  { d'2 d' e'1 <f' g'> }
  \new Lyrics { \lyricmode  { foo2 bar aaa1 mmm1 } }
>>

\markup "right-aligned:"
<<
  { d'2 d' e'1 <f' g'> }
  \new Lyrics {
    \lyricmode  {
      \override LyricText #'self-alignment-X = #RIGHT
      foo2 bar aaa1 mmm1
    }
  }
>>
