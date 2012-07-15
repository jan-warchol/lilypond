%% DO NOT EDIT this file manually; it is automatically
%% generated from LSR http://lsr.dsi.unimi.it
%% Make any changes in LSR itself, or in Documentation/snippets/new/ ,
%% and then run scripts/auxiliar/makelsr.py
%%
%% This file is in the public domain.
\version "2.14.2"

\header {
  lsrtags = "fretted-strings"

%% Translation of GIT committish: b482c3e5b56c3841a88d957e0ca12964bd3e64fa
  texidoces = "
La polifonía se crea de la misma forma en un @code{TabStaff} que
en una pauta normal.

"
  doctitlees = "Polifonía en tablaturas"


%% Translation of GIT committish: 0a868be38a775ecb1ef935b079000cebbc64de40
  texidocde = "
Polyphonie kann in einer Tabulatur (@code{TabStaff}) genauso wie in einem
normalen Notensystem erstellt werden.

"
  doctitlede = "Polyphonie in einer Tabulatur"

%% Translation of GIT committish: 28097cf54698db364afeb75658e4c8e0e0ccd716
  texidocfr = "
Une section polyphonique s'obtient dans un @code{TabStaff} de la même
 manière que dans une portée normale.

"
  doctitlefr = "Polyphonie en mode tablature"

  texidoc = "
Polyphony is created the same way in a @code{TabStaff} as in a regular
staff.

"
  doctitle = "Polyphony in tablature"
} % begin verbatim


upper = \relative c' {
  \time 12/8
  \key e \minor
  \voiceOne
  r4. r8 e, fis g16 b g e e' b c b a g fis e
}

lower = \relative c {
  \key e \minor
  \voiceTwo
  r16 e d c b a g4 fis8 e fis g a b c
}

\score {
  <<
    \new StaffGroup = "tab with traditional" <<
      \new Staff = "guitar traditional" <<
        \clef "treble_8"
        \context Voice = "upper" \upper
        \context Voice = "lower" \lower
      >>
      \new TabStaff = "guitar tab" <<
        \context TabVoice = "upper" \upper
        \context TabVoice = "lower" \lower
      >>
    >>
  >>
}

