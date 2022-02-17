/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.fir.scopes.impl

import org.jetbrains.kotlin.fir.FirSession
import org.jetbrains.kotlin.fir.declarations.builder.buildImport
import org.jetbrains.kotlin.fir.declarations.builder.buildResolvedImport
import org.jetbrains.kotlin.fir.resolve.ScopeSession
import org.jetbrains.kotlin.name.ClassId

class FirUnqualifiedEnumImportingScope(
    enumClassId: ClassId, session: FirSession, scopeSession: ScopeSession
) : FirExplicitStarImportingScope(
    listOf(buildResolvedImportByEnumClassId(enumClassId)),
    session, scopeSession, FirImportingScopeFilter.ALL
) {
    companion object {
        private fun buildResolvedImportByEnumClassId(enumClassId: ClassId) = buildResolvedImport {
            delegate = buildImport {
                importedFqName = enumClassId.asSingleFqName()
                isAllUnder = true
            }
            packageFqName = enumClassId.packageFqName
            relativeParentClassName = enumClassId.relativeClassName
        }
    }
}