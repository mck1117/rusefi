package com.rusefi.newparse.parsing;

public class EnumField implements Field {
    public final Type type;
    public final String name;
    public final String values;

    public EnumField(Type type, String name, String values) {
        this.type = type;
        this.name = name;
        this.values = values;
    }

    @Override
    public String toString() {
        return "enum " + type.cType + " " + name;
    }
}
