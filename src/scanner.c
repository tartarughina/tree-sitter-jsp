#include "tag.h"
#include "tree_sitter/alloc.h"
#include "tree_sitter/array.h"
#include "tree_sitter/parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <wctype.h>

enum TokenType {
  JSP_SCRIPTLET,
  JSP_EXPRESSION,
  JSP_DECLARATION,
  JSP_COMMENT,
  JSP_DIRECTIVE_START,
  EL_EXPRESSION,
  TEXT_FRAGMENT,
  INTERPOLATION_TEXT,
  START_TAG_NAME,
  TEMPLATE_START_TAG_NAME,
  SCRIPT_START_TAG_NAME,
  STYLE_START_TAG_NAME,
  END_TAG_NAME,
  ERRONEOUS_END_TAG_NAME,
  SELF_CLOSING_TAG_DELIMITER,
  IMPLICIT_END_TAG,
  RAW_TEXT,
  COMMENT
};

// Tag struct is now defined in tag_a.h

typedef struct {
  Array(Tag) tags;
} Scanner;

// Tag helper functions are now provided by tag_a.h

static TagType get_tag_type_for_name(const char *name) {
  String tag_name = {0};
  size_t len = strlen(name);

  // Convert to uppercase and create String
  array_reserve(&tag_name, len);
  for (size_t i = 0; i < len; i++) {
    array_push(&tag_name, towupper(name[i]));
  }

  TagType result = tag_type_for_name(&tag_name);
  array_delete(&tag_name);
  return result;
}

static Tag create_tag_from_name(const char *name) {
  String tag_name = {0};
  size_t len = strlen(name);

  // Convert to uppercase and create String
  array_reserve(&tag_name, len);
  for (size_t i = 0; i < len; i++) {
    array_push(&tag_name, towupper(name[i]));
  }

  return tag_for_name(tag_name); // Uses tag.h implementation
}

// tag_can_contain is now provided by tag_a.h

// Scanner management functions
static Scanner *scanner_new(void) {
  Scanner *scanner = ts_malloc(sizeof(Scanner));
  array_init(&scanner->tags);
  return scanner;
}

static void scanner_delete(Scanner *scanner) {
  for (uint32_t i = 0; i < scanner->tags.size; i++) {
    tag_free(&scanner->tags.contents[i]);
  }
  array_delete(&scanner->tags);
  ts_free(scanner);
}

static unsigned scanner_serialize(Scanner *scanner, char *buffer) {
  uint16_t tag_count =
      scanner->tags.size > UINT16_MAX ? UINT16_MAX : scanner->tags.size;
  uint16_t serialized_tag_count = 0;

  unsigned i = sizeof(tag_count);
  memcpy(&buffer[i], &tag_count, sizeof(tag_count));
  i += sizeof(tag_count);

  for (; serialized_tag_count < tag_count; serialized_tag_count++) {
    Tag *tag = &scanner->tags.contents[serialized_tag_count];
    if (tag->type == CUSTOM) {
      unsigned name_length = tag->custom_tag_name.size;
      if (name_length > UINT8_MAX)
        name_length = UINT8_MAX;
      if (i + 2 + name_length >= TREE_SITTER_SERIALIZATION_BUFFER_SIZE)
        break;
      buffer[i++] = (char)tag->type;
      buffer[i++] = name_length;
      if (name_length > 0) {
        memcpy(&buffer[i], tag->custom_tag_name.contents, name_length);
        i += name_length;
      }
    } else {
      if (i + 1 >= TREE_SITTER_SERIALIZATION_BUFFER_SIZE)
        break;
      buffer[i++] = (char)tag->type;
    }
  }

  memcpy(&buffer[0], &serialized_tag_count, sizeof(serialized_tag_count));
  return i;
}

static void scanner_deserialize(Scanner *scanner, const char *buffer,
                                unsigned length) {
  for (uint32_t i = 0; i < scanner->tags.size; i++) {
    tag_free(&scanner->tags.contents[i]);
  }
  array_clear(&scanner->tags);

  if (length > 0) {
    unsigned i = 0;
    uint16_t tag_count, serialized_tag_count;

    memcpy(&serialized_tag_count, &buffer[i], sizeof(serialized_tag_count));
    i += sizeof(serialized_tag_count);

    memcpy(&tag_count, &buffer[i], sizeof(tag_count));
    i += sizeof(tag_count);

    array_reserve(&scanner->tags, tag_count);
    array_grow_by(&scanner->tags, tag_count);

    for (unsigned j = 0; j < serialized_tag_count; j++) {
      Tag *tag = &scanner->tags.contents[j];
      *tag = tag_new();
      tag->type = (TagType)buffer[i++];
      if (tag->type == CUSTOM) {
        uint16_t name_length = (uint8_t)buffer[i++];
        if (name_length > 0) {
          array_reserve(&tag->custom_tag_name, name_length);
          for (uint16_t k = 0; k < name_length; k++) {
            array_push(&tag->custom_tag_name, buffer[i++]);
          }
        }
      }
    }
  }
}

// Scanning helper functions
static void scan_tag_name(TSLexer *lexer, char *buffer, size_t buffer_size) {
  size_t i = 0;
  while (i < buffer_size - 1 &&
         (iswalnum(lexer->lookahead) || lexer->lookahead == '-' ||
          lexer->lookahead == ':')) {
    buffer[i++] = towupper(lexer->lookahead);
    lexer->advance(lexer, false);
  }
  buffer[i] = '\0';
}

static bool scan_jsp_directive_start(TSLexer *lexer) {
  // We've already seen <%@, just return the start token
  // The grammar will handle parsing the rest
  lexer->result_symbol = JSP_DIRECTIVE_START;
  lexer->mark_end(lexer);
  return true;
}

// Generic JSP scanner that scans until %> and sets the appropriate token
static bool scan_jsp_until_close(TSLexer *lexer, enum TokenType token_type) {
  while (lexer->lookahead) {
    if (lexer->lookahead == '%') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '>') {
        lexer->advance(lexer, false);
        lexer->result_symbol = token_type;
        lexer->mark_end(lexer);
        return true;
      }
    } else {
      lexer->advance(lexer, false);
    }
  }
  return false;
}

static bool scan_jsp_scriptlet(TSLexer *lexer) {
  return scan_jsp_until_close(lexer, JSP_SCRIPTLET);
}

static bool scan_jsp_expression(TSLexer *lexer) {
  return scan_jsp_until_close(lexer, JSP_EXPRESSION);
}

static bool scan_jsp_declaration(TSLexer *lexer) {
  return scan_jsp_until_close(lexer, JSP_DECLARATION);
}

static bool scan_jsp_comment(TSLexer *lexer) {
  // We've already seen <%-, now check for second - and scan until --%>
  if (lexer->lookahead != '-') {
    return false;
  }
  lexer->advance(lexer, false);

  // Now scan until we find --%>
  while (lexer->lookahead) {
    if (lexer->lookahead == '-') {
      lexer->advance(lexer, false);
      if (lexer->lookahead == '-') {
        lexer->advance(lexer, false);
        if (lexer->lookahead == '%') {
          lexer->advance(lexer, false);
          if (lexer->lookahead == '>') {
            lexer->advance(lexer, false);
            lexer->result_symbol = JSP_COMMENT;
            lexer->mark_end(lexer);
            return true;
          }
          // If we don't find '>', continue scanning
        }
        // If we don't find '%', continue scanning
      }
      // If we don't find second '-', continue scanning
    } else {
      lexer->advance(lexer, false);
    }
  }
  return false;
}

static bool scan_el_expression(TSLexer *lexer) {
  // We've already seen ${, now scan until }
  int brace_count = 1;
  while (lexer->lookahead && brace_count > 0) {
    if (lexer->lookahead == '{') {
      brace_count++;
    } else if (lexer->lookahead == '}') {
      brace_count--;
    }
    lexer->advance(lexer, false);
  }

  if (brace_count == 0) {
    lexer->result_symbol = EL_EXPRESSION;
    lexer->mark_end(lexer);
    return true;
  }
  return false;
}

static bool scan_jsp_construct(TSLexer *lexer) {
  // We've seen <%, now check what follows
  if (lexer->lookahead == '@') {
    lexer->advance(lexer, false);
    return scan_jsp_directive_start(lexer);
  } else if (lexer->lookahead == '=') {
    lexer->advance(lexer, false);
    return scan_jsp_expression(lexer);
  } else if (lexer->lookahead == '!') {
    lexer->advance(lexer, false);
    return scan_jsp_declaration(lexer);
  } else if (lexer->lookahead == '-') {
    lexer->advance(lexer, false);
    return scan_jsp_comment(lexer);
  } else {
    return scan_jsp_scriptlet(lexer);
  }
}

static bool scan_comment(TSLexer *lexer) {
  if (lexer->lookahead != '-')
    return false;
  lexer->advance(lexer, false);
  if (lexer->lookahead != '-')
    return false;
  lexer->advance(lexer, false);

  unsigned dashes = 0;
  while (lexer->lookahead) {
    switch (lexer->lookahead) {
    case '-':
      ++dashes;
      break;
    case '>':
      if (dashes >= 2) {
        lexer->result_symbol = COMMENT;
        lexer->advance(lexer, false);
        lexer->mark_end(lexer);
        return true;
      }
      // fallthrough
    default:
      dashes = 0;
    }
    lexer->advance(lexer, false);
  }
  return false;
}

static bool scan_raw_text(Scanner *scanner, TSLexer *lexer) {
  if (scanner->tags.size == 0)
    return false;

  // Check if we're inside a script or style tag
  Tag *current_tag = &scanner->tags.contents[scanner->tags.size - 1];
  if (current_tag->type != SCRIPT && current_tag->type != STYLE)
    return false;

  lexer->mark_end(lexer);

  const char *end_delimiter =
      current_tag->type == SCRIPT ? "</SCRIPT" : "</STYLE";

  unsigned delimiter_index = 0;
  while (lexer->lookahead) {
    if (towupper(lexer->lookahead) == end_delimiter[delimiter_index]) {
      delimiter_index++;
      if (delimiter_index == strlen(end_delimiter))
        break;
      lexer->advance(lexer, false);
    } else {
      delimiter_index = 0;
      lexer->advance(lexer, false);
      lexer->mark_end(lexer);
    }
  }

  lexer->result_symbol = RAW_TEXT;
  return true;
}

static bool scan_implicit_end_tag(Scanner *scanner, TSLexer *lexer) {
  Tag *parent = scanner->tags.size == 0
                    ? NULL
                    : &scanner->tags.contents[scanner->tags.size - 1];

  bool is_closing_tag = false;
  if (lexer->lookahead == '/') {
    is_closing_tag = true;
    lexer->advance(lexer, false);
  } else {
    if (parent && tag_is_void(parent)) {
      array_pop(&scanner->tags);
      lexer->result_symbol = IMPLICIT_END_TAG;
      return true;
    }
  }

  char tag_name[256];
  scan_tag_name(lexer, tag_name, sizeof(tag_name));
  if (tag_name[0] == '\0')
    return false;

  Tag next_tag = create_tag_from_name(tag_name);

  if (is_closing_tag) {
    // The tag correctly closes the topmost element on the stack
    if (scanner->tags.size > 0 &&
        tag_eq(&scanner->tags.contents[scanner->tags.size - 1], &next_tag)) {
      tag_free(&next_tag);
      return false;
    }

    // Otherwise, dig deeper and queue implicit end tags
    for (uint32_t i = 0; i < scanner->tags.size; i++) {
      if (tag_eq(&scanner->tags.contents[i], &next_tag)) {
        tag_free(&scanner->tags.contents[scanner->tags.size - 1]);
        array_pop(&scanner->tags);
        lexer->result_symbol = IMPLICIT_END_TAG;
        tag_free(&next_tag);
        return true;
      }
    }
  } else if (parent && !tag_can_contain(parent, &next_tag)) {
    tag_free(&scanner->tags.contents[scanner->tags.size - 1]);
    array_pop(&scanner->tags);
    lexer->result_symbol = IMPLICIT_END_TAG;
    tag_free(&next_tag);
    return true;
  }

  tag_free(&next_tag);
  return false;
}

static bool scan_start_tag_name(Scanner *scanner, TSLexer *lexer) {
  char tag_name[256];
  scan_tag_name(lexer, tag_name, sizeof(tag_name));
  if (tag_name[0] == '\0')
    return false;

  Tag tag = create_tag_from_name(tag_name);
  array_push(&scanner->tags, tag);

  switch (tag.type) {
  case TEMPLATE:
    lexer->result_symbol = TEMPLATE_START_TAG_NAME;
    break;
  case SCRIPT:
    lexer->result_symbol = SCRIPT_START_TAG_NAME;
    break;
  case STYLE:
    lexer->result_symbol = STYLE_START_TAG_NAME;
    break;
  default:
    lexer->result_symbol = START_TAG_NAME;
    break;
  }
  return true;
}

static bool scan_end_tag_name(Scanner *scanner, TSLexer *lexer) {
  char tag_name[256];
  scan_tag_name(lexer, tag_name, sizeof(tag_name));
  if (tag_name[0] == '\0')
    return false;

  Tag tag = create_tag_from_name(tag_name);
  if (scanner->tags.size > 0 &&
      tag_eq(&scanner->tags.contents[scanner->tags.size - 1], &tag)) {
    tag_free(&scanner->tags.contents[scanner->tags.size - 1]);
    array_pop(&scanner->tags);
    lexer->result_symbol = END_TAG_NAME;
  } else {
    lexer->result_symbol = ERRONEOUS_END_TAG_NAME;
  }
  tag_free(&tag);
  return true;
}

static bool scan_self_closing_tag_delimiter(Scanner *scanner, TSLexer *lexer) {
  lexer->advance(lexer, false);
  if (lexer->lookahead == '>') {
    lexer->advance(lexer, false);
    if (scanner->tags.size > 0) {
      tag_free(&scanner->tags.contents[scanner->tags.size - 1]);
      array_pop(&scanner->tags);
      lexer->result_symbol = SELF_CLOSING_TAG_DELIMITER;
    }
    return true;
  }
  return false;
}

static bool scanner_scan(Scanner *scanner, TSLexer *lexer,
                         const bool *valid_symbols) {
  while (iswspace(lexer->lookahead)) {
    lexer->advance(lexer, true);
  }

  // Check if we're inside a script or style tag - if so, prioritize raw_text
  bool inside_script_or_style = false;
  if (scanner->tags.size > 0) {
    Tag *current_tag = &scanner->tags.contents[scanner->tags.size - 1];
    inside_script_or_style =
        (current_tag->type == SCRIPT || current_tag->type == STYLE);
  }

  // If we're inside script or style tags and raw_text is valid, scan for
  // raw_text
  if (inside_script_or_style && valid_symbols[RAW_TEXT]) {
    return scan_raw_text(scanner, lexer);
  }

  // Original raw_text condition for other contexts
  if (valid_symbols[RAW_TEXT] && !valid_symbols[START_TAG_NAME] &&
      !valid_symbols[END_TAG_NAME] && !valid_symbols[JSP_DIRECTIVE_START] &&
      !valid_symbols[JSP_SCRIPTLET] && !valid_symbols[JSP_EXPRESSION] &&
      !valid_symbols[JSP_DECLARATION] && !valid_symbols[JSP_COMMENT] &&
      !valid_symbols[EL_EXPRESSION]) {
    return scan_raw_text(scanner, lexer);
  }

  switch (lexer->lookahead) {
  case '<':
    lexer->mark_end(lexer);
    lexer->advance(lexer, false);

    if (lexer->lookahead == '!') {
      lexer->advance(lexer, false);
      return scan_comment(lexer);
    }

    if (lexer->lookahead == '%') {
      lexer->advance(lexer, false);
      return scan_jsp_construct(lexer);
    }

    if (valid_symbols[IMPLICIT_END_TAG]) {
      return scan_implicit_end_tag(scanner, lexer);
    }
    break;

  case '$':
    // Skip EL expression checking if we're inside script or style tags
    if (valid_symbols[EL_EXPRESSION] && !inside_script_or_style) {
      TSLexer saved_lexer = *lexer;
      lexer->advance(lexer, false);
      if (lexer->lookahead == '{') {
        lexer->advance(lexer, false);
        return scan_el_expression(lexer);
      } else {
        // Not an EL expression, restore position
        *lexer = saved_lexer;
      }
    }
    break;

  case '\0':
    if (valid_symbols[IMPLICIT_END_TAG]) {
      return scan_implicit_end_tag(scanner, lexer);
    }
    break;

  case '/':
    if (valid_symbols[SELF_CLOSING_TAG_DELIMITER]) {
      return scan_self_closing_tag_delimiter(scanner, lexer);
    }
    break;

  default:
    if ((valid_symbols[START_TAG_NAME] || valid_symbols[END_TAG_NAME]) &&
        !valid_symbols[RAW_TEXT]) {
      return valid_symbols[START_TAG_NAME] ? scan_start_tag_name(scanner, lexer)
                                           : scan_end_tag_name(scanner, lexer);
    }
  }

  return false;
}

// External scanner interface
void *tree_sitter_jsp_external_scanner_create(void) { return scanner_new(); }

void tree_sitter_jsp_external_scanner_destroy(void *payload) {
  Scanner *scanner = (Scanner *)payload;
  scanner_delete(scanner);
}

unsigned tree_sitter_jsp_external_scanner_serialize(void *payload,
                                                    char *buffer) {
  Scanner *scanner = (Scanner *)payload;
  return scanner_serialize(scanner, buffer);
}

void tree_sitter_jsp_external_scanner_deserialize(void *payload,
                                                  const char *buffer,
                                                  unsigned length) {
  Scanner *scanner = (Scanner *)payload;
  scanner_deserialize(scanner, buffer, length);
}

bool tree_sitter_jsp_external_scanner_scan(void *payload, TSLexer *lexer,
                                           const bool *valid_symbols) {
  Scanner *scanner = (Scanner *)payload;

  // Check if we're inside a script or style tag
  bool inside_script_or_style = false;
  if (scanner->tags.size > 0) {
    Tag *current_tag = &scanner->tags.contents[scanner->tags.size - 1];
    inside_script_or_style =
        (current_tag->type == SCRIPT || current_tag->type == STYLE);
  }

  bool is_error_recovery =
      valid_symbols[START_TAG_NAME] && valid_symbols[RAW_TEXT];
  if (!is_error_recovery) {
    // Check for EL expressions first, before text fragment scanning
    if (lexer->lookahead == '$' && valid_symbols[EL_EXPRESSION] &&
        !inside_script_or_style) {
      return scanner_scan(scanner, lexer, valid_symbols);
    }

    if (lexer->lookahead != '<' &&
        (valid_symbols[TEXT_FRAGMENT] || valid_symbols[INTERPOLATION_TEXT])) {
      bool has_text = false;
      for (;; has_text = true) {
        if (lexer->lookahead == 0) {
          lexer->mark_end(lexer);
          break;
        } else if (lexer->lookahead == '<') {
          lexer->mark_end(lexer);
          break;
        } else if (lexer->lookahead == '$' && !inside_script_or_style) {
          // Only check for EL expressions if we're NOT inside script or style
          // tags AND if EL_EXPRESSION is a valid symbol
          if (valid_symbols[EL_EXPRESSION]) {
            lexer->mark_end(
                lexer); // Mark end BEFORE checking for EL expression
            lexer->advance(lexer, false);
            if (lexer->lookahead == '{') {
              // This is an EL expression, break to return the text fragment
              break;
            } else {
              // This is just a $ character, continue scanning
              // Note: lexer is already advanced, so continue from current
              // position
            }
          } else {
            // EL_EXPRESSION is not valid, treat $ as regular text
            lexer->advance(lexer, false);
          }
        } else if (lexer->lookahead == '{') {
          lexer->mark_end(lexer);
          lexer->advance(lexer, false);
          if (lexer->lookahead == '{')
            break;
        } else if (lexer->lookahead == '}' &&
                   valid_symbols[INTERPOLATION_TEXT]) {
          lexer->mark_end(lexer);
          lexer->advance(lexer, false);
          if (lexer->lookahead == '}') {
            lexer->result_symbol = INTERPOLATION_TEXT;
            return has_text;
          }
        } else {
          lexer->advance(lexer, false);
        }
      }
      if (has_text) {
        lexer->result_symbol = TEXT_FRAGMENT;
        return true;
      }
    }
  }

  return scanner_scan(scanner, lexer, valid_symbols);
}
