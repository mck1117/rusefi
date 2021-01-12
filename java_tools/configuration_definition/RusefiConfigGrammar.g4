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

IntegerChars: [-]?[0-9]+;

IdentifierChars : [a-zA-Z_]([a-zA-Z0-9_]*);

String: [a-zA-Z_0-9@*]+;

// match a quote, then anything not a quote, then another quote
QuotedString: '"' ~'"'* '"';

integer: IntegerChars;

identifier: IdentifierChars;

definitionRhs: IntegerChars | IdentifierChars | String | QuotedString;
definition: Definition identifier definitionRhs (',' definitionRhs)*;
struct: (Struct | StructNoPrefix) identifier ENDL statements EndStruct;

fieldOption
    : ('min' | 'max' | 'scale' | 'offset' | 'digits') ':' integer
    | ('unit' | 'comment') ':' QuotedString
    ;

fieldOptionsList
    : '(' fieldOption (',' fieldOption)* ')'
    ;

scalarField: identifier identifier (fieldOptionsList)?;
arrayField: identifier '[' definitionRhs Iterate? ']' identifier (fieldOptionsList)?;
bitField: Bit identifier;

field
    : scalarField
    | arrayField
    | bitField
    ;

// Indicates X bytes of free space
unusedField: Unused integer;

bitsTypedefSuffix: definitionRhs Bits ',' Datatype ',' '@OFFSET@' ',' '[' integer ':' integer ']' ',' definitionRhs  ;
scalarTypedefSuffix: definitionRhs Scalar ',' Datatype ',' '@OFFSET@' fieldOptionsList ;
arrayTypedefSuffix: definitionRhs Array ',' Datatype ',' '@OFFSET@' ',' '[' definitionRhs ']' fieldOptionsList;

typedef: Custom identifier (bitsTypedefSuffix | scalarTypedefSuffix | arrayTypedefSuffix);

// Root statement is allowed to appear in the root of the file
rootStatement
    : definition
    | struct
    | typedef
    ;

rootStatements
    : (rootStatement ENDL+)*
    ;

// Statements are allowed to appear inside a struct
statement
    : rootStatement
    | field
    | unusedField
    ;

statements
    : (statement ENDL+)+
    ;

content: rootStatements EOF;
