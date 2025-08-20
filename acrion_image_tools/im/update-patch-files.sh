#!/usr/bin/env bash
diff -U1 --label PKGBUILD --label PKGBUILD PKGBUILD-arch-orig PKGBUILD-arch-modified >PKGBUILD-arch.patch
diff -U1 --label PKGBUILD --label PKGBUILD PKGBUILD-mingw-orig PKGBUILD-mingw-modified >PKGBUILD-mingw.patch
