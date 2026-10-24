# BZ_Trader — Traveling_Trader on Wheels

**Steam Workshop:** https://steamcommunity.com/sharedfiles/filedetails/?id=3822271044 · *English: [README.md](README.md)*

Un vehículo recorre una ruta grabada con uno o varios comerciantes de Expansion a bordo. En cada parada
**se bajan, caminan a su puesto, venden y vuelven a subir**, y el vehículo sigue. El vehículo lo maneja
**BZ_AutoDrive**. La venta es la del **Expansion Market**; las protecciones de los comerciantes están adaptadas de **Traveling Trader
Expansion** (FUBAR).

> **Mod de servidor:** los jugadores no lo descargan. Requiere BZ_AutoDrive 1.2.4 o posterior; probado en DayZ 1.30 con Expansion 1.9.79.
> El manual completo está junto a este archivo: `MANUAL_BZ_Trader.es.md` / `MANUAL_BZ_Trader.es.pdf` (inglés: `MANUAL_BZ_Trader.md` / `.pdf`).

## Créditos

- **Comercio ambulante: FUBAR**, autor de [Traveling Trader Expansion](https://steamcommunity.com/sharedfiles/filedetails/?id=3797172077). La idea del comerciante que recorre el mapa es suya, y el nombre de este mod lo homenajea. Abajo del vehículo, nuestros comerciantes caminan y se frenan frente a los jugadores como sus caminantes, y el archivo de un servicio usa los mismos campos que una ruta de TTE. Las protecciones que impiden reclutar, esposar o atender a nuestros comerciantes están adaptadas del código de TTE, con permiso de FUBAR. ¡Gracias, FUBAR!
- **DayZ Expansion** (AI, Market, Navigation): los comerciantes, el comercio, las zonas, los loadouts y los
  marcadores. Se usa su API; no se copia su código (Expansion es CC BY-NC-ND 4.0).
- **BZ_AutoDrive** (Sonom4n, Hiperhipo10): el chofer, el manejo y la receta para sentar a los bots.
- BZ_Trader: Sonom4n y Hiperhipo10.

## Cómo funciona

| | quién |
|---|---|
| manejar, detenerse (HOLD), seguir, desaparecer | BZ_AutoDrive |
| bajar, caminar al puesto, vender, volver y subir | BZ_Trader |
| qué vende cada comerciante (stock y precios) | el archivo de comerciante del Market |

- **Los comerciantes son la tripulación (`Crew`) de la ruta de AutoDrive**: AutoDrive los crea, BZ_Trader engancha a
  cada uno con su archivo del Market (`LoadTrader`) y lo sienta en su plaza.
- **El vehículo no sale hasta que todos los comerciantes vivos están sentados de verdad**, ni al lanzar ni en cada
  parada. Al que no lo está, BZ_Trader lo vuelve a sentar. Si no puede en el tope, relanza la vuelta con tripulación
  nueva: nunca sale con un comerciante a medio subir.
- **Maneja uno de los comerciantes** (la plaza 0: «Quién maneja» en el editor), con la ropa de su ficha. En cada marca
  vende desde el asiento, o baja a un puesto como los demás si la parada lo dice: AutoDrive tiene el vehículo detenido
  (HOLD) y BZ_Trader no sigue hasta que vuelve al volante. En una moto no baja (sin nadie arriba se cae). Un archivo
  viejo con `Driver.IsTrader: 0` (maneja el chofer de la ruta, que no vende) sigue andando, con una línea en el log
  del server.
- **Los comerciantes son siempre invencibles**, arriba y abajo, también el que maneja. La invencibilidad es de
  BZ_Trader (`SetAllowDamage(false)`, repetido cada segundo): los comerciantes de TTE se pueden matar y saquear
  siempre. BZ_Trader no lo usa: en un vehículo que sigue una ruta son un blanco demasiado fácil, incluso en PvP, y el
  servicio se volvería un convoy armado. `Traders[].Invincible` y `CanBeLooted` ya no se leen: si un archivo los trae,
  el log del server lo dice una vez y el editor los saca al guardar.
- **Nadie los recluta, ni los esposa, ni los atiende** (vendas, RCP, pulso, comida, sangre, suero, capucha…): sólo a
  nuestros comerciantes; los fijos de Expansion no cambian. Su facción de eAI es siempre `InvincibleObservers`, que es
  invencible, y Expansion esconde lo que no permite sobre un invencible: con las manos vacías, la única opción sobre un
  comerciante es «Comerciar» (ni «Reclutar» ni «Comprobar pulso» aparecen). Con un objeto en la mano, Expansion
  también esconde dar de comer o de beber, sacar o dar sangre, las inyecciones y el desfibrilador; con una venda en la
  mano, la única opción sigue siendo «Comerciar» (probado en el juego). Si alguna otra acción con un objeto llegara a
  aparecer (suero, capucha, esposas…), al elegirla no pasa nada: esa guarda corre en el server.
- **Los comerciantes van pasivos para eAI**: no buscan blancos, así que no pelean ni se desvían a levantar cosas del
  piso camino a su puesto. Por eso `LootingBehaviour` no tiene efecto en BZ_Trader; queda por compatibilidad con TTE.
- **Las manos vacías, siempre:** lo que un loadout les ponga en las manos va a su ranura (la espalda, `Melee`, o el
  hombro), si no al inventario, y si no entra se borra, con una línea en el log. A nuestros comerciantes, y sólo a
  ellos, eAI no les saca un arma a la mano (los demás bots no cambian).
- **A pie rodean el vehículo:** cada comerciante camina desde donde quedó parado al bajar (la **salida medida**). Si
  bajó adentro del casco (el pasillo de un bus), va derecho a su puesto; si bajó afuera y el puesto queda del otro lado
  (la compuerta de atrás de un APC), camina por la esquina más corta. A la vuelta sube desde un punto **afuera** del
  casco: su salida, si quedó afuera; si su asiento está adentro (el bus), el costado más cercano a la altura del asiento;
  si no, 1 m afuera de su puerta. Dos plazas con la misma puerta suben de a una.
- **Los vehículos de los servicios van siempre cerrados a jugadores:** nadie se sienta (ni al volante ni de pasajero),
  nadie abre las puertas ni toca el inventario; los comerciantes y el chofer suben igual. Es el cierre de vehículos de
  Expansion (viene con su módulo Vehicles, en el Bundle); sin ese módulo, el server igual niega los asientos: el jugador
  puede ver la acción, pero no pasa nada. En un vehículo donde se puede ir parado (la caja de un camión, el techo, el
  interior de un bus), un jugador puede viajar igual: es una limitación del vehículo, y cada admin elige qué vehículo
  usa. Un `PlayersCanBoard` de un archivo viejo se ignora (el log lo dice y el editor lo saca al guardar).
- **Nunca se retira un vehículo con un jugador a bordo** (el fin de la vuelta, `Enable` 0, un servicio relanzado…):
  al que está parado arriba o adentro (en cualquier vehículo: la caja de un camión, el techo, el interior del bus) se
  lo pone en el piso, al costado, y el vehículo se retira. Con un jugador sentado (desde DayZ 1.28 el server no puede
  bajar a nadie de un asiento), el servicio sigue con un vehículo nuevo y el viejo espera frenado y cerrado (nadie más
  sube; bajarse se puede) hasta que se baje; recién vacío se lo retira. Sin mensajes en el chat: lo dice el log del
  server. Al cerrarse el server, un vehículo con un jugador a bordo no se retira (no se guarda: no vuelve).
- **Un jugador en el asiento de un comerciante:** el comerciante no se sienta encima; espera al lado del vehículo (sin
  mensaje al jugador: lo dice el log). Si el jugador no se cambia, la parada espera hasta tres veces su
  `BoardTimeoutS` y después se relanza la vuelta (el vehículo viejo espera, como arriba, a que el jugador se baje).
- **El panel de control de AutoDrive:** **Respawn** (o **RESET** en la tarjeta del runner) es un relanzamiento para
  BZ_Trader: corta lo que estaba en curso (una parada, un saludo, la terminal), retira ese vehículo y relanza la vuelta
  desde la cabecera a los 3 s, con el embarque y las plazas de siempre. **Detener** (en la tarjeta) borra el vehículo:
  BZ_Trader lo ve como perdido y lo relanza a los 60 s. Para apagar un servicio, `Enable` 0 en su archivo (o el
  editor), no **Detener**.
- **Miran hacia donde se pida:** cada puesto puede tener un rumbo (`HeadingDeg`) o un punto al que mirar (`LookAt`);
  al llegar se da vuelta y lo sostiene mientras vende. Si se frena frente a un jugador, lo mira y después vuelve a su rumbo.
- **Las plazas de cada vehículo:** la primera vez que el mod ve una clase, escribe `profiles\BZ_Trader\_plazas\<clase>.json`
  (el casco, dónde está cada plaza, su puerta y, cuando bajan, su salida medida). El editor lo usa para saber cuántas
  plazas tiene, dibujar cada plaza donde está (en «Quién viaja» y en el plano de cada parada), el casco, las puertas y
  las salidas, y avisar si un puesto queda del otro lado del vehículo. Un archivo escrito antes de esta versión no trae
  dónde está cada plaza: el editor la aproxima por su entrada hasta el próximo lanzamiento de ese vehículo, que completa
  el archivo (y conserva las salidas medidas).
- **Paradas:** las marcas 4 de la toma (eventos del `_hdr.json`), y opcionalmente **el gesto de saludo** (F1) de un
  jugador adelante del vehículo. El vehículo **sigue cuando todas las plazas vuelven a estar ocupadas**.
  Toda espera tiene tope. Un saludo vale hasta que el vehículo para, pasa al jugador o se aleja (más de 10 ×
  `Wave.RadiusM`), como mucho `Wave.MemoryS` segundos; el log del server dice por qué se ignoró cada saludo que no lo
  paró (muy lejos, la espera entre saludos, no estaba adelante, una marca justo adelante, venció...).
  Para que los jugadores no lo paren todo el tiempo: después de cualquier parada (marca o saludo) el vehículo recorre
  `Wave.MinGapM` (500 m) sobre la ruta antes de parar por otro saludo, no para por un saludo si la próxima parada
  marcada está así de cerca adelante, y hace como mucho `Wave.MaxPerLap` (3) paradas por saludo por vuelta (vuelve a
  0 cada vez que sale de la cabecera). 0 = sin límite, en las dos.
  Por un saludo, el vehículo empieza a frenar a la distancia que necesita para quedar unos 10 m antes del jugador
  (según su velocidad; con un saludo vigente se mira cuatro veces por segundo); si ya no alcanza, no para encima de él.
  En la parada por saludo nadie baja, pero se abren las puertas de las plazas con comerciante (una vez por puerta
  compartida; en una moto, ninguna) para que vendan desde el asiento, y se cierran antes de seguir. El cierre sigue
  negando subir y el inventario.
- **Bocina al salir** (una opción de cada parada, `Stops[].HornOnDepart`, y de las paradas por saludo,
  `Wave.HornOnDepart`; apagada por defecto): cuando termina la parada, un toque de bocina, y el vehículo sale 2 s
  después. Si en esos 2 s la parada tiene que volver a esperar (un comerciante, un jugador enfrente, una pausa), vuelve
  a tocar antes de salir. Las motos no tienen bocina: salen sin ella.
- **Antes de seguir** (de una parada o de la cabecera), si hay un jugador en el camino de adelante (delante del
  vehículo, con su ancho y hasta 12 m), puede esperar a que se corra, como mucho `FrontPlayerWaitS` segundos, y después
  salir igual. Apagado por defecto (0): el aviso es la bocina al salir.
- **La zona de comercio del servicio** puede seguir al vehículo (`TraderZone`), para que venda con el mismo stock
  en cualquier punto del mapa.
- **Al final de la ruta:** desaparece, reaparece a los N s o empieza de nuevo (lo decide el admin).

## Requisitos

- **BZ_AutoDrive** (1.2.4 o posterior): artículo requerido en el Workshop. Traveling_Trader_Expansion ya no hace falta.
- DayZ Expansion: AI, Market y Navigation, el Bundle o esos módulos sueltos. Probado con el Bundle. Expansion no es un artículo requerido en el Workshop, a propósito: un server que ya usa módulos sueltos de Expansion chocaría con el Bundle.
- **Community Framework (CF)**, que Expansion necesita.
- **`@BZ_Trader` va dentro de la carpeta del server**, al lado de `DayZServer_x64.exe`, como los demás mods del server:
  copiala desde donde la baja Steam al suscribirte. El editor tiene que correr desde ahí: lee el server en el que está.
- En DayZ 1.30, **`-profiles` tiene que ser relativo** (por ejemplo `-profiles=profiles`): con una ruta absoluta el
  mod no puede listar sus archivos de servicio. Usá uno relativo.

## Armar un comerciante, paso a paso

Ejemplo: un comerciante en moto que vende bebidas.

| paso | qué | quién |
|---|---|---|
| 1 | **El comerciante del Market**: `profiles\ExpansionMod\Traders\<Nombre>.json` (por ejemplo, la copia de uno existente con `"Items": {"waterbottle": 1}`). Se arma como para un comerciante fijo. | el admin, en el Market |
| 2 | **La zona del Market** del servicio (`<misión>\expansion\traderzones\BZT_<servicio>.json`), si querés un stock propio que viaje con el vehículo. | el editor la ofrece si no existe |
| 3 | **La reputación de la Hardline**: con `UseReputation: 1`, los artículos con rareza piden reputación. | el admin (el editor avisa) |
| 4 | **Grabar la ruta** (NUMPAD 5 graba; NUMPAD 4 marca cada parada) y **convertirla** con el Wizard de AutoDrive. | AutoDrive |
| 5 | **El chofer y la tripulación en la ruta** (`DriverClass` y `Crew` del `_hdr.json` y del `.json`). | el editor |
| 6 | **El servicio**: `profiles\BZ_Trader\<servicio>.json` (por ejemplo `TRADER_PICKUP_01.json`). | el editor |

### El editor

Está en la carpeta del mod: **`@BZ_Trader\tools\editor\`**, sin instalar nada (no va dentro del PBO). **Abrilo desde el
`@BZ_Trader` del server**, el que está al lado de `DayZServer_x64.exe`, no desde la carpeta del Workshop. Si
`@BZ_Trader` no está en la raíz de un server (sin `DayZServer_x64.exe` ni `mpmissions` al lado), la ventana de
`Editor.bat` y lo de arriba de la página lo dicen, y **Escanear server** no escanea.
- **Lo más fácil: `Editor.bat`** (doble clic). Levanta un servidor chico, solo en esta PC, y abre el editor en el
  navegador ya leyendo **ese** server: nada que elegir, y guarda directo, también dentro de `Program Files` si Windows
  le deja escribir ahí a tu usuario (si no, descarga el archivo y dice adónde va). Su ventana dice de qué server es;
  cerrarla lo termina, y al cerrar el editor en el navegador (todas sus pestañas) la ventana se cierra sola. Una
  recarga de la página (F5) no la cierra.
- **Sin el `.bat`:** abrí **`editor_bz_trader.html`** con doble clic en **Chrome o Edge**.

Arranca en inglés; los botones **EN / ES** de arriba a la derecha lo pasan al español en el acto, y la elección queda
recordada. **Manual** abre el manual en PDF en el idioma del editor. El editor abre siempre limpio: la sesión es el
archivo del servicio en el server ("Abrir un servicio del server").

1. **El server**: el editor sabe de qué server es por su propia carpeta (`<server>\@BZ_Trader\tools\editor`) y lo dice.
   **Escanear server** lo lee entero de una: sus `.bat`, perfiles y misiones, las tomas convertidas y los servicios de
   cada mapa que carga `@BZ_Trader`, resumido en una línea (`server … · 2 mapa(s) (…) · 5 toma(s) · 3 servicio(s)`).
   Con `Editor.bat` lee solo; con el `.html`, la primera vez el navegador pide elegir esa carpeta (la que tiene
   `DayZServer_x64.exe`), y después la recuerda, **una carpeta por server**: el editor de un server nunca carga la
   carpeta de otro.
   En un server que todavía no arrancó con el mod, dice qué falta: arrancar el server una vez con `@BZ_Trader` (el mod
   crea `profiles\BZ_Trader\` con un servicio de ejemplo apagado) y una toma convertida.
   El editor busca solo **el mapa**: el perfil y la misión del `.bat` que carga `@BZ_Trader` (por ejemplo
   `start_server.bat` → Chernarus, `profiles`, `mpmissions\dayzOffline.chernarusplus`), también cuando el `.bat` los
   pone en variables (`set PROFILE=profiles`); las variables que no puede leer (definidas fuera del `.bat`) las nombra,
   y ese mapa se elige a mano. Si el server tiene varios mapas, se cambia con un clic (**Cambiar de mapa**).
   De ahí en más todo es el árbol de siempre (`BZ_AutoDrive`, `BZ_Trader`, `ExpansionMod\Traders`, `Market`,
   `Loadouts` en el perfil; `expansion\traderzones` y `expansion\settings` en la misión), y se muestra qué encontró.
   Nada se escribe a mano (**Avanzado**, cerrado, es para un server armado distinto). **Elegir otra carpeta** es para
   trabajar en otro server a propósito: el editor avisa que no es la suya y no la recuerda.
   **Si tu server está dentro de `Program Files`** (por ejemplo en `steamapps\common`) y abrís el `.html`, el navegador
   no te va a dejar darle permiso de escritura: **Escanear server** lo lee sin escribir (**Leerla sin permiso de
   escritura**) y te descarga los archivos. Con `Editor.bat` eso no pasa.
2. **La toma o el servicio**, cada uno con **una sola elección**:
   - **Nuevo desde una toma convertida**: se elige por su nombre, y el editor carga el `_hdr.json`, el `_wp.csv` y el
     `.json` de la ruta;
   - **Abrir un servicio del server**: uno que ya está en el server; su toma sale de `Route`, sola. El archivo del
     servicio ES la sesión.

   Si falta un archivo, el editor dice cuál y dónde lo buscó. Debajo se ve **Dónde está cada cosa**: la toma, el
   servicio, el archivo de cada comerciante y la zona.
3. **Comerciantes y paradas**:
   - **Punto A**: cómo para (en las marcas, por saludo o las dos), el saludo, las plazas del vehículo, quién maneja
     (uno de los comerciantes) y cada comerciante: clase, ropa (un loadout o prendas sueltas), **qué vende** (un archivo
     del Market: el editor muestra sus categorías y artículos, no los edita; un archivo que no sirve aparece en gris con
     el porqué), facción;
   - **cada marca 4**: quién baja y **dónde vende**: arrastrá cada ficha a su puesto (norte arriba; el vehículo está
     girado como llegó a la marca, con su izquierda y su derecha marcadas) y **hacia dónde mira**: arrastrá la punta de
     su flecha, o escribí los grados. Atajos: **En semicírculo mirando al centro** (radio, arco y centro: la cruz del
     plano) y **Todos mirando a la calle**. El tiempo vendiendo y el tope para volver.
     **La vista de la parada**: la rueda del mouse acerca y aleja, arrastrar el fondo mueve la vista, **Ajustar** encuadra
     todo y **Agrandar** la pone a toda la ventana (Esc vuelve). Con el teclado: flechas 0,5 m, Mayús 2 m, Ctrl 0,1 m;
   - **FIN**: desaparece, reaparece o empieza de nuevo.
4. **Revisar y guardar**: **Guardar todo** escribe el servicio, la tripulación en la ruta y la zona nueva, con respaldo
   de lo que pisa (`_bz_trader_backups\`), y lista cada archivo con su carpeta. La ruta se modifica **como texto**:
   sólo la línea `DriverClass` y el bloque `Crew`. Con la carpeta del server elegida, el editor **no abre ningún cuadro
   de archivos**: todo sale de sus listas y se escribe directo en su carpeta.

**Sin permiso de escritura** (un server dentro de `Program Files`, Firefox o una copia del server) también se elige **la
carpeta**, sólo para leer: **Leerla sin permiso de escritura** (en Firefox, "Leer la carpeta del server"). Nunca se
eligen archivos sueltos, porque el navegador no deja leer los vecinos de un archivo elegido (el `_hdr.json` sin su
`_wp.csv`). Cada archivo que haya que escribir se descarga con su nombre y la carpeta adonde va. El resultado es el
mismo archivo.

### Jugadores cerca: la secuencia fija
Por defecto una parada dura lo mismo haya o no jugadores (`MaxWithPlayersS: 0`): los jugadores cerca nunca demoran la
salida. Con un tope en `MaxWithPlayersS`, la venta (y la espera sentado) se estira hasta ahí mientras haya jugadores
cerca. Mientras vende, el comerciante se frena y mira a un jugador (`PlayerHaltM`); eso no demora la salida, porque el
tiempo de venta corre igual. El botón **Secuencia fija (sin esperar ni frenar por jugadores)** del editor pone
`MaxWithPlayersS: 0` y `PlayerHaltM: 0`: cada parada dura su mínimo y nadie se frena. No cambia "Comerciar" (lo da el
Market por distancia), la seña ni la regla de no salir sin los comerciantes vivos sentados. Se aplica en caliente, en el
próximo lanzamiento del servicio.

### Señales en la parada
Para que una parada se vea de lejos (una parada larga en un mercado armado, por ejemplo), cada parada puede encender
**señales**: humo de colores, una bengala de ruta (`Roadflare`) o fuegos artificiales, todo vanilla. En el editor, el
bloque **Señales** de cada parada: **+ Agregar señal** la pone a 10 m a la derecha del vehículo y se arrastra en el
plano como un puesto; el atajo **3 señales automáticas alrededor de la parada** pone tres (verde, amarilla y violeta)
en triángulo a 15 m de la marca, fuera de la ruta.
- **Automática**: humo del color elegido de día, bengala roja de noche (lo decide el server en cada encendido).
- Se encienden cuando bajan los comerciantes (o cuando para, si no baja nadie), cada una con su retardo, y cada
  `EverySeconds` se renueva **la que se apagó** (nunca dos en el mismo lugar). El humo dura 90 s, la bengala 20 min y
  los fuegos ~1 min (16 tiros, colores al azar). `EverySeconds: 0` = una sola vez.
- Los fuegos artificiales se ven de lejos, como el humo del airdrop.
- Al seguir, todas se apagan y se borran. No quedan guardadas en el mundo, y las acciones de tomar no las ofrecen.
- No hay humo azul en el juego (sale violeta). `flaregun`, de archivos viejos, se lee como `roadflare`.
- **Humo de airdrop** (`airdrop`, pide Expansion Missions, que viene en el Bundle): el humo rojo y la luz de un airdrop
  de Expansion, que se ven de lejos, también de noche (el humo se vio a 744 m): se le manda a cada jugador, no depende
  del alcance de red de los ítems. Sin infectados, sin loot y sin marcador en el mapa: no hay contenedor en el server, sólo la caja del
  cliente, enterrada en el lugar de la señal (ponela en la marca para que quede debajo del vehículo), y nadie la puede
  abrir. No se apaga sola: dura hasta que el vehículo sigue. El que entra durante la parada también la recibe.

### Sin reiniciar el server

- **El servicio** (`profiles\BZ_Trader\*.json`): el server lo relee cada 10 s. Uno nuevo se lanza; uno cambiado se
  aplica en el próximo lanzamiento de ese servicio (nunca con comerciantes abajo); con `Enable: 0` o borrado, el
  servicio se retira al terminar la parada en curso.
- **La ruta** (la tripulación): AutoDrive la relee en cada lanzamiento.
- **Un comerciante del Market nuevo** cuyas categorías ya existen, y **una zona nueva**: BZ_Trader los carga solo,
  sin reiniciar (en prueba: si un jugador no ve "Comerciar", reiniciar). Si el servicio nombra una zona que no existe
  (por ejemplo, un servicio escrito a mano), BZ_Trader la crea en la misión, sin stock: el Market le agrega los
  artículos de cada comerciante al engancharse.
- **Hace falta reiniciar** para una **categoría nueva** del Market, un cambio de precios o de stock máximo, o un
  cambio en un comerciante del Market que ya estaba cargado. El editor lo dice al guardar.

## Configuración: un archivo por servicio en `$profile:BZ_Trader/*.json`

Es el archivo que escribe el editor, y el editor lo vuelve a abrir para ajustarlo. Claves en inglés; donde el
campo es el mismo que en una ruta de TTE, el mismo nombre y formato. Lo que falta toma un valor seguro, y el log
del server dice qué campos salieron del default. Si la carpeta está vacía, el mod escribe un ejemplo apagado.

| campo | qué es | si falta |
|---|---|---|
| `Enable` | 1 = en servicio | 0 |
| `Name` | nombre del servicio (y firma en el log) | el nombre del archivo |
| `Route` | archivo de ruta de AutoDrive, en `$profile:BZ_AutoDrive/` | obligatorio |
| `StopMode` | `marks` · `wave_only` (sólo por saludo) · `marks_and_wave` | `marks` |
| `Wave` | el saludo: `Enabled`, `RadiusM` (la escala: el saludo se anota a menos de 10 × `RadiusM`; con `AheadOnly` 0, un jugador al costado o atrás a menos de `RadiusM` también lo para; adelante, la parada empieza con la distancia de frenado), `AheadOnly`, `CooldownS` (al salir de una parada, cuánto pasa antes de que un saludo lo pueda volver a parar; default 10), `TimeS`, `MemoryS` (el tope: un saludo vale hasta que el vehículo para, pasa al jugador o se aleja, como mucho estos segundos; se reevalúa cada segundo, default 60), `MinGapM` (después de cualquier parada, los metros que recorre sobre la ruta antes de parar por otro saludo; tampoco para por un saludo si la próxima parada marcada está más cerca que eso, o que 50 m; default 500, 0 = sin mínimo), `MaxPerLap` (paradas por saludo por vuelta, como mucho; vuelve a 0 en cada vuelta; default 3, 0 = sin tope), `HornOnDepart` (1 = un toque de bocina, y sale 2 s después; default 0). Cuenta sólo el saludo de un jugador (no el de un bot), y a menos de 10 × `RadiusM` del vehículo | apagado |
| `Driver` | `IsTrader` (1 = maneja uno de los comerciantes: el de `Seat` 0; el editor escribe siempre 1. 0 = el chofer de la ruta, que no vende: archivos viejos, con una línea en el log), `AllCanDrive` (reservado, todavía no disponible) | 0, 1 |
| `TraderZone` | `Name` (zona del Market), `FollowsVehicle`, `RadiusM` | la zona del lugar |
| `Traders[]` | `Seat` (0 = el que maneja), `Name`, `TraderAIClass`, `Loadout` o `Clothing[]`, `TraderFile`, `LootingBehaviour`. `Faction` se ignora: siempre `InvincibleObservers` (el editor la saca al guardar; en la `Crew` de la ruta la escribe así; las rutas del comerciante no usan `guard_stop`, el único que la leería) | — |
| `Stops[]` | por marca 4: `Mark`, `Pos`, `HeadingDeg` (el rumbo de DayZ al llegar), `DistM`, `Mode` (`always`/`wave_only`/`off`), `WaveRadiusM`, `WhoGetsOff[]` (0 = también baja el que maneja), `BoardTimeoutS`, `Traders[{Seat, Waypoints[{WayPointName, Position}], TimeS, MovementSpeed, HeadingDeg, LookAt}]`, `Signals[]`, `HornOnDepart` (1 = un toque de bocina antes de seguir, y sale 2 s después; default 0) | no para |
| `Stops[].Traders[].HeadingDeg` · `LookAt` | hacia dónde mira en su puesto mientras vende: un rumbo de DayZ (0 = norte, en sentido horario) o un punto `[x, z]`; si están los dos, gana `LookAt` | mira hacia donde llegó |
| `Stops[].Signals[]` | las señales de la parada (opcional): `Type` (`auto`/`smoke`/`roadflare`/`fireworks`/`airdrop`), `Color` (`green`/`purple`/`red`/`white`/`yellow`/`blue`/`black`, el del humo; el airdrop es siempre rojo), `Pos` `[x, z]` (la altura, del terreno), `EverySeconds`, `StartDelayS`. Hasta 10 por parada | ninguna · `auto`, `green`, 60, 0 |
| `OnRouteEnd` | `Action` (`despawn`/`respawn`/`restart`), `DelayS`, `WaitS` | `despawn` |
| `StartDelayS` | segundos antes del primer lanzamiento | 0 |
| `Markers` | marcador de mapa (formato `ExpansionMarkers` de TTE): el ícono sigue al vehículo en marcha (salta a la posición nueva cada `MarkerUpdateSeconds`, sin duplicarse ni apagarse) y titila en las paradas; cada actualización manda los settings del mapa a cada jugador (unos 40 bytes más 100 por marcador de servidor del mapa) | sin marcador |
| `PlayerRadiusM` · `MaxWithPlayersS` · `SeatedWaitS` | "jugador cerca", tope de la espera con jugadores (0 = no se estira), espera sentado | 8 · 0 · 20 |
| `PlayerHaltM` | mientras vende, a cuántos metros de un jugador se frena y lo mira (el alto de TTE); 0 = nunca. Al caminar no hay alto | 5 |
| `MaxStallDistanceM` | un puesto o una señal más lejos que esto de su marca se avisa (en el log y en el editor). Para un mercado armado, más | 25 |
| `FrontPlayerWaitS` | antes de seguir, con un jugador en el camino de adelante: cuánto espera que se corra, como mucho; 0 = no espera (el aviso es la bocina al salir, `HornOnDepart`) | 0 |
| `Editor` | lo que el editor necesita para rearmar la sesión (plazas, largo, inicio y fin); el mod lo ignora | — |

## Licencia

El código de BZ_Trader es abierto (MIT). Las protecciones de los comerciantes están adaptadas de Traveling Trader
Expansion, de FUBAR, con su permiso. DayZ Expansion es una dependencia: se suscribe aparte y tiene su propia licencia.
Esta licencia no da ningún derecho sobre ella: no se puede resubir ni repackear junto con este mod.
