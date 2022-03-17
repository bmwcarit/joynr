package io.joynr.androidhelloworldbinderconsumer

import android.app.Application
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import io.joynr.arbitration.DiscoveryQos
import io.joynr.arbitration.DiscoveryScope
import io.joynr.exceptions.JoynrRuntimeException
import io.joynr.messaging.MessagingQos
import io.joynr.proxy.Callback
import joynr.helloworld.HelloWorldProxy
import org.slf4j.LoggerFactory

class ConsumerViewModel : ViewModel() {

    private val logger = LoggerFactory.getLogger(ConsumerViewModel::class.java)

    private lateinit var helloWorldProxy: HelloWorldProxy
    val providedStr = MutableLiveData<String>()

    fun registerProxy(application: Application) {
        val discoveryQos = DiscoveryQos()
        discoveryQos.discoveryScope = DiscoveryScope.LOCAL_ONLY

        helloWorldProxy = (application as HelloWorldApplication).joynrRuntime
            .getProxyBuilder("domain", HelloWorldProxy::class.java)
            .setMessagingQos(MessagingQos()).setDiscoveryQos(discoveryQos).build()
    }

    fun getString() {
        val callback = object : Callback<String>() {
            override fun onSuccess(text: String?) {
                logger.debug("Received Message: $text")
                providedStr.postValue(text)
            }

            override fun onFailure(e: JoynrRuntimeException) {
                logger.debug("Failed Message Failed to receive message with the error: $e")
            }
        }

        helloWorldProxy.getHello(callback)
    }

}