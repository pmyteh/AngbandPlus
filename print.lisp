;;; -*- Mode: Lisp; Syntax: Common-Lisp; Package: org.langband.engine -*-

#|

DESC: print.lisp - various display code
Copyright (c) 2000-2002 - Stig Erik Sand�

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

----

ADD_DESC: Various code which just prints stuff somewhere

|#

(in-package :org.langband.engine)


(defconstant +token-name+ 1)
(defconstant +token-cur-mp+ 2)
(defconstant +token-max-mp+ 3)
(defconstant +token-nrm-lvl+ 4)
(defconstant +token-big-lvl+ 5)
(defconstant +token-nrm-xp+ 6)
(defconstant +token-big-xp+ 7)
(defconstant +token-cur-hp+ 8)
(defconstant +token-max-hp+ 9)
(defconstant +token-cur-ac+ 10)
(defconstant +token-au+ 11)
(defconstant +token-stat+ 12)
(defconstant +token-empty-field+ 24)

(defun print-field (str coord)
  "Print string at given coordinates in light-blue."
  ;; clear and then write
  (let ((y (car coord))
	(x (cdr coord)))

    (c-prt-token! +term-white+ +token-empty-field+
		  y x)

    ;; (c-col-put-str! +term-white+ "            " y x)
    (c-col-put-str! +term-l-blue+ str y x)
    ))

(defun print-stat (pl setting num)
  "Prints stats in the left frame."

  (let* ((stat-val (svref (player.active-stats pl) num))
	 (max-val (svref (player.modbase-stats pl) num))
	 (reduced-stat-p (> max-val stat-val))
	 (stat-set (slot-value setting 'stat))
	 (row (car stat-set))
	 (col (cdr stat-set))
	 ;;(name (get-stat-name-from-num num))
	 )
    
    (c-prt-token! +term-white+ (if reduced-stat-p
				   (+ +token-stat+ num)
				   (+ +token-stat+ num 6))
		  (+ num row)
		  col)
    
;;    (c-put-str! (if reduced-stat-p
;;		   name
;;		   (string-upcase name))
;;	       (+ num row)
;;	       col)

    (c-prt-stat! (if reduced-stat-p +term-yellow+ +term-l-green+)
		 stat-val (+ row num) (+ col 6))
    
    ))

(defun print-title (pl setting)
  "Prints the title for the class at the player's level."
  
  (let* ((the-class (player.class pl))
	 (the-lvl (player.level pl))
	 (title-set (slot-value setting 'title))
	 (title (get-title-for-level the-class the-lvl)))
    (print-field title title-set)))

(defun print-level (pl setting)
  "Prints level in the left frame."
  
  (let* ((lev (player.level pl))
	 (lev-set (slot-value setting 'level))
	 (lower-lvl-p (< lev (player.max-level pl))))
    
    (c-prt-token! +term-white+ (if lower-lvl-p +token-nrm-lvl+ +token-big-lvl+)
		  (car lev-set) (cdr lev-set))

    (c-prt-number! (if lower-lvl-p +term-yellow+ +term-l-green+)
		   lev
		   6
		   (car lev-set)
		   (+ (cdr lev-set) 6))))

(defun print-xp (pl setting)
  "Prints xp in the left frame."
  (let* ((xp (player.cur-xp pl))
	 (xp-set (slot-value setting 'xp))
	 (lower-xp-p (< xp (player.max-xp pl))))

    (c-prt-token! +term-white+ (if lower-xp-p +token-nrm-xp+ +token-big-xp+)
		  (car xp-set) (cdr xp-set))

    (c-prt-number! (if lower-xp-p +term-yellow+ +term-l-green+)
		   xp
		   8
		   (car xp-set)
		   (+ (cdr xp-set) 4))))


(defun print-gold (pl setting)
  "Prints gold to left frame."
  
  (let ((gold (player.gold pl))
	(gold-set (slot-value setting 'gold)))

    (c-prt-token! +term-white+ +token-au+ (car gold-set) (cdr gold-set))

    (c-prt-number! +term-l-green+ gold 9
		   (car gold-set)
		   (+ (cdr gold-set) 3))
    ))


(defun print-armour-class (pl setting)
  "Prints AC to left frame."
  
  (let* ((perc (player.perceived-abilities pl))
	 (ac (+ (pl-ability.base-ac perc)
		(pl-ability.ac-modifier perc)))
	(ac-set (slot-value setting 'ac)))

    (c-prt-token! +term-white+ +token-cur-ac+ (car ac-set) (cdr ac-set))

    (c-prt-number! +term-l-green+
		   ac
		   5
		   (car ac-set)
		   (+ (cdr ac-set) 7))))


(defun print-hit-points (pl setting)
  "Prints hit-points info to left frame."
  
  (let ((cur-hp (current-hp pl))
	(max-hp (maximum-hp pl))
	(cur-set (slot-value setting 'cur-hp))
	(max-set (slot-value setting 'max-hp)))
    
    (c-prt-token! +term-white+ +token-max-hp+ (car max-set) (cdr max-set))
    (c-prt-token! +term-white+ +token-cur-hp+ (car cur-set) (cdr cur-set))

    ;; max
    (c-prt-number! +term-l-green+
		    max-hp
		    5
		    (car max-set)
		    (+ (cdr max-set) 7))

    ;; cur
    (c-prt-number! (cond ((>= cur-hp max-hp) +term-l-green+)
			 ((> cur-hp (int-/ (* max-hp *hitpoint-warning*) 10)) +term-yellow+)
			 (t +term-red+))
		   cur-hp
		   5
		   (car cur-set)
		   (+ (cdr cur-set) 7))
    
    ))

(defun print-mana-points (pl setting)
  "Prints mana-points info to left frame."
  
  (let ((cur-hp (player.cur-mana pl))
	(max-hp (player.max-mana pl))
	(cur-set (slot-value setting 'cur-mana))
	(max-set (slot-value setting 'max-mana)))

    (c-prt-token! +term-white+ +token-max-mp+ (car max-set) (cdr max-set))
    (c-prt-token! +term-white+ +token-cur-mp+ (car cur-set) (cdr cur-set))
  
    (c-prt-number! +term-l-green+
		   max-hp
		   5
		   (car max-set)
		   (+ (cdr max-set) 7))


    (c-prt-number! (cond ((>= cur-hp max-hp) +term-l-green+)
			 ((> cur-hp (int-/ (* max-hp *hitpoint-warning*) 10)) +term-yellow+)
			 (t +term-red+))

		   cur-hp
		   5
		   (car cur-set)
		   (+ (cdr cur-set) 7))

    ))
  

(defun print-basic-frame (variant dun pl)
  "Prints the left frame with basic info"
  
  (declare (ignore dun))
  ;;  (warn "Printing basic frame..")

  (let ((pr-set (get-setting variant :basic-frame-printing))
	(stat-len (variant.stat-length variant)))
  
    (print-field (get-race-name pl) (slot-value pr-set 'race))
    (print-field (get-class-name pl) (slot-value pr-set 'class))
    (print-title pl pr-set)

    (print-level pl pr-set)
    (print-xp pl pr-set) 
  
    (dotimes (i stat-len)
      (print-stat pl pr-set i))

    (print-armour-class pl pr-set)
    (print-hit-points pl pr-set)
    (print-mana-points pl pr-set) 

    (print-gold pl pr-set)
    
    #+maintainer-mode
    (let ((food-set (slot-value pr-set 'food))
	  (energy-set (slot-value pr-set 'energy)))
      
      (print-field "Food" food-set)
      (print-field "Energy" energy-set)
      
      (c-prt-number! +term-l-green+ (player.food pl) 5
		     (car food-set) (+ (cdr food-set) 7))
      (c-prt-number! +term-l-green+ (player.energy pl) 5
		     (car energy-set) (+ (cdr energy-set) 7))
      )
      
    
    (print-depth *level* pr-set)
  
    ))


(defmethod print-depth (level setting)
  "prints current depth somewhere"
  (declare (ignore setting))
  (with-foreign-str (s)
    (lb-format s "~d ft" (* 50 (level.depth level)))
    (c-col-put-str! +term-l-blue+ s *last-console-line* 70) ;;fix 
    )) 


(defmethod display-creature ((variant variant) (player player) &key mode)

  (declare (ignore mode))
  
  (c-clear-from! 0)
  (display-player-misc  variant player)
  (display-player-stats variant player)
  (display-player-extra variant player)
  )

(defun display-player-misc (variant player)
  (declare (ignore variant))
  (let* ((the-class (player.class player))
	 (the-lvl (player.level player))
	 (title (get-title-for-level the-class the-lvl)))

    (flet ((print-info (title text row)
	     (c-put-str! title row 1)
	     (c-col-put-str! +term-l-blue+ text row 8)))

      (print-info "Name" (player.name player) 2)
      (print-info "Gender" (get-gender-name player) 3)
      (print-info "Race" (get-race-name player) 4)
      (print-info "Class"  (get-class-name player) 5)
      (print-info "Title" title 6)
      
      (print-info "HP" (format nil "~d/~d" (current-hp player)
			       (maximum-hp player))
		  7)
      
      (print-info "MP" (format nil "~d/~d" (player.cur-mana player)
			       (player.max-mana player))
		       8)

      )))

(defun display-player-extra (variant player)
  (declare (ignore variant))
  (let* ((pl player)
	 (col 26)
	 (f-col (+ 9 25)) 
	 (cur-xp (player.cur-xp pl))
	 (max-xp (player.max-xp pl))
	 (lvl (player.level pl))
	 (misc (player.misc pl)))
    

    (c-put-str! "Age" 3 col)
    (c-col-put-str! +term-l-blue+ (%get-4str (playermisc.age misc)) 3 f-col)

    (c-put-str! "Height" 4 col)
    (c-col-put-str! +term-l-blue+ (%get-4str (playermisc.height misc))  4 f-col)

    (c-put-str! "Weight" 5 col)
    (c-col-put-str! +term-l-blue+ (%get-4str (playermisc.weight misc))  5 f-col)
    (c-put-str! "Status" 6 col)
    (c-col-put-str! +term-l-blue+ (%get-4str (playermisc.status misc))  6 f-col)

    ;; always in maximize and preserve, do not include

    ;; another area
    (setq col 1)
    (setq f-col (+ col 8))

    (c-put-str! "Level" 10 col)
    (c-col-put-str! (if (>= lvl
			   (player.max-level pl))
		       +term-l-green+
		       +term-yellow+)
		   (format nil "~10d" lvl)  10 f-col)
  
  
    (c-put-str! "Cur Exp" 11 col)
    
    (c-col-put-str! (if (>= cur-xp
			   max-xp)
		       +term-l-green+
		       +term-yellow+)
		   (format nil "~10d" cur-xp)  11 f-col)
    
    (c-put-str! "Max Exp" 12 col)
    
    (c-col-put-str! +term-l-green+
		   (format nil "~10d" max-xp)  12 f-col)
    

    (c-put-str! "Adv Exp" 13 col)
    (c-col-put-str! +term-l-green+
		   (format nil "~10d" (aref (player.xp-table pl)
					    (player.level pl)))
		   13 f-col)

    
    
    (c-put-str! "Gold" 15 col)

    (c-col-put-str! +term-l-green+
		   (format nil "~10d" (player.gold pl))  15 f-col)

    (c-put-str! "Burden" 17 col)
    (let* ((weight (player.burden pl))
	   (pound (int-/ weight 10))
	   (frac (mod weight 10))
	   (str (format nil "~10d.~d lbs" pound frac)))
      (c-col-put-str! +term-l-green+ str 17 f-col))

    ;; middle again
    (setq col 26)
    (setq f-col (+ col 5))

    (c-put-str! "Armour" 10 col)
    (let* ((perc (player.perceived-abilities pl))
	   (p-ac (pl-ability.base-ac perc))
	   (p-acmod (pl-ability.ac-modifier perc)))
         
      (c-col-put-str! +term-l-blue+
		      (format nil "~12@a" (format nil "[~d,~@d]" p-ac p-acmod))
		   10 (1+ f-col)))

;;    (let ((weapon (get-weapon pl)))
;;     
;;      nil)
	  
      
    (c-put-str! "Fight" 11 col)
    (c-col-put-str! +term-l-blue+ (%get-13astr "(+0,+0)")
		   11 f-col)

    (c-put-str! "Melee" 12 col)
    (c-col-put-str! +term-l-blue+ (%get-13astr "(+0,+0)")
		   12 f-col)

    (c-put-str! "Shoot" 13 col)
    (c-col-put-str! +term-l-blue+ (%get-13astr "(+0,+0)")
		   13 f-col)
    
    (c-put-str! "Blows" 14 col)
    (c-col-put-str! +term-l-blue+ (%get-13astr "1/turn")
		   14 f-col)
    
    (c-put-str! "Shots" 15 col)
    (c-col-put-str! +term-l-blue+ (%get-13astr "1/turn")
		   15 f-col)
		   
    (c-put-str! "Infra" 17 col)
	    
    (c-col-put-str! +term-l-blue+
		   (format nil "~10d ft" (* 10 (player.infravision pl)))
		   17 f-col)

    ;; then right
    (setq col 49)
    (setq f-col (+ col 14))

    (flet ((print-skill (skill div row)
	     (declare (ignore div))
	     (let ((val (slot-value (player.skills pl) skill)))
	       (c-col-put-str! +term-l-green+
			      (format nil "~9d" val)
			      row
			      f-col))))
      
      (c-put-str! "Saving Throw" 10 col)
      (print-skill 'saving-throw 6 10)

      (c-put-str! "Stealth" 11 col)
      (print-skill 'stealth 1 11)
      
      (c-put-str! "Fighting" 12 col)
      (print-skill 'fighting 12 12)

      (c-put-str! "Shooting" 13 col)
      (print-skill 'shooting 12 13)

      (c-put-str! "Disarming" 14 col)
      (print-skill 'disarming 8 14)

      (c-put-str! "Magic Device" 15 col)
      (print-skill 'device 6 15)

      (c-put-str! "Perception" 16 col)
      (print-skill 'perception 6 16)

      (c-put-str! "Searching" 17 col)
      (print-skill 'searching 6 17))
      
  ))


(defun display-player-stats (variant player)
  ;;  (warn "Displaying character.. ")
  
  (let ((row 3)
	(col 42)
;;	(num-stats 6)
	;; more pressing variables
	(stat-len (variant.stat-length variant))
	(base (player.base-stats player))
;;	(cur (player.curbase-stats player))
	(mod (player.modbase-stats player))
	(active (player.active-stats player))
	(racial-adding (race.stat-changes (player.race player)))
	(class-adding (class.stat-changes (player.class player)))
	)
    ;; labels
    
    (c-col-put-str! +term-white+ "  Self" (1- row) (+ col  5))
    (c-col-put-str! +term-white+ " RB"    (1- row) (+ col 12))
    (c-col-put-str! +term-white+ " CB"    (1- row) (+ col 16))
    (c-col-put-str! +term-white+ " EB"    (1- row) (+ col 20))
    (c-col-put-str! +term-white+ "  Best" (1- row) (+ col 24))


    (dotimes (i stat-len)
      (let ((its-base (gsdfn base i))
	    ;;(its-cur (gsdfn cur i))
	    (its-mod (gsdfn mod i))
	    (its-active (gsdfn active i))
	      
	    (cur-race-add (gsdfn racial-adding i))
	    (cur-class-add (gsdfn  class-adding i)))

	(c-put-str! (get-stat-name-from-num i)
		   (+ i row)
		   col)

	;; base stat
	(c-prt-stat! +term-l-green+ its-base (+ row i) (+ col 5))

;;	(c-col-put-str! +term-l-green+ (%get-stat its-base)
;;		       (+ row i)
;;		       (+ col 5))


	;; racial bonus
	(c-col-put-str! +term-l-green+ (format nil "~3@d" cur-race-add)
		       (+ row i)
		       (+ col 12))

	;; class bonus
	(c-col-put-str! +term-l-green+ (format nil "~3@d" cur-class-add)
		       (+ row i)
		       (+ col 16))

	;; equipment
	(c-col-put-str! +term-l-green+ " +0"
		       (+ row i)
		       (+ col 20))

	;; max stat
	(c-prt-stat! +term-l-green+ its-mod (+ row i) (+ col 24))

;;	(c-col-put-str! +term-l-green+ (%get-stat its-mod)
;;		       (+ row i)
;;		       (+ col 24))

	;; if active is lower than max
	(when (< its-active its-mod)
	  ;; max stat
	  (c-prt-stat! +term-yellow+ its-active (+ row i) (+ col 28)))

;;	  (c-col-put-str! +term-yellow+ (%get-stat its-active)
;;			 (+ row i)
;;			 (+ col 28)))
	      
	      
	)))


  )


;; no warning on duplicates
(defun register-help-topic& (variant topic)
  "Registers a help-topic with the variant."
  (setf (gethash (help-topic-id topic) (variant.help-topics variant)) topic))


(defun display-help-topics (variant title start-row)
  "Displays help-topics to screen and asks for input on further help."
  (let ((topics (variant.help-topics variant))
	(title-len (length title)))

    (flet ((show-title ()
	     (c-col-put-str! +term-l-blue+ title start-row 12)
	     (c-col-put-str! +term-l-blue+ (make-string title-len :initial-element #\=)
			     (1+ start-row) 12))
	   (show-entries ()
	     (loop for cnt from 3
		   for i being the hash-values of topics
		   do
		   (let ((key (help-topic-key i)))
		     (c-col-put-str! +term-l-green+ (format nil "~a." key) (+ start-row cnt) 3)
		     (c-col-put-str! +term-l-green+ (help-topic-name i) (+ start-row cnt) 6)
		     )))
	   (get-valid-key ()
	     (c-col-put-str! +term-l-blue+ "-> Please enter selection (Esc to exit): " 20 3)
	     (read-one-character)))

      (loop 
       (show-title)
       (show-entries)
       (let ((key (get-valid-key)))
	 (when (eql key +escape+)
	   (return-from display-help-topics nil)))
       ))))

(defun print-attack-graph (var-obj player)
;;  (declare (ignore player))
    (c-clear-from! 0)
    
    (dotimes (i 10)
      (let ((y (- 19 (* i 2))))
	(c-col-put-str! +term-l-blue+ (format nil "~3d%" (* i 10))
			y 3)))
    (dotimes (i 20)
      (c-col-put-str! +term-l-blue+ "|" i 7))

    (c-col-put-str! +term-l-blue+ "CHANCE TO HIT" 1 64)
    (c-col-put-str! +term-l-blue+ "=============" 2 64)
    
    (c-col-put-str! +term-l-red+ "Unarm." 20 8)
    (c-col-put-str! +term-white+ "Leath." 21 13)
    (c-col-put-str! +term-orange+ "L. met." 22 18)
    (c-col-put-str! +term-yellow+ "Met." 20 22)
    (c-col-put-str! +term-violet+ "H. Met." 21 26)
    (c-col-put-str! +term-l-green+ "Plate" 22 30)
    (c-col-put-str! +term-l-red+ "H. Plate" 20 35)
    (c-col-put-str! +term-white+ "Dragon" 21 40)
    (c-col-put-str! +term-orange+ "H. Dragon" 22 44)
    (c-col-put-str! +term-yellow+ "Ench Drg" 20 50)
    (c-col-put-str! +term-violet+ "Legend" 21 62)
    (c-col-put-str! +term-l-green+ "Myth" 22 74)

    (let ((skill (get-known-combat-skill var-obj player)))
      
      (flet ((get-x (val)
	       (+ 8 val))
	     (get-y (val)
	       (- 19 (int-/ val 5))))
	(loop for i from 5 to 180 by 5
	      for j from 1 by 2
	      do
	      (let ((chance (get-chance var-obj skill i))
		    (desc (get-armour-desc var-obj i)))
		(check-type desc cons)
		(c-col-put-str! (cdr desc) "*" (get-y chance)
				(get-x j)))
	      
	      )))
      
			    
    (c-pause-line! *last-console-line*)
    )

(defun print-attack-table (var-obj player)
;;  (declare (ignore player))
    (c-clear-from! 0)

    (c-col-put-str! +term-l-blue+ "CHANCE TO HIT" 0 2)
    (c-col-put-str! +term-l-blue+ "=============" 1 2)
    (let ((last-colour +term-green+)
	  (count 2)
	  (skill (get-known-combat-skill var-obj player)))
      (loop for i from 5 to 200 by 10
	    
	    do
	    (let ((desc (get-armour-desc var-obj i))
		  (chance (get-chance var-obj skill i)))
	      (check-type desc cons)
	      (cond ((equal last-colour (cdr desc))
		     ;; next
		     )
		    (t
		     (setf last-colour (cdr desc))
		     (incf count)
		     (c-col-put-str! (cdr desc) (format nil "~40a: ~a%" (car desc) chance)
				     count 4)
		     ))
	      ))

      (c-print-text! 2 16 +term-l-blue+ "
The armour-value describes a full set-up of armour, including
    helmet, shield, gloves, boots, cloak and body-armour.  Parts of
    the outfit is expected to have appropriate enchantments, e.g a
    dragon-armour will always be slightly enchanted, just as a
    full-plate combat armour is enchanted to allow it's wearer to move
    at all.  A creature can have natural armour, but
    it might be just as tough as plated armour and as such use the
    plated armour label. " :end-col 75)

      
      
      (c-pause-line! *last-console-line*)
      ))

(defun print-resists (var-obj player)
;;  (declare (ignore player))
    (c-clear-from! 0)

    (c-col-put-str! +term-l-blue+ "RESISTANCES" 0 2)
    (c-col-put-str! +term-l-blue+ "===========" 1 2)

    (let ((elms (variant.elements var-obj))
	  (resists (player.resists player))
	  (row 3))

      (dolist (i elms)
	(let* ((idx (element.number i))
	       (str (format nil "~20a ~20s ~10s" (element.name i) (element.symbol i) idx)))
	  (when (plusp (aref resists idx))
	    (c-col-put-str! +term-l-red+ "*" row 1)) 
	  (c-col-put-str! +term-l-green+ str row 3)
	  (incf row)))

      (c-pause-line! *last-console-line*)
      ))

(defun print-misc-info (var-obj player)
  (declare (ignore var-obj player))

  (c-clear-from! 0)
  (c-col-put-str! +term-l-blue+ "MISC INFO" 0 2)
  (c-col-put-str! +term-l-blue+ "=========" 1 2)

  (c-col-put-str! +term-l-green+ "Height:" 3 2)
  (c-col-put-str! +term-yellow+ (format nil "~5d" 0) 3 12)
;;  (c-col-put-str! +term-l-green+ "Weight:" 4 2)
;;  (c-col-put-str! +term-yellow+ (format nil "~5d" (creature.weight player)) 4 12)
  (c-pause-line! *last-console-line*)
  )
