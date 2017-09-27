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
package io.joynr.integration;

import static com.jayway.restassured.RestAssured.given;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import net.sf.ehcache.CacheManager;
import net.sf.ehcache.distribution.CacheManagerPeerProvider;

import org.junit.Assert;

import io.joynr.integration.util.ServersUtil;
import io.joynr.messaging.MessagingPropertyKeys;

import org.eclipse.jetty.server.Server;
import org.hamcrest.BaseMatcher;
import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import com.jayway.restassured.RestAssured;
import com.jayway.restassured.path.json.JsonPath;
import com.jayway.restassured.response.Response;

public class ClusteredBounceProxyControllerTest {

    private static Server bounceProxyControllerServer1;
    private static Server bounceProxyControllerServer2;

    private static String bpcUrl1;
    private static String bpcUrl2;

    @BeforeClass
    public static void startServer() throws Exception {

        // start different servers to make sure that handling of different URLs
        // works
        System.setProperty("joynr.bounceproxy.controller.bp_cache_name", "bpCache1");
        System.setProperty("joynr.bounceproxy.controller.bp_cache_config_file", "ehcache_distributed1.xml");
        bounceProxyControllerServer1 = ServersUtil.startClusteredBounceproxyController();
        bpcUrl1 = System.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL);

        System.setProperty("joynr.bounceproxy.controller.bp_cache_name", "bpCache1");
        System.setProperty("joynr.bounceproxy.controller.bp_cache_config_file", "ehcache_distributed2.xml");
        bounceProxyControllerServer2 = ServersUtil.startClusteredBounceproxyController();
        bpcUrl2 = System.getProperty(MessagingPropertyKeys.BOUNCE_PROXY_URL);
    }

    @AfterClass
    public static void stopServer() throws Exception {
        bounceProxyControllerServer1.stop();
        bounceProxyControllerServer2.stop();
    }

    @Before
    public void setup() throws InterruptedException {
        waitForCachesJoiningCluster();
    }

    private void waitForCachesJoiningCluster() throws InterruptedException {
        CacheManager cacheManager = CacheManager.newInstance(getClass().getResource("/ehcache_distributed_test.xml"));

        CacheManagerPeerProvider peerProvider = cacheManager.getCacheManagerPeerProvider("RMI");

        int peers = 0;
        long timeoutMs = 10000;
        long startMs = System.currentTimeMillis();
        do {
            Thread.sleep(1000);

            peers = peerProvider.listRemoteCachePeers(cacheManager.getEhcache("bpCache1")).size();

        } while (peers < 2 && (System.currentTimeMillis() - startMs < timeoutMs));

        Assert.assertEquals(2, peers);

        cacheManager.shutdown();
    }

    @Test
    @Ignore("Ignore until servers are started in a separate JVM. Guice static problem")
    public void testBounceProxyRegistrationAtBpc1() throws Exception {

        RestAssured.baseURI = bpcUrl1;
        Response responseRegister = given().log()
                                           .all()
                                           .queryParam("url4cc", "http://testurl/url4cc")
                                           .and()
                                           .queryParam("url4bpc", "http://testurl/url4bpc")
                                           .when()
                                           .put("bounceproxies/?bpid=0.0");

        assertEquals(201 /* Created */, responseRegister.getStatusCode());

        waitForReplication();

        RestAssured.baseURI = bpcUrl2;

        JsonPath listBps = given().log().all().get("bounceproxies").body().jsonPath();
        assertThat(listBps, containsBounceProxy("0.0", "ALIVE"));
    }

    @Test
    @Ignore("Ignore until servers are started in a separate JVM. Guice static problem")
    public void testBounceProxyRegistrationAtBpc2() throws InterruptedException {

        RestAssured.baseURI = bpcUrl2;
        Response responseRegister = given().log()
                                           .all()
                                           .queryParam("url4cc", "http://testurl/url4cc")
                                           .and()
                                           .queryParam("url4bpc", "http://testurl/url4bpc")
                                           .when()
                                           .put("bounceproxies/?bpid=1.0");

        assertEquals(201 /* Created */, responseRegister.getStatusCode());

        waitForReplication();

        RestAssured.baseURI = bpcUrl1;
        JsonPath listBps = given().log().all().get("bounceproxies").body().jsonPath();

        assertThat(listBps, containsBounceProxy("1.0", "ALIVE"));
    }

    private void waitForReplication() {
        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            Assert.fail(e.getMessage());
        }
    }

    private Matcher<JsonPath> containsBounceProxy(final String id, final String status) {

        return new BaseMatcher<JsonPath>() {

            @Override
            public boolean matches(Object item) {

                JsonPath jsonPath = (JsonPath) item;

                for (int i = 0; i < jsonPath.getList("").size(); i++) {

                    if (jsonPath.get("[" + i + "].status").equals(status)
                            && jsonPath.get("[" + i + "].bounceProxyId").equals(id)) {
                        return true;
                    }
                }

                return false;
            }

            @Override
            public void describeTo(Description description) {
                description.appendText("contains entry with status=" + status + " and bounceProxyId=" + id);
            }

            @Override
            public void describeMismatch(final Object item, final Description description) {
                description.appendText("was").appendValue(((JsonPath) item).get(""));
            }
        };
    }
}
