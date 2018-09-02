#pragma once

#include <windows.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include "debug.h"

#include "config.h"
#include "p2p2.h"
#include "transferlog.h"
#include "http.h"
#include "irc.h"
#include "spread.h"
#include "proxy.h"
#include "ddos.h"
#ifndef NO_FTP_SERVER
#include "ftp.h"
#endif
#include "aim.h"
#include "msm.h"
#include "yahoo.h"
#include "triton.h"
//#include "scan.h"
#include "tcptunnel.h"
#include "idletrack.h"

namespace SEL{

enum TokenTypes{TOKEN_NONE, TOKEN_IDENTIFIER, TOKEN_NAMESPACE, TOKEN_VARIABLE, TOKEN_OPERATOR, TOKEN_COMMA, TOKEN_SEMICOLON, TOKEN_OPENPARENTHESIS, TOKEN_CLOSEPARENTHESIS, TOKEN_OPENBRACKET, TOKEN_CLOSEBRACKET, TOKEN_OPENBRACE, TOKEN_CLOSEBRACE, TOKEN_INTEGER, TOKEN_FLOAT, TOKEN_STRING, TOKEN_ERROR, TOKEN_END};
enum ErrorCodes{ERROR_NONE, ERROR_INVALIDVARIABLENAME, ERROR_NOSTRINGTERMINATOR, ERROR_NOCOMMENTCLOSE};
enum VariableTypes{VAR_NONE, VAR_INTEGER, VAR_FLOAT, VAR_STRING};
enum OperatorTypes{OP_UNKNOWN, OP_ASSIGN, OP_ADD, OP_SUBTRACT, OP_INCREMENT, OP_DECREMENT, OP_NEGATIVE, OP_MULTIPLY, OP_DIVIDE, OP_MODULUS, OP_LEFTSHIFT, OP_RIGHTSHIFT, OP_BITAND, OP_BITOR, OP_BITXOR, OP_BITNOT, OP_AND, OP_OR, OP_NOT, OP_EQUALS, OP_GREATERTHAN, OP_LESSTHAN, OP_INEQUALITY, OP_GREATEROREQUAL, OP_LESSOREQUAL};

const int MAX_ARRAY_SIZE = 1024;

class Script;
class Parser;

class Item
{
public:
	UINT Scope;
};

class Variable : public Item
{
public:
	Variable(UINT Type = VAR_NONE);
	~Variable();

public:
	VOID Assign(UINT TokenType, PCHAR Lexeme);
	VOID Assign(Variable & Variable);
	VOID Add(Variable & Variable);
	VOID Subtract(Variable & Variable);
	VOID Multiply(Variable & Variable);
	VOID Divide(Variable & Variable);
	VOID Modulus(Variable & Variable);
	VOID LeftShift(Variable & Variable);
	VOID RightShift(Variable & Variable);
	VOID And(Variable & Variable);
	VOID Or(Variable & Variable);
	VOID Not(VOID);
	VOID Negative(VOID);
	VOID BitNot(VOID);
	VOID Equals(Variable & Variable);
	VOID GreaterThan(Variable & Variable);
	VOID LessThan(Variable & Variable);
	VOID BitAnd(Variable & Variable);
	VOID BitOr(Variable & Variable);
	VOID BitXor(Variable & Variable);

public:
	BOOL IsInteger(VOID);
	BOOL IsFloat(VOID);
	BOOL IsString(VOID);
	BOOL IsNone(VOID);

public:
	VOID SetInteger(INT Value);
	VOID SetFloat(DOUBLE Value);
	VOID SetString(PCHAR Value);

public:
	VOID ToInteger(VOID);
	VOID ToFloat(VOID);
	VOID ToString(VOID);

public:
	VOID AllocateString(UINT Size);
	VOID ResizeString(UINT Size);
	VOID ParseString(VOID);

public:
	BOOL Constant;
	UINT Type;
	union {
		INT64 Integer;
		DOUBLE Float;
		PCHAR String;
	} Data;
};

class Array : public Item
{
public:
	Array(SEL::Parser* Parser);
	Variable* Get(UINT Index);
	VOID Insert(Variable* Variable);

private:
	std::vector<Variable*> VariableList;
	Parser* Parser;
};

class Lexer
{
public:
	Lexer(PCHAR Buffer);
	~Lexer();
	UINT GetNextToken(VOID);
	UINT PeekNextToken(VOID);
	VOID EndPeek(VOID);
	UINT GetError(VOID);
	PCHAR GetLexeme(VOID);
	UINT GetOperatorType(PCHAR Lexeme);
	VOID SetPosition(UINT Character);
	UINT GetPosition(VOID);

private:
	UINT GetTokenType(VOID);
	VOID SetError(UINT Error);

	PCHAR Buffer;
	UINT Size;
	UINT LexemeSize;
	BOOL NullAdded;
	CHAR OldChar;
	BOOL QuoteRemoved;
	UINT QuotePosition;
	UINT Error;
	UINT CurrentCharacter;
	BOOL Peeked;
	UINT OldCurrentCharacter;
	UINT OldLexemeSize;
	UINT TokenOffset;
	BOOL IsInteger;
	BOOL IsFloat;
	BOOL IsVariable;
	BOOL IsString;
	BOOL IsIdentifier;
	BOOL IsNamespace;
};

template <class T>
class ItemTable
{
public:
	~ItemTable();
	BOOL Add(PCHAR Name, T* T);
	T* Get(PCHAR Name, UINT Scope);
	VOID Remove(UINT Scope);

private:
	class ItemList
	{
	public:
		BOOL Add(T* T);
		T* Get(UINT Scope);
		VOID Remove(UINT Scope);

	private:
		std::vector<T*> Items;
	};
	std::vector<PCHAR> ItemNames;
	std::vector<ItemList*> ItemLists;
};

class MasterItemList
{
public:
	VOID Delete(UINT Scope, BOOL All = FALSE);
	VOID Add(PVOID Item);

private:
	std::vector<PVOID> ItemList;
};

class Procedure : public Item
{
public:
	Procedure(PCHAR Name, UINT ParameterCount);
	Variable* Call(std::vector<Variable*>& Parameters, SEL::Script* Script);
	UINT GetParameterCount(VOID);
	CHAR Name[256];

protected:
	virtual VOID Body(std::vector<Variable*>& Parameters) = 0;
	Variable* Return;
	Script* Script;

private:
	UINT ParameterCount;
};

template <class T>
class IdentifierList
{
public:
	IdentifierList(Parser* Parser);
	VOID Add(T* Identifier);
	T* Get(PCHAR Name);

private:
	std::vector<T*> List;
	Parser* Parser;
};

class NameSpace : public Item
{
public:
	NameSpace(PCHAR Name, Parser* Parser);
	CHAR Name[256];
	IdentifierList<Procedure> ProcedureList;
	IdentifierList<NameSpace> NameSpaceList;
};

class Parser
{
public:
	Parser(PCHAR Buffer, SEL::Script* Script);
	~Parser();
	VOID Run(VOID);
	VOID Abort(VOID);

	class Expression
	{
	public:
		Expression(SEL::Parser* Parser);
		Variable* Parse(UINT VariableType);
		VOID Push(UINT TokenType, PCHAR Lexeme, BOOL Constant);
		VOID Push(Variable & Variable, BOOL Constant);
	private:
		class StackObject
		{
		public:
			UINT TokenType;
			CHAR Lexeme[256];
		};
		BOOL CheckValidity(StackObject & StackObject);
		class Operator : public StackObject
		{
		public:
			UINT Type;
		};
		class Term : public StackObject
		{
		public:
			Term() {Variable = NULL;};
			Variable* Variable;
		};
		std::vector<Term> TermStack;
		std::vector<Operator> OperatorStack;
		StackObject LastObject;
		Parser* Parser;
	};
	static UINT GetOperatorType(PCHAR Lexeme);
	static BOOL IsAssignmentOperator(UINT Operator);
	static BOOL IsTerm(UINT TokenType);
	static BOOL IsOperator(UINT TokenType);
	static BOOL IsUnary(UINT TokenType, PCHAR Lexeme);
	static BOOL IsBinary(UINT TokenType, PCHAR Lexeme);
	Variable* ParseStatement(UINT EndToken = TOKEN_SEMICOLON);
	BOOL ParseBlock(VOID);
	BOOL ParseIf(VOID);
	BOOL ParseWhile(VOID);
	VOID SkipStatement(VOID);
	Variable* ParseFunction(Procedure* Procedure);
	VOID SetError(PCHAR Error);
	BOOL CheckForAbort(VOID);

	CHAR Error[256];
	BOOL AbortScript;
	Lexer Lexer;
	PCHAR Buffer;
	UINT CurrentToken;
	UINT CurrentScope;
	NameSpace GlobalNameSpace;
	ItemTable<Array> ArrayTable;
	ItemTable<Variable> VariableTable;
	MasterItemList MasterItemList;
	Script* Script;
};

class Script : public Thread
{
public:
	Script(PCHAR Buffer);
	~Script();
	VOID Run(VOID);
	VOID Abort(VOID);

private:
	VOID ThreadFunc(VOID);
	VOID OnEnd(VOID);
	Parser* Parser;
	PCHAR Buffer;
};

class Scripts
{
public:
	VOID Add(Script* Script);
	VOID Remove(Script* Script);
	VOID AbortAll(Script* Except);

private:
	std::vector<Script*> ScriptList;
	Mutex Mutex;
};

extern class Scripts Scripts;

}