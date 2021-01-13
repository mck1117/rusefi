package com.rusefi.newparse.parsing;

public class UnusedField implements Field {
    public final int size;

    public UnusedField(int size) {
        this.size = size;
    }

    @Override
    public int getSize() {
        return this.size;
    }
}
