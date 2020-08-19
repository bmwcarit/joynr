/*-
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
package io.joynr.android.contentprovider;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;

import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

import io.joynr.android.AndroidBinderRuntime;
import io.joynr.capabilities.PropertiesFileParticipantIdStorage;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.provider.ProviderContainer;
import io.joynr.provider.ProviderContainerFactory;
import io.joynr.runtime.PropertyLoader;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;

import static io.joynr.messaging.ConfigurableMessagingSettings.PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS;
import static io.joynr.messaging.MessagingPropertyKeys.DEFAULT_MESSAGING_PROPERTIES_FILE;
import static io.joynr.util.VersionUtil.getVersionFromAnnotation;

/**
 * Class that users can inherit in order to abstract from the process of initial persistent
 * provider registration.
 */
public abstract class PersistentJoynrContentProvider extends ContentProvider {

    private final int INIT_URL = 1;
    private final String[] CURSOR_COLUMN_NAMES = {
            "providerVersionMajor",
            "providerVersionMinor",
            "domain",
            "interfaceName",
            "participantId",
            "qosCustomParameters",
            "qosPriority",
            "qosScope",
            "qosSupportsOnChangeSubscriptions",
            "lastSeenDateMs",
            "expiryDateMs",
            "publicKeyId"
    };

    private long defaultExpiryTimeMs = 0L;

    private List<PersistentProvider> providers;

    static UriMatcher uriMatcher;

    public PersistentJoynrContentProvider() {
        providers = new ArrayList<>();
    }

    @Override
    public boolean onCreate() {
        String contentProviderName = getContext().getPackageName() + ".provider";
        String queryString = "init";
        uriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
        uriMatcher.addURI(contentProviderName, queryString, INIT_URL);
        providers = registerPersistentProvider();
        defaultExpiryTimeMs = getDefaultExpiryTimeMs();

        return true;
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        return 0;
    }

    @Override
    public String getType(Uri uri) {
        return null;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        return null;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
                        String[] selectionArgs, String sortOrder) {

        MatrixCursor mc = null;
        switch (uriMatcher.match(uri)) {
            case INIT_URL:
                mc = new MatrixCursor(CURSOR_COLUMN_NAMES);
                for (PersistentProvider persistentProvider : providers) {
                    DiscoveryEntry discoveryEntry = generatePrimitiveDiscoveryEntry(persistentProvider);
                    mc.newRow()
                            .add(CURSOR_COLUMN_NAMES[0], discoveryEntry.getProviderVersion().getMajorVersion())
                            .add(CURSOR_COLUMN_NAMES[1], discoveryEntry.getProviderVersion().getMinorVersion())
                            .add(CURSOR_COLUMN_NAMES[2], discoveryEntry.getDomain())
                            .add(CURSOR_COLUMN_NAMES[3], discoveryEntry.getInterfaceName())
                            .add(CURSOR_COLUMN_NAMES[4], discoveryEntry.getParticipantId())
                            .add(CURSOR_COLUMN_NAMES[5], discoveryEntry.getQos().getCustomParameters())
                            .add(CURSOR_COLUMN_NAMES[6], discoveryEntry.getQos().getPriority())
                            .add(CURSOR_COLUMN_NAMES[7], discoveryEntry.getQos().getScope())
                            .add(CURSOR_COLUMN_NAMES[8], discoveryEntry.getQos().getSupportsOnChangeSubscriptions())
                            .add(CURSOR_COLUMN_NAMES[9], discoveryEntry.getLastSeenDateMs())
                            .add(CURSOR_COLUMN_NAMES[10], discoveryEntry.getExpiryDateMs())
                            .add(CURSOR_COLUMN_NAMES[11], discoveryEntry.getPublicKeyId());
                }
            default:
                // no-op
        }
        return mc;
    }

    private DiscoveryEntry generatePrimitiveDiscoveryEntry(
            PersistentProvider provider
    ) {

        ProviderContainerFactory providerContainerFactory = AndroidBinderRuntime.getInjector().getInstance(ProviderContainerFactory.class);
        ProviderContainer providerContainer = providerContainerFactory.create(provider.joynrProvider);
        PropertiesFileParticipantIdStorage participantIdStorage =
                AndroidBinderRuntime
                        .getInjector()
                        .getInstance(PropertiesFileParticipantIdStorage.class);

        String participantId = participantIdStorage.getProviderParticipantId(
                provider.domain,
                providerContainer.getInterfaceName(),
                providerContainer.getMajorVersion());

        String defaultPublicKeyId = "";
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(
                getVersionFromAnnotation(provider.joynrProvider.getClass()),
                provider.domain,
                providerContainer.getInterfaceName(),
                participantId,
                provider.providerQos,
                System.currentTimeMillis(),
                System.currentTimeMillis() + defaultExpiryTimeMs,
                defaultPublicKeyId);

        return discoveryEntry;
    }

    private long getDefaultExpiryTimeMs() {
        Properties defaultMessagingProperties = PropertyLoader.loadProperties(DEFAULT_MESSAGING_PROPERTIES_FILE);
        return Long.parseLong(defaultMessagingProperties.getProperty(PROPERTY_DISCOVERY_PROVIDER_DEFAULT_EXPIRY_TIME_MS));
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        return 0;
    }

    /**
     * Method to create and register a list of providers that are meant to be subjected to special
     * conditions as soon as the component is started, since the Android ContentProvider is called
     * before anything else. These conditions, e.g., to control provider behavior within a system,
     * must be implemented by the interested party/system component.
     *
     * @return List of joynr persistent providers that are going to be registered
     */
    public abstract List<PersistentProvider> registerPersistentProvider();

    public class PersistentProvider {
        private AbstractJoynrProvider joynrProvider;
        private String domain;
        private ProviderQos providerQos;

        public PersistentProvider(AbstractJoynrProvider joynrProvider, String domain, ProviderQos providerQos) {
            this.joynrProvider = joynrProvider;
            this.domain = domain;
            this.providerQos = providerQos;
        }

        public AbstractJoynrProvider getJoynrProvider() {
            return joynrProvider;
        }

        public void setJoynrProvider(AbstractJoynrProvider joynrProvider) {
            this.joynrProvider = joynrProvider;
        }

        public String getDomain() {
            return domain;
        }

        public void setDomain(String domain) {
            this.domain = domain;
        }

        public ProviderQos getProviderQos() {
            return providerQos;
        }

        public void setProviderQos(ProviderQos providerQos) {
            this.providerQos = providerQos;
        }
    }
}