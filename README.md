# Warhammer Isaac · Arena de prueba

Prototipo local de combate para Unreal Engine 5.8. La estación se monta al iniciar la partida con piezas 3D y texturas del [Modular Sci-Fi Megakit de Quaternius](https://quaternius.com/packs/modularscifimegakit.html), con licencia CC0. El código del jugador, los proyectiles y los enemigos está separado para poder ajustar cada parte.

## Abrir el prototipo

En este equipo, abre `Jugar.cmd` para iniciar directamente la arena con DirectX 11. El primer arranque prepara shaders y puede tardar varios minutos.

1. Termina de instalar Unreal Engine 5.8 desde Epic Games Launcher.
2. Instala Visual Studio 2026 con la carga **Desarrollo de juegos con C++**, incluyendo MSVC y el SDK de Windows 11. [Guía oficial de Epic](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine).
3. Abre `WarhammerIsaac.uproject`. Si Unreal ofrece compilar los módulos del proyecto, acepta.
4. Pulsa **Play** en el editor. La sala se genera en ese momento. Haz clic en la vista de juego para darle el foco.

## Probar el combate

El soldado usa el esqueleto y las animaciones CC0 del [Toon Shooter Game Kit de Quaternius](https://quaternius.com/packs/toonshootergamekit.html), con una armadura añadida para el prototipo. La escopeta y su guardamanos móvil son modelos creados para el proyecto. Los tres robots y sus animaciones proceden del [Sci-Fi Essentials Kit de Quaternius](https://quaternius.com/packs/scifiessentialskit.html), con licencia CC0. Los modelos anteriores de Kenney siguen disponibles como respaldo.

- **WASD** mueve al soldado y el **ratón** orienta el arma.
- **Shift izquierdo** activa el sprint mientras lo mantienes pulsado. **Ctrl izquierdo** mientras te mueves inicia un deslizamiento de 0,46 segundos. Puedes corregir un poco la trayectoria, apuntar, disparar y recargar durante el deslizamiento. **Espacio** lo interrumpe con una voltereta defensiva.
- **Clic izquierdo** dispara. Mantenlo pulsado para seguir disparando mientras te mueves.
- La escopeta dispara siete perdigones de 10 puntos de daño cada uno. Acércate para acertar con más perdigones. Destruye los cuatro enemigos y comprueba que aparece **Sala despejada**.
- Dispara hacia el bloque central para comprobar que detiene las balas.
- **R** reinicia la posición, los blancos y el contador. **Esc** sale del modo Play del editor.

Hay tres salas conectadas. Tras limpiar una de las dos primeras, pulsa **1** para mejorar el daño por perdigón un 25 % o **2** para aumentar la cadencia un 30 %. La elección abre el portón y enciende las flechas del pasillo. La vida y las mejoras se conservan al avanzar. El cuadrúpedo persigue y avisa con un círculo naranja antes de golpear. El dron se mantiene fijo en el aire y marca una línea rosa antes de lanzar un disparo rápido. El trilobite mecánico se mueve lateralmente, marca un abanico naranja y dispara tres proyectiles. Las coberturas detienen los disparos. El jugador tiene 100 de vida y puede morir. **R** reinicia desde la primera sala con las estadísticas iniciales.

La escopeta tiene cinco cartuchos y reserva infinita. Pulsa **E** para recargar manualmente. Al agotar el cargador, la recarga se inicia sola. Dura 1,35 segundos y permite moverse. El dash interrumpe la recarga. Si el cargador está vacío, vuelve a iniciarse al terminar el dash.

**Espacio** activa el dash en la dirección de movimiento o la última registrada. Su duración y su ventana de invulnerabilidad se ajustan por separado en `Config/DefaultGame.ini`. La voltereta dura 0,34 segundos y la ventana de invulnerabilidad dura 0,30 segundos. Los proyectiles enemigos atraviesan al jugador durante la ventana de invulnerabilidad. El dash tiene 1,2 segundos de cooldown y se detiene al chocar con paredes. El disparo queda bloqueado durante el desplazamiento. Una pulsación de disparo puede quedar guardada hasta 0,12 segundos y se ejecuta en cuanto termina el bloqueo.

La cámara usa una inclinación inicial de 50 grados, giro diagonal de 45 grados, distancia de 3000 unidades y FOV de 50 grados. Cada sala tiene una zona central de seguimiento de 1200 por 1200 unidades. Al salir conserva el encuadre hasta que el jugador se acerca a un borde de pantalla, momento en que se desplaza lo justo para mantenerlo visible. Al volver a entrar, el seguimiento acelera progresivamente durante 0,45 segundos. **F2** muestra la zona. WASD usa los ejes de pantalla y el ratón apunta independientemente del movimiento.

En los pasillos la cámara vuelve a seguir al jugador al cruzar el portón y adelanta el encuadre hacia la sala siguiente. Ese adelanto se reduce al llegar a la zona central.

El ángulo, distancia, FOV y límites de cámara se pueden ajustar en `Config/DefaultGame.ini` y se aplican al volver a abrir el juego. Los valores de dash y disparo están en `ArenaPawn.h`. La vida inicial de los enemigos está en `ArenaGameMode.h` y los parámetros de su ataque en `ArenaEnemy.h`.
