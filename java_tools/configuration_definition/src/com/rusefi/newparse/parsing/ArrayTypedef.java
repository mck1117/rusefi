package com.rusefi.newparse.parsing;

public class ArrayTypedef extends Typedef {
    public final FieldOptions options;
    public final Type type;
    public final String arrayLength;

    public ArrayTypedef(String name, String arrayLength, Type type, FieldOptions options) {
        super(name);

        this.arrayLength = arrayLength;
        this.type = type;
        this.options = options;
    }
}
