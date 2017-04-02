;;; cinfo.el --- tiny emacs lisp interface for cinfo

;; $Id: cinfo.el,v 1.4 1996/06/29 13:21:37 sandro Exp $

;; Copyright (C) 1995, 1996 Sandro Sigala

;; the binded key is `C-x z'

;;; Code:

(message "[loading cinfo...]")

(defconst cinfo-binded-key "\C-xz")
(defconst cinfo-default-buffer-name "*cinfo output*")

(defvar cinfo-buffer nil)

(defun cinfo-get-word ()
  (let ((cinfo-casechars "[a-zA-Z_]+[a-zA-Z_0-9]*")
	(cinfo-not-casechars "[^A-Za-z_0-9]"))
    (if (not (looking-at cinfo-casechars))
	(re-search-forward cinfo-casechars (point-max) t))
    (re-search-backward cinfo-not-casechars (point-min) t)
    (or (re-search-forward cinfo-casechars (point-max) t)
	(re-search-backward cinfo-not-casechars (point-min) t)
	(error "[no word found]"))
    (buffer-substring (match-beginning 0) (point))))

(defun cinfo-make-documentation-buffer (sym)
  (if cinfo-buffer
      (kill-buffer cinfo-buffer))
  (setq cinfo-buffer (generate-new-buffer cinfo-default-buffer-name))
  (message "running cinfo...")
  (call-process "cinfo" nil cinfo-buffer nil sym)
  (if (save-excursion
	(set-buffer cinfo-buffer)
	(if (< 2 (point-max))
	    (progn
	      (goto-char (point-min))
	      (message nil) t)
	  nil))
      (display-buffer cinfo-buffer)
    (kill-buffer cinfo-buffer)
    (error "[no documentation found]" sym)))
 
(defun cinfo-document-symbol (arg)
"This command runs the program `cinfo' to retrive documentation about the
specified symbol and places the results in a buffer named `*cinfo output*'."
  (interactive "P")
  (let* ((default-symbol (cinfo-get-word))
	 (symbol
	  (read-string (format "Symbol: %s"
			       (if (string= default-symbol "") ""
				 (format "(default: %s) "
					 default-symbol))))))
    (and (string= symbol "")
	 (if (string= default-symbol "")
	     (error "[no symbol given]")
	   (setq symbol default-symbol)))
    (cinfo-make-documentation-buffer symbol)))

(global-set-key cinfo-binded-key 'cinfo-document-symbol)

(provide 'cinfo)

(message nil)

;;; cinfo.el ends here
