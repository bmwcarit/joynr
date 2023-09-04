/*-
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.tests.gracefulshutdown;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingQosEffort;
import io.joynr.proxy.Callback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.tests.graceful.shutdown.EchoAsync;
import joynr.tests.graceful.shutdown.EchoProxy;

public class ConsumerApplication extends AbstractJoynrApplication {

    private static final Logger logger = LoggerFactory.getLogger(ConsumerApplication.class);

    private static final int MESSAGE_THREAD_COUNT = 3;
    private static final int MULTIPLE_MESSAGE_COUNT = 10;
    private final ExecutorService senderThreadExecutor = Executors.newFixedThreadPool(MESSAGE_THREAD_COUNT);
    private final ExecutorService prepareForShutdownThreadExecutor = Executors.newSingleThreadExecutor();

    private final Semaphore semaphore = new Semaphore(1000);

    private final List<CallResult> calls = Collections.synchronizedList(new ArrayList<>());
    private final List<Future<?>> futureList = Collections.synchronizedList(new ArrayList<>());
    private final AtomicLong prepareForShutdownTime = new AtomicLong();
    private final AtomicBoolean waitIsFinished = new AtomicBoolean();
    private final AtomicBoolean sendMultipleMessages = new AtomicBoolean();

    @Override
    public void run() {
        logger.info("ConsumerApplication running ...");
        sleepInSeconds(1);
        final EchoAsync echoService = buildProxy();

        launchPrepareForShutdownThread();
        launchSendMessageThreads(echoService);
        waitForScenarioToFinish();
        analyzeAndLogResults();

        logger.info("Cancelling all futures");
        futureList.forEach(f -> f.cancel(false));
        logger.info("ConsumerApplication ended");
    }

    private EchoAsync buildProxy() {
        logger.info("Proxy building started.");
        final ProxyBuilder<EchoProxy> proxyBuilder = runtime.getProxyBuilder("io.joynr.tests.gracefulshutdown.jee.provider",
                                                                             EchoProxy.class);
        final DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setDiscoveryTimeoutMs(3000L);
        discoveryQos.setRetryIntervalMs(500L);
        proxyBuilder.setDiscoveryQos(discoveryQos);

        final MessagingQos messagingQos = new MessagingQos();
        messagingQos.setTtl_ms(30000L);
        messagingQos.setEffort(MessagingQosEffort.BEST_EFFORT);
        proxyBuilder.setMessagingQos(messagingQos);

        final io.joynr.proxy.Future<Void> proxyFuture = new io.joynr.proxy.Future<>();
        final EchoAsync echoService = proxyBuilder.build(new ProxyCreatedCallback<>() {
            @Override
            public void onProxyCreationFinished(final EchoProxy result) {
                proxyFuture.resolve();
            }

            @Override
            public void onProxyCreationError(final JoynrRuntimeException error) {
                proxyFuture.onFailure(error);
            }
        });
        try {
            proxyFuture.get();
        } catch (Exception e) {
            logger.error("Proxy creation failed", e);
        }
        logger.info("Proxy building finished.");
        return echoService;
    }

    private void analyzeAndLogResults() {
        synchronized (calls) {
            logger.info("Analysing and logging results");
            final var stopTime = System.currentTimeMillis();
            calls.stream().filter(CallResult::isUnknown).forEach(c -> c.stopProcessing(stopTime));

            final var successful = calls.stream().filter(CallResult::isSuccessful).count();
            final var failure = calls.stream().filter(CallResult::isFailure).count();
            final var unknown = calls.stream().filter(CallResult::isUnknown).count();

            final var processedBeforePrepareForShutdown = calls.stream()
                                                               .filter(c -> c.processedBefore(prepareForShutdownTime.get()))
                                                               .collect(Collectors.groupingBy(CallResult::getStatus));

            final var createdAfterPrepareForShutdown = calls.stream()
                                                            .filter(c -> c.createdAfter(prepareForShutdownTime.get()))
                                                            .collect(Collectors.groupingBy(CallResult::getStatus));

            final var createdBeforePrepareForShutdownAndProcessedAfter = calls.stream()
                                                                              .filter(c -> c.createdBeforeButProcessedAfter(prepareForShutdownTime.get()))
                                                                              .collect(Collectors.groupingBy(CallResult::getStatus));

            logger.info("Call results: total calls: {}, successful: {}, failure: {}, unknown: {}",
                        calls.size(),
                        successful,
                        failure,
                        unknown);
            logCheckResult("Messages processed before prepare for shutdown signal fired:",
                           processedBeforePrepareForShutdown,
                           false,
                           false);
            logCheckResult("Messages created after prepare for shutdown signal fired:",
                           createdAfterPrepareForShutdown,
                           true,
                           true);
            logCheckResult("Messages created before prepare for shutdown signal fired but processed after:",
                           createdBeforePrepareForShutdownAndProcessedAfter,
                           true,
                           false);
        }
    }

    /**
     * Waits until waitIsFinished flag will become true
     */
    private void waitForScenarioToFinish() {
        final var before = System.currentTimeMillis();
        logger.info("Before wait loop");
        while (!waitIsFinished.get()) {
            sleep(100);
        }
        logger.info("After wait loop");
        logger.info("Wait time: {} ms", System.currentTimeMillis() - before);
    }

    /**
     * Steps executed in separate thread:
     * 1. Wait 19 seconds;
     * 2. Enable sending multiple messages instead of one;
     * 3. Wait 1 second;
     * 4. Disable sending multiple messages;
     * 5. Send prepare for shutdown signal to Provider;
     * 6. Wait 20 seconds;
     * 7. Mark scenario is finished;
     */
    private void launchPrepareForShutdownThread() {
        prepareForShutdownThreadExecutor.submit(() -> {
            logger.info("Wait thread started");
            sleepInSeconds(19);
            logger.info("Enable sending multiple messages instead of single one");
            sendMultipleMessages.set(true);
            sleepInSeconds(1);
            logger.info("Disable sending multiple messages instead of single one");
            sendMultipleMessages.set(false);
            logger.info("Store time when prepare for shutdown signal will be fired");
            prepareForShutdownTime.set(System.currentTimeMillis());
            sendPrepareForShutdownSignal();
            sleepInSeconds(20);
            logger.info("Wait is finished. Time to stop.");
            waitIsFinished.set(true);
        });
    }

    /**
     * Create multiple threads.
     * Each thread sends a message to Provider repeatedly.
     * Threads are exited when scenario is marked as finished.
     * @param echoService proxy instance
     */
    private void launchSendMessageThreads(final EchoAsync echoService) {
        IntStream.range(0, MESSAGE_THREAD_COUNT).forEach(threadCounter -> {
            futureList.add(senderThreadExecutor.submit(() -> {
                while (!waitIsFinished.get()) {
                    sendMessage(echoService);
                    sleep(50);
                }
            }));
            sleep(100);
        });
    }

    private void sendPrepareForShutdownSignal() {
        try {
            logger.info("Sending prepare for shutdown signal to Provider");
            final URL url = new URL("http://provider:8080/control/prepareForShutdown");
            final HttpURLConnection con = (HttpURLConnection) url.openConnection();
            con.setRequestMethod("GET");
            final int status = con.getResponseCode();
            if (status != 204) {
                logger.error("Provider returned {} instead of 204 while sending prepare for shutdown signal", status);
            } else {
                logger.info("Prepare for shutdown was sent to provider");
            }
        } catch (final IOException e) {
            logger.error("Error while sending prepare for shutdown signal.", e);
        }
    }

    private void logCheckResult(final String message,
                                final Map<CallStatus, List<CallResult>> map,
                                final boolean allowFailure,
                                final boolean allowUnknown) {
        final var success = getSize(map, CallStatus.SUCCESS);
        final var failure = getSize(map, CallStatus.FAILURE);
        final var unknown = getSize(map, CallStatus.UNKNOWN);

        final var forbidden = new ArrayList<CallStatus>();
        if (!allowFailure && failure > 0) {
            forbidden.add(CallStatus.FAILURE);
        }
        if (!allowUnknown && unknown > 0) {
            forbidden.add(CallStatus.UNKNOWN);
        }
        final var checkResult = forbidden.isEmpty() ? "[SUCCESS]" : "[FAIL], not allowed: " + forbidden;

        logger.info(message + " {}, total {}, success: {}, failure: {}, unknown: {};",
                    checkResult,
                    success + failure + unknown,
                    success,
                    failure,
                    unknown);
    }

    private int getSize(final Map<CallStatus, List<CallResult>> map, final CallStatus status) {
        return map.containsKey(status) ? map.get(status).size() : 0;
    }

    private void sendMessage(final EchoAsync echoService) {
        try {
            if (!semaphore.tryAcquire(5, TimeUnit.SECONDS)) {
                throw new TimeoutException("Unable to acquire semaphore in time");
            }

            final int messageCount = sendMultipleMessages.get() ? MULTIPLE_MESSAGE_COUNT : 1;
            IntStream.rangeClosed(1, messageCount).forEach(i -> {
                invokeEcho(echoService);
                sleep(10);
            });
        } catch (final InterruptedException | TimeoutException exception) {
            logger.error("Unable to acquire semaphore for sending message in time.", exception);
        }
    }

    private void invokeEcho(final EchoAsync echoService) {
        final long requestTime = System.currentTimeMillis();
        final String inputData = "Test " + requestTime;
        final CallResult call = new CallResult(requestTime);
        calls.add(call);
        echoService.echoString(new Callback<>() {
            @Override
            public void onSuccess(final String result) {
                call.resolve(System.currentTimeMillis());
                if (result != null) {
                    logger.info("Got echo back: {}. Input data was: {};", result, inputData);
                    semaphore.release();
                } else {
                    logger.info("Got echo back: null. Input data was: {};", inputData);
                }
            }

            @Override
            public void onFailure(final JoynrRuntimeException runtimeException) {
                logger.error("Problem calling echo service. Input data was: {}", inputData, runtimeException);
                call.fail(System.currentTimeMillis());
                semaphore.release();
            }
        }, inputData);
    }

    private void sleepInSeconds(final long seconds) {
        sleep(seconds * 1000L);
    }

    private void sleep(final long millis) {
        try {
            Thread.sleep(millis);
        } catch (final InterruptedException e) {
            logger.error("Interrupted while sleeping.", e);
        }
    }

    enum CallStatus {
        SUCCESS, FAILURE, UNKNOWN
    }

    static class CallResult {
        private CallStatus status;
        private final long requestTime;
        private long responseTime;

        CallResult(final long requestTime) {
            this.requestTime = requestTime;
            this.status = CallStatus.UNKNOWN;
        }

        void resolve(final long responseTime) {
            this.responseTime = responseTime;
            this.status = CallStatus.SUCCESS;
        }

        void fail(final long responseTime) {
            this.responseTime = responseTime;
            this.status = CallStatus.FAILURE;
        }

        void stopProcessing(final long responseTime) {
            this.responseTime = responseTime;
        }

        CallStatus getStatus() {
            return this.status;
        }

        boolean isSuccessful() {
            return CallStatus.SUCCESS.equals(this.status);
        }

        boolean isFailure() {
            return CallStatus.FAILURE.equals(this.status);
        }

        boolean isUnknown() {
            return CallStatus.UNKNOWN.equals(this.status);
        }

        boolean createdBefore(final long timeInMs) {
            return this.requestTime <= timeInMs;
        }

        boolean createdAfter(final long timeInMs) {
            return this.requestTime >= timeInMs;
        }

        boolean processedBefore(final long timeInMs) {
            return this.responseTime <= timeInMs;
        }

        boolean processedAfter(final long timeInMs) {
            return this.responseTime >= timeInMs;
        }

        boolean createdBeforeButProcessedAfter(final long timeInMs) {
            return createdBefore(timeInMs) && processedAfter(timeInMs);
        }
    }
}
