package com.rusefi.newparse.parsing;

public class EnumField extends PrototypeField {
    public final Type type;
    public final String values;

    public EnumField(Type type, String name, String values) {
        super(name);

        this.type = type;
        this.values = values;
    }

    @Override
    public String toString() {
        return "enum " + type.cType + " " + this.name;
    }
}
