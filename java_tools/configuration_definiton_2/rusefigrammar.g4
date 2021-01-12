

grammar rusefigrammar;

rusefiConfig : statement* EOF
	;

struct
	: structStart '{' statement* '}'
	;

structStart
	: 'struct'
	| 'struct_no_prefix'
	;

identifier
	: '[A-Za-z0-9]+'
	;

integer
	: '[-]?[0-9]+'
	;

float
	: '[-]?([0-9]*[.])?[0-9]+'
	;

string
	: '[A-Za-z0-9\'_@\.\" ]+'
	;

newline
	: '\r'? '\n'
	;

comment
	: '!.*'
	;

definition
	: '#define' identifier string
	;

// todo fixme
declaration
	: 'custom' identifier string 
	;

statementLine
	: statement newline
	| statement comment newline
	| comment newline
	;

scalarField
	: identifier identifier ';' string ';' '"' string '"' float ',' float ',' float ',' float ',' float
	;

arrayField
	: identifier '[' string string? ']' identifier
	;

statement
	: definition
	| declaration
	| scalarField
	| arrayField
	| struct
	;
