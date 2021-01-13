package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.EnumField;
import com.rusefi.newparse.parsing.Type;

import java.io.PrintStream;

public class EnumLayout extends Layout {
    private String name;
    private Type type;
    private String values;

    public EnumLayout(EnumField field) {
        this.name = field.name;
        this.type = field.type;
        this.values = field.values;
    }

    @Override
    public int getSize() {
        return this.type.size;
    }

    @Override
    public void writeTunerstudioLayout(PrintStream ps, StructNamePrefixer prefixer) {
        ps.print(prefixer.get(this.name));
        ps.print(" = bits, ");
        ps.print(this.type.tsType);
        ps.print(", ");
        ps.print(this.offset);
        ps.print(", ");

        // TODO: automatically compute number of bits required?
        ps.print("[0:7], ");

        // TODO: where should value define resolution happen?
        ps.print(this.values);

        ps.println();
    }
}
