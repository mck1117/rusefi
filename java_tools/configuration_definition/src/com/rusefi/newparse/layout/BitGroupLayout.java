package com.rusefi.newparse.layout;

import com.rusefi.newparse.parsing.BitGroup;
import com.rusefi.newparse.parsing.EnumField;
import com.rusefi.newparse.parsing.Type;
import com.rusefi.newparse.parsing.UnusedField;

import java.io.PrintStream;
import java.util.List;
import java.util.stream.Collectors;

public class BitGroupLayout extends Layout {
    private class BitLayout {
        public final String name;
        public final String comment;

        public BitLayout(String name, String comment) {
            this.name = name;
            this.comment = comment;
        }
    }

    private final List<BitLayout> bits;

    public BitGroupLayout(BitGroup bitGroup) {
        this.bits = bitGroup.bitFields.stream().map(bf -> new BitLayout(bf.name, bf.comment)).collect(Collectors.toList());
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
            ps.print("\t/**\n\t");

            if (i < bits.size()) {
                BitLayout bit = this.bits.get(i);

                if (bit.comment != null) {
                    ps.println(" * " + bit.comment.replaceAll("[+]", "").replaceAll(";", "").replace("\\n", "\n\t * "));
                    ps.print('\t');
                }

                ps.println("offset " + this.offsetWithinStruct + " bit " + i + " */");
                ps.println("\tbool " + bit.name + " : 1;");
            } else {
                // Force pad out all bit groups to a full 32b/4B
                ps.println("offset " + this.offsetWithinStruct + " bit " + i + " */");
                ps.println("\tbool unusedBit_" + this.offsetWithinStruct + "_" + i + " : 1;");
            }
        }
    }
}
