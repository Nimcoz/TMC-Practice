# TMC Practice — Guía en español

[English](README_EN.md) · [Deutsch](README_DE.md) · [Français](README_FR.md) · [Español](README_ES.md) · [Italiano](README_IT.md) · [日本語](README_JA.md)

TMC Practice añade un menú de práctica y exploración a **The Legend of Zelda: The Minish Cap**.
La versión **v1.0.0** ofrece parches BPS separados para USA, Europa y Japón.
Esta guía está traducida; **el propio menú Practice sigue en inglés**.
Se conservan la selección de idiomas europea y los textos japoneses del juego.
Los nombres ingleses de las opciones se mantienen para que puedas encontrarlas en el juego.

## 1. Descarga e instalación

1. Abre [TMC Practice v1.0.0](https://github.com/Nimcoz/TMC-Practice/releases/tag/v1.0.0).
2. Descarga un solo parche para la **región de tu ROM original**, no para el idioma de esta guía:
   - USA: `TMC-Practice-v1.0.0-USA.bps`.
   - Europa (inglés, francés, alemán, español e italiano): `TMC-Practice-v1.0.0-Europe.bps`.
   - Japón: `TMC-Practice-v1.0.0-Japan.bps`.
3. Guarda una copia de seguridad independiente de tu partida normal.
4. Aplica el parche BPS a tu **ROM original sin modificar** con una herramienta compatible con BPS.
5. Abre el archivo `.gba` resultante en tu emulador y usa una partida normal de la misma región.
6. Pulsa **L + R + Select** para abrir o cerrar el menú. Si ya guardaste una combinación personalizada, esta tiene prioridad.

La [lista de compatibilidad de ROMs](ROM_COMPATIBILITY.md) contiene las huellas exactas admitidas.
No ignores los errores de suma de comprobación. No combines parches, códigos AR
externos ni otras modificaciones Practice/No-Clip. No cargues estados del emulador
creados con versiones anteriores. No se incluyen ROMs, partidas ni estados del
emulador. No se distribuye ningún conversor de partidas entre regiones.

## 2. Controles y ajustes

- **Cruceta:** selecciona una fila; izquierda/derecha cambia un valor editable.
- **A:** activar/confirmar. **B:** volver/cancelar.
- **Acciones destructivas:** sigue la confirmación en pantalla; normalmente debes mantener **L + R** y volver a pulsar **A** en la fila de confirmación.
- **Los valores en bruto son hexadecimales.** Izquierda/derecha cambia de uno en uno; L/R cambia en `0x10` los campos de un byte compatibles.
- **SETTINGS:** temas, botones y ajustes persistentes del menú. Guardar estos ajustes **no** guarda automáticamente el progreso de la partida.

## 3. Orientación por el menú

| Menú | Función |
| --- | --- |
| PRACTICE | Cronómetro y controles de práctica; tiempo visible durante el juego |
| PLAYER / MOVEMENT | Link, movimiento, No-Clip y cámara |
| INVENTORY | Objetos, equipo, botellas y elementos individuales |
| WORLD / WARP | Salas, favoritos y repeticiones de la introducción y el final |
| FLAGS | Consultar y modificar indicadores del estado del juego |
| DEBUG | Información de depuración visible durante el juego |
| CHEATS | Recursos, Enemy Freeze, Infinite Time y otros trucos |
| SETTINGS | Aspecto, controles y preferencias guardadas |
| ACTORS / OBJECTS | Lista de actores, generador en bruto, congelación, eliminación y teletransporte |

Las colecciones, los indicadores y la acción **100%** tras confirmarla pueden
cambiar el progreso de la historia. Haz una copia de seguridad antes de experimentar.
Las entradas sin usar/Beta no son contenido Beta reconstruido.

## 4. Abrir el menú fuera del juego normal

Se admiten las pantallas de inventario nativas, diálogos/cinemáticas, título/introducción
y final. Las acciones destructivas sobre el mundo necesitan juego activo y pueden
estar deshabilitadas en esas pantallas. Durante fundidos, transferencias de recursos
o escrituras reales de guardado EEPROM, el menú espera; no todos los fotogramas
de una transición se pueden interrumpir.

## 5. Repetir la introducción o el final

Durante el juego normal, selecciona **WORLD / WARP → STORY INTRO** o **ENDING / CREDITS**.
Pulsa **A** y después **A** otra vez para confirmar. No inicies una repetición desde
un diálogo, una cinemática o el inventario nativo.

**RETURN FROM REPLAY** termina la repetición y vuelve a cargar la sala original.
Los 1.204 bytes de la partida nativa se copian en RAM y se restauran al volver.
Los actores temporales de la sala no se restauran como en un estado del emulador.
Las demás modificaciones, los trucos y el cronometraje se suspenden temporalmente.
La repetición del final regresa antes del aviso normal de guardado; el final normal
de la historia conserva su comportamiento original. Reiniciar el emulador hace
que se pierda el punto de retorno temporal de la RAM.

## 6. Herramientas de actores

**ACTORS / OBJECTS → RAW NATIVE SPAWNER** muestra categorías, nombres y valores
nativos en bruto: kind/ID/type/type2/timer/subtimer/flags/parent/layer.
**QUICK SPAWN PRESETS** no limita el modo en bruto. No hay una lista restrictiva
de salas/plantas/variantes; siguen rechazándose las rutinas nativas inexistentes
y los grupos de actores llenos. Hay etiquetas para 546 posiciones nativas y
118 tipos de objetos en el suelo; 25 entradas permanecen como **UNKNOWN** intencionadamente.

En **ROOM ACTOR LIST**, selecciona un actor para congelarlo, eliminarlo o moverlo.
**LINK TO ACTOR** lleva a Link al actor; **ACTOR TO LINK** trae el actor a Link,
dentro de la sala actual y sin cambiar de sala. Los gestores sin una estructura
XYZ común muestran **NO GENERIC XYZ**: no permiten movimiento genérico, pero sí
congelación/eliminación. Congela primero al actor si su IA vuelve a moverlo.

Un segundo Link, jefes/tipos incompatibles o dependencias ausentes de sala, script
o actor padre pueden bloquear el juego o provocar un fallo. Eliminar un jefe
**no cuenta** como derrotarlo. Si hace falta, vuelve a cargar sin guardar.

## 7. Enemy Freeze, Break Free e Infinite Time

- **Enemy Freeze:** incluye jefes y partes visibles de su cuerpo sin colisión propia. Los proyectiles de otras categorías pueden necesitar congelación individual.
- **Break Free:** cierra el texto activo mediante el estado de cierre nativo y devuelve el control al jugador. No deshace scripts ya ejecutados; otros scripts pueden volver a tomar el control.
- **Infinite Time:** cubre la cuenta atrás de las gallinas de Anju, la del Castillo de Hyrule oscuro, la activación temporal de interruptores con forma de ojo y la duración de amuletos/pociones de suerte ya activos. No congela todos los temporizadores, animaciones o cinemáticas. Desactívalo para permitir la evaluación de las cuentas atrás, las recompensas y la finalización de efectos temporales.

## 8. Pruebas, problemas y colaboración

Esta versión superó **166 conjuntos de pruebas automatizadas de mGBA: 53 USA + 113 EU/JP**.
El probador del proyecto también confirmó la apertura del menú y No-Clip en su
emulador. No se documentó una validación completa en Android para cada región;
no se probó en una GBA real. Consulta la [verificación](VERIFICATION.md).

Comunica errores en [Issues](https://github.com/Nimcoz/TMC-Practice/issues), indicando
región, versión del parche, emulador y versión, pasos y, si ayuda, una captura.
No subas ROMs, partidas privadas ni credenciales. Colaborar en el proyecto oficial
requiere acuerdo previo con Nimcoz; informar de errores no lo requiere. Consulta
las [normas de colaboración](../CONTRIBUTING.md) y los [créditos y derechos](../THIRD_PARTY_NOTICES.md).
