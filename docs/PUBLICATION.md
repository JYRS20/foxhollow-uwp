# Publicar en GitHub

Esta carpeta es la copia limpia del código. Los discos, partidas, certificados, builds, herramientas locales y referencias de investigación permanecen fuera de ella.

1. Crea un repositorio vacío en tu cuenta, por ejemplo foxhollow-uwp.
2. Añade esta carpeta con GitHub Desktop o Git. Conserva las bibliotecas incluidas, avisos y licencias.
3. Revisa README y créditos antes de publicar. Las capturas son vistas previas del menú.
4. El workflow de compilación solo se ejecuta manualmente desde Actions y no publica automáticamente.

Los APPX se adjuntan a una **Release**, no al código fuente. Para 0.5.8.18 utiliza la etiqueta v0.5.8.18 y adjunta la distribución preparada por separado: APPX firmado, certificado público correspondiente, VCLibs x64, guía de uso, créditos, licencias y SHA256SUMS.txt.

Nunca adjuntes PFX, contraseñas, discos o partidas. Los hashes comprueban integridad; no sustituyen la comprobación del editor y su firma. La versión fue seleccionada para publicación tras la confirmación del usuario en Xbox; no se garantiza compatibilidad con todos los packs y discos.

El historial original no se incluye en la copia limpia. La procedencia se conserva en la documentación y los registros de dependencias. No se cambian las licencias del código de terceros.

## Informes de error

Incluye versión, consola/PC, región/revisión del disco, resolución, pasos para reproducir y log del intento afectado. Omite rutas personales que no sean necesarias.

## Archivos de esta publicación

- Código: extrae `foxhollow-uwp-source-0.5.8.18.zip` y sube el contenido de su carpeta `foxhollow-uwp` a la raíz del repositorio. README.md y CMakeLists.txt deben quedar en la raíz, no dentro de otra carpeta.
- Instalación: adjunta `foxhollow-uwp-0.5.8.18-x64-release.zip` a la Release con etiqueta `v0.5.8.18` y título **Foxhollow UWP 0.5.8.18**.
- Descripción de la Release: utiliza `docs/RELEASE_NOTES.md`.

Por la cantidad de archivos, usa Git o GitHub Desktop para subir el código completo. No subas la carpeta de desarrollo ni los ZIP de instalación como archivos del repositorio.
