/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 51 "mea-compilr2.yacc"
{
   char *str;
}
/* Line 1529 of yacc.c.  */
#line 129 "y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

