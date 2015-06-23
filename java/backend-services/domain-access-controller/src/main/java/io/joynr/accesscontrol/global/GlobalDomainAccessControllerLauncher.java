package io.joynr.accesscontrol.global;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;

import java.util.Properties;

import joynr.infrastructure.GlobalDomainAccessControllerAbstractProvider;

import com.google.inject.Inject;

public class GlobalDomainAccessControllerLauncher extends AbstractJoynrApplication {

    private static final String AUTH_TOKEN = "GlobalDomainAccessControllerLauncher_authToken";

    @Inject
    private GlobalDomainAccessControllerAbstractProvider globalDomainAccessSyncProvider;

    public static void main(String[] args) {
        GlobalDomainAccessControllerLauncher.start();
    }

    public static GlobalDomainAccessControllerLauncher start() {
        return start(new Properties());
    }

    public static GlobalDomainAccessControllerLauncher start(Properties joynrConfig) {

        JoynrInjectorFactory injectorFactory = new JoynrInjectorFactory(joynrConfig,
                                                                        new GlobalDomainAccessControllerModule());
        JoynrApplication domainAccessControllerLauncherApp = injectorFactory.createApplication(new JoynrApplicationModule(AUTH_TOKEN,
                                                                                                                          GlobalDomainAccessControllerLauncher.class));
        domainAccessControllerLauncherApp.run();

        return (GlobalDomainAccessControllerLauncher) domainAccessControllerLauncherApp;
    }

    @Override
    public void run() {

        runtime.registerCapability(localDomain, globalDomainAccessSyncProvider, AUTH_TOKEN);
    }

    @Override
    public void shutdown() {
        runtime.shutdown(false);
    }
}
