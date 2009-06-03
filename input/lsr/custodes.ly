%% Do not edit this file; it is auto-generated from LSR http://lsr.dsi.unimi.it
%% This file is in the public domain.
\version "2.13.1"

\header {
  lsrtags = "ancient-notation, tweaks-and-overrides"

%% Translation of GIT committish: aea975539ec44fd0f1a8fd25930b88b5ab64b53a
  texidoces = "
Se pueden tipografiar «custos» en diferentes estilos.

"
  doctitlees = "Custos"

  texidoc = "
Custodes may be engraved in various styles.

"
  doctitle = "Custodes"
} % begin verbatim

\layout { ragged-right = ##t }

\new Staff \with { \consists "Custos_engraver" } \relative c' {
  \override Staff.Custos #'neutral-position = #4
  
  \override Staff.Custos #'style = #'hufnagel
  c1^"hufnagel" \break
  <d a' f'>1
  
  \override Staff.Custos #'style = #'medicaea
  c1^"medicaea" \break
  <d a' f'>1
  
  \override Staff.Custos #'style = #'vaticana
  c1^"vaticana" \break
  <d a' f'>1
  
  \override Staff.Custos #'style = #'mensural
  c1^"mensural" \break
  <d a' f'>1
}

