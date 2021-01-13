package com.rusefi.newparse.parsing;

public class StringField implements Field{
    public final String name;
    public final int size;

    public StringField(String name, int size) {
        this.name = name;
        this.size = size;
    }
}
