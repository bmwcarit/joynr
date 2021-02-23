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
package io.joynr.serialize;

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Properties;
import java.util.concurrent.RejectedExecutionException;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonGenerationException;
import com.fasterxml.jackson.databind.JsonMappingException;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

import io.joynr.dispatching.subscription.FileSubscriptionRequestStorage;
import io.joynr.dispatching.subscription.PersistedSubscriptionRequest;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrCommunicationException;
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
import io.joynr.util.MultiMap;
import io.joynr.util.ObjectMapper;
import io.joynr.util.ReflectionUtils;
import joynr.BroadcastSubscriptionRequest;
import joynr.MulticastPublication;
import joynr.OnChangeSubscriptionQos;
import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.OneWayRequest;
import joynr.Reply;
import joynr.Request;
import joynr.SubscriptionPublication;
import joynr.SubscriptionRequest;
import joynr.SubscriptionStop;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.IllegalAccessException;
import joynr.exceptions.MethodInvocationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.tests.testBroadcastInterface;
import joynr.tests.testTypes.ComplexTestType2;
import joynr.tests.testTypes.TestEnum;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.Localisation.GpsFixEnum;
import joynr.types.Localisation.GpsLocation;
import joynr.types.Localisation.GpsPosition;
import joynr.types.ProviderQos;
import joynr.types.TestTypes.TEnum;
import joynr.types.TestTypes.TEverythingExtendedStruct;
import joynr.types.TestTypes.TEverythingMap;
import joynr.types.TestTypes.TIntegerKeyMap;
import joynr.types.TestTypes.TStringKeyMap;
import joynr.types.TestTypes.TStruct;
import joynr.types.TestTypes.Vowel;
import joynr.types.TestTypes.Word;
import joynr.types.Version;

/**
 * This test sends two messages in each direction, containing different TTL values. One with a very high TTL value to
 * ensure it is transfered and one with a very low TTL which should be dropped.
 */
public class SerializationTest {
    private static final int ONE_MINUTE_IN_MS = 60 * 1000;

    private static final Logger logger = LoggerFactory.getLogger(SerializationTest.class);

    private ObjectMapper objectMapper;
    private Injector injector;

    private Long expiryDateMs = System.currentTimeMillis() + ONE_MINUTE_IN_MS;
    private String publicKeyId = "publicKeyId";

    public static final String interfaceName = "interfaceName";

    public interface TestInterface {
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
    public void serializeDeserializeSubscriptionRequests() throws Exception {
        String persistenceFileName = "target/test_persistenceSubscriptionRequests_" + createUuidString();
        String proxyPid = "proxyPid";
        String providerPid = "providerPid";
        String subscriptionId = "subscriptionId";
        String subscribedToName = "subscribedToName";
        SubscriptionQos qos = new OnChangeSubscriptionQos();
        SubscriptionRequest subscriptionRequest = new SubscriptionRequest(subscriptionId, subscribedToName, qos);
        new File(persistenceFileName).delete();
        FileSubscriptionRequestStorage fileSubscriptionRequestStorage = new FileSubscriptionRequestStorage(persistenceFileName);
        fileSubscriptionRequestStorage.persistSubscriptionRequest(proxyPid, providerPid, subscriptionRequest);
        MultiMap<String, PersistedSubscriptionRequest> savedSubscriptionRequests = fileSubscriptionRequestStorage.getSavedSubscriptionRequests();
        assertEquals(1, savedSubscriptionRequests.get(providerPid).size());
        PersistedSubscriptionRequest persistedSubscriptionRequest = savedSubscriptionRequests.get(providerPid)
                                                                                             .iterator()
                                                                                             .next();
        assertEquals(subscriptionRequest, persistedSubscriptionRequest.getSubscriptonRequest());
        assertEquals(proxyPid, persistedSubscriptionRequest.getProxyParticipantId());
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
        TStruct typeDefForTStruct = new TStruct();

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
                                                                        typeDefForTStruct,
                                                                        tBooleanExtended,
                                                                        tStringExtended);
        tmap.put(TEnum.TLITERALA, value);
        tmap.put(TEnum.TLITERALB, value);

        String valueAsString = objectMapper.writeValueAsString(tmap);

        TEverythingMap readValue = objectMapper.readValue(valueAsString, TEverythingMap.class);
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
        OnChangeWithKeepAliveSubscriptionQos qos = new OnChangeWithKeepAliveSubscriptionQos().setMinIntervalMs(1000L)
                                                                                             .setMaxIntervalMs(30000L)
                                                                                             .setValidityMs(60000)
                                                                                             .setAlertAfterIntervalMs(0L)
                                                                                             .setPublicationTtlMs(5000);
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

        Reply reply = new Reply(createUuidString(), (Object) GpsLocations);

        String valueAsString = objectMapper.writeValueAsString(reply);

        System.out.println(valueAsString);

        Reply reply2 = objectMapper.readValue(valueAsString, Reply.class);
        assertEquals(reply, reply2);

    }

    @Test
    public void serializeAndDeserializeOneWayTest() throws Exception {
        GpsLocation gpsLocation = new GpsLocation(1.0d, 2.0d, 0d, GpsFixEnum.MODE2D, 0d, 0d, 0d, 0d, 0l, 0l, 0);
        OneWayRequest oneway = new OneWayRequest("methodName",
                                                 new Object[]{ gpsLocation },
                                                 new Class<?>[]{ GpsLocation.class });

        String valueAsString = objectMapper.writeValueAsString(oneway);

        logger.debug(valueAsString);

        OneWayRequest oneway2 = objectMapper.readValue(valueAsString, OneWayRequest.class);
        assertEquals(oneway, oneway2);
    }

    @Test
    public void serializeAndDeserializeGlobalDiscoveryEntryTest() throws Exception {
        ProviderQos qos = new ProviderQos();
        String address = "someGenericAddress";
        final GlobalDiscoveryEntry[] capInfos = { new GlobalDiscoveryEntry(new Version(47, 11),
                                                                           "domain",
                                                                           "interface",
                                                                           "participantId",
                                                                           qos,
                                                                           System.currentTimeMillis(),
                                                                           expiryDateMs,
                                                                           publicKeyId,
                                                                           address) };

        String writeValueAsString = null;

        writeValueAsString = objectMapper.writeValueAsString(capInfos);
        System.err.println(writeValueAsString);
        assertTrue(writeValueAsString.startsWith("[{\"_typeName\":\"joynr.types.GlobalDiscoveryEntry\""));

        GlobalDiscoveryEntry[] readValue = objectMapper.readValue(writeValueAsString, GlobalDiscoveryEntry[].class);
        assertArrayEquals(capInfos, readValue);

        GlobalDiscoveryEntry globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                             "domain",
                                                                             "interface",
                                                                             "participantId",
                                                                             qos,
                                                                             System.currentTimeMillis(),
                                                                             expiryDateMs,
                                                                             publicKeyId,
                                                                             address);
        writeValueAsString = objectMapper.writeValueAsString(globalDiscoveryEntry);
        System.err.println(writeValueAsString);
        GlobalDiscoveryEntry readCapInfo = objectMapper.readValue(writeValueAsString, GlobalDiscoveryEntry.class);
        assertEquals(globalDiscoveryEntry, readCapInfo);

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

        logger.debug("Serialized TestGpsLocation={}", serializedContent);
        assertTrue(serializedContent.startsWith("{\"_typeName\""));
        GpsLocation gps2 = objectMapper.readValue(serializedContent, GpsLocation.class);
        Assert.assertEquals(gps1, gps2);
    }

    @Test
    public void serializeSubscriptionRequest() throws JsonGenerationException, JsonMappingException, IOException {

        String subscriptionId = "1234";
        String subscribedToName = "myAttribute";
        long expiryDate = System.currentTimeMillis() + 60000;
        SubscriptionQos qos = new OnChangeSubscriptionQos().setMinIntervalMs(0)
                                                           .setExpiryDateMs(expiryDate)
                                                           .setPublicationTtlMs(1000);
        SubscriptionRequest request = new SubscriptionRequest(subscriptionId, subscribedToName, qos);

        String writeValueAsString = objectMapper.writeValueAsString(request);

        SubscriptionRequest receivedRequest = objectMapper.readValue(writeValueAsString, SubscriptionRequest.class);

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
        OnChangeSubscriptionQos qos = new OnChangeSubscriptionQos().setMinIntervalMs(0)
                                                                   .setValidityMs(60000)
                                                                   .setPublicationTtlMs(1000);
        BroadcastSubscriptionRequest broadcastSubscription = new BroadcastSubscriptionRequest(subscriptionId,
                                                                                              subscribedToName,
                                                                                              filterParameters,
                                                                                              qos);

        String writeValueAsString = objectMapper.writeValueAsString(broadcastSubscription);
        System.out.println(writeValueAsString);

        BroadcastSubscriptionRequest receivedbroadcastSubscription = objectMapper.readValue(writeValueAsString,
                                                                                            BroadcastSubscriptionRequest.class);

        Assert.assertEquals(broadcastSubscription, receivedbroadcastSubscription);

    }

    @Test
    public void serializeMulticast() throws JsonGenerationException, JsonMappingException, IOException {

        Object response = new GpsPosition(49.0065, 11.65);
        String multicastId = "1234";
        MulticastPublication publication = new MulticastPublication(Arrays.asList(response), multicastId);

        String writeValueAsString = objectMapper.writeValueAsString(publication);

        MulticastPublication receivedPublication = objectMapper.readValue(writeValueAsString,
                                                                          MulticastPublication.class);
        Assert.assertEquals(publication, receivedPublication);
    }

    @Test
    public void serializePublication() throws JsonGenerationException, JsonMappingException, IOException {

        Object response = new GpsPosition(49.0065, 11.65);
        String subscriptionId = "1234";
        SubscriptionPublication publication = new SubscriptionPublication(Arrays.asList(response), subscriptionId);

        String writeValueAsString = objectMapper.writeValueAsString(publication);

        SubscriptionPublication receivedPublication = objectMapper.readValue(writeValueAsString,
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

        SubscriptionPublication receivedPublication = objectMapper.readValue(writeValueAsString,
                                                                             SubscriptionPublication.class);
        Assert.assertEquals(publication, receivedPublication);
    }

    @Test
    public void serializeSubscriptionStop() throws JsonGenerationException, JsonMappingException, IOException {

        SubscriptionStop stop = new SubscriptionStop("testID");

        String writeValueAsString = objectMapper.writeValueAsString(stop);
        System.out.println(writeValueAsString);

        SubscriptionStop receivedStop = objectMapper.readValue(writeValueAsString, SubscriptionStop.class);
        Assert.assertEquals(stop, receivedStop);
    }

    @Test
    public void serializeRequest() throws JsonGenerationException, JsonMappingException, IOException {

        GpsPosition[] parameter = { new GpsPosition(49.0065, 11.65) };
        Object[] parameters = { parameter };
        Class<?>[] parameterTypes = { GpsPosition[].class };
        Request request = new Request("updateRoute", parameters, parameterTypes);

        String writeValueAsString = objectMapper.writeValueAsString(request);

        Request receivedRequest = objectMapper.readValue(writeValueAsString, Request.class);
        Assert.assertEquals(request, receivedRequest);

    }

    @Test
    public void serializeReply() throws JsonGenerationException, JsonMappingException, IOException {

        Object response = new GpsPosition(49.0065, 11.65);
        Reply reply = new Reply(createUuidString(), response);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);

    }

    @Test
    public void serializeReplyWithCapabilityInfoArray() throws JsonGenerationException, JsonMappingException,
                                                        IOException {

        Object response = new GlobalDiscoveryEntry[]{ new GlobalDiscoveryEntry(new Version(47, 11),
                                                                               "domain",
                                                                               "interface",
                                                                               "participantId",
                                                                               new ProviderQos(),
                                                                               System.currentTimeMillis(),
                                                                               expiryDateMs,
                                                                               publicKeyId,
                                                                               "channelId") };
        Reply reply = new Reply(createUuidString(), response);

        String writeValueAsString = objectMapper.writeValueAsString(reply);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        GlobalDiscoveryEntry[] convertValue = objectMapper.convertValue(receivedReply.getResponse()[0],
                                                                        GlobalDiscoveryEntry[].class);

        Assert.assertArrayEquals((GlobalDiscoveryEntry[]) reply.getResponse()[0], convertValue);

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
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrApplicationExceptionWithoutMessage() throws IOException {

        ApplicationException error = new ApplicationException(TestEnum.TWO);
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrIllegalAccessException() throws IOException {

        IllegalAccessException error = new IllegalAccessException("detail message: JoynrIllegalAccessException");
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithMethodInvocationException() throws IOException {

        MethodInvocationException error = new MethodInvocationException("detail message: MessageInvocationException");
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithProviderRuntimenException() throws IOException {

        ProviderRuntimeException error = new ProviderRuntimeException("detail message: ProviderRuntimeException");
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrRuntimeException() throws IOException {

        JoynrRuntimeException error = new JoynrRuntimeException("detail message: JoynrRuntimeException");
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrRuntimeExceptionWithoutMessage() throws IOException {

        JoynrRuntimeException error = new JoynrRuntimeException();
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrRuntimeExceptionWithCause() throws IOException {

        JoynrRuntimeException error = new JoynrRuntimeException("detail message: JoynrRuntimeExceptionWithCause",
                                                                new IOException("cause message"));
        System.out.println("error: " + error);
        System.out.println("cause: " + ((Throwable) error).getCause());
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithDiscoveryException() throws IOException {

        DiscoveryException error = new DiscoveryException("detail message: DiscoveryException");
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrCommunicationException() throws IOException {

        JoynrCommunicationException error = new JoynrCommunicationException("detail message: JoynrCommunicationException");
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrCommunicationExceptionWithoutMessage() throws IOException {

        JoynrCommunicationException error = new JoynrCommunicationException();
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrIllegalStateException() throws IOException {

        JoynrIllegalStateException error = new JoynrIllegalStateException("detail message: JoynrIllegalStateException");
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrMessageNotSentException() throws IOException {

        JoynrMessageNotSentException error = new JoynrMessageNotSentException("detail message: JoynrMessageNotSentException");
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrRequestInterruptedException() throws IOException {

        JoynrRequestInterruptedException error = new JoynrRequestInterruptedException("detail message: JoynrRequestInterruptedException");
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrSendBufferFullException() throws IOException {

        JoynrSendBufferFullException error = new JoynrSendBufferFullException(new RejectedExecutionException("cause message"));
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrShutdownException() throws IOException {

        JoynrShutdownException error = new JoynrShutdownException("detail message: JoynrShutdownException");
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrTimeoutException() throws IOException {

        JoynrTimeoutException error = new JoynrTimeoutException(42);
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);
        System.out.println(writeValueAsString);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeReplyWithJoynrWaitExpiredException() throws IOException {

        JoynrWaitExpiredException error = new JoynrWaitExpiredException();
        Reply reply = new Reply(createUuidString(), error);

        String writeValueAsString = objectMapper.writeValueAsString(reply);

        Reply receivedReply = objectMapper.readValue(writeValueAsString, Reply.class);
        Assert.assertEquals(reply, receivedReply);
    }

    @Test
    public void serializeSubtype() throws Exception {

        MqttAddress mqttAddress = new MqttAddress("brokerUri", "topic");

        String serializedMqttAddress = objectMapper.writeValueAsString(mqttAddress);

        Address receivedAddress = objectMapper.readValue(serializedMqttAddress, Address.class);
        Assert.assertTrue(receivedAddress instanceof MqttAddress);
        Assert.assertEquals(mqttAddress, receivedAddress);
    }

    @Test
    public void serializeSubtypeInCompoundType() throws Exception {

        MqttAddress mqttAddress = new MqttAddress("brokerUri", "topic");
        GlobalDiscoveryEntry globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                             "domain",
                                                                             "interface",
                                                                             "participantId",
                                                                             new ProviderQos(),
                                                                             System.currentTimeMillis(),
                                                                             expiryDateMs,
                                                                             publicKeyId,
                                                                             objectMapper.writeValueAsString(mqttAddress));

        String serializedGlobalDiscoveryEntry = objectMapper.writeValueAsString(globalDiscoveryEntry);

        GlobalDiscoveryEntry receivedDiscoveryEntry = objectMapper.readValue(serializedGlobalDiscoveryEntry,
                                                                             GlobalDiscoveryEntry.class);
        Assert.assertTrue(objectMapper.readValue(receivedDiscoveryEntry.getAddress(),
                                                 Address.class) instanceof MqttAddress);
        Assert.assertEquals(globalDiscoveryEntry, receivedDiscoveryEntry);
    }
}
