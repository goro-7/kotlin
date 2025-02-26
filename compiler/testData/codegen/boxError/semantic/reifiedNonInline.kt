// IGNORE_ERRORS
// ERROR_POLICY: SEMANTIC
// IGNORE_BACKEND_K2: JS_IR

// MODULE: lib
// FILE: t.kt

fun <reified T> bar(t: T) = t

fun <reified T> qux() = T::class

fun foo(): String {
    return bar<String>("OK")
}

fun dec() { qux() }

// MODULE: main(lib)
// FILE: b.kt

fun box(): String {
    try {
        dec()
    } catch (e: Throwable /*js ReferenceError*/) {
        return foo()
    }
}