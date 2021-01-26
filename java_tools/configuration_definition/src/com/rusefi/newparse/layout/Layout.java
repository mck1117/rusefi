package com.rusefi.newparse.layout;

import java.io.PrintStream;

public abstract class Layout {
    public int offset = -1;
    public int offsetWithinStruct = -1;

    public abstract int getSize();
    public int getAlignment() {
        // Default to size
        return this.getSize();
    }

    public void setOffset(int offset) {
        this.offset = offset;
    }

    public void setOffsetWithinStruct(int offset) {
        offsetWithinStruct = offset;
    }

    @Override
    public String toString() {
        return "offset = " + offset + " size = " + this.getSize();
    }

    public void writeTunerstudioLayout(PrintStream ps, StructNamePrefixer prefixer) {}

    protected void writeCOffsetHeader(PrintStream ps, String comment) {
        ps.println("\t/**");

        if (comment != null) {
            comment = comment.replaceAll(";", "");
            comment = comment.replaceAll("[+]", "");
            if (comment.length() == 0) {
                comment = null;
            }
        }

        if (comment != null) {
            comment = comment.replaceAll("\\\\n", "\n\t * ");

            ps.println("\t * " + comment);
        }
        ps.println("\t * offset " + this.offsetWithinStruct);
        ps.println("\t */");
    }

    public void writeCLayout(PrintStream ps) {
        ps.println("xx");

    }
}
