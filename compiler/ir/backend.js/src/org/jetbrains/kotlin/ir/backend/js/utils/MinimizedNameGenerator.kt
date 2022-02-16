/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.ir.backend.js.utils

class MinimizedNameGenerator(val enabled: Boolean) {
    private var index = 0
    private val functionSignatureToName = mutableMapOf<String, String>()

    fun generateNextName(): String {
        return index++.toJsIdentifier()
    }

    fun nameBySignature(signature: String): String {
        return functionSignatureToName.getOrPut(signature) {
            generateNextName()
        }
    }
}