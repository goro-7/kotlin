/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

@file:Suppress("FunctionName")

package org.jetbrains.kotlin.gradle.ide

import org.jetbrains.kotlin.compilerRunner.konanVersion
import org.jetbrains.kotlin.gradle.buildProjectWithMPP
import org.jetbrains.kotlin.gradle.dsl.multiplatformExtension
import org.jetbrains.kotlin.gradle.idea.testFixtures.tcs.assertMatches
import org.jetbrains.kotlin.gradle.idea.testFixtures.tcs.binaryCoordinates
import org.jetbrains.kotlin.gradle.plugin.ide.dependencyResolvers.IdeNativePlatformDependencyResolver
import org.jetbrains.kotlin.konan.target.HostManager
import org.junit.Assume
import kotlin.test.Test

class IdeNativePlatformDependencyResolverTest {

    @Test
    fun `test - posix on linux`() {
        val project = buildProjectWithMPP()
        val kotlin = project.multiplatformExtension
        kotlin.linuxX64()

        val commonMain = kotlin.sourceSets.getByName("commonMain")
        val commonTest = kotlin.sourceSets.getByName("commonTest")
        val linuxX64Main = kotlin.sourceSets.getByName("linuxX64Main")
        val linuxX64Test = kotlin.sourceSets.getByName("linuxX64Test")

        val dependencies = listOf(
            binaryCoordinates("org.jetbrains.kotlin.native:platform.posix:${project.konanVersion}"),
            binaryCoordinates(Regex("""org\.jetbrains\.kotlin\.native:.*:${project.konanVersion}"""))
        )

        IdeNativePlatformDependencyResolver.resolve(commonMain).assertMatches(dependencies)
        IdeNativePlatformDependencyResolver.resolve(commonTest).assertMatches(dependencies)
        IdeNativePlatformDependencyResolver.resolve(linuxX64Main).assertMatches(dependencies)
        IdeNativePlatformDependencyResolver.resolve(linuxX64Test).assertMatches(dependencies)
    }

    @Test
    fun `test - CoreFoundation on macos`() {
        Assume.assumeTrue("Macos host required for this test", HostManager.hostIsMac)
        val project = buildProjectWithMPP()
        val kotlin = project.multiplatformExtension
        kotlin.macosArm64()

        val commonMain = kotlin.sourceSets.getByName("commonMain")
        val commonTest = kotlin.sourceSets.getByName("commonTest")
        val macosArm64Main = kotlin.sourceSets.getByName("macosArm64Main")
        val macosArm64Test = kotlin.sourceSets.getByName("macosArm64Test")

        val dependencies = listOf(
            binaryCoordinates("org.jetbrains.kotlin.native:platform.CoreFoundation:${project.konanVersion}"),
            binaryCoordinates(Regex("""org\.jetbrains\.kotlin\.native:.*:${project.konanVersion}"""))
        )

        IdeNativePlatformDependencyResolver.resolve(commonMain).assertMatches(dependencies)
        IdeNativePlatformDependencyResolver.resolve(commonTest).assertMatches(dependencies)
        IdeNativePlatformDependencyResolver.resolve(macosArm64Main).assertMatches(dependencies)
        IdeNativePlatformDependencyResolver.resolve(macosArm64Test).assertMatches(dependencies)
    }

    @Test
    fun `test - non native source sets`() {
        val project = buildProjectWithMPP()
        val kotlin = project.multiplatformExtension
        kotlin.jvm()
        kotlin.linuxX64()

        val commonMain = kotlin.sourceSets.getByName("commonMain")
        val commonTest = kotlin.sourceSets.getByName("commonTest")
        val jvmMain = kotlin.sourceSets.getByName("jvmMain")
        val jvmTest = kotlin.sourceSets.getByName("jvmTest")

        IdeNativePlatformDependencyResolver.resolve(commonMain).assertMatches(emptyList<Any>())
        IdeNativePlatformDependencyResolver.resolve(commonTest).assertMatches(emptyList<Any>())
        IdeNativePlatformDependencyResolver.resolve(jvmMain).assertMatches(emptyList<Any>())
        IdeNativePlatformDependencyResolver.resolve(jvmTest).assertMatches(emptyList<Any>())
    }
}
