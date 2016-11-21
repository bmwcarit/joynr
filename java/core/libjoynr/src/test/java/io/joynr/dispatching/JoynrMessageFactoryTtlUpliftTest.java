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

import java.lang.reflect.Method;
import java.util.Properties;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.TypeLiteral;
import com.google.inject.multibindings.Multibinder;
import com.google.inject.name.Names;
import com.google.inject.util.Modules;

import io.joynr.common.ExpiryDate;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.MessagingQos;
import joynr.JoynrMessage;
import joynr.Request;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class JoynrMessageFactoryTtlUpliftTest {
    private static final long TTL = 1000;
    private static final long TTL_UPLIFT_MS = 10000;
    JoynrMessageFactory joynrMessageFactory;
    private String fromParticipantId;
    private String toParticipantId;
    private Request request;
    private String payload;
    private ExpiryDate expiryDate;
    private MessagingQos messagingQos;

    private Injector injector;
    private Injector injectorWithTtlUplift;
    private JoynrMessageFactory joynrMessageFactoryWithTtlUplift;

    @Before
    public void setUp() throws NoSuchMethodException, SecurityException {
        fromParticipantId = "sender";
        toParticipantId = "receiver";

        Module defaultModule = Modules.override(new JoynrPropertiesModule(new Properties()))
                                      .with(new JsonMessageSerializerModule(), new AbstractModule() {

                                          @Override
                                          protected void configure() {
                                              requestStaticInjection(Request.class);
                                              Multibinder<JoynrMessageProcessor> joynrMessageProcessorMultibinder = Multibinder.newSetBinder(binder(),
                                                                                                                                             new TypeLiteral<JoynrMessageProcessor>() {
                                                                                                                                             });
                                              joynrMessageProcessorMultibinder.addBinding()
                                                                              .toInstance(new JoynrMessageProcessor() {
                                                                                  @Override
                                                                                  public JoynrMessage process(JoynrMessage joynrMessage) {
                                                                                      joynrMessage.getHeader()
                                                                                                  .put("test", "test");
                                                                                      return joynrMessage;
                                                                                  }
                                                                              });
                                          }

                                      });
        injector = Guice.createInjector(defaultModule);

        joynrMessageFactory = injector.getInstance(JoynrMessageFactory.class);

        Module ttlUpliftModule = Modules.override(defaultModule).with(new AbstractModule() {
            @Override
            protected void configure() {
                bind(Long.class).annotatedWith(Names.named(ConfigurableMessagingSettings.PROPERTY_TTL_UPLIFT_MS))
                                .toInstance(TTL_UPLIFT_MS);
            }

        });
        injectorWithTtlUplift = Guice.createInjector(ttlUpliftModule);
        joynrMessageFactoryWithTtlUplift = injectorWithTtlUplift.getInstance(JoynrMessageFactory.class);

        payload = "payload";
        Method method = TestRequestCaller.class.getMethod("respond", new Class[]{ String.class });
        request = new Request(method.getName(), new String[]{ payload }, method.getParameterTypes());
        messagingQos = new MessagingQos(TTL);
        expiryDate = DispatcherUtils.convertTtlToExpirationDate(messagingQos.getRoundTripTtl_ms());
    }

    @Test
    public void testDefaultTtlUpliftMs() {
        expiryDate = DispatcherUtils.convertTtlToExpirationDate(messagingQos.getRoundTripTtl_ms());
        JoynrMessage message = joynrMessageFactory.createRequest(fromParticipantId,
                                                                 toParticipantId,
                                                                 request,
                                                                 messagingQos);

        long expiryDateValue = expiryDate.getValue();
        JoynrMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }

    @Test
    public void testTtlUpliftMs() {
        expiryDate = DispatcherUtils.convertTtlToExpirationDate(messagingQos.getRoundTripTtl_ms());
        JoynrMessage message = joynrMessageFactoryWithTtlUplift.createRequest(fromParticipantId,
                                                                              toParticipantId,
                                                                              request,
                                                                              messagingQos);

        long expiryDateValue = expiryDate.getValue() + TTL_UPLIFT_MS;
        JoynrMessageFactoryTest.assertExpiryDateEquals(expiryDateValue, message);
    }
}
