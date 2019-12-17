package com.rusefi.output;

import com.rusefi.*;
import com.rusefi.util.LazyFile;
import com.rusefi.util.SystemOut;

import java.io.IOException;

import static com.rusefi.ConfigDefinition.EOL;

/**
 * Configuration consumer which writes C header file
 */
public class CHeaderConsumer implements ConfigurationConsumer {
    public static final String BOOLEAN_TYPE = "bool";
    public static boolean withC_Defines;
    private final LazyFile cHeader;
    private final StringBuilder content = new StringBuilder();

    public CHeaderConsumer(String destCHeader) {
        SystemOut.println("Writing C header to " + destCHeader);
        cHeader = new LazyFile(destCHeader);
        cHeader.write("// this section " + ConfigDefinition.MESSAGE + EOL);
        cHeader.write("// by " + getClass() + EOL);
        cHeader.write("// begin" + EOL);
        String id = destCHeader.replaceAll("[\\\\\\.\\/]", "_").toUpperCase();
        cHeader.write("#ifndef " + id + EOL);
        cHeader.write("#define " + id + EOL);
        cHeader.write("#include \"rusefi_types.h\"" + EOL);
    }

    public static String getHeaderText(ConfigField configField, int currentOffset, int bitIndex) {
        if (configField.isBit()) {
            String comment = "\t/**" + EOL + ConfigDefinition.packComment(configField.getCommentContent(), "\t") + "\toffset " + currentOffset + " bit " + bitIndex + " */" + EOL;
            return comment + "\t" + BOOLEAN_TYPE + " " + configField.getName() + " : 1;" + EOL;
        }

        String cEntry = ConfigDefinition.getComment(configField.getCommentContent(), currentOffset);

        if (configField.getArraySize() == 1) {
            // not an array
            cEntry += "\t" + configField.getType() + " " + configField.getName();
            if (ConfigDefinition.needZeroInit && TypesHelper.isPrimitive(configField.getType())) {
                // we need this cast in case of enums
                cEntry += " = (" + configField.getType() + ")0";
            }
            cEntry += ";" + EOL;
        } else {
            cEntry += "\t" + configField.getType() + " " + configField.getName() + "[" + configField.arraySizeVariableName + "];" + EOL;
        }
        return cEntry;
    }

    @Override
    public void startFile() {
    }

    @Override
    public void handleEndStruct(ConfigStructure structure) throws IOException {
        if (structure.comment != null) {
            content.append("/**" + EOL + ConfigDefinition.packComment(structure.comment, "")  + EOL + "*/" + EOL);
        }

        content.append("// start of " + structure.name + EOL);
        content.append("struct " + structure.name + " {" + EOL);
        if (structure.isWithConstructor()) {
            content.append("\t" + structure.name + "();" + EOL);
        }

        int currentOffset = 0;

        BitState bitState = new BitState();
        for (int i = 0; i < structure.cFields.size(); i++) {
            ConfigField cf = structure.cFields.get(i);
            content.append(getHeaderText(cf, currentOffset, bitState.get()));
            ConfigField next = i == structure.cFields.size() - 1 ? ConfigField.VOID : structure.cFields.get(i + 1);

            bitState.incrementBitIndex(cf, next);
            currentOffset += cf.getSize(next);
        }

        content.append("\t/** total size " + currentOffset + "*/" + EOL);
        content.append("};" + EOL + EOL);

        // https://stackoverflow.com/questions/1675351/typedef-struct-vs-struct-definitions
        content.append("typedef struct " + structure.name + " " + structure.name + ";" + EOL + EOL);
    }

    @Override
    public void endFile() throws IOException {
        if (withC_Defines)
            cHeader.write(VariableRegistry.INSTANCE.getDefinesSection());
        cHeader.write(content.toString());
        cHeader.write("#endif" + EOL);
        cHeader.write("// end" + EOL);
        cHeader.write("// this section " + ConfigDefinition.MESSAGE + EOL);
        cHeader.close();
    }
}
