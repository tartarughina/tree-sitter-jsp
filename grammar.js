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
      repeat($.attribute),
      ">",
    ),

    template_start_tag: $ => seq(
      "<",
      alias($._template_start_tag_name, $.tag_name),
      repeat($.attribute),
      ">",
    ),

    script_start_tag: $ => seq(
      "<",
      alias($._script_start_tag_name, $.tag_name),
      repeat($.attribute),
      ">",
    ),

    style_start_tag: $ => seq(
      "<",
      alias($._style_start_tag_name, $.tag_name),
      repeat($.attribute),
      ">",
    ),

    self_closing_tag: $ => seq(
      "<",
      alias($._start_tag_name, $.tag_name),
      repeat($.attribute),
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

    attribute_value: _ => /[^<>"'=\s$]+/,

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

    jsp_directive_name: _ => /page|taglib|include/,

    // JSP declaration for declaring variables and methods
    jsp_declaration: $ => $._jsp_declaration,

    // JSP comment (different from HTML comment)
    jsp_comment: $ => $._jsp_comment,

    // Expression Language for accessing data and functions
    el_expression: $ => $._el_expression,
  },
});
