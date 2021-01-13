package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.BitGroup;
import com.rusefi.newparse.parsing.EnumField;
import com.rusefi.newparse.parsing.Type;
import com.rusefi.newparse.parsing.UnusedField;

import java.io.PrintStream;

public class BitGroupLayout extends Layout {
    private final int size;

    public BitGroupLayout(BitGroup bitGroup) {
        this.size = 4;
    }

    @Override
    public int getSize() {
        return this.size;
    }

    @Override
    public String toString() {
        return "Unused " + super.toString();
    }

    @Override
    public void writeTunerstudioLayout(PrintStream ps, StructNamePrefixer prefixer) {
        ps.println("; bit group " + this.size + " bytes at offset " + this.offset);
    }
}
