package com.rusefi.newparse;

import com.rusefi.generated.RusefiConfigGrammarBaseListener;
import com.rusefi.generated.RusefiConfigGrammarParser;
import com.rusefi.newparse.parsing.*;

import java.util.*;
import java.util.stream.Collectors;

public class ParseListener extends RusefiConfigGrammarBaseListener {
    List<Definition> definitions = new ArrayList<>();
    Map<String, Struct> structs = new HashMap<>();
    Map<String, Typedef> typedefs = new HashMap<>();

    class Scope {
        public List<Field> structFields = new ArrayList<>();
    }

    Scope scope = null;
    Stack<Scope> scopes = new Stack<>();

    String mergeDefinitionRhsMult(RusefiConfigGrammarParser.DefinitionRhsMultContext ctx) {
        return ctx.definitionRhs().stream().map(d -> d.getText()).collect(Collectors.joining(", "));
    }

    void decodeFieldOptions(FieldOptions options, List<RusefiConfigGrammarParser.FieldOptionContext> ctx) {

    }

    @Override
    public void enterDefinition(RusefiConfigGrammarParser.DefinitionContext ctx) {
        String name = ctx.identifier().getText();
        // glue the list of definitions back together
        String value = mergeDefinitionRhsMult(ctx.definitionRhsMult());

        definitions.add(new Definition(name, value));
    }

    String typedefName = null;

    @Override
    public void enterTypedef(RusefiConfigGrammarParser.TypedefContext ctx) {
        this.typedefName = ctx.identifier().getText();
    }

    @Override
    public void exitTypedef(RusefiConfigGrammarParser.TypedefContext ctx) {
        this.typedefName = null;
    }

    @Override
    public void enterScalarTypedefSuffix(RusefiConfigGrammarParser.ScalarTypedefSuffixContext ctx) {
        Type datatype = Type.findByTsType(ctx.Datatype().getText());

        FieldOptions options = new FieldOptions();
        handleFieldOptionsList(options, ctx.fieldOptionsList());

        this.typedefs.put(this.typedefName, new ScalarTypedef(this.typedefName, datatype, options));
    }

    @Override
    public void enterEnumTypedefSuffix(RusefiConfigGrammarParser.EnumTypedefSuffixContext ctx) {
        int startBit = Integer.parseInt(ctx.integer(1).getText());
        int endBit = Integer.parseInt(ctx.integer(2).getText());
        Type datatype = Type.findByTsType(ctx.Datatype().getText());

        String values = mergeDefinitionRhsMult(ctx.definitionRhsMult());

        this.typedefs.put(this.typedefName, new EnumTypedef(this.typedefName, datatype, startBit, endBit, values));
    }

    @Override
    public void enterArrayTypedefSuffix(RusefiConfigGrammarParser.ArrayTypedefSuffixContext ctx) {
        String arrayLength = ctx.arrayLengthSpec(1).getText();

        Type datatype = Type.findByTsType(ctx.Datatype().getText());

        FieldOptions options = new FieldOptions();
        handleFieldOptionsList(options, ctx.fieldOptionsList());

        this.typedefs.put(this.typedefName, new ArrayTypedef(this.typedefName, arrayLength, datatype, options));
    }

    @Override
    public void enterStruct(RusefiConfigGrammarParser.StructContext ctx) {
        // If we're already inside a struct, push that context on to the stack
        if (scope != null) {
            scopes.push(scope);
        }

        // Create new scratch space for this scope
        scope = new Scope();
    }

    void handleFieldOptionsList(FieldOptions options, RusefiConfigGrammarParser.FieldOptionsListContext ctx) {
        // Null means no options were configured, use defaults
        if (ctx == null) {
            return;
        }

        // this is a legacy field option list, parse it as such
        if (!ctx.numexpr().isEmpty()) {
            options.units = ctx.QuotedString().getText();
            options.scale = Float.parseFloat(ctx.numexpr(0).getText());
            options.offset = Float.parseFloat(ctx.numexpr(1).getText());
            options.min = Float.parseFloat(ctx.numexpr(2).getText());
            options.max = Float.parseFloat(ctx.numexpr(3).getText());
            options.digits = Integer.parseInt(ctx.integer().getText());
            return;
        }

        for (RusefiConfigGrammarParser.FieldOptionContext fo : ctx.fieldOption()) {
            String key = fo.getChild(0).getText();
            String value = fo.getChild(2).getText();

            switch (key) {
                case "min": options.min = Integer.parseInt(value); break;
                case "max": options.max = Integer.parseInt(value); break;
                case "scale": options.scale = Float.parseFloat(value); break;
                case "offset": options.offset = Float.parseFloat(value); break;
                case "digits": options.digits = Integer.parseInt(value); break;
                case "unit": options.units = value; break;
                case "comment": options.comment = value; break;
            }
        }
    }

    @Override
    public void enterScalarField(RusefiConfigGrammarParser.ScalarFieldContext ctx) {
        String type = ctx.identifier(0).getText();
        String name = ctx.identifier(1).getText();

        // First check if this is an instance of a struct
        if (structs.containsKey(type)) {
            scope.structFields.add(new StructField(structs.get(type), name));
            return;
        }

        // Check first if we have a typedef for this type
        Typedef typedef = this.typedefs.get(type);

        FieldOptions options = null;
        if (typedef != null) {
            if (typedef instanceof ScalarTypedef) {
                ScalarTypedef scTypedef = (ScalarTypedef)typedef;
                // Copy the typedef's options list - we don't want to edit it
                options = scTypedef.options.copy();
                // Switch to the "real" type, that is the typedef's type
                type = scTypedef.type.cType;
            } else if (typedef instanceof ArrayTypedef) {
                ArrayTypedef arTypedef = (ArrayTypedef) typedef;
                // Copy the typedef's options list - we don't want to edit it
                options = arTypedef.options.copy();

                // Merge the read-in options list with the default from the typedef (if exists)
                handleFieldOptionsList(options, ctx.fieldOptionsList());

                scope.structFields.add(new ArrayField(arTypedef.type, name, options, arTypedef.arrayLength, false));
                return;
            } else if (typedef instanceof EnumTypedef) {
                EnumTypedef bTypedef = (EnumTypedef)typedef;

                scope.structFields.add(new EnumField(bTypedef.type, name, bTypedef.values));
                return;
            } else {

                // TODO: throw
            }
        } else {
            // no typedef found, create new options list
            options = new FieldOptions();
        }

        // Merge the read-in options list with the default from the typedef (if exists)
        handleFieldOptionsList(options, ctx.fieldOptionsList());

        scope.structFields.add(new ScalarField(Type.findByCtype(type), name, options));
    }

    @Override
    public void enterBitField(RusefiConfigGrammarParser.BitFieldContext ctx) {
        String name = ctx.identifier().getText();

        // Check if there's already a bit group at the end of the current struct
        BitGroup group = null;
        if (!scope.structFields.isEmpty()) {
            Object lastElement = scope.structFields.get(scope.structFields.size() - 1);

            if (lastElement instanceof BitGroup) {
                group = (BitGroup)lastElement;
            }
        }

        // there was no group, create and add it
        if (group == null) {
            group = new BitGroup();
            scope.structFields.add(group);
        }

        // TODO: read comment off the end of the bit field
        group.addBitField(new BitField(name));
    }

    @Override
    public void enterArrayField(RusefiConfigGrammarParser.ArrayFieldContext ctx) {
        String type = ctx.identifier(0).getText();
        String name = ctx.identifier(1).getText();
        String length = ctx.arrayLengthSpec().getText();
        // check if the iterate token is present
        boolean iterate = ctx.Iterate() != null;


        // Check first if we have a typedef for this type
        Typedef typedef = this.typedefs.get(type);

        FieldOptions options = null;
        if (typedef != null) {
            if (typedef instanceof ScalarTypedef) {
                ScalarTypedef scTypedef = (ScalarTypedef)typedef;
                // Copy the typedef's options list - we don't want to edit it
                options = scTypedef.options.copy();
                // Switch to the "real" type, that is the typedef's type
                type = scTypedef.type.cType;
            } else {
                // TODO: throw
            }
        } else {
            // no typedef found, create new options list
            options = new FieldOptions();
        }

        // Merge the read-in options list with the default from the typedef (if exists)
        handleFieldOptionsList(options, ctx.fieldOptionsList());

        scope.structFields.add(new ArrayField(Type.findByCtype(type), name, options, length, iterate));
    }

    @Override
    public void enterUnusedField(RusefiConfigGrammarParser.UnusedFieldContext ctx) {
        scope.structFields.add(new UnusedField(Integer.parseInt(ctx.integer().getText())));
    }

    private Struct lastStruct = null;

    public Struct getLastStruct() {
        return lastStruct;
    }

    @Override
    public void exitStruct(RusefiConfigGrammarParser.StructContext ctx) {
        String structName = ctx.identifier().getText();

        assert(scope != null);
        assert(scope.structFields != null);

        Struct s = new Struct(structName, scope.structFields, ctx.StructNoPrefix() != null);
        structs.put(structName, s);
        lastStruct = s;

        // We're leaving with this struct, re-apply the next struct out so more fields can be added to it
        if (scopes.empty()) {
            scope = null;
        } else {
            scope = scopes.pop();
        }
    }
}
