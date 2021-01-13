package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.EnumField;
import com.rusefi.newparse.parsing.Type;
import com.rusefi.newparse.parsing.UnusedField;

import java.io.PrintStream;

public class UnusedLayout extends Layout {
    private final int size;

    public UnusedLayout(int size) {
        this.size = size;
    }

    public UnusedLayout(UnusedField field) {
        this.size = field.size;
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
        ps.println("; unused " + this.size + " bytes at offset " + this.offset);
    }
}
