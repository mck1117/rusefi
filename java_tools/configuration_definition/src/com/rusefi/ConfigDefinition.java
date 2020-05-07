package com.rusefi;

import com.rusefi.output.*;
import com.rusefi.util.IoUtils;
import com.rusefi.util.LazyFile;
import com.rusefi.util.SystemOut;

import java.io.*;
import java.lang.reflect.Array;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.List;

/**
 * (c) Andrey Belomutskiy
 * 1/12/15
 */
@SuppressWarnings("StringConcatenationInsideStringBufferAppend")
public class ConfigDefinition {
    public static final String EOL = "\n";
    public static String MESSAGE;

    public static String TOOL = "(unknown script)";
    private static final String ROM_RAIDER_XML_TEMPLATE = "rusefi_template.xml";
    public static final String KEY_DEFINITION = "-definition";
    private static final String KEY_ROM_INPUT = "-romraider";
    public static final String KEY_TS_DESTINATION = "-ts_destination";
    private static final String KEY_C_DESTINATION = "-c_destination";
    private static final String KEY_C_FSIO_CONSTANTS = "-c_fsio_constants";
    private static final String KEY_C_FSIO_GETTERS = "-c_fsio_getters";
    private static final String KEY_C_FSIO_NAMES = "-c_fsio_names";
    private static final String KEY_C_FSIO_STRING = "-c_fsio_strings";
    private static final String KEY_C_DEFINES = "-c_defines";
    private static final String KEY_WITH_C_DEFINES = "-with_c_defines";
    private static final String KEY_JAVA_DESTINATION = "-java_destination";
    private static final String KEY_ROMRAIDER_DESTINATION = "-romraider_destination";
    private static final String KEY_FIRING = "-firing_order";
    public static final String KEY_PREPEND = "-prepend";
    private static final String KEY_SKIP = "-skip";
    private static final String KEY_ZERO_INIT = "-initialize_to_zero";
    public static boolean needZeroInit = true;
    public static String definitionInputFile = null;

    public static String getGeneratedAutomaticallyTag() {
        return LazyFile.LAZY_FILE_TAG + "ConfigDefinition.jar based on " + TOOL + " ";
    }

    public static void main(String[] args) throws IOException {
        try {
            doJob(args);
        } catch (Throwable e) {
            SystemOut.println(e);
            e.printStackTrace();
            SystemOut.close();
            System.exit(-1);
        }
        SystemOut.close();
    }

    private static void doJob(String[] args) throws IOException {
        if (args.length < 2) {
            SystemOut.println("Please specify\r\n"
                    + KEY_DEFINITION + " x\r\n"
                    + KEY_TS_DESTINATION + " x\r\n"
                    + KEY_C_DESTINATION + " x\r\n"
                    + KEY_JAVA_DESTINATION + " x\r\n"
            );
            return;
        }

        SystemOut.println("Invoked with " + Arrays.toString(args));

        String tsPath = null;
        String destCHeaderFileName = null;
        String destCDefinesFileName = null;
        String destCFsioConstantsFileName = null;
        String destCFsioGettersFileName = null;
        String namesCFileName = null;
        String stringsCFileName = null;
        String javaDestinationFileName = null;
        String romRaiderDestination = null;
        List<String> prependFiles = new ArrayList<>();
        String skipRebuildFile = null;
        String romRaiderInputFile = null;
        CHeaderConsumer.withC_Defines = true;

        for (int i = 0; i < args.length - 1; i += 2) {
            String key = args[i];
            if (key.equals("-tool")) {
                ConfigDefinition.TOOL = args[i + 1];
            } else if (key.equals(KEY_DEFINITION)) {
                definitionInputFile = args[i + 1];
            } else if (key.equals(KEY_TS_DESTINATION)) {
                tsPath = args[i + 1];
            } else if (key.equals(KEY_C_DESTINATION)) {
                destCHeaderFileName = args[i + 1];
            } else if (key.equals(KEY_C_FSIO_GETTERS)) {
                destCFsioGettersFileName = args[i + 1];
            } else if (key.equals(KEY_C_FSIO_STRING)) {
                stringsCFileName = args[i + 1];
            } else if (key.equals(KEY_C_FSIO_NAMES)) {
                namesCFileName = args[i + 1];
            } else if (key.equals(KEY_C_FSIO_CONSTANTS)) {
                destCFsioConstantsFileName = args[i + 1];
            } else if (key.equals(KEY_ZERO_INIT)) {
                needZeroInit = Boolean.parseBoolean(args[i + 1]);
            } else if (key.equals(KEY_WITH_C_DEFINES)) {
                CHeaderConsumer.withC_Defines = Boolean.parseBoolean(args[i + 1]);
            } else if (key.equals(KEY_C_DEFINES)) {
                destCDefinesFileName = args[i + 1];
            } else if (key.equals(KEY_JAVA_DESTINATION)) {
                javaDestinationFileName = args[i + 1];
            } else if (key.equals(KEY_FIRING)) {
                String firingEnumFileName = args[i + 1];
                SystemOut.println("Reading firing from " + firingEnumFileName);
                VariableRegistry.INSTANCE.register("FIRINGORDER", FiringOrderTSLogic.invoke(firingEnumFileName));
            } else if (key.equals(KEY_ROMRAIDER_DESTINATION)) {
                romRaiderDestination = args[i + 1];
            } else if (key.equals(KEY_PREPEND)) {
                prependFiles.add(args[i + 1]);
            } else if (key.equals(KEY_SKIP)) {
                skipRebuildFile = args[i + 1];
            } else if (key.equals("-ts_output_name")) {
                TSProjectConsumer.TS_FILE_OUTPUT_NAME = args[i + 1];
            } else if (key.equals(KEY_ROM_INPUT)) {
                romRaiderInputFile = args[i + 1];
            }
        }

        MESSAGE = getGeneratedAutomaticallyTag() + definitionInputFile + " " + new Date();

        SystemOut.println("Reading definition from " + definitionInputFile);

        String currentMD5 = getDefinitionMD5(definitionInputFile);

        if (skipRebuildFile != null) {
            boolean nothingToDoHere = needToSkipRebuild(skipRebuildFile, currentMD5);
            if (nothingToDoHere) {
                SystemOut.println("Nothing to do here according to " + skipRebuildFile + " hash " + currentMD5);
                return;
            }
        }

        for (String prependFile : prependFiles)
            readPrependValues(prependFile);

        BufferedReader definitionReader = new BufferedReader(new InputStreamReader(new FileInputStream(definitionInputFile), IoUtils.CHARSET.name()));
        ReaderState state = new ReaderState();

        List<ConfigurationConsumer> destinations = new ArrayList<>();
        if (destCHeaderFileName != null) {
            destinations.add(new CHeaderConsumer(destCHeaderFileName));
        }
        if (tsPath != null) {
            CharArrayWriter tsWriter = new CharArrayWriter();
            destinations.add(new TSProjectConsumer(tsWriter, tsPath, state));
        }
        if (javaDestinationFileName != null) {
            destinations.add(new FileJavaFieldsConsumer(state, javaDestinationFileName));
        }

        if (destCFsioConstantsFileName != null || destCFsioGettersFileName != null) {
            destinations.add(new FileFsioSettingsConsumer(state,
                    destCFsioConstantsFileName,
                    destCFsioGettersFileName,
                    namesCFileName,
                    stringsCFileName));
        }

        if (destinations.isEmpty())
            throw new IllegalArgumentException("No destinations specified");
        state.readBufferedReader(definitionReader, destinations);



        if (destCDefinesFileName != null)
            VariableRegistry.INSTANCE.writeDefinesToFile(destCDefinesFileName);

        if (romRaiderDestination != null && romRaiderInputFile != null) {
            String inputFileName = romRaiderInputFile + File.separator + ROM_RAIDER_XML_TEMPLATE;
            processTextTemplate(inputFileName, romRaiderDestination);
        }
        if (skipRebuildFile != null) {
            SystemOut.println("Writing " + currentMD5 + " to " + skipRebuildFile);
            PrintWriter writer = new PrintWriter(new FileWriter(skipRebuildFile));
            writer.write(currentMD5);
            writer.close();
        }
    }

    private static boolean needToSkipRebuild(String skipRebuildFile, String currentMD5) throws IOException {
        if (currentMD5 == null || !(new File(skipRebuildFile).exists()))
            return false;
        String finishedMD5 = new BufferedReader(new FileReader(skipRebuildFile)).readLine();
        return finishedMD5 != null && finishedMD5.equals(currentMD5);
    }

    private static String getDefinitionMD5(String fullFileName) throws IOException {
        File source = new File(fullFileName);
        FileInputStream fileInputStream = new FileInputStream(fullFileName);
        byte content[] = new byte[(int) source.length()];
        if (fileInputStream.read(content) != content.length)
            return "";
        return getMd5(content);
    }

    private static void readPrependValues(String prependFile) throws IOException {
        BufferedReader definitionReader = new BufferedReader(new FileReader(prependFile));
        String line;
        while ((line = definitionReader.readLine()) != null) {
            line = trimLine(line);
            /**
             * we should ignore empty lines and comments
             */
            if (ReaderState.isEmptyDefinitionLine(line))
                continue;
            if (startsWithToken(line, ReaderState.DEFINE)) {
                processDefine(line.substring(ReaderState.DEFINE.length()).trim());
            }

        }
    }

    private static void processTextTemplate(String inputFileName, String outputFileName) throws IOException {
        SystemOut.println("Reading from " + inputFileName);
        SystemOut.println("Writing to " + outputFileName);

        VariableRegistry.INSTANCE.put("generator_message", ConfigDefinition.getGeneratedAutomaticallyTag() + new Date());

        File inputFile = new File(inputFileName);

        BufferedReader fr = new BufferedReader(new FileReader(inputFile));
        LazyFile fw = new LazyFile(outputFileName);

        String line;
        while ((line = fr.readLine()) != null) {
            line = VariableRegistry.INSTANCE.applyVariables(line);
            fw.write(line + ConfigDefinition.EOL);
        }
        fw.close();
    }

    static String trimLine(String line) {
        line = line.trim();
        line = line.replaceAll("\\s+", " ");
        return line;
    }

    static boolean startsWithToken(String line, String token) {
        return line.startsWith(token + " ") || line.startsWith(token + "\t");
    }


    public static String getComment(String comment, int currentOffset) {
        return "\t/**" + EOL + packComment(comment, "\t") + "\t * offset " + currentOffset + EOL + "\t */" + EOL;
    }

    public static String packComment(String comment, String linePrefix) {
        if (comment == null)
            return "";
        if (comment.trim().isEmpty())
            return "";
        String result = "";
        for (String line : comment.split("\\\\n")) {
            result += linePrefix + " * " + line + EOL;
        }
        return result;
    }

    public static int getSize(String s) {
        if (VariableRegistry.INSTANCE.intValues.containsKey(s)) {
            return VariableRegistry.INSTANCE.intValues.get(s);
        }
        return Integer.parseInt(s);
    }

    static void processDefine(String line) {
        int index = line.indexOf(' ');
        String name;
        if (index == -1) {
            name = line;
            line = "";
        } else {
            name = line.substring(0, index);
            line = line.substring(index).trim();
        }
        VariableRegistry.INSTANCE.register(name, line);
    }

    private static String getMd5(byte[] content) {
        try {
            // Static getInstance method is called with hashing MD5
            MessageDigest md = MessageDigest.getInstance("MD5");

            // digest() method is called to calculate message digest
            //  of an input digest() return array of byte
            byte[] messageDigest = md.digest(content);

            // Convert byte array into signum representation
            BigInteger no = new BigInteger(1, messageDigest);

            // Convert message digest into hex value
            String hashtext = no.toString(16);
            while (hashtext.length() < 32) {
                hashtext = "0" + hashtext;
            }
            return hashtext;
        } catch (NoSuchAlgorithmException e) {
            // For specifying wrong message digest algorithms
            throw new RuntimeException(e);
        }
    }
}
