from figures import main_panel, beads_panel, signal_flow

FLOW = dict(
    **{"in": "IN", "in_sub": "suma mono", "gate": "GATE", "gate_sub": "normal / sens", "rec_sub": "grupo 1 / 2 / todos",
       "comp_sub": "compresor", "lim": "LIMITADOR", "lim_sub": "tipo cinta", "mem": "MEMORIA 44,7 s",
       "mem_sub": "líneas primas · REVERSE · 1/2 velocidad", "blur_sub": "mezcla de líneas", "filt": "FILTROS",
       "fb_sub": "x supresor", "feedback": "realimentación", "drift_sub": "LFO de paneo + caos", "beads_sub": "posición WET",
       "mix_sub": "seco / memoria", "dry": "señal seca", "loop": "retorno LOOP",
       "caption": "Flujo de señal. Todo lo que se escribe en la memoria pasa por el limitador; la señal seca va directa a MIX. "
                  "La etapa BEADS se coloca a la salida de la memoria (WET) o alimenta el bucle (LOOP, en discontinua)."})

TOC = [
    ("intro", "Bienvenida"), ("history", "Un poco de historia"), ("concept", "Cómo funciona Deriva"),
    ("install", "Instalación"), ("quick", "Primeros pasos"), ("tour", "Recorrido por el panel"),
    ("algorithms", "Los doce algoritmos"), ("controls", "Los controles en detalle"),
    ("memory", "Trabajar con la memoria"), ("beads", "La etapa BEADS"), ("midi", "Control MIDI"),
    ("recipes", "Recetas"), ("technical", "Notas técnicas"), ("cosmos", "Deriva y el COSMOS"),
    ("faq", "Problemas y preguntas frecuentes"), ("credits", "Créditos y licencias"), ("params", "Apéndice: lista de parámetros"),
]

BODY = f"""
<h1 id="intro"><span class="num">01</span> Bienvenida</h1>
<p class="lead">Deriva es una memoria que deriva. Todo lo que tocas se graba en 44,7 segundos de memoria de sonido. La memoria se reparte en líneas de retardo cuyas longitudes son números primos distintos. Como esas longitudes no comparten ningún ritmo común, las repeticiones se deslizan unas sobre otras sin fin: la textura cambia continuamente y nunca se asienta en un bucle.</p>
<p>El plugin sigue el comportamiento del firmware principal del SOMA Laboratory COSMOS, la «drifting memory station» diseñada por Vlad Kreimer. Deriva tiene nombre, código e interfaz propios, y añade lo que un pedal no puede hacer: funciona a cualquier frecuencia de muestreo, sigue el tempo del host, responde a MIDI, puede guardar su memoria dentro del proyecto e incluye una etapa granular opcional construida con el DSP abierto de Mutable Instruments Beads.</p>
<p>Deriva se puede usar de dos maneras:</p>
<ul>
<li><strong>Como instrumento para tocar en tiempo real:</strong> entra una guitarra, un sintetizador, una voz o una mezcla entera, y moldeas la memoria con los controles y los pulsadores mientras tocas encima.</li>
<li><strong>Como efecto de estudio:</strong> colas largas que evolucionan, mantos infinitos, ecos granulares rotos y recuerdos lo-fi para cualquier pista.</li>
</ul>
<div class="tip"><b>En este manual</b>Los capítulos 2 y 3 cuentan la historia y explican la idea del instrumento. Del 4 al 6 lo pones en marcha. Del 7 al 11 se describen todos los controles. El 12 tiene recetas listas para usar. Los últimos capítulos tratan los detalles técnicos, cómo se compara Deriva con el hardware y las licencias.</div>

<h1 id="history"><span class="num">02</span> Un poco de historia</h1>
<h2>Cinta que recuerda</h2>
<p>La idea de Deriva es anterior a cualquier delay digital. A principios de los años sesenta, los compositores que trabajaban con cinta descubrieron que dos magnetófonos de bobina colocados uno junto a otro podían construir música por sí solos. El primero graba, la cinta viaja hasta el cabezal de reproducción del segundo y la señal se devuelve para grabarse otra vez. Cada pasada añade una capa nueva mientras las anteriores se desvanecen poco a poco. Terry Riley y Pauline Oliveros usaron este «Time-Lag Accumulator» en el entorno de la música en cinta de San Francisco. En 1963, Riley pasó un cuarteto de jazz por un bucle así en <em>Music for The Gift</em>.</p>
<h2>Fripp y Eno</h2>
<p>El 8 de septiembre de 1972, en su estudio casero de Londres, Brian Eno montó dos magnetófonos Revox A77 modificados con esta realimentación e invitó a Robert Fripp a tocar la guitarra a través de ellos. El resultado, <em>The Heavenly Music Corporation</em>, abrió el disco <em>(No Pussyfooting)</em>, publicado en noviembre de 1973. Fripp siguió refinando la técnica, más tarde con delays digitales, y acabó conociéndose como <strong>Frippertronics</strong>: capas largas que se apagan despacio, construidas en directo por un solo intérprete.</p>
<h2>SOMA Laboratory y el COSMOS</h2>
<p>SOMA Laboratory es el taller de Vlad Kreimer, conocido por instrumentos «organísmicos» como el sintetizador de drones LYRA-8 y la caja de ritmos PULSAR-23. En 2021, SOMA presentó el COSMOS, un aparato grande de suelo descrito como una estación de memoria a la deriva pensada para trabajar estados meditativos a través de la música. Su manual cita el sistema de Fripp y Eno como inspiración. El COSMOS sustituye los dos magnetófonos por 44,7 segundos de memoria digital, repartida en líneas de retardo con longitudes que son números primos grandes y moduladas por un LFO asíncrono y una señal caótica lenta. Un gran selector de doce posiciones elige entre algoritmos de dos delays, cuatro delays, reverb gigante y delay granular. Los firmwares alternativos añadieron un looper rítmico, transposición y un modo cuadrafónico para dos unidades.</p>
<h2>Mutable Instruments Beads</h2>
<p>Mutable Instruments, la empresa de Émilie Gillet, creó algunos de los módulos Eurorack más influyentes de la década de 2010, todos publicados como código abierto. Su «sintetizador de texturas» granular Clouds se convirtió en un clásico modular. En febrero de 2021, Gillet publicó su sucesor, <strong>Beads</strong>, rediseñado desde cero: sonido más nítido, parámetros con aleatoriedad, cuatro calidades de grabación y un modo delay sincronizable. Beads fue el último módulo que diseñó Gillet, y Mutable Instruments cerró en diciembre de 2022. Su código DSP sigue disponible con licencia MIT, y eso es lo que hace posible la etapa BEADS de Deriva.</p>
<div class="timeline">
<div class="ev"><span class="yr">1963</span>Terry Riley usa un bucle de dos magnetófonos en <em>Music for The Gift</em>.</div>
<div class="ev"><span class="yr">1972</span>Eno y Fripp graban <em>The Heavenly Music Corporation</em> con dos Revox A77.</div>
<div class="ev"><span class="yr">1973</span>Se publica <em>(No Pussyfooting)</em>; nace el «Frippertronics».</div>
<div class="ev"><span class="yr">2021</span>Mutable Instruments publica Beads (febrero). SOMA Laboratory presenta el COSMOS (primavera).</div>
<div class="ev"><span class="yr">2022</span>Cierra Mutable Instruments; su código sigue abierto.</div>
<div class="ev"><span class="yr">2026</span>Deriva reúne la memoria a la deriva y Beads en un solo plugin.</div>
</div>

<h1 id="concept"><span class="num">03</span> Cómo funciona Deriva</h1>
<h2>Una memoria, muchas líneas primas</h2>
<p>Deriva tiene un único bloque de memoria con capacidad para 44,7 segundos de sonido. Cada algoritmo lo divide en dos, cuatro u ocho líneas de retardo. Cada línea se comporta como un bucle de cinta con un solo cabezal, que reproduce lo que hay en la cinta y graba la nueva mezcla en el mismo sitio. Cuando la línea da la vuelta, oyes lo que se grabó una longitud antes.</p>
<p>Todas las longitudes son <strong>números primos de muestras distintos</strong>. Un sonido grabado a la vez en dos líneas vuelve de cada una en momentos ligeramente diferentes. En cada pasada la distancia entre las copias cambia, y como las longitudes no tienen ningún divisor común, el patrón nunca se repite. Eso es la «deriva»: no entra material nuevo y, aun así, la música no deja de cambiar.</p>
<h2>Tres fuentes de movimiento</h2>
<ul>
<li><strong>Desplazamiento primo:</strong> las líneas se deslizan lentamente unas respecto a otras.</li>
<li><strong>DRIFT:</strong> cada línea tiene un LFO asíncrono que la mueve por el campo estéreo. Su velocidad la empuja una señal caótica lenta (un atractor de Lorenz).</li>
<li><strong>BLUR:</strong> mezcla las líneas entre sí, de modo que las copias se funden poco a poco en un cuerpo difuso.</li>
</ul>
<h2>El flujo de señal</h2>
{signal_flow(FLOW)}
<p>La entrada pasa por la ganancia IN, la suma mono opcional, una puerta de ruido y la saturación DRIVE. RECORD decide qué líneas la reciben. El sonido nuevo se suma a la realimentación, pasa por el compresor si está activo, después por el limitador tipo cinta, y se escribe en la memoria. Lo que sale de la memoria se mezcla entre líneas (BLUR), se filtra, se escala con FB y el supresor, y vuelve a entrar. La salida de la memoria se reparte en el estéreo con DRIFT y MIX la mezcla con la señal seca.</p>
<h2>Las órbitas</h2>
<p>Alrededor del selector de algoritmos, Deriva dibuja un anillo por cada línea de retardo. Las barritas muestran lo que hay grabado a lo largo de la línea y el punto brillante es su cabezal. Como las longitudes son primas, los cabezales giran a velocidades ligeramente distintas: puedes ver literalmente cómo se separan. La zona grabada más recientemente de cada anillo brilla en violeta.</p>

<h1 id="install"><span class="num">04</span> Instalación</h1>
<p>Descarga el zip de tu sistema en la página de <strong>Releases</strong> del repositorio DERIVA y copia la carpeta <code>Deriva.vst3</code> en la carpeta VST3 de tu sistema:</p>
<table><tr><th>Sistema</th><th>Carpeta</th><th>Requisitos</th></tr>
<tr><td>Linux</td><td><code>~/.vst3/</code> o <code>/usr/lib/vst3/</code></td><td>x86_64, glibc 2.35 o posterior (Ubuntu 22.04+, Debian 12+, Fedora 35+, Arch), fontconfig y freetype.</td></tr>
<tr><td>Windows</td><td><code>C:\\Program Files\\Common Files\\VST3\\</code></td><td>Windows 10 u 11 de 64 bits. No necesita runtimes adicionales.</td></tr>
<tr><td>macOS</td><td><code>~/Library/Audio/Plug-Ins/VST3/</code></td><td>macOS 10.13 o posterior. Binario universal para Apple Silicon e Intel.</td></tr></table>
<p>Después vuelve a escanear los plugins en tu DAW. En REAPER: <em>Options › Preferences › Plug-ins › VST › Re-scan</em>. Deriva aparece como efecto de <em>Eloy Platas</em> en las categorías Delay y Reverb, con salida estéreo y entrada mono o estéreo. También acepta MIDI (capítulo 11).</p>
<div class="note"><b>macOS</b>El plugin no está notarizado por Apple. Si macOS dice que está dañado o que no se puede abrir, ejecuta esto una vez en el Terminal y vuelve a escanear:<br/><code>xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Deriva.vst3</code></div>

<h1 id="quick"><span class="num">05</span> Primeros pasos</h1>
<ol>
<li><strong>Inserta Deriva</strong> en una pista con guitarra, sintetizador o voz, o en un envío de efectos.</li>
<li><strong>Ajusta IN</strong> para que el LED verde de su lado se mueva con lo que tocas y solo se ponga rojo muy de vez en cuando.</li>
<li><strong>Elige un algoritmo</strong> con el selector grande. El de partida, 2 DELAYS · M, repite cada 9,5 segundos.</li>
<li><strong>Toca una frase.</strong> RECORD viene activado (LEDs rojos). La frase vuelve al cabo de la longitud de la línea y empieza a derivar.</li>
<li><strong>Dale forma.</strong> Sube DRIFT para un movimiento estéreo lento y BLUR para fundir las repeticiones. Deja FB justo por debajo de la marca 1.0, para que las capas más antiguas se apaguen mientras llegan las nuevas.</li>
<li><strong>Toca encima.</strong> Apaga RECORD para hacer un solo sobre la memoria sin añadirle nada, o sigue grabando para construir capas.</li>
<li><strong>Limpia.</strong> Mantén ERASE para borrar bajo los cabezales, o haz Mayús + clic en ERASE (o pulsa el chip ERASE ALL) para borrarlo todo.</li>
</ol>
<div class="tip"><b>Consejo</b>Con FB por encima de 1.0 la memoria crece hasta que el limitador la sujeta, como una cinta que satura. Es un sonido válido, no un fallo: vigila el LED rojo del medidor de nivel.</div>

<h1 id="tour"><span class="num">06</span> Recorrido por el panel</h1>
{main_panel("El panel principal. Los números corresponden a la tabla siguiente.")}
<table><tr><th>#</th><th>Elemento</th><th>Qué hace</th></tr>
<tr><td>1</td><td>IN</td><td>Ganancia de entrada, ±24 dB. El LED muestra el nivel de entrada (verde) y la saturación del conversor (rojo).</td></tr>
<tr><td>2</td><td>BLUR</td><td>Realimentación cruzada entre líneas (delays) o ruido en la posición de los granos (granular).</td></tr>
<tr><td>3</td><td>DRIFT</td><td>Movimiento con LFO caótico: posición estéreo de las líneas o posición de los granos.</td></tr>
<tr><td>4</td><td>DRIVE</td><td>Saturación suave del sonido que entra en la memoria.</td></tr>
<tr><td>5</td><td>Selector y órbitas</td><td>Doce algoritmos en cuatro familias. Los anillos muestran el contenido y el cabezal de cada línea.</td></tr>
<tr><td>6</td><td>SUP | COM</td><td>Izquierda: supresor. Centro: apagado. Derecha: compresor.</td></tr>
<tr><td>7</td><td>FB</td><td>Realimentación de 0 a 1,15, con una marca en la ganancia unidad (1.0).</td></tr>
<tr><td>8</td><td>MIX</td><td>Balance entre señal seca y memoria; en el centro, ambas a nivel completo.</td></tr>
<tr><td>9</td><td>OUT</td><td>Nivel de salida. El LED verde es el piloto de encendido.</td></tr>
<tr><td>10–11</td><td>HPF, LPF</td><td>Filtros dentro del bucle de realimentación, con tres frecuencias cada uno.</td></tr>
<tr><td>12</td><td>Medidor del bucle</td><td>Nivel dentro de la memoria; rojo cuando actúa el limitador.</td></tr>
<tr><td>13</td><td>ERASE</td><td>Mantener para borrar bajo los cabezales; Mayús + clic para borrarlo todo.</td></tr>
<tr><td>14</td><td>REVERSE</td><td>Reproduce la memoria al revés, como dar la vuelta a la cinta.</td></tr>
<tr><td>15</td><td>RECORD</td><td>Grabación sí/no, grupos de destino y media velocidad con pulsación larga.</td></tr>
<tr><td>16</td><td>BEADS</td><td>Abre el panel BEADS y activa la etapa granular.</td></tr>
<tr><td>17</td><td>Chips de opciones</td><td>MONO IN, GATE SENS, 1/2 SPEED, SAVE MEMORY, ERASE ALL.</td></tr>
<tr><td>18</td><td>Rótulo del algoritmo</td><td>Algoritmo actual, número de líneas y longitud de la línea mayor.</td></tr>
</table>
<p>Pasa el ratón por encima de un knob para leer su valor exacto en lugar de su nombre, y detente sobre cualquier control para ver una explicación breve. Doble clic en un knob lo devuelve a su valor por defecto. La ventana se puede redimensionar desde la esquina y mantiene sus proporciones.</p>

<h1 id="algorithms"><span class="num">07</span> Los doce algoritmos</h1>
<p>El selector tiene doce posiciones agrupadas en cuatro familias. Dentro de cada familia, los tres puntos crecen con el tamaño de la memoria. Puedes hacer clic en un punto, arrastrar el puntero o usar la rueda del ratón. Al cambiar de algoritmo la memoria <strong>no se borra</strong>: el nuevo algoritmo recoge el mismo material y empieza a procesarlo a su manera. Un fundido corto de salida y de entrada, más pequeños fundidos en los puntos de empalme, evitan los clics, así que cambiar de algoritmo se convierte en un gesto de interpretación.</p>
<table><tr><th>Pos.</th><th>Algoritmo</th><th>Líneas</th><th>Tamaño</th><th>Carácter</th></tr>
<tr><td>1–3</td><td>2 DELAYS · S / M / L</td><td>2 (L y R)</td><td>2,5 / 9,5 / 22 s</td><td>Frippertronics clásico. Dos líneas cuyas repeticiones se separan, una a cada lado.</td></tr>
<tr><td>4–6</td><td>4 DELAYS · S / M / L</td><td>4</td><td>2,5 / 8,5 / 11,5 s</td><td>Más denso y polirrítmico; cuatro copias que derivan repartidas por el estéreo.</td></tr>
<tr><td>7–9</td><td>REVERB · HUGE / SUPER / INSANE</td><td>8</td><td>línea mayor 1,1 / 2,6 / 5,3 s</td><td>Una sala de kilómetros. Las primeras reflexiones llegan como ecos sueltos y se funden muy despacio en una nube difusa.</td></tr>
<tr><td>10–12</td><td>GRAIN · S / M / L</td><td>2 (+1 central)</td><td>granos de 45 / 140 / 380 ms</td><td>Delay granular estéreo asíncrono. La variante grande añade una línea granular central.</td></tr>
</table>
<div class="two"><div><figure><img src="IMG/dial_reverb.png"/><figcaption>REVERB · SUPER: ocho anillos, uno por línea de la red de realimentación.</figcaption></figure></div>
<div><figure><img src="IMG/dial_grain.png"/><figcaption>GRAIN · M: dos anillos; los granos leen la memoria por detrás de cada cabezal.</figcaption></figure></div></div>
<h2>2 DELAYS</h2>
<p>La línea A graba la entrada izquierda y suena a la izquierda; la línea B graba la derecha y suena a la derecha. B es un 2,7 % más corta que A, así que una frase grabada en las dos vuelve como dos copias que se separan en cada pasada. BLUR hace girar el sonido entre las líneas; con BLUR al máximo, cada línea recibe la mitad de la otra. Admite media velocidad.</p>
<h2>4 DELAYS</h2>
<p>Cuatro líneas (dos por entrada) con longitudes repartidas en un 10 %, paneadas a la izquierda, a la derecha y a medio camino. BLUR las mezcla por parejas y luego entre parejas, hasta que al máximo cada línea oye a todas las demás por igual. Admite media velocidad.</p>
<h2>REVERB</h2>
<p>Una red de retardos realimentados de ocho líneas con matriz de Householder. Esta matriz reparte la energía con suavidad, por lo que las primeras reflexiones se siguen oyendo como ecos individuales. Unos difusores pasa-todo sin pérdidas dentro de cada línea hacen que los ecos se fundan un poco más en cada pasada. FB fija el decaimiento: en 1.0 el espacio se sostiene para siempre. BLUR y DRIFT no actúan, igual que en el hardware. A diferencia del hardware, la cola nunca se desafina.</p>
<h2>GRAIN</h2>
<p>Cada línea se escribe como un delay, pero en lugar de un cabezal de reproducción, se leen granos cortos con ventanas suaves por detrás del cabezal, en momentos irregulares. Cada grano tiene su propia posición estéreo. BLUR añade ruido al punto del que se toman los granos y DRIFT lo mueve con el LFO caótico. La realimentación devuelve el sonido granulado a la memoria, así que cada repetición se vuelve a granular.</p>

<h1 id="controls"><span class="num">08</span> Los controles en detalle</h1>
<h2>Knobs</h2>
<table><tr><th>Control</th><th>Rango</th><th>Descripción</th></tr>
<tr><td>IN</td><td>−24 … +24 dB</td><td>Ganancia de entrada, antes de todo lo demás. Busca el LED en verde, con algún rojo solo en los picos.</td></tr>
<tr><td>BLUR</td><td>0 … 100 %</td><td>Delays: realimentación cruzada ortogonal entre líneas. Conserva la energía, así que FB 1.0 sigue manteniendo el nivel. Granular: variación aleatoria de la posición de los granos. Reverb: sin efecto.</td></tr>
<tr><td>DRIFT</td><td>0 … 100 %</td><td>Delays: profundidad de los LFO de paneo de cada línea (periodos de unos 30–50 s, con velocidades dobladas por un atractor de Lorenz). En 0 las líneas se quedan en su posición de origen. Granular: movimiento de la posición de los granos. Reverb: sin efecto.</td></tr>
<tr><td>DRIVE</td><td>0 … 100 %</td><td>Saturación tanh suave del sonido que entra en la memoria, con antialiasing. No se acumula en cada pasada.</td></tr>
<tr><td>SUP | COM</td><td>SUP 100 % … OFF … COM 100 %</td><td>Mitad izquierda: <strong>supresor</strong>. Cuanto más fuerte tocas, más se baja la memoria existente, de modo que las frases nuevas sustituyen a las antiguas. Mitad derecha: <strong>compresor</strong> sobre lo que se escribe en la memoria, controlado por memoria más entrada. Lo suave crece y lo fuerte se contiene, para una memoria pareja y sostenida.</td></tr>
<tr><td>FB</td><td>0 … 1,15</td><td>Realimentación. Por debajo de 1.0 la memoria se apaga; en 1.0 se mantiene; por encima crece hasta que el limitador tipo cinta la sujeta. Una marca indica el 1.0. Justo por debajo está el punto ideal para superponer capas.</td></tr>
<tr><td>MIX</td><td>seco 100 / 0 … 0 / 100 memoria</td><td>En el centro, seco y memoria a nivel completo, como en el hardware. Al máximo: solo memoria (uso en envío/retorno).</td></tr>
<tr><td>OUT</td><td>−inf … +12 dB</td><td>Nivel de salida final.</td></tr>
</table>
<h2>Pulsadores</h2>
<table><tr><th>Pulsador</th><th>Clic</th><th>Clic derecho / rueda</th><th>Otros</th></tr>
<tr><td>HPF</td><td>Filtro sí/no</td><td>Corte MIN 60 Hz · MED 150 Hz · MAX 400 Hz</td><td>Dentro del bucle: cada pasada pierde más graves.</td></tr>
<tr><td>LPF</td><td>Filtro sí/no</td><td>Corte MIN 1,5 kHz · MED 4 kHz · MAX 9 kHz</td><td>MIN es el más oscuro. Cada pasada se oscurece, como una cinta vieja.</td></tr>
<tr><td>ERASE</td><td>Mantener para borrar</td><td>—</td><td>Mayús + clic: borrar todo. En los delays el borrado se oye al instante; en reverb y granular se nota en la siguiente pasada.</td></tr>
<tr><td>REVERSE</td><td>Sí/no</td><td>—</td><td>Invierte la memoria y todo lo que se grabe después.</td></tr>
<tr><td>RECORD</td><td>Grabación sí/no</td><td>Destino: GROUP 1 · GROUP 2 · ALL</td><td>Mantener 2 s: media velocidad (solo delays).</td></tr>
</table>
<p>Los dos LEDs sobre HPF, LPF y RECORD indican la opción elegida: el izquierdo = la primera, el derecho = la segunda, los dos = la tercera. Brillan con la función activa y se ven tenues con ella apagada. En media velocidad, los LEDs de RECORD parpadean.</p>
<h2>Chips de opciones</h2>
<table><tr><th>Chip</th><th>Función</th></tr>
<tr><td>BEADS</td><td>Abre el panel BEADS debajo y activa la etapa granular (capítulo 10). Vuelve a pulsarlo para cerrarlo y desactivarla.</td></tr>
<tr><td>MONO IN</td><td>Suma las entradas izquierda y derecha y envía el resultado a los dos grupos. Útil para tratar dos fuentes mono como una.</td></tr>
<tr><td>GATE SENS</td><td>Baja 20 dB el umbral de la puerta de ruido (de −60 a −80 dBFS) para no cortar ataques suaves.</td></tr>
<tr><td>1/2 SPEED</td><td>Equivale a la pulsación larga de RECORD.</td></tr>
<tr><td>SAVE MEMORY</td><td>Guarda el contenido de la memoria dentro del proyecto (capítulo 9).</td></tr>
<tr><td>ERASE ALL</td><td>Borra toda la memoria con un fundido corto.</td></tr>
</table>
<h2>Medidores</h2>
<p>Los cinco LEDs bajo el selector muestran el nivel <em>dentro</em> de la memoria: dos verdes (desde −36 y −24 dBFS), dos amarillos (−12 y −6 dBFS) y uno rojo que se enciende cuando el limitador está dando forma al sonido. Aunque el rojo esté siempre encendido, no se rompe nada: oyes una saturación suave tipo cinta, nunca un recorte digital.</p>

<h1 id="memory"><span class="num">09</span> Trabajar con la memoria</h1>
<h2>Grupos de grabación</h2>
<p>La entrada izquierda alimenta el grupo 1 y la derecha el grupo 2. En 2 DELAYS, el grupo 1 es la línea A y el grupo 2 la línea B. En 4 DELAYS, las líneas A y C forman el grupo 1, y B y D el grupo 2. Con clic derecho en RECORD puedes grabar solo en un grupo mientras el otro sigue sonando intacto. Así puedes meter dos fuentes distintas en Deriva y decidir cuál se captura, o conservar media memoria como fondo congelado. Con RECORD apagado, la entrada se sigue oyendo a través de MIX, pero no se graba nada.</p>
<h2>Borrar</h2>
<p>Mantener ERASE escribe silencio bajo los cabezales. En los algoritmos de delays también oyes cómo la memoria se calla al instante; suelta el pulsador para conservar el resto. ERASE ALL lo borra todo con un fundido corto de salida y de entrada.</p>
<h2>Reverse y media velocidad</h2>
<p>REVERSE cambia el sentido de los cabezales: lo grabado suena al revés y lo nuevo también se graba al revés. Si vuelves a invertir, ese material nuevo sonará invertido. La media velocidad (solo delays) reduce a la mitad la reproducción y la grabación: la memoria baja una octava y dura el doble. Lo que se graba a media velocidad vuelve a su tono original al recuperar la velocidad normal. El cambio dura unos 150 ms, como un magnetófono que frena.</p>
<h2>Cambiar de algoritmo</h2>
<p>Todos los algoritmos comparten el mismo bloque de memoria. Al cambiar, la nueva distribución lee lo que ya hay, así que una frase grabada en 2 DELAYS puede convertirse en la materia prima de la reverb gigante o del motor granular. Prueba a cambiar de algoritmo en mitad de una pieza.</p>
<h2>Guardar la memoria</h2>
<p>Con SAVE MEMORY activado, la memoria se guarda dentro del proyecto en 16 bits comprimidos (unos 3–4 MB a 48 kHz) y vuelve al abrirlo. Si el proyecto se abre a otra frecuencia de muestreo, la memoria se remuestrea. Cambiar la frecuencia de muestreo con el plugin en marcha vacía la memoria.</p>

<h1 id="beads"><span class="num">10</span> La etapa BEADS</h1>
<p class="lead">BEADS añade a Deriva el procesador granular de Mutable Instruments Beads, con el código DSP original de Émilie Gillet. Pulsa el chip BEADS de la cabecera: se abre un segundo panel bajo el principal y la etapa se activa con un fundido corto. Pulsa de nuevo BEADS, o CLOSE en el panel, para cerrarlo y dejar la etapa en bypass.</p>
{beads_panel("El panel BEADS. Los números corresponden a la tabla siguiente.")}
<table><tr><th>#</th><th>Control</th><th>Función</th></tr>
<tr><td>1</td><td>Collar de actividad</td><td>Las cuentas se iluminan con el nivel de salida de la etapa.</td></tr>
<tr><td>2</td><td>FREEZE</td><td>Deja de grabar en el buffer de Beads; los granos siguen leyendo el mismo material.</td></tr>
<tr><td>3</td><td>SEED</td><td>Enclavado (LED latiendo): se generan granos continuamente según DENSITY. Pulsar: modo manual, cada pulsación dispara granos. Mantener 1 s: vuelve a enclavar.</td></tr>
<tr><td>4</td><td>TIME</td><td>De dónde se leen los granos en el buffer de 4 segundos, de lo más reciente a lo más antiguo.</td></tr>
<tr><td>5</td><td>SIZE</td><td>Duración del grano. Por debajo de un tercio, los granos suenan al revés. Al máximo, Beads se convierte en un delay (ver más abajo).</td></tr>
<tr><td>6</td><td>SHAPE</td><td>Envolvente del grano, de percusiva a suave e invertida.</td></tr>
<tr><td>7</td><td>PITCH</td><td>Transposición de hasta ±24 semitonos, con paradas en 1, 3, 4, 7, 12 y 24.</td></tr>
<tr><td>8</td><td>RANDOM ×4</td><td>Aleatoriedad de TIME, SIZE, SHAPE y PITCH. A la derecha: un valor nuevo al azar en cada grano. A la izquierda: deriva aleatoria lenta.</td></tr>
<tr><td>9</td><td>DENSITY</td><td>Centro: sin granos (solo SEED). Izquierda: granos aleatorios, más densos cuanto más giras. Derecha: granos regulares, más rápidos cuanto más giras.</td></tr>
<tr><td>10</td><td>FEEDBACK</td><td>Realimentación propia de Beads sobre su buffer.</td></tr>
<tr><td>11</td><td>DRY / WET</td><td>En WET: balance entre la memoria de Deriva y los granos. En LOOP: cuánta realimentación de Deriva pasa por Beads.</td></tr>
<tr><td>12</td><td>REVERB</td><td>Reverb de Beads, tipo placa (topología Griesinger/Dattorro).</td></tr>
<tr><td>13</td><td>QUALITY</td><td>DIRTY TAPE (8 bits µ-law a 24 kHz con wow y flutter), CLEAN TAPE (12 bits, 24 kHz), VINTAGE (12 bits, 32 kHz) o DIGITAL (16 bits, 48 kHz). Al cambiarla, el buffer de Beads se reinicia con un fundido corto.</td></tr>
<tr><td>14</td><td>WET / LOOP</td><td>Dónde se coloca la etapa (ver más abajo).</td></tr>
<tr><td>15</td><td>MONO, SYNC</td><td>MONO: buffer mono con el doble de tiempo. SYNC: las negras del host entran por SEED.</td></tr>
<tr><td>16</td><td>CLOSE</td><td>Cierra el panel y deja la etapa en bypass.</td></tr>
</table>
<h2>WET y LOOP</h2>
<p>En <strong>WET</strong> (por defecto), Beads procesa la salida de la memoria antes de MIX. La memoria de Deriva no se toca; añades una capa granular encima de lo que sale. En <strong>LOOP</strong>, la realimentación de Deriva pasa por Beads, en la proporción que marca DRY/WET, antes de volver a escribirse. Cada repetición se vuelve a granular, transponer o degradar, así que la memoria se transforma en algo nuevo. En LOOP se anulan el FEEDBACK y la REVERB propios de Beads, para que nunca se apilen dos bucles de realimentación.</p>
<h2>Modo delay y SYNC</h2>
<p>Con SIZE al máximo, Beads funciona como un delay en lugar de un granulador. Con SYNC activado y el host en marcha, Beads recibe un pulso en cada negra. Con SEED enclavado, DENSITY elige entonces un divisor y una probabilidad de esos pulsos, y así se obtienen granos o ecos ligados al tempo. Cuando el transporte se para, Beads vuelve a su ritmo interno.</p>
<div class="tip"><b>Latencia</b>Beads funciona siempre a 48 kHz, su frecuencia nativa. A otras frecuencias, Deriva convierte a la entrada y a la salida con un remuestreador de alta calidad. Eso añade unos 5 ms a 48 kHz y unos 30 ms a 44,1 o 96 kHz, solo en el camino de la memoria, donde no se percibe.</div>

<h1 id="midi"><span class="num">11</span> Control MIDI</h1>
<p>Deriva acepta MIDI en cualquier canal, así que una pedalera MIDI puede tocarlo como el pedal. En REAPER, envía una pista MIDI o tu controlador a la pista que contiene Deriva.</p>
<div class="two"><div><h3>Notas (pulsadores)</h3><table><tr><th>Nota</th><th>Función</th></tr>
<tr><td>60</td><td>HPF sí/no</td></tr><tr><td>61</td><td>LPF sí/no</td></tr><tr><td>62</td><td>ERASE (mantenido)</td></tr>
<tr><td>63</td><td>REVERSE</td></tr><tr><td>64</td><td>RECORD sí/no</td></tr><tr><td>65</td><td>ERASE ALL</td></tr>
<tr><td>66</td><td>HALF SPEED</td></tr><tr><td>67</td><td>FREEZE de Beads</td></tr><tr><td>68</td><td>SEED de Beads (mantenido)</td></tr>
<tr><td>69</td><td>BEADS sí/no</td></tr></table></div>
<div><h3>Control change</h3><table><tr><th>CC</th><th>Parámetro</th></tr>
<tr><td>20</td><td>IN</td></tr><tr><td>21</td><td>BLUR</td></tr><tr><td>22</td><td>DRIFT</td></tr><tr><td>23</td><td>DRIVE</td></tr>
<tr><td>24</td><td>SUP | COM</td></tr><tr><td>25</td><td>FB</td></tr><tr><td>26</td><td>MIX</td></tr><tr><td>27</td><td>OUT</td></tr>
<tr><td>28</td><td>Algoritmo</td></tr><tr><td>29–32</td><td>TIME, SIZE, SHAPE, PITCH de Beads</td></tr>
<tr><td>33–36</td><td>DENSITY, FEEDBACK, DRY/WET, REVERB de Beads</td></tr></table></div></div>
<p>Además, todos los parámetros se pueden automatizar desde el host (la lista completa está en el apéndice).</p>

<h1 id="recipes"><span class="num">12</span> Recetas</h1>
<div class="recipe"><h3>Frippertronics</h3><p class="set">2 DELAYS · M · FB justo por debajo de 1.0 · DRIFT 30 % · BLUR 0 · LPF MED activado</p><p>Toca notas largas de guitarra y deja que vuelvan. Cada pasada pierde un poco de agudos, como el sistema de los Revox. Apaga RECORD para hacer un solo sobre el fondo y vuelve a encenderlo para añadir otra capa.</p></div>
<div class="recipe"><h3>Coro a la deriva</h3><p class="set">4 DELAYS · L · FB 0.98 · DRIFT 70 % · BLUR 50 % · SUP|COM: COM 40 %</p><p>Canta o toca pads. El compresor mantiene la memoria pareja y las cuatro líneas vagan por el estéreo.</p></div>
<div class="recipe"><h3>Sala infinita</h3><p class="set">REVERB · INSANE · FB 1.0 · MIX al centro · HPF MIN activado</p><p>Mete sonidos cortos: florecen como ecos sueltos y se convierten poco a poco en una nube quieta. El HPF evita que se acumulen los graves. Usa ERASE ALL para empezar de nuevo.</p></div>
<div class="recipe"><h3>La frase nueva sustituye a la vieja</h3><p class="set">2 DELAYS · S · FB 1.0 · SUP|COM: SUP 70 %</p><p>Cada vez que tocas, el bucle guardado baja y se reemplaza, así que la memoria siempre contiene tu última frase. Funciona muy bien con células rítmicas o melódicas.</p></div>
<div class="recipe"><h3>Memoria que brilla (LOOP)</h3><p class="set">2 DELAYS · M · FB 0.97 · BEADS activado · LOOP · PITCH +12 · SIZE 60 % · DENSITY 70 % · DRY/WET 35 %</p><p>Cada pasada sube una octava en granos mientras el original sigue debajo: un shimmer que asciende despacio. Baja DRY/WET para que la transformación sea más lenta.</p></div>
<div class="recipe"><h3>Casete roto</h3><p class="set">Cualquier delay · BEADS activado · WET · QUALITY DIRTY TAPE · SIZE al máximo (modo delay) · SYNC activado · REVERB 30 %</p><p>Un eco ligado al tempo y deformado, lleno de wow, flutter y grano µ-law, por encima de la memoria a la deriva.</p></div>
<div class="recipe"><h3>Fondo congelado y granos en vivo</h3><p class="set">GRAIN · L · graba una textura · RECORD apagado · BEADS activado · FREEZE · RANDOM de TIME 40 %</p><p>Deriva sostiene el fondo mientras Beads esparce granos congelados sobre él. Toca encima con la señal seca.</p></div>

<h1 id="technical"><span class="num">13</span> Notas técnicas</h1>
<table><tr><th>Elemento</th><th>Implementación</th></tr>
<tr><td>Líneas de retardo</td><td>Un único puntero fraccionario por línea lee y escribe (Rocchesso, <em>Fractionally-addressed delay lines</em>, 2000). A velocidad normal los cabezales están en muestras enteras, así que la reproducción es exacta y no se acumula ningún filtrado.</td></tr>
<tr><td>Interpolación</td><td>Hermite de 4 puntos (Niemitalo) para media velocidad, reverse y granos.</td></tr>
<tr><td>BLUR</td><td>Rotaciones de Givens (ortogonales, conservan la energía). A 45° equivalen a una matriz de Hadamard normalizada (Gerzon; Jot y Chaigne; Smith).</td></tr>
<tr><td>Reverb gigante</td><td>FDN de 8 líneas con matriz de Householder y dos pasa-todo de Schroeder sin pérdidas por línea.</td></tr>
<tr><td>Filtros</td><td>Filtros de variables de estado sin retardo en la realimentación (Zavalishin, <em>The Art of VA Filter Design</em>).</td></tr>
<tr><td>DRIVE</td><td>tanh con antialiasing por antiderivada de primer orden (Parker, Zavalishin y Le Bivic, DAFx-16).</td></tr>
<tr><td>DRIFT</td><td>LFOs con proporciones irracionales (1, √2, φ, √3…) y velocidades moduladas por un atractor de Lorenz lento.</td></tr>
<tr><td>Limitador</td><td>Curva suave estática: lineal hasta −6 dBFS y rodilla tanh hasta un techo de 0 dBFS, dentro del bucle.</td></tr>
<tr><td>BEADS</td><td>Código original de Mutable Instruments a 48 kHz en bloques de 32 muestras; remuestreo con r8brain (banda de transición del 6 %, 120 dB).</td></tr>
</table>
<p><strong>Rendimiento:</strong> entre el 0,4 y el 1,2 % de un núcleo para Deriva y en torno al 1 % para la etapa BEADS (medido en una CPU x86 de sobremesa a 48 kHz). Deriva no añade latencia; la etapa BEADS solo retrasa el camino de la memoria.</p>
<p><strong>Verificación:</strong> cada compilación pasa pruebas offline. Comprueban que FB 1.0 conserva la energía del bucle con ±0,04 dB durante 150 s, que el limitador contiene FB 1,15, que 100 cambios de algoritmo no producen clics, la octava exacta de la media velocidad, los doce algoritmos, ERASE ALL, las frecuencias de muestreo y que el remuestreo es estable entre 44,1 y 96 kHz con cualquier tamaño de bloque. El plugin pasa <em>pluginval</em> en su nivel más exigente.</p>

<h1 id="cosmos"><span class="num">14</span> Deriva y el COSMOS</h1>
<p>Deriva sigue lo que describen el manual y la FAQ de SOMA: la memoria de 44,7 segundos en líneas de longitud prima, las cuatro familias con sus tres tamaños y tiempos documentados, y el comportamiento de cada control en cada algoritmo. También reproduce los cuatro destinos de grabación, el borrado, el reverse, la media velocidad, la puerta de ruido, la suma mono y la memoria que se conserva entre algoritmos.</p>
<p>Lo que SOMA no ha publicado ha habido que estimarlo. Eso incluye los primos exactos, las curvas del limitador y del drive, las frecuencias de los filtros, los tiempos del supresor y del compresor, la topología de la reverb y los detalles del granular. Por eso Deriva se comporta como el COSMOS, pero no es un clon medido y su sonido será distinto, sobre todo en las familias de reverb y granular. Otras diferencias son intencionadas:</p>
<ul>
<li>La reverb nunca se desafina.</li>
<li>Las combinaciones ocultas de pulsadores son chips visibles.</li>
<li>La pulsación larga de RECORD dura 2 s en lugar de 5.</li>
<li>No se incluyen los firmwares rítmico, pitch y cuadrafónico.</li>
<li>Deriva añade lo que el pedal no tiene: la etapa BEADS, la sincronía de tempo, el MIDI y el guardado de la memoria.</li>
</ul>

<h1 id="faq"><span class="num">15</span> Problemas y preguntas frecuentes</h1>
<h3>No oigo nada de la memoria.</h3><p>Comprueba que RECORD está activo (LEDs rojos brillantes), que MIX no está del todo a la izquierda y que FB está por encima de 0. Una entrada muy floja puede quedarse en la puerta de ruido: activa GATE SENS o sube IN.</p>
<h3>La memoria no para de subir y distorsiona.</h3><p>FB está por encima de 1.0 o el compresor la está empujando. Baja FB justo por debajo de la marca 1.0, o usa el supresor.</p>
<h3>La etapa BEADS no suena.</h3><p>Con DENSITY en el centro no hay granos salvo que pulses SEED. Aleja DENSITY del centro, comprueba que SEED está enclavado (LED latiendo) y que DRY/WET no está del todo a la izquierda. Con SYNC activado, los granos necesitan el transporte del host en marcha.</p>
<h3>La memoria desaparece al volver a abrir el proyecto.</h3><p>Activa SAVE MEMORY antes de guardar el proyecto.</p>
<h3>Al cambiar la frecuencia de muestreo se ha borrado la memoria.</h3><p>Es lo esperado: la memoria se reconstruye para la nueva frecuencia.</p>
<h3>macOS dice que el plugin está dañado.</h3><p>Mira la nota del capítulo 4 sobre cómo quitar la marca de cuarentena.</p>

<h1 id="credits"><span class="num">16</span> Créditos y licencias</h1>
<ul>
<li><strong>Deriva</strong> está hecho con JUCE 8. JUCE se ofrece con licencia AGPLv3 o comercial, así que una versión distribuida sin licencia comercial de JUCE debe mantener su código abierto bajo AGPLv3.</li>
<li><strong>DSP de Beads y stmlib:</strong> © Émilie Gillet, Mutable Instruments, licencia MIT. Los cambios locales están en <code>ThirdParty/mi/README-Deriva.txt</code>.</li>
<li><strong>r8brain-free-src:</strong> © Aleksey Vaneev, licencia MIT.</li>
<li><strong>Tipografía Montserrat:</strong> SIL Open Font License.</li>
<li>SOMA Laboratory, COSMOS, Mutable Instruments y Beads son nombres de sus respectivos propietarios y aquí se usan solo como referencia. Deriva no tiene relación con ellos.</li>
</ul>
<p class="small">Referencias: Rocchesso, D. «Fractionally-addressed delay lines», IEEE TSAP 8(6), 2000 · Niemitalo, O. «Polynomial interpolators for high-quality resampling of oversampled audio», 2001 · Jot, J.-M. y Chaigne, A. «Digital delay networks for designing artificial reverberators», AES 1991 · Smith, J. O. <em>Physical Audio Signal Processing</em> · Luff, G. «Let's write a reverb», 2021 · Zavalishin, V. <em>The Art of VA Filter Design</em> 2.1 · Parker, J., Zavalishin, V. y Le Bivic, E. «Reducing the aliasing of nonlinear waveshaping using continuous-time convolution», DAFx-16 · SOMA Laboratory, manual del COSMOS v1.3 y FAQ · Mutable Instruments, código fuente de Beads.</p>

<h1 id="params"><span class="num">A</span> Apéndice: lista de parámetros</h1>
<p>Nombres tal como aparecen en los carriles de automatización del host.</p>
<table><tr><th>Parámetro</th><th>Rango / valores</th><th>Por defecto</th></tr>
<tr><td>Algorithm</td><td>1–12</td><td>2 (2 DELAYS · M)</td></tr>
<tr><td>In</td><td>−24 … +24 dB</td><td>0 dB</td></tr>
<tr><td>Blur / Drift / Drive</td><td>0–100 %</td><td>0 / 30 / 0 %</td></tr>
<tr><td>Sup|Com</td><td>SUP 100 % … OFF … COM 100 %</td><td>OFF</td></tr>
<tr><td>Feedback</td><td>0 … 1,15</td><td>0,97</td></tr>
<tr><td>Mix</td><td>seco/memoria 100/0 … 0/100</td><td>centro</td></tr>
<tr><td>Out</td><td>−inf … +12 dB</td><td>0 dB</td></tr>
<tr><td>HPF, LPF</td><td>OFF / ON</td><td>OFF</td></tr>
<tr><td>HPF Cutoff, LPF Cutoff</td><td>MIN / MED / MAX</td><td>MIN, MED</td></tr>
<tr><td>Record</td><td>OFF / ON</td><td>ON</td></tr>
<tr><td>Record Target</td><td>GROUP 1 / GROUP 2 / ALL</td><td>ALL</td></tr>
<tr><td>Reverse, Half Speed, Erase, Erase All</td><td>OFF / ON</td><td>OFF</td></tr>
<tr><td>Input</td><td>STEREO / MONO SUM</td><td>STEREO</td></tr>
<tr><td>Gate</td><td>NORMAL / SENSITIVE</td><td>NORMAL</td></tr>
<tr><td>Save Memory</td><td>OFF / ON</td><td>OFF</td></tr>
<tr><td>Beads</td><td>OFF / ON</td><td>OFF</td></tr>
<tr><td>Beads Place</td><td>WET / LOOP</td><td>WET</td></tr>
<tr><td>Beads Time / Size / Shape</td><td>0–100 %</td><td>40 / 55 / 50 %</td></tr>
<tr><td>Beads Pitch</td><td>−24 … +24 st</td><td>0 st</td></tr>
<tr><td>Beads … Random (×4)</td><td>−100 … +100 %</td><td>0</td></tr>
<tr><td>Beads Density</td><td>RANDOM … MANUAL … REGULAR</td><td>REGULAR 38 %</td></tr>
<tr><td>Beads Feedback / Dry/Wet / Reverb</td><td>0–100 %</td><td>0 / 60 / 25 %</td></tr>
<tr><td>Beads Freeze, Seed</td><td>OFF / ON</td><td>OFF</td></tr>
<tr><td>Beads Seed Latch</td><td>OFF / ON</td><td>ON</td></tr>
<tr><td>Beads Quality</td><td>DIRTY TAPE / CLEAN TAPE / VINTAGE / DIGITAL</td><td>DIGITAL</td></tr>
<tr><td>Beads Mono, Beads Sync</td><td>OFF / ON</td><td>OFF</td></tr>
</table>
"""

CONTENT = dict(lang="es", cover_title="MANUAL DE USUARIO", cover_meta="VERSIÓN {version} · VST3 · LINUX · WINDOWS · MACOS",
               toc_title="Índice", toc=TOC, body=BODY)
