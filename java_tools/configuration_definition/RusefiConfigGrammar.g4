grammar RusefiConfigGrammar;

@header {
	package com.rusefi.generated;
}

// ...be generous in line endings...
ENDL: ('\n' | '\r\n' | '\r');

LINE_COMMENT: '!' ~[\r\n]* -> skip;
WS: [ \t]+ -> skip ;

// Special tokens need highest priority
Struct: 'struct';
StructNoPrefix: 'struct_no_prefix';
EndStruct: 'end_struct';
Definition: '#define';
Unused: 'unused';
Custom: 'custom';
Datatype: (('S'|'U')('08'|'16'|'32')) | 'F32';
Iterate: 'iterate';
Bits: 'bits';
Bit: 'bit';
Array: 'array';
Scalar: 'scalar';
FsioVisible: 'fsio_visible';

ArrayDimensionSeparator: 'x';

MUL: '*';
DIV: '/';
ADD: '+';
SUB: '-';

IntegerChars: [-]?[0-9]+;
FloatChars: IntegerChars [.] ([0-9]+)?;

IdentifierChars : [a-zA-Z_]([a-zA-Z0-9_]*);

// TODO: do we need replacementIdent AND identifier to be here?
replacementIdent: '@@' IdentifierChars '@@' | identifier;

String: [a-zA-Z_0-9.]+;

// match a quote, then anything not a quote, then another quote
QuotedString: '"' ~'"'* '"';

// legacy, remove me!
SemicolonedString: ';' ~([;] | '\n')* ';';

integer: IntegerChars;
floatNum: FloatChars | IntegerChars;

expr
    : floatNum			# EvalNumber
    | '{' expr '}'		# EvalParens
    | expr MUL expr		# EvalMul
	| expr DIV expr		# EvalDiv
	| expr ADD expr		# EvalAdd
	| expr SUB expr		# EvalSub
    | replacementIdent	# EvalReplacement
    ;

numexpr: expr;

identifier: IdentifierChars | 'offset' | 'ArrayDimension';

definitionRhs: identifier | numexpr | String | QuotedString;
definitionRhsMult: definitionRhs (',' definitionRhs)*;
definition: Definition identifier definitionRhsMult;
struct: (Struct | StructNoPrefix) identifier ENDL+ statements EndStruct;

fieldOption
    : ('min' | 'max' | 'scale' | 'offset' | ) ':' numexpr
    | 'digits' ':' integer
    | ('unit' | 'comment') ':' QuotedString
    ;

fieldOptionsList
    : '(' fieldOption (',' fieldOption)* ')'
    | /* legacy! */ (',' | SemicolonedString)  QuotedString ',' numexpr ',' numexpr ',' numexpr ',' numexpr ',' /*digits =*/integer
    ;

arrayLengthSpec: numexpr (ArrayDimensionSeparator numexpr)?;

scalarField: identifier FsioVisible? identifier SemicolonedString? (fieldOptionsList)?;
arrayField: identifier '[' arrayLengthSpec Iterate? ']' identifier SemicolonedString? (fieldOptionsList)?;
bitField: Bit identifier ('(' 'comment' ':' QuotedString ')')?;

field
    : scalarField
    | arrayField
    | bitField
    ;

// Indicates X bytes of free space
unusedField: Unused integer;

enumRhs
    : replacementIdent
    | QuotedString (',' QuotedString)*
    ;

enumTypedefSuffix: /*ignored*/integer Bits ',' Datatype ',' '@OFFSET@' ',' '[' integer ':' integer ']' ',' enumRhs ;
scalarTypedefSuffix: /*ignored*/integer Scalar ',' Datatype ',' '@OFFSET@' fieldOptionsList ;
arrayTypedefSuffix: /*ignored*/arrayLengthSpec Array ',' Datatype ',' '@OFFSET@' ',' '[' arrayLengthSpec ']' fieldOptionsList;

typedef: Custom identifier (enumTypedefSuffix | scalarTypedefSuffix | arrayTypedefSuffix);

// Root statement is allowed to appear in the root of the file
rootStatement
    : ENDL
    | definition
    | struct
    | typedef
    ;

rootStatements
    : (rootStatement ENDL+)*
    ;

// Statements are allowed to appear inside a struct
statement
    : rootStatement
    | field /* tolerate trailing semicolon */ (';')?
    | unusedField
    ;

statements
    : (statement ENDL+)+
    ;

content: rootStatements EOF;
