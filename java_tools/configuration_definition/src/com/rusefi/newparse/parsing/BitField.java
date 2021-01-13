package com.rusefi.newparse.parsing;

public class BitField {
    public final String name;

    public BitField(String name) {
        this.name = name;
    }

    @Override
    public String toString() {
        return "BitField: " + this.name;
    }
}
