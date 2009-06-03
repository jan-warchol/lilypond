%% Do not edit this file; it is auto-generated from LSR http://lsr.dsi.unimi.it
%% This file is in the public domain.
\version "2.13.1"

\header {
  lsrtags = "pitches, staff-notation, tweaks-and-overrides"

%% Translation of GIT committish: 4866dfd58d5c3a8cab4c6c06d5c4fca8e05a3cd7
  doctitlees = "Trucaje de las propiedades de clave"
  texidoces = "
La instrucción @code{\\clef \"treble_8\"} equivale a un ajuste de
@code{clefGlyph}, @code{clefPosition} (que controla la posición
vertical de la clave), @code{middleCPosition} y
@code{clefOctavation}.  Se imprime una clave cada vez que se
modifica cualquiera de las propiedades excepto
@code{middleCPosition}.

Observe que la modificación del glifo, la posición de la clave o
su octavación, no cambian 'per se' la posición de las siguientes
notas del pentagrama: para hacer esto también se debe especificar
la posición del Do central.  Los parámetros posicionales están en
relación con la tercera línea del pentagrama, los números
positivos desplazan hacia arriba, contando una unidad por cada
línea y espacio.  El valor de @code{clefOctavation} se
establecería normalmente a 7, -7, 15 or -15, pero son válidos
otros valores.

Cuando se produce un cambio de clave en el salto de línea se
imprime la clave nueva tanto al final de la línea anterior como al
principio de la nueva, de forma predeterminada.  Si no se necesita
la clave de advertencia al final de la línea anterior, se puede
quitar estableciendo el valor de la propiedad
@code{explicitClefVisibility} de @code{Staff}, a
@code{end-of-line-invisible}.  El comportamiento predeterminado se
puede recuperar con @code{\\unset Staff.explicitClefVisibility}.

Los siguientes ejemplos muestran las posibilidades cuando se
ajustan estas propiedades manualmente.  En la primera línea, los
cambios manuales preservan el posicionamiento relativo estándar de
las claves y las notas, pero no lo hacen en la segunda línea.
"
  
%% Translation of GIT committish: 2f4e16a76afee992a5f8d11e119667efe7238e7d
  doctitlede = "Eigenschaften des Schlüssels optimieren"
  texidocde = "
Der Befehl @code{\\clef \"treble_8\"} ist gleichbedeutend mit einem
expliziten Setzen der Eigenschaften von @code{clefGlyph},
@code{clefPosition} (welche die vertikale Position des Schlüssels bestimmt),
@code{middleCPosition} und @code{clefOctavation}.  Ein Schlüssel wird
ausgegeben, wenn eine der Eigenschaften außer @code{middleCPosition} sich
ändert.

Eine Änderung des Schriftzeichens (Glyph), der Schlüsselposition oder der
Oktavierung selber ändert noch nicht die Position der darauf folgenden Noten
auf dem System: das geschieht nur, wenn auch die Position des
eingestrichenen@tie{}C (middleCPosition) angegeben wird.  Die
Positionsparameter sind relativ zur Mittellinie des Systems, dabei versetzen
positive Zahlen die Position nach oben, jeweils eine Zahl für jede Linie
plus Zwischenraum.  Der @code{clefOctavation}-Wert ist normalerweise auf 7,
-7, 15 oder -15 gesetzt, aber auch andere Werte sind gültig.

Wenn ein Schlüsselwechsel an einem Zeilenwechsel geschieht, wird das neue
Symbol sowohl am Ende der alten Zeilen als auch am Anfang der neuen Zeile
ausgegeben.  Wenn der Warnungs-Schlüssel am Ende der alten Zeile nicht
erforderlich ist, kann er unterdrückt werden, indem die
@code{explicitClefVisibility}-Eigenschaft des @code{Staff}-Kontextes auf den
Wert @code{end-of-line-invisible} gesetzt wird.  Das Standardverhalten kann
mit @code{\\unset Staff.explicitClefVisibility} wieder hergestellt werden.

Die folgenden Beispiele zeigen die Möglichkeiten, wenn man diese
Eigenschaften manuell setzt.  Auf der ersten Zeile erhalten die manuellen
Änderungen die ursprüngliche relative Positionierung von Schlüssel und
Noten, auf der zweiten Zeile nicht.
"

  texidoc = "
The command @code{\\clef \"treble_8\"} is equivalent to setting
@code{clefGlyph}, @code{clefPosition} (which controls the vertical
position of the clef), @code{middleCPosition} and
@code{clefOctavation}. A clef is printed when any of the properties
except @code{middleCPosition} are changed.


Note that changing the glyph, the position of the clef, or the
octavation does not in itself change the position of subsequent notes
on the staff: the position of middle C must also be specified to do
this. The positional parameters are relative to the staff center line,
positive numbers displacing upwards, counting one for each line and
space. The @code{clefOctavation} value would normally be set to 7, -7,
15 or -15, but other values are valid.


When a clef change takes place at a line break the new clef symbol is
printed at both the end of the previous line and the beginning of the
new line by default. If the warning clef at the end of the previous
line is not required it can be suppressed by setting the @code{Staff}
property @code{explicitClefVisibility} to the value
@code{end-of-line-invisible}. The default behavior can be recovered
with  @code{\\unset Staff.explicitClefVisibility}.

The following examples show the possibilities when setting these
properties manually. On the first line, the manual changes preserve the
standard relative positioning of clefs and notes, whereas on the second
line, they do not. 

"
  doctitle = "Tweaking clef properties"
} % begin verbatim

\layout { ragged-right = ##t }

{
  % The default treble clef
  c'1
  % The standard bass clef
  \set Staff.clefGlyph = #"clefs.F"
  \set Staff.clefPosition = #2
  \set Staff.middleCPosition = #6
  c'1
  % The baritone clef
  \set Staff.clefGlyph = #"clefs.C"
  \set Staff.clefPosition = #4
  \set Staff.middleCPosition = #4
  c'1
  % The standard choral tenor clef
  \set Staff.clefGlyph = #"clefs.G"
  \set Staff.clefPosition = #-2
  \set Staff.clefOctavation = #-7
  \set Staff.middleCPosition = #1
  c'1
  % A non-standard clef
  \set Staff.clefPosition = #0
  \set Staff.clefOctavation = #0
  \set Staff.middleCPosition = #-4
  c'1 \break

  % The following clef changes do not preserve
  % the normal relationship between notes and clefs:

  \set Staff.clefGlyph = #"clefs.F"
  \set Staff.clefPosition = #2
  c'1
  \set Staff.clefGlyph = #"clefs.G"
  c'1
  \set Staff.clefGlyph = #"clefs.C"
  c'1
  \set Staff.clefOctavation = #7
  c'1
  \set Staff.clefOctavation = #0
  \set Staff.clefPosition = #0
  c'1
  
  % Return to the normal clef:

  \set Staff.middleCPosition = #0
  c'1
}

