// ISSUE: KT-27084, KT-22379

fun takeInt(x: Int) {}

fun test_1() {
    var x: String? = null
    while (x!!.length > 42) {
        x = null
        break
    }
    takeInt(x<!UNSAFE_CALL!>.<!>length) // should be unsafe call
}

fun test_2() {
    var result: String? = null
    var i = 0
    while (result == null) {
        if (i == 10) result = "non null"
        else i++
    }
    takeInt(result.length) // should be smartcast
}
