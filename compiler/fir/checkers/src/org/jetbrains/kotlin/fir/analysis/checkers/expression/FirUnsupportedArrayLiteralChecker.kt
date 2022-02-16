/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.fir.analysis.checkers.expression

import org.jetbrains.kotlin.config.LanguageFeature
import org.jetbrains.kotlin.descriptors.ClassKind
import org.jetbrains.kotlin.diagnostics.DiagnosticReporter
import org.jetbrains.kotlin.diagnostics.reportOn
import org.jetbrains.kotlin.fir.analysis.checkers.context.CheckerContext
import org.jetbrains.kotlin.fir.analysis.checkers.toRegularClassSymbol
import org.jetbrains.kotlin.fir.analysis.diagnostics.FirErrors
import org.jetbrains.kotlin.fir.declarations.FirRegularClass
import org.jetbrains.kotlin.fir.declarations.FirValueParameter
import org.jetbrains.kotlin.fir.declarations.impl.FirPrimaryConstructor
import org.jetbrains.kotlin.fir.declarations.utils.isCompanion
import org.jetbrains.kotlin.fir.expressions.*

object FirUnsupportedArrayLiteralChecker : FirArrayOfCallChecker() {
    override fun check(expression: FirArrayOfCall, context: CheckerContext, reporter: DiagnosticReporter) {
        when (getContainerKind(expression, context)) {
            ContainerKind.AnnotationOrAnnotationClass -> {}
            ContainerKind.CompanionOfAnnotation -> {
                val isError = context.languageVersionSettings.supportsFeature(LanguageFeature.ProhibitArrayLiteralsInCompanionOfAnnotation)
                reportUnsupportedLiteral(expression, isError, context, reporter)
            }
            ContainerKind.Other -> reportUnsupportedLiteral(expression, true, context, reporter)
        }
    }

    private fun getContainerKind(expression: FirArrayOfCall, context: CheckerContext): ContainerKind {
        context.qualifiedAccessOrAnnotationCalls.lastOrNull()?.let {
            val arguments = if (it is FirFunctionCall) {
                if (it.typeRef.toRegularClassSymbol(context.session)?.classKind == ClassKind.ANNOTATION_CLASS ||
                    it.origin == FirFunctionCallOrigin.Operator && it.calleeReference.name.asString() == "plus"
                ) {
                    it.arguments
                } else {
                    null
                }
            } else if (it is FirAnnotationCall) {
                it.arguments
            } else {
                null
            }

            return if (arguments?.any { argument ->
                    val unwrapped =
                        (if (argument is FirVarargArgumentsExpression) {
                            argument.arguments[0]
                        } else {
                            argument
                        }).unwrapArgument()
                    if (unwrapped == expression) {
                        true
                    } else {
                        if (unwrapped is FirArrayOfCall) {
                            unwrapped.arguments.firstOrNull()?.unwrapArgument() == expression
                        } else {
                            false
                        }
                    }
                } == true
            ) {
                ContainerKind.AnnotationOrAnnotationClass
            } else {
                ContainerKind.Other
            }
        }

        var isCompanion = false
        for (declaration in context.containingDeclarations.asReversed()) {
            if (declaration is FirRegularClass) {
                if (declaration.isCompanion) {
                    isCompanion = true
                    continue
                }

                if (declaration.classKind == ClassKind.ANNOTATION_CLASS) {
                    return if (isCompanion) ContainerKind.CompanionOfAnnotation else ContainerKind.AnnotationOrAnnotationClass
                }
            } else if (declaration is FirValueParameter || declaration is FirPrimaryConstructor) {
                continue
            }

            break
        }

        return ContainerKind.Other
    }

    private fun reportUnsupportedLiteral(
        expression: FirArrayOfCall,
        isError: Boolean,
        context: CheckerContext,
        reporter: DiagnosticReporter
    ) {
        reporter.reportOn(
            expression.source,
            if (isError) FirErrors.UNSUPPORTED else FirErrors.UNSUPPORTED_WARNING,
            "Collection literals outside of annotations",
            context
        )
    }

    private enum class ContainerKind {
        AnnotationOrAnnotationClass,
        CompanionOfAnnotation,
        Other
    }
}