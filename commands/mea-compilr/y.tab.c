/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NOMBRE = 258,
     IDENTIFIANT = 259,
     CHAINE = 260,
     INSTRUCTION = 261,
     FONCTION = 262,
     SPECIAL = 263,
     BOOL = 264,
     OPERATOR_EQ = 265,
     OPERATOR_NE = 266,
     OPERATOR_GE = 267,
     OPERATOR_LE = 268,
     OPERATOR_GT = 269,
     OPERATOR_LW = 270,
     AFFECTATION = 271,
     CROCHET_O = 272,
     CROCHET_F = 273,
     ACCOLADE_O = 274,
     ACCOLADE_F = 275,
     PARENTHESE_O = 276,
     PARENTHESE_F = 277,
     VIRGULE = 278,
     INSTRUCTION_IS = 279,
     INSTRUCTION_DO = 280,
     INSTRUCTION_WITH = 281,
     INSTRUCTION_IF = 282,
     INSTRUCTION_ELSEIS = 283,
     INSTRUCTION_ONMATCH = 284,
     INSTRUCTION_ONNOTMATCH = 285,
     INSTRUCTION_WHEN = 286,
     ACTION_BREAK = 287,
     ACTION_CONTINUE = 288,
     ACTION_MOVEFORWARD = 289,
     FALL = 290,
     RISE = 291,
     CHANGE = 292,
     ERROR = 293
   };
#endif
/* Tokens.  */
#define NOMBRE 258
#define IDENTIFIANT 259
#define CHAINE 260
#define INSTRUCTION 261
#define FONCTION 262
#define SPECIAL 263
#define BOOL 264
#define OPERATOR_EQ 265
#define OPERATOR_NE 266
#define OPERATOR_GE 267
#define OPERATOR_LE 268
#define OPERATOR_GT 269
#define OPERATOR_LW 270
#define AFFECTATION 271
#define CROCHET_O 272
#define CROCHET_F 273
#define ACCOLADE_O 274
#define ACCOLADE_F 275
#define PARENTHESE_O 276
#define PARENTHESE_F 277
#define VIRGULE 278
#define INSTRUCTION_IS 279
#define INSTRUCTION_DO 280
#define INSTRUCTION_WITH 281
#define INSTRUCTION_IF 282
#define INSTRUCTION_ELSEIS 283
#define INSTRUCTION_ONMATCH 284
#define INSTRUCTION_ONNOTMATCH 285
#define INSTRUCTION_WHEN 286
#define ACTION_BREAK 287
#define ACTION_CONTINUE 288
#define ACTION_MOVEFORWARD 289
#define FALL 290
#define RISE 291
#define CHANGE 292
#define ERROR 293




/* Copy the first part of user declarations.  */
#line 1 "mea-compilr2.yacc"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <getopt.h>
#include <errno.h>

#include "cJSON.h"

/* allouees/definies dans/par lex (mea-compilr2.lex) */
extern int yylineno;
extern int file_offset;
extern int line_offset;
extern char *yytext;
extern FILE *yyin;

/* globales */
// 
char *current_file = NULL;
cJSON *rulesSet = NULL;
FILE *fdi = NULL;
FILE *fdo = NULL;
int nbInput = 0;
int nbOutput = 0;
int filenum = 0;

// options de la ligne de commande
int indent_flag = 0;
int debug_flag = 0;
int verbose_flag = 0;
int optimize_flag = 0;
int jsonerror_flag = 0;
char output_file[255] = "";
char rules_path[255] = "";
char rulesset_file[255] = "";

/* prototypes de fonctions */
int yylex (void);
int yyerror(char *s);

int nl(FILE *fd);
int space(FILE *fd, int nb);
int indent(FILE *fd, int nb);

void printError(int errortojson, int num, char *file, int line, int column, char *near, char *text);



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 51 "mea-compilr2.yacc"
{
   char *str;
}
/* Line 193 of yacc.c.  */
#line 226 "y.tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 239 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   64

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  39
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  33
/* YYNRULES -- Number of rules.  */
#define YYNRULES  59
/* YYNRULES -- Number of states.  */
#define YYNSTATES  90

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   293

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    11,    14,    19,    25,
      27,    28,    29,    33,    34,    35,    39,    41,    43,    46,
      47,    48,    60,    62,    64,    67,    69,    71,    73,    75,
      76,    78,    79,    84,    88,    92,    96,    97,   103,   106,
     107,   110,   111,   116,   120,   122,   124,   126,   128,   130,
     132,   134,   136,   138,   140,   142,   146,   151,   152,   154
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      40,     0,    -1,    -1,    40,    41,    -1,    42,    -1,    43,
      -1,    60,    44,    -1,    60,    61,    44,    46,    -1,    60,
      61,    63,    44,    46,    -1,    49,    -1,    -1,    -1,    29,
      45,    48,    -1,    -1,    -1,    30,    47,    48,    -1,    32,
      -1,    33,    -1,    34,     4,    -1,    -1,    -1,    52,    25,
      53,    26,    21,    50,    57,    51,    22,    31,    54,    -1,
       4,    -1,     4,    -1,    55,    56,    -1,     4,    -1,    36,
      -1,    35,    -1,    37,    -1,    -1,    59,    -1,    -1,    57,
      23,    58,    59,    -1,     4,    16,    69,    -1,     4,    24,
      69,    -1,     4,    24,     8,    -1,    -1,    27,    21,    62,
      64,    22,    -1,    28,    69,    -1,    -1,    65,    67,    -1,
      -1,    64,    23,    66,    67,    -1,    69,    68,    69,    -1,
      10,    -1,    11,    -1,    12,    -1,    13,    -1,    14,    -1,
      15,    -1,     3,    -1,     4,    -1,     5,    -1,     9,    -1,
      70,    -1,    19,     4,    20,    -1,     7,    17,    71,    18,
      -1,    -1,    69,    -1,    71,    23,    69,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   113,   113,   114,   118,   119,   123,   124,   125,   129,
     131,   132,   132,   135,   136,   136,   140,   141,   142,   146,
     146,   146,   150,   183,   186,   189,   192,   193,   194,   196,
     197,   198,   198,   202,   206,   249,   296,   296,   300,   304,
     304,   305,   305,   309,   324,   325,   326,   327,   328,   329,
     333,   334,   335,   336,   337,   338,   342,   364,   365,   371
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NOMBRE", "IDENTIFIANT", "CHAINE",
  "INSTRUCTION", "FONCTION", "SPECIAL", "BOOL", "OPERATOR_EQ",
  "OPERATOR_NE", "OPERATOR_GE", "OPERATOR_LE", "OPERATOR_GT",
  "OPERATOR_LW", "AFFECTATION", "CROCHET_O", "CROCHET_F", "ACCOLADE_O",
  "ACCOLADE_F", "PARENTHESE_O", "PARENTHESE_F", "VIRGULE",
  "INSTRUCTION_IS", "INSTRUCTION_DO", "INSTRUCTION_WITH", "INSTRUCTION_IF",
  "INSTRUCTION_ELSEIS", "INSTRUCTION_ONMATCH", "INSTRUCTION_ONNOTMATCH",
  "INSTRUCTION_WHEN", "ACTION_BREAK", "ACTION_CONTINUE",
  "ACTION_MOVEFORWARD", "FALL", "RISE", "CHANGE", "ERROR", "$accept",
  "Rules", "Rule", "InputRule", "OutputRule", "OnmatchBloc", "@1",
  "OnnotmatchBloc", "@2", "MatchActions", "DoBloc", "@3", "@4",
  "DoIdentifiant", "DoAction", "DoCondition", "DoConditionIdentifiant",
  "DoEdge", "DoActionParametres", "@5", "DoActionParametre", "IsBloc",
  "IfBloc", "@6", "ElseisBloc", "Conditions", "@7", "@8", "Condition",
  "Operateur", "Valeur", "Fonction", "Fonction_parametres", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    39,    40,    40,    41,    41,    42,    42,    42,    43,
      44,    45,    44,    46,    47,    46,    48,    48,    48,    50,
      51,    49,    52,    53,    54,    55,    56,    56,    56,    57,
      57,    58,    57,    59,    60,    60,    62,    61,    63,    65,
      64,    66,    64,    67,    68,    68,    68,    68,    68,    68,
      69,    69,    69,    69,    69,    69,    70,    71,    71,    71
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     2,     4,     5,     1,
       0,     0,     3,     0,     0,     3,     1,     1,     2,     0,
       0,    11,     1,     1,     2,     1,     1,     1,     1,     0,
       1,     0,     4,     3,     3,     3,     0,     5,     2,     0,
       2,     0,     4,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     4,     0,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,    22,     3,     4,     5,     9,     0,    10,
       0,     0,     0,    11,     6,    10,    50,    51,    52,     0,
      35,    53,     0,    34,    54,    23,     0,    36,     0,     0,
      13,    10,    57,     0,     0,    39,    16,    17,     0,    12,
      38,    14,     7,    13,    58,     0,    55,    19,     0,     0,
      18,     0,     8,    56,     0,    29,    37,    41,    40,     0,
      15,    59,     0,    20,    30,     0,    44,    45,    46,    47,
      48,    49,     0,     0,    31,     0,    42,    43,    33,     0,
       0,    32,     0,    25,    21,     0,    27,    26,    28,    24
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     1,     4,     5,     6,    14,    28,    42,    51,    39,
       7,    55,    75,     8,    26,    84,    85,    89,    63,    79,
      64,     9,    15,    35,    31,    48,    49,    65,    58,    72,
      59,    24,    45
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -19
static const yytype_int8 yypact[] =
{
     -19,    31,   -19,    -6,   -19,   -19,   -19,   -19,    -4,   -14,
      -2,    39,     2,   -19,   -19,    11,   -19,   -19,   -19,    28,
     -19,   -19,    42,   -19,   -19,   -19,    21,   -19,     0,     5,
      18,    20,     5,    30,    32,   -19,   -19,   -19,    47,   -19,
     -19,   -19,   -19,    18,   -19,    -7,   -19,   -19,    19,     5,
     -19,     0,   -19,   -19,     5,    48,   -19,   -19,   -19,    15,
     -19,   -19,    38,    33,   -19,     5,   -19,   -19,   -19,   -19,
     -19,   -19,     5,     5,   -19,    35,   -19,   -19,   -19,    48,
      24,   -19,    54,   -19,   -19,     1,   -19,   -19,   -19,   -19
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -19,   -19,   -19,   -19,   -19,   -11,   -19,    16,   -19,     9,
     -19,   -19,   -19,   -19,   -19,   -19,   -19,   -19,   -19,   -19,
     -18,   -19,   -19,   -19,   -19,   -19,   -19,   -19,    -1,   -19,
     -10,   -19,   -19
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      23,    16,    17,    18,    30,    19,    20,    21,    16,    17,
      18,    53,    19,    12,    21,    13,    54,    22,    10,    40,
      43,    11,    44,    27,    22,    66,    67,    68,    69,    70,
      71,     2,    36,    37,    38,     3,    86,    87,    88,    29,
      13,    56,    57,    25,    61,    32,    33,    34,    41,    13,
      46,    50,    62,    47,    73,    82,    74,    80,    83,    52,
      60,    81,    77,    78,    76
};

static const yytype_uint8 yycheck[] =
{
      10,     3,     4,     5,    15,     7,     8,     9,     3,     4,
       5,    18,     7,    27,     9,    29,    23,    19,    24,    29,
      31,    25,    32,    21,    19,    10,    11,    12,    13,    14,
      15,     0,    32,    33,    34,     4,    35,    36,    37,    28,
      29,    22,    23,     4,    54,    17,     4,    26,    30,    29,
      20,     4,     4,    21,    16,    31,    23,    22,     4,    43,
      51,    79,    72,    73,    65
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    40,     0,     4,    41,    42,    43,    49,    52,    60,
      24,    25,    27,    29,    44,    61,     3,     4,     5,     7,
       8,     9,    19,    69,    70,     4,    53,    21,    45,    28,
      44,    63,    17,     4,    26,    62,    32,    33,    34,    48,
      69,    30,    46,    44,    69,    71,    20,    21,    64,    65,
       4,    47,    46,    18,    23,    50,    22,    23,    67,    69,
      48,    69,     4,    57,    59,    66,    10,    11,    12,    13,
      14,    15,    68,    16,    23,    51,    67,    69,    69,    58,
      22,    59,    31,     4,    54,    55,    35,    36,    37,    56
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 4:
#line 118 "mea-compilr2.yacc"
    { nl(fdi); indent(fdi, 2); fputc('}', fdi); /* nl(fdi); */}
    break;

  case 5:
#line 119 "mea-compilr2.yacc"
    { nl(fdo); indent(fdo, 2); fputc('}', fdo); /* nl(fdo); */}
    break;

  case 11:
#line 132 "mea-compilr2.yacc"
    { fputc(',', fdi); nl(fdi); indent(fdi,3); fputs("\"onmatch\":", fdi); space(fdi, 1); }
    break;

  case 14:
#line 136 "mea-compilr2.yacc"
    { fputc(',', fdi); nl(fdi); indent(fdi, 3); fputs("\"onnotmatch\":", fdi); space(fdi, 1); }
    break;

  case 16:
#line 140 "mea-compilr2.yacc"
    { fputs("\"break\"", fdi); }
    break;

  case 17:
#line 141 "mea-compilr2.yacc"
    { fputs("\"continue\"", fdi); }
    break;

  case 18:
#line 142 "mea-compilr2.yacc"
    { fprintf(fdi, "\"moveforward %s\"", (yyvsp[(2) - (2)].str)); }
    break;

  case 19:
#line 146 "mea-compilr2.yacc"
    { fputc(',',fdo); nl(fdo); indent(fdo, 3); fputs("\"parameters\":", fdo); space(fdo, 1); fputc('{', fdo); nl(fdo); }
    break;

  case 20:
#line 146 "mea-compilr2.yacc"
    { nl(fdo); indent(fdo, 3); fputc('}', fdo); }
    break;

  case 21:
#line 146 "mea-compilr2.yacc"
    { space(fdo, 1); fputc('}', fdo); }
    break;

  case 22:
#line 150 "mea-compilr2.yacc"
    {
                   if(nbOutput>0)
                   {
                      fputc(',',fdo);
                      nl(fdo);
                   }
                   nbOutput++;

                   indent(fdo, 2);
                   fputc('{',fdo);
                   nl(fdo);
                   indent(fdo, 3);
                   fputs("\"name\":",fdo);
                   space(fdo, 1);
                   fprintf(fdo, "\"%s\"", (yyvsp[(1) - (1)].str));

                   if(debug_flag == 1)
                   {
                      fputc(',',fdo);
                      nl(fdo);
                      indent(fdo, 3);
                      fputs("\"file\":", fdo);
                      space(fdo, 1);
                      fprintf(fdo, "%d,", filenum);
                      nl(fdo);
                      indent(fdo, 3);
                      fputs("\"line\":", fdo);
                      space(fdo, 1);
                      fprintf(fdo, "%d", yylineno);
                   }
                }
    break;

  case 23:
#line 183 "mea-compilr2.yacc"
    { fputc(',',fdo); nl(fdo); indent(fdo, 3); fputs( "\"action\":", fdo); space(fdo, 1); fprintf(fdo, "\"%s\"", (yyvsp[(1) - (1)].str)); }
    break;

  case 25:
#line 189 "mea-compilr2.yacc"
    { fputc(',',fdo); nl(fdo); indent(fdo, 3); fputs("\"condition\":",fdo); space(fdo, 1); fputc('{',fdo); space(fdo,1); fprintf(fdo,"\"%s\":", (yyvsp[(1) - (1)].str)); space(fdo, 1); }
    break;

  case 26:
#line 192 "mea-compilr2.yacc"
    { fprintf(fdo, "1"); }
    break;

  case 27:
#line 193 "mea-compilr2.yacc"
    { fprintf(fdo, "2"); }
    break;

  case 28:
#line 194 "mea-compilr2.yacc"
    { fprintf(fdo, "3"); }
    break;

  case 31:
#line 198 "mea-compilr2.yacc"
    { fputc(',', fdo); nl(fdo); }
    break;

  case 33:
#line 202 "mea-compilr2.yacc"
    { indent(fdo, 4); fprintf(fdo, "\"%s\": \"%s\"", (yyvsp[(1) - (3)].str), (yyvsp[(3) - (3)].str)); free((yyvsp[(3) - (3)].str)); }
    break;

  case 34:
#line 206 "mea-compilr2.yacc"
    {
                                          if(nbInput>0)
                                          {
                                             fputc(',', fdi);
                                             nl(fdi);
                                          }
                                          nbInput++;

                                          indent(fdi, 2);
                                          fputc('{', fdi);
                                          nl(fdi);
                                          indent(fdi, 3); 
                                          fputs("\"name\":", fdi);
                                          space(fdi, 1);
                                          fprintf(fdi,"\"%s\",", (yyvsp[(1) - (3)].str));
                                          nl(fdi);
                                          indent(fdi, 3);
                                          fputs("\"value\":",fdi);
                                          space(fdi, 1);
                                          fprintf(fdi,"\"%s\",", (yyvsp[(3) - (3)].str));
                                          nl(fdi);
                                          indent(fdi, 3);
                                          fputs("\"num\":", fdi);
                                          space(fdi, 1);
                                          fprintf(fdi, "%d", nbInput);

                                          if(debug_flag == 1)
                                          {
                                             fputc(',',fdi); 
                                             nl(fdi);
                                             indent(fdi, 3);
                                             fputs("\"file\":", fdi);
                                             space(fdi, 1);
                                             fprintf(fdi, "%d,", filenum);
                                             nl(fdi);
                                             indent(fdi, 3);
                                             fputs("\"line\":", fdi);
                                             space(fdi, 1);
                                             fprintf(fdi, "%d", yylineno);
                                          }

                                          free((yyvsp[(3) - (3)].str));
                                       }
    break;

  case 35:
#line 249 "mea-compilr2.yacc"
    {
                                          nbInput++;
                                          if(nbInput>0)
                                          {
                                             fputc(',', fdi);
                                             nl(fdi);
                                          }
                                          nbInput++;

                                          indent(fdi, 2);
                                          fputc('{', fdi);
                                          nl(fdi);
                                          indent(fdi, 3);
                                          fputs("\"name\":", fdi);
                                          space(fdi, 1);
                                          fprintf(fdi,"\"%s\",", (yyvsp[(1) - (3)].str));
                                          nl(fdi);
                                          indent(fdi, 3);
                                          fputs("\"value\":",fdi);
                                          space(fdi, 1);
                                          fprintf(fdi,"\"%s\",", (yyvsp[(3) - (3)].str));
                                          nl(fdi);
                                          indent(fdi, 3);
                                          fputs("\"num\":", fdi);
                                          space(fdi, 1);
                                          fprintf(fdi, "%d", nbInput);

                                          if(debug_flag == 1)
                                          {
                                             fputc(',',fdi);
                                             nl(fdi);
                                             indent(fdi, 3);
                                             fputs("\"file\":", fdi);
                                             space(fdi, 1);
                                             fprintf(fdi, "%d,", filenum);
                                             nl(fdi);
                                             indent(fdi, 3);
                                             fputs("\"line\":", fdi);
                                             space(fdi, 1);
                                             fprintf(fdi, "%d", yylineno);
                                          }
 
                                          free((yyvsp[(3) - (3)].str));
                                       }
    break;

  case 36:
#line 296 "mea-compilr2.yacc"
    { fputc(',',fdi); nl(fdi); indent(fdi, 3); fputs("\"conditions\":[", fdi); nl(fdi); }
    break;

  case 37:
#line 296 "mea-compilr2.yacc"
    { nl(fdi); indent(fdi, 3); fputc(']', fdi); }
    break;

  case 38:
#line 300 "mea-compilr2.yacc"
    { fputc(',',fdi); nl(fdi); indent(fdi, 3); fputs("\"altvalue\":", fdi); space(fdi, 1); fprintf(fdi, "\"%s\"", (yyvsp[(2) - (2)].str)); free((yyvsp[(2) - (2)].str)); }
    break;

  case 39:
#line 304 "mea-compilr2.yacc"
    { indent(fdi, 4); fputc('{',fdi); }
    break;

  case 40:
#line 304 "mea-compilr2.yacc"
    { fputc('}',fdi); }
    break;

  case 41:
#line 305 "mea-compilr2.yacc"
    { fputc(',', fdi); nl(fdi); indent(fdi, 4); fputc('{', fdi); }
    break;

  case 42:
#line 305 "mea-compilr2.yacc"
    { fputc('}', fdi); }
    break;

  case 43:
#line 309 "mea-compilr2.yacc"
    {
                             fputs("\"value1\":", fdi);
                             space(fdi, 1);
                             fprintf(fdi, "\"%s\",",(yyvsp[(1) - (3)].str));
                             space(fdi, 1);
                             fputs("\"value2\":", fdi);
                             fprintf(fdi, "\"%s\",", (yyvsp[(3) - (3)].str));
                             space(fdi, 1);
                             fputs("\"op\":",fdi);
                             fprintf(fdi, "\"%s\"", (yyvsp[(2) - (3)].str));
                             free((yyvsp[(3) - (3)].str));
                          }
    break;

  case 44:
#line 324 "mea-compilr2.yacc"
    { (yyval.str) = "=="; }
    break;

  case 45:
#line 325 "mea-compilr2.yacc"
    { (yyval.str) = "!="; }
    break;

  case 46:
#line 326 "mea-compilr2.yacc"
    { (yyval.str) = ">="; }
    break;

  case 47:
#line 327 "mea-compilr2.yacc"
    { (yyval.str) = "<="; }
    break;

  case 48:
#line 328 "mea-compilr2.yacc"
    { (yyval.str) = ">";  }
    break;

  case 49:
#line 329 "mea-compilr2.yacc"
    { (yyval.str) = "<";  }
    break;

  case 50:
#line 333 "mea-compilr2.yacc"
    { char *ptr = malloc(strlen((yyvsp[(1) - (1)].str))+1); strcpy(ptr, (yyvsp[(1) - (1)].str)); (yyval.str) = ptr; free((yyvsp[(1) - (1)].str)); }
    break;

  case 51:
#line 334 "mea-compilr2.yacc"
    { char *ptr = malloc(strlen((yyvsp[(1) - (1)].str))+1); strcpy(ptr, (yyvsp[(1) - (1)].str)); (yyval.str) = ptr; free((yyvsp[(1) - (1)].str)); }
    break;

  case 52:
#line 335 "mea-compilr2.yacc"
    { char *ptr = malloc(strlen((yyvsp[(1) - (1)].str))+1); strcpy(ptr, (yyvsp[(1) - (1)].str)); (yyval.str) = ptr; free((yyvsp[(1) - (1)].str)); }
    break;

  case 53:
#line 336 "mea-compilr2.yacc"
    { char *ptr = malloc(strlen((yyvsp[(1) - (1)].str))+1); strcpy(ptr, (yyvsp[(1) - (1)].str)); (yyval.str) = ptr; free((yyvsp[(1) - (1)].str)); }
    break;

  case 54:
#line 337 "mea-compilr2.yacc"
    { (yyval.str) = (yyvsp[(1) - (1)].str); }
    break;

  case 55:
#line 338 "mea-compilr2.yacc"
    { char *ptr = malloc(strlen((yyvsp[(2) - (3)].str))+3); sprintf(ptr, "{%s}", (yyvsp[(2) - (3)].str)); (yyval.str) = ptr; free((yyvsp[(2) - (3)].str)); }
    break;

  case 56:
#line 342 "mea-compilr2.yacc"
    {
                                                      if((yyvsp[(3) - (4)].str) == NULL)
                                                      {
                                                         char *ptr = (char *)malloc(strlen((yyvsp[(1) - (4)].str))+3);
                                                         strcpy(ptr,(yyvsp[(1) - (4)].str));
                                                         strcat(ptr,"[]");
                                                         (yyval.str) = ptr;
                                                      }
                                                      else
                                                      {
                                                         char *ptr = (char *)malloc(strlen((yyvsp[(1) - (4)].str))+strlen((yyvsp[(3) - (4)].str))+3);
                                                         strcpy(ptr,(yyvsp[(1) - (4)].str));
                                                         strcat(ptr,"[");
                                                         strcat(ptr,(yyvsp[(3) - (4)].str));
                                                         strcat(ptr,"]");
                                                         free((yyvsp[(3) - (4)].str));
                                                         (yyval.str) = ptr;
                                                      }
                                                   }
    break;

  case 57:
#line 364 "mea-compilr2.yacc"
    { (yyval.str) = NULL; }
    break;

  case 58:
#line 365 "mea-compilr2.yacc"
    {
              char *ptr = (char *)malloc(strlen((yyvsp[(1) - (1)].str))+1);
              strcpy(ptr,(yyvsp[(1) - (1)].str));
              free((yyvsp[(1) - (1)].str));
              (yyval.str) = ptr;
           }
    break;

  case 59:
#line 371 "mea-compilr2.yacc"
    {
                                          char *ptr = (char *)realloc((yyvsp[(1) - (3)].str), strlen((yyvsp[(1) - (3)].str))+strlen((yyvsp[(3) - (3)].str))+2);
                                          strcat(ptr,",");
                                          strcat(ptr,(yyvsp[(3) - (3)].str));
                                          free((yyvsp[(3) - (3)].str));
                                          (yyval.str) = ptr;
                                       }
    break;


/* Line 1267 of yacc.c.  */
#line 1890 "y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 379 "mea-compilr2.yacc"



int yyerror(char *s)
{
   printError(jsonerror_flag, 1, current_file, yylineno, line_offset, yytext, s);
}

/*
char ifname[255];
char ofname[255];
char line[65536];
int ret = -1;
*/
pid_t pid=-1;

/*
int indent_flag = 0;
int debug_flag = 0;
int verbose_flag = 0;
int optimize_flag = 0;
char output_file[255] = "";
char rules_path[255] = "";
char rulesset_file[255] = "";
*/
 
int indent(FILE *fd, int nb)
{
   if(indent_flag == 1)
      for(int i=0; i<nb; i++)
         fputs("   ", fd);
}


int space(FILE *fd, int nb)
{
   if(indent_flag == 1)
      for(int i=0; i<nb; i++)
         fputc(' ', fd);
}


int nl(FILE *fd)
{
   if(indent_flag == 1)
      fprintf(fd, "\n");
}


void usage(const char *name)
{
   fprintf(stderr,
      "\n"
      "Usage: %s [options] rules_files\n"
      "\n"
      "Options:\n"
      "  -h, --help            show this help message and exit\n"
      "  -o FILE, --output=FILE\n"
      "                        write result to FILE\n"
      "  -i, --indent          JSON indented output\n"
      "  -v, --verbose         verbose\n"
      "  -d, --debug           debug\n"
      "  -O, --optimize        optimize result code\n"
      "  -s RULESSETFILE, --set=RULESSETFILE\n"
      "                        from rules set\n"
      "-p RULESPATH, --rulespath=RULESPATH\n"
      "                        default rules files path\n"
      "-j, --errorjson         error in json format\n",
      name
   );
   exit(1);
}


void clean_exit(int num)
{
   exit(num);
}


void printError(int errortojson, int num, char *file, int line, int column, char *near, char *text)
{
   char *iserror = "true";
   if(num == 0)
      iserror = "false";

   if(file==NULL)
      file = "<unknown>";

   if(errortojson > 0)
   {
      printf("{ \"iserror\": %s, \"errno\": %d, \"file\": \"%s\", \"line\": %d, \"column\": %d, \"message\": \"%s\" }", iserror, num, file, line, column, text);
   }
   else
   {
     if(num!=0)
        fprintf(stderr, "Error: %s, file \"%s\" line %d, column %d, near \"%s\".\n", text, file, line, column, near);
   }
}


int main(int argc, const char * argv[])
{
   pid = getpid();
   filenum = 0;
   FILE *out = NULL;
   int out_open_flag = 0;
   char errmsg[255];
   char ifname[255];
   char ofname[255];
   char line[65536];

   int ret = -1;

   static struct option long_options[] = {
      {"help",              no_argument,       0,  'h'                  },
      {"indent",            no_argument,       0,  'i'                  },
      {"debug",             no_argument,       0,  'd'                  },
      {"verbose",           no_argument,       0,  'v'                  },
      {"optimize",          no_argument,       0,  'O'                  },
      {"errorjson",         no_argument,       0,  'j'                  },
      {"output",            required_argument, 0,  'o'                  }, // 'o'
      {"set",               required_argument, 0,  's'                  }, // 's'
      {"rulespath",         required_argument, 0,  'p'                  }, // 'p'
      {0,                   0,                 0,  0                    }
   };

   int option_index = 0;
   int c;
   while ((c = getopt_long(argc, (char * const *)argv, "hidvOjo:s:p:", long_options, &option_index)) != -1)
   {
      switch (c)
      {
         case 'h':
            usage(argv[0]);
            break;
         case 'i':
            indent_flag = 1;
            break;
         case 'd':
            debug_flag = 1;
            break;
         case 'v':
            verbose_flag = 1;
            break;
         case 'O':
            optimize_flag = 1; 
            break;
         case 'j':
            jsonerror_flag = 1; 
            break;
         case 'o':
            strcpy(output_file, optarg);
            break;
         case 'p':
            strcpy(rules_path, optarg);
            break;
         case 's':
            strcpy(rulesset_file, optarg);
            break;
         default:
            usage(argv[0]);
            break;
      }
  } 


  // récupération des noms de fichier de la ligne de commandes
  char **files = NULL;
  int nb_files = argc - optind;

  int nextSlot = 0;
  if (nb_files)
  {
     files = malloc(sizeof(char *) * (nb_files+1)); 
     while (optind < argc)
     {
        char *s = NULL;
        if(rules_path[0])
        {
           s = malloc(strlen(argv[optind])+strlen(rules_path)+strlen("/")+1);
           sprintf(s,"%s/%s", rules_path, (char *)argv[optind]);
        }
        else
        {
           s = malloc(strlen(argv[optind])+1);
           strcpy(s, argv[optind]);
        }
        files[nextSlot++]=s;
        optind++;
     }
     files[nextSlot]=NULL;
  }


  // récupération des noms de fichier d'un rulesset
  if(rulesset_file[0])
  {
     #define RS_BLOCKSIZE 200 

     FILE *rsfd = NULL;
     int rs_size = 0;
     char *rs = NULL;
     char *_rs = NULL;
     int  rs_count = 0;
     char rs_buff[21];


     rsfd = fopen(rulesset_file, "r");
     while(!feof(rsfd))
     {
        int nb=fread(rs_buff, 1, sizeof(rs_buff)-1, rsfd);
        rs_buff[nb]=0;
        if((rs_count + nb) >= rs_size)
        {
           rs_size=rs_size+RS_BLOCKSIZE;
           _rs = realloc(rs, rs_size);
           if(!_rs)
           {
              snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't load json file", strerror(errno));
              printError(jsonerror_flag, 254, rulesset_file, 0, 0, "N/A", errmsg);
              clean_exit(1);
           }
           rs = _rs;
        }
        memcpy(&(rs[rs_count]), rs_buff, nb);
        rs_count+=nb;
     } 
     fclose(rsfd);

     rulesSet = cJSON_Parse(rs);

     free(rs);

     if(rulesSet)
     {
        cJSON *e=rulesSet->child;
        while(e)
        {
           char **_files = realloc(files, sizeof(char *) * (nextSlot + 1) + sizeof(char *) );
           if(!_files)
           {
              snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't create rules files list", strerror(errno));
              printError(jsonerror_flag, 253, rulesset_file, 0, 0, "N/A", errmsg);
              clean_exit(1);
           } 
           files=_files;
           char *s = NULL;
           if(rules_path[0])
           {
              s = malloc(strlen(e->valuestring)+strlen(rules_path)+strlen(".srules")+strlen("/")+1);
              sprintf(s, "%s/%s.srules", rules_path, e->valuestring);
           }
           else
           {
              s = malloc(strlen(e->valuestring)+strlen(".srules")+1);
              sprintf(s, "%s.srules", e->valuestring);
           }
           files[nextSlot]=s;
           nextSlot++;
           e=e->next;
        }

        if(files)
           files[nextSlot]=NULL;

        cJSON_Delete(rulesSet);
     }

  }

  if(!files) // pas de fichier trouvé
  {
     printError(jsonerror_flag, 255, "N/A", 0, 0, "N/A", "no input file");
     if(jsonerror_flag == 0)
        usage(argv[0]);
  }


  // récupération du nom de fichier de sortie
  if(output_file[0]==0)
     out = stdout;
  else
  {
     char _output_file[255];
     if(rules_path[0])
        snprintf(_output_file, sizeof(_output_file)-1, "%s/%s", rules_path, output_file);
     else
        strncpy(_output_file, output_file, sizeof(_output_file)-1);
     out = fopen(_output_file, "w");
     if(out == NULL)
     {
        snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't create file", strerror(errno));
        printError(jsonerror_flag, 254, _output_file, 0, 0, "N/A", errmsg);
        clean_exit(1);
     }
     out_open_flag = 1;
  }


  // traitement des fichiers
  for(filenum=0;files[filenum];filenum++)
  {
     if(verbose_flag == 1) fprintf(stderr,"Processing file : %s\n", files[filenum]);
     current_file = NULL;
     yyin=fopen(files[filenum],"r");
     if(yyin==NULL)
     {
        sprintf(errmsg, "%s (%s)", "can't open file", strerror(errno));
        printError(jsonerror_flag, 2, files[filenum], 0, 0, "N/A", errmsg);
        clean_exit(1);
     } 
     current_file = files[filenum];

     sprintf(ifname, "/tmp/input.%d.%d.tmp", filenum, pid);
     sprintf(ofname, "/tmp/output.%d.%d.tmp", filenum, pid);

     fdi = fopen(ifname,"w+");
     if(fdi == NULL)
     {
        snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't create tmp file", strerror(errno));
        printError(jsonerror_flag, 3, ifname, 0, 0, "N/A", errmsg);
        clean_exit(1);
     }

     fdo = fopen(ofname,"w+");
     if(fdo == NULL)
     {
        snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't create tmp file", strerror(errno));
        printError(jsonerror_flag, 4, ofname, 0, 0, "N/A", errmsg);
        clean_exit(1);
     }

     file_offset=0;
     line_offset=0;
     yylineno=1;
     ret = yyparse();

     if(ret != 0)
     {
        clean_exit(1);
     }

     fclose(fdo);
     fdo = NULL;
     fclose(fdi);
     fdi = NULL;
  }


  // constitution du fichier final
  fputc('{', out);
  nl(out);

  // entrees
  indent(out,1); fprintf(out, "\"inputs\":["); nl(out);
  for(filenum=0;files[filenum];filenum++)
  {
     sprintf(ifname, "/tmp/input.%d.%d.tmp", filenum, pid);

     fdi = fopen(ifname, "r");
     if(fdi == NULL)
     {
        snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't open tmp file", strerror(errno));
        printError(jsonerror_flag, 5, ifname, 0, 0, "N/A", errmsg);
        clean_exit(1);
     }
     while(!feof(fdi))
     {
        line[0]=0;
        fgets(line, sizeof(line)-1, fdi);
        fputs(line, out);
     }
     fclose(fdi);

     unlink(ifname); // suppression du fichier temporaire
  }
  nl(out);
  indent(out,1);
  fputc(']', out);


  fputc(',', out);
  nl(out);

  // sorties
  indent(out,1); fprintf(out, "\"outputs\":["); nl(out);
  for(filenum=0;files[filenum];filenum++)
  {
     sprintf(ofname, "/tmp/output.%d.%d.tmp", filenum, pid);
     fdo = fopen(ofname, "r");
     if(fdo == NULL)
     {
        snprintf(errmsg, sizeof(errmsg)-1, "%s (%s)", "can't open tmp file", strerror(errno));
        printError(jsonerror_flag, 5, ofname, 0, 0, "N/A", errmsg);
        clean_exit(1);
     }
     while(!feof(fdo))
     {
        line[0]=0;
        fgets(line, sizeof(line)-1, fdo);
        fputs(line, out);
     }
     unlink(ofname); // suppression du fichier temporaire
  }
  nl(out);
  indent(out,1);
  fputc(']',out);

  // noms des fichiers
  if(files && debug_flag == 1)
  {
     fputc(',', out);
     nl(out);
     indent(out, 1);
     fputs("\"files\":[", out);
     for(int i=0;files[i];i++)
     {
        if(i!=0)
           fputc(',', out);
        nl(out);
        indent(out, 2);
        fprintf(out, "\"%s\"", files[i]);
     
        free(files[i]);
     }
     free(files);
  }
  nl(out);
  indent(out,1);
  fputc(']',out);

  nl(out);
  fputc('}', out);
  nl(out);

  if(out_open_flag == 1)
     fclose(out);

  printError(jsonerror_flag, 0, "N/A", 0, 0, "N/A", "no error");

  exit(0);
}

