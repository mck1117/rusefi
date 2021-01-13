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

IntegerChars: [-]?[0-9]+;

IdentifierChars : [a-zA-Z_]([a-zA-Z0-9_]*);

String: [a-zA-Z_0-9@*\.]+;

// match a quote, then anything not a quote, then another quote
QuotedString: '"' ~'"'* '"';

integer: IntegerChars;

identifier: IdentifierChars | 'offset';

definitionRhs: IntegerChars | IdentifierChars | String | QuotedString;
definitionRhsMult: definitionRhs (',' definitionRhs)*;
definition: Definition identifier definitionRhsMult;
struct: (Struct | StructNoPrefix) identifier ENDL+ statements EndStruct;

fieldOption
    : ('min' | 'max' | 'scale' | 'offset' | ) ':' integer
    | 'digits' ':' integer
    | ('unit' | 'comment') ':' QuotedString
    ;

fieldOptionsList
    : '(' fieldOption (',' fieldOption)* ')'
    ;

arrayLengthSpec: definitionRhs;

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
arrayTypedefSuffix: /*ignored*/definitionRhs Array ',' Datatype ',' '@OFFSET@' ',' '[' arrayLengthSpec ']' fieldOptionsList;

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
    | field
    | unusedField
    ;

statements
    : (statement ENDL+)+
    ;

content: rootStatements EOF;
