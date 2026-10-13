# Compilar y firmar

## Requisitos

Windows x64, Visual Studio 2022 con MSVC v143 y herramientas C++ de UWP x64, Windows SDK, LLVM/clang-cl 20, CMake 3.25+, Ninja y Git. CMake, Ninja y clang-cl deben estar en PATH. El script configura el entorno MSVC mediante vswhere.

Las carpetas de extern contienen código modificado incluido en esta distribución. No las reemplaces con submódulos originales.

## Compilar

Desde la raíz:

```powershell
.\platforms\uwp\Build.ps1 -Mode Game -Package
```

Se obtiene `build-game-ninja/foxhollow-Release-x64.appx`. CMake descarga las dependencias fijadas en `platforms/uwp/Dependencies.cmake`. Para usar una copia local:

```powershell
.\platforms\uwp\Build.ps1 -Mode Game -Package -DependencyRoot 'C:\deps\uwp-dep'
```

El modo Game del script no sustituye **App type → Game** en la consola. Los modos Smoke y Desktop son para desarrollo y no representan esta aplicación UWP completa.

## Firmar con tu certificado

El Subject del certificado debe coincidir con el Publisher del manifiesto. El valor predeterminado es `CN=FoxhollowPersonal`. Usa tu propio PFX y conserva su contraseña fuera del repositorio.

```powershell
$password = Read-Host 'Contraseña del certificado' -AsSecureString
.\platforms\uwp\Build.ps1 -Mode Game -Package `
  -Publisher 'CN=MiEditor' `
  -CertificatePath 'C:\certificados\MiEditor.pfx' `
  -CertificatePassword $password
```

Publica únicamente el certificado público .cer correspondiente. Para actualizar una instalación existente conserva editor y certificado; cambiar el editor cambia la identidad del paquete.

Consulta [Microsoft, certificados de firma de paquetes](https://learn.microsoft.com/windows/msix/package/create-certificate-package-signing).

## GitHub Actions

El workflow `.github/workflows/build-uwp.yml` se ejecuta manualmente desde Actions y genera un artifact con APPX sin firma. No crea Releases, no instala certificados y no se conecta al launcher original.

Esta configuración todavía no se ha ejecutado en GitHub. Las pruebas locales no garantizan que un runner remoto tenga todos los componentes de UWP instalados.
