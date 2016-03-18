//CHECKSTYLE:OFF ignore FileLengthCheck
package io.joynr.test.interlanguage;

//CHECKSTYLE:ON

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

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Properties;

import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.interlanguagetest.Enumeration;
import joynr.interlanguagetest.TestInterface.MethodWithAnonymousErrorEnumErrorEnum;
import joynr.interlanguagetest.TestInterface.MethodWithExtendedErrorEnumErrorEnum;
import joynr.interlanguagetest.TestInterfaceAsync.MethodWithMultipleStructParametersCallback;
import joynr.interlanguagetest.TestInterfaceAsync.MethodWithMultipleStructParametersFuture;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithFilteringBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithFilteringBroadcastFilterParameters;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithMultipleArrayParametersBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithMultipleEnumerationParametersBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithMultiplePrimitiveParametersBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithMultipleStructParametersBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithSingleArrayParameterBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithSingleEnumerationParameterBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithSinglePrimitiveParameterBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceBroadcastInterface.BroadcastWithSingleStructParameterBroadcastAdapter;
import joynr.interlanguagetest.TestInterfaceProxy;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultipleArrayParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultipleEnumParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultiplePrimitiveParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultipleStructParametersReturned;
import joynr.interlanguagetest.TestInterfaceSync.OverloadedMethodOverloadedMethod1Returned;
import joynr.interlanguagetest.TestInterfaceSync.OverloadedMethodWithSelectorOverloadedMethodWithSelector1Returned;
import joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray;
import joynr.interlanguagetest.namedTypeCollection2.BaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.BaseStructWithoutElements;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedEnumerationWithPartlyDefinedValues;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedErrorEnumTc;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedEnumeration;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedInterfaceEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;
import joynr.interlanguagetest.namedTypeCollection2.MapStringString;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.Module;
import com.google.inject.name.Named;
import com.google.inject.util.Modules;

import edu.umd.cs.findbugs.annotations.SuppressWarnings;

@SuppressWarnings("checkstyle:filelength")
public class IltConsumerApplication extends AbstractJoynrApplication {
    private static final Logger LOG = LoggerFactory.getLogger(IltConsumerApplication.class);
    public static final String INTER_LANGUAGE_PROVIDER_DOMAIN = "inter-language-test.provider.domain";
    private static final String STATIC_PERSISTENCE_FILE = "java-consumer.persistence_file";

    @Inject
    @Named(INTER_LANGUAGE_PROVIDER_DOMAIN)
    private String providerDomain;
    private TestInterfaceProxy testInterfaceProxy;
    @Inject
    private ObjectMapper objectMapper;

    /**
     * Main method.
     *
     * @throws IOException
     */
    public static void main(String[] args) throws IOException {
        // run application from cmd line using Maven:
        // mvn exec:java -Dexec.mainClass="io.joynr.demo.IltonsumerApplication" -Dexec.args="<provider-domain>"
        if (args.length != 1 && args.length != 2) {
            LOG.error("USAGE: java {} <provider-domain> [websocket]", IltConsumerApplication.class.getName());
            return;
        }
        String providerDomain = args[0];
        LOG.debug("Searching for providers on domain \"{}\"", providerDomain);
        Properties joynrConfig = new Properties();
        Module runtimeModule = getRuntimeModule(args, joynrConfig);
        LOG.debug("Using the following runtime module: " + runtimeModule.getClass().getSimpleName());
        LOG.debug("Searching for providers on domain \"{}\"", providerDomain);

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, "inter_language_test_consumer_local_domain");
        Properties appConfig = new Properties();
        appConfig.setProperty(INTER_LANGUAGE_PROVIDER_DOMAIN, providerDomain);

        JoynrApplication myConsumerApp = new JoynrInjectorFactory(joynrConfig, runtimeModule).createApplication(new JoynrApplicationModule(IltConsumerApplication.class,
                                                                                                                                           appConfig));
        myConsumerApp.run();

        myConsumerApp.shutdown();
    }

    private static Module getRuntimeModule(String[] args, Properties joynrConfig) {
        Module runtimeModule;
        if (args.length >= 2) {
            String transport = args[1].toLowerCase();
            if (transport.contains("websocket")) {
                joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
                joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "4242");
                joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
                joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
                runtimeModule = new LibjoynrWebSocketRuntimeModule();
            } else {
                runtimeModule = new CCInProcessRuntimeModule();
            }

            Module backendTransportModules = Modules.EMPTY_MODULE;
            if (transport.contains("http")) {
                backendTransportModules = Modules.combine(backendTransportModules, new AtmosphereMessagingModule());
            }

            if (transport.contains("mqtt")) {
                backendTransportModules = Modules.combine(backendTransportModules, new MqttPahoModule());
            }

            return Modules.override(runtimeModule).with(backendTransportModules);
        }

        return Modules.override(new CCInProcessRuntimeModule()).with(new AtmosphereMessagingModule());
    }

    @Override
    @SuppressWarnings(value = "DM_EXIT", justification = "WORKAROUND to be removed")
    public void shutdown() {
        // Add any clean up code here for your application.
        runtime.shutdown(true);

        // TODO currently there is a bug preventing all threads being stopped
        // WORKAROUND
        try {
            Thread.sleep(3000);
        } catch (InterruptedException e) {
            // do nothing; exiting application
        }
        System.exit(0);
    }

    /*
     * SYNCHRONOUS METHOD CALLS
     */

    // no check possible other than handling exceptions
    public boolean callMethodWithoutParameters() {
        LOG.info("callMethodWithoutParameters");
        try {
            testInterfaceProxy.methodWithoutParameters();
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithoutParameters - FAILED");
            return false;
        }
        LOG.info("callMethodWithoutParameters - OK");
        return true;
    }

    public boolean callMethodWithoutInputParameter() {
        LOG.info("callMethodWithoutInputParameter");
        try {
            Boolean b;
            b = testInterfaceProxy.methodWithoutInputParameter();
            // expect true to be returned
            if (!b) {
                LOG.info("callMethodWithoutInputParameter - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithoutInputParameter - FAILED");
            return false;
        }
        LOG.info("callMethodWithoutInputParameter - OK");
        return true;
    }

    public boolean callMethodWithoutOutputParameter() {
        LOG.info("callMethodWithoutOutputParameter");
        try {
            boolean arg = false;
            testInterfaceProxy.methodWithoutOutputParameter(arg);
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithoutOutputParameter - FAILED");
            return false;
        }
        LOG.info("callMethodWithoutOutputParameter - OK");
        return true;
    }

    public boolean callMethodWithSinglePrimitiveParameters() {
        LOG.info("callMethodWithSinglePrimitiveParameters");
        try {
            // short arg = (short)65535;
            short arg = (short) 32767;
            String result;
            result = testInterfaceProxy.methodWithSinglePrimitiveParameters(arg);
            if (result == null) {
                LOG.info("callMethodWithSinglePrimitiveParameters - got null as result");
                LOG.info("callMethodWithSinglePrimitiveParameters - FAILED");
            }
            if (!result.equals(new Integer(Short.toUnsignedInt(arg)).toString())) {
                LOG.info("callMethodWithSinglePrimitiveParameters - invalid result");
                LOG.info("callMethodWithSinglePrimitiveParameters - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithSinglePrimitiveParameters - FAILED");
            return false;
        }
        LOG.info("callMethodWithSinglePrimitiveParameters - OK");
        return true;
    }

    public boolean callmethodWithSingleMapParameters() {
        LOG.info("callmethodWithSingleMapParameters");
        try {
            MapStringString mapArg = new MapStringString();
            mapArg.put("keyString1", "valueString1");
            mapArg.put("keyString2", "valueString2");
            mapArg.put("keyString3", "valueString3");
            MapStringString result;
            result = testInterfaceProxy.methodWithSingleMapParameters(mapArg);
            if (result == null) {
                LOG.info("callmethodWithSingleMapParameters - got null as result");
                LOG.info("callmethodWithSingleMapParameters - FAILED");
            }
            MapStringString expected = new MapStringString();
            expected.put("valueString1", "keyString1");
            expected.put("valueString2", "keyString2");
            expected.put("valueString3", "keyString3");
            if (!result.equals(expected)) {
                LOG.info("callmethodWithSingleMapParameters - invalid result");
                LOG.info("callmethodWithSingleMapParameters - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callmethodWithSingleMapParameters - FAILED");
            return false;
        }
        LOG.info("callmethodWithSingleMapParameters - OK");
        return true;
    }

    // problems might be to expect wrt. float or double comparison
    public boolean callMethodWithMultiplePrimitiveParameters() {
        LOG.info("callMethodWithMultiplePrimitiveParameters");
        try {
            int arg1 = 2147483647;
            float arg2 = 47.11f;
            boolean arg3 = false;
            MethodWithMultiplePrimitiveParametersReturned result;
            result = testInterfaceProxy.methodWithMultiplePrimitiveParameters(arg1, arg2, arg3);
            // It might be difficult to compare a float since number representation
            // might be different.
            if (result == null) {
                LOG.info("callMethodWithMultiplePrimitiveParameters - got null as result");
                LOG.info("callMethodWithMultiplePrimitiveParameters - FAILED");
                return false;
            }
            if (!IltUtil.cmpDouble(result.doubleOut, arg2) || !result.stringOut.equals(Integer.toString(arg1))) {
                LOG.info("callMethodWithMultiplePrimitiveParameters - int32Arg = " + arg1);
                LOG.info("callMethodWithMultiplePrimitiveParameters - input floatArg= " + arg2);
                LOG.info("callMethodWithMultiplePrimitiveParameters - result.doubleOut = " + result.doubleOut);
                LOG.info("callMethodWithMultiplePrimitiveParameters - result.stringOut = " + result.stringOut);
                LOG.info("callMethodWithMultiplePrimitiveParameters - FAILED");
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithMultiplePrimitiveParameters - FAILED");
            return false;
        }
        LOG.info("callMethodWithMultiplePrimitiveParameters - OK");
        return true;
    }

    public boolean callMethodWithSingleArrayParameters() {
        LOG.info("callMethodWithSingleArrayParameters");
        try {
            Double[] arg = IltUtil.createDoubleArray();
            String[] result;

            result = testInterfaceProxy.methodWithSingleArrayParameters(arg);
            if (result == null) {
                LOG.info("callMethodWithSingleArrayParameters - got null as result");
                LOG.info("callMethodWithSingleArrayParameters - FAILED");
                return false;
            }
            if (!IltUtil.checkStringArray(result)) {
                LOG.info("callMethodWithSingleArrayParameters - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithSingleArrayParameters - FAILED");
            return false;
        }
        LOG.info("callMethodWithSingleArrayParameters - OK");
        return true;
    }

    public boolean callMethodWithMultipleArrayParameters() {
        LOG.info("callMethodWithMultipleArrayParameters");
        try {
            String[] arg1 = IltUtil.createStringArray();
            Byte[] arg2 = IltUtil.createByteArray();
            ExtendedInterfaceEnumerationInTypeCollection[] arg3 = IltUtil.createExtendedInterfaceEnumerationInTypeCollectionArray();
            StructWithStringArray[] arg4 = IltUtil.createStructWithStringArrayArray();
            MethodWithMultipleArrayParametersReturned result;

            result = testInterfaceProxy.methodWithMultipleArrayParameters(arg1, arg2, arg3, arg4);

            if (result == null) {
                LOG.info("callMethodWithMultipleArrayParameters - got null as result");
                LOG.info("callMethodWithMultipleArrayParameters - FAILED");
                return false;
            }
            if (!IltUtil.checkUInt64Array(result.uInt64ArrayOut)) {
                LOG.info("callMethodWithMultipleArrayParameters - invalid stringArrayArg");
                LOG.info("callMethodWithMultipleArrayParameters - FAILED");
                return false;
            }
            if (result.structWithStringArrayArrayOut.length != 2) {
                LOG.info("callMethodWithMultipleArrayParameters - invalid length of structWithStringArrayArrayOut");
                LOG.info("callMethodWithMultipleArrayParameters - FAILED");
            }
            if (!IltUtil.checkStructWithStringArray(result.structWithStringArrayArrayOut[0])) {
                LOG.info("callMethodWithMultipleArrayParameters - invalid structWithStringArrayArrayOut[0]");
                LOG.info("callMethodWithMultipleArrayParameters - FAILED");
            }
            if (!IltUtil.checkStructWithStringArray(result.structWithStringArrayArrayOut[1])) {
                LOG.info("callMethodWithMultipleArrayParameters - invalid structWithStringArrayArrayOut[1]");
                LOG.info("callMethodWithMultipleArrayParameters - FAILED");
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithMultipleArrayParameters - FAILED");
            return false;
        }
        LOG.info("callMethodWithMultipleArrayParameters - OK");
        return true;
    }

    public boolean callMethodWithSingleEnumParameters() {
        LOG.info("callMethodWithSingleEnumParameters");
        try {
            ExtendedEnumerationWithPartlyDefinedValues enumerationArg = ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES;
            ExtendedTypeCollectionEnumerationInTypeCollection result;

            result = testInterfaceProxy.methodWithSingleEnumParameters(enumerationArg);
            if (result == null) {
                LOG.info("callMethodWithSingleEnumParameters - got null as result");
                LOG.info("callMethodWithSingleEnumParameters - FAILED");
                return false;
            }
            if (result != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
                LOG.info("callMethodWithSingleEnumParameters - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithSingleEnumParameters - FAILED");
            return false;
        }
        LOG.info("callMethodWithSingleEnumParameters - OK");
        return true;
    }

    public boolean callMethodWithMultipleEnumParameters() {
        LOG.info("callMethodWithMultipleEnumParameters");
        try {
            joynr.interlanguagetest.Enumeration enumerationArg;
            ExtendedTypeCollectionEnumerationInTypeCollection extendedEnumerationArg;
            MethodWithMultipleEnumParametersReturned result;

            enumerationArg = joynr.interlanguagetest.Enumeration.ENUM_0_VALUE_3;
            extendedEnumerationArg = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;

            result = testInterfaceProxy.methodWithMultipleEnumParameters(enumerationArg, extendedEnumerationArg);
            if (result == null) {
                LOG.info("callMethodWithMultipleEnumParameters - got null as result");
                LOG.info("callMethodWithMultipleEnumParameters - FAILED");
                return false;
            }
            if (result.enumerationOut != Enumeration.ENUM_0_VALUE_1
                    || result.extendedEnumerationOut != ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
                LOG.info("callMethodWithMultipleEnumParameters - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithMultipleEnumParameters - FAILED");
            return false;
        }
        LOG.info("callMethodWithMultipleEnumParameters - OK");
        return true;
    }

    public boolean callMethodWithSingleStructParameters() {
        LOG.info("callMethodWithSingleStructParameters");
        try {
            ExtendedBaseStruct arg = IltUtil.createExtendedBaseStruct();
            ExtendedStructOfPrimitives result;

            result = testInterfaceProxy.methodWithSingleStructParameters(arg);
            if (result == null) {
                LOG.info("callMethodWithSingleStructParameters - got null as result");
                LOG.info("callMethodWithSingleStructParameters - FAILED");
                return false;
            }
            if (!IltUtil.checkExtendedStructOfPrimitives(result)) {
                LOG.info("callMethodWithSingleStructParameters 1 - FAILED");
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithSingleStructParameters exception - FAILED");
            return false;
        }
        LOG.info("callMethodWithSingleStructParameters - OK");
        return true;
    }

    public boolean callMethodWithMultipleStructParameters() {
        LOG.info("callMethodWithMultipleStructParameters");
        try {
            MethodWithMultipleStructParametersReturned result;

            // setup input parameters
            ExtendedStructOfPrimitives extendedStructOfPrimitivesOut = IltUtil.createExtendedStructOfPrimitives();
            BaseStruct baseStructOut = IltUtil.createBaseStruct();

            result = testInterfaceProxy.methodWithMultipleStructParameters(extendedStructOfPrimitivesOut, baseStructOut);
            if (result == null) {
                LOG.info("callMethodWithMultipleStructParameters - got null as result");
                LOG.info("callMethodWithMultipleStructParameters - FAILED");
                return false;
            }
            if (!IltUtil.checkBaseStructWithoutElements(result.baseStructWithoutElementsOut)) {
                LOG.info("callMethodWithMultipleStructParameters - FAILED");
                return false;
            }

            if (!IltUtil.checkExtendedExtendedBaseStruct(result.extendedExtendedBaseStructOut)) {
                LOG.info("callMethodWithMultipleStructParameters - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithMultipleStructParameters - FAILED");
            return false;
        }
        LOG.info("callMethodWithMultipleStructParameters - OK");
        return true;
    }

    public boolean callOverloadedMethod_1() {
        LOG.info("callOverloadedMethod_1");
        try {
            String result;
            result = testInterfaceProxy.overloadedMethod();
            if (result == null) {
                LOG.info("callOverloadedMethod_1 - got null as result");
                LOG.info("callOverloadedMethod_1 - FAILED");
                return false;
            }
            if (!result.equals("TestString 1")) {
                LOG.info("callOverloadedMethod_1 - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callOverloadedMethod_1 - FAILED");
            return false;
        }
        LOG.info("callOverloadedMethod_1 - OK");
        return true;
    }

    public boolean callOverloadedMethod_2() {
        LOG.info("callOverloadedMethod_2");
        try {
            String result;
            Boolean booleanArg = false;
            result = testInterfaceProxy.overloadedMethod(booleanArg);
            if (result == null) {
                LOG.info("callOverloadedMethod_2 - got null as result");
                LOG.info("callOverloadedMethod_2 - FAILED");
                return false;
            }
            if (!result.equals("TestString 2")) {
                LOG.info("callOverloadedMethod_2 invalid result - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callOverloadedMethod_2 exception - FAILED");
            return false;
        }
        LOG.info("callOverloadedMethod_2 - OK");
        return true;
    }

    public boolean callOverloadedMethod_3() {
        LOG.info("callOverloadedMethod_3");
        try {
            OverloadedMethodOverloadedMethod1Returned result;
            ExtendedExtendedEnumeration[] enumArrayArg = IltUtil.createExtendedExtendedEnumerationArray();
            Long int64Arg = 1L;
            BaseStruct baseStructArg = IltUtil.createBaseStruct();
            Boolean booleanArg = false;

            result = testInterfaceProxy.overloadedMethod(enumArrayArg, int64Arg, baseStructArg, booleanArg);

            if (result == null) {
                LOG.info("callOverloadedMethod_3 - got null as result");
                LOG.info("callOverloadedMethod_3 - FAILED");
                return false;
            }
            if (result.doubleOut != 0d || !IltUtil.checkStringArray(result.stringArrayOut)) {
                LOG.info("callOverloadedMethod_3 invalid result - FAILED");
                return false;
            }
            if (!IltUtil.checkExtendedBaseStruct(result.extendedBaseStructOut)) {
                LOG.info("callOverloadedMethod_3 invalid result - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callOverloadedMethod_3 exception - FAILED");
            return false;
        }
        LOG.info("callOverloadedMethod_3 - OK");
        return true;
    }

    public boolean callOverloadedMethodWithSelector_1() {
        LOG.info("callOverloadedMethodWithSelector_1");
        try {
            String result;
            result = testInterfaceProxy.overloadedMethodWithSelector();
            if (result == null) {
                LOG.info("callOverloadedMethodWithSelector_1 - got null as result");
                LOG.info("callOverloadedMethodWithSelector_1 - FAILED");
                return false;
            }
            if (!result.equals("Return value from overloadedMethodWithSelector 1")) {
                LOG.info("callOverloadedMethodWithSelector_1 invalid result - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callOverloadedMethodWithSelector_1 - FAILED");
            return false;
        }
        LOG.info("callOverloadedMethodWithSelector_1 - OK");
        return true;
    }

    public boolean callOverloadedMethodWithSelector_2() {
        LOG.info("callOverloadedMethodWithSelector_2");
        try {
            String result;
            Boolean booleanArg = false;
            result = testInterfaceProxy.overloadedMethodWithSelector(booleanArg);
            if (result == null) {
                LOG.info("callOverloadedMethodWithSelector_2 - got null as result");
                LOG.info("callOverloadedMethodWithSelector_2 - FAILED");
                return false;
            }
            if (!result.equals("Return value from overloadedMethodWithSelector 2")) {
                LOG.info("callOverloadedMethodWithSelector_2 invalid result - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callOverloadedMethodWithSelector_2 - FAILED");
            return false;
        }
        LOG.info("callOverloadedMethodWithSelector_2 - OK");
        return true;
    }

    public boolean callOverloadedMethodWithSelector_3() {
        LOG.info("callOverloadedMethodWithSelector_3");
        try {
            OverloadedMethodWithSelectorOverloadedMethodWithSelector1Returned result;
            ExtendedExtendedEnumeration[] enumArrayArg = IltUtil.createExtendedExtendedEnumerationArray();
            Long int64arg = 1L;
            BaseStruct baseStructArg = IltUtil.createBaseStruct();
            Boolean booleanArg = false;
            result = testInterfaceProxy.overloadedMethodWithSelector(enumArrayArg, int64arg, baseStructArg, booleanArg);
            if (result == null) {
                LOG.info("callOverloadedMethodWithSelector_3 - got null as result");
                LOG.info("callOverloadedMethodWithSelector_3 - FAILED");
                return false;
            }
            if (result.doubleOut != 1.1d || !IltUtil.checkExtendedBaseStruct(result.extendedBaseStructOut)
                    || !IltUtil.checkStringArray(result.stringArrayOut)) {
                LOG.info("callOverloadedMethodWithSelector_3 invalid result - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callOverloadedMethodWithSelector_3 - FAILED");
            return false;
        }
        LOG.info("callOverloadedMethodWithSelector_3 - OK");
        return true;
    }

    public boolean callMethodWithStringsAndSpecifiedStringOutLength() {
        LOG.info("callMethodWithStringsAndSpecifiedStringOutLength");
        try {
            String stringArg = "Hello world";
            Integer int32StringLengthArg = 32;
            String result;
            result = testInterfaceProxy.methodWithStringsAndSpecifiedStringOutLength(stringArg, int32StringLengthArg);
            if (result == null) {
                LOG.info("callMethodWithStringsAndSpecifiedStringOutLength - got null as result");
                LOG.info("callMethodWithStringsAndSpecifiedStringOutLength - FAILED");
                return false;
            }
            if (result.length() != int32StringLengthArg) {
                LOG.info("callMethodWithStringsAndSpecifiedStringOutLength invalid result - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithStringsAndSpecifiedStringOutLength - FAILED");
            return false;
        }
        LOG.info("callMethodWithStringsAndSpecifiedStringOutLength - OK");
        return true;
    }

    /*
     * SYNCHRONOUS METHOD CALLS WITH EXCEPTION HANDLING
     */

    public boolean callMethodWithoutErrorEnum() {
        LOG.info("callMethodWithoutErrorEnum");
        try {
            String wantedExceptionArg = "ProviderRuntimeException";
            testInterfaceProxy.methodWithoutErrorEnum(wantedExceptionArg);
            LOG.info("callMethodWithoutErrorEnum - Unexpected return without exception");
            LOG.info("callMethodWithoutErrorEnum - FAILED");
            return false;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null || !e.getMessage().endsWith("Exception from methodWithoutErrorEnum")) {
                LOG.info("callMethodWithoutErrorEnum - invalid exception message");
                LOG.info("callMethodWithoutErrorEnum - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithoutErrorEnum - unexpected exception type");
            LOG.info("callMethodWithoutErrorEnum - FAILED");
            return false;
        }
        LOG.info("callMethodWithoutErrorEnum - OK");
        return true;
    }

    public boolean callMethodWithAnonymousErrorEnum() {
        LOG.info("callMethodWithAnonymousErrorEnum");
        try {
            String wantedExceptionArg = "ProviderRuntimeException";
            testInterfaceProxy.methodWithAnonymousErrorEnum(wantedExceptionArg);
            LOG.info("callMethodWithAnonymousErrorEnum - Unexpected return without exception");
            LOG.info("callMethodWithAnonymousErrorEnum - FAILED");
            return false;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null || !e.getMessage().endsWith("Exception from methodWithAnonymousErrorEnum")) {
                LOG.info("callMethodWithAnonymousErrorEnum - invalid exception message");
                LOG.info("callMethodWithAnonymousErrorEnum - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithAnonymousErrorEnum - unexpected exception type");
            LOG.info("callMethodWithAnonymousErrorEnum - FAILED");
            return false;
        }

        // 2nd test
        try {
            String wantedExceptionArg = "ApplicationException";
            testInterfaceProxy.methodWithAnonymousErrorEnum(wantedExceptionArg);
            LOG.info("callMethodWithAnonymousErrorEnum - Unexpected return without exception");
            LOG.info("callMethodWithAnonymousErrorEnum - FAILED");
            return false;
        } catch (ApplicationException e) {
            if (e.getError() != MethodWithAnonymousErrorEnumErrorEnum.ERROR_3_1_NTC) {
                LOG.info("callMethodWithAnonymousErrorEnum - unexpected exception error enum value");
                LOG.info("callMethodWithAnonymousErrorEnum - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithAnonymousErrorEnum - unexpected exception type");
            LOG.info("callMethodWithAnonymousErrorEnum - FAILED");
            return false;
        }
        LOG.info("callMethodWithAnonymousErrorEnum - OK");
        return true;
    }

    public boolean callMethodWithExistingErrorEnum() {
        LOG.info("callMethodWithExistingErrorEnum");

        // 1st test
        try {
            String wantedExceptionArg = "ProviderRuntimeException";
            testInterfaceProxy.methodWithExistingErrorEnum(wantedExceptionArg);
            LOG.info("callMethodWithExistingErrorEnum - 1st -Unexpected return without exception");
            LOG.info("callMethodWithExistingErrorEnum - FAILED");
            return false;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null || !e.getMessage().endsWith("Exception from methodWithExistingErrorEnum")) {
                LOG.info("callMethodWithExistingErrorEnum - 1st -invalid exception message");
                LOG.info("callMethodWithExistingErrorEnum - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithExistingErrorEnum - 1st -unexpected exception type");
            LOG.info("callMethodWithExistingErrorEnum - FAILED");
            return false;
        }

        // 2nd test
        try {
            String wantedExceptionArg = "ApplicationException_1";
            testInterfaceProxy.methodWithExistingErrorEnum(wantedExceptionArg);
            LOG.info("callMethodWithExistingErrorEnum - 2nd -Unexpected return without exception");
            LOG.info("callMethodWithExistingErrorEnum - FAILED");
            return false;
        } catch (ApplicationException e) {
            if (e.getError() != ExtendedErrorEnumTc.ERROR_2_3_TC2) {
                LOG.info("callMethodWithExistingErrorEnum - 2nd -unexpected exception error enum value");
                LOG.info("callMethodWithExistingErrorEnum - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithExistingErrorEnum - 2nd -unexpected exception type");
            LOG.info("callMethodWithExistingErrorEnum - FAILED");
            return false;
        }

        // 3rd test
        try {
            String wantedExceptionArg = "ApplicationException_2";
            testInterfaceProxy.methodWithExistingErrorEnum(wantedExceptionArg);
            LOG.info("callMethodWithExistingErrorEnum - 3rd - Unexpected return without exception");
            LOG.info("callMethodWithExistingErrorEnum - FAILED");
            return false;
        } catch (ApplicationException e) {
            if (e.getError() != ExtendedErrorEnumTc.ERROR_1_2_TC_2) {
                LOG.info("callMethodWithExistingErrorEnum - 3rd - unexpected exception error enum value");
                LOG.info("callMethodWithExistingErrorEnum - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithExistingErrorEnum - 3rd - unexpected exception type");
            LOG.info("callMethodWithExistingErrorEnum - FAILED");
            return false;
        }
        LOG.info("callMethodWithExistingErrorEnum - OK");
        return true;
    }

    public boolean callMethodWithExtendedErrorEnum() {
        LOG.info("callMethodWithExtendedErrorEnum");

        // 1st test
        try {
            String wantedExceptionArg = "ProviderRuntimeException";
            testInterfaceProxy.methodWithExtendedErrorEnum(wantedExceptionArg);
            LOG.info("callMethodWithExtendedErrorEnum - 1st - Unexpected return without exception");
            LOG.info("callMethodWithExtendedErrorEnum - FAILED");
            return false;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null || !e.getMessage().endsWith("Exception from methodWithExtendedErrorEnum")) {
                LOG.info("callMethodWithExtendedErrorEnum - 1st - invalid exception message");
                LOG.info("callMethodWithExtendedErrorEnum - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithExtendedErrorEnum - 1st - unexpected exception type");
            LOG.info("callMethodWithExtendedErrorEnum - FAILED");
            return false;
        }

        // 2nd test
        try {
            String wantedExceptionArg = "ApplicationException_1";
            testInterfaceProxy.methodWithExtendedErrorEnum(wantedExceptionArg);
            LOG.info("callMethodWithExtendedErrorEnum - 2nd - Unexpected return without exception");
            LOG.info("callMethodWithExtendedErrorEnum - FAILED");
            return false;
        } catch (ApplicationException e) {
            if (e.getError() != MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC) {
                LOG.info("callMethodWithExtendedErrorEnum - 2nd - unexpected exception error enum value");
                LOG.info("callMethodWithExtendedErrorEnum - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithExtendedErrorEnum - 2nd - unexpected exception type");
            LOG.info("callMethodWithExtendedErrorEnum - FAILED");
            return false;
        }

        // 3rd test
        try {
            String wantedExceptionArg = "ApplicationException_2";
            testInterfaceProxy.methodWithExtendedErrorEnum(wantedExceptionArg);
            LOG.info("callMethodWithExtendedErrorEnum - 3rd - Unexpected return without exception");
            LOG.info("callMethodWithExtendedErrorEnum - FAILED");
            return false;
        } catch (ApplicationException e) {
            if (e.getError() != MethodWithExtendedErrorEnumErrorEnum.ERROR_2_1_TC2) {
                LOG.info("callMethodWithExtendedErrorEnum - 3rd - unexpected exception error enum value");
                LOG.info("callMethodWithExtendedErrorEnum - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithExtendedErrorEnum - 3rd - unexpected exception type");
            LOG.info("callMethodWithExtendedErrorEnum - FAILED");
            return false;
        }
        LOG.info("callMethodWithExtendedErrorEnum - OK");
        return true;
    }

    /*
     * GETTER AND SETTER CALLS
     */

    public boolean callGetAttributeUInt8() {
        LOG.info("callGetAttributeUInt8");
        try {
            Byte result;
            result = testInterfaceProxy.getAttributeUInt8();
            if (result == null) {
                LOG.info("callGetAttributeUInt8 - got null as result");
                LOG.info("callGetAttributeUInt8 - FAILED");
                return false;
            }
            if (result != 127) {
                LOG.info("callGetAttributeUInt8 - Unexpected content");
                LOG.info("callGetAttributeUInt8 - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeUInt8 - unexpected exception");
            LOG.info("callGetAttributeUInt8 - FAILED");
            return false;
        }

        LOG.info("callGetAttributeUInt8 - OK");
        return true;
    }

    public boolean callSetAttributeUInt8() {
        LOG.info("callSetAttributeUInt8");
        try {
            byte byteArg = 127;
            testInterfaceProxy.setAttributeUInt8(byteArg);
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callSetAttributeUInt8 - unexpected exception");
            LOG.info("callSetAttributeUInt8 - FAILED");
            return false;
        }

        LOG.info("callSetAttributeUInt8 - OK");
        return true;
    }

    public boolean callGetAttributeDouble() {
        LOG.info("callGetAttributeDouble");
        try {
            Double result;
            result = testInterfaceProxy.getAttributeDouble();
            if (result == null) {
                LOG.info("callGetAttributeDouble - got null as result");
                LOG.info("callGetAttributeDouble - FAILED");
                return false;
            }
            if (!IltUtil.cmpDouble(result, 1.1d)) {
                LOG.info("callGetAttributeDouble - Unexpected content");
                LOG.info("callGetAttributeDouble - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeDouble - unexpected exception");
            LOG.info("callGetAttributeDouble - FAILED");
            return false;
        }

        LOG.info("callGetAttributeDouble - OK");
        return true;
    }

    public boolean callSetAttributeDouble() {
        LOG.info("callSetAttributeDouble");
        try {
            double doubleArg = 1.1d;
            testInterfaceProxy.setAttributeDouble(doubleArg);
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callSetAttributeDouble - unexpected exception");
            LOG.info("callSetAttributeDouble - FAILED");
            return false;
        }

        LOG.info("callSetAttributeDouble - OK");
        return true;
    }

    public boolean callGetAttributeBooleanReadOnly() {
        LOG.info("callGetAttributeBooleanReadOnly");
        try {
            Boolean result;
            result = testInterfaceProxy.getAttributeBooleanReadonly();
            if (result == null) {
                LOG.info("callGetAttributeBooleanReadOnly - got null as result");
                LOG.info("callGetAttributeBooleanReadOnly - FAILED");
                return false;
            }
            if (result != true) {
                LOG.info("callGetAttributeBooleanReadOnly - Unexpected content");
                LOG.info("callGetAttributeBooleanReadOnly - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeBooleanReadOnly - unexpected exception");
            LOG.info("callGetAttributeBooleanReadOnly - FAILED");
            return false;
        }

        LOG.info("callGetAttributeBooleanReadOnly - OK");
        return true;
    }

    // there is no setter for attributeBooleanReadOnly, but this cannot
    // be checked since it would create a compiler error

    public boolean callGetAttributeStringNoSubscriptions() {
        LOG.info("callGetAttributeStringNoSubscriptions");
        try {
            String result;
            result = testInterfaceProxy.getAttributeStringNoSubscriptions();
            if (result == null) {
                LOG.info("callGetAttributeStringNoSubscriptions - got null as result");
                LOG.info("callGetAttributeStringNoSubscriptions - FAILED");
                return false;
            }
            if (!result.equals("Hello world")) {
                LOG.info("callGetAttributeStringNoSubscriptions - Unexpected content");
                LOG.info("callGetAttributeStringNoSubscriptions - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeStringNoSubscriptions - unexpected exception");
            LOG.info("callGetAttributeStringNoSubscriptions - FAILED");
            return false;
        }

        LOG.info("callGetAttributeStringNoSubscriptions - OK");
        return true;
    }

    public boolean callSetAttributeStringNoSubscriptions() {
        LOG.info("callSetAttributeStringNoSubscriptions");
        try {
            String stringArg = "Hello world";
            testInterfaceProxy.setAttributeStringNoSubscriptions(stringArg);
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callSetAttributeStringNoSubscriptions - unexpected exception");
            LOG.info("callSetAttributeStringNoSubscriptions - FAILED");
            return false;
        }

        LOG.info("callSetAttributeStringNoSubscriptions - OK");
        return true;
    }

    public boolean callGetAttributeInt8readonlyNoSubscriptions() {
        LOG.info("callGetAttributeInt8readonlyNoSubscriptions");
        try {
            Byte result;
            result = testInterfaceProxy.getAttributeInt8readonlyNoSubscriptions();
            if (result == null) {
                LOG.info("callGetAttributeInt8readonlyNoSubscriptions - got null as result");
                LOG.info("callGetAttributeInt8readonlyNoSubscriptions - FAILED");
                return false;
            }
            if (result != -128) {
                LOG.info("callGetAttributeInt8readonlyNoSubscriptions - Unexpected content");
                LOG.info("callGetAttributeInt8readonlyNoSubscriptions - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeInt8readonlyNoSubscriptions - unexpected exception");
            LOG.info("callGetAttributeInt8readonlyNoSubscriptions - FAILED");
            return false;
        }

        LOG.info("callGetAttributeInt8readonlyNoSubscriptions - OK");
        return true;
    }

    // no setter for attributeInt8readonlyNoSubscriptions

    public boolean callGetAttributeArrayOfStringImplicit() {
        LOG.info("callGetAttributeArrayOfStringImplicit");
        try {
            String[] result;

            result = testInterfaceProxy.getAttributeArrayOfStringImplicit();
            if (result == null) {
                LOG.info("callGetAttributeArrayOfStringImplicit - got null as result");
                LOG.info("callGetAttributeArrayOfStringImplicit - FAILED");
                return false;
            }
            if (!IltUtil.checkStringArray(result)) {
                LOG.info("callGetAttributeArrayOfStringImplicit - Unexpected content");
                LOG.info("callGetAttributeArrayOfStringImplicit - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeArrayOfStringImplicit - unexpected exception");
            LOG.info("callGetAttributeArrayOfStringImplicit - FAILED");
            return false;
        }

        LOG.info("callGetAttributeArrayOfStringImplicit - OK");
        return true;
    }

    public boolean callSetAttributeArrayOfStringImplicit() {
        LOG.info("callSetAttributeArrayOfStringImplicit");
        try {
            String[] stringArrayArg = IltUtil.createStringArray();
            testInterfaceProxy.setAttributeArrayOfStringImplicit(stringArrayArg);
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callSetAttributeArrayOfStringImplicit - unexpected exception");
            LOG.info("callSetAttributeArrayOfStringImplicit - FAILED");
            return false;
        }

        LOG.info("callSetAttributeArrayOfStringImplicit - OK");
        return true;
    }

    public boolean callGetAttributeEnumeration() {
        LOG.info("callGetAttributeEnumeration");
        try {
            Enumeration result;
            result = testInterfaceProxy.getAttributeEnumeration();
            if (result == null) {
                LOG.info("callGetAttributeEnumeration - got null as result");
                LOG.info("callGetAttributeEnumeration - FAILED");
                return false;
            }
            if (result != Enumeration.ENUM_0_VALUE_2) {
                LOG.info("callGetAttributeEnumeration - Unexpected content");
                LOG.info("callGetAttributeEnumeration - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeEnumeration - unexpected exception");
            LOG.info("callGetAttributeEnumeration - FAILED");
            return false;
        }

        LOG.info("callGetAttributeEnumeration - OK");
        return true;
    }

    public boolean callSetAttributeEnumeration() {
        LOG.info("callSetAttributeEnumeration");
        try {
            Enumeration enumerationArg = Enumeration.ENUM_0_VALUE_2;
            testInterfaceProxy.setAttributeEnumeration(enumerationArg);
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callSetAttributeEnumeration - unexpected exception");
            LOG.info("callSetAttributeEnumeration - FAILED");
            return false;
        }

        LOG.info("callSetAttributeEnumeration - OK");
        return true;
    }

    public boolean callGetAttributeExtendedEnumerationReadonly() {
        LOG.info("callGetAttributeExtendedEnumerationReadonly");
        try {
            ExtendedEnumerationWithPartlyDefinedValues result;
            result = testInterfaceProxy.getAttributeExtendedEnumerationReadonly();
            if (result == null) {
                LOG.info("callGetAttributeExtendedEnumerationReadonly - got null as result");
                LOG.info("callGetAttributeExtendedEnumerationReadonly - FAILED");
                return false;
            }
            if (result != ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES) {
                LOG.info("callGetAttributeExtendedEnumerationReadonly - Unexpected content");
                LOG.info("callGetAttributeExtendedEnumerationReadonly - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeExtendedEnumerationReadonly - unexpected exception");
            LOG.info("callGetAttributeExtendedEnumerationReadonly - FAILED");
            return false;
        }

        LOG.info("callGetAttributeExtendedEnumerationReadonly - OK");
        return true;
    }

    // no setter for attributeExtendedEnumerationReadonly

    public boolean callGetAttributeBaseStruct() {
        LOG.info("callGetAttributeBaseStruct");
        try {
            BaseStruct result;
            result = testInterfaceProxy.getAttributeBaseStruct();
            if (result == null) {
                LOG.info("callGetAttributeBaseStruct - got null as result");
                LOG.info("callGetAttributeBaseStruct - FAILED");
                return false;
            }
            if (!IltUtil.checkBaseStruct(result)) {
                LOG.info("callGetAttributeBaseStruct - Unexpected content");
                LOG.info("callGetAttributeBaseStruct - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeBaseStruct - unexpected exception");
            LOG.info("callGetAttributeBaseStruct - FAILED");
            return false;
        }

        LOG.info("callGetAttributeBaseStruct - OK");
        return true;
    }

    public boolean callSetAttributeBaseStruct() {
        LOG.info("callSetAttributeBaseStruct");
        try {
            BaseStruct baseStructArg = IltUtil.createBaseStruct();
            testInterfaceProxy.setAttributeBaseStruct(baseStructArg);
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callSetAttributeBaseStruct - unexpected exception");
            LOG.info("callSetAttributeBaseStruct - FAILED");
            return false;
        }

        LOG.info("callSetAttributeBaseStruct - OK");
        return true;
    }

    public boolean callGetAttributeExtendedExtendedBaseStruct() {
        LOG.info("callGetAttributeExtendedExtendedBaseStruct");
        try {
            ExtendedExtendedBaseStruct result;
            result = testInterfaceProxy.getAttributeExtendedExtendedBaseStruct();
            if (result == null) {
                LOG.info("callGetAttributeExtendedExtendedBaseStruct - got null as result");
                LOG.info("callGetAttributeExtendedExtendedBaseStruct - FAILED");
                return false;
            }
            if (!IltUtil.checkExtendedExtendedBaseStruct(result)) {
                LOG.info("callGetAttributeExtendedExtendedBaseStruct - Unexpected content");
                LOG.info("callGetAttributeExtendedExtendedBaseStruct - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeExtendedExtendedBaseStruct - unexpected exception");
            LOG.info("callGetAttributeExtendedExtendedBaseStruct - FAILED");
            return false;
        }

        LOG.info("callGetAttributeExtendedExtendedBaseStruct - OK");
        return true;
    }

    public boolean callSetAttributeExtendedExtendedBaseStruct() {
        LOG.info("callSetAttributeExtendedExtendedBaseStruct");
        try {
            ExtendedExtendedBaseStruct extendedExtendedBaseStructArg = IltUtil.createExtendedExtendedBaseStruct();
            testInterfaceProxy.setAttributeExtendedExtendedBaseStruct(extendedExtendedBaseStructArg);
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callSetAttributeExtendedExtendedBaseStruct - unexpected exception");
            LOG.info("callSetAttributeExtendedExtendedBaseStruct - FAILED");
            return false;
        }

        LOG.info("callSetAttributeExtendedExtendedBaseStruct - OK");
        return true;
    }

    public boolean callSetAttributeMapStringString() {
        LOG.info("callSetAttributeMapStringString");
        try {
            MapStringString attributeMapStringStringArg = new MapStringString();
            attributeMapStringStringArg.put("keyString1", "valueString1");
            attributeMapStringStringArg.put("keyString2", "valueString2");
            attributeMapStringStringArg.put("keyString3", "valueString3");
            testInterfaceProxy.setAttributeMapStringString(attributeMapStringStringArg);
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callSetAttributeMapStringString - unexpected exception");
            LOG.info("callSetAttributeMapStringString - FAILED");
            return false;
        }

        LOG.info("callSetAttributeMapStringString - OK");
        return true;
    }

    public boolean callGetAttributeMapStringString() {
        LOG.info("callGetAttributeMapStringString");
        try {
            MapStringString result = testInterfaceProxy.getAttributeMapStringString();
            if (result == null) {
                LOG.info("callGetAttributeMapStringString - got null as result");
                LOG.info("callGetAttributeMapStringString - FAILED");
                return false;
            }
            MapStringString expected = new MapStringString();
            expected.put("keyString1", "valueString1");
            expected.put("keyString2", "valueString2");
            expected.put("keyString3", "valueString3");
            if (!result.equals(expected)) {
                LOG.info("callGetAttributeMapStringString - Unexpected content");
                LOG.info("callGetAttributeMapStringString - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeMapStringString - unexpected exception");
            LOG.info("callGetAttributeMapStringString - FAILED");
            return false;
        }

        LOG.info("callGetAttributeMapStringString - OK");
        return true;
    }

    /*
     * GETTER AND SETTER CALLS WITH EXCEPTION
     */

    public boolean callGetAttributeWithException() {
        LOG.info("callGetAttributeWithException");
        try {
            testInterfaceProxy.getAttributeWithException();
            LOG.info("callGetAttributeWithException - Unexpected return without exception");
            LOG.info("callGetAttributeWithException - FAILED");
            return false;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null || !e.getMessage().endsWith("Exception from getAttributeWithException")) {
                LOG.info("callGetAttributeWithException - invalid exception message");
                LOG.info("callGetAttributeWithException - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callGetAttributeWithException - unexpected exception type");
            LOG.info("callGetAttributeWithException - FAILED");
            return false;
        }

        LOG.info("callGetAttributeWithException - OK");
        return true;
    }

    public boolean callSetAttributeWithException() {
        LOG.info("callSetAttributeWithException");
        try {
            testInterfaceProxy.setAttributeWithException(false);
            LOG.info("callSetAttributeWithException - Unexpected return without exception");
            LOG.info("callSetAttributeWithException - FAILED");
            return false;
        } catch (ProviderRuntimeException e) {
            if (e.getMessage() == null || !e.getMessage().endsWith("Exception from setAttributeWithException")) {
                LOG.info("callSetAttributeWithException - invalid exception message");
                LOG.info("callSetAttributeWithException - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callSetAttributeWithException - unexpected exception type");
            LOG.info("callSetAttributeWithException - FAILED");
            return false;
        }

        LOG.info("callSetAttributeWithException - OK");
        return true;
    }

    /*
     * ASYNC METHOD CALLS
     *
     * limit the number of example calls here
     * - one with single out parameters
     * - one with multiple out parameters
     * - one with multiple lists
     */

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean methodWithMultipleStructParametersAsyncCallbackDone = false;
    volatile boolean methodWithMultipleStructParametersAsyncCallbackResult = false;

    public boolean callMethodWithMultipleStructParametersAsync() {
        LOG.info("callMethodWithMultipleStructParametersAsync");
        try {
            // setup input parameters
            ExtendedStructOfPrimitives extendedStructOfPrimitivesArg = IltUtil.createExtendedStructOfPrimitives();
            BaseStruct baseStructArg = IltUtil.createBaseStruct();

            MethodWithMultipleStructParametersCallback callback = new MethodWithMultipleStructParametersCallback() {
                @Override
                public void onSuccess(BaseStructWithoutElements baseStructWithoutElementsOut,
                                      ExtendedExtendedBaseStruct extendedExtendedBaseStructOut) {
                    // check results
                    if (!IltUtil.checkBaseStructWithoutElements(baseStructWithoutElementsOut)) {
                        methodWithMultipleStructParametersAsyncCallbackResult = false;
                        methodWithMultipleStructParametersAsyncCallbackDone = true;
                        LOG.info("callMethodWithMultipleStructParametersAsync - callback - invalid baseStructWithoutElementsOut");
                        LOG.info("callMethodWithMultipleStructParametersAsync - FAILED");
                        return;
                    }

                    if (!IltUtil.checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut)) {
                        methodWithMultipleStructParametersAsyncCallbackResult = false;
                        methodWithMultipleStructParametersAsyncCallbackDone = true;
                        LOG.info("callMethodWithMultipleStructParametersAsync - callback - invalid extendedExtendedBaseStructOut");
                        LOG.info("callMethodWithMultipleStructParametersAsync - FAILED");
                        return;
                    }
                    methodWithMultipleStructParametersAsyncCallbackResult = true;
                    methodWithMultipleStructParametersAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    methodWithMultipleStructParametersAsyncCallbackResult = false;
                    methodWithMultipleStructParametersAsyncCallbackDone = true;
                    if (error instanceof JoynrRuntimeException) {
                        LOG.info("callMethodWithMultipleStructParametersAsync - callback - caught exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        LOG.info("callMethodWithMultipleStructParametersAsync - callback - caught exception");
                    }
                    LOG.info("callMethodWithMultipleStructParametersAsync - FAILED");
                }
            };

            MethodWithMultipleStructParametersFuture future = testInterfaceProxy.methodWithMultipleStructParameters(callback,
                                                                                                                    extendedStructOfPrimitivesArg,
                                                                                                                    baseStructArg);

            try {
                long timeoutInMilliseconds = 8000;
                LOG.info("callMethodWithMultipleStructParametersAsync - about to call future.get");
                MethodWithMultipleStructParametersReturned result = future.get(timeoutInMilliseconds);
                if (result == null) {
                    LOG.info("callMethodWithMultipleStructParametersAsync - got null as result");
                    LOG.info("callMethodWithMultipleStructParametersAsync - FAILED");
                    return false;
                }
                LOG.info("callMethodWithMultipleStructParametersAsync - returned from future.get");

                // check results from future
                if (!IltUtil.checkBaseStructWithoutElements(result.baseStructWithoutElementsOut)) {
                    LOG.info("callMethodWithMultipleStructParametersAsync - invalid baseStructWithoutElementsOut");
                    LOG.info("callMethodWithMultipleStructParametersAsync - FAILED");
                    return false;
                }

                if (!IltUtil.checkExtendedExtendedBaseStruct(result.extendedExtendedBaseStructOut)) {
                    LOG.info("callMethodWithMultipleStructParametersAsync - invalid extendedExtendedBaseStructOut");
                    LOG.info("callMethodWithMultipleStructParametersAsync - FAILED");
                    return false;
                }
                LOG.info("callMethodWithMultipleStructParametersAsync - future result checks OK");

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (methodWithMultipleStructParametersAsyncCallbackDone == false) {
                    LOG.info("callMethodWithMultipleStructParametersAsync - about to wait for a second for callback");
                    Thread.sleep(1000);
                    LOG.info("callMethodWithMultipleStructParametersAsync - wait for callback is over");
                } else {
                    LOG.info("callMethodWithMultipleStructParametersAsync - callback already done");
                }
                if (methodWithMultipleStructParametersAsyncCallbackDone == false) {
                    LOG.info("callMethodWithMultipleStructParametersAsync - callback NOT done");
                    LOG.info("callMethodWithMultipleStructParametersAsync - FAILED");
                    return false;
                }
                if (methodWithMultipleStructParametersAsyncCallbackResult == false) {
                    LOG.info("callMethodWithMultipleStructParametersAsync - callback reported error");
                    LOG.info("callMethodWithMultipleStructParametersAsync - FAILED");
                    return false;
                }
                LOG.info("callMethodWithMultipleStructParametersAsync - callback has set OK flags");
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException e) {
                LOG.info("callMethodWithMultipleStructParametersAsync - caught exception " + e.getMessage());
                LOG.info("callMethodWithMultipleStructParametersAsync - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithMultipleStructParametersAsync - FAILED");
            return false;
        }
        LOG.info("callMethodWithMultipleStructParametersAsync - OK");
        return true;
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean methodWithSingleArrayParametersAsyncCallbackDone = false;
    volatile boolean methodWithSingleArrayParametersAsyncCallbackResult = false;

    public boolean callMethodWithSingleArrayParametersAsync() {
        LOG.info("callMethodWithSingleArrayParametersAsync");
        try {
            // setup input parameters
            Double[] doubleArrayArg = IltUtil.createDoubleArray();

            Callback<String[]> callback = new Callback<String[]>() {
                @Override
                public void onSuccess(String[] stringOut) {
                    // check results
                    if (!IltUtil.checkStringArray(stringOut)) {
                        LOG.info("callMethodWithSingleArrayParametersAsync - invalid stringOut from callback");
                        LOG.info("callMethodWithSingleArrayParametersAsync - FAILED");
                        methodWithSingleArrayParametersAsyncCallbackResult = false;
                        methodWithSingleArrayParametersAsyncCallbackDone = true;
                        return;
                    }
                    methodWithSingleArrayParametersAsyncCallbackResult = true;
                    methodWithSingleArrayParametersAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    methodWithSingleArrayParametersAsyncCallbackResult = false;
                    methodWithSingleArrayParametersAsyncCallbackDone = true;
                    if (error instanceof JoynrRuntimeException) {
                        LOG.info("callMethodWithSingleArrayParametersAsync - callback - caught exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        LOG.info("callMethodWithSingleArrayParametersAsync - callback - caught exception");
                    }
                    LOG.info("callMethodWithSingleArrayParametersAsync - FAILED");
                }
            };

            Future<String[]> future = testInterfaceProxy.methodWithSingleArrayParameters(callback, doubleArrayArg);

            try {
                long timeoutInMilliseconds = 8000;
                LOG.info("callMethodWithSingleArrayParametersAsync - about to call future.get");
                String[] result;

                result = future.get(timeoutInMilliseconds);
                LOG.info("callMethodWithSingleArrayParametersAsync - returned from future.get");

                // check results from future
                if (result == null) {
                    LOG.info("callMethodWithSingleArrayParametersAsync - got null as result");
                    LOG.info("callMethodWithSingleArrayParametersAsync - FAILED");
                    return false;
                }
                if (!IltUtil.checkStringArray(result)) {
                    LOG.info("callMethodWithSingleArrayParametersAsync - invalid result from future");
                    LOG.info("callMethodWithSingleArrayParametersAsync - FAILED");
                }

                LOG.info("callMethodWithSingleArrayParametersAsync - future result checks OK");

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (methodWithSingleArrayParametersAsyncCallbackDone == false) {
                    LOG.info("callMethodWithSingleArrayParametersAsync - about to wait for a second for callback");
                    Thread.sleep(1000);
                    LOG.info("callMethodWithSingleArrayParametersAsync - wait for callback is over");
                } else {
                    LOG.info("callMethodWithSingleArrayParametersAsync - callback already done");
                }
                if (methodWithSingleArrayParametersAsyncCallbackDone == false) {
                    LOG.info("callMethodWithSingleArrayParametersAsync - callback NOT done");
                    LOG.info("callMethodWithSingleArrayParametersAsync - FAILED");
                    return false;
                }
                if (methodWithSingleArrayParametersAsyncCallbackResult == false) {
                    LOG.info("callMethodWithSingleArrayParametersAsync - callback reported error");
                    LOG.info("callMethodWithSingleArrayParametersAsync - FAILED");
                    return false;
                }
                LOG.info("callMethodWithSingleArrayParametersAsync - callback has set OK flags");
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException e) {
                LOG.info("callMethodWithSingleArrayParametersAsync - caught exception " + e.getMessage());
                LOG.info("callMethodWithSingleArrayParametersAsync - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithSingleArrayParametersAsync - FAILED");
            return false;
        }
        LOG.info("callMethodWithSingleArrayParametersAsync - OK");
        return true;
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean methodWithSinglePrimitiveParametersAsyncCallbackDone = false;
    volatile boolean methodWithSinglePrimitiveParametersAsyncCallbackResult = false;

    public boolean callMethodWithSinglePrimitiveParametersAsync() {
        LOG.info("callMethodWithSinglePrimitiveParametersAsync");
        try {
            // setup input parameters
            // final short arg = (short)65535;
            final short arg = (short) 32767;

            Callback<String> callback = new Callback<String>() {
                @Override
                public void onSuccess(String stringOut) {
                    // check results
                    if (!stringOut.equals(String.valueOf(Short.toUnsignedInt(arg)))) {
                        LOG.info("callMethodWithSinglePrimitiveParametersAsync - invalid stringOut from callback");
                        LOG.info("callMethodWithSinglePrimitiveParametersAsync - FAILED");
                        methodWithSinglePrimitiveParametersAsyncCallbackResult = false;
                        methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
                        return;
                    }
                    methodWithSinglePrimitiveParametersAsyncCallbackResult = true;
                    methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    methodWithSinglePrimitiveParametersAsyncCallbackResult = false;
                    methodWithSinglePrimitiveParametersAsyncCallbackDone = true;
                    if (error instanceof JoynrRuntimeException) {
                        LOG.info("callMethodWithSinglePrimitiveParametersAsync - callback - caught exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        LOG.info("callMethodWithSinglePrimitiveParametersAsync - callback - caught exception");
                    }
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - FAILED");
                }
            };

            Future<String> future = testInterfaceProxy.methodWithSinglePrimitiveParameters(callback, arg);

            try {
                long timeoutInMilliseconds = 8000;
                LOG.info("callMethodWithSinglePrimitiveParametersAsync - about to call future.get");
                String result = future.get(timeoutInMilliseconds);
                LOG.info("callMethodWithSinglePrimitiveParametersAsync - returned from future.get");

                // check results from future
                if (result == null) {
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - got null as result");
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - FAILED");
                    return false;
                }
                if (!result.equals(String.valueOf(Short.toUnsignedInt(arg)))) {
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - invalid result from future");
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - FAILED");
                    return false;
                }

                LOG.info("callMethodWithSinglePrimitiveParametersAsync - future result checks OK");

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (methodWithSinglePrimitiveParametersAsyncCallbackDone == false) {
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - about to wait for a second for callback");
                    Thread.sleep(1000);
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - wait for callback is over");
                } else {
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - callback already done");
                }
                if (methodWithSinglePrimitiveParametersAsyncCallbackDone == false) {
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - callback NOT done");
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - FAILED");
                    return false;
                }
                if (methodWithSinglePrimitiveParametersAsyncCallbackResult == false) {
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - callback reported error");
                    LOG.info("callMethodWithSinglePrimitiveParametersAsync - FAILED");
                    return false;
                }
                LOG.info("callMethodWithSinglePrimitiveParametersAsync - callback has set OK flags");
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException e) {
                LOG.info("callMethodWithSinglePrimitiveParametersAsync - caught exception " + e.getMessage());
                LOG.info("callMethodWithSinglePrimitiveParametersAsync - FAILED");
                return false;
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithSinglePrimitiveParametersAsync - FAILED");
            return false;
        }
        LOG.info("callMethodWithSinglePrimitiveParametersAsync - OK");
        return true;
    }

    /*
     * ASYNC METHOD CALLS WITH EXCEPTION
     */

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean methodWithExtendedErrorEnumAsyncCallbackDone = false;
    volatile boolean methodWithExtendedErrorEnumAsyncCallbackResult = false;

    @SuppressWarnings("checkstyle:methodlength")
    public boolean callMethodWithExtendedErrorEnumAsync() {
        LOG.info("callMethodWithExtendedErrorEnumAsync");
        try {
            // setup input parameters
            String wantedExceptionArg = "ProviderRuntimeException";

            CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum> callback = new CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum>() {
                @Override
                public void onSuccess(Void result) {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - unexpected positive return in callback");
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    if (error instanceof ProviderRuntimeException) {
                        if (((ProviderRuntimeException) error).getMessage()
                                                              .endsWith("Exception from methodWithExtendedErrorEnum")) {
                            LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - callback - got expected exception "
                                    + ((JoynrRuntimeException) error).getMessage());
                            methodWithExtendedErrorEnumAsyncCallbackResult = true;
                            methodWithExtendedErrorEnumAsyncCallbackDone = true;
                            return;
                        }
                        LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - callback - caught invalid exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    } else {
                        LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - callback - caught invalid exception "
                                + ((JoynrRuntimeException) error).getMessage());
                    }

                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                }

                @Override
                public void onFailure(MethodWithExtendedErrorEnumErrorEnum errorEnum) {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - callback - caught invalid exception ");
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                }
            };

            Future<Void> future = testInterfaceProxy.methodWithExtendedErrorEnum(callback, wantedExceptionArg);

            try {
                long timeoutInMilliseconds = 8000;
                LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - about to call future.get");
                future.get(timeoutInMilliseconds);
                LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - unexpected return from future");
                LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException error) {
                if (error instanceof ProviderRuntimeException) {
                    if (((ProviderRuntimeException) error).getMessage()
                                                          .endsWith("Exception from methodWithExtendedErrorEnum")) {
                        LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - caught expected exception "
                                + ((JoynrRuntimeException) error).getMessage());
                        // OK, fallthrough
                    } else {
                        // incorrect message
                        LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - caught invalid exception with message "
                                + ((JoynrRuntimeException) error).getMessage());
                        LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                        return false;
                    }
                } else if (error instanceof JoynrRuntimeException) {
                    // incorrect exception, can output message
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - caught invalid exception with message "
                            + ((JoynrRuntimeException) error).getMessage());
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                    return false;
                } else {
                    // incorrect exception, can not output message
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - caught invalid exception ");
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                    return false;
                }

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (methodWithExtendedErrorEnumAsyncCallbackDone == false) {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - wait a second for callback");
                    Thread.sleep(1000);
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - wait for callback is over");
                } else {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - callback already done");
                }
                if (methodWithExtendedErrorEnumAsyncCallbackDone == false) {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - callback NOT done");
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                    return false;
                }
                if (!methodWithExtendedErrorEnumAsyncCallbackResult) {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - callback reported failure");
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                    return false;
                }
                LOG.info("callMethodWithExtendedErrorEnumAsync - 1st - callback caught expected exception");
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
            return false;
        }

        LOG.info("callMethodWithExtendedErrorEnumAsync - ProviderRuntimeException check done");

        // 2nd test
        try {
            // setup input parameters
            String wantedExceptionArg = "ApplicationException_1";

            CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum> callback = new CallbackWithModeledError<Void, MethodWithExtendedErrorEnumErrorEnum>() {
                @Override
                public void onSuccess(Void result) {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - unexpected positive return in callback");
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(JoynrRuntimeException error) {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - callback - caught invalid JoynrRuntime like exception "
                            + ((JoynrRuntimeException) error).getMessage());
                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }

                @Override
                public void onFailure(MethodWithExtendedErrorEnumErrorEnum errorEnum) {
                    if (errorEnum == MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC) {
                        LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - callback - caught ApplicationException with expected error enum");
                        methodWithExtendedErrorEnumAsyncCallbackResult = true;
                        methodWithExtendedErrorEnumAsyncCallbackDone = true;
                        return;
                    }
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - callback - caught invalid ApplicationException with enum "
                            + errorEnum);

                    methodWithExtendedErrorEnumAsyncCallbackResult = false;
                    methodWithExtendedErrorEnumAsyncCallbackDone = true;
                }
            };

            Future<Void> future = testInterfaceProxy.methodWithExtendedErrorEnum(callback, wantedExceptionArg);

            try {
                long timeoutInMilliseconds = 8000;
                LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - about to call future.get");
                future.get(timeoutInMilliseconds);
                LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - unexpected return from future");
                LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                return false;
            } catch (InterruptedException | JoynrRuntimeException | ApplicationException error) {
                if (error instanceof ApplicationException) {
                    if (((ApplicationException) error).getError() == MethodWithExtendedErrorEnumErrorEnum.ERROR_3_3_NTC) {
                        LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - caught expected ApplicationException with correct error enum");
                    } else {
                        LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - caught invalid ApplicationException with enum "
                                + ((ApplicationException) error).getError());
                        LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                        return false;
                    }
                } else if (error instanceof JoynrRuntimeException) {
                    // incorrect exception, can output message
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - caught invalid JoynrRuntime like exception "
                            + ((JoynrRuntimeException) error).getMessage());
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                    return false;
                } else {
                    // incorrect exception, can not output message
                    LOG.info("callMethodWithExtendedErrorEnumAsync - callback - caught invalid other exception ");
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                    return false;
                }

                // check results from callback; expect to be finished within 1 second
                // should have been called ahead anyway
                if (methodWithExtendedErrorEnumAsyncCallbackDone == false) {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - about to wait for a second for callback");
                    Thread.sleep(1000);
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - wait for callback is over");
                } else {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - callback already done");
                }
                if (methodWithExtendedErrorEnumAsyncCallbackDone == false) {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - callback NOT done");
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                    return false;
                }
                if (!methodWithExtendedErrorEnumAsyncCallbackResult) {
                    LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - callback reported success");
                    LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
                    return false;
                }
                LOG.info("callMethodWithExtendedErrorEnumAsync - 2nd - callback has got expected exception");
            }
        } catch (Exception e) {
            System.out.println(e);
            e.printStackTrace();
            LOG.info("callMethodWithExtendedErrorEnumAsync - FAILED");
            return false;
        }
        LOG.info("callMethodWithExtendedErrorEnumAsync - ApplicationException check done");
        LOG.info("callMethodWithExtendedErrorEnumAsync - OK");
        return true;

        // 3rd test omitted
    }

    /*
     * ATTRIBUTE SUBSCRIPTIONS
     */

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeAttributeEnumerationCallbackDone = false;
    volatile boolean subscribeAttributeEnumerationCallbackResult = false;

    boolean callSubscribeAttributeEnumeration() {
        String subscriptionId;
        int minInterval_ms = 0;
        int maxInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);
        boolean result;

        LOG.info("callSubscribeAttributeEnumeration");

        try {
            subscriptionId = testInterfaceProxy.subscribeToAttributeEnumeration(new AttributeSubscriptionAdapter<Enumeration>() {
                                                                                    @Override
                                                                                    public void onReceive(Enumeration value) {
                                                                                        if (value == Enumeration.ENUM_0_VALUE_2) {
                                                                                            LOG.info("callSubscribeAttributeEnumeration - callback - got publication with correct value");
                                                                                            subscribeAttributeEnumerationCallbackResult = true;
                                                                                        } else {
                                                                                            subscribeAttributeEnumerationCallbackResult = false;
                                                                                            LOG.info("callSubscribeAttributeEnumeration - callback - got publication with invalid value");
                                                                                        }
                                                                                        subscribeAttributeEnumerationCallbackDone = true;
                                                                                    }

                                                                                    @Override
                                                                                    public void onError(JoynrRuntimeException error) {
                                                                                        LOG.info("callSubscribeAttributeEnumeration - callback - got unexpected exception");
                                                                                        subscribeAttributeEnumerationCallbackResult = false;
                                                                                        subscribeAttributeEnumerationCallbackDone = true;
                                                                                    }
                                                                                },
                                                                                subscriptionQos);
            LOG.info("callSubscribeAttributeEnumeration - subscription successful");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeAttributeEnumerationCallbackDone == false) {
                LOG.info("callSubscribeAttributeEnumeration - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeAttributeEnumeration - wait for callback is over");
            } else {
                LOG.info("callSubscribeAttributeEnumeration - callback already done");
            }
            if (subscribeAttributeEnumerationCallbackDone && subscribeAttributeEnumerationCallbackResult) {
                result = true;
            } else {
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromAttributeEnumeration(subscriptionId);
                LOG.info("callSubscribeAttributeEnumeration - unsubscribe successful");
            } catch (Exception e) {
                LOG.info("callSubscribeAttributeEnumeration - caught unexpected exception on unsubscribe");
                LOG.info("callSubscribeAttributeEnumeration - FAILED");
                result = false;
            }

            if (!result) {
                LOG.info("callSubscribeAttributeEnumeration - FAILED");
            } else {
                LOG.info("callSubscribeAttributeEnumeration - OK");
            }
            return result;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info("callSubscribeAttributeEnumeration - caught unexpected exception");
            LOG.info("callSubscribeAttributeEnumeration - FAILED");
            return false;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeAttributeWithExceptionCallbackDone = false;
    volatile boolean subscribeAttributeWithExceptionCallbackResult = false;

    boolean callSubscribeAttributeWithException() {
        String subscriptionId;
        int minInterval_ms = 0;
        int maxInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);
        boolean result;

        LOG.info("callSubscribeAttributeWithException");

        try {
            subscriptionId = testInterfaceProxy.subscribeToAttributeWithException(new AttributeSubscriptionAdapter<Boolean>() {
                                                                                      @Override
                                                                                      public void onReceive(Boolean value) {
                                                                                          LOG.info("callSubscribeAttributeWithException - callback - got unexpected publication");
                                                                                          subscribeAttributeWithExceptionCallbackResult = false;
                                                                                          subscribeAttributeWithExceptionCallbackDone = true;
                                                                                      }

                                                                                      @Override
                                                                                      public void onError(JoynrRuntimeException error) {
                                                                                          if (error instanceof ProviderRuntimeException) {
                                                                                              if (((ProviderRuntimeException) error).getMessage()
                                                                                                                                    .equals("Exception from getAttributeWithException")) {
                                                                                                  LOG.info("callSubscribeAttributeWithException - callback - got expected exception "
                                                                                                          + ((JoynrRuntimeException) error).getMessage());
                                                                                                  subscribeAttributeWithExceptionCallbackResult = true;
                                                                                                  subscribeAttributeWithExceptionCallbackDone = true;
                                                                                                  return;
                                                                                              }
                                                                                              LOG.info("callSubscribeAttributeWithException - callback - caught invalid exception "
                                                                                                      + ((JoynrRuntimeException) error).getMessage());
                                                                                          } else if (error instanceof JoynrRuntimeException) {
                                                                                              LOG.info("callSubscribeAttributeWithException - callback - caught invalid exception "
                                                                                                      + ((JoynrRuntimeException) error).getMessage());
                                                                                          } else {
                                                                                              LOG.info("callSubscribeAttributeWithException - callback - caught invalid exception ");
                                                                                          }
                                                                                          subscribeAttributeWithExceptionCallbackResult = false;
                                                                                          subscribeAttributeWithExceptionCallbackDone = true;
                                                                                      }
                                                                                  },
                                                                                  subscriptionQos);
            LOG.info("callSubscribeAttributeWithException - subscription successful");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeAttributeWithExceptionCallbackDone == false) {
                LOG.info("callSubscribeAttributeWithException - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeAttributeWithException - wait for callback is over");
            } else {
                LOG.info("callSubscribeAttributeWithException - callback already done");
            }
            if (!subscribeAttributeWithExceptionCallbackDone) {
                LOG.info("callSubscribeAttributeWithException - callback did not get called in time");
                result = false;
            } else if (subscribeAttributeWithExceptionCallbackResult) {
                LOG.info("callSubscribeAttributeWithException - callback got called and received expected exception");
                result = true;
            } else {
                LOG.info("callSubscribeAttributeWithException - callback got called but received unexpected result");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromAttributeWithException(subscriptionId);
                LOG.info("callSubscribeAttributeWithException - unsubscribe successful");
            } catch (Exception e) {
                LOG.info("callSubscribeAttributeWithException - caught unexpected exception on unsubscribe");
                LOG.info("callSubscribeAttributeWithException - FAILED");
                result = false;
            }

            if (!result) {
                LOG.info("callSubscribeAttributeWithException - FAILED");
            } else {
                LOG.info("callSubscribeAttributeWithException - OK");
            }
            return result;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info("callSubscribeAttributeWithException - caught unexpected exception");
            LOG.info("callSubscribeAttributeWithException - FAILED");
            return false;
        }
    }

    /*
     * BROADCAST SUBSCRIPTIONS
     */

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;

    boolean callSubscribeBroadcastWithSinglePrimitiveParameter() {
        String subscriptionId;
        int minInterval_ms = 0;
        int maxInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);
        boolean result;

        LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithSinglePrimitiveParameterBroadcast(new BroadcastWithSinglePrimitiveParameterBroadcastAdapter() {
                                                                                                              @Override
                                                                                                              public void onReceive(String stringOut) {
                                                                                                                  LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - callback - got broadcast");
                                                                                                                  if (!stringOut.equals("boom")) {
                                                                                                                      LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - callback - invalid content");
                                                                                                                      subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;
                                                                                                                  } else {
                                                                                                                      LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - callback - content OK");
                                                                                                                      subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = true;
                                                                                                                  }
                                                                                                                  subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = true;
                                                                                                              }

                                                                                                              @Override
                                                                                                              public void onError() {
                                                                                                                  LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - callback - error");
                                                                                                                  subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;
                                                                                                                  subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = true;
                                                                                                              }
                                                                                                          },
                                                                                                          subscriptionQos);
            LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - subscription successful");
            LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - Waiting one second");
            Thread.sleep(1000);
            LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSinglePrimitiveParameter();
            LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSinglePrimitiveParameterCallbackDone == false) {
                LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - wait for callback is over");
            } else {
                LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - callback already done");
            }
            if (!subscribeBroadcastWithSinglePrimitiveParameterCallbackDone) {
                LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithSinglePrimitiveParameterCallbackResult) {
                LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - callback got called and received expected publication");
                result = true;
            } else {
                LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSinglePrimitiveParameterBroadcast(subscriptionId);
                LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - unsubscribe successful");
            } catch (Exception e) {
                LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - caught unexpected exception on unsubscribe");
                LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - FAILED");
                result = false;
            }

            if (!result) {
                LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - FAILED");
            } else {
                LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - OK");
            }
            return result;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - caught unexpected exception");
            LOG.info("callSubscribeBroadcastWithSinglePrimitiveParameter - FAILED");
            return false;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;

    boolean callSubscribeBroadcastWithMultiplePrimitiveParameters() {
        String subscriptionId;
        int minInterval_ms = 0;
        int maxInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);
        boolean result;

        LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithMultiplePrimitiveParametersBroadcast(new BroadcastWithMultiplePrimitiveParametersBroadcastAdapter() {
                                                                                                                 @Override
                                                                                                                 public void onReceive(Double doubleOut,
                                                                                                                                       String stringOut) {
                                                                                                                     LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - got broadcast");
                                                                                                                     if (!stringOut.equals("boom")
                                                                                                                             || !IltUtil.cmpDouble(doubleOut,
                                                                                                                                                   1.1d)) {
                                                                                                                         LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - invalid content");
                                                                                                                         subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
                                                                                                                     } else {
                                                                                                                         LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - content OK");
                                                                                                                         subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = true;
                                                                                                                     }
                                                                                                                     subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = true;
                                                                                                                 }

                                                                                                                 @Override
                                                                                                                 public void onError() {
                                                                                                                     LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - callback - error");
                                                                                                                     subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
                                                                                                                     subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = true;
                                                                                                                 }
                                                                                                             },
                                                                                                             subscriptionQos);
            LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - subscription successful");
            LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - Waiting one second");
            Thread.sleep(1000);
            LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultiplePrimitiveParameters();
            LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone == false) {
                LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - wait for callback is over");
            } else {
                LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - callback already done");
            }
            if (!subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone) {
                LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult) {
                LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - callback got called and received expected publication");
                result = true;
            } else {
                LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultiplePrimitiveParametersBroadcast(subscriptionId);
                LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - unsubscribe successful");
            } catch (Exception e) {
                LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - caught unexpected exception on unsubscribe");
                LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - FAILED");
                result = false;
            }

            if (!result) {
                LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - FAILED");
            } else {
                LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - OK");
            }
            return result;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - caught unexpected exception");
            LOG.info("callSubscribeBroadcastWithMultiplePrimitiveParameters - FAILED");
            return false;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSingleArrayParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSingleArrayParameterCallbackResult = false;

    boolean callSubscribeBroadcastWithSingleArrayParameter() {
        String subscriptionId;
        int minInterval_ms = 0;
        int maxInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);
        boolean result;

        LOG.info("callSubscribeBroadcastWithSingleArrayParameter");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithSingleArrayParameterBroadcast(new BroadcastWithSingleArrayParameterBroadcastAdapter() {
                                                                                                          @Override
                                                                                                          public void onReceive(String[] stringArrayOut) {
                                                                                                              //
                                                                                                              LOG.info("callSubscribeBroadcastWithSingleArrayParameter - callback - got broadcast");
                                                                                                              if (!IltUtil.checkStringArray(stringArrayOut)) {
                                                                                                                  LOG.info("callSubscribeBroadcastWithSingleArrayParameter - callback - invalid content");
                                                                                                                  subscribeBroadcastWithSingleArrayParameterCallbackResult = false;
                                                                                                              } else {
                                                                                                                  LOG.info("callSubscribeBroadcastWithSingleArrayParameter - callback - content OK");
                                                                                                                  subscribeBroadcastWithSingleArrayParameterCallbackResult = true;
                                                                                                              }
                                                                                                              subscribeBroadcastWithSingleArrayParameterCallbackDone = true;
                                                                                                          }

                                                                                                          @Override
                                                                                                          public void onError() {
                                                                                                              LOG.info("callSubscribeBroadcastWithSingleArrayParameter - callback - error");
                                                                                                              subscribeBroadcastWithSingleArrayParameterCallbackResult = false;
                                                                                                              subscribeBroadcastWithSingleArrayParameterCallbackDone = true;
                                                                                                          }
                                                                                                      },
                                                                                                      subscriptionQos);
            LOG.info("callSubscribeBroadcastWithSingleArrayParameter - subscription successful");
            LOG.info("callSubscribeBroadcastWithSingleArrayParameter - Waiting one second");
            Thread.sleep(1000);
            LOG.info("callSubscribeBroadcastWithSingleArrayParameter - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSingleArrayParameter();
            LOG.info("callSubscribeBroadcastWithSingleArrayParameter - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSingleArrayParameterCallbackDone == false) {
                LOG.info("callSubscribeBroadcastWithSingleArrayParameter - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeBroadcastWithSingleArrayParameter - wait for callback is over");
            } else {
                LOG.info("callSubscribeBroadcastWithSingleArrayParameter - callback already done");
            }
            if (!subscribeBroadcastWithSingleArrayParameterCallbackDone) {
                LOG.info("callSubscribeBroadcastWithSingleArrayParameter - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithSingleArrayParameterCallbackResult) {
                LOG.info("callSubscribeBroadcastWithSingleArrayParameter - callback got called and received expected publication");
                result = true;
            } else {
                LOG.info("callSubscribeBroadcastWithSingleArrayParameter - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSingleArrayParameterBroadcast(subscriptionId);
                LOG.info("callSubscribeBroadcastWithSingleArrayParameter - unsubscribe successful");
            } catch (Exception e) {
                LOG.info("callSubscribeBroadcastWithSingleArrayParameter - caught unexpected exception on unsubscribe");
                LOG.info("callSubscribeBroadcastWithSingleArrayParameter - FAILED");
                result = false;
            }

            if (!result) {
                LOG.info("callSubscribeBroadcastWithSingleArrayParameter - FAILED");
            } else {
                LOG.info("callSubscribeBroadcastWithSingleArrayParameter - OK");
            }
            return result;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info("callSubscribeBroadcastWithSingleArrayParameter - caught unexpected exception");
            LOG.info("callSubscribeBroadcastWithSingleArrayParameter - FAILED");
            return false;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithMultipleArrayParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;

    boolean callSubscribeBroadcastWithMultipleArrayParameters() {
        String subscriptionId;
        int minInterval_ms = 0;
        int maxInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);
        boolean result;

        LOG.info("callSubscribeBroadcastWithMultipleArrayParameters");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithMultipleArrayParametersBroadcast(new BroadcastWithMultipleArrayParametersBroadcastAdapter() {
                                                                                                             @Override
                                                                                                             public void onReceive(Long[] uInt64ArrayOut,
                                                                                                                                   StructWithStringArray[] structWithStringArrayArrayOut) {
                                                                                                                 LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - callback - got broadcast");
                                                                                                                 if (!IltUtil.checkUInt64Array(uInt64ArrayOut)
                                                                                                                         || !IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
                                                                                                                     LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - callback - invalid content");
                                                                                                                     subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
                                                                                                                 } else {
                                                                                                                     LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - callback - content OK");
                                                                                                                     subscribeBroadcastWithMultipleArrayParametersCallbackResult = true;
                                                                                                                 }
                                                                                                                 subscribeBroadcastWithMultipleArrayParametersCallbackDone = true;
                                                                                                             }

                                                                                                             @Override
                                                                                                             public void onError() {
                                                                                                                 LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - callback - error");
                                                                                                                 subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
                                                                                                                 subscribeBroadcastWithMultipleArrayParametersCallbackDone = true;
                                                                                                             }
                                                                                                         },
                                                                                                         subscriptionQos);
            LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - subscription successful");
            LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - Waiting one second");
            Thread.sleep(1000);
            LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultipleArrayParameters();
            LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultipleArrayParametersCallbackDone == false) {
                LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - wait for callback is over");
            } else {
                LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - callback already done");
            }
            if (!subscribeBroadcastWithMultipleArrayParametersCallbackDone) {
                LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithMultipleArrayParametersCallbackResult) {
                LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - callback got called and received expected publication");
                result = true;
            } else {
                LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultipleArrayParametersBroadcast(subscriptionId);
                LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - unsubscribe successful");
            } catch (Exception e) {
                LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - caught unexpected exception on unsubscribe");
                LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - FAILED");
                result = false;
            }

            if (!result) {
                LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - FAILED");
            } else {
                LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - OK");
            }
            return result;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - caught unexpected exception");
            LOG.info("callSubscribeBroadcastWithMultipleArrayParameters - FAILED");
            return false;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSingleEnumerationParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;

    boolean callSubscribeBroadcastWithSingleEnumerationParameter() {
        String subscriptionId;
        int minInterval_ms = 0;
        int maxInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);
        boolean result;

        LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithSingleEnumerationParameterBroadcast(new BroadcastWithSingleEnumerationParameterBroadcastAdapter() {
                                                                                                                @Override
                                                                                                                public void onReceive(ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut) {
                                                                                                                    LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - callback - got broadcast");
                                                                                                                    if (enumerationOut != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
                                                                                                                        LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - callback - invalid content");
                                                                                                                        subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
                                                                                                                    } else {
                                                                                                                        LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - callback - content OK");
                                                                                                                        subscribeBroadcastWithSingleEnumerationParameterCallbackResult = true;
                                                                                                                    }
                                                                                                                    subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
                                                                                                                }

                                                                                                                @Override
                                                                                                                public void onError() {
                                                                                                                    LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - callback - error");
                                                                                                                    subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
                                                                                                                    subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
                                                                                                                }
                                                                                                            },
                                                                                                            subscriptionQos);
            LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - subscription successful");
            LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - Waiting one second");
            Thread.sleep(1000);
            LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter();
            LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSingleEnumerationParameterCallbackDone == false) {
                LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - wait for callback is over");
            } else {
                LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - callback already done");
            }
            if (!subscribeBroadcastWithSingleEnumerationParameterCallbackDone) {
                LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithSingleEnumerationParameterCallbackResult) {
                LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - callback got called and received expected publication");
                result = true;
            } else {
                LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSingleEnumerationParameterBroadcast(subscriptionId);
                LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - unsubscribe successful");
            } catch (Exception e) {
                LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - caught unexpected exception on unsubscribe");
                LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - FAILED");
                result = false;
            }

            if (!result) {
                LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - FAILED");
            } else {
                LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - OK");
            }
            return result;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - caught unexpected exception");
            LOG.info("callSubscribeBroadcastWithSingleEnumerationParameter - FAILED");
            return false;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;

    boolean callSubscribeBroadcastWithMultipleEnumerationParameters() {
        String subscriptionId;
        int minInterval_ms = 0;
        int maxInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);
        boolean result;

        LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithMultipleEnumerationParametersBroadcast(new BroadcastWithMultipleEnumerationParametersBroadcastAdapter() {
                                                                                                                   @Override
                                                                                                                   public void onReceive(ExtendedEnumerationWithPartlyDefinedValues extendedEnumerationOut,
                                                                                                                                         Enumeration enumerationOut) {
                                                                                                                       LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - callback - got broadcast");
                                                                                                                       if (extendedEnumerationOut != ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES
                                                                                                                               || enumerationOut != Enumeration.ENUM_0_VALUE_1) {
                                                                                                                           LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - callback - invalid content");
                                                                                                                           subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
                                                                                                                       } else {
                                                                                                                           LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - callback - content OK");
                                                                                                                           subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = true;
                                                                                                                       }
                                                                                                                       subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = true;
                                                                                                                   }

                                                                                                                   @Override
                                                                                                                   public void onError() {
                                                                                                                       LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - callback - error");
                                                                                                                       subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
                                                                                                                       subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = true;
                                                                                                                   }
                                                                                                               },
                                                                                                               subscriptionQos);
            LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - subscription successful");
            LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - Waiting one second");
            Thread.sleep(1000);
            LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultipleEnumerationParameters();
            LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultipleEnumerationParametersCallbackDone == false) {
                LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - wait for callback is over");
            } else {
                LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - callback already done");
            }
            if (!subscribeBroadcastWithMultipleEnumerationParametersCallbackDone) {
                LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithMultipleEnumerationParametersCallbackResult) {
                LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - callback got called and received expected publication");
                result = true;
            } else {
                LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultipleEnumerationParametersBroadcast(subscriptionId);
                LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - unsubscribe successful");
            } catch (Exception e) {
                LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - caught unexpected exception on unsubscribe");
                LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - FAILED");
                result = false;
            }

            if (!result) {
                LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - FAILED");
            } else {
                LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - OK");
            }
            return result;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - caught unexpected exception");
            LOG.info("callSubscribeBroadcastWithMultipleEnumerationParameters - FAILED");
            return false;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSingleStructParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSingleStructParameterCallbackResult = false;

    boolean callSubscribeBroadcastWithSingleStructParameter() {
        String subscriptionId;
        int minInterval_ms = 0;
        int maxInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);
        boolean result;

        LOG.info("callSubscribeBroadcastWithSingleStructParameter");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithSingleStructParameterBroadcast(new BroadcastWithSingleStructParameterBroadcastAdapter() {
                                                                                                           @Override
                                                                                                           public void onReceive(ExtendedStructOfPrimitives extendedStructOfPrimitivesOut) {
                                                                                                               LOG.info("callSubscribeBroadcastWithSingleStructParameter - callback - got broadcast");
                                                                                                               if (!IltUtil.checkExtendedStructOfPrimitives(extendedStructOfPrimitivesOut)) {
                                                                                                                   LOG.info("callSubscribeBroadcastWithSingleStructParameter - callback - invalid content");
                                                                                                                   subscribeBroadcastWithSingleStructParameterCallbackResult = false;
                                                                                                               } else {
                                                                                                                   LOG.info("callSubscribeBroadcastWithSingleStructParameter - callback - content OK");
                                                                                                                   subscribeBroadcastWithSingleStructParameterCallbackResult = true;
                                                                                                               }
                                                                                                               subscribeBroadcastWithSingleStructParameterCallbackDone = true;
                                                                                                           }

                                                                                                           @Override
                                                                                                           public void onError() {
                                                                                                               LOG.info("callSubscribeBroadcastWithSingleStructParameter - callback - error");
                                                                                                               subscribeBroadcastWithSingleStructParameterCallbackResult = false;
                                                                                                               subscribeBroadcastWithSingleStructParameterCallbackDone = true;
                                                                                                           }
                                                                                                       },
                                                                                                       subscriptionQos);
            LOG.info("callSubscribeBroadcastWithSingleStructParameter - subscription successful");
            LOG.info("callSubscribeBroadcastWithSingleStructParameter - Waiting one second");
            Thread.sleep(1000);
            LOG.info("callSubscribeBroadcastWithSingleStructParameter - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSingleStructParameter();
            LOG.info("callSubscribeBroadcastWithSingleStructParameter - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSingleStructParameterCallbackDone == false) {
                LOG.info("callSubscribeBroadcastWithSingleStructParameter - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeBroadcastWithSingleStructParameter - wait for callback is over");
            } else {
                LOG.info("callSubscribeBroadcastWithSingleStructParameter - callback already done");
            }
            if (!subscribeBroadcastWithSingleStructParameterCallbackDone) {
                LOG.info("callSubscribeBroadcastWithSingleStructParameter - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithSingleStructParameterCallbackResult) {
                LOG.info("callSubscribeBroadcastWithSingleStructParameter - callback got called and received expected publication");
                result = true;
            } else {
                LOG.info("callSubscribeBroadcastWithSingleStructParameter - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSingleStructParameterBroadcast(subscriptionId);
                LOG.info("callSubscribeBroadcastWithSingleStructParameter - unsubscribe successful");
            } catch (Exception e) {
                LOG.info("callSubscribeBroadcastWithSingleStructParameter - caught unexpected exception on unsubscribe");
                LOG.info("callSubscribeBroadcastWithSingleStructParameter - FAILED");
                result = false;
            }

            if (!result) {
                LOG.info("callSubscribeBroadcastWithSingleStructParameter - FAILED");
            } else {
                LOG.info("callSubscribeBroadcastWithSingleStructParameter - OK");
            }
            return result;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info("callSubscribeBroadcastWithSingleStructParameter - caught unexpected exception");
            LOG.info("callSubscribeBroadcastWithSingleStructParameter - FAILED");
            return false;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithMultipleStructParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultipleStructParametersCallbackResult = false;

    boolean callSubscribeBroadcastWithMultipleStructParameters() {
        String subscriptionId;
        int minInterval_ms = 0;
        int maxInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);
        boolean result;

        LOG.info("callSubscribeBroadcastWithMultipleStructParameters");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithMultipleStructParametersBroadcast(new BroadcastWithMultipleStructParametersBroadcastAdapter() {
                                                                                                              @Override
                                                                                                              public void onReceive(BaseStructWithoutElements baseStructWithoutElementsOut,
                                                                                                                                    ExtendedExtendedBaseStruct extendedExtendedBaseStructOut) {
                                                                                                                  LOG.info("callSubscribeBroadcastWithMultipleStructParameters - callback - got broadcast");
                                                                                                                  if (!IltUtil.checkBaseStructWithoutElements(baseStructWithoutElementsOut)
                                                                                                                          || !IltUtil.checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut)) {
                                                                                                                      LOG.info("callSubscribeBroadcastWithMultipleStructParameters - callback - invalid content");
                                                                                                                      subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
                                                                                                                  } else {
                                                                                                                      LOG.info("callSubscribeBroadcastWithMultipleStructParameters - callback - content OK");
                                                                                                                      subscribeBroadcastWithMultipleStructParametersCallbackResult = true;
                                                                                                                  }
                                                                                                                  subscribeBroadcastWithMultipleStructParametersCallbackDone = true;
                                                                                                              }

                                                                                                              @Override
                                                                                                              public void onError() {
                                                                                                                  LOG.info("callSubscribeBroadcastWithMultipleStructParameters - callback - error");
                                                                                                                  subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
                                                                                                                  subscribeBroadcastWithMultipleStructParametersCallbackDone = true;
                                                                                                              }
                                                                                                          },
                                                                                                          subscriptionQos);
            LOG.info("callSubscribeBroadcastWithMultipleStructParameters - subscription successful");
            LOG.info("callSubscribeBroadcastWithMultipleStructParameters - Waiting one second");
            Thread.sleep(1000);
            LOG.info("callSubscribeBroadcastWithMultipleStructParameters - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultipleStructParameters();
            LOG.info("callSubscribeBroadcastWithMultipleStructParameters - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultipleStructParametersCallbackDone == false) {
                LOG.info("callSubscribeBroadcastWithMultipleStructParameters - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeBroadcastWithMultipleStructParameters - wait for callback is over");
            } else {
                LOG.info("callSubscribeBroadcastWithMultipleStructParameters - callback already done");
            }
            if (!subscribeBroadcastWithMultipleStructParametersCallbackDone) {
                LOG.info("callSubscribeBroadcastWithMultipleStructParameters - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithMultipleStructParametersCallbackResult) {
                LOG.info("callSubscribeBroadcastWithMultipleStructParameters - callback got called and received expected publication");
                result = true;
            } else {
                LOG.info("callSubscribeBroadcastWithMultipleStructParameters - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultipleStructParametersBroadcast(subscriptionId);
                LOG.info("callSubscribeBroadcastWithMultipleStructParameters - unsubscribe successful");
            } catch (Exception e) {
                LOG.info("callSubscribeBroadcastWithMultipleStructParameters - caught unexpected exception on unsubscribe");
                LOG.info("callSubscribeBroadcastWithMultipleStructParameters - FAILED");
                result = false;
            }

            if (!result) {
                LOG.info("callSubscribeBroadcastWithMultipleStructParameters - FAILED");
            } else {
                LOG.info("callSubscribeBroadcastWithMultipleStructParameters - OK");
            }
            return result;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info("callSubscribeBroadcastWithMultipleStructParameters - caught unexpected exception");
            LOG.info("callSubscribeBroadcastWithMultipleStructParameters - FAILED");
            return false;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithFilteringCallbackDone = false;
    volatile boolean subscribeBroadcastWithFilteringCallbackResult = false;

    @SuppressWarnings("checkstyle:methodlength")
    boolean callSubscribeBroadcastWithFiltering() {
        String subscriptionId;
        int minInterval_ms = 0;
        int maxInterval_ms = 10000;
        long expiryDate_ms = System.currentTimeMillis() + 60000;
        int alertAfterInterval_ms = 20000;
        int publicationTtl_ms = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(minInterval_ms,
                                                                                                        maxInterval_ms,
                                                                                                        expiryDate_ms,
                                                                                                        alertAfterInterval_ms,
                                                                                                        publicationTtl_ms);
        boolean result;

        LOG.info("callSubscribeBroadcastWithFiltering");

        try {
            BroadcastWithFilteringBroadcastFilterParameters filterParameters = new BroadcastWithFilteringBroadcastFilterParameters();
            String stringOfInterst = "fireBroadcast";
            filterParameters.setStringOfInterest(stringOfInterst);

            String[] stringArrayOfInterest = IltUtil.createStringArray();
            String json;
            try {
                json = objectMapper.writeValueAsString(stringArrayOfInterest);
            } catch (JsonProcessingException je) {
                LOG.info("callSubscribeBroadcastWithFiltering - got exception when serializing stringArrayOfInterest");
                LOG.info("callSubscribeBroadcastWithFiltering - FAILED");
                return false;
            }
            filterParameters.setStringArrayOfInterest(json);

            ExtendedTypeCollectionEnumerationInTypeCollection enumerationOfInterest = ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION;
            try {
                json = objectMapper.writeValueAsString(enumerationOfInterest);
            } catch (JsonProcessingException je) {
                LOG.info("callSubscribeBroadcastWithFiltering - got exception when serializing enumerationOfInterest");
                LOG.info("callSubscribeBroadcastWithFiltering - FAILED");
                return false;
            }
            filterParameters.setEnumerationOfInterest(json);

            StructWithStringArray structWithStringArrayOfInterest = IltUtil.createStructWithStringArray();
            try {
                json = objectMapper.writeValueAsString(structWithStringArrayOfInterest);
            } catch (JsonProcessingException je) {
                LOG.info("callSubscribeBroadcastWithFiltering - got exception when serializing structWithStringArrayOfInterest");
                LOG.info("callSubscribeBroadcastWithFiltering - FAILED");
                return false;
            }
            filterParameters.setStructWithStringArrayOfInterest(json);

            StructWithStringArray[] structWithStringArrayArrayOfInterest = IltUtil.createStructWithStringArrayArray();
            try {
                json = objectMapper.writeValueAsString(structWithStringArrayArrayOfInterest);
            } catch (JsonProcessingException je) {
                LOG.info("callSubscribeBroadcastWithFiltering - got exception when serializing structWithStringArrayArrayOfInterest");
                LOG.info("callSubscribeBroadcastWithFiltering - FAILED");
                return false;
            }
            filterParameters.setStructWithStringArrayArrayOfInterest(json);

            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithFilteringBroadcast(new BroadcastWithFilteringBroadcastAdapter() {
                                                                                               @Override
                                                                                               public void onReceive(String stringOut,
                                                                                                                     String[] stringArrayOut,
                                                                                                                     ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut,
                                                                                                                     StructWithStringArray structWithStringArrayOut,
                                                                                                                     StructWithStringArray[] structWithStringArrayArrayOut) {

                                                                                                   LOG.info("callSubscribeBroadcastWithFiltering - callback - got broadcast");

                                                                                                   if (!IltUtil.checkStringArray(stringArrayOut)) {
                                                                                                       subscribeBroadcastWithFilteringCallbackResult = false;
                                                                                                   } else if (enumerationOut != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
                                                                                                       LOG.info("callSubscribeBroadcastWithFiltering - callback - invalid content");
                                                                                                       subscribeBroadcastWithFilteringCallbackResult = false;
                                                                                                   } else if (!IltUtil.checkStructWithStringArray(structWithStringArrayOut)) {
                                                                                                       LOG.info("callSubscribeBroadcastWithFiltering - callback - invalid content");
                                                                                                       subscribeBroadcastWithFilteringCallbackResult = false;
                                                                                                   } else if (!IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
                                                                                                       LOG.info("callSubscribeBroadcastWithFiltering - callback - invalid content");
                                                                                                       subscribeBroadcastWithFilteringCallbackResult = false;
                                                                                                   } else {
                                                                                                       LOG.info("callSubscribeBroadcastWithFiltering - callback - content OK");
                                                                                                       subscribeBroadcastWithFilteringCallbackResult = true;
                                                                                                   }
                                                                                                   subscribeBroadcastWithFilteringCallbackDone = true;
                                                                                               }

                                                                                               @Override
                                                                                               public void onError() {
                                                                                                   LOG.info("callSubscribeBroadcastWithFiltering - callback - error");
                                                                                                   subscribeBroadcastWithFilteringCallbackResult = false;
                                                                                                   subscribeBroadcastWithFilteringCallbackDone = true;
                                                                                               }
                                                                                           },
                                                                                           subscriptionQos,
                                                                                           filterParameters);
            LOG.info("callSubscribeBroadcastWithFiltering - subscription successful");
            LOG.info("callSubscribeBroadcastWithFiltering - Waiting one second");
            Thread.sleep(1000);
            LOG.info("callSubscribeBroadcastWithFiltering - Wait done, invoking fire method");
            String stringArg = "fireBroadcast";
            testInterfaceProxy.methodToFireBroadcastWithFiltering(stringArg);
            LOG.info("callSubscribeBroadcastWithFiltering - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithFilteringCallbackDone == false) {
                LOG.info("callSubscribeBroadcastWithFiltering - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeBroadcastWithFiltering - wait for callback is over");
            } else {
                LOG.info("callSubscribeBroadcastWithFiltering - callback already done");
            }
            if (!subscribeBroadcastWithFilteringCallbackDone) {
                LOG.info("callSubscribeBroadcastWithFiltering - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithFilteringCallbackResult) {
                LOG.info("callSubscribeBroadcastWithFiltering - callback got called and received expected publication");
                result = true;
            } else {
                LOG.info("callSubscribeBroadcastWithFiltering - callback got called but received unexpected error or publication content");
                result = false;
            }

            // get out, if first test run failed
            if (result == false) {
                LOG.info("callSubscribeBroadcastWithFiltering - FAILED");
                return false;
            }

            // reset counter for 2nd test
            subscribeBroadcastWithFilteringCallbackResult = false;
            subscribeBroadcastWithFilteringCallbackDone = false;

            LOG.info("callSubscribeBroadcastWithFiltering - invoking fire method with wrong stringArg");
            stringArg = "doNotfireBroadcast";
            testInterfaceProxy.methodToFireBroadcastWithFiltering(stringArg);
            LOG.info("callSubscribeBroadcastWithFiltering - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithFilteringCallbackDone == false) {
                LOG.info("callSubscribeBroadcastWithFiltering - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info("callSubscribeBroadcastWithFiltering - wait for callback is over");
            } else {
                LOG.info("callSubscribeBroadcastWithFiltering - callback already done");
            }
            if (!subscribeBroadcastWithFilteringCallbackDone) {
                LOG.info("callSubscribeBroadcastWithFiltering - callback did not get called in time (expected)");
                result = true;
            } else {
                LOG.info("callSubscribeBroadcastWithFiltering - callback got called unexpectedly");
                result = false;
            }

            // try to unsubscribe
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithFilteringBroadcast(subscriptionId);
                LOG.info("callSubscribeBroadcastWithFiltering - unsubscribe successful");
            } catch (Exception e) {
                LOG.info("callSubscribeBroadcastWithFiltering - caught unexpected exception on unsubscribe");
                LOG.info("callSubscribeBroadcastWithFiltering - FAILED");
                result = false;
            }

            if (!result) {
                LOG.info("callSubscribeBroadcastWithFiltering - FAILED");
            } else {
                LOG.info("callSubscribeBroadcastWithFiltering - OK");
            }
            return result;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info("callSubscribeBroadcastWithFiltering - caught unexpected exception");
            LOG.info("callSubscribeBroadcastWithFiltering - FAILED");
            return false;
        }
    }

    // Helper class to store tests and their results
    public class TestResult {
        private String name;
        private boolean result;

        TestResult(String name, boolean result) {
            this.name = name;
            this.result = result;
        }

        String getName() {
            return this.name;
        }

        boolean getResult() {
            return this.result;
        }
    }

    ArrayList<TestResult> tests = new ArrayList<TestResult>();

    public void reportTest(String name, boolean result) {
        tests.add(new TestResult(name, result));
    }

    public int evaluateAndPrintResults() {
        int exitCode;
        int cntFailed = 0;
        int cntOk = 0;
        int cols = 75;
        char[] buffer = new char[cols];
        Arrays.fill(buffer, '=');
        String horizontalRuler = new String(buffer);
        StringBuilder output = new StringBuilder("\n");
        output.append(horizontalRuler);
        output.append("\n");
        output.append("INTERLANGUAGE TEST SUMMARY (JAVA CONSUMER):\n");
        output.append(horizontalRuler);
        output.append("\n");
        for (TestResult x : tests) {
            String result = (x.getResult() == true) ? "OK" : "FAILED";
            int length = cols - x.getName().length() - result.length();
            length = (length > 0) ? length : 1;
            buffer = new char[length];
            Arrays.fill(buffer, '.');
            String filler = new String(buffer);
            output.append(String.format("%s%s%s\n", x.getName(), filler, result));
            if (x.getResult()) {
                cntOk++;
            } else {
                cntFailed++;
            }
        }
        output.append(horizontalRuler);
        output.append("\n");
        output.append("Tests executed: " + (cntOk + cntFailed) + ", Success: " + cntOk + ", Failures: " + cntFailed
                + "\n");
        output.append(horizontalRuler);
        output.append("\n");
        if (cntFailed > 0) {
            output.append("Final result: FAILED\n");
            exitCode = 1;
        } else {
            output.append("Final result: SUCCESS\n");
            exitCode = 0;
        }
        output.append(horizontalRuler + "\n");
        LOG.info(output.toString());
        return exitCode;
    }

    @SuppressWarnings("checkstyle:methodlength")
    @Override
    public void run() {
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeout(10000);
        discoveryQos.setCacheMaxAge(Long.MAX_VALUE);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        ProxyBuilder<TestInterfaceProxy> proxyBuilder = runtime.getProxyBuilder(providerDomain,
                                                                                TestInterfaceProxy.class);

        try {
            testInterfaceProxy = proxyBuilder.setMessagingQos(new MessagingQos(10000))
                                             .setDiscoveryQos(discoveryQos)
                                             .build();
            reportTest("proxy built", true);

            // Synchronous method calls

            // tests without input and/or output parameters
            reportTest("callMethodWithoutParameters", callMethodWithoutParameters());
            reportTest("callMethodWithoutInputParameter", callMethodWithoutInputParameter());
            reportTest("callMethodWithoutOutputParameter", callMethodWithoutOutputParameter());

            // tests with single/multiple parameters
            reportTest("callMethodWithSinglePrimitiveParameters", callMethodWithSinglePrimitiveParameters());
            reportTest("callMethodWithMultiplePrimitiveParameters", callMethodWithMultiplePrimitiveParameters());

            // tests with array parameters
            reportTest("callMethodWithSingleArrayParameters", callMethodWithSingleArrayParameters());
            reportTest("callMethodWithMultipleArrayParameters", callMethodWithMultipleArrayParameters());

            // tests with enum parameters
            reportTest("callMethodWithSingleEnumParameters", callMethodWithSingleEnumParameters());
            reportTest("callMethodWithMultipleEnumParameters", callMethodWithMultipleEnumParameters());

            // tests with struct parameters
            reportTest("callMethodWithSingleStructParameters", callMethodWithSingleStructParameters());
            reportTest("callMethodWithMultipleStructParameters", callMethodWithMultipleStructParameters());

            // tests with overloaded methods
            reportTest("callOverloadedMethod_1", callOverloadedMethod_1());
            reportTest("callOverloadedMethod_2", callOverloadedMethod_2());
            reportTest("callOverloadedMethod_3", callOverloadedMethod_3());

            // tests with overloaded method with selectors
            reportTest("callOverloadedMethodWithSelector_1", callOverloadedMethodWithSelector_1());
            reportTest("callOverloadedMethodWithSelector_2", callOverloadedMethodWithSelector_2());
            reportTest("callOverloadedMethodWithSelector_3", callOverloadedMethodWithSelector_3());

            // test with special method returning string of dynamic length
            // depending on input parameters which can be used for
            // performance testing
            reportTest("callMethodWithStringsAndSpecifiedStringOutLength",
                       callMethodWithStringsAndSpecifiedStringOutLength());

            // tests with methods that use exceptions / error enums
            reportTest("callMethodWithoutErrorEnum", callMethodWithoutErrorEnum());
            reportTest("callMethodWithAnonymousErrorEnum", callMethodWithAnonymousErrorEnum());
            reportTest("callMethodWithExistingErrorEnum", callMethodWithExistingErrorEnum());
            reportTest("callMethodWithExtendedErrorEnum", callMethodWithExtendedErrorEnum());

            // asynchronous method calls

            // (only a selection since the messages sent are the same as
            // for the synchronous method calls)
            reportTest("callMethodWithMultipleStructParametersAsync", callMethodWithMultipleStructParametersAsync());
            reportTest("callMethodWithSingleArrayParametersAsync", callMethodWithSingleArrayParametersAsync());
            reportTest("callMethodWithSinglePrimitiveParametersAsync", callMethodWithSinglePrimitiveParametersAsync());

            // asynchronous method calls that use exceptions / error enums
            reportTest("callMethodWithExtendedErrorEnumAsync", callMethodWithExtendedErrorEnumAsync());

            // tests with synchronous attribute setter and getter
            reportTest("callSetAttributeUInt8", callSetAttributeUInt8());
            reportTest("callGetAttributeUInt8", callGetAttributeUInt8());

            reportTest("callSetAttributeDouble", callSetAttributeDouble());
            reportTest("callGetAttributeDouble", callGetAttributeDouble());

            // no setter because attribute is readonly
            reportTest("callGetAttributeBooleanReadOnly", callGetAttributeBooleanReadOnly());

            reportTest("callSetAttributeStringNoSubscriptions", callSetAttributeStringNoSubscriptions());
            reportTest("callGetAttributeStringNoSubscriptions", callGetAttributeStringNoSubscriptions());

            // no setter because attribute is readonly
            reportTest("callGetAttributeInt8readonlyNoSubscriptions", callGetAttributeInt8readonlyNoSubscriptions());

            reportTest("callSetAttributeArrayOfStringImplicit", callSetAttributeArrayOfStringImplicit());
            reportTest("callGetAttributeArrayOfStringImplicit", callGetAttributeArrayOfStringImplicit());

            reportTest("callSetAttributeEnumeration", callSetAttributeEnumeration());
            reportTest("callGetAttributeEnumeration", callGetAttributeEnumeration());

            // no setter because attribute is readonly
            reportTest("callGetAttributeExtendedEnumerationReadonly", callGetAttributeExtendedEnumerationReadonly());

            reportTest("callSetAttributeBaseStruct", callSetAttributeBaseStruct());
            reportTest("callGetAttributeBaseStruct", callGetAttributeBaseStruct());

            reportTest("callSetAttributeExtendedExtendedBaseStruct", callSetAttributeExtendedExtendedBaseStruct());
            reportTest("callGetAttributeExtendedExtendedBaseStruct", callGetAttributeExtendedExtendedBaseStruct());

            // tests with attribute setters / getters that use exceptions
            reportTest("callSetAttributeWithException", callSetAttributeWithException());
            reportTest("callGetAttributeWithException", callGetAttributeWithException());

            // TODO: tests with asynchronous attribute setter and getter

            // tests with subscription to attribute calls
            // (only a selection of possible attributes)
            // expect that setter calls have already placed suitable values
            reportTest("callSubscribeAttributeEnumeration", callSubscribeAttributeEnumeration());
            reportTest("callSubscribeAttributeWithException", callSubscribeAttributeWithException());

            // tests with subscription to broadcast calls
            reportTest("callSubscribeBroadcastWithSinglePrimitiveParameter",
                       callSubscribeBroadcastWithSinglePrimitiveParameter());
            reportTest("callSubscribeBroadcastWithMultiplePrimitiveParameters",
                       callSubscribeBroadcastWithMultiplePrimitiveParameters());
            reportTest("callSubscribeBroadcastWithSingleArrayParameter",
                       callSubscribeBroadcastWithSingleArrayParameter());
            reportTest("callSubscribeBroadcastWithMultipleArrayParameters",
                       callSubscribeBroadcastWithMultipleArrayParameters());
            reportTest("callSubscribeBroadcastWithSingleEnumerationParameter",
                       callSubscribeBroadcastWithSingleEnumerationParameter());
            reportTest("callSubscribeBroadcastWithMultipleEnumerationParameters",
                       callSubscribeBroadcastWithMultipleEnumerationParameters());
            reportTest("callSubscribeBroadcastWithSingleStructParameter",
                       callSubscribeBroadcastWithSingleStructParameter());
            reportTest("callSubscribeBroadcastWithMultipleStructParameters",
                       callSubscribeBroadcastWithMultipleStructParameters());

            reportTest("callSetAttributeMapStringString", callSetAttributeMapStringString());
            reportTest("callGetAttributeMapStringString", callGetAttributeMapStringString());
            reportTest("callmethodWithSingleMapParameters", callmethodWithSingleMapParameters());

            // test with subscription to broadcast calls with filtering
            reportTest("callSubscribeBroadcastWithFiltering", callSubscribeBroadcastWithFiltering());

        } catch (DiscoveryException e) {
            reportTest("proxy built", false);
            LOG.error("No provider found", e);
        } catch (JoynrCommunicationException e) {
            reportTest("proxy built", false);
            LOG.error("The message was not sent: ", e);
        }

        System.exit(evaluateAndPrintResults());
    }
}
