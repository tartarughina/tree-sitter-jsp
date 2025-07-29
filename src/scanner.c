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

typedef enum {
  AREA,
  BASE,
  BASEFONT,
  BGSOUND,
  BR,
  COL,
  COMMAND,
  EMBED,
  FRAME,
  HR,
  IMAGE,
  IMG,
  INPUT,
  ISINDEX,
  KEYGEN,
  LINK,
  MENUITEM,
  META,
  NEXTID,
  PARAM,
  SOURCE,
  TRACK,
  WBR,
  END_OF_VOID_TAGS,
  A,
  ABBR,
  ADDRESS,
  ARTICLE,
  ASIDE,
  AUDIO,
  B,
  BDI,
  BDO,
  BLOCKQUOTE,
  BODY,
  BUTTON,
  CANVAS,
  CAPTION,
  CITE,
  CODE,
  COLGROUP,
  DATA,
  DATALIST,
  DD,
  DEL,
  DETAILS,
  DFN,
  DIALOG,
  DIV,
  DL,
  DT,
  EM,
  FIELDSET,
  FIGCAPTION,
  FIGURE,
  FOOTER,
  FORM,
  H1,
  H2,
  H3,
  H4,
  H5,
  H6,
  HEAD,
  HEADER,
  HGROUP,
  HTML,
  I,
  IFRAME,
  INS,
  KBD,
  LABEL,
  LEGEND,
  LI,
  MAIN,
  MAP,
  MARK,
  MATH,
  MENU,
  METER,
  NAV,
  NOSCRIPT,
  OBJECT,
  OL,
  OPTGROUP,
  OPTION,
  OUTPUT,
  P,
  PICTURE,
  PRE,
  PROGRESS,
  Q,
  RB,
  RP,
  RT,
  RTC,
  RUBY,
  S,
  SAMP,
  SCRIPT,
  SECTION,
  SELECT,
  SLOT,
  SMALL,
  SPAN,
  STRONG,
  STYLE,
  SUB,
  SUMMARY,
  SUP,
  SVG,
  TABLE,
  TBODY,
  TD,
  TEMPLATE,
  TEXTAREA,
  TFOOT,
  TH,
  THEAD,
  TIME,
  TITLE,
  TR,
  U,
  UL,
  VAR,
  VIDEO,
  CUSTOM,
} TagType;

typedef struct {
  TagType type;
  char *custom_tag_name;
  size_t custom_tag_name_capacity;
} Tag;

typedef struct {
  Array(Tag) tags;
} Scanner;

// Tag helper functions
static void tag_init(Tag *tag) {
  tag->type = CUSTOM;
  tag->custom_tag_name = NULL;
  tag->custom_tag_name_capacity = 0;
}

static void tag_clear(Tag *tag) {
  if (tag->custom_tag_name) {
    ts_free(tag->custom_tag_name);
    tag->custom_tag_name = NULL;
    tag->custom_tag_name_capacity = 0;
  }
}

static void tag_set_name(Tag *tag, const char *name, size_t length) {
  tag_clear(tag);
  if (length > 0) {
    tag->custom_tag_name = ts_malloc(length + 1);
    memcpy(tag->custom_tag_name, name, length);
    tag->custom_tag_name[length] = '\0';
    tag->custom_tag_name_capacity = length + 1;
  }
}

static bool tag_eq(const Tag *a, const Tag *b) {
  if (a->type != b->type)
    return false;
  if (a->type == CUSTOM) {
    if (!a->custom_tag_name || !b->custom_tag_name)
      return false;
    return strcmp(a->custom_tag_name, b->custom_tag_name) == 0;
  }
  return true;
}

static bool tag_is_void(const Tag *tag) { return tag->type < END_OF_VOID_TAGS; }

static TagType get_tag_type_for_name(const char *name) {
  // Convert to uppercase for comparison
  size_t len = strlen(name);
  char *upper_name = ts_malloc(len + 1);
  for (size_t i = 0; i < len; i++) {
    upper_name[i] = towupper(name[i]);
  }
  upper_name[len] = '\0';

  TagType result = CUSTOM;

  // Check known tag types - void tags first
  if (strcmp(upper_name, "AREA") == 0)
    result = AREA;
  else if (strcmp(upper_name, "BASE") == 0)
    result = BASE;
  else if (strcmp(upper_name, "BASEFONT") == 0)
    result = BASEFONT;
  else if (strcmp(upper_name, "BGSOUND") == 0)
    result = BGSOUND;
  else if (strcmp(upper_name, "BR") == 0)
    result = BR;
  else if (strcmp(upper_name, "COL") == 0)
    result = COL;
  else if (strcmp(upper_name, "COMMAND") == 0)
    result = COMMAND;
  else if (strcmp(upper_name, "EMBED") == 0)
    result = EMBED;
  else if (strcmp(upper_name, "FRAME") == 0)
    result = FRAME;
  else if (strcmp(upper_name, "HR") == 0)
    result = HR;
  else if (strcmp(upper_name, "IMAGE") == 0)
    result = IMAGE;
  else if (strcmp(upper_name, "IMG") == 0)
    result = IMG;
  else if (strcmp(upper_name, "INPUT") == 0)
    result = INPUT;
  else if (strcmp(upper_name, "ISINDEX") == 0)
    result = ISINDEX;
  else if (strcmp(upper_name, "KEYGEN") == 0)
    result = KEYGEN;
  else if (strcmp(upper_name, "LINK") == 0)
    result = LINK;
  else if (strcmp(upper_name, "MENUITEM") == 0)
    result = MENUITEM;
  else if (strcmp(upper_name, "META") == 0)
    result = META;
  else if (strcmp(upper_name, "NEXTID") == 0)
    result = NEXTID;
  else if (strcmp(upper_name, "PARAM") == 0)
    result = PARAM;
  else if (strcmp(upper_name, "SOURCE") == 0)
    result = SOURCE;
  else if (strcmp(upper_name, "TRACK") == 0)
    result = TRACK;
  else if (strcmp(upper_name, "WBR") == 0)
    result = WBR;
  // Non-void tags
  else if (strcmp(upper_name, "A") == 0)
    result = A;
  else if (strcmp(upper_name, "ABBR") == 0)
    result = ABBR;
  else if (strcmp(upper_name, "ADDRESS") == 0)
    result = ADDRESS;
  else if (strcmp(upper_name, "ARTICLE") == 0)
    result = ARTICLE;
  else if (strcmp(upper_name, "ASIDE") == 0)
    result = ASIDE;
  else if (strcmp(upper_name, "AUDIO") == 0)
    result = AUDIO;
  else if (strcmp(upper_name, "B") == 0)
    result = B;
  else if (strcmp(upper_name, "BDI") == 0)
    result = BDI;
  else if (strcmp(upper_name, "BDO") == 0)
    result = BDO;
  else if (strcmp(upper_name, "BLOCKQUOTE") == 0)
    result = BLOCKQUOTE;
  else if (strcmp(upper_name, "BODY") == 0)
    result = BODY;
  else if (strcmp(upper_name, "BUTTON") == 0)
    result = BUTTON;
  else if (strcmp(upper_name, "CANVAS") == 0)
    result = CANVAS;
  else if (strcmp(upper_name, "CAPTION") == 0)
    result = CAPTION;
  else if (strcmp(upper_name, "CITE") == 0)
    result = CITE;
  else if (strcmp(upper_name, "CODE") == 0)
    result = CODE;
  else if (strcmp(upper_name, "COLGROUP") == 0)
    result = COLGROUP;
  else if (strcmp(upper_name, "DATA") == 0)
    result = DATA;
  else if (strcmp(upper_name, "DATALIST") == 0)
    result = DATALIST;
  else if (strcmp(upper_name, "DD") == 0)
    result = DD;
  else if (strcmp(upper_name, "DEL") == 0)
    result = DEL;
  else if (strcmp(upper_name, "DETAILS") == 0)
    result = DETAILS;
  else if (strcmp(upper_name, "DFN") == 0)
    result = DFN;
  else if (strcmp(upper_name, "DIALOG") == 0)
    result = DIALOG;
  else if (strcmp(upper_name, "DIV") == 0)
    result = DIV;
  else if (strcmp(upper_name, "DL") == 0)
    result = DL;
  else if (strcmp(upper_name, "DT") == 0)
    result = DT;
  else if (strcmp(upper_name, "EM") == 0)
    result = EM;
  else if (strcmp(upper_name, "FIELDSET") == 0)
    result = FIELDSET;
  else if (strcmp(upper_name, "FIGCAPTION") == 0)
    result = FIGCAPTION;
  else if (strcmp(upper_name, "FIGURE") == 0)
    result = FIGURE;
  else if (strcmp(upper_name, "FOOTER") == 0)
    result = FOOTER;
  else if (strcmp(upper_name, "FORM") == 0)
    result = FORM;
  else if (strcmp(upper_name, "H1") == 0)
    result = H1;
  else if (strcmp(upper_name, "H2") == 0)
    result = H2;
  else if (strcmp(upper_name, "H3") == 0)
    result = H3;
  else if (strcmp(upper_name, "H4") == 0)
    result = H4;
  else if (strcmp(upper_name, "H5") == 0)
    result = H5;
  else if (strcmp(upper_name, "H6") == 0)
    result = H6;
  else if (strcmp(upper_name, "HEAD") == 0)
    result = HEAD;
  else if (strcmp(upper_name, "HEADER") == 0)
    result = HEADER;
  else if (strcmp(upper_name, "HGROUP") == 0)
    result = HGROUP;
  else if (strcmp(upper_name, "HTML") == 0)
    result = HTML;
  else if (strcmp(upper_name, "I") == 0)
    result = I;
  else if (strcmp(upper_name, "IFRAME") == 0)
    result = IFRAME;
  else if (strcmp(upper_name, "INS") == 0)
    result = INS;
  else if (strcmp(upper_name, "KBD") == 0)
    result = KBD;
  else if (strcmp(upper_name, "LABEL") == 0)
    result = LABEL;
  else if (strcmp(upper_name, "LEGEND") == 0)
    result = LEGEND;
  else if (strcmp(upper_name, "LI") == 0)
    result = LI;
  else if (strcmp(upper_name, "MAIN") == 0)
    result = MAIN;
  else if (strcmp(upper_name, "MAP") == 0)
    result = MAP;
  else if (strcmp(upper_name, "MARK") == 0)
    result = MARK;
  else if (strcmp(upper_name, "MATH") == 0)
    result = MATH;
  else if (strcmp(upper_name, "MENU") == 0)
    result = MENU;
  else if (strcmp(upper_name, "METER") == 0)
    result = METER;
  else if (strcmp(upper_name, "NAV") == 0)
    result = NAV;
  else if (strcmp(upper_name, "NOSCRIPT") == 0)
    result = NOSCRIPT;
  else if (strcmp(upper_name, "OBJECT") == 0)
    result = OBJECT;
  else if (strcmp(upper_name, "OL") == 0)
    result = OL;
  else if (strcmp(upper_name, "OPTGROUP") == 0)
    result = OPTGROUP;
  else if (strcmp(upper_name, "OPTION") == 0)
    result = OPTION;
  else if (strcmp(upper_name, "OUTPUT") == 0)
    result = OUTPUT;
  else if (strcmp(upper_name, "P") == 0)
    result = P;
  else if (strcmp(upper_name, "PICTURE") == 0)
    result = PICTURE;
  else if (strcmp(upper_name, "PRE") == 0)
    result = PRE;
  else if (strcmp(upper_name, "PROGRESS") == 0)
    result = PROGRESS;
  else if (strcmp(upper_name, "Q") == 0)
    result = Q;
  else if (strcmp(upper_name, "RB") == 0)
    result = RB;
  else if (strcmp(upper_name, "RP") == 0)
    result = RP;
  else if (strcmp(upper_name, "RT") == 0)
    result = RT;
  else if (strcmp(upper_name, "RTC") == 0)
    result = RTC;
  else if (strcmp(upper_name, "RUBY") == 0)
    result = RUBY;
  else if (strcmp(upper_name, "S") == 0)
    result = S;
  else if (strcmp(upper_name, "SAMP") == 0)
    result = SAMP;
  else if (strcmp(upper_name, "SCRIPT") == 0)
    result = SCRIPT;
  else if (strcmp(upper_name, "SECTION") == 0)
    result = SECTION;
  else if (strcmp(upper_name, "SELECT") == 0)
    result = SELECT;
  else if (strcmp(upper_name, "SLOT") == 0)
    result = SLOT;
  else if (strcmp(upper_name, "SMALL") == 0)
    result = SMALL;
  else if (strcmp(upper_name, "SPAN") == 0)
    result = SPAN;
  else if (strcmp(upper_name, "STRONG") == 0)
    result = STRONG;
  else if (strcmp(upper_name, "STYLE") == 0)
    result = STYLE;
  else if (strcmp(upper_name, "SUB") == 0)
    result = SUB;
  else if (strcmp(upper_name, "SUMMARY") == 0)
    result = SUMMARY;
  else if (strcmp(upper_name, "SUP") == 0)
    result = SUP;
  else if (strcmp(upper_name, "SVG") == 0)
    result = SVG;
  else if (strcmp(upper_name, "TABLE") == 0)
    result = TABLE;
  else if (strcmp(upper_name, "TBODY") == 0)
    result = TBODY;
  else if (strcmp(upper_name, "TD") == 0)
    result = TD;
  else if (strcmp(upper_name, "TEMPLATE") == 0)
    result = TEMPLATE;
  else if (strcmp(upper_name, "TEXTAREA") == 0)
    result = TEXTAREA;
  else if (strcmp(upper_name, "TFOOT") == 0)
    result = TFOOT;
  else if (strcmp(upper_name, "TH") == 0)
    result = TH;
  else if (strcmp(upper_name, "THEAD") == 0)
    result = THEAD;
  else if (strcmp(upper_name, "TIME") == 0)
    result = TIME;
  else if (strcmp(upper_name, "TITLE") == 0)
    result = TITLE;
  else if (strcmp(upper_name, "TR") == 0)
    result = TR;
  else if (strcmp(upper_name, "U") == 0)
    result = U;
  else if (strcmp(upper_name, "UL") == 0)
    result = UL;
  else if (strcmp(upper_name, "VAR") == 0)
    result = VAR;
  else if (strcmp(upper_name, "VIDEO") == 0)
    result = VIDEO;

  ts_free(upper_name);
  return result;
}

static Tag tag_for_name(const char *name) {
  Tag tag;
  tag_init(&tag);

  TagType type = get_tag_type_for_name(name);
  tag.type = type;

  if (type == CUSTOM) {
    tag_set_name(&tag, name, strlen(name));
  }

  return tag;
}

static bool tag_can_contain(const Tag *parent, const Tag *child) {
  TagType child_type = child->type;

  switch (parent->type) {
  case LI:
    return child_type != LI;

  case DT:
  case DD:
    return child_type != DT && child_type != DD;

  case P: {
    // Tags not allowed in paragraphs
    TagType not_allowed[] = {
        ADDRESS,  ARTICLE,    ASIDE,  BLOCKQUOTE, DETAILS, DIV, DL,
        FIELDSET, FIGCAPTION, FIGURE, FOOTER,     FORM,    H1,  H2,
        H3,       H4,         H5,     H6,         HEADER,  HR,  MAIN,
        NAV,      OL,         P,      PRE,        SECTION};
    size_t count = sizeof(not_allowed) / sizeof(TagType);
    for (size_t i = 0; i < count; i++) {
      if (child_type == not_allowed[i]) {
        return false;
      }
    }
    return true;
  }

  case COLGROUP:
    return child_type == COL;

  case RB:
  case RT:
  case RP:
    return child_type != RB && child_type != RT && child_type != RP;

  case OPTGROUP:
    return child_type != OPTGROUP;

  case TR:
    return child_type != TR;

  case TD:
  case TH:
    return child_type != TD && child_type != TH && child_type != TR;

  default:
    return true;
  }
}

// Scanner management functions
static Scanner *scanner_new(void) {
  Scanner *scanner = ts_malloc(sizeof(Scanner));
  array_init(&scanner->tags);
  return scanner;
}

static void scanner_delete(Scanner *scanner) {
  for (uint32_t i = 0; i < scanner->tags.size; i++) {
    tag_clear(&scanner->tags.contents[i]);
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
      unsigned name_length =
          tag->custom_tag_name ? strlen(tag->custom_tag_name) : 0;
      if (name_length > UINT8_MAX)
        name_length = UINT8_MAX;
      if (i + 2 + name_length >= TREE_SITTER_SERIALIZATION_BUFFER_SIZE)
        break;
      buffer[i++] = (char)tag->type;
      buffer[i++] = name_length;
      if (name_length > 0) {
        memcpy(&buffer[i], tag->custom_tag_name, name_length);
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
    tag_clear(&scanner->tags.contents[i]);
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
      tag_init(tag);
      tag->type = (TagType)buffer[i++];
      if (tag->type == CUSTOM) {
        uint16_t name_length = (uint8_t)buffer[i++];
        if (name_length > 0) {
          tag_set_name(tag, &buffer[i], name_length);
          i += name_length;
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

  const char *end_delimiter = current_tag->type == SCRIPT ? "</SCRIPT" : "</STYLE";

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

  Tag next_tag = tag_for_name(tag_name);

  if (is_closing_tag) {
    // The tag correctly closes the topmost element on the stack
    if (scanner->tags.size > 0 &&
        tag_eq(&scanner->tags.contents[scanner->tags.size - 1], &next_tag)) {
      tag_clear(&next_tag);
      return false;
    }

    // Otherwise, dig deeper and queue implicit end tags
    for (uint32_t i = 0; i < scanner->tags.size; i++) {
      if (tag_eq(&scanner->tags.contents[i], &next_tag)) {
        tag_clear(&scanner->tags.contents[scanner->tags.size - 1]);
        array_pop(&scanner->tags);
        lexer->result_symbol = IMPLICIT_END_TAG;
        tag_clear(&next_tag);
        return true;
      }
    }
  } else if (parent && !tag_can_contain(parent, &next_tag)) {
    tag_clear(&scanner->tags.contents[scanner->tags.size - 1]);
    array_pop(&scanner->tags);
    lexer->result_symbol = IMPLICIT_END_TAG;
    tag_clear(&next_tag);
    return true;
  }

  tag_clear(&next_tag);
  return false;
}

static bool scan_start_tag_name(Scanner *scanner, TSLexer *lexer) {
  char tag_name[256];
  scan_tag_name(lexer, tag_name, sizeof(tag_name));
  if (tag_name[0] == '\0')
    return false;

  Tag tag = tag_for_name(tag_name);
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

  Tag tag = tag_for_name(tag_name);
  if (scanner->tags.size > 0 &&
      tag_eq(&scanner->tags.contents[scanner->tags.size - 1], &tag)) {
    tag_clear(&scanner->tags.contents[scanner->tags.size - 1]);
    array_pop(&scanner->tags);
    lexer->result_symbol = END_TAG_NAME;
  } else {
    lexer->result_symbol = ERRONEOUS_END_TAG_NAME;
  }
  tag_clear(&tag);
  return true;
}

static bool scan_self_closing_tag_delimiter(Scanner *scanner, TSLexer *lexer) {
  lexer->advance(lexer, false);
  if (lexer->lookahead == '>') {
    lexer->advance(lexer, false);
    if (scanner->tags.size > 0) {
      tag_clear(&scanner->tags.contents[scanner->tags.size - 1]);
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
    inside_script_or_style = (current_tag->type == SCRIPT || current_tag->type == STYLE);
  }

  // If we're inside script or style tags and raw_text is valid, scan for raw_text
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
      lexer->advance(lexer, false);
      if (lexer->lookahead == '{') {
        lexer->advance(lexer, false);
        return scan_el_expression(lexer);
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
    inside_script_or_style = (current_tag->type == SCRIPT || current_tag->type == STYLE);
  }

  bool is_error_recovery =
      valid_symbols[START_TAG_NAME] && valid_symbols[RAW_TEXT];
  if (!is_error_recovery) {
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
          // Only check for EL expressions if we're NOT inside script or style tags
          TSLexer saved_lexer = *lexer;
          lexer->advance(lexer, false);
          if (lexer->lookahead == '{') {
            // This is an EL expression, restore position and break
            *lexer = saved_lexer;
            lexer->mark_end(lexer);
            break;
          } else {
            // This is just a $ character, continue scanning
            // Note: lexer is already advanced, so continue from current position
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
