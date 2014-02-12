\version "2.17.30"

\header {
  lsrtags = "staff-notation, ancient-notation, really-cool"
  texidoc = "
Incipits can be added using the instrument name grob, but keeping
separate the instrument name definition and the incipit definition.
"
  doctitle = "Incipit"
}

incipit =
#(define-music-function (parser location incipit-music) (ly:music?)
  #{
    \once \override Staff.InstrumentName.self-alignment-X = #RIGHT
    \once \override Staff.InstrumentName.self-alignment-Y = ##f
    \once \override Staff.InstrumentName.padding = #0.3
    \once \override Staff.InstrumentName.stencil =
      #(lambda (grob)
	 (let* ((instrument-name (ly:grob-property grob 'long-text)))
	   (set! (ly:grob-property grob 'long-text)
		 #{ \markup
		      \score
		         {
			   { \context MensuralStaff \with {
	                        instrumentName = #instrument-name
	                     } $incipit-music
			   }
	                   \layout { $(ly:grob-layout grob)
			             line-width = \indent
		                     indent =
				% primitive-eval is probably easiest for
				% escaping lexical closure and evaluating
				% everything respective to (current-module).
	                             #(primitive-eval
                                       '(or (false-if-exception (- indent incipit-width))
					    (* 0.5 indent)))
			             ragged-right = ##f
			             ragged-last = ##f
			             system-count = #1 }
			 }
		  #})
           (system-start-text::print grob)))
  #})

%%%%%%%%%%%%%%%%%%%%%%%%%

global = {
  \set Score.skipBars = ##t
  \key g \major
  \time 4/4

  % the actual music
  \skip 1*8

  % let finis bar go through all staves
  \override Staff.BarLine.transparent = ##f

  % finis bar
  \bar "|."
}

discantusIncipit = <<
  \new MensuralVoice = "discantusIncipit" <<
    \repeat unfold 9 { s1 \noBreak }
    {
      \clef "neomensural-c1"
      \key f \major
      \time 2/2
      c''1.
    }
  >>
  \new Lyrics \lyricsto discantusIncipit { IV- }
>>

discantusNotes = {
  \transpose c' c'' {
    \clef "treble"
    d'2. d'4 |
    b e' d'2 |
    c'4 e'4.( d'8 c' b |
    a4) b a2 |
    b4.( c'8 d'4) c'4 |
    \once \hide NoteHead
    c'1 |
    b\breve |
  }
}

discantusLyrics = \lyricmode {
  Ju -- bi -- |
  la -- te De -- |
  o, om --
  nis ter -- |
  ra, __ om- |
  "..." |
  -us. |
}

altusIncipit = <<
  \new MensuralVoice = "altusIncipit" <<
    \repeat unfold 9 { s1 \noBreak }
    {
      \clef "neomensural-c3"
      \key f \major
      \time 2/2
      r1 f'1.
    }
  >>
  \new Lyrics \lyricsto altusIncipit { IV- }
>>

altusNotes = {
  \transpose c' c'' {
    \clef "treble"
    % two measures
    r2 g2. e4 fis g |
    a2 g4 e |
    fis g4.( fis16 e fis4) |
    g1 |
    \once \hide NoteHead
    g1 |
    g\breve |
  }
}

altusLyrics = \lyricmode {
  % two measures
  Ju -- bi -- la -- te |
  De -- o, om -- |
  nis ter -- ra, |
  "..." |
  -us. |
}

tenorIncipit = <<
  \new MensuralVoice = "tenorIncipit" <<
    \repeat unfold 9 { s1 \noBreak }
    {
      \clef "neomensural-c4"
      \key f \major
      \time 2/2
      r\longa
      r\breve
      r1 c'1.
    }
  >>
  \new Lyrics \lyricsto tenorIncipit { IV- }
>>

tenorNotes = {
  \transpose c' c' {
    \clef "treble_8"
    R1 |
    R1 |
    R1 |
    % two measures
    r2 d'2. d'4 b e' |
    \once \hide NoteHead
    e'1 |
    d'\breve |
  }
}

tenorLyrics = \lyricmode {
  % two measures
  Ju -- bi -- la -- te |
  "..." |
  -us.
}

bassusIncipit = <<
  \new MensuralVoice = "bassusIncipit" <<
    \repeat unfold 9 { s1 \noBreak }
    {
      \clef "bass"
      \key f \major
      \time 2/2
      %% incipit
      r\maxima
      f1.
    }
  >>
  \new Lyrics \lyricsto bassusIncipit { IV- }
>>

bassusNotes = {
  \transpose c' c' {
    \clef "bass"
    R1 |
    R1 |
    R1 |
    R1 |
    g2. e4 |
    \once \hide NoteHead
    e1 |
    g\breve |
  }
}

bassusLyrics = \lyricmode {
  Ju -- bi- |
  "..." |
  -us.
}

\score {
  <<
    \new StaffGroup = choirStaff <<
      \new Voice = "discantusNotes" <<
        \global
        \set Staff.instrumentName = #"Discantus"
        \incipit \discantusIncipit
        \discantusNotes
      >>
      \new Lyrics = "discantusLyrics" \lyricsto discantusNotes { \discantusLyrics }
      \new Voice = "altusNotes" <<
        \global
        \set Staff.instrumentName = #"Altus"
        \incipit \altusIncipit
        \altusNotes
      >>
      \new Lyrics = "altusLyrics" \lyricsto altusNotes { \altusLyrics }
      \new Voice = "tenorNotes" <<
        \global
        \set Staff.instrumentName = #"Tenor"
        \incipit \tenorIncipit
        \tenorNotes
      >>
      \new Lyrics = "tenorLyrics" \lyricsto tenorNotes { \tenorLyrics }
      \new Voice = "bassusNotes" <<
        \global
        \set Staff.instrumentName = #"Bassus"
        \incipit \bassusIncipit
        \bassusNotes
      >>
      \new Lyrics = "bassusLyrics" \lyricsto bassusNotes { \bassusLyrics }
    >>
  >>
  \layout {
    \context {
      \Score
      %% no bar lines in staves or lyrics
      \hide BarLine
    }
    %% the next two instructions keep the lyrics between the bar lines
    \context {
      \Lyrics
      \consists "Bar_engraver"
      \consists "Separating_line_group_engraver"
    }
    \context {
      \Voice
      %% no slurs
      \hide Slur
      %% Comment in the below "\remove" command to allow line
      %% breaking also at those bar lines where a note overlaps
      %% into the next measure.  The command is commented out in this
      %% short example score, but especially for large scores, you
      %% will typically yield better line breaking and thus improve
      %% overall spacing if you comment in the following command.
      %%\remove "Forbid_line_break_engraver"
    }
    indent = 6\cm
    incipit-width = 4\cm
  }
}
