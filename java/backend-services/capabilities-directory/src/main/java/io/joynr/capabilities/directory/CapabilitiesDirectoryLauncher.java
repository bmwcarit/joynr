/*
 * #%L
 * %%
 * Copyright (C) 2025 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
package io.joynr.capabilities.directory;

import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.charset.StandardCharsets;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Properties;
import java.util.concurrent.atomic.AtomicBoolean;

import com.google.inject.persist.jpa.JpaPersistOptions;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.Module;
import com.google.inject.name.Names;
import com.google.inject.persist.PersistService;
import com.google.inject.persist.jpa.JpaPersistModule;
import com.google.inject.util.Modules;

import io.joynr.capabilities.directory.util.GcdUtilities;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrWaitExpiredException;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.proxy.Future;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.PropertyLoader;
import joynr.exceptions.ApplicationException;

public class CapabilitiesDirectoryLauncher {

    private static final Logger logger = LoggerFactory.getLogger(CapabilitiesDirectoryLauncher.class);
    public static final String GCD_DB_HOST = CapabilitiesDirectoryImpl.PROPERTY_PREFIX + "db.host";
    public static final String GCD_DB_PORT = CapabilitiesDirectoryImpl.PROPERTY_PREFIX + "db.port";
    private static final String DEFAULT_DB_HOST = "localhost";
    private static String dbHost = DEFAULT_DB_HOST;
    private static final String DEFAULT_DB_PORT = "5432";
    private static String dbPort = DEFAULT_DB_PORT;
    static final String GCD_DB_NAME = CapabilitiesDirectoryImpl.PROPERTY_PREFIX + "test.db.name";
    static final String GCD_JPA_PROPERTIES = CapabilitiesDirectoryImpl.PROPERTY_PREFIX + "test.jpa.properties";
    private static String dbName = "gcd";
    private static AtomicBoolean shutdown = new AtomicBoolean(false);
    private static AtomicBoolean started = new AtomicBoolean(false);
    private static int shutdownPort = Integer.parseInt(System.getProperty("joynr.capabilitiesdirectorylauncher.shutdownport",
                                                                          "9999"));
    private static int readyPort = Integer.parseInt(System.getProperty("joynr.capabilitiesdirectorylauncher.readyport",
                                                                       "9998"));
    private static Thread readyThread;
    private static AtomicBoolean ready = new AtomicBoolean(false);
    private static ServerSocket readyServer;
    private static JoynrRuntime runtime;
    private static CapabilitiesDirectoryImpl capabilitiesDirectory;

    private static PersistService persistService;

    private static String getUserProperty(String key, Properties userProperties, String defaultValue) {
        String value;
        String property = PropertyLoader.getPropertiesWithPattern(userProperties, key).getProperty(key);
        if (property == null || property.isEmpty()) {
            value = defaultValue;
            logger.warn("Using default {}: {}", key, value);
        } else {
            value = property;
            logger.info("Using {}: {}", key, value);
        }
        return value;
    }

    private static void prepareConfig(Properties joynrConfig) {
        Properties userProperties = new Properties();
        userProperties.putAll(System.getenv());
        userProperties.putAll(PropertyLoader.getPropertiesWithPattern(System.getProperties(),
                                                                      "^" + CapabilitiesDirectoryImpl.PROPERTY_PREFIX
                                                                              + ".*$"));

        // GCD properties
        String gcdGbid = getUserProperty(CapabilitiesDirectoryImpl.GCD_GBID,
                                         userProperties,
                                         GcdUtilities.loadDefaultGbidsFromDefaultMessagingProperties()[0]);
        joynrConfig.put(CapabilitiesDirectoryImpl.GCD_GBID, gcdGbid);

        String validGbidsString = getUserProperty(CapabilitiesDirectoryImpl.VALID_GBIDS, userProperties, gcdGbid);
        joynrConfig.put(CapabilitiesDirectoryImpl.VALID_GBIDS, validGbidsString);

        // DB properties
        dbHost = getUserProperty(GCD_DB_HOST, userProperties, DEFAULT_DB_HOST);
        dbPort = getUserProperty(GCD_DB_PORT, userProperties, DEFAULT_DB_PORT);
    }

    private static Module getRuntimeModule(Properties joynrConfig) {
        // JPA
        JpaPersistOptions jpaOptions = JpaPersistOptions.builder()
                                                        .setAutoBeginWorkOnEntityManagerCreation(true)
                                                        .build();
        JpaPersistModule jpaModule = new JpaPersistModule("CapabilitiesDirectory", jpaOptions);
        Properties jpaProperties = new Properties();

        String dbNameProperty = joynrConfig.getProperty(GCD_DB_NAME);
        if (dbNameProperty != null) {
            dbName = dbNameProperty;
            logger.warn("Using custom GCD_DB_NAME: {}", dbName);
            joynrConfig.remove(GCD_DB_NAME);
        }
        @SuppressWarnings("unchecked")
        Map<String, String> additionalJpaProperties = (Map<String, String>) joynrConfig.get(GCD_JPA_PROPERTIES);
        if (additionalJpaProperties != null) {
            for (Entry<String, String> entry : additionalJpaProperties.entrySet()) {
                logger.warn("Using custom GCD_JPA_PROPERTIES: {}: {}", entry.getKey(), entry.getValue());
                jpaProperties.setProperty(entry.getKey(), entry.getValue());
            }
            joynrConfig.remove(GCD_JPA_PROPERTIES);
        }

        jpaProperties.setProperty("jakarta.persistence.jdbc.url",
                                  "jdbc:postgresql://" + dbHost + ":" + dbPort + "/" + dbName);
        jpaModule.properties(jpaProperties);

        // runtime Module
        return Modules.override(jpaModule, new CCInProcessRuntimeModule()).with(new HivemqMqttClientModule(),
                                                                                new JoynrPropertiesModule(joynrConfig));
    }

    public static void start(Properties joynrConfig) throws UnknownHostException, InterruptedException {
        if (shutdown.get()) {
            logger.error("Already shut down");
            return;
        }
        if (started.getAndSet(true)) {
            logger.error("Already started");
            return;
        }
        prepareConfig(joynrConfig);
        waitForDb();
        Module runtimeModule = getRuntimeModule(joynrConfig);
        // CapabilitiesDirectoryModule (part of runtimeModule) overrides the CHANNELID property that is generated in
        // JoynrPropertiesModule with the GCD channelId
        Injector injector = Guice.createInjector(Modules.override(runtimeModule)
                                                        .with(new CapabilitiesDirectoryModule()));
        runtime = injector.getInstance(JoynrRuntime.class);
        persistService = injector.getInstance(PersistService.class);
        capabilitiesDirectory = injector.getInstance(CapabilitiesDirectoryImpl.class);
        String localDomain = injector.getInstance(Key.get(String.class,
                                                          Names.named(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL)));
        Future<Void> future = runtime.getProviderRegistrar(localDomain, capabilitiesDirectory)
                                     .awaitGlobalRegistration()
                                     .register();
        try {
            future.get(10000);
            ready.set(true);
            logger.info("GCD ready.");
        } catch (InterruptedException ex) {
            logger.error("Thread interrupted while starting. Provider registration failed.");
            throw ex;
        } catch (JoynrRuntimeException | ApplicationException e) {
            logger.error("Provider registration failed.", e);
            return;
        }
    }

    static CapabilitiesDirectoryImpl getCapabilitiesDirectory() {
        return capabilitiesDirectory;
    }

    private static void waitForDb() throws UnknownHostException, InterruptedException {
        Socket s;
        for (long i = 0; i < 60; i++) {
            try {
                s = new Socket(dbHost, Integer.parseInt(dbPort));
                s.close();
                logger.info("Database available.");
                return;
            } catch (UnknownHostException e) {
                throw e;
            } catch (IOException e) {
                logger.warn("Database not yet available.", e);
            }
            try {
                Thread.sleep(i * 500);
            } catch (InterruptedException e) {
                logger.warn("Sleep interrupted.", e);
                throw e;
            }
        }
        logger.error("Database not available in time.");
        throw new JoynrWaitExpiredException();
    }

    private static void startReadyServer() throws IOException {
        readyServer = new ServerSocket(readyPort);
        readyThread = new Thread(new Runnable() {
            @Override
            public void run() {
                while (!shutdown.get()) {
                    try {
                        Socket s = readyServer.accept();
                        try (PrintWriter out = new PrintWriter(new OutputStreamWriter(s.getOutputStream(),
                                                                                      StandardCharsets.US_ASCII),
                                                               true);) {
                            if (ready.get()) {
                                out.println("OK");
                            } else {
                                out.println("ERROR");
                            }
                        }
                        s.close();
                    } catch (IOException e) {
                        if (shutdown.get() && e instanceof SocketException) {
                            // Ignore SocketException from serverSocket.close()
                            break;
                        }
                        logger.error("Exception from readyServer.accept()", e);
                    }
                }
            }
        });
        readyThread.start();
    }

    private static void waitUntilShutdownRequested() {
        try (ServerSocket serverSocket = new ServerSocket(shutdownPort)) {
            serverSocket.accept();
            serverSocket.close();
        } catch (IOException e) {
            logger.error("Exception from shutdown server socket", e);
            return;
        }
        return;
    }

    public static void shutdown() {
        if (!started.get()) {
            logger.error("Not started");
            return;
        }
        if (shutdown.getAndSet(true)) {
            logger.error("Already shut down");
            return;
        }
        ready.set(false);
        if (persistService != null) {
            persistService.stop();
        }
        if (runtime != null) {
            runtime.shutdown(true);
        }
        if (readyServer != null) {
            stopReadyServer();
        }
    }

    private static void stopReadyServer() {
        try {
            readyServer.close();
        } catch (IOException e1) {
            // ignore
        }
        readyServer = null;
        try {
            readyThread.join();
        } catch (InterruptedException e) {
            logger.error("Thread interrupted while stopping ready server. ");
            Thread.currentThread().interrupt();
        }
        readyThread = null;
    }

    // if invoked as standalone program then after initialization wait
    // until a connection on a shutdownPort gets established, to
    // allow to initiate shutdown from outside.
    public static void main(String[] args) throws IOException {
        try {
            start(new Properties());
            startReadyServer();
            waitUntilShutdownRequested();
        } catch (InterruptedException e) {
            logger.error("Thread interrupted. ", e);
            Thread.currentThread().interrupt();
        }
        shutdown();
    }
}
