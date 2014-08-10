\version "2.19.12"

\header {
  texidoc = "Long lyric syllables should not be centered under noteheads.
This is particularly important in case of recitative (multiple words written
under one note)."
}

\relative f' { r4 r8 g8 a[( g]) a[( g]) g2 r2 }
\addlyrics { the crook -- ed straight, }

\relative f'' { \time 4/2 \tweak style #'altdefault d\breve }
\addlyrics { "Tunc acceptabis sacrificium iustitiae," }

{ e'1 1 1 }
\addlyrics { laa laaaal laaaaa }
