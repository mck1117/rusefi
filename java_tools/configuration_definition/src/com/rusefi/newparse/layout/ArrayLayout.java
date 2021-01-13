package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.ArrayField;
import com.rusefi.newparse.parsing.FieldOptions;
import com.rusefi.newparse.parsing.Type;

import java.io.PrintStream;

public class ArrayLayout extends Layout {
    public final Type type;
    public final String name;
    public final String arrayDimension;
    public final FieldOptions options;

    public ArrayLayout(ArrayField field) {
        this.type = field.type;
        this.name = field.name;
        this.options = field.options;
        this.arrayDimension = field.size;
    }

    @Override
    public int getSize() {
        return type.size;
    }

    @Override
    public void writeTunerstudioLayout(PrintStream ps, StructNamePrefixer prefixer) {
        ps.print(prefixer.get(this.name));
        ps.print(" = array, ");
        ps.print(this.type.tsType);
        ps.print(", ");
        ps.print(this.offset);
        ps.print(", ");
        ps.print("[" + this.arrayDimension + "]");
        ps.print(", ");

        options.printTsFormat(ps);

        ps.println();
    }
}
