package com.rusefi.newparse.parsing;

public class StructField implements Field {
    public final Struct struct;
    public final String name;

    public StructField(Struct struct, String name) {
        this.struct = struct;
        this.name = name;
    }

    @Override
    public int getSize() {
        return 0;
    }
}
