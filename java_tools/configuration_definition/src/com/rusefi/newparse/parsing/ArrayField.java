package com.rusefi.newparse.parsing;

public class ArrayField implements Field {
    public final Type type;
    public final String name;
    public final FieldOptions options;
    public final String size;
    public final Boolean iterate;

    public ArrayField(Type type, String name, FieldOptions options, String size, Boolean iterate) {
        this.type = type;
        this.name = name;
        this.options = options;
        this.size = size;
        this.iterate = iterate;
    }

    @Override
    public int getSize() {
        return 0;
    }
}
