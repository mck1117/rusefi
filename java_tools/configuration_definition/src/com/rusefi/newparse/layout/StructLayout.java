package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.*;

import java.io.PrintStream;
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.List;

public class StructLayout extends Layout {
    private List<Layout> children = new ArrayList<>();

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
            Layout l = null;

            if (f instanceof StructField) {
                // Special case for structs - we have to compute base offset first
                StructField sf = (StructField) f;

                offset = addStruct(offset, sf.struct, sf.name);
            } /*else if (f instanceof ArrayStructField) {
                ArrayStructField asf = (ArrayStructField)f;

                // TODO: is it valid to have an array of structs without 'iterate' specified?
                assert(asf.iterate);

                // TODO: this only works for TS, not c where we need it to stay an array
                for (int i = 0; i < asf.length; i++) {
                    offset = addStruct(offset, asf.struct, asf.name + (i + 1));
                }
            }*/ else {
                if (f instanceof ScalarField) {
                    l = new ScalarLayout((ScalarField)f);
                } else if (f instanceof ArrayField) {
                    l = new ArrayLayout((ArrayField)f);
                } else if (f instanceof EnumField) {
                    l = new EnumLayout((EnumField)f);
                } else if (f instanceof UnusedField) {
                    l = new UnusedLayout((UnusedField) f);
                } else {
                    // TODO: throw
                }

                // Slide the offset up by the size of this element
                int elementSize = l.getSize();
                offset = padOffsetWithUnused(offset, elementSize);

                // place the element
                l.setOffset(offset);
                children.add(l);

                offset += elementSize;
            }
        }

        // Structs are always a multiple of 4 bytes long, pad the end appropriately
        offset = padOffsetWithUnused(offset, 4);

        size = offset - initialOffest;
    }

    private int addStruct(int offset, Struct struct, String name) {
        offset = padOffsetWithUnused(offset, 4);

        // Recurse and build this new struct
        StructLayout sl = new StructLayout(offset, name, struct);

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
}
