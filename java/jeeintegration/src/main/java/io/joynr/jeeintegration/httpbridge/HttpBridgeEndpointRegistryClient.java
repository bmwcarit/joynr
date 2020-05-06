/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
package io.joynr.jeeintegration.httpbridge;

import static java.lang.String.format;

import java.io.IOException;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CompletionStage;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import javax.ws.rs.core.MediaType;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys;
import io.joynr.messaging.routing.MessageRouter;

/**
 * A {@link HttpBridgeRegistryClient} implementation which communicates with an Endpoint Registration service in
 * order to register this applications channel (mqtt topic) against the configured application URL.<br>
 * An Endpoint Registration Service is a RESTful service which can be reached via HTTP(s). The URL of the service
 * is configured with the property <code>joynr.jeeintegration.endpointregistry.uri</code>.
 *
 * The registration is performed asynchronously and will also perform up to 10 retries if the registration service can't
 * be reached.
 */
public class HttpBridgeEndpointRegistryClient implements HttpBridgeRegistryClient {

    private static final Logger logger = LoggerFactory.getLogger(HttpBridgeEndpointRegistryClient.class);

    private final HttpClient httpClient;

    private final String endpointRegistryUri;

    private final ScheduledExecutorService scheduledExecutorService;

    public static class EndpointRegistryUriHolder {

        public EndpointRegistryUriHolder() {
        }

        public EndpointRegistryUriHolder(String endpointRegistryUri) {
            this.endpointRegistryUri = endpointRegistryUri;
        }

        @Inject(optional = true)
        @Named(JeeIntegrationPropertyKeys.JEE_INTEGRATION_ENDPOINTREGISTRY_URI)
        private String endpointRegistryUri;

        public String get() {
            return endpointRegistryUri;
        }

    }

    @Inject
    public HttpBridgeEndpointRegistryClient(HttpClient httpClient,
                                            EndpointRegistryUriHolder endpointRegistryUriHolder,
                                            @Named(MessageRouter.SCHEDULEDTHREADPOOL) ScheduledExecutorService scheduledExecutorService) {
        if (logger.isDebugEnabled()) {
            logger.debug(format("Initialising HTTP Bridge Endpoint Registry Client with: %s and %s.",
                                httpClient,
                                endpointRegistryUriHolder.get()));
        }
        this.httpClient = httpClient;
        this.endpointRegistryUri = endpointRegistryUriHolder.get();
        this.scheduledExecutorService = scheduledExecutorService;
    }

    @Override
    public CompletionStage<Void> register(String endpointUrl, String channelId) {
        if (logger.isDebugEnabled()) {
            logger.debug(format("Registering endpoint %s with registry %s for channel ID %s",
                                endpointUrl,
                                endpointRegistryUri,
                                channelId));
        }
        CompletableFuture<Void> result = new CompletableFuture<>();
        scheduleWithRetries(result, 0L, 10, endpointUrl, channelId);
        return result;
    }

    /**
     * This method uses the {@link #scheduledExecutorService} in order to schedule an attempt to register the
     * endpointUrl and channelId (topic) with the registration service HTTP ReST endpoint. If there is a problem with
     * the attempt, then up-to 'remainingRetries' attempts will be made, by calling this method again from within the
     * scheduled action.<br>
     * Once the registration is successful or the number of retries runs out, then the passed in
     * <code>CompletableFuture</code> is completed, so that any subsequent actions / error handling can be triggered.
     *
     * @param result
     *            the completable future which will be completed upon success or we run out of retries.
     * @param delay
     *            the delay for scheduling the action with the {@link #scheduledExecutorService}.
     * @param remainingRetries
     *            the remaining number of retries we can attempt on any errors during registration.
     * @param endpointUrl
     *            the endpoint URL to be registered.
     * @param channelId
     *            the channel ID to be registered.
     */
    private void scheduleWithRetries(CompletableFuture<Void> result,
                                     long delay,
                                     int remainingRetries,
                                     String endpointUrl,
                                     String channelId) {
        if (remainingRetries <= 0) {
            result.completeExceptionally(new JoynrRuntimeException("Unable to register channel. Too many unsuccessful retries."));
        } else {
            logger.debug("Scheduling execution of HTTP post for registering endpoint {} for channel {}.",
                         endpointUrl,
                         channelId);
            scheduledExecutorService.schedule(() -> {
                String topic = channelId;
                HttpPost message = new HttpPost(endpointRegistryUri);
                message.addHeader("Content-Type", MediaType.APPLICATION_JSON);
                try {
                    HttpEntity entity = new StringEntity(format("{\"endpointUrl\": \"%s\", \"topic\": \"%s\"}",
                                                                endpointUrl,
                                                                topic));
                    message.setEntity(entity);
                    boolean executionResult = executeRequest(message);
                    if (executionResult) {
                        result.complete(null);
                        return;
                    }
                } catch (IOException e) {
                    logger.error(format("Unable to register endpoint %s / channel ID %s with broker at %s.",
                                        endpointUrl,
                                        channelId,
                                        endpointRegistryUri),
                                 e);
                }
                scheduleWithRetries(result, 10L, remainingRetries - 1, endpointUrl, channelId);
            }, delay, TimeUnit.SECONDS);
        }
    }

    private boolean executeRequest(HttpPost message) throws IOException {
        if (logger.isDebugEnabled()) {
            logger.debug("About to send message: " + message);
        }
        HttpResponse response = httpClient.execute(message);
        boolean result = false;
        int responseCode = response.getStatusLine().getStatusCode();
        if (responseCode >= 200 && responseCode < 300) {
            result = true;
        } else {
            String errorMessage = format("Response from registration endpoint was not successful.%n%s", response);
            logger.error(errorMessage);
        }
        return result;
    }

}
