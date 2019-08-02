package io.joynr.android.consumer;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import android.content.Context;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class JoynrConsumerInstrumentedTest {

    @Test
    public void testLibJoynr() {

        Context appContext = ApplicationProvider.getApplicationContext();

        RadioConsumerApp radioConsumerApp = new RadioConsumerApp();
        radioConsumerApp.init(appContext);
        String station = radioConsumerApp.getCurrentStation();

        assertEquals(station, "JoynrStation");

    }
}
