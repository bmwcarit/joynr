package io.joynr.serialize;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import io.joynr.common.ExpiryDate;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.messaging.MessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.pubsub.SubscriptionQos;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Properties;
import java.util.UUID;

import joynr.BroadcastSubscriptionRequest;
import joynr.JoynrApplicationException;
import joynr.JoynrMessage;
import joynr.OnChangeSubscriptionQos;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.OneWay;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.tests.testBroadcastInterface;
import joynr.tests.testtypes.TestEnum;
import joynr.types.CapabilityInformation;
import joynr.types.ProviderQos;
import joynr.types.localisation.GpsFixEnum;
import joynr.types.localisation.GpsLocation;
import joynr.types.localisation.GpsPosition;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

/**
 * This test sends two messages in each direction, containing different TTL values. One with a very high TTL value to
 * ensure it is transfered and one with a very low TTL which should be dropped.
 */
public class SerializationTest {
    private static final Logger LOG = LoggerFactory.getLogger(SerializationTest.class);

    private ObjectMapper objectMapper;
    private Injector injector;

    public static final String interfaceName = "interfaceName";

    public interface TestInterface extends JoynrInterface {
        public static final String INTERFACE_NAME = interfaceName;
    }

    public static class TestClass {
        private Byte myByte;
        private Object[] objects;

        public Object[] getObjects() {
            // copying to suppress findbugs warning
            return Arrays.copyOf(objects, objects.length);
        }

        public void setObjects(Object[] objects) {
            // copying to suppress findbugs warning
            this.objects = Arrays.copyOf(objects, objects.length);
        }

        public Byte getMyByte() {
            return myByte;
        }

        public void setMyByte(Byte myByte) {
            this.myByte = myByte;
        }

        @Override
        public boolean equals(Object obj) {
            if (!(obj instanceof TestClass)) {
                return false;
            }

            TestClass other = (TestClass) obj;

            return Arrays.equals(this.objects, other.objects) && this.getMyByte().equals(other.getMyByte());
        }

        @Override
        public int hashCode() {
            // TODO Auto-generated method stub
            return super.hashCode();
        }
    }

    @Before
    public void setUp() {
        Properties properties = new Properties();
        properties.put(MessagingPropertyKeys.CHANNELID, "testChannelId");
        injector = Guice.createInjector(new MessagingModule(),
                                        new JoynrPropertiesModule(properties),
                                        new AbstractModule() {

                                            @Override
                                            protected void configure() {
                                                requestStaticInjection(Request.class);

                                            }

                                        });

        objectMapper = injector.getInstance(ObjectMapper.class);
    }

    @Test
    public void serializeAndDeserializeRequestWithVariousTypesTest() throws Exception {

        String methodName = "methodName";
        // System.err.println(objectMapper.writeValueAsString(params));

        // GpsLocation GpsLocation = new GpsLocation(1.0, 2.0, new GpsLocation(4.0, 3.0, new String("hello")));
        GpsLocation gpsLocation = new GpsLocation(1.0d, 2.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0);
        List<GpsLocation> gpsLocations = new ArrayList<GpsLocation>();
        gpsLocations.add(gpsLocation);
        gpsLocations.add(gpsLocation);
        gpsLocations.add(gpsLocation);

        TestClass testObject = new TestClass();
        testObject.setMyByte((byte) 4);
        testObject.setObjects(new Object[]{ gpsLocation, "hello" });

        String[] strings = new String[]{ "test1", "test2", "test3" };
        List<String> stringList = Arrays.asList(strings);
        List<Boolean> booleanArray = Arrays.asList(new Boolean[]{ true, false });
        List<Boolean> emptyArray = new ArrayList<Boolean>();
        List<Object> mixedArray = Arrays.asList(new Object[]{ "one", gpsLocation, stringList });

        Object[] params = new Object[]{ true, Integer.MAX_VALUE, Long.MAX_VALUE, Double.MAX_VALUE, gpsLocation,
                "param1", gpsLocations, stringList, booleanArray, emptyArray, mixedArray };
        String[] paramDatatypes = new String[]{ "Boolean", "Integer", "Long", "Double", "joynr.vehicle.GPSPosition",
                "String", "List", "List", "List", "List", "List" };
        Request request = new Request(methodName, params, paramDatatypes, null);

        String valueAsString = objectMapper.writeValueAsString(request);

        System.out.println(valueAsString);

        Request request2 = objectMapper.readValue(valueAsString, Request.class);
        assertEquals(request, request2);

    }

    @Test
    public void serializeEnumTest() throws Exception {
        String valueAsString = objectMapper.writeValueAsString(TestEnum.TWO);
        System.out.println(valueAsString);

        TestEnum readValue = objectMapper.readValue(valueAsString, TestEnum.class);
        assertEquals(TestEnum.TWO, readValue);

    }

    @Test
    public void serializeAndDeserializeSubscriptionQosTest() throws Exception {
        OnChangeWithKeepAliveSubscriptionQos qos = new OnChangeWithKeepAliveSubscriptionQos(1000L,
                                                                                            30000L,
                                                                                            System.currentTimeMillis() + 60000,
                                                                                            0L,
                                                                                            5000);
        String valueAsString = objectMapper.writeValueAsString(qos);
        System.out.println(valueAsString);

        SubscriptionQos readValue = objectMapper.readValue(valueAsString, SubscriptionQos.class);
        assertEquals(qos, readValue);

    }

    @Test
    public void serializeAndDeserializeJsonReplyTest() throws Exception {

        GpsLocation GpsLocation = new GpsLocation(1.0d, 2.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0);
        List<GpsLocation> GpsLocations = new ArrayList<GpsLocation>();
        GpsLocations.add(GpsLocation);
        GpsLocations.add(GpsLocation);
        GpsLocations.add(GpsLocation);

        Reply reply = new Reply(UUID.randomUUID().toString(), GpsLocations);

        String valueAsString = objectMapper.writeValueAsString(reply);

        System.out.println(valueAsString);

        Reply reply2 = objectMapper.readValue(valueAsString, Reply.class);
        assertEquals(reply, reply2);

    }

    @Test
    public void serializeAndDeserializeOneWayTest() throws Exception {

        GpsLocation GpsLocation = new GpsLocation(1.0d, 2.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0);
        OneWay oneway = new OneWay(GpsLocation);

        String valueAsString = objectMapper.writeValueAsString(oneway);

        System.out.println(valueAsString);

        OneWay oneway2 = objectMapper.readValue(valueAsString, OneWay.class);
        assertEquals(oneway, oneway2);

    }

    @Test
    public void serializeAndDeserializeCapabilityInformationTest() throws Exception {
        ProviderQos qos = new ProviderQos();
        final List<CapabilityInformation> capInfoList = new ArrayList<CapabilityInformation>();

        capInfoList.add(new CapabilityInformation("domain", "interface", qos, "channelId", "participantId"));
        capInfoList.add(new CapabilityInformation("domain", "interface", qos, "channelId", "participantId"));
        capInfoList.add(new CapabilityInformation("domain", "interface", qos, "channelId", "participantId"));

        String writeValueAsString = null;

        writeValueAsString = objectMapper.writeValueAsString(capInfoList.toArray());
        System.err.println(writeValueAsString);
        assertTrue(writeValueAsString.startsWith("[{\"_typeName\":\"joynr.types.CapabilityInformation\""));

        List<CapabilityInformation> readValue = objectMapper.readValue(writeValueAsString,
                                                                       new TypeReference<List<CapabilityInformation>>() {
                                                                       });
        assertEquals(capInfoList, readValue);

        CapabilityInformation capabilityInformation = new CapabilityInformation("domain",
                                                                                "interface",
                                                                                qos,
                                                                                "channelId",
                                                                                "participantId");
        writeValueAsString = objectMapper.writeValueAsString(capabilityInformation);
        // assertTrue(writeValueAsString.startsWith("{\"_typeName\":\"joynr.types.CapabilityInformation\""));
        System.err.println(writeValueAsString);
        CapabilityInformation readCapInfo = objectMapper.readValue(writeValueAsString, CapabilityInformation.class);
        assertEquals(capabilityInformation, readCapInfo);

    }

    @Test
    public void serializeJoynrMessageTest() throws Exception {

        ExpiryDate expirationDate = ExpiryDate.fromRelativeTtl(1000);
        String payload = "/67589ß8zhkbvöäüÜÖLÖLkjöjhljvhl汉字/漢字";
        JoynrMessage message = new JoynrMessage();
        String type = "TESTTYPE";
        message.setType(type);
        message.setExpirationDate(expirationDate);
        message.setPayload(payload);

        String writeValueAsString = objectMapper.writeValueAsString(message);

        JoynrMessage receivedMessage = objectMapper.readValue(writeValueAsString, JoynrMessage.class);

        Assert.assertEquals(message, receivedMessage);
    }

    @Test
    public void serialzeAndDeserializeTestGpsLocation() throws JsonGenerationException, JsonMappingException,
                                                       IOException {
        GpsLocation gps1 = new GpsLocation();
        gps1.setGpsFix(GpsFixEnum.MODE3D);
        gps1.setLongitude(1.1);
        gps1.setLatitude(2.2);
        gps1.setAltitude(3.3);
        gps1.setTime(17);

        String serializedContent = objectMapper.writeValueAsString(gps1);

        LOG.debug("Serialized TestGpsLocation={}", serializedContent);
        assertTrue(serializedContent.startsWith("{\"_typeName\""));
        GpsLocation gps2 = objectMapper.readValue(serializedContent, GpsLocation.class);
        Assert.assertEquals(gps1, gps2);
    }

    @Test
    public void serializeSubscriptionRequest() throws JsonGenerationException, JsonMappingException, IOException {

        String subscriptionId = "1234";
        String subscribedToName = "myAttribute";
        long expiryDate = System.currentTimeMillis() + 60000;
        SubscriptionQos qos = new OnChangeSubscriptionQos(0, expiryDate, 1000);
        SubscriptionRequest request = new SubscriptionRequest(subscriptionId, subscribedToName, qos);

        String writeValueAsString = objectMapper.writeValueAsString(request);
        System.out.println(writeValueAsString);

        JoynrMessage message = new JoynrMessage();
        String type = JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_REQUEST;
        message.setFrom(UUID.randomUUID().toString());
        message.setTo(UUID.randomUUID().toString());
        message.setType(type);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(60000));
        message.setPayload(writeValueAsString);
        String messageAsString = objectMapper.writeValueAsString(message);
        System.out.println(messageAsString);

        JoynrMessage receivedMessage = objectMapper.readValue(messageAsString, JoynrMessage.class);
        SubscriptionRequest receivedRequest = objectMapper.readValue(receivedMessage.getPayload(),
                                                                     SubscriptionRequest.class);

        Assert.assertEquals(request, receivedRequest);

    }

    @Test
    public void serializeBroadcastSubscriptionRequest() throws JsonGenerationException, JsonMappingException,
                                                       IOException {

        String subscriptionId = "1234";
        String subscribedToName = "myEvent";
        testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters filterParameters = new testBroadcastInterface.LocationUpdateWithSpeedSelectiveBroadcastFilterParameters();
        filterParameters.setCountry("Germany");
        filterParameters.setStartTime("4:00");
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos(0, System.currentTimeMillis() + 60000, 1000);
        BroadcastSubscriptionRequest broadcastSubscription = new BroadcastSubscriptionRequest(subscriptionId,
                                                                                              subscribedToName,
                                                                                              filterParameters,
                                                                                              qos);

        String writeValueAsString = objectMapper.writeValueAsString(broadcastSubscription);
        System.out.println(writeValueAsString);

        JoynrMessage message = new JoynrMessage();
        String type = JoynrMessage.MESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST;
        message.setFrom(UUID.randomUUID().toString());
        message.setTo(UUID.randomUUID().toString());
        message.setType(type);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(60000));
        message.setPayload(writeValueAsString);
        String messageAsString = objectMapper.writeValueAsString(message);
        System.out.println(messageAsString);

        JoynrMessage receivedMessage = objectMapper.readValue(messageAsString, JoynrMessage.class);
        BroadcastSubscriptionRequest receivedbroadcastSubscription = objectMapper.readValue(receivedMessage.getPayload(),
                                                                                            BroadcastSubscriptionRequest.class);

        Assert.assertEquals(broadcastSubscription, receivedbroadcastSubscription);

    }

    @Test
    public void serializePublication() throws JsonGenerationException, JsonMappingException, IOException {

        Object response = new GpsPosition(49.0065, 11.65);
        String subscriptionId = "1234";
        SubscriptionPublication publication = new SubscriptionPublication(response, subscriptionId);

        String writeValueAsString = objectMapper.writeValueAsString(publication);

        JoynrMessage message = new JoynrMessage();
        String type = JoynrMessage.MESSAGE_TYPE_PUBLICATION;
        message.setFrom(UUID.randomUUID().toString());
        message.setTo(UUID.randomUUID().toString());
        message.setType(type);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(60000));
        message.setPayload(writeValueAsString);
        String messageAsString = objectMapper.writeValueAsString(message);
        System.out.println(messageAsString);

        System.out.println(writeValueAsString);
        JoynrMessage receivedMessage = objectMapper.readValue(messageAsString, JoynrMessage.class);
        SubscriptionPublication receivedPublication = objectMapper.readValue(receivedMessage.getPayload(),
                                                                             SubscriptionPublication.class);
        Assert.assertEquals(publication, receivedPublication);
    }

    @Test
    public void serializeSubscriptionStop() throws JsonGenerationException, JsonMappingException, IOException {

        SubscriptionStop stop = new SubscriptionStop("testID");

        String writeValueAsString = objectMapper.writeValueAsString(stop);
        System.out.println(writeValueAsString);

        JoynrMessage message = new JoynrMessage();
        String type = JoynrMessage.MESSAGE_TYPE_SUBSCRIPTION_STOP;
        message.setFrom(UUID.randomUUID().toString());
        message.setTo(UUID.randomUUID().toString());
        message.setType(type);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(60000));
        message.setPayload(writeValueAsString);
        String messageAsString = objectMapper.writeValueAsString(message);
        System.out.println(messageAsString);

        JoynrMessage receivedMessage = objectMapper.readValue(messageAsString, JoynrMessage.class);
        SubscriptionStop receivedStop = objectMapper.readValue(receivedMessage.getPayload(), SubscriptionStop.class);
        Assert.assertEquals(stop, receivedStop);
    }

    @Test
    public void serializeRequest() throws JsonGenerationException, JsonMappingException, IOException {

        Object parameter = new GpsPosition(49.0065, 11.65);
        Object[] parameters = { parameter };
        Class<?>[] parameterTypes = { GpsPosition.class };
        Request request = new Request("updateRoute", parameters, parameterTypes);

        String writeValueAsString = objectMapper.writeValueAsString(request);
        System.out.println(writeValueAsString);

        JoynrMessage message = new JoynrMessage();
        String type = JoynrMessage.MESSAGE_TYPE_REQUEST;
        message.setFrom(UUID.randomUUID().toString());
        message.setTo(UUID.randomUUID().toString());
        message.setType(type);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(60000));
        message.setPayload(writeValueAsString);
        String messageAsString = objectMapper.writeValueAsString(message);
        System.out.println(messageAsString);

        JoynrMessage receivedMessage = objectMapper.readValue(messageAsString, JoynrMessage.class);
        Request receivedReply = objectMapper.readValue(receivedMessage.getPayload(), Request.class);
        Assert.assertEquals(request, receivedReply);

    }

    @Test
    public void serializeReply() throws JsonGenerationException, JsonMappingException, IOException {

        Object response = new GpsPosition(49.0065, 11.65);
        Reply reply = new Reply(UUID.randomUUID().toString(), response);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        JoynrMessage message = new JoynrMessage();
        String type = JoynrMessage.MESSAGE_TYPE_REPLY;
        message.setFrom(UUID.randomUUID().toString());
        message.setTo(UUID.randomUUID().toString());
        message.setType(type);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(60000));
        message.setPayload(writeValueAsString);
        String messageAsString = objectMapper.writeValueAsString(message);
        System.out.println(messageAsString);

        JoynrMessage receivedMessage = objectMapper.readValue(messageAsString, JoynrMessage.class);
        Reply receivedReply = objectMapper.readValue(receivedMessage.getPayload(), Reply.class);
        Assert.assertEquals(reply, receivedReply);

    }

    @Test
    public void serializeReplyWithJoynrApplicationException() throws IOException {

        JoynrApplicationException error = new JoynrApplicationException(TestEnum.ONE, "detail message");
        Reply reply = new Reply(UUID.randomUUID().toString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        JoynrMessage message = new JoynrMessage();
        String type = JoynrMessage.MESSAGE_TYPE_REPLY;
        message.setFrom(UUID.randomUUID().toString());
        message.setTo(UUID.randomUUID().toString());
        message.setType(type);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(60000));
        message.setPayload(writeValueAsString);
        String messageAsString = objectMapper.writeValueAsString(message);
        System.out.println(messageAsString);

        JoynrMessage receivedMessage = objectMapper.readValue(messageAsString, JoynrMessage.class);
        Reply receivedReply = objectMapper.readValue(receivedMessage.getPayload(), Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrApplicationExceptionWithoutMessage() throws IOException {

        JoynrApplicationException error = new JoynrApplicationException(TestEnum.TWO);
        Reply reply = new Reply(UUID.randomUUID().toString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        JoynrMessage message = new JoynrMessage();
        String type = JoynrMessage.MESSAGE_TYPE_REPLY;
        message.setFrom(UUID.randomUUID().toString());
        message.setTo(UUID.randomUUID().toString());
        message.setType(type);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(60000));
        message.setPayload(writeValueAsString);
        String messageAsString = objectMapper.writeValueAsString(message);
        System.out.println(messageAsString);

        JoynrMessage receivedMessage = objectMapper.readValue(messageAsString, JoynrMessage.class);
        Reply receivedReply = objectMapper.readValue(receivedMessage.getPayload(), Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

}
