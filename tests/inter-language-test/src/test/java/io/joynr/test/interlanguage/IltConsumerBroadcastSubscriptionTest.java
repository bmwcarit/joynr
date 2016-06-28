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

import joynr.OnChangeWithKeepAliveSubscriptionQos;
import joynr.interlanguagetest.Enumeration;
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

import org.junit.Test;
import static org.junit.Assert.fail;

public class IltConsumerBroadcastSubscriptionTest extends IltConsumerTest {
    private static final Logger LOG = LoggerFactory.getLogger(IltConsumerTest.class);

    /*
     * BROADCAST SUBSCRIPTIONS
     */

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithSinglePrimitiveParameter() {
        String subscriptionId;
        int minIntervalMs = 0;
        int maxIntervalMs = 10000;
        long validityMs = 60000;
        int alertAfterIntervalMs = 20000;
        int publicationTtlMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMinIntervalMs(minIntervalMs)
                                                                                                         .setMaxIntervalMs(maxIntervalMs)
                                                                                                         .setValidityMs(validityMs)
                                                                                                         .setAlertAfterIntervalMs(alertAfterIntervalMs)
                                                                                                         .setPublicationTtlMs(publicationTtlMs);
        boolean result;

        LOG.info(name.getMethodName() + "");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithSinglePrimitiveParameterBroadcast(new BroadcastWithSinglePrimitiveParameterBroadcastAdapter() {
                                                                                                              @Override
                                                                                                              public void onReceive(String stringOut) {
                                                                                                                  LOG.info(name.getMethodName()
                                                                                                                          + " - callback - got broadcast");
                                                                                                                  if (!stringOut.equals("boom")) {
                                                                                                                      LOG.info(name.getMethodName()
                                                                                                                              + " - callback - invalid content");
                                                                                                                      subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;
                                                                                                                  } else {
                                                                                                                      LOG.info(name.getMethodName()
                                                                                                                              + " - callback - content OK");
                                                                                                                      subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = true;
                                                                                                                  }
                                                                                                                  subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = true;
                                                                                                              }

                                                                                                              @Override
                                                                                                              public void onError() {
                                                                                                                  LOG.info(name.getMethodName()
                                                                                                                          + " - callback - error");
                                                                                                                  subscribeBroadcastWithSinglePrimitiveParameterCallbackResult = false;
                                                                                                                  subscribeBroadcastWithSinglePrimitiveParameterCallbackDone = true;
                                                                                                              }
                                                                                                          },
                                                                                                          subscriptionQos);
            LOG.info(name.getMethodName() + " - subscription successful");
            LOG.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            LOG.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSinglePrimitiveParameter();
            LOG.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSinglePrimitiveParameterCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeBroadcastWithSinglePrimitiveParameterCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithSinglePrimitiveParameterCallbackResult) {
                LOG.info(name.getMethodName() + " - callback got called and received expected publication");
                result = true;
            } else {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSinglePrimitiveParameterBroadcast(subscriptionId);
                LOG.info(name.getMethodName() + " - unsubscribe successful");
            } catch (Exception e) {
                fail(name.getMethodName() + " - FAILED - caught unexpected exception on subscribe: " + e.getMessage());
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
    volatile boolean subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithMultiplePrimitiveParameters() {
        String subscriptionId;
        int minIntervalMs = 0;
        int maxIntervalMs = 10000;
        long validityMs = 60000;
        int alertAfterIntervalMs = 20000;
        int publicationTtlMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMinIntervalMs(minIntervalMs)
                                                                                                         .setMaxIntervalMs(maxIntervalMs)
                                                                                                         .setValidityMs(validityMs)
                                                                                                         .setAlertAfterIntervalMs(alertAfterIntervalMs)
                                                                                                         .setPublicationTtlMs(publicationTtlMs);
        boolean result;

        LOG.info(name.getMethodName() + "");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithMultiplePrimitiveParametersBroadcast(new BroadcastWithMultiplePrimitiveParametersBroadcastAdapter() {
                                                                                                                 @Override
                                                                                                                 public void onReceive(Double doubleOut,
                                                                                                                                       String stringOut) {
                                                                                                                     LOG.info(name.getMethodName()
                                                                                                                             + " - callback - got broadcast");
                                                                                                                     if (!stringOut.equals("boom")
                                                                                                                             || !IltUtil.cmpDouble(doubleOut,
                                                                                                                                                   1.1d)) {
                                                                                                                         LOG.info(name.getMethodName()
                                                                                                                                 + " - callback - invalid content");
                                                                                                                         subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
                                                                                                                     } else {
                                                                                                                         LOG.info(name.getMethodName()
                                                                                                                                 + " - callback - content OK");
                                                                                                                         subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = true;
                                                                                                                     }
                                                                                                                     subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = true;
                                                                                                                 }

                                                                                                                 @Override
                                                                                                                 public void onError() {
                                                                                                                     LOG.info(name.getMethodName()
                                                                                                                             + " - callback - error");
                                                                                                                     subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult = false;
                                                                                                                     subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone = true;
                                                                                                                 }
                                                                                                             },
                                                                                                             subscriptionQos);
            LOG.info(name.getMethodName() + " - subscription successful");
            LOG.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            LOG.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultiplePrimitiveParameters();
            LOG.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeBroadcastWithMultiplePrimitiveParametersCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithMultiplePrimitiveParametersCallbackResult) {
                LOG.info(name.getMethodName() + " - callback got called and received expected publication");
                result = true;
            } else {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultiplePrimitiveParametersBroadcast(subscriptionId);
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
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSingleArrayParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSingleArrayParameterCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithSingleArrayParameter() {
        String subscriptionId;
        int minIntervalMs = 0;
        int maxIntervalMs = 10000;
        long validityMs = 60000;
        int alertAfterIntervalMs = 20000;
        int publicationTtlMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMinIntervalMs(minIntervalMs)
                                                                                                         .setMaxIntervalMs(maxIntervalMs)
                                                                                                         .setValidityMs(validityMs)
                                                                                                         .setAlertAfterIntervalMs(alertAfterIntervalMs)
                                                                                                         .setPublicationTtlMs(publicationTtlMs);
        boolean result;

        LOG.info(name.getMethodName() + "");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithSingleArrayParameterBroadcast(new BroadcastWithSingleArrayParameterBroadcastAdapter() {
                                                                                                          @Override
                                                                                                          public void onReceive(String[] stringArrayOut) {
                                                                                                              //
                                                                                                              LOG.info(name.getMethodName()
                                                                                                                      + " - callback - got broadcast");
                                                                                                              if (!IltUtil.checkStringArray(stringArrayOut)) {
                                                                                                                  LOG.info(name.getMethodName()
                                                                                                                          + " - callback - invalid content");
                                                                                                                  subscribeBroadcastWithSingleArrayParameterCallbackResult = false;
                                                                                                              } else {
                                                                                                                  LOG.info(name.getMethodName()
                                                                                                                          + " - callback - content OK");
                                                                                                                  subscribeBroadcastWithSingleArrayParameterCallbackResult = true;
                                                                                                              }
                                                                                                              subscribeBroadcastWithSingleArrayParameterCallbackDone = true;
                                                                                                          }

                                                                                                          @Override
                                                                                                          public void onError() {
                                                                                                              LOG.info(name.getMethodName()
                                                                                                                      + " - callback - error");
                                                                                                              subscribeBroadcastWithSingleArrayParameterCallbackResult = false;
                                                                                                              subscribeBroadcastWithSingleArrayParameterCallbackDone = true;
                                                                                                          }
                                                                                                      },
                                                                                                      subscriptionQos);
            LOG.info(name.getMethodName() + " - subscription successful");
            LOG.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            LOG.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSingleArrayParameter();
            LOG.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSingleArrayParameterCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeBroadcastWithSingleArrayParameterCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithSingleArrayParameterCallbackResult) {
                LOG.info(name.getMethodName() + " - callback got called and received expected publication");
                result = true;
            } else {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSingleArrayParameterBroadcast(subscriptionId);
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
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithMultipleArrayParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithMultipleArrayParameters() {
        String subscriptionId;
        int minIntervalMs = 0;
        int maxIntervalMs = 10000;
        long validityMs = 60000;
        int alertAfterIntervalMs = 20000;
        int publicationTtlMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMinIntervalMs(minIntervalMs)
                                                                                                         .setMaxIntervalMs(maxIntervalMs)
                                                                                                         .setValidityMs(validityMs)
                                                                                                         .setAlertAfterIntervalMs(alertAfterIntervalMs)
                                                                                                         .setPublicationTtlMs(publicationTtlMs);
        boolean result;

        LOG.info(name.getMethodName() + "");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithMultipleArrayParametersBroadcast(new BroadcastWithMultipleArrayParametersBroadcastAdapter() {
                                                                                                             @Override
                                                                                                             public void onReceive(Long[] uInt64ArrayOut,
                                                                                                                                   StructWithStringArray[] structWithStringArrayArrayOut) {
                                                                                                                 LOG.info(name.getMethodName()
                                                                                                                         + " - callback - got broadcast");
                                                                                                                 if (!IltUtil.checkUInt64Array(uInt64ArrayOut)
                                                                                                                         || !IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
                                                                                                                     LOG.info(name.getMethodName()
                                                                                                                             + " - callback - invalid content");
                                                                                                                     subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
                                                                                                                 } else {
                                                                                                                     LOG.info(name.getMethodName()
                                                                                                                             + " - callback - content OK");
                                                                                                                     subscribeBroadcastWithMultipleArrayParametersCallbackResult = true;
                                                                                                                 }
                                                                                                                 subscribeBroadcastWithMultipleArrayParametersCallbackDone = true;
                                                                                                             }

                                                                                                             @Override
                                                                                                             public void onError() {
                                                                                                                 LOG.info(name.getMethodName()
                                                                                                                         + " - callback - error");
                                                                                                                 subscribeBroadcastWithMultipleArrayParametersCallbackResult = false;
                                                                                                                 subscribeBroadcastWithMultipleArrayParametersCallbackDone = true;
                                                                                                             }
                                                                                                         },
                                                                                                         subscriptionQos);
            LOG.info(name.getMethodName() + " - subscription successful");
            LOG.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            LOG.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultipleArrayParameters();
            LOG.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultipleArrayParametersCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeBroadcastWithMultipleArrayParametersCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithMultipleArrayParametersCallbackResult) {
                LOG.info(name.getMethodName() + " - callback got called and received expected publication");
                result = true;
            } else {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication event");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultipleArrayParametersBroadcast(subscriptionId);
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
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSingleEnumerationParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithSingleEnumerationParameter() {
        String subscriptionId;
        int minIntervalMs = 0;
        int maxIntervalMs = 10000;
        long validityMs = 60000;
        int alertAfterIntervalMs = 20000;
        int publicationTtlMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMinIntervalMs(minIntervalMs)
                                                                                                         .setMaxIntervalMs(maxIntervalMs)
                                                                                                         .setValidityMs(validityMs)
                                                                                                         .setAlertAfterIntervalMs(alertAfterIntervalMs)
                                                                                                         .setPublicationTtlMs(publicationTtlMs);
        boolean result;

        LOG.info(name.getMethodName() + "");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithSingleEnumerationParameterBroadcast(new BroadcastWithSingleEnumerationParameterBroadcastAdapter() {
                                                                                                                @Override
                                                                                                                public void onReceive(ExtendedTypeCollectionEnumerationInTypeCollection enumerationOut) {
                                                                                                                    LOG.info(name.getMethodName()
                                                                                                                            + " - callback - got broadcast");
                                                                                                                    if (enumerationOut != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION) {
                                                                                                                        LOG.info(name.getMethodName()
                                                                                                                                + " - callback - invalid content");
                                                                                                                        subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
                                                                                                                    } else {
                                                                                                                        LOG.info(name.getMethodName()
                                                                                                                                + " - callback - content OK");
                                                                                                                        subscribeBroadcastWithSingleEnumerationParameterCallbackResult = true;
                                                                                                                    }
                                                                                                                    subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
                                                                                                                }

                                                                                                                @Override
                                                                                                                public void onError() {
                                                                                                                    LOG.info(name.getMethodName()
                                                                                                                            + " - callback - error");
                                                                                                                    subscribeBroadcastWithSingleEnumerationParameterCallbackResult = false;
                                                                                                                    subscribeBroadcastWithSingleEnumerationParameterCallbackDone = true;
                                                                                                                }
                                                                                                            },
                                                                                                            subscriptionQos);
            LOG.info(name.getMethodName() + " - subscription successful");
            LOG.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            LOG.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSingleEnumerationParameter();
            LOG.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSingleEnumerationParameterCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeBroadcastWithSingleEnumerationParameterCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithSingleEnumerationParameterCallbackResult) {
                LOG.info(name.getMethodName() + " - callback got called and received expected publication");
                result = true;
            } else {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSingleEnumerationParameterBroadcast(subscriptionId);
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
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithMultipleEnumerationParameters() {
        String subscriptionId;
        int minIntervalMs = 0;
        int maxIntervalMs = 10000;
        long validityMs = 60000;
        int alertAfterIntervalMs = 20000;
        int publicationTtlMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMinIntervalMs(minIntervalMs)
                                                                                                         .setMaxIntervalMs(maxIntervalMs)
                                                                                                         .setValidityMs(validityMs)
                                                                                                         .setAlertAfterIntervalMs(alertAfterIntervalMs)
                                                                                                         .setPublicationTtlMs(publicationTtlMs);
        boolean result;

        LOG.info(name.getMethodName() + "");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithMultipleEnumerationParametersBroadcast(new BroadcastWithMultipleEnumerationParametersBroadcastAdapter() {
                                                                                                                   @Override
                                                                                                                   public void onReceive(ExtendedEnumerationWithPartlyDefinedValues extendedEnumerationOut,
                                                                                                                                         Enumeration enumerationOut) {
                                                                                                                       LOG.info(name.getMethodName()
                                                                                                                               + " - callback - got broadcast");
                                                                                                                       if (extendedEnumerationOut != ExtendedEnumerationWithPartlyDefinedValues.ENUM_2_VALUE_EXTENSION_FOR_ENUM_WITHOUT_DEFINED_VALUES
                                                                                                                               || enumerationOut != Enumeration.ENUM_0_VALUE_1) {
                                                                                                                           LOG.info(name.getMethodName()
                                                                                                                                   + " - callback - invalid content");
                                                                                                                           subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
                                                                                                                       } else {
                                                                                                                           LOG.info(name.getMethodName()
                                                                                                                                   + " - callback - content OK");
                                                                                                                           subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = true;
                                                                                                                       }
                                                                                                                       subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = true;
                                                                                                                   }

                                                                                                                   @Override
                                                                                                                   public void onError() {
                                                                                                                       LOG.info(name.getMethodName()
                                                                                                                               + " - callback - error");
                                                                                                                       subscribeBroadcastWithMultipleEnumerationParametersCallbackResult = false;
                                                                                                                       subscribeBroadcastWithMultipleEnumerationParametersCallbackDone = true;
                                                                                                                   }
                                                                                                               },
                                                                                                               subscriptionQos);
            LOG.info(name.getMethodName() + " - subscription successful");
            LOG.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            LOG.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultipleEnumerationParameters();
            LOG.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultipleEnumerationParametersCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeBroadcastWithMultipleEnumerationParametersCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithMultipleEnumerationParametersCallbackResult) {
                LOG.info(name.getMethodName() + " - callback got called and received expected publication");
                result = true;
            } else {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultipleEnumerationParametersBroadcast(subscriptionId);
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
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
    }

    // variables that are to be changed inside callbacks must be declared global
    volatile boolean subscribeBroadcastWithSingleStructParameterCallbackDone = false;
    volatile boolean subscribeBroadcastWithSingleStructParameterCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithSingleStructParameter() {
        String subscriptionId;
        int minIntervalMs = 0;
        int maxIntervalMs = 10000;
        long validityMs = 60000;
        int alertAfterIntervalM = 20000;
        int publicationTtlMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMinIntervalMs(minIntervalMs)
                                                                                                         .setMaxIntervalMs(maxIntervalMs)
                                                                                                         .setValidityMs(validityMs)
                                                                                                         .setAlertAfterIntervalMs(alertAfterIntervalM)
                                                                                                         .setPublicationTtlMs(publicationTtlMs);
        boolean result;

        LOG.info(name.getMethodName() + "");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithSingleStructParameterBroadcast(new BroadcastWithSingleStructParameterBroadcastAdapter() {
                                                                                                           @Override
                                                                                                           public void onReceive(ExtendedStructOfPrimitives extendedStructOfPrimitivesOut) {
                                                                                                               LOG.info(name.getMethodName()
                                                                                                                       + " - callback - got broadcast");
                                                                                                               if (!IltUtil.checkExtendedStructOfPrimitives(extendedStructOfPrimitivesOut)) {
                                                                                                                   LOG.info(name.getMethodName()
                                                                                                                           + " - callback - invalid content");
                                                                                                                   subscribeBroadcastWithSingleStructParameterCallbackResult = false;
                                                                                                               } else {
                                                                                                                   LOG.info(name.getMethodName()
                                                                                                                           + " - callback - content OK");
                                                                                                                   subscribeBroadcastWithSingleStructParameterCallbackResult = true;
                                                                                                               }
                                                                                                               subscribeBroadcastWithSingleStructParameterCallbackDone = true;
                                                                                                           }

                                                                                                           @Override
                                                                                                           public void onError() {
                                                                                                               LOG.info(name.getMethodName()
                                                                                                                       + " - callback - error");
                                                                                                               subscribeBroadcastWithSingleStructParameterCallbackResult = false;
                                                                                                               subscribeBroadcastWithSingleStructParameterCallbackDone = true;
                                                                                                           }
                                                                                                       },
                                                                                                       subscriptionQos);
            LOG.info(name.getMethodName() + " - subscription successful");
            LOG.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            LOG.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithSingleStructParameter();
            LOG.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithSingleStructParameterCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeBroadcastWithSingleStructParameterCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get called in time");
                result = false;
            } else if (subscribeBroadcastWithSingleStructParameterCallbackResult) {
                LOG.info(name.getMethodName() + " - callback got called and received expected publication");
                result = true;
            } else {
                fail(name.getMethodName()
                        + " - FAILED - callback got callback but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithSingleStructParameterBroadcast(subscriptionId);
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
    volatile boolean subscribeBroadcastWithMultipleStructParametersCallbackDone = false;
    volatile boolean subscribeBroadcastWithMultipleStructParametersCallbackResult = false;

    @Test
    public void callSubscribeBroadcastWithMultipleStructParameters() {
        String subscriptionId;
        int minIntervalMs = 0;
        int maxIntervalMs = 10000;
        long validityMs = 60000;
        int alertAfterIntervalMs = 20000;
        int publicationTtlMs = 5000;
        OnChangeWithKeepAliveSubscriptionQos subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos().setMinIntervalMs(minIntervalMs)
                                                                                                         .setMaxIntervalMs(maxIntervalMs)
                                                                                                         .setValidityMs(validityMs)
                                                                                                         .setAlertAfterIntervalMs(alertAfterIntervalMs)
                                                                                                         .setPublicationTtlMs(publicationTtlMs);
        boolean result;

        LOG.info(name.getMethodName() + "");

        try {
            subscriptionId = testInterfaceProxy.subscribeToBroadcastWithMultipleStructParametersBroadcast(new BroadcastWithMultipleStructParametersBroadcastAdapter() {
                                                                                                              @Override
                                                                                                              public void onReceive(BaseStructWithoutElements baseStructWithoutElementsOut,
                                                                                                                                    ExtendedExtendedBaseStruct extendedExtendedBaseStructOut) {
                                                                                                                  LOG.info(name.getMethodName()
                                                                                                                          + " - callback - got broadcast");
                                                                                                                  if (!IltUtil.checkBaseStructWithoutElements(baseStructWithoutElementsOut)
                                                                                                                          || !IltUtil.checkExtendedExtendedBaseStruct(extendedExtendedBaseStructOut)) {
                                                                                                                      LOG.info(name.getMethodName()
                                                                                                                              + " - callback - invalid content");
                                                                                                                      subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
                                                                                                                  } else {
                                                                                                                      LOG.info(name.getMethodName()
                                                                                                                              + " - callback - content OK");
                                                                                                                      subscribeBroadcastWithMultipleStructParametersCallbackResult = true;
                                                                                                                  }
                                                                                                                  subscribeBroadcastWithMultipleStructParametersCallbackDone = true;
                                                                                                              }

                                                                                                              @Override
                                                                                                              public void onError() {
                                                                                                                  LOG.info(name.getMethodName()
                                                                                                                          + " - callback - error");
                                                                                                                  subscribeBroadcastWithMultipleStructParametersCallbackResult = false;
                                                                                                                  subscribeBroadcastWithMultipleStructParametersCallbackDone = true;
                                                                                                              }
                                                                                                          },
                                                                                                          subscriptionQos);
            LOG.info(name.getMethodName() + " - subscription successful");
            LOG.info(name.getMethodName() + " - Waiting one second");
            Thread.sleep(1000);
            LOG.info(name.getMethodName() + " - Wait done, invoking fire method");
            testInterfaceProxy.methodToFireBroadcastWithMultipleStructParameters();
            LOG.info(name.getMethodName() + " - fire method invoked");

            // check results from callback; expect to be finished within 1 second
            // should have been called ahead anyway
            if (subscribeBroadcastWithMultipleStructParametersCallbackDone == false) {
                LOG.info(name.getMethodName() + " - about to wait for a second for callback");
                Thread.sleep(1000);
                LOG.info(name.getMethodName() + " - wait for callback is over");
            } else {
                LOG.info(name.getMethodName() + " - callback already done");
            }
            if (!subscribeBroadcastWithMultipleStructParametersCallbackDone) {
                fail(name.getMethodName() + " - FAILED - callback did not get callback in time");
                result = false;
            } else if (subscribeBroadcastWithMultipleStructParametersCallbackResult) {
                LOG.info(name.getMethodName() + " - callback got called and received expected publication");
                result = true;
            } else {
                fail(name.getMethodName()
                        + " - FAILED - callback got called but received unexpected error or publication content");
                result = false;
            }

            // try to unsubscribe in any case
            try {
                testInterfaceProxy.unsubscribeFromBroadcastWithMultipleStructParametersBroadcast(subscriptionId);
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
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
    }
}
