/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.light.classes.symbol.classes

import com.intellij.psi.PsiManager
import com.intellij.psi.PsiModifier
import com.intellij.psi.PsiModifierList
import com.intellij.psi.PsiReferenceList
import org.jetbrains.kotlin.analysis.api.KtAnalysisSession
import org.jetbrains.kotlin.analysis.api.lifetime.isValid
import org.jetbrains.kotlin.analysis.api.symbols.KtClassKind
import org.jetbrains.kotlin.analysis.api.symbols.KtNamedClassOrObjectSymbol
import org.jetbrains.kotlin.asJava.classes.lazyPub
import org.jetbrains.kotlin.asJava.elements.KtLightField
import org.jetbrains.kotlin.light.classes.symbol.NullabilityType
import org.jetbrains.kotlin.light.classes.symbol.annotations.computeAnnotations
import org.jetbrains.kotlin.light.classes.symbol.modifierLists.SymbolLightClassModifierList
import org.jetbrains.kotlin.light.classes.symbol.toPsiVisibilityForClass

context(KtAnalysisSession)
internal abstract class SymbolLightInterfaceOrAnnotationClass(
    private val classOrObjectSymbol: KtNamedClassOrObjectSymbol,
    manager: PsiManager
) : SymbolLightClassForClassOrObject(classOrObjectSymbol, manager) {

    init {
        require(
            classOrObjectSymbol.classKind == KtClassKind.OBJECT ||
                    classOrObjectSymbol.classKind == KtClassKind.INTERFACE ||
                    classOrObjectSymbol.classKind == KtClassKind.ANNOTATION_CLASS
        )
    }

    private val _modifierList: PsiModifierList? by lazyPub {
        val lazyModifiers = lazy {
            buildSet {
                add(classOrObjectSymbol.toPsiVisibilityForClass(isNested = !isTopLevel))
                add(PsiModifier.ABSTRACT)
                if (!isTopLevel && !classOrObjectSymbol.isInner) {
                    add(PsiModifier.STATIC)
                }
            }
        }

        val lazyAnnotations = lazyPub {
            classOrObjectSymbol.computeAnnotations(
                parent = this@SymbolLightInterfaceOrAnnotationClass,
                nullability = NullabilityType.Unknown,
                annotationUseSiteTarget = null,
            )
        }

        SymbolLightClassModifierList(this@SymbolLightInterfaceOrAnnotationClass, lazyModifiers, lazyAnnotations)
    }

    override fun getModifierList(): PsiModifierList? = _modifierList

    private val _ownFields: List<KtLightField> by lazyPub {
        mutableListOf<KtLightField>().also {
            addCompanionObjectFieldIfNeeded(it)
            addFieldsFromCompanionIfNeeded(it)
        }
    }

    override fun getOwnFields(): List<KtLightField> = _ownFields

    override fun getImplementsList(): PsiReferenceList? = null

    override fun isInterface(): Boolean = true

    override fun isEnum(): Boolean = false

    override fun isValid(): Boolean = super.isValid() && classOrObjectSymbol.isValid()
}
