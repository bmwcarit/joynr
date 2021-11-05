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
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.when;

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import com.google.inject.Injector;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;
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
public class PersistentJoynrContentProviderTest {

    @Mock
    private Context context;

    private String testPackageName;
    private String wrongTestPackageName;
    private String testDomain;
    private ProviderQos testProviderQos;
    private PersistentJoynrContentProvider.PersistentProvider testPersistentProvider;

    private MockedStatic<PersistentJoynrContentProvider> persistentJoynrContentProviderMockedStatic;

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

    private PersistentJoynrContentProvider testPersistentJoynrContentProvider;
    private PersistentJoynrContentProvider badTestPersistentJoynrContentProvider;

    private List<PersistentJoynrContentProvider.PersistentProvider> testProviderList;

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

        testPackageName = "io.joynr.android.contentprovider";
        wrongTestPackageName = "io.joynr.android.wrongpackagename";
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

        testProviderList = new ArrayList<>();

        testPersistentJoynrContentProvider = new PersistentJoynrContentProvider() {
            @Override
            public List<PersistentProvider> registerPersistentProvider() {
                return testProviderList;
            }
        };

        badTestPersistentJoynrContentProvider = new PersistentJoynrContentProvider() {
            @Override
            public List<PersistentProvider> registerPersistentProvider() {
                return null;
            }
        };

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

        DiscoveryEntry discoveryEntry = PersistentJoynrContentProvider.generatePrimitiveDiscoveryEntry(testPersistentProvider);

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

        assertNull(PersistentJoynrContentProvider.generatePrimitiveDiscoveryEntry(null));
    }

    @Test
    public void generatePrimitiveDiscoveryEntry_whenNullJoynrProvider_returnsNull() {

        assertNull(
                PersistentJoynrContentProvider.generatePrimitiveDiscoveryEntry(
                    new PersistentJoynrContentProvider.PersistentProvider(
                        null,
                        testDomain,
                        testProviderQos
                    )
                )
        );
    }

    @Test
    public void generatePrimitiveDiscoveryEntry_whenNullDomain_returnsNull() {

        assertNull(
                PersistentJoynrContentProvider.generatePrimitiveDiscoveryEntry(
                    new PersistentJoynrContentProvider.PersistentProvider(
                        joynrProvider,
                        null,
                        testProviderQos
                    )
                )
        );
    }

    @Test
    public void generatePrimitiveDiscoveryEntry_whenNullProvideQos_returnsNull() {

        assertNull(
                PersistentJoynrContentProvider.generatePrimitiveDiscoveryEntry(
                    new PersistentJoynrContentProvider.PersistentProvider(
                        joynrProvider,
                        testDomain,
                        null
                    )
                )
        );
    }

    @Test
    public void init_whenRegisterPersistentProviderIsConfigured_variablesAreInitAndInitIsTrue() {

        persistentJoynrContentProviderMockedStatic = Mockito.mockStatic(PersistentJoynrContentProvider.class);

        persistentJoynrContentProviderMockedStatic.when(PersistentJoynrContentProvider::getDefaultExpiryTimeMs).thenReturn(testDefaultExpiryTimeMs);
        testProviderList.add(testPersistentProvider);
        when(context.getPackageName()).thenReturn(testPackageName);

        assertTrue(testPersistentJoynrContentProvider.init(context));

        assertNotNull(testPersistentJoynrContentProvider.uriMatcher);
        assertEquals(testProviderList, testPersistentJoynrContentProvider.providers);
        assertEquals(PersistentJoynrContentProvider.defaultExpiryTimeMs, testDefaultExpiryTimeMs);

        persistentJoynrContentProviderMockedStatic.close();
    }

    @Test
    public void init_whenBadRegisterPersistentProvider_providersListIsNullAndInitIsFalse() {

        persistentJoynrContentProviderMockedStatic = Mockito.mockStatic(PersistentJoynrContentProvider.class);

        persistentJoynrContentProviderMockedStatic.when(PersistentJoynrContentProvider::getDefaultExpiryTimeMs).thenReturn(testDefaultExpiryTimeMs);
        testProviderList.add(testPersistentProvider);
        when(context.getPackageName()).thenReturn(testPackageName);

        assertFalse(badTestPersistentJoynrContentProvider.init(context));

        assertNotNull(badTestPersistentJoynrContentProvider.uriMatcher);
        assertNull(badTestPersistentJoynrContentProvider.providers);
        assertEquals(PersistentJoynrContentProvider.defaultExpiryTimeMs, testDefaultExpiryTimeMs);

        persistentJoynrContentProviderMockedStatic.close();
    }


    @Test
    public void doQuery_whenRegisterPersistentProviderConfigured_returnsDiscoveryEntry() {

        persistentJoynrContentProviderMockedStatic = Mockito.mockStatic(PersistentJoynrContentProvider.class);

        persistentJoynrContentProviderMockedStatic.when(PersistentJoynrContentProvider::getDefaultExpiryTimeMs).thenReturn(testDefaultExpiryTimeMs);
        persistentJoynrContentProviderMockedStatic.when(() -> PersistentJoynrContentProvider.generatePrimitiveDiscoveryEntry(any())).thenReturn(testDiscoveryEntry);
        testProviderList.add(testPersistentProvider);
        when(context.getPackageName()).thenReturn(testPackageName);

        testPersistentJoynrContentProvider.init(context);

        Cursor matrixCursor = testPersistentJoynrContentProvider.query(Uri.parse("content://" + testPackageName + ".provider/init"), null, null, null);

        assertNotNull(matrixCursor);
        matrixCursor.moveToFirst();

        assertEquals(testDiscoveryEntry.getProviderVersion().getMajorVersion(), Integer.valueOf(matrixCursor.getInt(0)));
        assertEquals(testDiscoveryEntry.getProviderVersion().getMinorVersion(), Integer.valueOf(matrixCursor.getInt(1)));
        assertEquals(testDiscoveryEntry.getDomain(), matrixCursor.getString(2));
        assertEquals(testDiscoveryEntry.getInterfaceName(), matrixCursor.getString(3));
        assertEquals(testDiscoveryEntry.getParticipantId(), matrixCursor.getString(4));
        //assertEquals(testDiscoveryEntry.getQos().getCustomParameters(), matrixCursor.getString(5));
        assertEquals(testDiscoveryEntry.getQos().getPriority(), Long.valueOf(matrixCursor.getLong(6)));
        assertEquals(testDiscoveryEntry.getQos().getScope(), ProviderScope.valueOf(matrixCursor.getString(7)));
        assertEquals(testDiscoveryEntry.getQos().getSupportsOnChangeSubscriptions(), Boolean.valueOf(matrixCursor.getString(8)));
        assertEquals(testDiscoveryEntry.getLastSeenDateMs(), Long.valueOf(matrixCursor.getLong(9)));
        assertEquals(testDiscoveryEntry.getExpiryDateMs(), Long.valueOf(matrixCursor.getLong(10)));
        assertEquals(testDiscoveryEntry.getPublicKeyId(), matrixCursor.getString(11));

        persistentJoynrContentProviderMockedStatic.close();
    }

    @Test
    public void doQuery_whenQueryWrongPackageName_returnsNull() {

        persistentJoynrContentProviderMockedStatic = Mockito.mockStatic(PersistentJoynrContentProvider.class);

        persistentJoynrContentProviderMockedStatic.when(PersistentJoynrContentProvider::getDefaultExpiryTimeMs).thenReturn(testDefaultExpiryTimeMs);
        testProviderList.add(testPersistentProvider);
        when(context.getPackageName()).thenReturn(testPackageName);

        testPersistentJoynrContentProvider.init(context);

        Cursor matrixCursor = testPersistentJoynrContentProvider.query(Uri.parse("content://" + wrongTestPackageName + ".provider/init"), null, null, null);

        assertNull(matrixCursor);

        persistentJoynrContentProviderMockedStatic.close();
    }

    @Test
    public void doQuery_whenRegisterPersistentProviderReturnsNull_returnsNull() {

        persistentJoynrContentProviderMockedStatic = Mockito.mockStatic(PersistentJoynrContentProvider.class);

        persistentJoynrContentProviderMockedStatic.when(PersistentJoynrContentProvider::getDefaultExpiryTimeMs).thenReturn(testDefaultExpiryTimeMs);
        testProviderList.add(testPersistentProvider);
        when(context.getPackageName()).thenReturn(testPackageName);

        badTestPersistentJoynrContentProvider.init(context);

        Cursor matrixCursor = badTestPersistentJoynrContentProvider.query(Uri.parse("content://" + testPackageName + ".provider/init"), null, null, null);

        assertNull(matrixCursor);

        persistentJoynrContentProviderMockedStatic.close();
    }

    @Test
    public void getDefaultExpiryTimeMs_returnsConfiguredDefaultExpiryTimeMsFromProperties() {

        Properties testDefaultMessagingProperties = new Properties();
        testDefaultMessagingProperties.setProperty(PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS, String.valueOf(testDefaultExpiryTimeMs));

        MockedStatic<PropertyLoader> propertyLoaderMockedStatic = Mockito.mockStatic(PropertyLoader.class);
        propertyLoaderMockedStatic.when(() -> PropertyLoader.loadProperties(anyString())).thenReturn(testDefaultMessagingProperties);

        long getDefaultExpiryTimeMs = PersistentJoynrContentProvider.getDefaultExpiryTimeMs();

        assertEquals(testDefaultExpiryTimeMs, getDefaultExpiryTimeMs);

        propertyLoaderMockedStatic.close();
    }
}
