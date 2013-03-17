
\version "2.17.13"
#(set-global-staff-size 30)

\header {
  texidoc = "Lyrics unattached to notes should be centered
  on appropriate PaperColumns; in this case they should
  align horizontally with dynamics."
}

\paper {
  ragged-right = ##f
}

<<
  \new Staff << f'1 { s4 s4^\p s^\fff s4^\fp } >>
  \new Lyrics \lyricmode {
    \skip4 p4 fff4 fp4
  }
>>
