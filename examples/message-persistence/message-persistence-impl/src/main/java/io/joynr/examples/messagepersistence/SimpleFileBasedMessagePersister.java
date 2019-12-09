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
package io.joynr.examples.messagepersistence;

import java.io.BufferedReader;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Base64;
import java.util.HashSet;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.persistence.MessagePersister;
import io.joynr.messaging.routing.DelayableImmutableMessage;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.system.RoutingTypes.Address;

public class SimpleFileBasedMessagePersister implements MessagePersister {
    private static final Logger logger = LoggerFactory.getLogger(SimpleFileBasedMessagePersister.class);
    private static final String PERSIST_DIRECTORY = "persisted-messages";
    private static final String PERSISTED_MESSAGE_FILENAME_FORMAT = "%s_%s.message";
    private final ObjectMapper objectMapper;

    public SimpleFileBasedMessagePersister() {
        objectMapper = new ObjectMapper();
        objectMapper.enableDefaultTypingAsProperty(DefaultTyping.JAVA_LANG_OBJECT, "_typeName");
    }

    @Override
    public boolean persist(String messageQueueId, DelayableImmutableMessage message) {
        if (!Message.MessageType.VALUE_MESSAGE_TYPE_REQUEST.equals(Message.MessageType.fromString(message.getMessage()
                                                                                                         .getHeaders()
                                                                                                         .get(Message.HEADER_MSG_TYPE)))) {
            // persisting criteria, e.g. in this example
            // only persist messages that are requests
            return false;
        } else {
            logger.info("Persisting message {} for queue id {}", message.getMessage().getId(), messageQueueId);
            Path directory = Paths.get(PERSIST_DIRECTORY);
            if (!Files.exists(directory)) {
                try {
                    Files.createDirectory(directory);
                } catch (IOException e) {
                    logger.error("Unable to create directory {}", directory);
                    throw new JoynrRuntimeException("Unable to create directory: " + directory.toAbsolutePath());
                }
            }

            String serializedMessage = String.valueOf(message.getDelay(TimeUnit.MILLISECONDS)) + "\n";
            serializedMessage += String.valueOf(message.getRetriesCount()) + "\n";
            serializedMessage += Base64.getEncoder().encodeToString(message.getMessage().getSerializedMessage()) + "\n";

            Path persistedMessageFile = directory.resolve(String.format(PERSISTED_MESSAGE_FILENAME_FORMAT,
                                                                        messageQueueId,
                                                                        message.getMessage().getId()));
            // InProcessAddresses cannot be serialized and do not have to be persisted
            Set<Address> addresses = message.getDestinationAddresses()
                                            .stream()
                                            .filter(address -> !InProcessAddress.class.isInstance(address))
                                            .collect(Collectors.toSet());
            try {
                serializedMessage += objectMapper.writeValueAsString(addresses);
                Files.write(persistedMessageFile, serializedMessage.getBytes("UTF-8"));
            } catch (IOException e) {
                logger.error("Unable to persist {} / {}.", messageQueueId, message, e);
            }
            return true;
        }
    }

    @Override
    public Set<DelayableImmutableMessage> fetchAll(String messageQueueId) {
        Path directory = Paths.get(PERSIST_DIRECTORY);
        if (!Files.exists(directory)) {
            logger.warn("No persisted messages. Returning an empty set.");
            return new HashSet<DelayableImmutableMessage>();
        }

        Set<DelayableImmutableMessage> messages = new HashSet<>();
        try {
            messages = Files.list(directory)
                            .filter(f -> f.getFileName().toString().startsWith(messageQueueId))
                            .map(f -> readMessageFromFile(f))
                            .filter(Objects::nonNull)
                            .collect(Collectors.toSet());
        } catch (IOException e) {
            logger.error("Error scanning folder {}", directory);
        }
        logger.info("Persisted messages loaded. Returning {}", messages);
        return messages;
    }

    private DelayableImmutableMessage readMessageFromFile(Path file) {
        DelayableImmutableMessage deserializedMessage = null;

        try (BufferedReader reader = Files.newBufferedReader(file)) {
            String delayInMsString = reader.readLine();
            String retriesCountString = reader.readLine();
            String base64SerialisedMessage = reader.readLine();
            String destinationAddressesString = reader.readLine();
            if (delayInMsString == null || retriesCountString == null || base64SerialisedMessage == null) {
                throw new IllegalArgumentException(String.format("Invalid message file %s. Content must be three lines.",
                                                                 file));
            }
            ImmutableMessage immutableMessage = new ImmutableMessage(Base64.getDecoder()
                                                                           .decode(base64SerialisedMessage));
            deserializedMessage = new DelayableImmutableMessage(immutableMessage,
                                                                Long.parseLong(delayInMsString),
                                                                objectMapper.readValue(destinationAddressesString,
                                                                                       new TypeReference<Set<Address>>() {
                                                                                       }),
                                                                Integer.parseInt(retriesCountString));
        } catch (IOException | EncodingException | UnsuppportedVersionException e) {
            logger.error("Error reading {}.", file, e);
        }
        return deserializedMessage;
    }

    @Override
    public void remove(String messageQueueId, DelayableImmutableMessage message) {
        logger.debug("Removing from persistence message {} for queue id {}",
                     message.getMessage().getId(),
                     messageQueueId);

        Path directory = Paths.get(PERSIST_DIRECTORY);
        String filename = String.format(PERSISTED_MESSAGE_FILENAME_FORMAT,
                                        messageQueueId,
                                        message.getMessage().getId());
        if (Files.exists(directory)) {
            Path persistedMessageFile = directory.resolve(filename);
            try {
                if (Files.deleteIfExists(persistedMessageFile)) {
                    logger.info("Successfully removed {}", filename);
                } else {
                    logger.debug("No matching persisted message file found for {}", filename);
                }
            } catch (IOException e) {
                logger.error("Error removing {}", persistedMessageFile);
            }
        } else {
            logger.info("No persisted messages directory. Unable to remove persisted message.");
        }
    }
}
