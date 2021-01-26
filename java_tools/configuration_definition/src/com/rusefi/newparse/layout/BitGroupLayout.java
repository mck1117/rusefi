package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.BitGroup;
import com.rusefi.newparse.parsing.EnumField;
import com.rusefi.newparse.parsing.Type;
import com.rusefi.newparse.parsing.UnusedField;

import java.io.PrintStream;
import java.util.List;
import java.util.stream.Collectors;

public class BitGroupLayout extends Layout {
    private final List<String> bits;

    public BitGroupLayout(BitGroup bitGroup) {
        this.bits = bitGroup.bitFields.stream().map(bf -> bf.name).collect(Collectors.toList());
    }

    @Override
    public int getSize() {
        return 4;
    }

    @Override
    public String toString() {
        return "Bit group " + super.toString();
    }

    @Override
    public void writeTunerstudioLayout(PrintStream ps, StructNamePrefixer prefixer) {
        ps.println("; bit group " + this.getSize() + " bytes at offset " + this.offset);
    }

    @Override
    public void writeCLayout(PrintStream ps) {
        // always emit all 32 bits
        for (int i = 0; i < 32; i++) {
            ps.println("\t/**\n\toffset " + this.offsetWithinStruct + " bit " + i + " */");

            if (i < bits.size()) {
                ps.println("\tbool " + bits.get(i) + " : 1;");
            } else {
                // Force pad out all bit groups to a full 32b/4B
                ps.println("\tbool unusedBit_" + this.offsetWithinStruct + "_" + i + " : 1;");
            }
        }
    }
}
