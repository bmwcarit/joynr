package io.joynr.androidhelloworldbinderprovider

import io.joynr.provider.Deferred
import io.joynr.provider.DeferredVoid
import io.joynr.provider.Promise
import joynr.helloworld.HelloWorldAbstractProvider

class HelloWorldProvider : HelloWorldAbstractProvider() {

    private var helloWorld: String? = "Hello World!"

    override fun getHello(): Promise<Deferred<String>> {
        val deferredString = Deferred<String>()
        deferredString.resolve(helloWorld)
        return Promise(deferredString)
    }

    override fun setHello(hello: String?): Promise<DeferredVoid> {
        val deferredVoid = DeferredVoid()
        this.helloWorld = hello
        deferredVoid.resolve()
        return Promise(deferredVoid)
    }

}