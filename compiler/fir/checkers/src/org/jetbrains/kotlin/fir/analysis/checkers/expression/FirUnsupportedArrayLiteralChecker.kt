/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.fir.analysis.checkers.expression

import org.jetbrains.kotlin.descriptors.ClassKind
import org.jetbrains.kotlin.diagnostics.DiagnosticReporter
import org.jetbrains.kotlin.diagnostics.reportOn
import org.jetbrains.kotlin.fir.analysis.checkers.context.CheckerContext
import org.jetbrains.kotlin.fir.analysis.diagnostics.FirErrors
import org.jetbrains.kotlin.fir.declarations.FirConstructor
import org.jetbrains.kotlin.fir.declarations.FirRegularClass
import org.jetbrains.kotlin.fir.expressions.FirArrayOfCall

object FirUnsupportedArrayLiteralChecker : FirArrayOfCallChecker() {
    override fun check(expression: FirArrayOfCall, context: CheckerContext, reporter: DiagnosticReporter) {
        context.containingDeclarations.asReversed().forEach {
            if (it is FirConstructor) {
                return
            }

            if (it is FirRegularClass && it.classKind == ClassKind.ANNOTATION_CLASS) {
                reporter.reportOn(
                    expression.source,
                    FirErrors.UNSUPPORTED,
                    "Collection literals are not allowed inside annotation classes",
                    context
                )
                return
            }
        }
    }
}