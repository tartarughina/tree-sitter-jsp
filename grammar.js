module.exports = grammar({
  name: "jsp",

  externals: $ => [
    $.jsp_scriptlet,
    $.jsp_expression,
    $._jsp_declaration,
    $._jsp_comment,
    $._jsp_directive_start,
    $._el_expression,
    $._text_fragment,
    $._interpolation_text,
    $._start_tag_name,
    $._template_start_tag_name,
    $._script_start_tag_name,
    $._style_start_tag_name,
    $._end_tag_name,
    $.erroneous_end_tag_name,
    "/>",
    $._implicit_end_tag,
    $.raw_text,
    $.comment,
  ],

  extras: $ => [
    /\s+/,
    $.jsp_scriptlet,
    $.jsp_expression,
    $.jsp_declaration,
    $.jsp_comment
  ],

  rules: {
    component: $ => repeat(
      choice(
        $.comment,
        $.jsp_directive,
        $.jsp_scriptlet,
        $.jsp_expression,
        $.jsp_declaration,
        $.jsp_comment,
        $.el_expression,
        $.element,
        $.template_element,
        $.script_element,
        $.style_element,
        $.doc_type,
      ),
    ),

    _node: $ => choice(
      $.comment,
      $.jsp_directive,
      $.jsp_scriptlet,
      $.jsp_expression,
      $.jsp_declaration,
      $.jsp_comment,
      $.el_expression,
      $.text,
      $.interpolation,
      $.element,
      $.template_element,
      $.script_element,
      $.style_element,
      $.erroneous_end_tag,
    ),

    doc_type: $ => seq('<', '!', $.text),

    element: $ => choice(
      seq(
        $.start_tag,
        repeat($._node),
        choice($.end_tag, $._implicit_end_tag),
      ),
      $.self_closing_tag,
    ),

    template_element: $ => seq(
      alias($.template_start_tag, $.start_tag),
      repeat($._node),
      $.end_tag,
    ),

    script_element: $ => seq(
      alias($.script_start_tag, $.start_tag),
      optional($.raw_text),
      $.end_tag,
    ),

    style_element: $ => seq(
      alias($.style_start_tag, $.start_tag),
      optional($.raw_text),
      $.end_tag,
    ),

    start_tag: $ => seq(
      "<",
      alias($._start_tag_name, $.tag_name),
      repeat(choice($.attribute, $.directive_attribute)),
      ">",
    ),

    template_start_tag: $ => seq(
      "<",
      alias($._template_start_tag_name, $.tag_name),
      repeat(choice($.attribute, $.directive_attribute)),
      ">",
    ),

    script_start_tag: $ => seq(
      "<",
      alias($._script_start_tag_name, $.tag_name),
      repeat(choice($.attribute, $.directive_attribute)),
      ">",
    ),

    style_start_tag: $ => seq(
      "<",
      alias($._style_start_tag_name, $.tag_name),
      repeat(choice($.attribute, $.directive_attribute)),
      ">",
    ),

    self_closing_tag: $ => seq(
      "<",
      alias($._start_tag_name, $.tag_name),
      repeat(choice($.attribute, $.directive_attribute)),
      "/>",
    ),

    end_tag: $ => seq(
      "</",
      alias($._end_tag_name, $.tag_name),
      ">",
    ),

    erroneous_end_tag: $ => seq(
      "</",
      $.erroneous_end_tag_name,
      ">",
    ),

    attribute: $ => seq(
      $.attribute_name,
      optional(seq(
        "=",
        choice(
          $.attribute_value,
          $.quoted_attribute_value,
          $.el_expression
        ),
      )),
    ),

    attribute_name: _ => /[^<>"'=/\s]+/,

    attribute_value: _ => /[^<>"'=\s]+/,

    quoted_attribute_value: $ =>
      choice(
        seq("'", optional($._attribute_content_single), "'"),
        seq('"', optional($._attribute_content_double), '"'),
      ),

    // Attribute content can contain mixed text and EL expressions
    _attribute_content_single: $ => repeat1(
      choice(
        alias(/[^'$]+/, $.attribute_value),
        $.el_expression,
      )
    ),

    _attribute_content_double: $ => repeat1(
      choice(
        alias(/[^"$]+/, $.attribute_value),
        $.el_expression,
      )
    ),

    text: $ => choice($._text_fragment, "{{"),

    interpolation: $ => seq(
      "{{",
      optional(alias($._interpolation_text, $.raw_text)),
      "}}",
    ),

    // JSP directive with simplified grammar reusing existing attribute rules
    jsp_directive: $ => seq(
      $._jsp_directive_start,
      $.jsp_directive_name,
      repeat($.attribute),
      '%>'
    ),

    jsp_directive_name: _ => choice(
      'page',
      'taglib',
      'include'
    ),

    // JSP declaration for declaring variables and methods
    jsp_declaration: $ => $._jsp_declaration,

    // JSP comment (different from HTML comment)
    jsp_comment: $ => $._jsp_comment,

    // Expression Language for accessing data and functions
    el_expression: $ => $._el_expression,

    directive_attribute: $ =>
      seq(
        choice(
          seq(
            $.directive_name,
            optional(seq(
              token.immediate(prec(1, ":")),
              choice($.directive_argument, $.directive_dynamic_argument),
            )),
          ),
          seq(
            alias($.directive_shorthand, $.directive_name),
            choice($.directive_argument, $.directive_dynamic_argument),
          ),
        ),
        optional($.directive_modifiers),
        optional(seq("=", choice($.attribute_value, $.quoted_attribute_value))),
      ),
    directive_name: $ => token(prec(1, /v-[^<>'"=/\s:.]+/)),
    directive_shorthand: $ => token(prec(1, choice(":", "@", "#"))),
    directive_argument: $ => token.immediate(/[^<>"'/=\s.]+/),
    directive_dynamic_argument: $ => seq(
      token.immediate(prec(1, "[")),
      optional($.directive_dynamic_argument_value),
      token.immediate("]"),
    ),
    directive_dynamic_argument_value: $ => token.immediate(/[^<>"'/=\s\]]+/),
    directive_modifiers: $ => repeat1(seq(token.immediate(prec(1, ".")), $.directive_modifier)),
    directive_modifier: $ => token.immediate(/[^<>"'/=\s.]+/),
  },
});
