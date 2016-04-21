package io.joynr.test.interlanguage;

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

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;

import java.io.StringWriter;
import java.io.PrintWriter;

import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.exceptions.ProviderRuntimeException;
import joynr.interlanguagetest.Enumeration;
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
import joynr.interlanguagetest.namedTypeCollection1.StructWithStringArray;
import joynr.interlanguagetest.namedTypeCollection2.BaseStructWithoutElements;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedEnumerationWithPartlyDefinedValues;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedExtendedBaseStruct;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedStructOfPrimitives;
import joynr.interlanguagetest.namedTypeCollection2.ExtendedTypeCollectionEnumerationInTypeCollection;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.JsonProcessingException;

import org.junit.Test;
import static org.junit.Assert.fail;

import edu.umd.cs.findbugs.annotations.SuppressWarnings;

public class IltConsumerAttributeSubscriptionTest extends IltConsumerTest {
    private static final Logger LOG = LoggerFactory.getLogger(IltConsumerTest.class);

    /*
     * ATTRIBUTE SUBSCRIPTIONS
     */

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeAttributeEnumerationCallbackDone = false;
    volatile boolean subscribeAttributeEnumerationCallbackResult = false;

    @Test
    public void callSubscribeAttributeEnumeration() {
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

        LOG.info(name.getMethodName() + "");

        try {
            // must set the value before it can be retrieved again via subscription
            Enumeration enumerationArg = Enumeration.ENUM_0_VALUE_2;
            testInterfaceProxy.setAttributeEnumeration(enumerationArg);

            subscriptionId = testInterfaceProxy.subscribeToAttributeEnumeration(new AttributeSubscriptionAdapter<Enumeration>() {
                                                                                    @Override
                                                                                    public void onReceive(Enumeration value) {
                                                                                        if (value == Enumeration.ENUM_0_VALUE_2) {
                                                                                            LOG.info(name.getMethodName()
                                                                                                    + " - callback - got publication with correct value");
                                                                                            subscribeAttributeEnumerationCallbackResult = true;
                                                                                        } else {
                                                                                            subscribeAttributeEnumerationCallbackResult = false;
                                                                                            LOG.info(name.getMethodName()
                                                                                                    + " - callback - got publication with invalid value");
                                                                                        }
                                                                                        subscribeAttributeEnumerationCallbackDone = true;
                                                                                    }

                                                                                    @Override
                                                                                    public void onError(JoynrRuntimeException error) {
                                                                                        LOG.info(name.getMethodName()
                                                                                                + " - callback - got unexpected exception");
                                                                                        subscribeAttributeEnumerationCallbackResult = false;
                                                                                        subscribeAttributeEnumerationCallbackDone = true;
                                                                                    }
                                                                                },
                                                                                subscriptionQos);
            LOG.info(name.getMethodName() + " - subscription successful");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeAttributeEnumerationCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (subscribeAttributeEnumerationCallbackDone && subscribeAttributeEnumerationCallbackResult) {
                result = true;
            } else {
                fail(name.getMethodName() + " - FAILED - callback NOT done");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromAttributeEnumeration(subscriptionId);
                LOG.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
                result = false;
            }

            if (!result) {
                LOG.info(name.getMethodName() + " - FAILED");
            } else {
                LOG.info(name.getMethodName() + " - OK");
            }
            return;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeAttributeWithExceptionCallbackDone = false;
    volatile boolean subscribeAttributeWithExceptionCallbackResult = false;

    @Test
    public void callSubscribeAttributeWithException() {
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

        LOG.info(name.getMethodName() + "");

        try {
            subscriptionId = testInterfaceProxy.subscribeToAttributeWithException(new AttributeSubscriptionAdapter<Boolean>() {
                                                                                      @Override
                                                                                      public void onReceive(Boolean value) {
                                                                                          LOG.info(name.getMethodName()
                                                                                                  + " - callback - got unexpected publication");
                                                                                          subscribeAttributeWithExceptionCallbackResult = false;
                                                                                          subscribeAttributeWithExceptionCallbackDone = true;
                                                                                      }

                                                                                      @Override
                                                                                      public void onError(JoynrRuntimeException error) {
                                                                                          if (error instanceof ProviderRuntimeException) {
                                                                                              if (((ProviderRuntimeException) error).getMessage()
                                                                                                                                    .equals("Exception from getAttributeWithException")) {
                                                                                                  LOG.info(name.getMethodName()
                                                                                                          + " - callback - got expected exception "
                                                                                                          + ((JoynrRuntimeException) error).getMessage());
                                                                                                  subscribeAttributeWithExceptionCallbackResult = true;
                                                                                                  subscribeAttributeWithExceptionCallbackDone = true;
                                                                                                  return;
                                                                                              }
                                                                                              LOG.info(name.getMethodName()
                                                                                                      + " - callback - caught invalid exception "
                                                                                                      + ((JoynrRuntimeException) error).getMessage());
                                                                                          } else if (error instanceof JoynrRuntimeException) {
                                                                                              LOG.info(name.getMethodName()
                                                                                                      + " - callback - caught invalid exception "
                                                                                                      + ((JoynrRuntimeException) error).getMessage());
                                                                                          } else {
                                                                                              LOG.info(name.getMethodName()
                                                                                                      + " - callback - caught invalid exception ");
                                                                                          }
                                                                                          subscribeAttributeWithExceptionCallbackResult = false;
                                                                                          subscribeAttributeWithExceptionCallbackDone = true;
                                                                                      }
                                                                                  },
                                                                                  subscriptionQos);
            LOG.info(name.getMethodName() + " - subscription successful");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeAttributeWithExceptionCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeAttributeWithExceptionCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
                result = false;
            } else if (subscribeAttributeWithExceptionCallbackResult) {
                LOG.info(name.getMethodName() + " - callback got called and received expected exception");
                result = true;
            } else {
                fail(name.getMethodName() + " - FAILED - callback got called but received unexpected result");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromAttributeWithException(subscriptionId);
                LOG.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on unsubscribe: " + e.getMessage());
                result = false;
            }

            if (!result) {
                LOG.info(name.getMethodName() + " - FAILED");
            } else {
                LOG.info(name.getMethodName() + " - OK");
            }
            return;
        } catch (Exception e) {
            // also catches InterruptedException from Thread.sleep() call
            LOG.info(name.getMethodName() + " - caught unexpected exception");
            LOG.info(name.getMethodName() + " - FAILED");
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
    }
}
