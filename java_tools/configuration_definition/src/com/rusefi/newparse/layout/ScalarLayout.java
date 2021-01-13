package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.FieldOptions;
import com.rusefi.newparse.parsing.ScalarField;
import com.rusefi.newparse.parsing.Type;

import java.io.PrintStream;

public class ScalarLayout extends Layout {
    private String name;
    private Type type;
    private FieldOptions options;

    public ScalarLayout(ScalarField field) {
        this.name = field.name;
        this.options = field.options;
        this.type = field.type;
    }

    @Override
    public int getSize() {
        return this.type.size;
    }

    @Override
    public String toString() {
        return "Scalar " + type.cType + " " + super.toString();
    }

    @Override
    public void writeTunerstudioLayout(PrintStream ps, StructNamePrefixer prefixer) {
        ps.print(prefixer.get(this.name));
        ps.print(" = scalar, ");
        ps.print(this.type.tsType);
        ps.print(", ");
        ps.print(this.offset);
        ps.print(", ");

        options.printTsFormat(ps);

        ps.println();
    }
}
