/*
 * Copyright 2010-2021 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.fir.expressions.builder

import kotlin.contracts.*
import org.jetbrains.kotlin.KtSourceElement
import org.jetbrains.kotlin.fir.builder.FirAnnotationContainerBuilder
import org.jetbrains.kotlin.fir.builder.FirBuilderDsl
import org.jetbrains.kotlin.fir.expressions.FirAnnotation
import org.jetbrains.kotlin.fir.expressions.FirExpression
import org.jetbrains.kotlin.fir.expressions.FirSyntheticsAccessorExpression
import org.jetbrains.kotlin.fir.expressions.builder.FirExpressionBuilder
import org.jetbrains.kotlin.fir.expressions.impl.FirSyntheticsAccessorExpressionImpl
import org.jetbrains.kotlin.fir.types.FirTypeRef
import org.jetbrains.kotlin.fir.visitors.*

/*
 * This file was generated automatically
 * DO NOT MODIFY IT MANUALLY
 */

@FirBuilderDsl
class FirSyntheticsAccessorExpressionBuilder : FirAnnotationContainerBuilder, FirExpressionBuilder {
    lateinit var delegate: FirExpression

    override fun build(): FirSyntheticsAccessorExpression {
        return FirSyntheticsAccessorExpressionImpl(
            delegate,
        )
    }

    @Deprecated("Modification of 'source' has no impact for FirSyntheticsAccessorExpressionBuilder", level = DeprecationLevel.HIDDEN)
    override var source: KtSourceElement?
        get() = throw IllegalStateException()
        set(_) {
            throw IllegalStateException()
        }

    @Deprecated("Modification of 'typeRef' has no impact for FirSyntheticsAccessorExpressionBuilder", level = DeprecationLevel.HIDDEN)
    override var typeRef: FirTypeRef
        get() = throw IllegalStateException()
        set(_) {
            throw IllegalStateException()
        }

    @Deprecated("Modification of 'annotations' has no impact for FirSyntheticsAccessorExpressionBuilder", level = DeprecationLevel.HIDDEN)
    override val annotations: MutableList<FirAnnotation> = mutableListOf()
}

@OptIn(ExperimentalContracts::class)
inline fun buildSyntheticsAccessorExpression(init: FirSyntheticsAccessorExpressionBuilder.() -> Unit): FirSyntheticsAccessorExpression {
    contract {
        callsInPlace(init, kotlin.contracts.InvocationKind.EXACTLY_ONCE)
    }
    return FirSyntheticsAccessorExpressionBuilder().apply(init).build()
}
