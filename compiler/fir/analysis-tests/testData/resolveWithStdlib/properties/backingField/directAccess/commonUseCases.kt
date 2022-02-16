class A {
    var number: String
        field = 10
        get() = field.toString()
        set(newValue) {
            field = newValue.length
        }

    fun updateNumber() {
        number#field += 100
    }

    fun represent(): String {
        return "field = " + number#field
    }
}

fun process(a: A): String {
    return StringBuilder().append {
        append("a = $a, length = " + a.number.length)
        a.updateNumber()
        append("a = $a, length = " + a.number.length)
    }.build()
}
