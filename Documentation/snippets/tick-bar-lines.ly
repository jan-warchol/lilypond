%% DO NOT EDIT this file manually; it is automatically
%% generated from LSR http://lsr.dsi.unimi.it
%% Make any changes in LSR itself, or in Documentation/snippets/new/ ,
%% and then run scripts/auxiliar/makelsr.py
%%
%% This file is in the public domain.
\version "2.14.2"

\header {
  lsrtags = "staff-notation"

%% Translation of GIT committish: d5307870fe0ad47904daba73792c7e17b813737f
  texidocfr = "
Lorsque les barres de mesure ne sont là que dans un but de coordination
et non pour accentuer le rythme, il arrive souvent qu'elles se
présentent sous la forme d'une simple encoche.

"
  doctitlefr = "Barre de mesure en encoche"

  texidoc = "
'Tick' bar lines are often used in music where the bar line is used
only for coordination and is not meant to imply any rhythmic stress.

"
  doctitle = "Tick bar lines"
} % begin verbatim


\relative c' {
  \set Score.defaultBarType = #"'"
  c4 d e f
  g4 f e d
  c4 d e f
  g4 f e d
  \bar "|."
}
