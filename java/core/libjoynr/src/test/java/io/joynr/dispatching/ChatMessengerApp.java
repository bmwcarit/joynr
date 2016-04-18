package io.joynr.dispatching;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import io.joynr.dispatching.rpc.ReplyCaller;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.provider.ProviderContainer;
import io.joynr.runtime.JoynrBaseModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.PropertyLoader;

import java.io.IOException;
import java.util.Properties;
import java.util.UUID;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;

import joynr.Reply;
import joynr.types.ProviderQos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.inject.Injector;

/**
 * Auto responding message client to test interaction with the c++ message client.
 */

public class ChatMessengerApp implements PayloadListener<String>, ReplyCaller {
    private static final long TIME_TO_LIVE = 5000L;
    private static final long CHECK_USER_INPUT_DELAY = 2000;

    private RequestReplyManager requestReplyManager;

    String ownParticipant = UUID.randomUUID().toString();

    private static final Logger logger = LoggerFactory.getLogger(ChatMessengerApp.class);
    private final ScheduledExecutorService scheduler;
    boolean autoRespond = false;
    boolean sendRequests = false;
    private String remoteParticipantId;

    static class ObjTestClass {
        String content;

        public ObjTestClass(String content) {
            this.content = content;
        }

        public String getPayload() {
            return content;
        }
    }

    static class ChatMessengerAppRequestCaller implements RequestCaller {

        public Object respond(Object payload) {
            return "Reply to " + payload.toString();
        }

        @Override
        public ProviderQos getProviderQos() {
            return new ProviderQos();
        }

    }

    public static void main(String[] args) throws JoynrMessageNotSentException {
        String sourceChannel = "";
        String destinationChannel = "";
        String remoteParticipantId = "";
        boolean autoRespond = false;
        boolean sendRequests = false;

        for (int i = 0; i < args.length; i++) {
            if (args[i].contains("--sourceChannel=")) {
                sourceChannel = args[i].substring(16);
            }
            if (args[i].contains("--destinationChannel=")) {
                destinationChannel = args[i].substring(21);
            }
            if (args[i].contains("--destinationId=")) {
                remoteParticipantId = args[i].substring(16);
            }
            if (args[i].contains("--autoRespond")) {
                autoRespond = true;
            }
            if (args[i].contains("--sendRequests")) {
                sendRequests = true;
            }
        }
        if (sourceChannel.length() == 0 || destinationChannel.length() == 0) {
            logger.error("Add sourceChannel/destinationChannel channel to argument list (e.g. --destinationChannel=10");
            return;
        }
        if (remoteParticipantId.length() == 0) {
            logger.error("Add destinationId to argument list.");
        }

        new ChatMessengerApp(sourceChannel, destinationChannel, autoRespond, sendRequests, remoteParticipantId);
    }

    public ChatMessengerApp(String sourceChannel,
                            String destinationChannel,
                            boolean autoRespond,
                            boolean sendRequests,
                            String remoteParticipant) throws JoynrMessageNotSentException {
        this.autoRespond = autoRespond;
        this.sendRequests = sendRequests;
        this.remoteParticipantId = remoteParticipant;
        Properties factoryProperties = PropertyLoader.loadProperties("demo.properties");
        factoryProperties.put(MessagingPropertyKeys.CHANNELID, sourceChannel);
        Injector injector = new JoynrInjectorFactory(new JoynrBaseModule(factoryProperties)).getInjector();

        requestReplyManager = injector.getInstance(RequestReplyManager.class);
        ProviderDirectory providerDirectory = injector.getInstance(ProviderDirectory.class);

        providerDirectory.add(ownParticipant, new ProviderContainer("interfaceName",
                                                                    ChatMessengerApp.class,
                                                                    new ChatMessengerAppRequestCaller(),
                                                                    null));

        try {
            requestReplyManager.sendOneWay(ownParticipant,
                                           remoteParticipant,
                                           "Hello participant " + remoteParticipant,
                                           TIME_TO_LIVE);
        } catch (JoynrSendBufferFullException e) {
            e.printStackTrace();
        } catch (JsonGenerationException e) {
            e.printStackTrace();
        } catch (JsonMappingException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }

        ThreadFactory namedThreadFactory = new ThreadFactoryBuilder().setNameFormat("TestScheduler-%d").build();
        scheduler = Executors.newScheduledThreadPool(1, namedThreadFactory);
        scheduler.scheduleWithFixedDelay(new Runnable() {
            @Override
            public void run() {
                getUserInput();
            }
        }, CHECK_USER_INPUT_DELAY, CHECK_USER_INPUT_DELAY, TimeUnit.MILLISECONDS);

        logger.info("\n--------------------------------------------------------------------------------\n"
                + "Chat DEMO started ...\n" + "Sending to channel: " + destinationChannel + "\n"
                + "Listening to channel: " + sourceChannel
                + "\n--------------------------------------------------------------------------------");
    }

    @edu.umd.cs.findbugs.annotations.SuppressWarnings(value = "DM_DEFAULT_ENCODING", justification = "Just reading key-input, encoding does not matter here")
    private void getUserInput() {
        byte[] buffer = new byte[80];
        String input = "";
        int read;
        long ttl_ms;
        try {
            read = System.in.read(buffer, 0, 80);
            input = new String(buffer, 0, read);
            input = input.replaceAll("\n", "");
            input = input.replaceAll("\r", "");
            System.out.println("Payload entered: " + input);
            System.out.print("Please specify time to live [ms] (default = 5s): ");
            read = System.in.read(buffer, 0, 80);
            if (read > 2) {
                ttl_ms = Integer.parseInt(new String(buffer, 0, read).trim());
            } else {
                ttl_ms = TIME_TO_LIVE;
            }

            requestReplyManager.sendOneWay(ownParticipant, remoteParticipantId, input, ttl_ms);
            System.out.println("____________________________________________________________________________________________________________________");

        } catch (Exception e) {
            System.out.println(e);
        }
    }

    @Override
    public void receive(String payload) {
        if (payload != null) {
            if (autoRespond) {
                String response = "Auto-response on: " + payload.toString();
                try {
                    requestReplyManager.sendOneWay(ownParticipant, remoteParticipantId, response, TIME_TO_LIVE);
                } catch (JoynrSendBufferFullException e) {
                    e.printStackTrace();
                } catch (JsonGenerationException e) {
                    e.printStackTrace();
                } catch (JsonMappingException e) {
                    e.printStackTrace();
                } catch (IOException e) {
                    e.printStackTrace();
                } catch (JoynrMessageNotSentException e) {
                    e.printStackTrace();
                }
            } else {
                System.out.println("Payload received: " + payload.toString());
                System.out.println("____________________________________________________________________________________________________________________");
            }
        } else {
            if (autoRespond) {
                String response = "Empty Payload received!";
                try {
                    requestReplyManager.sendOneWay(ownParticipant, remoteParticipantId, response, TIME_TO_LIVE);
                } catch (JoynrSendBufferFullException e) {
                    e.printStackTrace();
                } catch (JsonGenerationException e) {
                    e.printStackTrace();
                } catch (JsonMappingException e) {
                    e.printStackTrace();
                } catch (IOException e) {
                    e.printStackTrace();
                } catch (JoynrMessageNotSentException e) {
                    e.printStackTrace();
                }
            } else {
                System.out.println("Empty Payload received!");
            }
        }
    }

    @Override
    public Class<String> getNotificationType() {
        return String.class;
    }

    @Override
    public void messageCallBack(Reply payload) {
        System.out.println("____________________________________________________________________________________________________________________");
        System.out.println("Reply received :" + payload);
        System.out.println("____________________________________________________________________________________________________________________");
    }

    @Override
    public void error(Throwable error) {
        System.out.println("____________________________________________________________________________________________________________________");
        System.out.println("ERROR received :" + error.getMessage());
        System.out.println("____________________________________________________________________________________________________________________");

    }

    @Override
    public String getRequestReplyId() {
        return "not implemented";
    }

}
