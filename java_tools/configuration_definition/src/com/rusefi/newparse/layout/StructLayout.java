package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.*;

import java.io.PrintStream;
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.List;

public class StructLayout extends Layout {
    /*private*/public List<Layout> children = new ArrayList<>();

    private final String typeName;
    private final String name;
    private final Boolean noPrefix;
    private final int size;

    private static int getAlignedOffset(int offset, int alignment) {
        // Align each element to its own size
        if ((offset % alignment) != 0) {
            return offset + alignment - (offset % alignment);
        } else {
            return offset;
        }
    }

    int padOffsetWithUnused(int offset, int align) {
        int alignedOffset = getAlignedOffset(offset, align);

        int needsUnused = alignedOffset - offset;

        if (needsUnused > 0) {
            UnusedLayout ul = new UnusedLayout(needsUnused);
            ul.setOffset(offset);
            ul.setOffsetWithinStruct(offset - this.offset);
            children.add(ul);
            return alignedOffset;
        }

        return offset;
    }

    public StructLayout(int offset, String name, Struct parsedStruct) {
        setOffset(offset);

        this.typeName = parsedStruct.name;
        this.name = name;
        this.noPrefix = parsedStruct.noPrefix;

        int initialOffest = offset;

        for (Field f : parsedStruct.fields) {
            if (f instanceof ArrayField) {
                ArrayField asf = (ArrayField)f;

                if (asf.iterate) {

                    // TODO: this only works for TS, not c where we need it to stay an array
                    for (int i = 0; i < asf.length; i++) {
                        offset = addItem(offset, asf.prototype);
                        //offset = addStruct(offset, asf.struct, asf.name + (i + 1));
                    }
                } else /* !iterate */ {
                    // If not a scalar, you must iterate
                    assert(asf.prototype instanceof ScalarField);

                    ScalarField prototype = (ScalarField)asf.prototype;

                    offset = addItem(offset, new ArrayLayout(prototype, asf.length));
                }
            } else {
                offset = addItem(offset, f);
            }
        }

        // Structs are always a multiple of 4 bytes long, pad the end appropriately
        offset = padOffsetWithUnused(offset, 4);

        size = offset - initialOffest;
    }

    private int addItem(int offset, Field f) {
        if (f instanceof StructField) {
            // Special case for structs - we have to compute base offset first
            StructField sf = (StructField) f;

            return addStruct(offset, sf.struct, sf.name);
        }

        Layout l = null;
        if (f instanceof ScalarField) {
            l = new ScalarLayout((ScalarField)f);
        } else if (f instanceof EnumField) {
            l = new EnumLayout((EnumField)f);
        } else if (f instanceof UnusedField) {
            l = new UnusedLayout((UnusedField) f);
        } else if (f instanceof BitGroup) {
            l = new BitGroupLayout((BitGroup) f);
        } else if (f instanceof StringField) {
            l = new StringLayout((StringField) f);
        } else {
            throw new RuntimeException("unexpected field type during layout");
            // TODO: throw
        }

        return addItem(offset, l);
    }

    private int addItem(int offset, Layout l) {
        // Slide the offset up by the required alignment of this element
        offset = padOffsetWithUnused(offset, l.getAlignment());

        // place the element
        l.setOffset(offset);
        l.setOffsetWithinStruct(offset - this.offset);
        children.add(l);

        return offset + l.getSize();
    }

    private int addStruct(int offset, Struct struct, String name) {
        offset = padOffsetWithUnused(offset, 4);

        // Recurse and build this new struct
        StructLayout sl = new StructLayout(offset, name, struct);

        sl.setOffsetWithinStruct(offset - this.offset);
        this.children.add(sl);

        // Update offset with the struct size - it's guaranteed to be a multiple of 4 bytes
        int structSize = sl.getSize();
        return offset + structSize;
    }

    @Override
    public int getSize() {
        return this.size;
    }

    @Override
    public String toString() {
        return "Struct " + this.typeName + " " + super.toString();
    }


    @Override
    public void writeTunerstudioLayout(PrintStream ps, StructNamePrefixer prefixer) {
        ps.println("; start struct " + this.typeName);

        if (!this.noPrefix) {
            prefixer.push(this.name);
        }

        // print all children in sequence
        this.children.forEach(c -> c.writeTunerstudioLayout(ps, prefixer));

        if (!this.noPrefix) {
            prefixer.pop();
        }

        ps.println("; end struct " + this.typeName);
    }

    @Override
    public void writeCLayout(PrintStream ps) {
        this.writeCOffsetHeader(ps, null);
        ps.println("\t" + this.typeName + " " + this.name + ";");
    }

    public void writeCLayoutRoot(PrintStream ps) {
        ps.println("struct " + this.typeName + " {");

        this.children.forEach(c -> c.writeCLayout(ps));

        ps.println("\t/** total size " + getSize() + " */");
        ps.println("};");
        ps.println();
        ps.println("typedef struct " + this.typeName + " " + this.typeName + ";");
    }
}
