package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.StringField;
import com.rusefi.newparse.parsing.UnusedField;

import java.io.PrintStream;

public class StringLayout extends Layout {
    private final String name;
    private final int size;

    public StringLayout(StringField field) {
        this.name = field.name;
        this.size = field.size;
    }

    @Override
    public int getSize() {
        return this.size;
    }

    @Override
    public int getAlignment() {
        // char can be single aligned
        return 1;
    }

    @Override
    public String toString() {
        return "Unused " + super.toString();
    }

    @Override
    public void writeTunerstudioLayout(PrintStream ps, StructNamePrefixer prefixer) {
        ps.print(prefixer.get(this.name));
        ps.print(" = scalar, ASCII, ");
        ps.print(this.offset);
        ps.print(", ");
        ps.print(size);

        ps.println();
    }
}
