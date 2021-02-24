#include "lexer.h"

u8 GlobalCharToDigit[256];
u8 GlobalEscapeToChar[256];

struct __lexer_init
{
    __lexer_init() {
        GlobalEscapeToChar['n'] = '\n';
        GlobalEscapeToChar['r'] = '\r';
        GlobalEscapeToChar['t'] = '\t';
        GlobalEscapeToChar['v'] = '\v';
        GlobalEscapeToChar['b'] = '\b';
        GlobalEscapeToChar['a'] = '\a';
        GlobalEscapeToChar['0'] = 0;
        
        GlobalCharToDigit['0'] = 0;
        GlobalCharToDigit['1'] = 1;
        GlobalCharToDigit['2'] = 2;
        GlobalCharToDigit['3'] = 3;
        GlobalCharToDigit['4'] = 4;
        GlobalCharToDigit['5'] = 5;
        GlobalCharToDigit['6'] = 6;
        GlobalCharToDigit['7'] = 7;
        GlobalCharToDigit['8'] = 8;
        GlobalCharToDigit['9'] = 9;
        GlobalCharToDigit['a'] = 10;
        GlobalCharToDigit['b'] = 11;
        GlobalCharToDigit['c'] = 12;
        GlobalCharToDigit['d'] = 13;
        GlobalCharToDigit['e'] = 14;
        GlobalCharToDigit['f'] = 15;
        GlobalCharToDigit['A'] = 10;
        GlobalCharToDigit['B'] = 11;
        GlobalCharToDigit['C'] = 12;
        GlobalCharToDigit['D'] = 13;
        GlobalCharToDigit['E'] = 14;
        GlobalCharToDigit['F'] = 15;
    }
};

global_variable __lexer_init __LexerInit;

internal void
LexerError(char* String, ...)
{
    va_list Args;
    va_start(Args, String);
    
    printf("Lexer error: ");
    vprintf(String, Args);
    printf("\n");
    
    va_end(Args);
    
    Assert(0);
}

internal void
ScanInt(lexer* Lexer) {
    u64 Base = 10;
    if (*Lexer->Stream == '0') {
        Lexer->Stream++;
        if (tolower(*Lexer->Stream) == 'x') {
            Lexer->Stream++;
            Base = 16;
        } else if (tolower(*Lexer->Stream) == 'b') {
            Lexer->Stream++;
            Base = 2;
        } else if (isdigit(*Lexer->Stream)) {
            Base = 8;
        }
    }
    u64 Val = 0;
    for (;;) {
        u64 Digit = GlobalCharToDigit[*Lexer->Stream];
        if (Digit == 0 && *Lexer->Stream != '0') {
            break;
        }
        if (Digit >= Base) {
            LexerError("Digit '%c' out of range for Base %dll", *Lexer->Stream, (s64)Base);
            Digit = 0;
        }
        if (Val > (UINT64_MAX - Digit)/Base) {
            LexerError("Integer literal overflow");
            while (isdigit(*Lexer->Stream)) {
                Lexer->Stream++;
            }
            Val = 0;
            break;
        }
        Val = Val*Base + Digit;
        Lexer->Stream++;
    }
    Lexer->Token.Kind = TOKEN_INT;
    Lexer->Token.IntVal = Val;
}

internal void 
ScanFloat(lexer* Lexer) {
    const char *start = Lexer->Stream;
    while (isdigit(*Lexer->Stream)) {
        Lexer->Stream++;
    }
    if (*Lexer->Stream == '.') {
        Lexer->Stream++;
    }
    while (isdigit(*Lexer->Stream)) {
        Lexer->Stream++;
    }
    if (tolower(*Lexer->Stream) == 'e') {
        Lexer->Stream++;
        if (*Lexer->Stream == '+' || *Lexer->Stream == '-') {
            Lexer->Stream++;
        }
        if (!isdigit(*Lexer->Stream)) {
            LexerError("Expected digit after float literal exponent, found '%c'.", *Lexer->Stream);
        }
        while (isdigit(*Lexer->Stream)) {
            Lexer->Stream++;
        }
    }
    double Val = strtod(start, NULL);
    if (Val == HUGE_VAL || Val == -HUGE_VAL) {
        LexerError("Invalid float value");
    }
    Lexer->Token.Kind = TOKEN_FLOAT;
    Lexer->Token.FloatVal = Val;
}

internal void 
ScanString(lexer* Lexer) {
    Assert(*Lexer->Stream == '"');
    Lexer->Stream++;
    _sbuf_ char *String = 0;
    while (*Lexer->Stream && *Lexer->Stream != '"') {
        char Val = *Lexer->Stream;
        if (Lexer->Flags & LEXER_NO_NEWLINE_IN_STRING && Val == '\n') {
            LexerError("String literal cannot contain newline");
        } else if (Lexer->Flags & LEXER_ESCAPE_IN_STRING && Val == '\\') {
            Lexer->Stream++;
            Val = GlobalEscapeToChar[*Lexer->Stream];
            if (Val == 0 && *Lexer->Stream != '0') {
                LexerError("Invalid string literal escape '\\%c'", *Lexer->Stream);
            }
        }
        SbufPush(String, Val);
        Lexer->Stream++;
    }
    if (*Lexer->Stream) {
        Assert(*Lexer->Stream == '"');
        Lexer->Stream++;
    } else {
        LexerError("Unexpected end of file within string literal");
    }
    SbufPush(String, (char)0);
    Lexer->Token.Kind = TOKEN_STRING;
    Lexer->Token.StringVal = String;
}

internal void 
ScanChar(lexer* Lexer) {
    Assert(*Lexer->Stream == '\'');
    Lexer->Stream++;
    char Val = 0;
    if (*Lexer->Stream == '\'') {
        LexerError("Char literal cannot be empty");
        Lexer->Stream++;
    } else if (*Lexer->Stream == '\n') {
        LexerError("Char literal cannot contain newline");
    } else if (*Lexer->Stream == '\\') {
        Lexer->Stream++;
        Val = GlobalEscapeToChar[*Lexer->Stream];
        if (Val == 0 && *Lexer->Stream != '0') {
            LexerError("Invalid char literal escape '\\%c'", *Lexer->Stream);
        }
        Lexer->Stream++;
    } else {
        Val = *Lexer->Stream;
        Lexer->Stream++;
    }
    if (*Lexer->Stream != '\'') {
        LexerError("Expected closing char quote, got '%c'", *Lexer->Stream);
    } else {
        Lexer->Stream++;
    }
    Lexer->Token.Kind = TOKEN_INT;
    Lexer->Token.IntVal = Val;
}


#define CASE1(c1, k1) \
case c1: \
Lexer->Token.Kind = k1; \
Lexer->Stream++; \
break;

#define CASE2(c1, k1, c2, k2) \
case c1: \
Lexer->Token.Kind = k1; \
Lexer->Stream++; \
if (*Lexer->Stream == c2) { \
Lexer->Token.Kind = k2; \
Lexer->Stream++; \
} \
break;

#define CASE3(c1, k1, c2, k2, c3, k3) \
case c1: \
Lexer->Token.Kind = k1; \
Lexer->Stream++; \
if (*Lexer->Stream == c2) { \
Lexer->Token.Kind = k2; \
Lexer->Stream++; \
} else if (*Lexer->Stream == c3) { \
Lexer->Token.Kind = k3; \
Lexer->Stream++; \
} \
break;


internal void
NextToken(lexer* Lexer)
{
    repeat:
    Lexer->Token.Start = Lexer->Stream;
    switch (*Lexer->Stream) {
        case ' ': case '\n': case '\r': case '\t': case '\v':
        while (isspace(*Lexer->Stream)) {
            if(*Lexer->Stream++ == '\n' && !(Lexer->Flags & LEXER_IGNORE_NEWLINES))
            {
                Lexer->Token.Kind = TOKEN_NEWLINE;
                goto no_repeat;
            }
        }
        goto repeat;
        no_repeat:
        break;
        
        case '\'':
        ScanChar(Lexer);
        break;
        
        case '"':
        ScanString(Lexer);
        break;
        
        case '.':
        if(isdigit(Lexer->Stream[1]))
        {
            ScanFloat(Lexer);
        } 
        else if(Lexer->Stream[1] == '.' && Lexer->Stream[2] == '.')
        {
            Lexer->Token.Kind = TOKEN_ELLIPSIS;
            Lexer->Stream += 3;
        }
        else
        {
            Lexer->Token.Kind = TOKEN_DOT;
            Lexer->Stream++;
        }
        break;
        
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
            while (isdigit(*Lexer->Stream)) {
                Lexer->Stream++;
            }
            char C = *Lexer->Stream;
            Lexer->Stream = Lexer->Token.Start;
            if (C == '.' || tolower(C) == 'e') {
                ScanFloat(Lexer);
            } else {
                ScanInt(Lexer);
            }
            break;
        }
        
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case '_': 
        while (isalnum(*Lexer->Stream) || *Lexer->Stream == '_') {
            Lexer->Stream++;
        }
        if(Lexer->Flags & LEXER_NO_NAME_INTERNING)
        {
            Lexer->Token.Name = 0;
        }
        else
        {
            Lexer->Token.Name = InternStringRange(&Lexer->Intern, Lexer->Token.Start, Lexer->Stream);
        }
        Lexer->Token.Kind = TOKEN_NAME;
        break;
        
        case '<':
        Lexer->Token.Kind = TOKEN_LT;
        Lexer->Stream++;
        if (*Lexer->Stream == '<') {
            Lexer->Token.Kind = TOKEN_LSHIFT;
            Lexer->Stream++;
            if (*Lexer->Stream == '=') {
                Lexer->Token.Kind = TOKEN_LSHIFT_ASSIGN;
                Lexer->Stream++;
            }
        } else if (*Lexer->Stream == '=') {
            Lexer->Token.Kind = TOKEN_LTEQ;
            Lexer->Stream++;
        } else if(*Lexer->Stream == '!' && Lexer->Flags & LEXER_XML_COMMENTS) {
            while(*Lexer->Stream)
            {
                if(*Lexer->Stream == '>')
                {
                    Lexer->Stream++;
                    break;
                }
                Lexer->Stream++;
            }
        }
        break;
        
        case '>':
        Lexer->Token.Kind = TOKEN_GT;
        Lexer->Stream++;
        if (*Lexer->Stream == '>') {
            Lexer->Token.Kind = TOKEN_RSHIFT;
            Lexer->Stream++;
            if (*Lexer->Stream == '=') {
                Lexer->Token.Kind = TOKEN_RSHIFT_ASSIGN;
                Lexer->Stream++;
            }
        } else if (*Lexer->Stream == '=') {
            Lexer->Token.Kind = TOKEN_GTEQ;
            Lexer->Stream++;
        }
        break;
        
        case '/':
        Lexer->Token.Kind = TOKEN_DIV;
        Lexer->Stream++;
        if (*Lexer->Stream == '=') {
            Lexer->Token.Kind = TOKEN_DIV_ASSIGN;
            Lexer->Stream++;
        } else if (*Lexer->Stream == '/' && Lexer->Flags & LEXER_DOUBLE_SLASH_COMMENT) {
            Lexer->Stream++;
            while (*Lexer->Stream && *Lexer->Stream != '\n') {
                Lexer->Stream++;
            }
            goto repeat;
        } else if (*Lexer->Stream == '*' && Lexer->Flags & LEXER_SLASH_STAR_COMMENT) {
            Lexer->Stream++;
            int level = 1;
            while (*Lexer->Stream && level > 0) {
                if (Lexer->Stream[0] == '/' && Lexer->Stream[1] == '*') {
                    level++;
                    Lexer->Stream += 2;
                } else if (Lexer->Stream[0] == '*' && Lexer->Stream[1] == '/') {
                    level--;
                    Lexer->Stream += 2;
                } else {
                    Lexer->Stream++;
                }
            }
            goto repeat;
        }
        break;
        
        case '\0':
        Lexer->Token.Kind = TOKEN_EOF;
        break;
        
        CASE1('(', TOKEN_LPAREN)
            CASE1(')', TOKEN_RPAREN)
            CASE1('{', TOKEN_LBRACE)
            CASE1('}', TOKEN_RBRACE)
            CASE1('[', TOKEN_LBRACKET)
            CASE1(']', TOKEN_RBRACKET)
            CASE1(',', TOKEN_COMMA)
            CASE1('@', TOKEN_AT)
            CASE1('#', TOKEN_POUND)
            CASE1('?', TOKEN_QUESTION)
            CASE1(';', TOKEN_SEMICOLON)
            CASE1('~', TOKEN_BNOT)
            CASE2('!', TOKEN_NOT, '=', TOKEN_NOTEQ)
            CASE3(':', TOKEN_COLON, ':', TOKEN_COLON_COLON, '=', TOKEN_COLON_ASSIGN)
            CASE2('=', TOKEN_ASSIGN, '=', TOKEN_EQ)
            CASE2('^', TOKEN_XOR, '=', TOKEN_XOR_ASSIGN)
            CASE2('*', TOKEN_MUL, '=', TOKEN_MUL_ASSIGN)
            CASE2('%', TOKEN_MOD, '=', TOKEN_MOD_ASSIGN)
            CASE3('+', TOKEN_ADD, '=', TOKEN_ADD_ASSIGN, '+', TOKEN_INC)
            CASE3('-', TOKEN_SUB, '=', TOKEN_SUB_ASSIGN, '-', TOKEN_DEC)
            CASE3('&', TOKEN_AND, '=', TOKEN_AND_ASSIGN, '&', TOKEN_AND_AND)
            CASE3('|', TOKEN_OR, '=', TOKEN_OR_ASSIGN, '|', TOKEN_OR_OR)
            
            default:
        LexerError("Invalid '%c' token", *Lexer->Stream);
        Lexer->Stream++;
        goto repeat;
    }
    
    Lexer->Token.End = Lexer->Stream;
}

#undef CASE1
#undef CASE2
#undef CASE3

inline void
JumpToChar(lexer* Lexer, char Char)
{
    while(*Lexer->Stream && *Lexer->Stream != Char)
    {
        Lexer->Stream++;
    }
    NextToken(Lexer);
}

inline b32
IsToken(lexer* Lexer, token_kind Kind)
{
    return Lexer->Token.Kind == Kind;
}

internal b32
ExpectToken(lexer* Lexer, token_kind Kind)
{
    if(IsToken(Lexer, Kind))
    {
        NextToken(Lexer);
        return true;
    }
    else
    {
        LexerError("Expected token %d, got token %d", Kind, Lexer->Token.Kind);
        return false;
    }
}

internal b32
MatchToken(lexer* Lexer, token_kind Kind)
{
    if(IsToken(Lexer, Kind))
    {
        NextToken(Lexer);
        return true;
    }
    else
    {
        return false;
    }
}

internal char*
ParseName(lexer* Lexer)
{
    char* Name = Lexer->Token.Name;
    ExpectToken(Lexer, TOKEN_NAME);
    return Name;
}

internal f32
ParseFloat(lexer* Lexer)
{
    f32 Value = (f32)Lexer->Token.FloatVal;
    ExpectToken(Lexer, TOKEN_FLOAT);
    return Value;
}

internal f32
ParseFloatOrIntWithSign(lexer* Lexer)
{
    b32 Negative = false;
    if(MatchToken(Lexer, TOKEN_SUB))
    {
        Negative = true;
    }
    
    f32 Value;
    if(IsToken(Lexer, TOKEN_FLOAT))
    {
        Value = (f32)Lexer->Token.FloatVal;
        NextToken(Lexer);
    }
    else
    {
        Value = (f32)Lexer->Token.IntVal;
        ExpectToken(Lexer, TOKEN_INT);
    }
    
    if(Negative) Value = -Value;
    return Value;
}

internal u32
ParseInt(lexer* Lexer)
{
    u32 Result = (u32)Lexer->Token.IntVal;
    ExpectToken(Lexer, TOKEN_INT);
    return Result;
}

internal mat4
ParseMat4RowMajor(lexer* Lexer)
{
    mat4 Result;
    for(u32 Row = 0; Row < 4; Row++)
    {
        for(u32 Column = 0; Column < 4; Column++)
        {
            Result.e[Column][Row] = ParseFloatOrIntWithSign(Lexer);
        }
    }
    
    return Result;
}

internal void
InitLexer(lexer* Lexer, char* Stream, u32 Flags = 0)
{
    *Lexer = {};
    Lexer->Flags = Flags | LEXER_INIT;
    Lexer->Stream = Stream;
    NextToken(Lexer);
};

internal void
FreeLexer(lexer* Lexer)
{
    FreeInternMap(&Lexer->Intern);
    *Lexer = {};
}
