package io.joynr.database;

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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.sql.SQLException;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.google.inject.Guice;

/**
 * Tests the interaction of the dispatcher and communication manager.
 */
public class JoynrEmbeddedDatabaseTest {

    private static final String testTable = "testtable";
    private static final String testColumn1 = "testColumn1";
    private static final String testColumn2 = "testColumn2";
    private static final String CREATE_TABLE_STATEMENT = "create table " + testTable + "(" + testColumn1
            + " varchar(20), " + testColumn2 + " varchar(80))";
    private static final String DELETE_TABLE_STATEMENT = "drop table " + testTable;
    private JoynrEmbeddedDatabase fixture;

    @Before
    public void setUp() throws SQLException {
        fixture = Guice.createInjector(new JoynrEmbeddedDatabaseTestModule()).getInstance(JoynrEmbeddedDatabase.class);
        fixture.start();
    }

    @After
    public void tearDown() throws SQLException {
        fixture.close();
    }

    @Test
    public void testTableNotExists() throws Exception {
        assertFalse(fixture.isTableAlreadyContainedInDatabase(testTable));
    }

    @Test
    public void testReconnectToDatabase() throws Exception {
        fixture.close();
        fixture.start();
    }

    @Test
    public void testTableExists() throws Exception {
        createTable();
        assertTrue(fixture.isTableAlreadyContainedInDatabase(testTable));
        deleteTable();
    }

    private void deleteTable() throws SQLException {
        fixture.execute(DELETE_TABLE_STATEMENT);
    }

    private void createTable() throws SQLException {
        fixture.execute(CREATE_TABLE_STATEMENT);
    }
}
