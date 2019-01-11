package com.opensr5;

import com.opensr5.io.IniFileMetaInfo;
import com.opensr5.io.IniFileReader;
import com.opensr5.io.RawIniFile;

import java.io.File;

public class Main {

    public static void main(String[] args) {
        System.out.println("OpenSR5 - load/program tune via serial port utility");
        System.out.println("  (c) Andrey Belomutskiy 2013-2018");
        System.out.println("  https://github.com/rusefi/opensr5_flash");
        System.out.flush();

        if (args.length < 1) {
            System.err.println("Usage:");
            System.err.println("opensr5 DEFINITION_FILE.INI");
            System.exit(-1);
        }

        String projectIniFileName = args[0];

        RawIniFile content = IniFileReader.read(new File(projectIniFileName));

        IniFileMetaInfo meta = new IniFileMetaInfo(content);

        System.out.println("nPages = " + meta.getnPages());
        System.out.println("blockingFactor = " + meta.getBlockingFactor());

    }
}
