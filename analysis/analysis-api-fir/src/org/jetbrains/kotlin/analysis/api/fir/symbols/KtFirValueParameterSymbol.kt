/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.analysis.api.fir.symbols

import com.intellij.psi.PsiElement
import org.jetbrains.kotlin.KtFakeSourceElementKind
import org.jetbrains.kotlin.analysis.api.analyze
import org.jetbrains.kotlin.analysis.api.fir.KtSymbolByFirBuilder
import org.jetbrains.kotlin.analysis.api.fir.annotations.KtFirAnnotationListForDeclaration
import org.jetbrains.kotlin.analysis.api.fir.findPsi
import org.jetbrains.kotlin.analysis.api.fir.symbols.pointers.KtFirValueParameterSymbolPointer
import org.jetbrains.kotlin.analysis.api.fir.utils.cached
import org.jetbrains.kotlin.analysis.api.fir.utils.firSymbol
import org.jetbrains.kotlin.analysis.api.lifetime.KtLifetimeToken
import org.jetbrains.kotlin.analysis.api.lifetime.withValidityAssertion
import org.jetbrains.kotlin.analysis.api.symbols.KtFunctionLikeSymbol
import org.jetbrains.kotlin.analysis.api.symbols.KtKotlinPropertySymbol
import org.jetbrains.kotlin.analysis.api.symbols.KtValueParameterSymbol
import org.jetbrains.kotlin.analysis.api.symbols.pointers.KtPsiBasedSymbolPointer
import org.jetbrains.kotlin.analysis.api.symbols.pointers.KtSymbolPointer
import org.jetbrains.kotlin.analysis.low.level.api.fir.api.LLFirResolveSession
import org.jetbrains.kotlin.fir.correspondingProperty
import org.jetbrains.kotlin.fir.declarations.FirFunction
import org.jetbrains.kotlin.fir.renderWithType
import org.jetbrains.kotlin.fir.symbols.impl.FirValueParameterSymbol
import org.jetbrains.kotlin.fir.types.arrayElementType
import org.jetbrains.kotlin.name.Name

internal class KtFirValueParameterSymbol(
    override val firSymbol: FirValueParameterSymbol,
    override val firResolveSession: LLFirResolveSession,
    override val token: KtLifetimeToken,
    private val builder: KtSymbolByFirBuilder
) : KtValueParameterSymbol(), KtFirSymbol<FirValueParameterSymbol> {
    override val psi: PsiElement? by cached { firSymbol.findPsi() }

    override val name: Name get() = withValidityAssertion { firSymbol.name }

    override val isVararg: Boolean get() = withValidityAssertion { firSymbol.isVararg }

    override val isImplicitLambdaParameter: Boolean
        get() = withValidityAssertion {
            firSymbol.source?.kind == KtFakeSourceElementKind.ItLambdaParameter
        }

    override val isCrossinline: Boolean get() = withValidityAssertion { firSymbol.isCrossinline }

    override val isNoinline: Boolean get() = withValidityAssertion { firSymbol.isNoinline }

    override val returnType by cached {
        val returnType = firSymbol.resolvedReturnType
        return@cached if (firSymbol.isVararg) {
            // There SHOULD always be an array element type (even if it is an error type, e.g., unresolved).
            val arrayElementType = returnType.arrayElementType()
                ?: error("No array element type for vararg value parameter: ${firSymbol.fir.renderWithType()}")
            builder.typeBuilder.buildKtType(arrayElementType)
        } else {
            builder.typeBuilder.buildKtType(returnType)
        }
    }

    override val hasDefaultValue: Boolean get() = withValidityAssertion { firSymbol.hasDefaultValue }

    override val annotationsList by cached {
        KtFirAnnotationListForDeclaration.create(
            firSymbol,
            firResolveSession.useSiteFirSession,
            token,
        )
    }

    override val generatedPrimaryConstructorProperty: KtKotlinPropertySymbol? by cached {
        val propertySymbol = firSymbol.fir.correspondingProperty?.symbol ?: return@cached null
        val ktPropertySymbol = builder.variableLikeBuilder.buildPropertySymbol(propertySymbol)
        check(ktPropertySymbol is KtKotlinPropertySymbol) {
            "Unexpected symbol for primary constructor property ${ktPropertySymbol.javaClass} for fir: ${firSymbol.fir.renderWithType()}"
        }

        ktPropertySymbol
    }

    override fun createPointer(): KtSymbolPointer<KtValueParameterSymbol> = withValidityAssertion {
        KtPsiBasedSymbolPointer.createForSymbolFromSource<KtValueParameterSymbol>(this)?.let { return it }

        analyze(firResolveSession.useSiteKtModule) {
            when (val owner = getContainingSymbol()) {
                is KtFunctionLikeSymbol ->
                    KtFirValueParameterSymbolPointer(
                        owner.createPointer(),
                        name,
                        (owner.firSymbol.fir as FirFunction).valueParameters.indexOf(firSymbol.fir),
                    )

                else -> error("${requireNotNull(owner)::class}")
            }
        }
    }

    override fun equals(other: Any?): Boolean = symbolEquals(other)
    override fun hashCode(): Int = symbolHashCode()
}
