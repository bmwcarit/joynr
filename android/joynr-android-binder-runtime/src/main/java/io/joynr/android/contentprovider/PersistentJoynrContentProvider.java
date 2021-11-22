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
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;

import java.util.ArrayList;
import java.util.List;

import io.joynr.provider.AbstractJoynrProvider;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderQos;

import static io.joynr.android.contentprovider.PersistentProviderUtils.generatePrimitiveDiscoveryEntry;
import static io.joynr.android.contentprovider.PersistentProviderUtils.getDefaultExpiryTimeMs;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Class that users can inherit in order to abstract from the process of initial persistent
 * provider registration.
 */
public abstract class PersistentJoynrContentProvider extends ContentProvider {

    private static final Logger logger = LoggerFactory.getLogger(PersistentJoynrContentProvider.class);

    private static final int INIT_URL = 1;
    private static final String[] CURSOR_COLUMN_NAMES = {
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

    protected long defaultExpiryTimeMs = 0L;

    protected List<PersistentProvider> providers;

    protected UriMatcher uriMatcher;

    private String packageName;

    public PersistentJoynrContentProvider() {
        providers = new ArrayList<>();
    }

    @Override
    public boolean onCreate() {
        return init(getContext());
    }

    /**
     * Initializes variables for the Persistent Provider Content Provider functionality
     *
     * @param context Application Context
     * @return true if successful, false if something is wrong
     */
    protected boolean init(Context context) {
        packageName = context.getPackageName();
        String contentProviderName = packageName + ".provider";
        String queryString = "init";
        uriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
        uriMatcher.addURI(contentProviderName, queryString, INIT_URL);
        defaultExpiryTimeMs = getDefaultExpiryTimeMs();
        providers = registerPersistentProvider();

        logger.info("Initializing Content Provider on " + contentProviderName + "/" + queryString
                + " with expiryTime " + defaultExpiryTimeMs);

        boolean providersIsNull = providers != null;

        if (providersIsNull) {
            logger.info("Successfully initialized PersistentJoynrContentProvider on "
                    + packageName);
        } else {
            logger.error("Error initializing PersistentJoynrContentProvider on "
                    + packageName + ". Joynr Persistent Provider is null");
        }
        return providersIsNull;
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

    /**
     * Gets the DiscoveryEntry for the Persistent Provider
     *
     * @param uri           of the Persistent Provider Content Provider
     * @param projection    not relevant
     * @param selection     not relevant
     * @param selectionArgs not relevant
     * @param sortOrder     not relevant
     * @return MatrixCursor with DiscoveryEntry info
     */
    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
                        String[] selectionArgs, String sortOrder) {

        MatrixCursor mc = null;

        if (providers != null) {
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

                        logger.info("Returning DiscoveryEntry " + discoveryEntry
                                + "from PersistentJoynrContentProvider query on " + packageName);
                    }

                default:
                    logger.error("Error querying PersistentJoynrContentProvider on "
                            + packageName + ". Uri doesn't match.");
            }
        } else {
            logger.error("Error querying PersistentJoynrContentProvider on "
                    + packageName + ". Joynr Persistent Provider is null");
        }
        return mc;
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

    public static class PersistentProvider {
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