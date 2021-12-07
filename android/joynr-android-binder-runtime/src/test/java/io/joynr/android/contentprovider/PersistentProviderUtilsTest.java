/*-
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.android.contentprovider;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.when;
import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.google.inject.Injector;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import java.util.Properties;

import io.joynr.android.AndroidBinderRuntime;
import io.joynr.capabilities.PropertiesFileParticipantIdStorage;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.provider.ProviderContainer;
import io.joynr.provider.ProviderContainerFactory;
import io.joynr.runtime.PropertyLoader;
import io.joynr.util.VersionUtil;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

@RunWith(AndroidJUnit4.class)
public class PersistentProviderUtilsTest {

    private String testDomain;
    private ProviderQos testProviderQos;
    private PersistentJoynrContentProvider.PersistentProvider testPersistentProvider;

    @Mock
    private Injector injector;

    @Mock
    private ProviderContainerFactory providerContainerFactory;

    @Mock
    private ProviderContainer providerContainer;

    @Mock
    private PropertiesFileParticipantIdStorage participantIdStorage;

    @Mock
    private AbstractJoynrProvider joynrProvider;

    private String testParticipantId;
    private String testInterfaceName;
    private int testMajorVersion;
    private int testMinorVersion;

    private String testPublicKeyId;

    private DiscoveryEntry testDiscoveryEntry;

    private long testDefaultExpiryTimeMs;

    @Before
    public void setup() {

        MockitoAnnotations.openMocks(this);

        testDomain = "domain";
        testProviderQos = new ProviderQos();
        testProviderQos.setScope(ProviderScope.LOCAL);

        testPersistentProvider = new PersistentJoynrContentProvider.PersistentProvider(
                joynrProvider,
                testDomain,
                testProviderQos
        );

        testParticipantId = "participantId";
        testInterfaceName = "interfaceName";
        testMajorVersion = 1;
        testMinorVersion = 0;

        testPublicKeyId = "";

        testDiscoveryEntry = new DiscoveryEntry();
        testDiscoveryEntry.setQos(testProviderQos);
        testDiscoveryEntry.setPublicKeyId(testPublicKeyId);
        testDiscoveryEntry.setProviderVersion(new Version(testMajorVersion, testMinorVersion));
        testDiscoveryEntry.setDomain(testDomain);
        testDiscoveryEntry.setParticipantId(testParticipantId);
        testDiscoveryEntry.setInterfaceName(testInterfaceName);
        testDiscoveryEntry.setExpiryDateMs(0L);
        testDiscoveryEntry.setLastSeenDateMs(0L);

        testDefaultExpiryTimeMs = 1L;

    }

    @Test
    public void generatePrimitiveDiscoveryEntry_whenGoodPersistentProvider_returnsDiscoveryEntry() {

        MockedStatic<AndroidBinderRuntime> androidBinderRuntimeMockedStatic = Mockito.mockStatic(AndroidBinderRuntime.class);
        MockedStatic<VersionUtil> versionUtilMockedStatic = Mockito.mockStatic(VersionUtil.class);

        versionUtilMockedStatic.when(() -> VersionUtil.getVersionFromAnnotation(any())).thenReturn(new Version(1, 0));
        when(providerContainer.getInterfaceName()).thenReturn(testInterfaceName);
        when(providerContainer.getMajorVersion()).thenReturn(testMajorVersion);
        when(providerContainerFactory.create(any())).thenReturn(providerContainer);
        when(injector.getInstance(ProviderContainerFactory.class)).thenReturn(providerContainerFactory);
        when(participantIdStorage.getProviderParticipantId(anyString(), anyString(), anyInt())).thenReturn(testParticipantId);
        when(injector.getInstance(PropertiesFileParticipantIdStorage.class)).thenReturn(participantIdStorage);
        androidBinderRuntimeMockedStatic.when(AndroidBinderRuntime::getInjector).thenReturn(injector);

        DiscoveryEntry discoveryEntry = PersistentProviderUtils.generatePrimitiveDiscoveryEntry(testPersistentProvider, testDefaultExpiryTimeMs);

        assertNotNull(discoveryEntry);
        assertEquals(testDomain, discoveryEntry.getDomain());
        assertEquals((Integer) testMajorVersion, discoveryEntry.getProviderVersion().getMajorVersion());
        assertEquals(testInterfaceName, discoveryEntry.getInterfaceName());
        assertEquals(testParticipantId, discoveryEntry.getParticipantId());
        assertEquals(testPublicKeyId, discoveryEntry.getPublicKeyId());
        assertEquals(testProviderQos, discoveryEntry.getQos());

        androidBinderRuntimeMockedStatic.close();
        versionUtilMockedStatic.close();
    }

    @Test
    public void generatePrimitiveDiscoveryEntry_whenNullPersistentProvider_returnsNull() {

        assertNull(PersistentProviderUtils.generatePrimitiveDiscoveryEntry(null, testDefaultExpiryTimeMs));
    }

    @Test
    public void generatePrimitiveDiscoveryEntry_whenNullJoynrProvider_returnsNull() {

        assertNull(
                PersistentProviderUtils.generatePrimitiveDiscoveryEntry(
                    new PersistentJoynrContentProvider.PersistentProvider(
                        null,
                        testDomain,
                        testProviderQos
                    ), testDefaultExpiryTimeMs
                )
        );
    }

    @Test
    public void generatePrimitiveDiscoveryEntry_whenNullDomain_returnsNull() {

        assertNull(
                PersistentProviderUtils.generatePrimitiveDiscoveryEntry(
                    new PersistentJoynrContentProvider.PersistentProvider(
                        joynrProvider,
                        null,
                        testProviderQos
                    ), testDefaultExpiryTimeMs
                )
        );
    }

    @Test
    public void generatePrimitiveDiscoveryEntry_whenNullProvideQos_returnsNull() {

        assertNull(
                PersistentProviderUtils.generatePrimitiveDiscoveryEntry(
                    new PersistentJoynrContentProvider.PersistentProvider(
                        joynrProvider,
                        testDomain,
                        null
                    ), testDefaultExpiryTimeMs
                )
        );
    }

    @Test
    public void getDefaultExpiryTimeMs_returnsConfiguredDefaultExpiryTimeMsFromProperties() {

        Properties testDefaultMessagingProperties = new Properties();
        testDefaultMessagingProperties.setProperty(PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS, String.valueOf(testDefaultExpiryTimeMs));

        MockedStatic<PropertyLoader> propertyLoaderMockedStatic = Mockito.mockStatic(PropertyLoader.class);
        propertyLoaderMockedStatic.when(() -> PropertyLoader.loadProperties(anyString())).thenReturn(testDefaultMessagingProperties);

        long getDefaultExpiryTimeMs = PersistentProviderUtils.getDefaultExpiryTimeMs();

        assertEquals(testDefaultExpiryTimeMs, getDefaultExpiryTimeMs);

        propertyLoaderMockedStatic.close();
    }

    @After
    public void cleanup() {
        reset(injector);
        reset(providerContainerFactory);
        reset(providerContainer);
        reset(participantIdStorage);
        reset(joynrProvider);
    }
}
