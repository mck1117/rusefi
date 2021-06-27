package com.rusefi.newparse.parsing;

public class ScalarField extends PrototypeField {
    public final Type type;
    public final FieldOptions options;
    public final boolean scaled;

    public ScalarField(Type type, String name, FieldOptions options, boolean scaled) {
        super(name);

        this.type = type;
        this.options = options;
        this.scaled = scaled;
    }

    @Override
    public String toString() {
        return type.cType + " " + name;
    }
}
