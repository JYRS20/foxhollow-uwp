# Cambios de Foxhollow UWP

La numeración es propia del paquete UWP. El historial de escritorio se conserva en docs/upstream/CHANGELOG.md.

## 0.5.8.16 — 2026-10-13

TEXTURAS en la barra durante la partida. Panel con nombre del pack, recuentos, estado y botón para activar/desactivar sin reiniciar, con selección persistente. Importación de ZIP reservada al menú principal. El botón conserva el foco del mando al cambiar de estado.

Pruebas de navegación y dos cambios consecutivos de estado con mando simulado, panel sin pack, opciones de salida y activación inicial de A en Windows. Vistas previas a 720p y 1080p y regresión del importador correctas. Compilación y firma verificadas; versión seleccionada para publicación tras la confirmación del usuario en Xbox.

## 0.5.8.15 — 2026-10-13

Juego y dispositivos de audio pausados durante la copia, extracción, comprobación y activación de un pack. El menú y los eventos siguen activos para mostrar progreso y cancelar. Al finalizar, cancelar o fallar se reanuda el juego sin avanzar fotogramas de simulación durante la espera. Registro de las nuevas texturas después de la pausa. La navegación por carpetas no pausa el juego.

Prueba del bucle real VIWaitForRetrace con eventos y menú activos, callbacks y bombeo de audio detenidos durante la espera, reanudación y fotogramas normales sin cambios. Regresión de importación, cancelación, rechazo de ZIP y selección USB correcta en Windows. Compilación y firma verificadas; pendiente de comprobar la pausa y el audio en Xbox.

## 0.5.8.14 — 2026-10-12

Carpetas, enumeración, renombrado y limpieza de texturas gestionados con Windows.Storage. Eliminada la resolución weakly_canonical que daba acceso denegado en Xbox antes de copiar el ZIP. Lectura y escritura del extractor mediante SDL; registro de reemplazos mediante callbacks SDL para evitar la enumeración de carpetas del motor basada en std::filesystem. Conservadas validaciones de ZIP, CRC, DDS y rutas y la selección previa ante errores.

Compilación, firma y pruebas de importación, activación, cancelación y lectura USB comprobadas en Windows. El juego ya abrió en Xbox con 0.5.8.13 según la captura del usuario. La corrección del importador necesita una nueva prueba de consola. Los errores previos de memoria correspondían a App type App según el usuario; luego cambió a Game.

## 0.5.8.13 — 2026-10-12

Corregida la apertura del disco USB: el arranque obtiene nuevamente la ruta después del menú, antes de pasarla a Aurora. Evita utilizar una referencia liberada al sustituir la ruta anterior. Añadido registro de la ruta que se abre.

Compilación y firma verificadas, pruebas de selección directa e importador de texturas correctas en Windows. La prueba anterior en Xbox detectó el defecto; falta confirmar la corrección en consola. Persisten errores intermitentes de memoria D3D12 observados al iniciar; este cambio no los corrige.

## 0.5.8.12 — 2026-10-12

Disco leído directamente desde USB, sin copia a Xbox. Selección validada y ruta persistente; disco ausente requiere conectar la unidad o elegir otra ruta. El motor recibe la ruta seleccionada al iniciar; cambios posteriores se aplican al reiniciar. Copias locales antiguas conservadas y texturas locales sin cambios.

Pruebas en Windows de selección, ausencia de copia, persistencia, rechazo de disco inválido y USB ausente; regresión de instalación, activación, validación y cancelación de texturas. APPX compilado y firmado. Pendiente de prueba de lectura directa en Xbox.

## 0.5.8.11 — 2026-10-12

TEXTURAS en el menú principal: selección de ZIP desde USB, progreso de copia y extracción, validación de DDS/PNG y CRC, instalación con conservación del pack anterior ante errores, activación y desactivación persistentes. Reemplazos Aurora actualizados en el hilo del juego y caché UWP de 512 MiB. Mods de código desactivados.

Pruebas en Windows con el pack completo y casos de CRC, rutas inseguras, DDS incompatible, cancelación y conservación del pack anterior; regresión de importación de discos y navegación del menú. Compilación y firma verificadas. Pendiente de comprobar aspecto y rendimiento del pack en Xbox.

## 0.5.8.10 — 2026-10-12

Barra durante el juego simplificada a Configuración y Salir. Salir abre un submenú con Reiniciar y Salir de la aplicación. Juego/USB permanece solo en el menú principal. Comprobadas la navegación del submenú y la activación inicial de A con mando simulado en Windows.

## 0.5.8.9 — 2026-10-12

Menú principal reservado al inicio de la aplicación. Barra superior independiente durante la partida, abierta con View/F1, con paneles de ajustes y USB, reinicio, salida y cierre.

Corregida la activación inicial de A sin navegación previa. Pruebas de mando en Windows para inicio con y sin disco y para la barra durante el juego; diseños comprobados a 720p y 1080p. Pendiente de confirmar en consola.

## 0.5.8.8 — 2026-10-12

Contador opcional de FPS, tiempo medio por fotograma y pico del intervalo, con estado persistente. Ajustes antiguos compatibles y contador desactivado por defecto. Guardar el contador no vuelve a configurar los gráficos si aspecto y resolución no cambiaron.

Pruebas deterministas de ritmo de fotogramas y parón de 120 ms, guardado de la opción y navegación del menú en Windows. Pendiente de prueba en Xbox.

## 0.5.8.7 — 2026-10-12

Fondo completamente negro mientras no haya imagen visible del juego. Conserva el menú sobre el juego y la barra de importación USB.

## 0.5.8.6 — 2026-10-12

Porcentaje real, MB copiados y nombre del archivo durante la importación USB; fase de verificación y cancelación. Pruebas de copia, cancelación y rechazo de un disco inválido en Windows.

## 0.5.8.5 — 2026-10-09

Cambio de asignación de recursos D3D12 para evitar el fallo registrado de CreatePlacedResource y registros de tamaños de buffers. Menú UWP, selector USB, ajustes de aspecto, resolución e idioma PAL.

Persisten errores de memoria registrados en algunos arranques de consola. Las versiones posteriores requieren nuevas pruebas de Xbox.
