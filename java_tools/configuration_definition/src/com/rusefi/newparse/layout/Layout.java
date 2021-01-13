package com.rusefi.newparse.layout;

import java.io.PrintStream;

public abstract class Layout {
    public int offset = -1;

    public abstract int getSize();

    public void setOffset(int offset) {
        this.offset = offset;
    }

    @Override
    public String toString() {
        return "offset = " + offset + " size = " + this.getSize();
    }

    public void writeTunerstudioLayout(PrintStream ps, StructNamePrefixer prefixer) {};
}
