/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.fir.resolve.dfa

import org.jetbrains.kotlin.fir.contracts.description.*
import org.jetbrains.kotlin.fir.expressions.LogicOperationKind
import org.jetbrains.kotlin.fir.resolve.substitution.ConeSubstitutor
import org.jetbrains.kotlin.fir.types.*

fun ConeConstantReference.toOperation(): Operation? = when (this) {
    ConeConstantReference.WILDCARD -> null
    ConeConstantReference.NULL -> Operation.EqNull
    ConeConstantReference.NOT_NULL -> Operation.NotEqNull
    ConeBooleanConstantReference.TRUE -> Operation.EqTrue
    ConeBooleanConstantReference.FALSE -> Operation.EqFalse
    else -> throw IllegalArgumentException("$this can not be transformed to Operation")
}

// Returns `null` if the statement is always false.
fun <F : Flow> LogicSystem<F>.approveContractStatement(
    flow: F,
    statement: ConeBooleanExpression,
    arguments: Array<out DataFlowVariable?>, // 0 = receiver (null if doesn't exist)
    substitutor: ConeSubstitutor?,
    removeApprovedOrImpossible: Boolean = false,
): TypeStatements? {
    fun OperationStatement.approve() =
        approveOperationStatement(flow, this, removeApprovedOrImpossible)

    fun DataFlowVariable.processEqNull(isEq: Boolean): TypeStatements =
        OperationStatement(this, if (isEq) Operation.EqNull else Operation.NotEqNull).approve()

    fun ConeBooleanExpression.visit(inverted: Boolean): TypeStatements? = when (this) {
        is ConeBooleanConstantReference ->
            if (inverted == (this == ConeBooleanConstantReference.TRUE)) null else mapOf()
        is ConeLogicalNot -> arg.visit(inverted = !inverted)
        is ConeIsInstancePredicate ->
            arguments.getOrNull(arg.parameterIndex + 1)?.let {
                val isType = inverted == isNegated
                val substitutedType = substitutor?.substituteOrNull(type) ?: type
                when {
                    substitutedType.isAny -> it.processEqNull(!isType)
                    substitutedType.isNullableNothing -> it.processEqNull(isType)
                    else -> {
                        // x is (T & Any) => x != null
                        // TODO? (KT-22996) x !is T? => x != null: change `&&` to `==`
                        val fromNullability = if (isType && !type.canBeNull) it.processEqNull(false) else mapOf()
                        if (isType && it is RealVariable) {
                            andForTypeStatements(fromNullability, mapOf(it to (it typeEq substitutedType)))
                        } else {
                            fromNullability
                        }
                    }
                }
            } ?: mapOf()
        is ConeIsNullPredicate ->
            arguments.getOrNull(arg.parameterIndex + 1)?.processEqNull(inverted == isNegated) ?: mapOf()
        is ConeBooleanValueParameterReference ->
            arguments.getOrNull(parameterIndex + 1)?.let {
                OperationStatement(it, if (inverted) Operation.EqFalse else Operation.EqTrue).approve()
            } ?: mapOf()
        is ConeBinaryLogicExpression -> {
            val a = left.visit(inverted)
            val b = right.visit(inverted)
            val isAnd = inverted != (kind == LogicOperationKind.AND)
            when {
                a == null -> b.takeIf { !isAnd } // false || b == b; false && b = false
                b == null -> a.takeIf { !isAnd } // a || false == a; a && false = false
                isAnd -> andForTypeStatements(a, b)
                else -> orForTypeStatements(a, b)
            }
        }
        else -> mapOf()
    }

    return statement.visit(inverted = false)
}
