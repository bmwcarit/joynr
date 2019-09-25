package io.joynr.android.messaging.binder;


interface JoynrBinder {

    void transmit(in byte[] message);
}
