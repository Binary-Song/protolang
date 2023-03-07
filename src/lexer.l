%top{
  #include <cstdio>
  #include <iostream>
  #include <iomanip>
  using namespace std;
  enum class Rule
  {
    int_dec,
    int_oct,
    int_hex,
    int_bin,
    fp,
    keyword,
    id,
    op,
    left_paren,
    right_paren,
    semicol,
    str,
    comment,
    err_amb_int,
    err_unknown_char,
    eof,
  };
%}

%class{
public: 
    virtual ~Lexer() = default;
protected:
    virtual int vlex(Rule ruleno) = 0;
%}

%init{
 
%}

%option unicode
%option header-file
%option namespace=protolang_generated

int_dec     0|[1-9][0-9]*
int_oct     0o([0-7]+)
int_hex     0x([0-9a-fA-F]+)
int_bin     0b([01]+)
fp          {int_dec}"."[0-9]+
blank       [ \n\t]+
keyword     "var"|"func"|"struct"|"class"|"return"|"if"|"else"|"while"|"true"|"false"
id          \p{PythonIdentifierStart}\p{PythonIdentifierPart}*
op1         "!"|"+"|"-"|"*"|"/"|"%"|"="|">"|"<"|"&"|"|"
op2         "+="|"-="|"*="|"/="|"%="|"!="|">="|"<="|"=="|"&&"|"||"

dq          \"
sq          '
str_esc     [nts"'\\]
str_sq      {sq}(\\{str_esc}|[^\\])*?{sq}
str_dq      {dq}(\\{str_esc}|[^\\])*?{dq}

comment_sl  "//".*\n
comment_ml  "/*"(.|\n)*?"*/"

err_amb_int 0[0-9]+

%%

{err_amb_int}   return vlex(Rule::err_amb_int);
{int_dec}       return vlex(Rule::int_dec);
{int_oct}       return vlex(Rule::int_oct);
{int_hex}       return vlex(Rule::int_hex);
{int_bin}       return vlex(Rule::int_bin);

{fp}            return vlex(Rule::fp);
{blank}         // do nothing
{keyword}       return vlex(Rule::keyword);
{id}            return vlex(Rule::id);
{op2}           return vlex(Rule::op);
{op1}           return vlex(Rule::op);
"("             return vlex(Rule::left_paren);
")"             return vlex(Rule::right_paren);
";"             return vlex(Rule::semicol);
{comment_sl}    // do nothing
{comment_ml}    // do nothing

.               return vlex(Rule::err_unknown_char);
<<EOF>>         return vlex(Rule::eof);
%%
