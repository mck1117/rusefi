package com.rusefi.proxy;

import com.devexperts.logging.Logging;
import com.opensr5.ConfigurationImage;
import com.rusefi.Timeouts;
import com.rusefi.binaryprotocol.BinaryProtocol;
import com.rusefi.config.generated.Fields;
import com.rusefi.io.AbstractConnectionStateListener;
import com.rusefi.io.IoStream;
import com.rusefi.io.LinkManager;
import com.rusefi.io.commands.HelloCommand;
import com.rusefi.io.tcp.BinaryProtocolServer;
import com.rusefi.io.tcp.TcpIoStream;
import com.rusefi.rusEFIVersion;
import com.rusefi.server.ControllerInfo;
import com.rusefi.server.SessionDetails;
import com.rusefi.server.rusEFISSLContext;
import com.rusefi.tools.VehicleToken;
import com.rusefi.tools.online.HttpUtil;
import org.jetbrains.annotations.NotNull;

import java.io.Closeable;
import java.io.IOException;
import java.net.Socket;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import static com.rusefi.binaryprotocol.BinaryProtocol.sleep;

/**
 * Connector between rusEFI ECU and rusEFI server
 * see NetworkConnectorStartup
 */
public class NetworkConnector implements Closeable {
    public static final byte DISCONNECT = 14;
    public static final byte UPDATE_CONNECTOR_SOFTWARE = 15;
    public static final byte UPDATE_FIRMWARE = 16;
    private final static Logging log = Logging.getLogging(NetworkConnector.class);
    private boolean isClosed;

    public NetworkConnectorResult start(Implementation implementation, String authToken, String controllerPort, NetworkConnectorContext context) {
        return start(implementation, authToken, controllerPort, context, ReconnectListener.VOID);
    }

    public NetworkConnectorResult start(Implementation implementation, String authToken, String controllerPort, NetworkConnectorContext context, ReconnectListener reconnectListener) {
        LinkManager controllerConnector = new LinkManager()
                .setCompositeLogicEnabled(false)
                .setNeedPullData(false);

        CountDownLatch onConnected = new CountDownLatch(1);
        controllerConnector.startAndConnect(controllerPort, new AbstractConnectionStateListener() {
            @Override
            public void onConnectionEstablished() {
                onConnected.countDown();
            }
        });

        log.info("Connecting to controller...");
        try {
            onConnected.await(1, TimeUnit.MINUTES);
        } catch (InterruptedException e) {
            throw new IllegalStateException(e);
        }
        if (onConnected.getCount() != 0) {
            log.info("Connection to controller failed");
            return NetworkConnectorResult.ERROR;
        }

        return start(implementation, authToken, context, reconnectListener, controllerConnector, ActivityListener.VOID);
    }

    public NetworkConnectorResult start(Implementation implementation, String authToken, NetworkConnectorContext context, ReconnectListener reconnectListener, LinkManager linkManager, ActivityListener activityListener) {
        ControllerInfo controllerInfo;
        try {
            controllerInfo = getControllerInfo(linkManager, linkManager.getConnector().getBinaryProtocol().getStream());
        } catch (IOException e) {
            return NetworkConnectorResult.ERROR;
        }

        int vehicleToken = VehicleToken.getOrCreate();

        BinaryProtocolServer.getThreadFactory("Proxy Reconnect").newThread(() -> {
            Semaphore proxyReconnectSemaphore = new Semaphore(1);
            try {
                while (!isClosed) {
                    proxyReconnectSemaphore.acquire();

                    try {
                        start(implementation,
                                activityListener,
                                context.serverPortForControllers(), linkManager, authToken, (String message) -> {
                                    log.error(message + " Disconnect from proxy server detected, now sleeping " + context.reconnectDelay() + " seconds");
                                    sleep(context.reconnectDelay() * Timeouts.SECOND);
                                    log.debug("Releasing semaphore");
                                    proxyReconnectSemaphore.release();
                                    reconnectListener.onReconnect();
                                }, vehicleToken, controllerInfo, context);
                    } catch (IOException e) {
                        log.error("IO error", e);
                    }
                }
            } catch (InterruptedException e) {
                throw new IllegalStateException(e);
            }
        }).start();

        return new NetworkConnectorResult(controllerInfo, vehicleToken);
    }

    @NotNull
    private static SessionDetails start(Implementation implementation, ActivityListener activityListener, int serverPortForControllers, LinkManager linkManager, String authToken, final TcpIoStream.DisconnectListener disconnectListener, int oneTimeToken, ControllerInfo controllerInfo, final NetworkConnectorContext context) throws IOException {
        IoStream targetEcuSocket = linkManager.getConnector().getBinaryProtocol().getStream();

        SessionDetails deviceSessionDetails = new SessionDetails(implementation, controllerInfo, authToken, oneTimeToken, rusEFIVersion.CONSOLE_VERSION);

        Socket socket;
        try {
            log.info("Connecting to proxy server " + HttpUtil.RUSEFI_PROXY_HOSTNAME + " " + serverPortForControllers);
            socket = rusEFISSLContext.getSSLSocket(HttpUtil.RUSEFI_PROXY_HOSTNAME, serverPortForControllers);
        } catch (IOException e) {
            // socket open exception is a special case and should be handled separately
            disconnectListener.onDisconnect("on socket open");
            return deviceSessionDetails;
        }
        BaseBroadcastingThread baseBroadcastingThread = new BaseBroadcastingThread(socket,
                deviceSessionDetails,
                disconnectListener, context) {
            @Override
            protected void handleCommand(BinaryProtocolServer.Packet packet, TcpIoStream stream) throws IOException {
                super.handleCommand(packet, stream);
                byte command = packet.getPacket()[0];
                if (command == Fields.TS_ONLINE_PROTOCOL) {
                    byte connectorCommand = packet.getPacket()[1];
                    log.info("Got connector command " + packet.getPacket());
                    if (connectorCommand == NetworkConnector.UPDATE_CONNECTOR_SOFTWARE) {
                        context.onConnectorSoftwareUpdateRequest();
                    } else if (connectorCommand == NetworkConnector.UPDATE_FIRMWARE) {

                    }
                    return;
                }

                log.info("Relaying request to controller " + BinaryProtocol.findCommand(command));
                targetEcuSocket.sendPacket(packet);

                BinaryProtocolServer.Packet response = targetEcuSocket.readPacket();
                log.info("Relaying response to proxy size=" + response.getPacket().length);
                stream.sendPacket(response);
                activityListener.onActivity(targetEcuSocket);
            }
        };
        baseBroadcastingThread.start();
        return deviceSessionDetails;
    }

    @NotNull
    private static ControllerInfo getControllerInfo(LinkManager linkManager, IoStream targetEcuSocket) throws IOException {
        HelloCommand.send(targetEcuSocket);
        String helloResponse = HelloCommand.getHelloResponse(targetEcuSocket.getDataBuffer());
        if (helloResponse == null)
            throw new IOException("Error getting hello response");
        String controllerSignature = helloResponse.trim();

        ConfigurationImage image = linkManager.getConnector().getBinaryProtocol().getControllerConfiguration();
        String vehicleName = Fields.VEHICLENAME.getStringValue(image);
        String engineMake = Fields.ENGINEMAKE.getStringValue(image);
        String engineCode = Fields.ENGINECODE.getStringValue(image);
        return new ControllerInfo(vehicleName, engineMake, engineCode, controllerSignature);
    }

    @Override
    public void close() {
        isClosed = true;
    }

    public static class NetworkConnectorResult {
        static NetworkConnectorResult ERROR = new NetworkConnectorResult(null, 0);
        private final ControllerInfo controllerInfo;
        private final int oneTimeToken;

        public NetworkConnectorResult(ControllerInfo controllerInfo, int oneTimeToken) {
            this.controllerInfo = controllerInfo;
            this.oneTimeToken = oneTimeToken;
        }

        public ControllerInfo getControllerInfo() {
            return controllerInfo;
        }

        public int getOneTimeToken() {
            return oneTimeToken;
        }

        @Override
        public String toString() {
            return "NetworkConnectorResult{" +
                    "controllerInfo=" + controllerInfo +
                    '}';
        }
    }

    public interface ReconnectListener {
        ReconnectListener VOID = new ReconnectListener() {
            @Override
            public void onReconnect() {

            }
        };

        void onReconnect();
    }

    public interface ActivityListener {
        ActivityListener VOID = new ActivityListener() {
            @Override
            public void onActivity(IoStream targetEcuSocket) {

            }
        };
        void onActivity(IoStream targetEcuSocket);
    }

    public enum Implementation {
        Android,
        Plugin,
        SBC,
        Unknown;

        public static Implementation find(String name) {
            for (Implementation implementation : values()) {
                if (implementation.name().equalsIgnoreCase(name))
                    return implementation;
            }
            return Unknown;
        }
    }
}
