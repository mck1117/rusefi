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

// @a = escaped '@'
replacementIdent: '@@' IdentifierChars '@@';

String: [a-zA-Z_0-9.]+;

// match a quote, then anything not a quote, then another quote
QuotedString: '"' ~'"'* '"';

// legacy, remove me!
SemicolonedString: ';' ~';'* ';';

integer: IntegerChars;
floatNum: FloatChars | IntegerChars;

numexpr
    : exprMult ADD numexpr
    | exprMult SUB numexpr
    | exprMult
    ;

exprMult
    : exprAtom MUL exprMult
    | exprAtom DIV exprMult
    | exprAtom
    ;

exprAtom
    : '{' numexpr '}'
    // TODO: do we need replacementIdent AND identifier to be here?
    | replacementIdent
    | identifier
    | floatNum
    ;

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

scalarField: identifier FsioVisible? identifier (fieldOptionsList)?;
arrayField: identifier '[' arrayLengthSpec Iterate? ']' identifier (fieldOptionsList)?;
bitField: Bit identifier ('(' 'comment' ':' QuotedString ')')?;

field
    : scalarField
    | arrayField
    | bitField
    ;

// Indicates X bytes of free space
unusedField: Unused integer;

enumTypedefSuffix: /*ignored*/integer Bits ',' Datatype ',' '@OFFSET@' ',' '[' integer ':' integer ']' ',' definitionRhsMult ;
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
