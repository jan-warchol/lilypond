;;;; This file is part of LilyPond, the GNU music typesetter.
;;;;
;;;; Copyright (C) 2012 Mike Solomon <mike@apollinemike.com>
;;;;
;;;; LilyPond is free software: you can redistribute it and/or modify
;;;; it under the terms of the GNU General Public License as published by
;;;; the Free Software Foundation, either version 3 of the License, or
;;;; (at your option) any later version.
;;;;
;;;; LilyPond is distributed in the hope that it will be useful,
;;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;;; GNU General Public License for more details.
;;;;
;;;; You should have received a copy of the GNU General Public License
;;;; along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.

(define (group-tokens-into-commands tokens out temp convert-number)
  (cond
    ((null? tokens)
     (filter (lambda (x) (not (null? x))) (append out (list (reverse temp)))))
    ((not ((if convert-number string->number number?) (car tokens)))
     (group-tokens-into-commands (cdr tokens)
                                 (append out (list (reverse temp)))
                                 (list (car tokens))
                                 convert-number))
    (else
      (group-tokens-into-commands (cdr tokens)
                                  out
                                  (cons ((if convert-number
                                             string->number
                                             values)
                                         (car tokens))
                                        temp)
                                  convert-number))))

(define (split-into-sublists l n)
  (if (eq? n 0)
      '(())
      (map (lambda (x) (list-head (list-tail l (* x n)) n))
         (iota (/ (length l) n)))))

(define n-for-command
  '(("m" . 2) ("c" . 6) ("s" . 4) ("l" . 2) ("v" . 1) ("h" . 1) ("z" . 0)
    ("M" . 2) ("C" . 6) ("S" . 4) ("L" . 2) ("V" . 1) ("H" . 1) ("Z" . 0)))

(define (expand-command c)
  (map (lambda (x) (cons (car c) x))
       (split-into-sublists (cdr c) (assoc-get (car c) n-for-command))))

(define (update-coords l x y)
  (map (lambda (idx) (+ (list-ref l idx)
                        (if (eq? 0 (modulo idx 2)) x y)))
       (iota (length l))))

(define (all-commands-to-absolute commands
                                  out
                                  first?
                                  start-x
                                  start-y
                                  cur-x
                                  cur-y)
  (cond
    ((null? commands)
     (reverse out))
    ; a close command
    ((member (caar commands) '("Z" "z"))
     (all-commands-to-absolute (cdr commands)
                               (cons '("Z") out)
                               #f
                               start-x
                               start-y
                               start-x
                               start-y))
    ; an absolute moveto or the first moveto, abs or relative
    ((or (equal? (caar commands) "M")
         (and (equal? (caar commands) "m") first?))
     (all-commands-to-absolute (cdr commands)
                               (cons (cons "M" (cdar commands)) out)
                               #f
                               (cadar commands)
                               (caddar commands)
                               (cadar commands)
                               (caddar commands)))
    ; an absolute vertical line
    ((equal? (caar commands) "V")
     (all-commands-to-absolute (cdr commands)
                               (cons (car commands) out)
                               #f
                               start-x
                               start-y
                               cur-x
                               (cadar commands)))
    ; an absolute horizontal line
    ((equal? (caar commands) "H")
     (all-commands-to-absolute (cdr commands)
                               (cons (car commands) out)
                               #f
                               start-x
                               start-y
                               (cadar commands)
                               cur-y))
    ; a relative vertical line
    ((equal? (caar commands) "v")
     (all-commands-to-absolute (cdr commands)
                               (cons `("V" ,(+ cur-y (cadar commands))) out)
                               #f
                               start-x
                               start-y
                               cur-x
                               (+ cur-y (cadar commands))))
    ; a relative horizontal line
    ((equal? (caar commands) "h")
     (all-commands-to-absolute (cdr commands)
                               (cons `("H" ,(+ cur-x (cadar commands))) out)
                               #f
                               start-x
                               start-y
                               (+ cur-x (cadar commands))
                               cur-y))
    ; a relative moveto after the first
    ((equal? (caar commands) "m")
     (let* ((new-pts (update-coords (cdar commands) cur-x cur-y)))
       (all-commands-to-absolute (cdr commands)
                                 (cons (cons "M" new-pts) out)
                                 #f
                                 (car new-pts)
                                 (cadr new-pts)
                                 (car new-pts)
                                 (cadr new-pts))))
    ; a relative curve, smooth curve, or line
    ((member (caar commands) '("s" "l" "c"))
     (let* ((new-pts (update-coords (cdar commands) cur-x cur-y)))
       (all-commands-to-absolute (cdr commands)
                                 (cons (cons (string-upcase (caar commands)) new-pts) out)
                                 #f
                                 start-x
                                 start-y
                                 (cadr (reverse new-pts))
                                 (car (reverse new-pts)))))
    ; an absolute coordinate
    ; no manipulation necessary
    (else
     (all-commands-to-absolute (cdr commands)
                               (cons (car commands) out)
                               #f
                               start-x
                               start-y
                               (cadr (reverse (car commands)))
                               (car (reverse (car commands)))))))

(define (flip-over to-reflect origin)
  (- (* 2 origin) to-reflect))

(define (absolute-commands-to-coords commands
                                     out
                                     start-x
                                     start-y
                                     cur-x
                                     cur-y)
  (cond
    ((null? commands)
     (reverse out))
    ; a close command
    ((equal? (caar commands) "Z")
     (absolute-commands-to-coords (cdr commands)
                                  (cons `(,cur-x ,cur-y ,start-x ,start-y) out)
                                  start-x
                                  start-y
                                  start-x
                                  start-y))
    ; a moveto
    ((equal? (caar commands) "M")
     (absolute-commands-to-coords (cdr commands)
                                  out
                                  (cadar commands)
                                  (caddar commands)
                                  (cadar commands)
                                  (caddar commands)))
    ; a vertical line
    ((equal? (caar commands) "V")
     (absolute-commands-to-coords (cdr commands)
                                  (cons `(,cur-x ,cur-y ,cur-x ,(cadar commands)) out)
                                  start-x
                                  start-y
                                  cur-x
                                  (cadar commands)))
    ; a horizontal line
    ((equal? (caar commands) "H")
     (absolute-commands-to-coords (cdr commands)
                                  (cons `(,cur-x ,cur-y ,(cadar commands) ,cur-y) out)
                                  start-x
                                  start-y
                                  (cadar commands)
                                  cur-y))
    ; a smooth curve
    ((equal? (caar commands) "S")
     (let* ((last-item (car out))
            (origin (list-tail last-item (- (length last-item) 2)))
            (to-reflect (list-head (list-tail last-item (- (length last-item) 4)) 2)))
       (absolute-commands-to-coords (cdr commands)
                                    (cons (append `(,cur-x ,cur-y) (map flip-over to-reflect origin) (cdar commands)) out)
                                    start-x
                                    start-y
                                    (cadr (reverse (car commands)))
                                    (car (reverse (car commands))))))
    ; a line or curve
    (else
     (absolute-commands-to-coords (cdr commands)
                                  (cons (append `(,cur-x ,cur-y) (cdar commands)) out)
                                  start-x
                                  start-y
                                  (cadr (reverse (car commands)))
                                  (car (reverse (car commands)))))))

(define (chop-string-at-tokens str tokens out)
  (let ((idx (string-index str (string-ref (car tokens) 0))))
    (if (eq? (length tokens) 1)
      (reverse (filter (lambda (x) (not (equal? x "")))
                       (append `(,(substring str (1+ idx))
                                 ,(car tokens)
                                 ,(substring str 0 idx))
                               out)))
      (chop-string-at-tokens (substring str (1+ idx))
                             (cdr tokens)
                             (cons (car tokens)
                                   (cons (substring str 0 idx)
                                         out))))))

(define (numbers-and-letters d)
  (let* ((tokens (map match:substring
                      (list-matches "[MZHVLSCmzhvlsc]" d))))
  (chop-string-at-tokens d tokens '())))

(define (not-vestigial-closing-line l)
  (if (= (length l) 4)
      (not (and (= (car l) (caddr l)) (= (cadr l) (cadddr l))))
      #t))

(define (parse-d-tokens-into-absolute-lines-and-curves d-l convert-number)
  (let* ((commands (group-tokens-into-commands d-l '() '() convert-number))
         (commands (reverse (reduce append '() (map expand-command commands))))
         (commands (all-commands-to-absolute commands
                                             '()
                                             #t
                                             0
                                             0
                                             0
                                             0))
         (commands (absolute-commands-to-coords commands
                                                '()
                                                0
                                                0
                                                0
                                                0)))
  (filter not-vestigial-closing-line commands)))

(define (parse-d-into-absolute-lines-and-curves d)
  (let* ((d (regexp-substitute/global #f "[\n]+" d 'pre " " 'post))
         (d-l (filter (lambda (x) (not (equal? x "")))
                     (reduce append
                             '()
                             (reverse (map (lambda (y) (string-split y #\space))
                                      (numbers-and-letters d)))))))
    (parse-d-tokens-into-absolute-lines-and-curves d-l #t)))
