/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.fir.scopes.impl

import org.jetbrains.kotlin.descriptors.EffectiveVisibility
import org.jetbrains.kotlin.descriptors.Modality
import org.jetbrains.kotlin.descriptors.Visibilities
import org.jetbrains.kotlin.fir.FirSession
import org.jetbrains.kotlin.fir.declarations.FirDeclaration
import org.jetbrains.kotlin.fir.declarations.FirDeclarationOrigin
import org.jetbrains.kotlin.fir.declarations.FirProperty
import org.jetbrains.kotlin.fir.declarations.builder.buildProperty
import org.jetbrains.kotlin.fir.declarations.impl.FirResolvedDeclarationStatusImpl
import org.jetbrains.kotlin.fir.moduleData
import org.jetbrains.kotlin.fir.scopes.FirTypeScope
import org.jetbrains.kotlin.fir.scopes.ProcessorAction
import org.jetbrains.kotlin.fir.symbols.impl.FirNamedFunctionSymbol
import org.jetbrains.kotlin.fir.symbols.impl.FirPropertySymbol
import org.jetbrains.kotlin.fir.symbols.impl.FirVariableSymbol
import org.jetbrains.kotlin.fir.types.FirTypeRef
import org.jetbrains.kotlin.name.CallableId
import org.jetbrains.kotlin.name.Name

class FirSyntheticsScope(
    private val declaration: FirDeclaration,
    private val session: FirSession,
) : FirTypeScope() {
    override fun processDirectOverriddenFunctionsWithBaseScope(
        functionSymbol: FirNamedFunctionSymbol,
        processor: (FirNamedFunctionSymbol, FirTypeScope) -> ProcessorAction
    ): ProcessorAction = ProcessorAction.NEXT

    override fun processDirectOverriddenPropertiesWithBaseScope(
        propertySymbol: FirPropertySymbol,
        processor: (FirPropertySymbol, FirTypeScope) -> ProcessorAction
    ): ProcessorAction = ProcessorAction.NEXT

    override fun getCallableNames(): Set<Name> = emptySet()

    override fun getClassifierNames(): Set<Name> = emptySet()

    override fun processFunctionsByName(
        name: Name,
        processor: (FirNamedFunctionSymbol) -> Unit
    ) {
    }

    override fun processPropertiesByName(
        name: Name,
        processor: (FirVariableSymbol<*>) -> Unit
    ) {
        getPropertyByName(name)?.let {
            processor(it)
        }
    }

    private val fieldName = Name.identifier("field")

    private fun getPropertyByName(name: Name) = when (name) {
        fieldName -> tryGetFieldProperty()?.symbol
        else -> null
    }

    private fun tryGetFieldProperty(): FirProperty? {
        val property = declaration as? FirProperty ?: return null
        val backingField = property.backingField ?: return null
        return getFieldProperty(backingField.returnTypeRef)
    }

    private fun getFieldProperty(backingFieldType: FirTypeRef): FirProperty = buildProperty {
        this.name = fieldName
        this.symbol = FirPropertySymbol(CallableId(this.name))

        status = FirResolvedDeclarationStatusImpl(
            Visibilities.Local,
            Modality.FINAL,
            EffectiveVisibility.Local,
        )

        moduleData = session.moduleData
        origin = FirDeclarationOrigin.Synthetic
        returnTypeRef = backingFieldType
        isVar = true
        isLocal = false
    }
}