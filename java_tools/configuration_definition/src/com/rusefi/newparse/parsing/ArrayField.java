package com.rusefi.newparse.parsing;

public class ArrayField<PrototypeType extends PrototypeField> implements Field {
    public final int[] length;
    public final boolean iterate;
    public final PrototypeType prototype;

    public ArrayField(PrototypeType prototype, int[] length, boolean iterate) {
        this.length = length;
        this.iterate = iterate;
        this.prototype = prototype;
    }
}
