typedef enum token_kind {
    TOKEN_EOF,
    TOKEN_NEWLINE,
    TOKEN_COLON,
    TOKEN_COLON_COLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_AT,
    TOKEN_POUND,
    TOKEN_ELLIPSIS,
    TOKEN_QUESTION,
    TOKEN_SEMICOLON,
    TOKEN_INT, 
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_NAME,
    // Unary negation
    TOKEN_BNOT,
    TOKEN_NOT,
    // Multiplicative precedence
    TOKEN_FIRST_MUL,
    TOKEN_MUL = TOKEN_FIRST_MUL,
    TOKEN_DIV,
    TOKEN_MOD,
    TOKEN_AND,
    TOKEN_LSHIFT,
    TOKEN_RSHIFT,
    TOKEN_LAST_MUL = TOKEN_RSHIFT,
    // Additive precedence
    TOKEN_FIRST_ADD,
    TOKEN_ADD = TOKEN_FIRST_ADD,
    TOKEN_SUB,
    TOKEN_XOR,
    TOKEN_OR,
    TOKEN_LAST_ADD = TOKEN_OR,
    // Comparative precedence
    TOKEN_FIRST_CMP,
    TOKEN_EQ = TOKEN_FIRST_CMP,
    TOKEN_NOTEQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LTEQ,
    TOKEN_GTEQ,
    TOKEN_LAST_CMP = TOKEN_GTEQ,
    // Logical And and Or have the two lowest precedences, no need to "guard" them with enum 
    TOKEN_AND_AND,
    TOKEN_OR_OR,
    // Assignment operators
    TOKEN_FIRST_ASSIGN,
    TOKEN_ASSIGN = TOKEN_FIRST_ASSIGN,
    TOKEN_ADD_ASSIGN,
    TOKEN_SUB_ASSIGN,
    TOKEN_OR_ASSIGN,
    TOKEN_AND_ASSIGN,
    TOKEN_XOR_ASSIGN,
    TOKEN_LSHIFT_ASSIGN,
    TOKEN_RSHIFT_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_MOD_ASSIGN,
    TOKEN_LAST_ASSIGN = TOKEN_MOD_ASSIGN,
    
    TOKEN_INC,
    TOKEN_DEC,
    TOKEN_COLON_ASSIGN,
    NUM_TOKEN_KINDS,
} token_kind;

typedef struct token {
    token_kind Kind;
    char *Start;
    char *End;
    union {
        s64 IntVal;
        f64 FloatVal;
        char* StringVal;
        char* Name;
    };
} token;

typedef enum lexer_flags
{
    LEXER_INIT = 1,
    LEXER_IGNORE_NEWLINES = 2,
    LEXER_NO_NEWLINE_IN_STRING = 4,
    LEXER_ESCAPE_IN_STRING = 8,
    LEXER_DOUBLE_SLASH_COMMENT = 16,
    LEXER_SLASH_STAR_COMMENT = 32,
    //If you use this flag the Name field is left null. Use Token.Start and Token.End if needed
    LEXER_NO_NAME_INTERNING = 64, 
    LEXER_XML_COMMENTS = 128,
} lexer_flags;

typedef struct lexer {
    intern_map Intern;
    char* Stream;
    token Token;
    u32 Flags;
} lexer;
