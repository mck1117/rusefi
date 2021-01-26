package com.rusefi.newparse.parsing;

public class EnumField extends PrototypeField {
    public final Type type;
    public final String enumType;
    public final String values;

    public EnumField(Type type, String enumType, String name, String values) {
        super(name);

        this.type = type;
        this.enumType = enumType;
        this.values = values;
    }

    @Override
    public String toString() {
        return "enum " + type.cType + " " + this.name;
    }
}
