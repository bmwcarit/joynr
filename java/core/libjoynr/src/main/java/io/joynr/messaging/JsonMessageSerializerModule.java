/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.messaging;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.Version;
import com.fasterxml.jackson.databind.DeserializationFeature;
import com.fasterxml.jackson.databind.MapperFeature;
import com.fasterxml.jackson.databind.ObjectMapper.DefaultTyping;
import com.fasterxml.jackson.databind.SerializationFeature;
import com.fasterxml.jackson.databind.jsontype.TypeDeserializer;
import com.fasterxml.jackson.databind.jsontype.TypeResolverBuilder;
import com.fasterxml.jackson.databind.jsontype.impl.LaissezFaireSubTypeValidator;
import com.fasterxml.jackson.databind.module.SimpleModule;
import com.fasterxml.jackson.databind.type.TypeFactory;
import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;

import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRequestInterruptedException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.exceptions.JoynrWaitExpiredException;
import io.joynr.exceptions.SubscriptionException;
import io.joynr.messaging.serialize.ApplicationExceptionDeserializer;
import io.joynr.messaging.serialize.DiscoveryExceptionDeserializer;
import io.joynr.messaging.serialize.IllegalAccessExceptionDeserializer;
import io.joynr.messaging.serialize.JoynrArraySerializer;
import io.joynr.messaging.serialize.JoynrCommunicationExceptionDeserializer;
import io.joynr.messaging.serialize.JoynrEnumSerializer;
import io.joynr.messaging.serialize.JoynrIllegalStateExceptionDeserializer;
import io.joynr.messaging.serialize.JoynrListSerializer;
import io.joynr.messaging.serialize.JoynrMessageNotSentExceptionDeserializer;
import io.joynr.messaging.serialize.JoynrRequestInterruptedExceptionDeserializer;
import io.joynr.messaging.serialize.JoynrRuntimeExceptionDeserializer;
import io.joynr.messaging.serialize.JoynrShutdownExceptionDeserializer;
import io.joynr.messaging.serialize.JoynrTimeoutExceptionDeserializer;
import io.joynr.messaging.serialize.JoynrUntypedObjectDeserializer;
import io.joynr.messaging.serialize.JoynrWaitExpiredExceptionDeserializer;
import io.joynr.messaging.serialize.MethodInvocationExceptionDeserializer;
import io.joynr.messaging.serialize.OneWayRequestDeserializer;
import io.joynr.messaging.serialize.ProviderRuntimeExceptionDeserializer;
import io.joynr.messaging.serialize.RequestDeserializer;
import io.joynr.messaging.serialize.SubscriptionExceptionDeserializer;
import io.joynr.util.ObjectMapper;
import joynr.OneWayRequest;
import joynr.Request;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.IllegalAccessException;
import joynr.exceptions.MethodInvocationException;
import joynr.exceptions.ProviderRuntimeException;

import java.util.Map;

public class JsonMessageSerializerModule extends AbstractModule {

    private final ObjectMapper objectMapper;

    public abstract class ThrowableMixIn {
        // force serialization of detailMessage
        @JsonProperty()
        private String detailMessage;
    }

    public JsonMessageSerializerModule() {
        objectMapper = new ObjectMapper();
        objectMapper.configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false);
        objectMapper.configure(SerializationFeature.WRAP_ROOT_VALUE, false);
        objectMapper.configure(DeserializationFeature.USE_JAVA_ARRAY_FOR_JSON_ARRAY, true);
        // objectMapper.configure(SerializationFeature.ORDER_MAP_ENTRIES_BY_KEYS, true);
        objectMapper.configOverrideSetInclude(Map.class, JsonInclude.Include.NON_NULL, JsonInclude.Include.NON_NULL);
        objectMapper.configure(SerializationFeature.FAIL_ON_EMPTY_BEANS, false);
        objectMapper.configure(MapperFeature.SORT_PROPERTIES_ALPHABETICALLY, true);

        objectMapper.activateDefaultTypingAsProperty(LaissezFaireSubTypeValidator.instance,
                                                     DefaultTyping.JAVA_LANG_OBJECT,
                                                     "_typeName");
        TypeResolverBuilder<?> joynrTypeResolverBuilder = objectMapper.getSerializationConfig()
                                                                      .getDefaultTyper(TypeFactory.unknownType());

        SimpleModule module = new SimpleModule("NonTypedModule", new Version(1, 0, 0, "", "", ""));
        module.addSerializer(new JoynrEnumSerializer());
        module.addSerializer(new JoynrListSerializer());
        module.addSerializer(new JoynrArraySerializer());

        TypeDeserializer typeDeserializer = joynrTypeResolverBuilder.buildTypeDeserializer(objectMapper.getDeserializationConfig(),
                                                                                           TypeFactory.unknownType(),
                                                                                           null);

        module.addDeserializer(Request.class, new RequestDeserializer(objectMapper));
        module.addDeserializer(OneWayRequest.class, new OneWayRequestDeserializer(objectMapper));
        module.addDeserializer(Object.class, new JoynrUntypedObjectDeserializer(typeDeserializer));
        module.addDeserializer(ApplicationException.class, new ApplicationExceptionDeserializer());
        module.addDeserializer(JoynrRuntimeException.class, new JoynrRuntimeExceptionDeserializer());
        module.addDeserializer(ProviderRuntimeException.class, new ProviderRuntimeExceptionDeserializer());
        module.addDeserializer(MethodInvocationException.class, new MethodInvocationExceptionDeserializer());
        module.addDeserializer(JoynrCommunicationException.class, new JoynrCommunicationExceptionDeserializer());
        module.addDeserializer(JoynrTimeoutException.class, new JoynrTimeoutExceptionDeserializer());
        module.addDeserializer(JoynrWaitExpiredException.class, new JoynrWaitExpiredExceptionDeserializer());
        module.addDeserializer(JoynrMessageNotSentException.class, new JoynrMessageNotSentExceptionDeserializer());
        module.addDeserializer(DiscoveryException.class, new DiscoveryExceptionDeserializer());
        module.addDeserializer(JoynrShutdownException.class, new JoynrShutdownExceptionDeserializer());
        module.addDeserializer(IllegalAccessException.class, new IllegalAccessExceptionDeserializer());
        module.addDeserializer(JoynrIllegalStateException.class, new JoynrIllegalStateExceptionDeserializer());
        module.addDeserializer(JoynrRequestInterruptedException.class,
                               new JoynrRequestInterruptedExceptionDeserializer());
        module.addDeserializer(SubscriptionException.class, new SubscriptionExceptionDeserializer());

        module.setMixInAnnotation(Throwable.class, ThrowableMixIn.class);
        objectMapper.registerModule(module);
    }

    @Provides
    @Singleton
    public ObjectMapper provideObjectMapper() {
        return new ObjectMapper(objectMapper);
    }

    @Override
    protected void configure() {
    }

}
