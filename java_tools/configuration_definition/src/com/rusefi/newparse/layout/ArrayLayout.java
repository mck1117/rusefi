package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.FieldOptions;
import com.rusefi.newparse.parsing.ScalarField;
import com.rusefi.newparse.parsing.Type;

import java.io.PrintStream;

public class ArrayLayout extends Layout {
    private String name;
    private Type type;
    private FieldOptions options;
    private int length;

    public ArrayLayout(ScalarField prototype, int length) {
        this.name = prototype.name;
        this.options = prototype.options;
        this.type = prototype.type;
        this.length = length;
    }

    @Override
    public int getSize() {
        return this.type.size * this.length;
    }

    @Override
    public int getAlignment() {
        // Arrays only need to be aligned on the size of the element, not the size of the array
        return this.type.size;
    }

    @Override
    public String toString() {
        return "Scalar " + type.cType + " " + super.toString();
    }

    @Override
    public void writeTunerstudioLayout(PrintStream ps, StructNamePrefixer prefixer) {
        ps.print(prefixer.get(this.name));
        ps.print(" = array, ");
        ps.print(this.type.tsType);
        ps.print(", ");
        ps.print(this.offset);
        ps.print(", ");
        ps.print(this.length);
        ps.print(", ");

        options.printTsFormat(ps);

        ps.println();
    }
}
