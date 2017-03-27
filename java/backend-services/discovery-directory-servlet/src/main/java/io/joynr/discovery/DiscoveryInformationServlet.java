package io.joynr.discovery;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashSet;
import java.util.Set;

import javax.inject.Singleton;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.inject.Inject;
import io.joynr.capabilities.DiscoveryEntryStore;
import io.joynr.capabilities.directory.Persisted;
import io.joynr.servlet.JoynrWebServlet;
import joynr.types.DiscoveryEntry;
import joynr.types.ProviderScope;

@Singleton
@JoynrWebServlet(value = "/capabilities/")
public class DiscoveryInformationServlet extends HttpServlet {
    private static final long serialVersionUID = 8839103126167589803L;
    private final transient DiscoveryEntryStore discoveryEntryStore;
    private final transient Gson gson = new GsonBuilder().create();

    @Inject
    public DiscoveryInformationServlet(@Persisted DiscoveryEntryStore discoveryEntryStore) {
        this.discoveryEntryStore = discoveryEntryStore;
    }

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
        response.setContentType("text/html");
        PrintWriter out;
        try {
            out = response.getWriter();
        } catch (Exception e) {
            log("error getting writer", e);
            return;
        }

        Set<DiscoveryEntry> globalDiscoveryEntries = new HashSet<>();
        Set<DiscoveryEntry> allDiscoveryEntries = discoveryEntryStore.getAllDiscoveryEntries();
        for (DiscoveryEntry discoveryEntry : allDiscoveryEntries) {
            if (discoveryEntry.getQos().getScope() == ProviderScope.GLOBAL) {
                try {
                    globalDiscoveryEntries.add(discoveryEntry);
                } catch (Exception e) {
                    log("error adding channel information", e);
                }
            }
        }
        out.println(gson.toJson(globalDiscoveryEntries));
    }

    @Override
    public void doDelete(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
        String queryString = request.getQueryString();
        String[] query = queryString.split("=");
        boolean removed = false;
        if (query.length > 1) {
            removed = discoveryEntryStore.remove(query[1]);
        }
        response.setStatus(200);
        try {
            PrintWriter out = response.getWriter();
            if (removed) {
                out.println("OK");
            } else {
                out.println("NOK");
            }
        } catch (Exception e) {
            log("error getting writer", e);
        }
    }

    @Override
    public void destroy() {
        // do nothing.
    }
}
