package com.rusefi;

import com.fathzer.soft.javaluator.DoubleEvaluator;
import com.rusefi.autodetect.PortDetector;
import com.rusefi.binaryprotocol.BinaryProtocol;
import com.rusefi.binaryprotocol.BinaryProtocolHolder;
import com.rusefi.config.generated.Fields;
import com.rusefi.core.EngineState;
import com.rusefi.core.MessagesCentral;
import com.rusefi.core.Sensor;
import com.rusefi.core.SensorCentral;
import com.rusefi.io.*;
import com.rusefi.io.serial.PortHolder;
import com.rusefi.io.tcp.BinaryProtocolServer;
import com.rusefi.maintenance.FirmwareFlasher;
import com.rusefi.maintenance.VersionChecker;
import com.rusefi.ui.*;
import com.rusefi.ui.engine.EngineSnifferPanel;
import com.rusefi.ui.logview.LogViewer;
import com.rusefi.ui.storage.Node;
import com.rusefi.ui.util.DefaultExceptionHandler;
import com.rusefi.ui.util.FrameHelper;
import com.rusefi.ui.util.JustOneInstance;
import com.rusefi.ui.util.UiUtils;
import jssc.SerialPortList;

import javax.swing.*;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import java.awt.*;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.TimeZone;
import java.util.concurrent.atomic.AtomicReference;

import static com.rusefi.ui.storage.PersistentConfiguration.getConfig;

/**
 * this is the main entry point of rusEfi ECU console
 * <p/>
 * <p/>
 * 12/25/12
 * (c) Andrey Belomutskiy 2013-2019
 *
 * @see StartupFrame
 * @see EngineSnifferPanel
 */
public class Launcher {
    public static final int CONSOLE_VERSION = 20200323;
    public static final String INI_FILE_PATH = System.getProperty("ini_file_path", "..");
    public static final String INPUT_FILES_PATH = System.getProperty("input_files_path", "..");
    public static final String TOOLS_PATH = System.getProperty("tools_path", ".");
    private static final String TAB_INDEX = "main_tab";
    protected static final String PORT_KEY = "port";
    protected static final String SPEED_KEY = "speed";

    private static final String TOOL_NAME_COMPILE_FSIO_FILE = "compile_fsio_file";
    private static final String TOOL_NAME_REBOOT_ECU = "reboot_ecu";
    private static final String TOOL_NAME_FIRING_ORDER = "firing_order";
    private static final String TOOL_NAME_FUNCTIONAL_TEST = "functional_test";
    private static final String TOOL_NAME_PERF_ENUMS = "ptrace_enums";
    // todo: rename to something more FSIO-specific? would need to update documentation somewhere
    private static final String TOOL_NAME_COMPILE = "compile";

    private final String port;
    // todo: the logic around 'fatalError' could be implemented nicer
    private String fatalError;
    public static EngineSnifferPanel engineSnifferPanel;
    private static SensorCentral.SensorListener wrongVersionListener;

    private final JTabbedPane tabbedPane = new JTabbedPane() {
        @Override
        public void paint(Graphics g) {
            super.paint(g);
            paintStatusText(g);
        }

        private void paintStatusText(Graphics g) {
            Font f = g.getFont();
            g.setFont(new Font(f.getName(), f.getStyle(), f.getSize() * 4));
            Dimension d = getSize();
            String text;
            switch (ConnectionStatus.INSTANCE.getValue()) {
                case NOT_CONNECTED:
                    text = "Not connected";
                    break;
                case LOADING:
                    text = "Loading";
                    break;
                default:
                    text = "";
            }
            if (fatalError != null) {
                text = fatalError;
                g.setColor(Color.red);
            }
            int labelWidth = g.getFontMetrics().stringWidth(text);
            g.drawString(text, (d.width - labelWidth) / 2, d.height / 2);
        }
    };
    public static AtomicReference<String> firmwareVersion = new AtomicReference<>("N/A");

    private static Frame staticFrame;
    private final TableEditorPane tableEditor = new TableEditorPane();
    private final SettingsTab settingsTab = new SettingsTab();
    private final LogDownloader logsManager = new LogDownloader();
    private final FuelTunePane fuelTunePane;
    private final PaneSettings paneSettings;

    /**
     * @see StartupFrame
     */
    private FrameHelper mainFrame = new FrameHelper() {
        @Override
        protected void onWindowOpened() {
            super.onWindowOpened();
            windowOpenedHandler();
        }

        @Override
        protected void onWindowClosed() {
            /**
             * here we would close the port and log a message about it
             */
            windowClosedHandler();
            /**
             * here we would close the log file
             */
            super.onWindowClosed();
        }
    };
    private final Map<JComponent, ActionListener> tabSelectedListeners = new HashMap<JComponent, ActionListener>();

    public Launcher(String port) {
        this.port = port;
        staticFrame = mainFrame.getFrame();
        FileLog.MAIN.logLine("Console " + CONSOLE_VERSION);

        FileLog.MAIN.logLine("Hardware: " + FirmwareFlasher.getHardwareKind());

        getConfig().getRoot().setProperty(PORT_KEY, port);
        getConfig().getRoot().setProperty(SPEED_KEY, PortHolder.BAUD_RATE);

        LinkManager.start(port);

        MessagesCentral.getInstance().addListener(new MessagesCentral.MessageListener() {
            @Override
            public void onMessage(Class clazz, String message) {
                if (message.startsWith(ConnectionStatus.FATAL_MESSAGE_PREFIX))
                    fatalError = message;
            }
        });

        paneSettings = new PaneSettings(getConfig().getRoot().getChild("panes"));

        engineSnifferPanel = new EngineSnifferPanel(getConfig().getRoot().getChild("digital_sniffer"));
        if (!LinkManager.isLogViewerMode(port))
            engineSnifferPanel.setOutpinListener(LinkManager.engineState);

        if (LinkManager.isLogViewerMode(port))
            tabbedPane.add("Log Viewer", new LogViewer(engineSnifferPanel));

        ConnectionWatchdog.start();


        GaugesPanel.DetachedRepository.INSTANCE.init(getConfig().getRoot().getChild("detached"));
        GaugesPanel.DetachedRepository.INSTANCE.load();
        if (!LinkManager.isLogViewer())
            tabbedPane.addTab("Gauges", new GaugesPanel(getConfig().getRoot().getChild("gauges"), paneSettings).getContent());

        if (!LinkManager.isLogViewer()) {
            MessagesPane messagesPane = new MessagesPane(getConfig().getRoot().getChild("messages"));
            tabbedPaneAdd("Messages", messagesPane.getContent(), messagesPane.getTabSelectedListener());
        }
        if (!LinkManager.isLogViewer()) {
            tabbedPane.add("Bench Test", new BenchTestPane().getContent());
            if (paneSettings.showEtbPane)
                tabbedPane.add("ETB", new ETBPane().getContent());
            tabbedPane.add("Presets", new PresetsPane().getContent());
        }

        tabbedPaneAdd("Engine Sniffer", engineSnifferPanel.getPanel(), engineSnifferPanel.getTabSelectedListener());

        if (!LinkManager.isLogViewer()) {
            SensorSnifferPane sensorSniffer = new SensorSnifferPane(getConfig().getRoot().getChild("sensor_sniffer"));
            tabbedPaneAdd("Sensor Sniffer", sensorSniffer.getPanel(), sensorSniffer.getTabSelectedListener());
        }

//        tabbedPane.addTab("LE controls", new FlexibleControls().getPanel());

//        tabbedPane.addTab("ADC", new AdcPanel(new BooleanInputsModel()).createAdcPanel());
        if (paneSettings.showStimulatorPane && !LinkManager.isSimulationMode && !LinkManager.isLogViewerMode(port)) {
            // todo: rethink this UI? special command line key to enable it?
            EcuStimulator stimulator = EcuStimulator.getInstance();
            tabbedPane.add("ECU stimulation", stimulator.getPanel());
        }
//        tabbedPane.addTab("live map adjustment", new Live3DReport().getControl());
        if (!LinkManager.isLogViewer())
            tabbedPane.addTab("Table Editor", tableEditor);
//        tabbedPane.add("Wizards", new Wizard().createPane());

        if (!LinkManager.isLogViewer())
            tabbedPane.add("Settings", settingsTab.createPane());
        if (!LinkManager.isLogViewer()) {
            tabbedPane.addTab("Formulas/Live Data", new FormulasPane().getContent());
            tabbedPane.addTab("Sensors Live Data", new SensorsLiveDataPane().getContent());
        }

        if (!LinkManager.isLogViewer() && false) // todo: fix it & better name?
            tabbedPane.add("Logs Manager", logsManager.getContent());
        fuelTunePane = new FuelTunePane(getConfig().getRoot().getChild("fueltune"));
        if (paneSettings.showFuelTunePane)
            tabbedPane.add("Fuel Tune", fuelTunePane.getContent());


        if (!LinkManager.isLogViewer()) {
            if (paneSettings.showTriggerShapePane)
                tabbedPane.add("Trigger Shape", new AverageAnglePanel().getPanel());
        }

        if (!LinkManager.isLogViewerMode(port)) {
            int selectedIndex = getConfig().getRoot().getIntProperty(TAB_INDEX, 2);
            if (selectedIndex < tabbedPane.getTabCount())
                tabbedPane.setSelectedIndex(selectedIndex);
        }

        tabbedPane.addChangeListener(new ChangeListener() {
            @Override
            public void stateChanged(ChangeEvent e) {
                if (e.getSource() instanceof JTabbedPane) {
                    JTabbedPane pane = (JTabbedPane) e.getSource();
                    int selectedIndex = pane.getSelectedIndex();
                    System.out.println("Selected paneNo: " + selectedIndex);
                    ActionListener actionListener = tabSelectedListeners.get(pane.getComponentAt(selectedIndex));
                    if (actionListener != null)
                        actionListener.actionPerformed(null);
                }
            }
        });

        StartupFrame.setAppIcon(mainFrame.getFrame());
        mainFrame.showFrame(tabbedPane);
    }

    private void tabbedPaneAdd(String title, JComponent component, ActionListener tabSelectedListener) {
        tabSelectedListeners.put(component, tabSelectedListener);
        tabbedPane.add(title, component);
    }

    private void windowOpenedHandler() {
        setTitle();
        ConnectionStatus.INSTANCE.addListener(new ConnectionStatus.Listener() {
            @Override
            public void onConnectionStatus(boolean isConnected) {
                setTitle();
                UiUtils.trueRepaint(tabbedPane); // this would repaint status label
                if (ConnectionStatus.INSTANCE.getValue() == ConnectionStatus.Value.CONNECTED) {
                    long unixGmtTime = System.currentTimeMillis() / 1000L;
                    long withOffset = unixGmtTime + TimeZone.getDefault().getOffset(System.currentTimeMillis()) / 1000;
                    CommandQueue.getInstance().write("set " +
                                    Fields.CMD_DATE +
                                    " " + withOffset, CommandQueue.DEFAULT_TIMEOUT,
                            InvocationConfirmationListener.VOID, false);
                }
            }
        });

        LinkManager.open(new ConnectionStateListener() {
            @Override
            public void onConnectionFailed() {
            }

            @Override
            public void onConnectionEstablished() {
                FileLog.MAIN.logLine("onConnectionEstablished");
                tableEditor.showContent();
                settingsTab.showContent();
                logsManager.showContent();
                fuelTunePane.showContent();
                BinaryProtocolServer.start();
            }
        });

        LinkManager.engineState.registerStringValueAction(Fields.PROTOCOL_VERSION_TAG, new EngineState.ValueCallback<String>() {
            @Override
            public void onUpdate(String firmwareVersion) {
                Launcher.firmwareVersion.set(firmwareVersion);
                SensorLogger.init();
                setTitle();
                VersionChecker.getInstance().onFirmwareVersion(firmwareVersion);
            }
        });
    }

    private void setTitle() {
        String disconnected = ConnectionStatus.INSTANCE.isConnected() ? "" : "DISCONNECTED ";
        mainFrame.getFrame().setTitle(disconnected + "Console " + CONSOLE_VERSION + "; firmware=" + Launcher.firmwareVersion.get() + "@" + port);
    }

    private void windowClosedHandler() {
        /**
         * looks like reconnectTimer in {@link com.rusefi.ui.RpmPanel} keeps AWT alive. Simplest solution would be to 'exit'
         */
        SimulatorHelper.onWindowClosed();
        Node root = getConfig().getRoot();
        root.setProperty("version", CONSOLE_VERSION);
        root.setProperty(TAB_INDEX, tabbedPane.getSelectedIndex());
        GaugesPanel.DetachedRepository.INSTANCE.saveConfig();
        getConfig().save();
        BinaryProtocol bp = BinaryProtocolHolder.getInstance().get();
        if (bp != null && !bp.isClosed)
            bp.close(); // it could be that serial driver wants to be closed explicitly
        System.exit(0);
    }

    /**
     * rusEfi console entry point
     * @see StartupFrame if no parameters specified
     */
    public static void main(final String[] args) throws Exception {
        String toolName = args.length == 0 ? null : args[0];

        if (TOOL_NAME_FUNCTIONAL_TEST.equals(toolName)) {
            // passing port argument if it was specified
            String[] toolArgs = args.length == 1 ? new String[0] : new String[]{args[1]};
            RealHwTest.main(toolArgs);
            return;
        }

        if (TOOL_NAME_COMPILE_FSIO_FILE.equalsIgnoreCase(toolName)) {
            int returnCode = invokeCompileFileTool(args);
            System.exit(returnCode);
        }

        if (TOOL_NAME_COMPILE.equals(toolName)) {
            invokeCompileExpressionTool(args);
            System.exit(0);
        }

        if (TOOL_NAME_FIRING_ORDER.equals(toolName)) {
            FiringOrderTSLogic.invoke(args[1]);
            System.exit(0);
        }

        if (TOOL_NAME_PERF_ENUMS.equals(toolName)) {
            PerfTraceTool.readPerfTrace(args[1], args[2], args[3], args[4]);
            System.exit(0);
        }

        System.out.println("Optional tools: " + Arrays.asList(TOOL_NAME_COMPILE_FSIO_FILE,
                TOOL_NAME_COMPILE,
                TOOL_NAME_REBOOT_ECU,
                TOOL_NAME_FIRING_ORDER));
        System.out.println("Starting rusEfi UI console " + CONSOLE_VERSION);

        FileLog.MAIN.start();

        if (TOOL_NAME_REBOOT_ECU.equalsIgnoreCase(toolName)) {
            invokeRebootTool();
            return;
        }


        getConfig().load();
        FileLog.suspendLogging = getConfig().getRoot().getBoolProperty(GaugesPanel.DISABLE_LOGS);
        Thread.setDefaultUncaughtExceptionHandler(new DefaultExceptionHandler());
        VersionChecker.start();
        SwingUtilities.invokeAndWait(new Runnable() {
            public void run() {
                awtCode(args);
            }
        });
    }

    private static int invokeCompileFileTool(String[] args) throws IOException {
        /**
         * re-packaging array which contains input and output file names
         */
        return CompileTool.run(Arrays.asList(args).subList(1, args.length));
    }

    private static void invokeRebootTool() throws IOException {
        String autoDetectedPort = PortDetector.autoDetectPort(null);
        if (autoDetectedPort == null) {
            System.err.println("rusEfi not detected");
            return;
        }
        PortHolder.EstablishConnection establishConnection = new PortHolder.EstablishConnection(autoDetectedPort).invoke();
        if (!establishConnection.isConnected())
            return;
        IoStream stream = establishConnection.getStream();
        byte[] commandBytes = BinaryProtocol.getTextCommandBytes(Fields.CMD_REBOOT);
        stream.sendPacket(commandBytes, FileLog.LOGGER);
    }

    private static void invokeCompileExpressionTool(String[] args) {
        if (args.length != 2) {
            System.err.println("input expression parameter expected");
            System.exit(-1);
        }
        String expression = args[1];
        System.out.println(DoubleEvaluator.process(expression).getPosftfixExpression());
    }

    private static void awtCode(String[] args) {
        if (JustOneInstance.isAlreadyRunning()) {
            int result = JOptionPane.showConfirmDialog(null, "Looks like another instance is already running. Do you really want to start another instance?",
                    "rusEfi", JOptionPane.YES_NO_OPTION);
            if (result == JOptionPane.NO_OPTION)
                System.exit(-1);
        }
        wrongVersionListener = new SensorCentral.SensorListener() {
            @Override
            public void onSensorUpdate(double value) {
                // todo: we need to migrate to TS_SIGNATURE validation!!!
                if (value != Fields.TS_FILE_VERSION) {
                    String message = "This copy of rusEfi console is not compatible with this version of firmware\r\n" +
                            "Console compatible with " + Fields.TS_FILE_VERSION + " while firmware compatible with " +
                            (int) value;
                    JOptionPane.showMessageDialog(Launcher.getFrame(), message);
                    assert wrongVersionListener != null;
                    SensorCentral.getInstance().removeListener(Sensor.TS_CONFIG_VERSION, wrongVersionListener);
                }
            }
        };
        SensorCentral.getInstance().addListener(Sensor.TS_CONFIG_VERSION, wrongVersionListener);
        JustOneInstance.onStart();
        try {
            boolean isPortDefined = args.length > 0;
            boolean isBaudRateDefined = args.length > 1;
            if (isBaudRateDefined)
                PortHolder.BAUD_RATE = Integer.parseInt(args[1]);

            String port = null;
            if (isPortDefined)
                port = args[0];


            if (isPortDefined) {
                port = PortDetector.autoDetectSerialIfNeeded(port);
                if (port == null) {
                    isPortDefined = false;
                }
            }

            if (isPortDefined) {
                new Launcher(port);
            } else {
                for (String p : SerialPortList.getPortNames())
                    MessagesCentral.getInstance().postMessage(Launcher.class, "Available port: " + p);
                new StartupFrame().chooseSerialPort();
            }

        } catch (Throwable e) {
            throw new IllegalStateException(e);
        }
    }

    public static Frame getFrame() {
        return staticFrame;
    }
}
