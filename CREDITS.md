# Créditos

## Código de base

- **JackPriceBurns y colaboradores de [Foxhollow](https://github.com/JackPriceBurns/foxhollow)**: port nativo original. Base registrada: `b6145f7b5accab19c8f226c696d4a4af5a165362`.
- **zcanann y colaboradores de [SFA-Decomp](https://github.com/zcanann/SFA-Decomp)**: código del juego. La procedencia y revisión se conservan en `game/VENDORED.txt`.
- **Luke Street / encounter y colaboradores de [Aurora](https://github.com/encounter/aurora)**: interfaces modernas de gráficos, audio y entrada. Este árbol utiliza el [fork de JackPriceBurns](https://github.com/JackPriceBurns/aurora) con adaptaciones UWP locales.
- **Luke Street / encounter y colaboradores de [Borealis](https://github.com/encounter/borealis)**: infraestructura del port de escritorio. Sus servicios están desactivados para el frontend UWP propio.
- **Axiomatic Data Laboratories y colaboradores de [MusyX](https://github.com/AxioDL/musyx)**: implementación de audio incluida en extern/musyx.
- **Luke Street / encounter y colaboradores de [nod](https://github.com/encounter/nod)**: lectura de discos ISO, RVZ y GCM.

## Soporte UWP y referencias

- **[SternXD/uwp-dep](https://github.com/SternXD/uwp-dep)**: depósito UWP fijado a `ae39aa004ae127bd00f8262d28b401f8e4ed8728`.
- **Daniel Worley y colaboradores de libuwp**: soporte WinRT incluido en ese depósito, con su aviso de licencia.
- **[XboxEmulationHub/dusklight-uwp](https://github.com/XboxEmulationHub/dusklight-uwp)** y su base **[worleydl/dusklight-uwp](https://github.com/worleydl/dusklight-uwp)**: referencia de compatibilidad UWP, estructura documental y presentación. El menú propio se implementa con ImGui en platforms/uwp/menu_view.cpp.
- **[XboxEmulationHub/SDL3-uwp](https://github.com/XboxEmulationHub/SDL3-uwp)**, **[XboxEmulationHub/dawn-uwp](https://github.com/XboxEmulationHub/dawn-uwp)** y los forks de Aurora registrados en `platforms/uwp/Dependencies.lock.json`: referencias técnicas utilizadas durante el trabajo. El archivo distingue referencias de las dependencias binarias utilizadas.

## Bibliotecas y recursos

Gracias a los autores y colaboradores de SDL, Dawn/Tint, Dear ImGui, libjpeg-turbo, zlib, libpng, FreeType, Abseil, fmt, xxHash, SQLite, Zstandard, Tracy y magic_enum. Véase [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

Roboto conserva Apache 2.0 en `platforms/uwp/Assets/Roboto-LICENSE.txt`. El logo suministrado de Foxhollow se utiliza para identificar el proyecto; no se declara nueva autoría sobre ese recurso.

## Agradecimientos originales

Se conservan los agradecimientos de Foxhollow a **[Dusklight / TwilitRealm](https://twilitrealm.dev/)** y **[Starship / Harbour Masters](https://github.com/HarbourMasters/Starship)**. Son agradecimientos del proyecto original; no se presenta Starship como una dependencia nueva del menú UWP.

Star Fox Adventures fue desarrollado por Rare y publicado por Nintendo. Esta adaptación comunitaria no implica afiliación ni respaldo de sus titulares.
