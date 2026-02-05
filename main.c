#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_OPCODE_LENGTH 4
#define NUM_OPCODES 2

typedef enum _Opcode
{
    OPCODE_JUMP,
    OPCODE_LW,
} Opcode;

typedef enum _InstrType
{
    INSTR_TYPE_I,
    INSTR_TYPE_J,
    INSTR_TYPE_R,
} InstrType;

typedef struct _InstrProperties
{
    InstrType type;
    unsigned short opcode;
    char name[MAX_OPCODE_LENGTH];
} InstrProperties;

typedef enum _TokenType
{
    TOKEN_TYPE_INVALID,
    TOKEN_TYPE_OPCODE,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_REGISTER,
    TOKEN_TYPE_WHITESPACE,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_NEW_LINE,
    TOKEN_TYPE_EOF,
} TokenType;

union TokenData
{
    unsigned int number;
    char* string;
    Opcode opcode;
};

typedef struct _Token
{
    TokenType type;
    unsigned int lineNumber;
    union TokenData data;
    struct _Token* next;
} Token;

const InstrProperties instructionProperties[] = {
    {INSTR_TYPE_J, 0b000010, "J"},
    {INSTR_TYPE_I, 0b100011, "LW"},
};

inline int IsWhitespaceChar(char c)
{
    return c == ' ' || c == '\t';
}

inline int IsValidNameChar(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

void ParseRegisterToken(size_t* p, char* text, Token* token)
{
    char* numStart = &text[++(*p)];
    char* end = 0;

    unsigned int value = strtoul(numStart, &end, 10);
    
    if(numStart == end || value > 31)
    {
        token->type = TOKEN_TYPE_INVALID;
        return;
    } else
    {
        token->type = TOKEN_TYPE_REGISTER;
        token->data.number = value;

        *p += end - numStart;
    }
}

void ParseWhitespaceToken(size_t* p, char* text, Token* token)
{
    while(IsWhitespaceChar(text[*p]))
    {
        (*p)++;
    }

    token->type = TOKEN_TYPE_WHITESPACE;
}

void ParseNumberToken(size_t* p, char* text, Token* token)
{
    char* numStart = &text[*p];
    char* end = 0;

    unsigned int value = strtoul(numStart, &end, 0);
    
    if(numStart == end)
    {
        token->type = TOKEN_TYPE_INVALID;
        return;
    } else
    {
        token->type = TOKEN_TYPE_NUMBER;
        token->data.number = value;

        *p += end - numStart;
    }
}

void ParseOpcodeToken(size_t* p, char* text, Token* token)
{
    token->type = TOKEN_TYPE_INVALID;

    char opcodeName[MAX_OPCODE_LENGTH];
    memset(opcodeName, 0, MAX_OPCODE_LENGTH);

    for(int i = 0; i < MAX_OPCODE_LENGTH; i++)
    {
        char readChar = text[*p];

        if(!IsValidNameChar(readChar))
        {
            break;
        }
        
        opcodeName[i] = readChar;

        (*p)++;
    }

    for(int i = 0; i < NUM_OPCODES; i++)
    {
        if(strcmp(opcodeName, instructionProperties[i].name) == 0)
        {
            token->type = TOKEN_TYPE_OPCODE;
            token->data.opcode = (Opcode)i;
            return;
        }
    }
    
    printf("Invalid opcode!\n");
}

int ParseToken(size_t* p, char* text, Token* token, unsigned int* lineNumber)
{
    token->lineNumber = *lineNumber;

    char firstChar = text[*p];

    switch(firstChar)
    {
    case '$':
        ParseRegisterToken(p, text, token);
        break;

    case ' ':
        ParseWhitespaceToken(p, text, token);
        break;

    case '\t':
        ParseWhitespaceToken(p, text, token);
        break;

    case ',':
        token->type = TOKEN_TYPE_COMMA;
        (*p)++;
        break;

    case '\n':
        token->type = TOKEN_TYPE_NEW_LINE;
        (*p)++;
        (*lineNumber)++;
        break;

    case '\0':
        token->type = TOKEN_TYPE_EOF;
        (*p)++;
        break;

    default:
        if(firstChar >= '0' && firstChar <= '9') // If firstChar is a number char.
        {
            ParseNumberToken(p, text, token);
        } else if(
            (firstChar >= 'A' && firstChar <= 'Z') ||
            (firstChar >= 'a' && firstChar <= 'z')
        ) // If firstChar is a standard letter.
        {
            ParseOpcodeToken(p, text, token);
        } else
        {
            token->type = TOKEN_TYPE_INVALID;
        }
        break;
    }

    if(token->type == TOKEN_TYPE_INVALID)
    {
        printf("Invalid token on line %d!\n", token->lineNumber);
        return 1;
    }

    return 0;
}

void DestroyTokens(Token* rootToken)
{
    Token* token = rootToken;
    
    while(token->next)
    {
        Token* next = token->next;

        if(token != rootToken)
        {
            free(token);
        }

        token = next;
    }
}

int Tokenize(char* text, Token* rootToken)
{
    size_t p = 0;
    unsigned int lineNumber = 1;
    
    Token* token = 0;
    Token* nextToken = rootToken;

    do
    {
        token = nextToken;

        int result = ParseToken(&p, text, token, &lineNumber);
        if(result == 1)
        {
            token->next = 0;
            return 1;
        }

        token->next = malloc(sizeof(Token));
        if(!token->next)
        {
            printf("Memory allocation for next token failed!\n");
            return 1;
        }

        nextToken = token->next;
    } while(token->type != TOKEN_TYPE_EOF);

    token->next = 0;

    return 0;
}

unsigned int CountTokensOfType(Token* rootToken, TokenType type)
{
    Token* token = rootToken;

    unsigned int count = 0;

    while(token->type != TOKEN_TYPE_EOF)
    {
        if(token->type == type)
        {
            count++;
        }

        token = token->next;
    }
    
    return count;
}

Token* GetParams(unsigned int numParams, TokenType* types, unsigned int** out, Token* token)
{
    token = token->next;

    for(unsigned int i = 0; i < numParams; i++)
    {
        TokenType type = types[i];
        if(token->type != type)
        {
            printf("Invalid arguemnt type!");
            return 0;
        }

        *(out[i]) = token->data.number;
        
        token = token->next;
        if(i < numParams - 1)
        {
            if(token->type != TOKEN_TYPE_COMMA)
            {
                printf("Expected comma after non-final argument!");
                return 0;
            }

            token = token->next;
            if(token->type == TOKEN_TYPE_WHITESPACE)
            {
                token = token->next;
            }
        }
    }

    return token;
}

Token* ParseITypeInstr(unsigned int* rs, unsigned int* rt, unsigned int* imm, Token* token)
{
    TokenType paramTypes[] = {TOKEN_TYPE_REGISTER, TOKEN_TYPE_REGISTER, TOKEN_TYPE_NUMBER};
    unsigned int* params[] = {rs, rt, imm};
    
    token = GetParams(3, paramTypes, params, token);
    if(token == 0)
    {
        return 0;
    }

    if(*imm > 0xFFFF)
    {
        printf("Immediate must only contain 16-bit values!");
        return 0;
    }

    return token;
}

Token* ParseJTypeInstr(unsigned int* target, Token* token)
{
    TokenType paramType = TOKEN_TYPE_NUMBER;
    token = GetParams(1, &paramType, &target, token);
    if(token == 0)
    {
        return 0;
    }

    if(*target > 0x3FFFFFF)
    {
        printf("Target must only contain 26-bit values!");
        return 0;
    }
    return token;
}

Token* ParseRTypeInstr(
    unsigned int* rs, unsigned int* rt, unsigned int* rd,
    unsigned int* shift, unsigned int* function,
    Token* token
)
{
    TokenType paramTypes[] = {
        TOKEN_TYPE_REGISTER, TOKEN_TYPE_REGISTER, TOKEN_TYPE_REGISTER,
        TOKEN_TYPE_NUMBER, TOKEN_TYPE_NUMBER
    };
    unsigned int* params[] = {rs, rt, rd, shift, function};
    
    token = GetParams(5, paramTypes, params, token);
    if(token == 0)
    {
        return 0;
    }

    if(*shift > 0x1F)
    {
        printf("Shift must only contain 5-bit values!");
        return 0;
    }

    if(*function > 0x3F)
    {
        printf("Function must only contain 6-bit values!");
        return 0;
    }

    return token;
}

unsigned int CreateInstruction(
    unsigned int opcode, unsigned int rs, unsigned int rt,
    unsigned int imm,
    unsigned int target,
    unsigned int rd, unsigned int shift, unsigned int function,
    InstrType type
)
{
    switch(type)
    {
    case INSTR_TYPE_I:
        return (opcode << 26) | (rs << 21) | (rt << 16) | imm;
        break;

    case INSTR_TYPE_J:
        return (opcode << 26) | (target);
        break;

    case INSTR_TYPE_R:
        return (opcode << 26) | (rs << 21) | (rt << 16) | (rd << 11) | (shift << 6) | function;
        break;
    }

    return 0;
}

int GenerateMachineCode(Token* rootToken, unsigned int* binary, unsigned int numInstr)
{
    Token* token = rootToken;

    int result = 0;

    for(unsigned int i = 0; i < numInstr; i++)
    {
        unsigned int lineNumber = token->lineNumber;

        // I and R type.
        unsigned int opcode = 0;
        unsigned int rs = 0, rt = 0;

        // Instruction type I only.
        unsigned int imm = 0;

        // Instruction type J only
        unsigned int target = 0;

        // Instruction type R only.
        unsigned int rd = 0, shift = 0;
        unsigned int function = 0;

        if(token->type != TOKEN_TYPE_OPCODE)
        {
            printf("Expected opcode on line %u!\n", token->lineNumber);
            result = 1;
            break;
        }

        InstrProperties properties = instructionProperties[token->data.opcode];

        opcode = properties.opcode;

        token = token->next;
        if(token->type != TOKEN_TYPE_WHITESPACE)
        {
            printf("Expected whitespace after opcode on line %u\n", token->lineNumber);
            result = 1;
            break;
        }

        switch(properties.type)
        {
        case INSTR_TYPE_I:
            token = ParseITypeInstr(&rs, &rt, &imm, token);
            break;

        case INSTR_TYPE_J:
            token = ParseJTypeInstr(&target, token);
            break;
        
        case INSTR_TYPE_R:
            token = ParseRTypeInstr(&rs, &rt, &rd, &shift, &function, token);
            break;
        }

        if(token == 0)
        {
            printf(" On line %u\n", lineNumber);
            result = 1;
            break;
        }

        binary[i] = CreateInstruction(
            opcode, rs, rt,
            imm,
            target,
            rd, shift, function,
            properties.type
        );

        if(properties.type == INSTR_TYPE_I)
        {
            printf("opcode %u, rs %u, rt %u, imm %u\n", opcode, rs, rt, imm);
        } else if(properties.type == INSTR_TYPE_J)
        {
            printf("opcode %u, target %u\n", opcode, target);
        }

        if(token->type == TOKEN_TYPE_EOF)
        {
            break;
        }

        if(token->type != TOKEN_TYPE_NEW_LINE)
        {
            printf("Expected new line after instruction!\n");
            result = 1;
            break;
        }
        token = token->next;
    }

    return result;
}

int WriteToDst(char* dstFilename, unsigned int* binary, unsigned int numInstr)
{
    FILE* dstFile = fopen(dstFilename, "w");
    if(!dstFile)
    {
        printf("'%s' no such file in this directory!\n", dstFilename);
        return 1;
    }
    
    for(unsigned int i = 0; i < numInstr; i++)
    {
        fprintf(dstFile, "%08X\n", binary[i]);
    }

    fclose(dstFile);

    return 0;
}

int Assemble(char* srcFilename, char* dstFilename)
{
    FILE* srcFile = fopen(srcFilename, "r");
    if(!srcFile)
    {
        printf("'%s' no such file in this directory!\n", srcFilename);
        return 1;
    }

    fseek(srcFile, 0, SEEK_END);

    size_t srcFileSize = ftell(srcFile);

    fseek(srcFile, 0, SEEK_SET);

    char* text = malloc(srcFileSize + 1); // +1 for null byte.
    if(!text)
    {
        printf("Memory allocation for src file contents failed!\n");

        fclose(srcFile);
        return 1;
    }

    fread(text, 1, srcFileSize, srcFile);

    text[srcFileSize] = '\0';

    fclose(srcFile);

    Token rootToken = {0};
    int result = Tokenize(text, &rootToken);
    if(result == 1)
    {        
        DestroyTokens(&rootToken);
        free(text);

        return 1;
    }

    free(text);

    unsigned int numOpcodes = CountTokensOfType(&rootToken, TOKEN_TYPE_OPCODE);
    if(numOpcodes == 0)
    {
        printf("File must have at least one instruction!\n");
        return 1;
    }

    // Use numOpcodes as an estimate for instruction count.
    unsigned int* binary = calloc(numOpcodes, sizeof(unsigned int));

    result = GenerateMachineCode(&rootToken, binary, numOpcodes);
    if(result == 1)
    {
        free(binary);
        DestroyTokens(&rootToken);
        return 1;
    }

    DestroyTokens(&rootToken);

    result = WriteToDst(dstFilename, binary, numOpcodes);

    free(binary);

    return result;
}

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        printf("Invalid argument count!\n");
        return 1;
    }

    return Assemble(argv[1], argv[2]);
}