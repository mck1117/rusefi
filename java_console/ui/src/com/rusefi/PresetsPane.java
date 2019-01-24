package com.rusefi;

import com.rusefi.io.CommandQueue;
import org.jetbrains.annotations.NotNull;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionListener;

/**
 * (c) Andrey Belomutskiy 2013-2019
 */
public class PresetsPane {
    // see rusefi_enums.h
    private static final int MIATA_NA_1_6 = 41;
    private static final int MAZDA_MIATA_2003 = 47;

    private JPanel content = new JPanel(new GridLayout(2, 4));

    public PresetsPane() {
        content.add(new SetEngineTypeCommandControl("Miata NA6", "engines/miata_na.png", MIATA_NA_1_6).getContent());
        content.add(new SetEngineTypeCommandControl("Miata NB2", "engines/miata_nb.png", MAZDA_MIATA_2003).getContent());
    }

    public JPanel getContent() {
        return content;
    }

    private class SetEngineTypeCommandControl extends FixedCommandControl {
        private final String labelTest;

        public SetEngineTypeCommandControl(String labelTest, String imageFileName, int engineType) {
            super(labelTest, imageFileName, CommandControl.SET, "set engine_type " + engineType);
            this.labelTest = labelTest;
        }

        @NotNull
        @Override
        protected ActionListener createButtonListener() {
            return e -> {
                int dialogResult = JOptionPane.showConfirmDialog(panel, "Do you really want to reset all settings to " + labelTest,
                        "Warning", JOptionPane.YES_NO_OPTION);
                if (dialogResult != JOptionPane.YES_OPTION)
                    return;

                CommandQueue.getInstance().write(getCommand());
            };
        }
    }
}
