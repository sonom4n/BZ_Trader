# BZ_Trader — Manual del admin

> **Traveling_Trader on Wheels.** La idea del comerciante ambulante de Traveling Trader Expansion, de FUBAR, puesta en un vehículo que maneja BZ AutoDrive. En cada parada los comerciantes se bajan, caminan a su puesto, venden, vuelven y suben, y el vehículo sigue.
> *Manual 1.0 (24/10/2026) — DayZ 1.30 · BZ AutoDrive 1.2.4 o posterior · DayZ Expansion 1.9.79.*

---

## Contenido

1. Qué hace
2. Requisitos
3. Tu primer comerciante ambulante
4. Qué vende un comerciante: el Market de Expansion
5. El editor
6. Cómo es una parada
7. Vehículos, plazas y puertas
8. El archivo de servicio
9. Cambios sin reiniciar
10. Leer el log del server
11. Problemas y límites conocidos
12. Créditos y licencia

---

## 1. Qué hace

BZ_Trader usa dos mods y agrega la parte que va entre ellos. No reemplaza a ninguno.

| Tarea | La hace |
|---|---|
| Manejar la ruta, detenerse en la parada, seguir, retirar el vehículo | BZ AutoDrive |
| Bajar, caminar al puesto, vender, volver, subir, las señales, la bocina, decidir cuándo salir, y proteger a los comerciantes | BZ_Trader |
| Qué vende cada comerciante, a qué precio y con qué stock | El Market de Expansion (tus archivos) |

### Lo que viene de Traveling Trader Expansion

BZ_Trader no necesita Traveling Trader Expansion (TTE), pero nace de él: es la idea del comerciante ambulante de FUBAR, puesta en un vehículo. De TTE toma:

- **La idea.** Un comerciante que recorre el mapa. En TTE se lo puede matar y saquear; a los de BZ_Trader, no (sección 7).
- **Cómo se mueven a pie.** Abajo del vehículo, cada comerciante camina como un caminante de TTE: por waypoints, dentro de su grupo de AI. Mientras vende, se frena y mira al jugador que se acerca.
- **El formato del archivo.** Donde un campo significa lo mismo que en una ruta de TTE, tiene el mismo nombre y formato: `TraderFile`, `Waypoints` (`WayPointName`, `Position`), `MovementSpeed`, `LootingBehaviour` (con el valor por defecto de TTE) y el marcador de mapa (`Markers`). Un admin que conoce TTE ya conoce esos campos.
- **Las protecciones.** Que nadie pueda reclutar, esposar o atender a los comerciantes está adaptado del código de TTE, con permiso de FUBAR (sección 7).

El esquema de caminar es código propio. Si tu server además tiene TTE, los dos conviven.

### Qué podés armar

- Un vendedor de bebidas en moto que vende desde el asiento en cada parada.
- Una camioneta con un médico, un almacenero y un vendedor de ropa que arman su puesto en cada parada.
- Un bus que lleva un mercado entero: seis comerciantes en semicírculo, con el humo rojo de un airdrop para que los jugadores lo vean de lejos.
- Una zona de comercio que viaja con el vehículo y conserva su stock en cualquier punto del mapa.
- Una tripulación que nadie puede matar, con uno de los comerciantes al volante, para que el vehículo termine siempre su ruta.
- Vehículos cerrados a jugadores: nadie se sienta, y los jugadores compran en los puestos o por las puertas abiertas.

---

## 2. Requisitos

| Mod | Notas |
|---|---|
| `@CF` | Community Framework. |
| DayZ Expansion | AI, Market y Navigation (y Vehicles, para el cierre de los vehículos: sección 7; Missions, sólo para la señal `airdrop`): el `@DayZ-Expansion-Bundle` (con lo que pide: Dabs Framework, Expansion Licensed) o esos módulos sueltos. Probado con el Bundle. |
| `@BZ_AutoDrive` | Versión 1.2.4 o posterior. [Workshop 3751815579](https://steamcommunity.com/sharedfiles/filedetails/?id=3751815579) |
| `@BZ_Trader` | Sólo de servidor: los jugadores no lo descargan. [Workshop 3822271044](https://steamcommunity.com/sharedfiles/filedetails/?id=3822271044) |

- **Expansion no figura entre los artículos requeridos del Workshop, a propósito.** Un server que ya usa módulos sueltos de Expansion chocaría con el Bundle. Suscribite a lo que use tu server.
- **Traveling Trader Expansion no hace falta.** Si tu server ya lo tiene, puede quedarse: los dos conviven.
- **`@BZ_Trader` va dentro de la carpeta del server,** al lado de `DayZServer_x64.exe`, como los demás mods del server. Copiala desde donde la baja Steam al suscribirte. El editor tiene que correr desde ahí: lee el server en el que está. Abierto desde la carpeta del Workshop, no encuentra ningún server.
- **El primer arranque.** El mod crea `profiles\BZ_Trader\`. Si la carpeta está vacía, escribe un servicio de ejemplo apagado (`Enable: 0`).
- **El editor** es una página web en `@BZ_Trader\tools\editor\`, sin nada que instalar. Se abre con `Editor.bat`, al lado: lee el server donde está y guarda directo en él, también dentro de `Program Files` si Windows le deja escribir al usuario. Sin el `.bat` se abre en Chrome o Edge, y dentro de `Program Files` sólo puede leer (sección 5).

---

## 3. Tu primer comerciante ambulante

De un server vacío a un comerciante en la ruta, sin reiniciar. Este recorrido completo se probó en el juego, con el server y el cliente corriendo.

1. **Elegí qué vende.** Un archivo de comerciante de Expansion en `profiles\ExpansionMod\Traders\`, o la copia de uno con otros artículos. Ejemplo: un comerciante de bebidas con la botella de agua y latas de gaseosa. Ver la sección 4.
2. **Grabá la ruta.** En el juego, con BZ AutoDrive: **NUMPAD 5** graba y **NUMPAD 4** marca cada lugar donde el comerciante tiene que parar. Manejala como querés que la maneje él. El manual de AutoDrive explica la grabación en detalle.
3. **Convertila con el Wizard de AutoDrive.** El Wizard escribe tres archivos de ruta en `profiles\BZ_AutoDrive\`: `<ruta>.json`, `<ruta>_hdr.json` y `<ruta>_wp.csv`.
4. **Abrí el editor (`Editor.bat`, en el `@BZ_Trader` de la carpeta del server) y tocá Escanear server.** Después armá un servicio nuevo desde la toma convertida, eligiéndola por su nombre. Si tu server está en un host, ver **Server en un host** (sección 5).
5. **Decí quién viaja.** Para cada comerciante: plaza, personaje, ropa y qué vende. Uno de ellos maneja (**Quién maneja**); en un servicio nuevo, el primero que agregás.
6. **Armá cada parada.** Quién baja, dónde queda cada puesto, hacia dónde mira cada comerciante, cuánto vende y, si querés, señales.
7. **Revisá y guardá.** **Guardar todo** escribe el archivo de servicio, la tripulación en la ruta y la zona de comercio si es nueva.
8. **El server lo toma solo.** En menos de 10 segundos el server lee el servicio nuevo y lo lanza. No hace falta reiniciar, salvo en los casos de la sección 9.

---

## 4. Qué vende un comerciante: el Market de Expansion

BZ_Trader nunca toca tu economía. Un comerciante ambulante vende igual que un comerciante fijo de Expansion, y tres archivos del Market deciden qué y cuánto.

| Archivo | Dónde | Qué decide |
|---|---|---|
| Comerciante | `profiles\ExpansionMod\Traders\<Nombre>.json` | Las categorías y los artículos que maneja. En `Items`, el número es el modo de comercio (`1` = los jugadores lo compran y lo venden). No es el stock. |
| Categoría | `profiles\ExpansionMod\Market\<Categoría>.json` | Los precios, `MaxStockThreshold` e `InitStockPercent`. |
| Zona de comercio | `<misión>\expansion\traderzones\<Zona>.json` | El stock real de esa zona. |

- **Una zona que viaja.** Con `TraderZone.FollowsVehicle: 1` la zona del servicio se mueve con el vehículo, así que sus comerciantes venden con el mismo stock en cualquier punto del mapa. El editor ofrece crear la zona (con el nombre `BZT_<servicio>`) si no existe. Cuando un comerciante se engancha a una zona, el Market le agrega los artículos que le faltan.
- **La reputación.** Si la Hardline tiene `UseReputation: 1` (`profiles\ExpansionMod\Settings\HardlineSettings.json`), los artículos con rareza piden reputación para comprarlos. El editor te avisa.
- **Artículos que el Market no tiene.** El Market de Expansion 1.9.79 no trae la ropa nueva de la DayZ 1.30. Para venderla, agregá un archivo de categoría con esos artículos y sus precios, y un archivo de comerciante que nombre esa categoría. Una categoría nueva pide reiniciar el server.
- **La botella de agua se vende vacía.** Así es el artículo vanilla: el jugador la llena. Para bebidas listas, sumá latas de gaseosa.

---

## 5. El editor

El editor corre siempre desde la copia de `@BZ_Trader` que está dentro de la carpeta del server (sección 2). Lo más fácil es `Editor.bat`, en `@BZ_Trader\tools\editor\`. Abre el editor ya leyendo ese server, sin elegir carpetas, y guarda directo, también dentro de `Program Files` si Windows le deja escribir al usuario (en un server de Steam, normalmente sí; si no, descarga el archivo y dice adónde va). Su ventana dice de qué server es; cerrarla lo termina. Al cerrar el editor en el navegador (todas sus pestañas), la ventana se cierra sola a los pocos segundos; una recarga (F5) no la cierra. Si la pestaña queda dormida mucho tiempo, la ventana también se cierra: abrí `Editor.bat` otra vez. Sin el `.bat`: `editor_bz_trader.html` con doble clic, en Chrome o Edge. Los botones **EN** y **ES** de arriba a la derecha cambian el idioma en el acto, y la elección queda recordada; **Manual**, al lado, abre este manual en PDF, en el mismo idioma. El editor abre siempre vacío: recuerda la carpeta de cada server, las plazas de cada vehículo y el idioma, pero no el último servicio. El editor trabaja en cuatro pasos, de arriba hacia abajo.

### Paso 1 · El server

- El editor dice de qué server es. **Escanear server** lo lee entero (los `.bat`, los perfiles y las misiones, y las tomas y los servicios de cada mapa que carga `@BZ_Trader`) y lo resume en una línea. Con el `.html`, la primera vez se elige la carpeta del server (la que tiene `DayZServer_x64.exe`) y queda recordada **por server**. **Elegir otra carpeta** es para otro server a propósito: el editor avisa y no la recuerda.
- El editor lee el `.bat` del server que carga `@BZ_Trader` y saca de ahí el perfil y la misión. Ejemplo: un `start_server.bat` con `-profiles=profiles` y `-mission=mpmissions\dayzOffline.chernarusplus` da ese perfil y esa misión. Si el server tiene varios mapas, se cambia con **Cambiar de mapa**. Un `.bat` que pone el perfil o la misión en variables (`set`) se lee; una variable definida fuera del `.bat` se nombra, y ese mapa se elige a mano en **Cambiar de mapa**.
- Todo lo demás es el árbol de carpetas de siempre, y el editor muestra lo que encontró en **Dónde está cada cosa**. **Avanzado** es sólo para un server armado distinto.
- **Sólo lectura** (sin `Editor.bat`). Dentro de `Program Files`, en Firefox o en una copia del server, el navegador no da permiso de escritura: **Escanear server** lee la carpeta sin escribir (lo mismo que el botón **Leerla sin permiso de escritura**). Igual se elige la carpeta, nunca archivos sueltos. Cada archivo que el editor tenga que guardar se descarga, con su nombre y la carpeta adonde va.

### Server en un host

El editor corre en tu PC, no en el host. Se trabaja igual que con el Wizard de BZ AutoDrive: sobre una copia, y después se suben los archivos.

1. **Armá la copia en tu PC.** Bajá del host la carpeta del perfil (la de `-profiles`, con `BZ_AutoDrive`, `BZ_Trader` y `ExpansionMod`) y la de la misión, dentro de `mpmissions`. Ponelas en una carpeta, con el mismo árbol que en el host.
2. **Poné `@BZ_Trader` al lado,** en esa misma carpeta, y abrí `@BZ_Trader\tools\editor\Editor.bat`. El editor toma esa carpeta como el server.
3. **Elegí el mapa.** Una copia de un host no suele tener el `.bat` de arranque, así que el editor no sabe qué perfil y qué misión usa el server: elegilos en la lista.
4. **Armá el servicio y guardá.** **Guardar todo** lista cada archivo que escribió, con su carpeta.
5. **Subí esos archivos al host,** a las mismas carpetas. El server toma un servicio nuevo o cambiado sin reiniciar; lo que pide reinicio está en la sección 9.

### Paso 2 · La toma o el servicio

- **Nuevo desde una toma convertida:** se elige por su nombre, y el editor carga sus tres archivos.
- **Abrir un servicio del server**, uno que ya está ahí: su toma sale de su campo `Route`. El archivo de servicio guarda la sesión entera, así que es también el que volvés a abrir para cambiar lo que sea.
- Si falta un archivo, el editor dice cuál y dónde lo buscó.

### Paso 3 · Comerciantes y paradas

La ruta aparece como una cinta con el **Punto A**, un punto por cada marca NUMPAD 4 y el **FIN**. Hacé clic en un punto para editarlo.

- **Punto A:** cómo para (en las marcas, sólo cuando un jugador saluda con F1, o las dos), y la parada por saludo con su **Tocar bocina al salir de una parada por saludo**; las plazas del vehículo y **Quién maneja**; el personaje de cada comerciante (`ExpansionTraderAI…`), su ropa (un loadout o prendas sueltas) y qué vende (la facción es siempre `InvincibleObservers`: sección 7); y los jugadores cerca del vehículo.
- **Cada marca:** quién baja (también el que maneja, si lo marcás), y el plano de la parada (dónde queda cada puesto y hacia dónde mira cada comerciante); cuánto vende, el tope para volver a subir y **Tocar bocina al salir**; las señales. La sección 6 explica una parada en detalle.
- **FIN:** desaparece, reaparece a los N segundos o empieza de nuevo desde A, con una espera opcional en el último punto antes de eso.

**Qué vende** es una lista de los archivos de comerciante del Market. El editor muestra las categorías y los artículos de cada uno, pero no los edita. Un archivo que no sirve aparece en gris, con el porqué.

**Quién maneja** es una lista con los comerciantes del servicio: el vehículo lo maneja siempre uno de ellos. El elegido pasa a la plaza 0; si manejaba otro, intercambian plazas, y cada uno conserva sus paradas. El que maneja lleva la ropa de su ficha, como los demás. Un servicio nuevo arranca con el primer comerciante al volante, que vende desde el asiento hasta que lo marques para bajar en una parada. Un servicio de una versión anterior, con un chofer que no vendía (`Driver.IsTrader: 0`), no se puede guardar hasta que elijas quién maneja.

**Marcador en el mapa**. Un bloque propio en el punto A muestra el vehículo del servicio en el mapa, para que los jugadores lo distingan de otros servicios o de una misión. **Mostrar el vehículo en el mapa** lo prende (viene apagado); después, el **Texto del marcador** (vacío = el nombre del servicio), el **Color** (8 muestras, u **Otro color**) y **Actualizar cada (s)** (2 s al prenderlo, mínimo 1). El marcador es de Navigation de Expansion. Mientras el vehículo anda, BZ_Trader mueve ese mismo marcador: el ícono salta a la posición nueva cada **Actualizar cada (s)** segundos si el vehículo se movió 1 m o más, sin verse doble y sin desaparecer. No puede deslizarse como la flecha de un jugador: para eso haría falta un mod de cliente. En una parada titila, para que los jugadores sepan que están vendiendo (**Parpadear en las paradas**, prendido por defecto, y **Parpadeo: cada cuánto cambia (s)**, 2 por defecto; 0 lo deja fijo). Titila también en una parada por saludo; en la cabecera y al final de la ruta queda fijo. Cada cambio del parpadeo cuesta un envío, como un movimiento. Cada actualización le manda a cada jugador los settings del mapa de Expansion: unos 40 bytes más 100 por cada marcador de servidor del mapa. Al arrancar, el log del server dice cuántos hay (`marcadores: K marcadores de servidor en el mapa`); si son muchos, subí **Actualizar cada (s)**. Si son pocos, lo habitual, lo podés bajar a 1 s para que los saltos sean más cortos: el costo sigue siendo chico (con un solo marcador de servidor, unos 140 bytes por jugador y por actualización). La revisión avisa si la misión no tiene `expansion\settings\MapSettings.json` (sin Navigation) o si tiene `EnableServerMarkers: 0` (los jugadores no lo verían).

### El plano de la parada

El norte está arriba. El vehículo se dibuja girado como llegó a la marca, con su izquierda y su derecha, sus puertas y el lugar donde baja cada plaza. Arrastrá cada comerciante a su puesto, y arrastrá la punta de su flecha (o escribí los grados) para elegir hacia dónde mira. Mientras bajás por los campos de la parada, la cinta de la ruta queda arriba y el plano a la vista al costado.

| Control | Qué hace |
|---|---|
| Rueda del mouse | Acerca y aleja alrededor del puntero. Las fichas de los comerciantes mantienen su tamaño en pantalla. |
| Arrastrar el fondo | Mueve la vista sin mover nada. |
| Arrastrar la punta de una flecha | Gira el comerciante de a 5°. Con **Mayús**, de a 1°. |
| **Ajustar** | Encuadra el vehículo, los puestos y las señales. Cada parada abre así. |
| **Agrandar** | Pone el plano a toda la ventana. **Esc** vuelve. |
| Flechas | Mueven la ficha elegida 0,5 m. Con **Mayús**, 2 m. Con **Ctrl**, 0,1 m. |

| En el plano | Qué es |
|---|---|
| El círculo alrededor de un comerciante | Su alto: a esa distancia de un jugador se frena y lo mira mientras vende (`PlayerHaltM`). |
| Rombo | Por dónde sale de verdad esa plaza al bajar (medido por el mod). |
| `?` | La salida de esa plaza todavía no se midió: el mod la mide la primera vez que alguien baja por ahí. |
| Cuadrado | Por dónde sube ese comerciante a la vuelta. |
| La línea que rodea el vehículo | Su casco, medido por el mod. |

### Paso 4 · Revisar y guardar

- La revisión hace los mismos chequeos que el mod al leer el archivo, más lo que el editor sabe del server: un archivo del Market que falta, la zona, la reputación, un puesto muy lejos de su marca. Los errores no dejan guardar; los avisos sí.
- **Guardar todo** escribe el archivo de servicio, la tripulación en la ruta y la zona nueva. Antes respalda cada archivo que pisa en `_bz_trader_backups\`, y lista cada archivo con su carpeta.
- En la ruta cambia sólo la línea `DriverClass` y el bloque `Crew`, como texto. Nada más de la toma se toca.
- Con el server escaneado, el editor no abre ningún cuadro de archivos: todo sale de sus listas y se escribe directo en su carpeta.

---

## 6. Cómo es una parada

1. **Llegada.** El vehículo para en una marca NUMPAD 4. Si queda a más de 15 m de la marca (puede pasar después de que AutoDrive destraba un vehículo), no baja nadie y sigue. Las paradas por saludo son distintas: ver abajo.
2. **Bajada.** Bajan los comerciantes de `WhoGetsOff`, y se encienden las señales de la parada. El que maneja vende desde el asiento, salvo que la parada lo haga bajar también: ahí el vehículo espera en la parada, frenado, hasta que vuelve al volante. El chofer de una moto nunca baja (sin nadie arriba, se cae), ni el de un vehículo que todavía anda.
3. **Al puesto.** Cada comerciante camina a su puesto, rodeando el vehículo si el puesto queda del otro lado, caminando o trotando.
4. **Venta.** Durante `TimeS` segundos, mirando hacia donde elegiste. Si `MaxWithPlayersS` es mayor que 0 (por defecto es 0), la venta se estira mientras haya un jugador a menos de `PlayerRadiusM`, hasta ese tope. Con un jugador a menos de `PlayerHaltM`, el comerciante se frena y lo mira, y después vuelve a su rumbo. Camino al puesto o de vuelta no se frena.
5. **Subida.** Cada comerciante vuelve y sube desde un punto afuera del vehículo. El que no está sentado a los `BoardTimeoutS` se pasa directo a su asiento.
6. **Salida.** Si la parada tiene **Tocar bocina al salir**, el vehículo toca un toque de bocina y sale 2 s después. Las señales se apagan y se borran, y el vehículo sigue.

> **La regla de salida.** El vehículo no sale hasta que todos los comerciantes vivos están sentados de verdad, al arrancar y en cada parada. BZ_Trader vuelve a sentar al que no lo está. Si no puede dentro del tope, relanza la vuelta con tripulación nueva antes que salir con alguien a medio subir. Al que maneja se lo espera como a los demás. Los jugadores a bordo no cuentan. Al empezar una vuelta, el vehículo espera además unos segundos quieto: AutoDrive prepara la ruta recién cuando los comerciantes están sentados.

### Bocina al salir

Una opción de cada parada (`Stops[].HornOnDepart`) y otra de las paradas por saludo (`Wave.HornOnDepart`), apagadas por defecto. Es el aviso de que el vehículo arranca: un jugador parado enfrente tiene 2 s para correrse.

- Cuando la parada decide seguir, el vehículo toca un toque corto y sale 2 s después.
- Si en esos 2 s la parada vuelve a esperar (un comerciante que todavía sube, un jugador enfrente, una pausa del panel), la salida se cancela; cuando vuelve a decidir, toca otra vez.
- La salida de la cabecera no toca bocina: no es una parada.
- En una moto no hay bocina: sale sin ella, y el log lo dice una vez.
- La bocina es también ruido para la IA del juego: puede atraer infectados a la parada.

### La pausa del panel de AutoDrive

La pausa del panel de BZ AutoDrive (o NUMPAD .) detiene también el reloj del servicio.

- **En una parada o en el embarque de la cabecera,** la parada se congela: no cuenta el tiempo, las puertas quedan como están, los comerciantes siguen en sus puestos y el vehículo no sale. Las señales no se renuevan; el humo que ya estaba se apaga solo.
- **Al quitar la pausa,** todo sigue donde estaba, y la parada dura por lo menos 10 s más: el jugador que esperaba tiene tiempo de abrir el menú.
- **En marcha,** al vehículo lo frena AutoDrive y el servicio no cuenta el tiempo: un saludo vigente no vence.
- El log dice cuándo empieza y cuándo termina cada pausa (sección 10).

### Paradas por saludo

Con `StopMode: wave_only` o `marks_and_wave`, un jugador que saluda (F1) adelante del vehículo lo detiene en cualquier punto de la ruta. Ahí no hay puestos, y el terreno puede no dejar bajar, así que **nadie baja**: los comerciantes venden desde sus asientos, con sus puertas abiertas, durante `Wave.TimeS` segundos (más con jugadores cerca sólo si `MaxWithPlayersS` es mayor que 0), no se encienden señales y el vehículo sigue y las cierra. Una plaza que comparte la puerta con otra la abre una vez; en una moto no se abre ninguna. Las puertas se abren, pero el vehículo sigue cerrado: nadie sube ni abre su inventario.

- Con `marks_and_wave` el vehículo además hace sus paradas normales en las marcas, con los comerciantes en sus puestos.
- El saludo se nota hasta 150 m del vehículo (10 × `Wave.RadiusM`) y vale hasta que el vehículo para, pasa al jugador o se aleja, como mucho `Wave.MemoryS` (60 s).
- Se puede saludar desde el camino o desde la banquina, pegado a él. El vehículo empieza a frenar con tiempo para quedar con el frente unos 10-20 m antes del jugador, venga a la velocidad que venga: el jugador no tiene que calcularla. Mientras hay un saludo vigente revisa cuatro veces por segundo; si ya está demasiado cerca para frenar a tiempo, el saludo se ignora.
- Una parada por saludo es una frenada de emergencia: AutoDrive frena fuerte. En una recta el vehículo queda sobre su línea; rápido y en una curva puede deslizarse unos metros, y AutoDrive lo vuelve a encaminar (sección 11).
- Se ignora el saludo si la próxima parada en una marca está a menos de `Wave.MinGapM` por delante sobre la ruta (500 m; 50 m como mínimo, también con 0): el vehículo está por parar ahí.
- Después de cualquier parada, en una marca o por saludo, el vehículo tiene que recorrer `Wave.MinGapM` (500 m) antes de que un saludo lo vuelva a detener. Un grupo que saluda uno detrás del otro, o un jugador que sigue al vehículo, consigue una sola parada.
- Como mucho `Wave.MaxPerLap` (3) paradas por saludo por vuelta. Una vuelta va desde la cabecera hasta el final de la ruta, o hasta un relanzamiento.
- Después de cualquier parada, en una marca o por saludo, otro saludo puede detener el vehículo recién `Wave.CooldownS` (10 s) después de que sale.
- Cada saludo que no detiene el vehículo deja una línea en el log del server, con el porqué (sección 10).
- Una marca con `Mode: wave_only` para sólo si alguien saluda cerca de ella, y ahí los comerciantes bajan a sus puestos, como en cualquier marca.
- Comprarle desde afuera a un comerciante sentado se probó en el juego en una moto, y en un Land Rover con las puertas abiertas. En un vehículo cerrado (un bus, un APC) todavía no se probó si la opción de comerciar le llega desde afuera; en el bus, un jugador parado en el pasillo puede comprarles (probado en el juego).

### Hacia dónde miran

- Cada puesto puede tener un rumbo (`HeadingDeg`, el rumbo de DayZ: 0 = norte, en sentido horario) o un punto al que mirar (`LookAt: [x, z]`). Si están los dos, gana `LookAt`. Sin ninguno, el comerciante mira hacia donde llegó caminando.
- Al llegar se da vuelta y sostiene ese rumbo mientras vende. Después de mirar a un jugador cercano, vuelve a su rumbo.
- Atajos del editor: **En semicírculo mirando al centro** (radio, arco, y el centro es la cruz del plano, que se arrastra) y **Todos mirando a la calle**.
- El semicírculo pone a los que bajan del lado de afuera del arco, cada uno con `LookAt` al centro. Si después movés un puesto, ese comerciante sigue mirando al centro.

### Señales

Para que los jugadores vean una parada de lejos, por ejemplo una parada larga en un mercado que armaste, cada parada puede encender señales. Todos los tipos están probados en el juego, de día y de noche.

| `Type` | Qué se enciende | Dura |
|---|---|---|
| `auto` | Humo del color elegido de día, una bengala roja de noche. Lo decide el server cada vez que enciende una. | — |
| `smoke` | Granada de humo: `green`, `purple`, `red`, `white`, `yellow` o `black`. El juego no tiene humo azul: `blue` sale violeta. | 90 s |
| `roadflare` | Bengala de ruta roja en el piso. | 20 min |
| `fireworks` | Fuegos artificiales: 16 tiros, colores al azar. | ~75 s |
| `airdrop` | El humo rojo y la luz de un airdrop de Expansion (pide Expansion Missions, que viene en el Bundle). La caja va enterrada en el lugar de la señal y nadie la abre. Sin infectados, sin loot, sin marcador. Siempre rojo. | Hasta que sigue |

`flaregun`, de un archivo viejo, se lee como `roadflare`: el juego crea el proyectil de una pistola de bengalas sólo cuando alguien la dispara con el arma en la mano.

- Se encienden cuando bajan los comerciantes (o cuando el vehículo para, si no baja nadie), cada una a su `StartDelayS`. Cada `EverySeconds` se vuelve a encender la que se apagó, nunca dos en el mismo lugar: con `EverySeconds` menor que 75, los fuegos siguen toda la parada. `EverySeconds: 0` la enciende una sola vez. El airdrop no se apaga solo: dura hasta que el vehículo sigue.
- Cuando el vehículo sigue, todas se apagan y se borran. No quedan guardadas en el mundo, y los jugadores no las pueden levantar.
- **El humo de color y las bengalas se ven hasta unos 150 m.** Es lo lejos que DayZ les manda los objetos chicos a los jugadores (`networkRangeNear` en `serverDZ.cfg`, 150 m por defecto); más lejos, el juego del jugador no las tiene. Para encontrar el vehículo de lejos está el marcador en el mapa (sección 5).
- **Los fuegos artificiales y el airdrop se ven de lejos.** Los fuegos, como el humo del airdrop. El server le manda el airdrop a cada jugador, esté donde esté: su humo se vio a 744 m en una prueba, y se ve también de noche. Un jugador que entra al server durante la parada también lo recibe.
- Hasta 10 por parada. En el editor, **+ Agregar señal** pone una a 10 m a la derecha del vehículo, y **3 señales automáticas alrededor de la parada** pone verde, amarilla y violeta en triángulo a 15 m de la marca, fuera de la ruta, y reemplaza las que hubiera.
- **La posición es tuya.** El mod nunca mueve una señal. El humo al lado o debajo del vehículo funciona y queda bien; adentro de un edificio puede no verse. Si hace falta, movela en el editor.

### Jugadores cerca, o secuencia fija

Por defecto la parada dura lo mismo haya o no jugadores (`MaxWithPlayersS: 0`). Mientras vende, el comerciante se frena y mira a un jugador cercano (`PlayerHaltM`); eso no demora la salida, porque el tiempo de venta corre igual. Con `MaxWithPlayersS` mayor que 0 la venta se estira con jugadores cerca, hasta ese tope, y ahí cualquier jugador puede demorar la salida. El botón **Secuencia fija (sin esperar ni frenar por jugadores)** del editor pone `MaxWithPlayersS: 0` y `PlayerHaltM: 0`: cada parada dura su mínimo y nadie se frena por jugadores. No cambia quién puede comerciar (lo decide el Market por distancia), el saludo ni la regla de salida.

**Un jugador parado enfrente.** Viene apagado (`FrontPlayerWaitS: 0`): el aviso de que el vehículo arranca es la **Bocina al salir**. Si lo prendés, antes de salir de una parada o de la cabecera, con un jugador justo delante del vehículo (hasta 12 m adelante, dentro de su ancho más 1 m por lado), el vehículo espera a que se corra, como mucho `FrontPlayerWaitS` segundos, y después sale igual. Sirve porque arrancar contra un jugador parado enfrente lo empuja, y AutoDrive lo toma como un choque. Un jugador al costado, o sentado en el vehículo, no cuenta.

### Paradas largas en un mercado armado

Diez minutos es `TimeS: 600` por comerciante. Un puesto o una señal a más de `MaxStallDistanceM` (25 m por defecto) de su marca sólo da un aviso, en el log y en el editor. Para un mercado grande, subilo. La zona de comercio crece sola hasta el puesto más lejano.

---

## 7. Vehículos, plazas y puertas

- Sirve cualquier vehículo que BZ AutoDrive pueda manejar: autos, camionetas, buses y, en la DayZ 1.30, motos.
- La plaza 0 es el chofer, y el chofer es siempre uno de los comerciantes. Los comerciantes son el chofer y la tripulación de la ruta (`DriverClass` y `Crew`): AutoDrive los crea, BZ_Trader engancha a cada uno con su archivo del Market y lo sienta. El editor los escribe en la ruta por vos.
- La primera vez que el mod ve una clase de vehículo, escribe `profiles\BZ_Trader\_plazas\<clase>.json`: el casco, dónde se sienta cada plaza, su puerta y, cuando bajan, por dónde sale de verdad cada plaza. El editor lo lee para saber cuántas plazas hay y dibujarlas en su lugar, con el casco, las puertas y las salidas.
- **Un vehículo nuevo, en dos pasadas por el editor.** Antes del primer lanzamiento, el editor dibuja las plazas aproximadas, y lo avisa: alcanza para elegir quién va en cada plaza. Guardás, el server lanza el servicio solo, sin que haga falta un jugador conectado, y en ese lanzamiento mide el vehículo. Volvé a escanear el server y las plazas quedan donde se sientan. Es una sola vez por clase de vehículo.
- A pie, cada comerciante sale de donde bajó. Desde adentro del casco (el pasillo de un bus) va derecho a su puesto. Desde afuera, con el puesto del otro lado (la compuerta trasera de un APC), rodea por la esquina más cercana. Para subir va a un punto afuera del casco; dos plazas con la misma puerta suben de a una. Las puertas se cierran al sentarse y antes de salir.

| Vehículo probado | Plazas | En el juego |
|---|---|---|
| Moto (DayZ 1.30) | El chofer es el comerciante y vende desde el asiento. | Probado |
| `ExpansionUAZRoofless` | Maneja un comerciante; otros tres en las plazas 1 a 3. | Probado, con un chofer que no vendía (una versión anterior) |
| `ExpansionBus` | 22 plazas. Cada plaza baja al pasillo, adentro del bus, y los comerciantes salen derecho a sus puestos. | Probado con 6 comerciantes |
| APC con compuerta trasera | 6 plazas: 0 y 1 por las puertas laterales, 2 a 5 por la compuerta de atrás. | Probado: desde la compuerta, los comerciantes rodean el vehículo hasta un puesto del otro lado, y a veces lo trepan |

### Jugadores a bordo

- **Los vehículos de los servicios van siempre cerrados a jugadores:** nadie se sienta (ni al volante ni de pasajero), nadie abre las puertas ni toca el inventario; los comerciantes y el chofer suben igual. Es el cierre de vehículos de Expansion (viene con su módulo **Vehicles**, parte del Bundle). Sin ese módulo, el server igual niega los asientos: el jugador puede ver la acción, pero no pasa nada.
- **Viajar parado:** en un vehículo donde se puede ir parado (la caja de un camión, el techo, el interior de un bus), un jugador puede viajar igual. Es una limitación del vehículo, y cada admin elige qué vehículo usa.
- **Nunca se retira un vehículo con un jugador a bordo** (fin de la vuelta, `Enable: 0`, relanzamiento). Al que va parado arriba o adentro se lo pone en el piso, al costado, y el vehículo se retira. Si alguien quedó sentado (no debería poder), el servicio sigue con un vehículo nuevo y el viejo espera frenado y cerrado hasta que se baje. Sin mensajes en el chat: lo dice el log del server (sección 10). Si el server se cierra con un jugador a bordo, el vehículo no se retira; tampoco se guarda, así que no vuelve.
- `PlayersCanBoard` ya no se usa: si un servicio lo trae, el log lo dice y el editor lo quita al guardar.

### Manos vacías

Los comerciantes caminan, venden y viajan siempre con las manos vacías, cualquiera sea su loadout. Algunos loadouts de fábrica de Expansion traen una pala, un pico o un bate en la espalda, o una pistola en la mano. BZ_Trader no deja que eAI los pase a la mano, y guarda lo que ya tengan: en la espalda, en el hombro, en el inventario o, si no entra en ningún lado, se borra.

### A los comerciantes no se los puede matar

BZ_Trader hace invencibles a todos los comerciantes, también al que maneja, arriba y a pie, y lo revisa cada segundo. Es a propósito: el vehículo tiene que terminar su ruta, y un chofer muerto lo dejaría detenido en el camino. Pelear por un vehículo es tarea de un mod de convoyes; BZ_Trader es comercio. (En Traveling Trader Expansion los comerciantes se pueden matar y saquear; en BZ_Trader, no.)

- El que maneja depende también de BZ AutoDrive: dejá prendido su `DriverInvincible` (viene así), y no uses el verbo de ruta `set_driver_mortality` en una ruta de BZ_Trader.
- **Si alguna vez un servicio se traba,** por el motivo que sea, tocá **Detener** sobre su vehículo en el panel de BZ AutoDrive: es el botón rojo. El vehículo se retira y BZ_Trader vuelve a lanzar el servicio, desde el principio, a los 60 s. Para sacar un servicio de circulación, en cambio, usá `Enable: 0` (sección 9).

### Lo que un jugador no les puede hacer

Las protecciones valen sólo para los comerciantes de BZ_Trader: los comerciantes fijos de Expansion y los de otros mods no cambian. Están adaptadas de Traveling Trader Expansion, con permiso de FUBAR.

- **No se los puede reclutar.** Todos los comerciantes, también el que maneja, son de la facción `InvincibleObservers` (la misma que Expansion usa para sus comerciantes IA), diga lo que diga el archivo. Con esa facción, «Reclutar» ni aparece. Despedirlos o abrir su inventario pide reclutarlos antes, así que tampoco se puede.
- **No se los puede esposar, amordazar, encapuchar, alimentar ni atender:** esposas, cuerda o cinta, capucha, comida o bebida, vendas, inyecciones, sangre, suero, RCP, pulso, coser, entablillar, sacar sangre, medir la temperatura.
- **Qué ve el jugador.** Con las manos vacías, la única opción sobre un comerciante es **Comerciar** (probado en el juego): Expansion esconde lo que no se puede hacer sobre alguien de una facción invencible, también **Comprobar pulso**. Con una venda en la mano, la única opción sigue siendo **Comerciar** (probado en el juego): vendar se ofrece sólo a alguien que sangra, y los comerciantes no sangran. Con un objeto en la mano, Expansion esconde además dar de comer o de beber, sacar o dar sangre, las inyecciones y el desfibrilador. Si alguna otra acción con un objeto llegara a aparecer (suero, capucha, esposas…), al elegirla no pasa nada: BZ_Trader es un mod de servidor y esa protección corre en el server. No hay animación y el objeto no se gasta.
- **La IA los trata igual:** zombis, animales, bandidos, guardias y la IA de misiones no los atacan. Por ser de una facción invencible, eAI no les esquiva el gas: si un puesto o un camino pasa por un área contaminada, caminan igual. Como son invencibles, no les hace nada.
- Las rutas del comerciante no usan el verbo `guard_stop` de AutoDrive: con esta facción, los que bajaran no pelearían.

---

## 8. El archivo de servicio

Un archivo por servicio en `profiles\BZ_Trader\`. Es el que escribe y vuelve a abrir el editor; también se puede editar a mano. Las claves están en inglés, y donde un campo significa lo mismo que en una ruta de Traveling Trader Expansion, tiene el mismo nombre y formato. Un campo que falta toma un valor seguro, y el log del server dice cuáles lo tomaron.

Un ejemplo corto, con dos comerciantes y una parada: Mirek maneja y vende desde el asiento, y Denis baja a vender a un puesto:

```json
{
  "Enable": 1,
  "Name": "TRADER_PICKUP_01",
  "Route": "BZBusRoute_TRADER_PICKUP_01.json",
  "VehicleClass": "ExpansionUAZRoofless_LightGreen",
  "StopMode": "marks",
  "Driver": { "IsTrader": 1, "AllCanDrive": 1 },
  "TraderZone": { "Name": "BZT_TRADER_PICKUP_01", "FollowsVehicle": 1, "RadiusM": 5.4 },
  "Traders": [
    { "Seat": 0, "Name": "Mirek", "TraderAIClass": "ExpansionTraderAIMirek",
      "Loadout": "SurvivorLoadout", "TraderFile": "Consumables" },
    { "Seat": 1, "Name": "Denis", "TraderAIClass": "ExpansionTraderAIDenis",
      "Loadout": "SurvivorLoadout", "TraderFile": "Medicals" }
  ],
  "Stops": [
    { "Mark": 1, "Pos": [6381.3, 8085.7], "Mode": "always",
      "WhoGetsOff": [1], "BoardTimeoutS": 20, "HornOnDepart": 1,
      "Traders": [
        { "Seat": 1, "TimeS": 120, "MovementSpeed": "WALK", "HeadingDeg": 125,
          "Waypoints": [ { "WayPointName": "M1-1", "Position": [6382.3, 0, 8090.5] } ] }
      ],
      "Signals": [
        { "Type": "auto", "Color": "green", "Pos": [6370.0, 8095.0], "EverySeconds": 60, "StartDelayS": 0 }
      ] }
  ],
  "OnRouteEnd": { "Action": "restart", "DelayS": 0, "WaitS": 0 }
}
```

### Primer nivel

| Campo | Si falta | Qué es |
|---|---|---|
| `Enable` | 0 | 1 = en servicio. |
| `Name` | nombre del archivo | El nombre del servicio. Firma sus líneas del log. |
| `Route` | obligatorio | El archivo de ruta de AutoDrive, en `profiles\BZ_AutoDrive\`. |
| `VehicleClass` | — | Informativo: se compara con el vehículo de la ruta y avisa si no coinciden. |
| `StopMode` | `marks` | `marks`, `wave_only` o `marks_and_wave`: para en las marcas, sólo por saludo (nadie baja), o las dos. |
| `Wave` | apagado | Parar cuando un jugador saluda. Ver abajo. |
| `Driver` | — | `IsTrader`: 1 = maneja el comerciante de la plaza 0 (y vende). 0 es sólo para archivos de una versión anterior: maneja el chofer de la ruta, que no vende, y el log avisa. `AllCanDrive` (1): reservado, todavía sin efecto. |
| `TraderZone` | — | `Name` (la zona del Market; vacío = la zona del lugar), `FollowsVehicle` (0), `RadiusM` (5 como mínimo). |
| `Traders[]` | — | Quién viaja. Ver abajo. |
| `Stops[]` | no para | Una por marca NUMPAD 4. Ver abajo. |
| `OnRouteEnd` | `despawn` | `Action`: `despawn` (desaparece), `respawn` (vuelve a los `DelayS`) o `restart` (empieza de nuevo desde A). `WaitS`: una espera opcional en el último punto antes. |
| `StartDelayS` | 0 | Segundos antes del primer lanzamiento. |
| `Markers` | sin marcador | Un marcador de mapa, con el formato de Traveling Trader Expansion: `useMapMarker` (0), `MarkerUpdateSeconds` (30, mínimo 1; el editor pone 2), `MarkerLabel` (el nombre), `MarkersIcon` (`Trader`), `Expansion3DMarkers` (0), `Red`, `Green`, `Blue`, `Alpha` (`"255"`, como texto). BZ_Trader suma `BlinkAtStops` (1: titila en las paradas; 0 = fijo) y `BlinkSeconds` (2: cada cuánto cambia; 0 = fijo también en las paradas). |
| `RespawnSeconds` | 600 | Una red de seguridad: si alguna vez muere el que maneja (los comerciantes son invencibles), segundos hasta que el servicio vuelve. Menos de 0 = nunca. |
| `PlayerRadiusM` | 8 | La distancia que cuenta como jugador cerca. |
| `MaxWithPlayersS` | 0 | Lo más que se estira una parada con jugadores cerca. 0 = no se estira (un jugador no puede demorar la salida). |
| `SeatedWaitS` | 20 | La espera mínima en una parada donde nadie baja (el que maneja vende desde el asiento). |
| `PlayerHaltM` | 5 | Mientras vende, a cuántos metros de un jugador el comerciante se frena y lo mira. 0 = nunca. |
| `FrontPlayerWaitS` | 0 | Antes de salir, cuánto espera el vehículo a que se corra un jugador parado justo enfrente (sección 6). 0 = no espera: el aviso es la bocina al salir. |
| `MaxStallDistanceM` | 25 | Un puesto o una señal más lejos que esto de su marca da un aviso. |
| `Editor` | — | Lo que el editor necesita para rearmar la sesión. El mod lo ignora. |

### Traders[]

| Campo | Si falta | Qué es |
|---|---|---|
| `Seat` | obligatorio | La plaza. 0 es el chofer (con `Driver.IsTrader: 1`). Sin ella, la entrada se ignora. |
| `Name` | comerciante de la plaza N | El nombre que muestra el log. |
| `TraderAIClass` | — | El personaje, una clase `ExpansionTraderAI…`. AutoDrive crea el bot desde el `Crew` de la ruta; este campo se verifica contra eso. |
| `Loadout`, `Clothing[]` | — | La ropa: un loadout de Expansion, o una lista de prendas. |
| `TraderFile` | — | El archivo de comerciante del Market con el que vende, sin `.json`. |
| `Faction` | — | Se ignora: la facción es siempre `InvincibleObservers` (sección 7), y el log lo dice si el archivo pide otra. El editor la saca al guardar y escribe `InvincibleObservers` en la tripulación de la ruta. |
| `LootingBehaviour` | el de TTE | Sin efecto: los comerciantes van pasivos y no levantan nada. Queda por compatibilidad con el formato de Traveling Trader Expansion. |

El valor por defecto de `LootingBehaviour` es el de Traveling Trader Expansion: `"BANDAGES | FOOD | CLOTHING | UPGRADE"`.

`Invincible` y `CanBeLooted`, de archivos viejos, se ignoran: los comerciantes son siempre invencibles (sección 7). El log lo dice, y el editor los saca al guardar.

### Stops[]

| Campo | Si falta | Qué es |
|---|---|---|
| `Mark` | el orden | Qué marca NUMPAD 4, contando desde 1. |
| `Pos` | — | `[x, z]` de la marca, para verificar el archivo contra la toma. |
| `HeadingDeg`, `DistM` | — | El rumbo del vehículo al llegar y la distancia sobre la ruta. Los escribe el editor. |
| `Mode` | `always` | `always`, `wave_only` u `off`: para siempre, sólo si alguien saluda a menos de `WaveRadiusM` (20) de la marca, o nunca. |
| `WhoGetsOff[]` | `[]` | Las plazas que bajan acá. Puede incluir la 0, el que maneja: el vehículo lo espera (no en una moto). |
| `BoardTimeoutS` | 60 | Segundos para volver al asiento antes de pasarlo directo. Mínimo 5. |
| `HornOnDepart` | 0 | 1 = un toque de bocina al salir de esta parada, y sale 2 s después (sección 6). |
| `Traders[]` | — | Una entrada por cada comerciante que baja. Ver abajo. |
| `Signals[]` | ninguna | `Type` (`auto`, `smoke`, `roadflare`, `fireworks` o `airdrop`; por defecto `auto`), `Color` (`green`), `Pos` `[x, z]` (la altura, del terreno), `EverySeconds` (60), `StartDelayS` (0). Hasta 10 por parada. |

Cada entrada de `Stops[].Traders[]`:

| Campo | Si falta | Qué es |
|---|---|---|
| `Seat` | — | Qué comerciante. |
| `Waypoints[]` | ninguno | `WayPointName` y `Position` `[x, y, z]`. El último es el puesto. Sin ninguno, el comerciante vende donde bajó. |
| `TimeS` | 60 | Segundos vendiendo. |
| `MovementSpeed` | `WALK` | `WALK` o `JOG` (`RUN` cuenta como `JOG`). |
| `HeadingDeg`, `LookAt` | — | Hacia dónde mira el comerciante en su puesto (sección 6). |

### Wave

| Campo | Si falta | Qué es |
|---|---|---|
| `Enabled` | 0 | 1 = el saludo de un jugador (F1) detiene el vehículo. El de un bot no cuenta. |
| `RadiusM` | 15 | La escala del saludo: se nota a menos de 10 × `RadiusM` del vehículo, y termina cuando el vehículo llegó a menos de 3 × `RadiusM` y dejó al jugador atrás. Dónde para no depende de este valor: frena para quedar antes del jugador. |
| `AheadOnly` | 1 | Sólo un jugador adelante del vehículo. |
| `CooldownS` | 10 | Después de que el vehículo sale de una parada (en una marca o por saludo), segundos antes de que un saludo lo pueda volver a detener. |
| `MinGapM` | 500 | Después de cualquier parada, los metros que el vehículo tiene que recorrer antes de que un saludo lo vuelva a detener. También se ignora el saludo si la próxima parada en una marca está así de cerca sobre la ruta (50 m como mínimo). 0 = sin mínimo. El editor avisa si el valor deja la ruta sin ningún tramo donde el saludo pueda valer. |
| `MaxPerLap` | 3 | Lo más de paradas por saludo en una vuelta (de la cabecera al final de la ruta, o a un relanzamiento). 0 = sin tope. |
| `TimeS` | 60 | Cuánto dura una parada por saludo. Los comerciantes venden desde sus asientos, con sus puertas abiertas. |
| `MemoryS` | 60 | Lo más que vale un saludo. Termina antes si el vehículo para, pasa al jugador o se aleja. |
| `HornOnDepart` | 0 | 1 = un toque de bocina al salir de una parada por saludo, y sale 2 s después. |

---

## 9. Cambios sin reiniciar

| Cambiás | Qué pasa |
|---|---|
| Un archivo de servicio | El server los relee cada 10 s. Uno nuevo se lanza. Uno cambiado se aplica en el próximo lanzamiento de ese servicio, nunca con comerciantes abajo. Con `Enable: 0`, o borrado, el servicio se retira al terminar la parada en curso. |
| La ruta (su tripulación) | AutoDrive la relee en cada lanzamiento. |
| Un archivo de comerciante nuevo cuyas categorías ya existen | Se carga sin reiniciar. Si los jugadores no ven la opción de comerciar, reiniciá. |
| Una zona de comercio nueva | Se carga sin reiniciar. Si un servicio nombra una zona que no existe (un archivo escrito a mano, por ejemplo), BZ_Trader la crea en la misión sin stock, y el Market la llena con los artículos de cada comerciante. Comprar en una zona creada así no se probó en el juego. |
| Una categoría nueva del Market, precios, stock máximo, o un archivo de comerciante ya cargado | **Reiniciá el server.** El editor lo avisa al guardar. |

---

## 10. Leer el log del server

Cada línea empieza con `[BZ_Trader] [<servicio>]`; los avisos con `[BZ_Trader][WARN]` y los errores con `[BZ_Trader][ERROR]`. Para seguir un servicio, buscá su nombre en el RPT. Esta tabla explica las líneas que más vas a ver.

| La línea dice | Quiere decir |
|---|---|
| `por defecto (no escritos en …): …` | Los campos que faltan en el archivo y tomaron su valor por defecto. |
| `APAGADO (Enable 0)` | El servicio está apagado. |
| `embarque: … SALE tras N s` | Todos los comerciantes vivos sentados en la cabecera: el vehículo sale. |
| `soltado hace N s (controlador armado hace N s) y se movio 0 m` | El vehículo se soltó, pero no se movió en los 10 s después de que AutoDrive estuvo listo. Es sólo un diagnóstico: si se repite, leé las líneas de AutoDrive de ese vehículo. |
| `soltado hace N s y AutoDrive todavia no armo el controlador (…; tope 60 s)` | AutoDrive no estaba listo a los 60 s de soltarlo. Es sólo un diagnóstico. |
| `PARADA (…): … despliegue N/M` | Detenido en una parada; bajaron N de los M comerciantes. |
| `PARADA (…): nadie baja` | Acá no baja nadie; el vehículo espera `SeatedWaitS`. |
| `… NO BAJA · …` | Un comerciante no bajó, con el motivo. |
| `se lo sienta por TELEPORT` · `SENTADO (teleport)` | Un comerciante no volvió a tiempo (`BoardTimeoutS`) y se lo pasó al asiento. |
| `FANTASMA` | Un comerciante anotado en una plaza sin estar sentado de verdad. BZ_Trader lo vuelve a sentar donde está. |
| `embarcados N de M vivos · muertos K` | Cuántos comerciantes vivos volvieron a su asiento. |
| `señales: N encendidas` · `… apagadas y borradas` | Las señales de la parada encendidas; después, borradas al salir. |
| `señal N NO SE CREO` | Una señal no se pudo crear. Se reintenta. |
| `señales: 1 encendidas (humo de airdrop (…) … creado en los clientes)` · `humo de airdrop: N hasta que sigue (no se renueva)` | Una señal `airdrop`: el server le mandó a cada jugador el humo del airdrop. Dura hasta que el vehículo sigue. |
| `PARADA (…): bocina al salir · suelta en 2 s` | El vehículo tocó la bocina y sale 2 s después. |
| `bocina al salir: se cancela (la parada volvio a esperar) · se vuelve a decidir` | En esos 2 s la parada volvió a esperar: cuando vuelva a decidir, toca otra vez. |
| `bocina al salir: el vehiculo no es un auto (una moto) … sale sin bocina` | Una moto no tiene bocina. Sale una vez por servicio. |
| `PAUSA del panel de AutoDrive · … se congela` · `pausa quitada tras … · sigue donde estaba, no antes de 10 s` | Un admin pausó el vehículo desde el panel de AutoDrive en una parada o en la cabecera, y después quitó la pausa (sección 6). En marcha dice `en ruta: PAUSA del panel de AutoDrive`. |
| `· faccion InvincibleObservers (el archivo dice … : se ignora)` | Al cargar un servicio: el archivo pide otra facción, y se ignora (sección 7). |
| `<comerciante>: faccion InvincibleObservers (su grupo era Passive; cambia para todo el grupo)` | En cada lanzamiento: BZ_Trader le pone la facción al grupo del que maneja. |
| `jugador <nombre> (entro hace 2 s): datos de N comerciante(s) enviados` | A un jugador que entra al server se le mandan los datos de los comerciantes, a los 2 s y a los 8 s, para que le aparezca **Comerciar**. |
| `parada (…): datos de N comerciante(s) reenviados a J jugador(es) a <= 500 m` | En cada parada se reenvían los datos de los comerciantes a los jugadores cercanos, por las dudas. |
| `COMERCIO en la zona '…'` | Un jugador compró o vendió. |
| `cierre: vehiculo cerrado para jugadores (…)` | El vehículo quedó cerrado para los jugadores al aparecer. |
| `… parado arriba o adentro de <clase>: se lo pone en el piso, al costado` | Al retirar el vehículo (fin de la vuelta, `Enable: 0`, relanzamiento): a un jugador parado arriba o adentro se lo puso en el piso, al costado. |
| `… a bordo -> el vehiculo NO se retira: queda frenado …` · `desalojo: vacio de jugadores tras … -> UnregisterRunner hecho` | Alguien seguía sentado cuando tocaba retirar el vehículo: esperó frenado y cerrado, y se retiró recién vacío (sección 7). |
| `VEHICULO PERDIDO` | El vehículo ya no está en el mundo, por ejemplo porque un admin lo detuvo desde el panel de AutoDrive. El servicio se vuelve a lanzar a los 60 s. |
| `PARADA (marca N): el vehiculo esta a X m de la marca … no se baja nadie y sigue` | El vehículo paró a más de 15 m de su marca (puede pasar después de que AutoDrive destraba un vehículo): no baja nadie. |
| `PARADA por saludo de <jugador> · … nadie baja: venden sentados` | Un jugador saludó: el vehículo para y los comerciantes venden desde sus asientos. |
| `PARADA (…): puertas abiertas para vender: N (plazas …)` | En una parada por saludo: cuántas puertas se abrieron y de qué plazas. Con el cierre puesto agrega `· con el cierre de Expansion puesto (nadie sube ni toca el inventario)`. Un vehículo sin puertas de plaza (un bus) dice `sin puertas de plaza que abrir (venden sentados igual)`. |
| `saludo de <jugador> ignorado: la proxima parada marcada (marca N) esta a X m (MinGapM …)` · `… (va a parar ahi)` | La próxima parada en una marca está más cerca que `MinGapM` (o 50 m): el vehículo para ahí. |
| `saludo de <jugador> ignorado: paro hace X m (MinGapM …: faltan … m)` | Muy cerca de la última parada: el vehículo todavía no recorrió `MinGapM`. |
| `saludo de <jugador> ignorado: ya hizo N paradas por saludo en esta vuelta (MaxPerLap N)` | La vuelta ya tuvo sus paradas por saludo. |
| `parada por saludo N de la vuelta M (MaxPerLap N)` | Una parada por saludo, contada contra el tope de la vuelta. |
| `vuelta N: K parada(s) por saludo (MaxPerLap N)` | Al empezar una vuelta: cuántas paradas por saludo tuvo la anterior. |
| `sena de <jugador> anotada a X m del vehiculo · …` | Se anotó el saludo de un jugador. Vale hasta que el vehículo para, pasa al jugador o se aleja (como mucho `MemoryS`). |
| `saludo de <jugador> ignorado: …` | Un saludo que no detuvo el vehículo, con el porqué: fuera del alcance, la espera entre saludos, muy cerca de la última parada (`MinGapM`), el tope de la vuelta (`MaxPerLap`), el vehículo pasó al jugador o se alejó, venció `MemoryS`, el jugador no estaba adelante, o el vehículo ya estaba parado. Una línea por saludo. |
| `saludo de <jugador> atendido por la parada en marca N` | El vehículo paró en una marca antes de llegar al jugador: esa parada atendió el saludo. |
| `SENA de <jugador> a X m · adelante · parada por saludo en wp N (V km/h, frena con Y m de frenado)` | El vehículo empezó a frenar por un saludo. |
| `parada por saludo: el frente quedo a X m de <jugador>` | Dónde quedó el frente del vehículo, contra los 10 m que busca (negativo = pasó al jugador). |
| `… ignorado: no alcanza a frenar antes del jugador (a X m, necesita Y m)` | El vehículo ya estaba demasiado cerca para frenar antes del jugador. |
| `jugador <nombre> enfrente a X m: espera que se corra (tope N s)` · `se corrio a los N s` · `sigue enfrente: sale igual tras N s` | Un jugador estaba enfrente cuando el vehículo iba a salir (sólo con `FrontPlayerWaitS` mayor que 0). |
| `marcadores: K marcadores de servidor en el mapa (MapSettings) · cada envio ~N bytes por jugador` | Al arrancar: cuántos marcadores de servidor tiene el mapa, y cuánto cuesta cada actualización del marcador por jugador. |
| `marcador: N envios en la vuelta (…)` | Al terminar una vuelta: cuántas actualizaciones del marcador se mandaron. Multiplicadas por el costo de una (la línea `marcadores:` del arranque), da lo que cuesta una vuelta por jugador. La cantidad de marcadores es la del último envío de la vuelta. |
| `TraderZone '…': ya cargada por el Market · se asigna` · `no estaba cargada · se carga del archivo` | Cómo se encontró la zona de comercio del servicio. |
| `cierre del server: N señales borradas` | Las señales de la parada se borraron al cerrar el server. |
| `Wave.WhoGetsOff ya no se usa` | Un campo viejo del archivo: se ignora, porque en una parada por saludo nadie baja. |
| `Invincible/CanBeLooted ya no se usan` | Campos de un archivo viejo: se ignoran, porque los comerciantes son siempre invencibles. |
| `PlayersCanBoard ya no se usa` | Un campo viejo: se ignora, porque los vehículos de los servicios van siempre cerrados a jugadores. El editor lo quita al guardar. |
| `Driver.IsTrader = 0: maneja el DriverClass de la ruta, que no vende …` | Un archivo de una versión anterior: maneja el chofer de la ruta, que no vende. Abrilo en el editor y elegí quién maneja. |
| `plaza 0 (…): BAJA · … es el que maneja: el vehiculo espera …` | El que maneja baja en esta parada; el vehículo lo espera, frenado, hasta que vuelve. |
| `0(no es un auto: …)` · `0(el vehiculo todavia anda: …)` | El que maneja estaba marcado para bajar pero queda sentado: es una moto, o el vehículo todavía andaba. |
| `marcador: parpadea en la parada (…)` · `marcador: fin del parpadeo (…)` · `fijo otra vez` | El marcador del mapa titila durante una parada y después queda fijo otra vez. |
| `manos vacias · <objeto> al hombro` · `a la espalda` · `al inventario` | Se guardó algo que un comerciante tenía en la mano: en la espalda, en el hombro, en el inventario, o se borró si no entraba. Una vez por objeto. |
| `eAI le VOLVIO a sacar …` | eAI le volvió a pasar el objeto a la mano. No debería pasar: avisá. |
| `el cierre no quedo puesto (falta el modulo Vehicles de Expansion?)` | Falta el módulo Vehicles de Expansion: los jugadores pueden abrir las puertas y el inventario, pero igual no pueden sentarse. |
| `FIN del servicio · …` | El servicio terminó, con el motivo. |
| `no se pudo leer …` | No se pudo leer un archivo de servicio (casi siempre, un JSON roto). Corregí el archivo; el server lo relee en 10 s. |

---

## 11. Problemas y límites conocidos

| Qué ves | Por qué, y qué hacer |
|---|---|
| No arranca ningún servicio, y el log no muestra ninguno de BZ_Trader. | En la DayZ 1.30, un `-profiles` con ruta absoluta no deja que el mod liste sus archivos de servicio. Usá una relativa, como `-profiles=profiles`. |
| El editor (o la ventana de `Editor.bat`) dice «BZ_Trader no está en la raíz de un server de DayZ». | Lo abriste desde la carpeta donde lo baja Steam (ahí la carpeta del mod se llama con un número). Copiá `@BZ_Trader` a la carpeta del server, al lado de `DayZServer_x64.exe`, y abrí el editor desde ahí (sección 2). |
| El editor no puede guardar. | El server está dentro de `Program Files`, o el navegador es Firefox. Usá el modo de sólo lectura y copiá los archivos descargados a las carpetas que indica, o mové el server. |
| Un artículo no aparece o no se puede comprar. | La reputación de la Hardline, o el artículo no está en ninguna categoría del archivo de comerciante (sección 4). |
| La botella de agua llega vacía. | Así se vende el artículo vanilla. Sumá latas para bebidas listas. |
| Un comerciante tarda en subir, o aparece de golpe en el asiento. | A los `BoardTimeoutS` se pasa al comerciante al asiento (`se lo sienta por TELEPORT` en el log). Un puesto del lado opuesto a la puerta alarga la vuelta. |
| Una señal no se ve de lejos. | El humo de color y las bengalas son objetos chicos: DayZ se los manda sólo a los jugadores a menos de unos 150 m (`networkRangeNear`). Para que se vea de lejos, usá el `airdrop` o los fuegos artificiales; para encontrar el vehículo, el marcador en el mapa. |
| Un jugador elige sobre un comerciante una acción con un objeto en la mano (esposas, una venda) y no pasa nada. | Es lo esperado: las protecciones corren en el server (sección 7). |
| La bocina atrae infectados. | Es ruido para la IA del juego, como cualquier bocina. Si molesta, apagala en esa parada. |
| Una señal no se ve (adentro de una pared o de un edificio). | La posición de las señales es tuya; el mod nunca las mueve. Movela en el editor. El humo al lado o debajo del vehículo funciona bien. |
| Un jugador se sube a la caja, al techo o al bus y viaja. | Es lo esperado: no se puede impedir. Al retirar el vehículo, se lo pone en el piso, al costado (sección 7). |
| Los jugadores pueden abrir las puertas o el inventario del vehículo. | Falta el módulo Vehicles de Expansion (viene con el Bundle). Igual no pueden sentarse. El log dice `el cierre no quedo puesto`. |
| Usé el panel de AutoDrive (**Detener**, **Respawn**) sobre un vehículo de BZ_Trader y el servicio volvió. | Es lo esperado: BZ_Trader relanza el servicio solo. **Detener** es el botón rojo para un servicio trabado (sección 7). Para sacar un servicio de circulación o volver a ponerlo, usá `Enable` en su archivo (o en el editor). |
| El vehículo viejo queda frenado al final de la ruta y el servicio sigue con uno nuevo. | Alguien sigue sentado adentro, y no debería pasar: a los jugadores se les niegan los asientos. Se retira cuando se baja (sección 7). |
| Errores `GetTraderFromObject … is NULL!` cuando un servicio se relanza con un jugador cerca. | Inofensivos. El juego del jugador pidió los datos del comerciante un instante antes de que BZ_Trader lo enganchara al Market; BZ_Trader se los manda apenas lo engancha, y comerciar anda. |
| En una parada por saludo, el vehículo se sale de su línea y tarda en seguir. | La parada por saludo frena fuerte (la frenada de emergencia de AutoDrive). En una recta queda sobre su línea; rápido y en una curva puede deslizarse unos metros, y AutoDrive lo vuelve a encaminar. Una parada más suave necesita una versión posterior de BZ AutoDrive. |
| Un aviso de un puesto demasiado lejos. | Es sólo un aviso. Para un mercado grande, subí `MaxStallDistanceM`. |

### Todavía no

- **Convoyes.** Un servicio maneja un vehículo. Varios servicios pueden compartir caminos, pero no viajan juntos.
- **La pistola de bengalas** como señal: el juego crea su proyectil sólo cuando alguien la dispara con el arma en la mano. El juego tampoco tiene humo azul.

---

## 12. Créditos y licencia

- **Comercio ambulante: FUBAR**, autor de [Traveling Trader Expansion](https://steamcommunity.com/sharedfiles/filedetails/?id=3797172077). La idea del comerciante que recorre el mapa es suya, y el nombre de este mod lo homenajea. Abajo del vehículo, nuestros comerciantes caminan y se frenan frente a los jugadores como sus caminantes, y el archivo de un servicio usa los mismos campos que una ruta de TTE. Las protecciones que impiden reclutar, esposar o atender a nuestros comerciantes están adaptadas del código de TTE, con permiso de FUBAR. ¡Gracias, FUBAR!
- **DayZ Expansion**: los comerciantes, el Market, las zonas, los loadouts, los marcadores y el humo del airdrop. BZ_Trader usa su API y no copia su código.
- **BZ AutoDrive**: el chofer y el manejo.
- BZ_Trader, de **Sonom4n** y **Hiperhipo10** — BrigadaZ.

> **Licencia.** El código de BZ_Trader es abierto (MIT). Las protecciones de los comerciantes están adaptadas de Traveling Trader Expansion, de FUBAR, con su permiso. DayZ Expansion es una dependencia aparte, con su propia licencia: no se puede resubir ni repackear junto con este mod.
