package com.rusefi.newparse.parsing;

public class ScalarField implements Field {
    public final Type type;
    public final String name;
    public final FieldOptions options;

    public ScalarField(Type type, String name, FieldOptions options) {
        this.type = type;
        this.name = name;
        this.options = options;
    }

    @Override
    public int getSize() {
        return type.size;
    }

    @Override
    public String toString() {
        return type.cType + " " + name;
    }
}
