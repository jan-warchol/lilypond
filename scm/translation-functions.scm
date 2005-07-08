;;;; translation-functions.scm --
;;;;
;;;;  source file of the GNU LilyPond music typesetter
;;;; 
;;;; (c) 1998--2005 Han-Wen Nienhuys <hanwen@cs.uu.nl>
;;;;		     Jan Nieuwenhuizen <janneke@gnu.org>

(define-public (denominator-tuplet-formatter mus)
  (number->string (ly:music-property mus 'denominator)))

(define-public (fraction-tuplet-formatter mus)
  (string-append
   (number->string (ly:music-property mus 'denominator))
   ":"
   (number->string (ly:music-property mus 'numerator))))

;; metronome marks
(define-public (format-metronome-markup event context)
  (let* ((dur (ly:music-property event 'tempo-unit))
       (count (ly:music-property event 'metronome-count))
       (note-mark (make-smaller-markup
		   (make-note-by-number-markup (ly:duration-log dur)
					       (ly:duration-dot-count dur)
					       1))))  
    (make-line-markup
     (list
      (make-general-align-markup Y DOWN note-mark)
      (make-simple-markup  "=")
      (make-simple-markup (number->string count))))))

(define-public (format-mark-alphabet mark context)
  (make-bold-markup (make-markalphabet-markup (1- mark))))

(define-public (format-mark-box-alphabet mark context)
  (make-bold-markup (make-box-markup (make-markalphabet-markup (1- mark)))))

(define-public (format-mark-letters mark context)
  (make-bold-markup (make-markletter-markup (1- mark))))

(define-public (format-mark-numbers mark context)
  (make-bold-markup (number->string mark)))

(define-public (format-mark-barnumbers mark context)
  (make-bold-markup (number->string (ly:context-property context 'currentBarNumber))))

(define-public (format-mark-box-letters mark context)
  (make-bold-markup (make-box-markup (make-markletter-markup (1- mark)))))

(define-public (format-mark-box-numbers mark context)
  (make-bold-markup (make-box-markup (number->string mark))))

(define-public (format-mark-box-barnumbers mark context)
  (make-bold-markup (make-box-markup
    (number->string (ly:context-property context 'currentBarNumber)))))

(define-public (format-pitched-trill-head pitch do-print-accidental context)
  (make-override-markup
   '(word-space . 0.0)
   (make-line-markup
    (append
     (list
      (make-musicglyph-markup "accidentals.leftparen"))
     (if do-print-accidental
	 (list (make-musicglyph-markup
		(string-append "accidentals."
			       (number->string (ly:pitch-alteration pitch))))
	       (make-hspace-markup 0.2))
	 '())
     
     (list
      (make-musicglyph-markup "noteheads.s2")
      (make-musicglyph-markup "accidentals.rightparen"))
     ))))
