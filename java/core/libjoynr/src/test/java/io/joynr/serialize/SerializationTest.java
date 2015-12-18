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

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import io.joynr.common.ExpiryDate;
import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.dispatcher.rpc.ReflectionUtils;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrChannelMissingException;
import io.joynr.exceptions.JoynrChannelNotAssignableException;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrHttpException;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrMessageNotSentException;
import io.joynr.exceptions.JoynrRequestInterruptedException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrSendBufferFullException;
import io.joynr.exceptions.JoynrShutdownException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.exceptions.JoynrWaitExpiredException;
import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.pubsub.SubscriptionQos;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Properties;
import java.util.UUID;
import java.util.concurrent.RejectedExecutionException;

import joynr.BroadcastSubscriptionRequest;
import joynr.JoynrMessage;
import joynr.OnChangeSubscriptionQos;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.OneWay;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.IllegalAccessException;
import joynr.exceptions.MethodInvocationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.tests.testBroadcastInterface;
import joynr.tests.testTypes.ComplexTestType2;
import joynr.tests.testTypes.TestEnum;
import joynr.types.CapabilityInformation;
import joynr.types.ProviderQos;
import joynr.types.Localisation.GpsFixEnum;
import joynr.types.Localisation.GpsLocation;
import joynr.types.Localisation.GpsPosition;
import joynr.types.TestTypes.TDoubleKeyMap;
import joynr.types.TestTypes.TEnum;
import joynr.types.TestTypes.TEverythingExtendedStruct;
import joynr.types.TestTypes.TEverythingMap;
import joynr.types.TestTypes.TIntegerKeyMap;
import joynr.types.TestTypes.TStringKeyMap;
import joynr.types.TestTypes.Vowel;
import joynr.types.TestTypes.Word;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
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
        injector = Guice.createInjector(new JsonMessageSerializerModule(), new AbstractModule() {

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

        GpsLocation gpsLocation = new GpsLocation(1.0d, 2.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0);
        GpsLocation[] gpsLocations = { new GpsLocation(1.0d, 2.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0),
                new GpsLocation(1.0d, 2.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0),
                new GpsLocation(1.0d, 2.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0) };

        TestClass testObject = new TestClass();
        testObject.setMyByte((byte) 4);
        testObject.setObjects(new Object[]{ gpsLocation, "hello" });

        String[][] stringArray = new String[][]{ { "test1", "test2", "test3" }, { "test4", "test5", "test6" } };
        Boolean[] booleanArray = { true, false };
        Boolean[] emptyArray = {};
        Object[] mixedArray = new Object[]{ "one", gpsLocation, stringArray };

        Object[] params = new Object[]{ true, Integer.MAX_VALUE, Long.MAX_VALUE, Double.MAX_VALUE, gpsLocation,
                "param1", gpsLocations, stringArray, booleanArray, emptyArray, mixedArray };
        String[] paramDatatypes = new String[params.length];
        for (int i = 0; i < params.length; i++) {
            paramDatatypes[i] = ReflectionUtils.toDatatypeNames(params[i].getClass())[0];
        }
        Request request = new Request(methodName, params, paramDatatypes, null);

        String valueAsString = objectMapper.writeValueAsString(request);

        System.out.println(valueAsString);

        Request request2 = objectMapper.readValue(valueAsString, Request.class);
        assertEquals(request, request2);

    }

    @Test
    public void serializeDeserializeSimpleMapTest() throws Exception {
        TStringKeyMap tStringMap = new TStringKeyMap();
        tStringMap.put("key1", "value1");
        tStringMap.put("key2", "value2");

        String valueAsString = objectMapper.writeValueAsString(tStringMap);

        TStringKeyMap readValue = objectMapper.readValue(valueAsString, TStringKeyMap.class);
        assertEquals(tStringMap, readValue);
    }

    @Test
    public void serializeDeserializeMapWithEnumKeyTest() throws Exception {
        TEverythingMap tmap = new TEverythingMap();
        Byte tInt8 = 1;
        Byte tUInt8 = 2;
        Short tInt16 = 3;
        Short tUInt16 = 4;
        Integer tInt32 = 5;
        Integer tUInt32 = 6;
        Long tInt64 = 7L;
        Long tUInt64 = 8L;
        Double tDouble = 9.0;
        Float tFloat = 10.0f;
        Boolean tBoolean = false;
        String tString = "tString";
        Byte[] tByteBuffer = { 1, 2, 3 };
        Byte[] tUInt8Array = { 4, 5, 6 };
        TEnum tEnum = TEnum.TLITERALA;
        TEnum[] tEnumArray = { TEnum.TLITERALA, TEnum.TLITERALB };
        String[] tStringArray = { "tStringArray1", "tStringArray2" };
        Word word = new Word(new Vowel[]{ Vowel.A, Vowel.E });
        Word[] wordArray = { word, word };
        Boolean tBooleanExtended = true;
        String tStringExtended = "tStringExtended";
        TStringKeyMap tStringMap = new TStringKeyMap();
        tStringMap.put("key1", "value1");
        tStringMap.put("key2", "value2");

        TEverythingExtendedStruct value = new TEverythingExtendedStruct(tInt8,
                                                                        tUInt8,
                                                                        tInt16,
                                                                        tUInt16,
                                                                        tInt32,
                                                                        tUInt32,
                                                                        tInt64,
                                                                        tUInt64,
                                                                        tDouble,
                                                                        tFloat,
                                                                        tString,
                                                                        tBoolean,
                                                                        tByteBuffer,
                                                                        tUInt8Array,
                                                                        tEnum,
                                                                        tEnumArray,
                                                                        tStringArray,
                                                                        word,
                                                                        wordArray,
                                                                        tStringMap,
                                                                        tBooleanExtended,
                                                                        tStringExtended);
        tmap.put(TEnum.TLITERALA, value);
        tmap.put(TEnum.TLITERALB, value);

        String valueAsString = objectMapper.writeValueAsString(tmap);

        TEverythingMap readValue = objectMapper.readValue(valueAsString, TEverythingMap.class);
        assertEquals(tmap, readValue);
    }

    @Test
    public void serializeMapWithDoubleKeyTest() throws Exception {
        TDoubleKeyMap tmap = new TDoubleKeyMap();

        tmap.put(1d, "value1");
        tmap.put(2d, "value2");

        String valueAsString = objectMapper.writeValueAsString(tmap);
        System.out.println(valueAsString);

        TDoubleKeyMap readValue = objectMapper.readValue(valueAsString, TDoubleKeyMap.class);
        assertEquals(tmap, readValue);
    }

    @Test
    public void serializeMapWithIntegerKeyTest() throws Exception {
        TIntegerKeyMap tmap = new TIntegerKeyMap();

        tmap.put(1, "value1");
        tmap.put(2, "value2");

        String valueAsString = objectMapper.writeValueAsString(tmap);
        System.out.println(valueAsString);

        TIntegerKeyMap readValue = objectMapper.readValue(valueAsString, TIntegerKeyMap.class);
        assertEquals(tmap, readValue);
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

        GpsLocation[] GpsLocations = { new GpsLocation(1.0d, 2.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0),
                new GpsLocation(3.0d, 4.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0),
                new GpsLocation(5.0d, 6.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0) };

        Reply reply = new Reply(UUID.randomUUID().toString(), (Object) GpsLocations);

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
        final CapabilityInformation[] capInfos = { new CapabilityInformation("domain",
                                                                             "interface",
                                                                             qos,
                                                                             "channelId",
                                                                             "participantId") };

        String writeValueAsString = null;

        writeValueAsString = objectMapper.writeValueAsString(capInfos);
        System.err.println(writeValueAsString);
        assertTrue(writeValueAsString.startsWith("[{\"_typeName\":\"joynr.types.CapabilityInformation\""));

        CapabilityInformation[] readValue = objectMapper.readValue(writeValueAsString, CapabilityInformation[].class);
        assertArrayEquals(capInfos, readValue);

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
        SubscriptionPublication publication = new SubscriptionPublication(Arrays.asList(response), subscriptionId);

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
    public void serializePublicationWithJoynrRuntimeException() throws JsonGenerationException, JsonMappingException,
                                                               IOException {

        JoynrRuntimeException error = new JoynrRuntimeException("detail message: JoynrRuntimeException");
        String subscriptionId = "12345";
        SubscriptionPublication publication = new SubscriptionPublication(error, subscriptionId);

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

        GpsPosition[] parameter = { new GpsPosition(49.0065, 11.65) };
        Object[] parameters = { parameter };
        Class<?>[] parameterTypes = { GpsPosition[].class };
        Request request = new Request("updateRoute", parameters, parameterTypes);

        String writeValueAsString = objectMapper.writeValueAsString(request);
        System.err.println(writeValueAsString);

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
        Request receivedRequest = objectMapper.readValue(receivedMessage.getPayload(), Request.class);
        Assert.assertEquals(request, receivedRequest);

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
    public void serializeReplyWithCapabilityInfoArray() throws JsonGenerationException, JsonMappingException,
                                                       IOException {

        Object response = new CapabilityInformation[]{ new CapabilityInformation("domain",
                                                                                 "interface",
                                                                                 new ProviderQos(),
                                                                                 "channelId",
                                                                                 "participantId") };
        Reply reply = new Reply(UUID.randomUUID().toString(), response);

        String writeValueAsString = objectMapper.writeValueAsString(reply);

        JoynrMessage message = new JoynrMessage();
        String type = JoynrMessage.MESSAGE_TYPE_REPLY;
        message.setFrom(UUID.randomUUID().toString());
        message.setTo(UUID.randomUUID().toString());
        message.setType(type);
        message.setExpirationDate(ExpiryDate.fromRelativeTtl(60000));
        message.setPayload(writeValueAsString);
        String messageAsString = objectMapper.writeValueAsString(message);

        JoynrMessage receivedMessage = objectMapper.readValue(messageAsString, JoynrMessage.class);
        Reply receivedReply = objectMapper.readValue(receivedMessage.getPayload(), Reply.class);
        CapabilityInformation[] convertValue = objectMapper.convertValue(receivedReply.getResponse()[0],
                                                                         CapabilityInformation[].class);

        Assert.assertArrayEquals((CapabilityInformation[]) reply.getResponse()[0], convertValue);

        ComplexTestType2[] complexTestType2Array = { new ComplexTestType2(3, 4), new ComplexTestType2(5, 6) };
        ArrayList<ComplexTestType2> customListParam2List = new ArrayList<ComplexTestType2>();
        customListParam2List.add(new ComplexTestType2(3, 4));
        customListParam2List.add(new ComplexTestType2(5, 6));

        ComplexTestType2[] convertValue2 = objectMapper.convertValue(customListParam2List, ComplexTestType2[].class);

        Assert.assertArrayEquals(complexTestType2Array, convertValue2);

    }

    @Test
    public void serializeReplyWithJoynrApplicationException() throws IOException {

        ApplicationException error = new ApplicationException(TestEnum.ONE, "detail message");
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

        ApplicationException error = new ApplicationException(TestEnum.TWO);
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
    public void serializeReplyWithJoynrIllegalAccessException() throws IOException {

        IllegalAccessException error = new IllegalAccessException("detail message: JoynrIllegalAccessException");
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
    public void serializeReplyWithMethodInvocationException() throws IOException {

        MethodInvocationException error = new MethodInvocationException("detail message: MessageInvocationException");
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
    public void serializeReplyWithProviderRuntimenException() throws IOException {

        ProviderRuntimeException error = new ProviderRuntimeException("detail message: ProviderRuntimeException");
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
    public void serializeReplyWithJoynrRuntimeException() throws IOException {

        JoynrRuntimeException error = new JoynrRuntimeException("detail message: JoynrRuntimeException");
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
    public void serializeReplyWithJoynrRuntimeExceptionWithoutMessage() throws IOException {

        JoynrRuntimeException error = new JoynrRuntimeException();
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
    public void serializeReplyWithJoynrRuntimeExceptionWithCause() throws IOException {

        JoynrRuntimeException error = new JoynrRuntimeException("detail message: JoynrRuntimeExceptionWithCause",
                                                                new IOException("cause message"));
        System.out.println("error: " + error);
        System.out.println("cause: " + ((Throwable) error).getCause());
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
    public void serializeReplyWithDiscoveryException() throws IOException {

        DiscoveryException error = new DiscoveryException("detail message: DiscoveryException");
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
    public void serializeReplyWithJoynrChannelNotAssignableException() throws IOException {

        JoynrChannelNotAssignableException error = new JoynrChannelNotAssignableException("detail message: JoynrChannelNotAssignableException",
                                                                                          "CCID");
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
    public void serializeReplyWithJoynrCommunicationException() throws IOException {

        JoynrCommunicationException error = new JoynrCommunicationException("detail message: JoynrCommunicationException");
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
    public void serializeReplyWithJoynrCommunicationExceptionWithoutMessage() throws IOException {

        JoynrCommunicationException error = new JoynrCommunicationException();
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
    public void serializeReplyWithJoynrChannelMissingException() throws IOException {

        JoynrChannelMissingException error = new JoynrChannelMissingException("detail message: JoynrChannelMissingException");
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
    public void serializeReplyWithJoynrHttpException() throws IOException {

        JoynrHttpException error = new JoynrHttpException(404, "detail message: JoynrHttpException");
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
    public void serializeReplyWithJoynrIllegalStateException() throws IOException {

        JoynrIllegalStateException error = new JoynrIllegalStateException("detail message: JoynrIllegalStateException");
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
    public void serializeReplyWithJoynrMessageNotSentException() throws IOException {

        JoynrMessageNotSentException error = new JoynrMessageNotSentException("detail message: JoynrMessageNotSentException");
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
    public void serializeReplyWithJoynrRequestInterruptedException() throws IOException {

        JoynrRequestInterruptedException error = new JoynrRequestInterruptedException("detail message: JoynrRequestInterruptedException");
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
    public void serializeReplyWithJoynrSendBufferFullException() throws IOException {

        JoynrSendBufferFullException error = new JoynrSendBufferFullException(new RejectedExecutionException("cause message"));
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
    public void serializeReplyWithJoynrShutdownException() throws IOException {

        JoynrShutdownException error = new JoynrShutdownException("detail message: JoynrShutdownException");
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
    public void serializeReplyWithJoynrTimeoutException() throws IOException {

        JoynrTimeoutException error = new JoynrTimeoutException(42);
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
    public void serializeReplyWithJoynrWaitExpiredException() throws IOException {

        JoynrWaitExpiredException error = new JoynrWaitExpiredException();
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
