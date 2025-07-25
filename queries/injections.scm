; JavaScript injection in script tags with type attribute
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

; CSS injection in style tags with type attribute  
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
