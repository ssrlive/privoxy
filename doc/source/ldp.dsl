<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY % html "IGNORE">
<![%html;[
<!ENTITY % print "IGNORE">
<!ENTITY docbook.dsl SYSTEM "/usr/share/sgml/docbook/dsssl-stylesheets/html/docbook.dsl" CDATA dsssl>
]]>
<!ENTITY % print "INCLUDE">
<![%print;[
<!ENTITY docbook.dsl SYSTEM "/usr/share/sgml/docbook/dsssl-stylesheets/print/docbook.dsl" CDATA dsssl>
]]>
]>

<!--

;; borrowed from the LDP stylesheet, with modifications, HB.
;; Added support for css 03/20/02, and other mods.

-->

<style-sheet>

<style-specification id="print" use="docbook">
<style-specification-body> 

;; ==============================
;; customize the print stylesheet
;; ==============================

</style-specification-body>
</style-specification>


<!--
;; ===================================================
;; customize the html stylesheet; borrowed from Cygnus
;; at http://sourceware.cygnus.com/ (cygnus-both.dsl)
;; ===================================================
-->

<style-specification id="html" use="docbook">
<style-specification-body> 

;; this is necessary because right now jadetex does not understand
;; symbolic entities, whereas things work well with numeric entities.
(declare-characteristic preserve-sdata?
  "UNREGISTERED::James Clark//Characteristic::preserve-sdata?"
  #f)

;; put the legal notice in a separate file
(define %generate-legalnotice-link%
  #t)

;; use graphics in admonitions, and have their path be "stylesheet-images"
;; NO: they do not yet look very good
(define %admon-graphics-path%
  "./stylesheet-images/")

(define %admon-graphics%
  #f)

(define %funcsynopsis-decoration%
  ;; make funcsynopsis look pretty
  #t)

(define %html-ext%
  ".html")

(define %generate-article-toc% 
  ;; Should a Table of Contents be produced for Articles?
  ;; If true, a Table of Contents will be generated for each 'Article'.
  #t)

(define %root-filename%
  ;; The filename of the root HTML document (e.g, "index").
  "index")

(define %generate-part-toc%
  #t)

(define %shade-verbatim%
  #t)

(define %use-id-as-filename%
  ;; Use ID attributes as name for component HTML files?
  #t)

(define %graphic-default-extension% 
  "gif")

(define %section-autolabel% #t)
  ;; For enumerated sections (1.1, 1.1.1, 1.2, etc.)
  
;; HB changed TOC depth to 3 levels.
(define (toc-depth nd)
  3)

;; HB added 03/20/02, see dbparam.dsl ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define %body-attr% 
  ;; REFENTRY body-attr
  ;; PURP What attributes should be hung off of BODY?
  ;; DESC
  ;; A list of the the BODY attributes that should be generated.
  ;; The format is a list of lists, each interior list contains the
  ;; name and value of a BODY attribute.
  ;; /DESC
  ;; AUTHOR N/A
  ;; /REFENTRY
  (list
   (list "BGCOLOR" "#EEEEEE")
   (list "TEXT" "#000000")
   (list "LINK" "#0000FF")
   (list "VLINK" "#840084")
   (list "ALINK" "#0000FF")))

(define %stylesheet%
  ;; REFENTRY stylesheet
  ;; PURP Name of the stylesheet to use
  ;; DESC
  ;; The name of the stylesheet to place in the HTML LINK TAG, or '#f' to
  ;; suppress the stylesheet LINK.
  ;; /DESC
  ;; AUTHOR N/A
  ;; /REFENTRY
  "../p_doc.css")

(define %stylesheet-type%
  ;; REFENTRY stylesheet-type
  ;; PURP The type of the stylesheet to use
  ;; DESC
  ;; The type of the stylesheet to place in the HTML LINK TAG.
  ;; /DESC
  ;; AUTHOR N/A
  ;; /REFENTRY
  "text/css")

(define %css-liststyle-alist%
  ;; REFENTRY css-liststyle-alist
  ;; PURP Map DocBook OVERRIDE and MARK attributes to CSS
  ;; DESC
  ;; If '%css-decoration%' is turned on then the list-style-type property of
  ;; list items will be set to reflect the list item style selected in the
  ;; DocBook instance.  This associative list maps the style type names used
  ;; in your instance to the appropriate CSS names.  If no mapping exists,
  ;; the name from the instance will be used.
  ;; /DESC
  ;; AUTHOR N/A
  ;; /REFENTRY
  '(("bullet" "disc")
    ("box" "square")))

(define %css-decoration%
  ;; REFENTRY css-decoration
  ;; PURP Enable CSS decoration of elements
  ;; DESC
  ;; If '%css-decoration%' is turned on then HTML elements produced by the
  ;; stylesheet may be decorated with STYLE attributes.  For example, the
  ;; LI tags produced for list items may include a fragment of CSS in the
  ;; STYLE attribute which sets the CSS property "list-style-type".
  ;; /DESC
  ;; AUTHOR N/A
  ;; /REFENTRY
  #t)


</style-specification-body>
</style-specification>

<external-specification id="docbook" document="docbook.dsl">

</style-sheet>
