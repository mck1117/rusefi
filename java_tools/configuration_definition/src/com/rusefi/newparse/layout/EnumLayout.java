package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.EnumField;
import com.rusefi.newparse.parsing.Type;

import java.io.PrintStream;

public class EnumLayout extends Layout {
    private final String name;
    private final Type type;
    private final String enumType;
    private final String values;

    public EnumLayout(EnumField field) {
        this.name = field.name;
        this.type = field.type;
        this.enumType = field.enumType;
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

    @Override
    public void writeCLayout(PrintStream ps) {
        this.writeCOffsetHeader(ps, null);
        ps.println("\t" + this.enumType + " " + this.name + ";");
    }
}
