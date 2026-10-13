# Instalación y uso

## Xbox Series X|S

1. Activa Developer Mode y abre Xbox Device Portal desde el PC.
2. Instala el APPX x64 de Foxhollow. Si se solicitan dependencias, proporciona `Microsoft.VCLibs.x64.14.00.appx` de la distribución.
3. Cierra Foxhollow. En Dev Home selecciónalo, pulsa **View → View details** y configura **App type → Game**.
4. Abre Foxhollow y conecta una USB reconocida por Xbox que contenga tu ISO, RVZ o GCM.
5. En **SELECCIONAR JUEGO**, entra en la unidad y selecciona el archivo.
6. Espera la comprobación del disco. No se copia al almacenamiento de Xbox.
7. Selecciona **JUGAR**. Mantén la USB conectada durante toda la partida.

El juego puede inicializarse detrás del menú cuando hay un disco válido. El fondo permanece negro hasta que exista imagen visible del juego. Cancelar la selección conserva el disco anterior. Se recuerda la ruta para el siguiente inicio; si desconectas la USB o cambia su ruta, debes volver a seleccionarlo.

Para recursos UWP y Game mode: [Microsoft](https://learn.microsoft.com/en-us/previous-versions/windows/uwp/xbox-apps/system-resource-allocation) y [guía técnica de DirectX](https://walbourn.github.io/directx-and-uwp-on-xbox-one/).

## Windows

Instala el mismo APPX para probar el menú UWP. El ejecutable de escritorio del proyecto original utiliza otro flujo.

Si aparece **0x800B010A**, Windows no confía en el certificado del editor. Para un paquete de origen verificado:

1. Abre el certificado público **.cer** correspondiente al APPX.
2. Selecciona **Instalar certificado → Equipo local**.
3. Elige **Colocar todos los certificados en el siguiente almacén → Personas de confianza**.
4. Finaliza y vuelve a instalar el APPX.

Requiere administrador y permite confiar en los paquetes firmados con ese certificado. No necesitas el PFX. Si se solicita VCLibs, instala la dependencia x64 incluida.

Referencia: [Microsoft, certificados de firma](https://learn.microsoft.com/windows/msix/package/create-certificate-package-signing).

## Controles del menú

| Entrada | Acción |
|---|---|
| Cruceta / stick | Navegar |
| A | Seleccionar |
| B | Volver |
| View durante el juego | Abrir/cerrar la barra durante la partida |
| F1 en Windows | Abrir/cerrar la barra durante la partida |
| Flechas, Enter y Escape | Navegar, seleccionar y volver |

## Ajustes

720p o 1080p; aspecto 4:3 o 16:9; idiomas PAL automático, inglés, francés, alemán, italiano y español.

Usa **GUARDAR Y VOLVER** para conservar los ajustes. Cambiar el disco o idioma puede requerir **REINICIAR**. Si el sistema no acepta el reinicio, cierra y abre Foxhollow. Para la primera prueba de consola, empieza con 720p.

## Partidas y almacenamiento

Abre la aplicación al menos una vez para crear sus carpetas. En Device Portal busca `LocalState/Foxhollow/` dentro de la aplicación. En Windows la ruta habitual es `%LOCALAPPDATA%/Packages/<familia del paquete>/LocalState/Foxhollow/`; la familia depende del certificado.

| Ruta relativa | Función |
|---|---|
| `disc-path.txt` | Ruta del disco USB seleccionado |
| `disc.iso` | Copia local de versiones anteriores, conservada por compatibilidad |

| `settings.ini` | Ajustes |
| `foxhollow.log` | Registro acumulado |
| `cache/` | Caché gráfica |
| `EUR/Card A`, `USA/Card A`, `JPN/Card A` | Tarjeta de memoria según región; confirma la ruta en el log |

Copia la tarjeta de memoria con el juego cerrado antes de desinstalar o cambiar la identidad del paquete.

## Problemas conocidos

- **E_OUTOFMEMORY / 0x8007000E:** comprueba App type → Game, cierra otras aplicaciones y prueba a 720p. Si continúa, adjunta el log del intento afectado.
- **0x800B0100:** paquete sin firma; instala un APPX firmado.
- **0x800B010A:** certificado no confiable; consulta los pasos para Windows.
- **USB ausente:** comprueba que Xbox reconozca la unidad y pulsa Actualizar.
- **Disco rechazado:** revisa ID y revisión en el README; solo admite Star Fox Adventures.
- **initial_pipeline_cache.db ausente:** ese mensaje por sí solo no demuestra un fallo; la caché puede generarse al jugar.

Al informar de un error, indica versión, consola/PC, región y revisión del disco, resolución y pasos para reproducir. No adjuntes discos, partidas ni claves privadas.

## Comprobar el rendimiento

Abre Configuración, marca **Contador de FPS** y pulsa **GUARDAR Y VOLVER**. La opción se guarda, está desactivada por defecto y puede cambiarse sin reiniciar el juego.

El recuadro superior derecho muestra FPS, tiempo medio por fotograma y el pico más alto del intervalo de medición, actualizado aproximadamente cada medio segundo. Mide el ritmo de los fotogramas del port, incluidos menú, esperas y trabajo de CPU; no es una medición de uso de GPU ni de fotogramas únicos de la simulación original.

Como referencia, 60 FPS corresponden a unos 16,7 ms por fotograma y 30 FPS a unos 33,3 ms. Un pico de tiempo alto puede revelar un parón que el promedio de FPS oculta.

Prueba la misma zona a 720p y 1080p, con el menú cerrado, y compara FPS y picos. Repite el recorrido para distinguir el primer paso de los siguientes. Si el parón continúa, envía un vídeo con el contador visible y el log del mismo intento; la herramienta identifica los tiempos, no demuestra por sí sola su causa.

## Menú de inicio y barra durante el juego

El menú principal aparece al abrir la aplicación y se abandona al seleccionar **JUGAR**. Durante la partida, **View** (o **F1** en Windows) abre y cierra la barra superior. El botón central Xbox conserva su función de sistema; la aplicación utiliza View.

La barra ofrece Configuración y Salir, además de una X para cerrar. Los ajustes se muestran en un panel sobre el juego. **B** vuelve desde el panel a la barra; desde la barra permite continuar la partida. Si hay una importación de texturas en curso, usa Cancelar importación antes de cerrar el menú.

Al abrir el menú se establece el foco de navegación, de modo que A puede activar la primera opción disponible sin mover antes la cruceta. Esta interacción se ha comprobado con entrada simulada de mando en Windows; se requiere confirmación en Xbox.

## Opciones de la barra desde 0.5.8.10

Durante la partida, la barra ofrece **Configuración** y **Salir**, además del cierre con X/View. Seleccionar **Salir** abre un panel con **Reiniciar** y **Salir de la aplicación**; ninguna de esas acciones se ejecuta al abrir el panel. Reiniciar vuelve a abrir la aplicación y Salir de la aplicación la cierra. **Juego/USB** queda exclusivamente en el menú principal al iniciar la app.

## Texturas

En el menú principal, abre **TEXTURAS** y selecciona un ZIP desde la USB. Espera la copia y validación antes de jugar. Puedes activar o desactivar el pack desde esa página. Consulta [TEXTURES.md](TEXTURES.md) para formatos, espacio necesario y resultados de la prueba.

## Lectura desde USB desde 0.5.8.12

Seleccionar un ISO, RVZ o GCM verifica su cabecera y región/revisión y guarda su ruta; no copia el disco ni realiza una comprobación completa de su hash. El motor lee el archivo original desde USB. La velocidad y estabilidad de esa unidad pueden afectar a las cargas. Mantén la unidad conectada mientras juegas.

Las copias locales creadas por versiones anteriores se conservan; no se borran automáticamente. Al seleccionar un juego desde USB, la ruta elegida tiene prioridad. Las partidas, ajustes y texturas continúan guardándose en Xbox. La importación de ZIP de texturas conserva su funcionamiento.

## Texturas durante la partida desde 0.5.8.16

Pulsa View/F1 y abre **TEXTURAS** en la barra superior. El panel muestra el pack instalado y su estado; selecciona **ACTIVAR TEXTURAS** o **DESACTIVAR TEXTURAS** para aplicar y guardar el cambio sin reiniciar. Si no hay pack, el panel indica que debes instalarlo desde el menú principal. La búsqueda e importación de ZIP permanecen en el menú principal. B vuelve a la barra y View la cierra.
