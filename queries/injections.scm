; JavaScript injection in script tags with explicit type attribute
((script_element
  (start_tag
    (attribute
      (attribute_name) @_attr
      (quoted_attribute_value
        (attribute_value) @injection.language)))
  (raw_text) @injection.content)
 (#eq? @_attr "type")
 (#match? @injection.language "(text/)?javascript"))

; JavaScript injection in script tags (default - no type attribute needed)
((script_element
  (raw_text) @injection.content)
 (#set! injection.language "javascript"))

; CSS injection in style tags with explicit type attribute  
((style_element
  (start_tag
    (attribute
      (attribute_name) @_attr
      (quoted_attribute_value
        (attribute_value) @injection.language)))
  (raw_text) @injection.content)
 (#eq? @_attr "type")
 (#match? @injection.language "(text/)?css"))

; CSS injection in style tags (default - no type attribute needed)
((style_element
  (raw_text) @injection.content)
 (#set! injection.language "css"))

; Java injection in JSP scriptlets - these contain Java code blocks
(jsp_scriptlet) @injection.content
(#set! injection.language "java")

; Java injection in JSP expressions - these contain Java expressions
(jsp_expression) @injection.content
(#set! injection.language "java")

; Java injection in JSP declarations - these contain Java declarations (fields, methods)
(jsp_declaration) @injection.content
(#set! injection.language "java")

; Expression Language (EL) injection
; EL is a simple expression language, we'll inject as Java for basic syntax highlighting
; Individual editors can override this with EL-specific parsers if available
(el_expression) @injection.content
(#set! injection.language "java")
(#set! injection.include-children)
