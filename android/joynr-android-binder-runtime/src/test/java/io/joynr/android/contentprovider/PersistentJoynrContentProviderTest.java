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
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockedStatic;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;
import io.joynr.provider.AbstractJoynrProvider;
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

    private MockedStatic<PersistentProviderUtils> persistentProviderUtilsMockedStatic;

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

        persistentProviderUtilsMockedStatic = Mockito.mockStatic(PersistentProviderUtils.class);

    }

    @Test
    public void init_whenRegisterPersistentProviderIsConfigured_variablesAreInitAndInitIsTrue() {

        persistentProviderUtilsMockedStatic.when(PersistentProviderUtils::getDefaultExpiryTimeMs).thenReturn(testDefaultExpiryTimeMs);
        testProviderList.add(testPersistentProvider);
        when(context.getPackageName()).thenReturn(testPackageName);

        assertTrue(testPersistentJoynrContentProvider.init(context));

        assertNotNull(testPersistentJoynrContentProvider.uriMatcher);
        assertEquals(testProviderList, testPersistentJoynrContentProvider.providers);
        assertEquals(testPersistentJoynrContentProvider.defaultExpiryTimeMs, testDefaultExpiryTimeMs);

    }

    @Test
    public void init_whenBadRegisterPersistentProvider_providersListIsNullAndInitIsFalse() {

        persistentProviderUtilsMockedStatic.when(PersistentProviderUtils::getDefaultExpiryTimeMs).thenReturn(testDefaultExpiryTimeMs);
        testProviderList.add(testPersistentProvider);
        when(context.getPackageName()).thenReturn(testPackageName);

        assertFalse(badTestPersistentJoynrContentProvider.init(context));

        assertNotNull(badTestPersistentJoynrContentProvider.uriMatcher);
        assertNull(badTestPersistentJoynrContentProvider.providers);
        assertEquals(badTestPersistentJoynrContentProvider.defaultExpiryTimeMs, testDefaultExpiryTimeMs);

    }


    @Test
    public void doQuery_whenRegisterPersistentProviderConfigured_returnsDiscoveryEntry() {

        persistentProviderUtilsMockedStatic.when(PersistentProviderUtils::getDefaultExpiryTimeMs).thenReturn(testDefaultExpiryTimeMs);
        persistentProviderUtilsMockedStatic.when(() -> PersistentProviderUtils.generatePrimitiveDiscoveryEntry(any(), eq(testDefaultExpiryTimeMs))).thenReturn(testDiscoveryEntry);
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

    }

    @Test
    public void doQuery_whenQueryWrongPackageName_returnsNull() {

        persistentProviderUtilsMockedStatic.when(PersistentProviderUtils::getDefaultExpiryTimeMs).thenReturn(testDefaultExpiryTimeMs);
        testProviderList.add(testPersistentProvider);
        when(context.getPackageName()).thenReturn(testPackageName);

        testPersistentJoynrContentProvider.init(context);

        Cursor matrixCursor = testPersistentJoynrContentProvider.query(Uri.parse("content://" + wrongTestPackageName + ".provider/init"), null, null, null);

        assertNull(matrixCursor);

    }

    @Test
    public void doQuery_whenRegisterPersistentProviderReturnsNull_returnsNull() {

        persistentProviderUtilsMockedStatic.when(PersistentProviderUtils::getDefaultExpiryTimeMs).thenReturn(testDefaultExpiryTimeMs);
        testProviderList.add(testPersistentProvider);
        when(context.getPackageName()).thenReturn(testPackageName);

        badTestPersistentJoynrContentProvider.init(context);

        Cursor matrixCursor = badTestPersistentJoynrContentProvider.query(Uri.parse("content://" + testPackageName + ".provider/init"), null, null, null);

        assertNull(matrixCursor);

    }

    @After
    public void cleanup() {
        persistentProviderUtilsMockedStatic.close();
        reset(context);
        reset(joynrProvider);
    }
}
