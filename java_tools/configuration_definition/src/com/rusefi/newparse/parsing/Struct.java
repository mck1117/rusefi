package com.rusefi.newparse.parsing;

import java.io.PrintStream;
import java.util.List;

public class Struct implements Field {
    public final String name;
    public final Boolean noPrefix;
    public final List<Field> fields;

    public Struct(String name, List<Field> fields, boolean noPrefix) {
        this.name = name;
        this.noPrefix = noPrefix;
        this.fields = fields;
    }
}
